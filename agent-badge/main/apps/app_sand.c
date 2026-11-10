/* Sand — the falling-sand simulation from nurdism/esp_sand_sim, adapted to
 * the Badge as an LVGL canvas app. The physics is the upstream sand_sim.c
 * (12 materials, fire/acid/water reactions); this file owns the canvas
 * texture, the touch brush and a small material palette.
 */
#include "app.h"
#include "port.h"
#include "assets/assets.h"
#include "sand_sim.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAND_PX 466
#define SAND_CELL 4

static lv_obj_t *s_canvas;
static uint16_t *s_pix;
static lv_timer_t *s_timer;
static lv_obj_t *s_btns[13];
static lv_obj_t *s_name;
static lv_obj_t *s_name_btn;
static int s_mat = MAT_SAND;
static int s_last_x = -1, s_last_y = -1;
static uint8_t *s_scene_buf;
static lv_obj_t *s_menu;
static lv_obj_t *s_toast;
static uint32_t s_toast_until;
static volatile bool s_import_busy;
static volatile bool s_alive;
static volatile bool s_import_ready;
static volatile int  s_import_len;
static char s_import_msg[32];

#define SCENE_MAX (2 * GRID_CELLS_MAX + 16)
#define SCENE_SLOT_BYTES 0x8000u          /* two 32 KB slots at the end of "rec" */
#define SCENE_MAGIC 0x53414E44u

/* Base RGB565 colours; shade is applied from the cell's low nibble. */
static const int kMats[12] = {
    MAT_SAND, MAT_WATER, MAT_FIRE, MAT_WOOD, MAT_ROCK, MAT_EMPTY,
    MAT_DIRT, MAT_ACID, MAT_LAVA, MAT_SMOKE, MAT_STEAM, MAT_WALL,
};
static const char *kMatName[12] = {
    "SAND", "WATER", "FIRE", "WOOD", "ROCK", "ERASE",
    "DIRT", "ACID", "LAVA", "SMOKE", "STEAM", "WALL",
};

static const uint16_t MAT565[MAT_COUNT] = {
    0x0000, /* empty */
    0x5A6570, /* smoke */
    0x9AA7B5, /* steam */
    0x7C9A58, /* acid gas */
    0xFF7A1A, /* fire */
    0x2E7DFF, /* water */
    0x9BE34A, /* acid */
    0xFF5A1F, /* lava */
    0xE0C080, /* sand */
    0x9A6B3F, /* dirt */
    0x8B5A2B, /* wood */
    0x70757C, /* rock */
    0x2B3038, /* wall */
};

static uint16_t shade565(uint16_t c, int shade)
{
    int r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
    float f = 0.62f + shade * (0.38f / (SHADE_COUNT - 1));
    r = (int)(r * f); g = (int)(g * f); b = (int)(b * f);
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static void render_grid(void)
{
    const uint8_t *grid = sand_grid();
    for (int y = 0; y < g_grid_h; ++y) {
        int py = g_origin_y + y * g_cell_px;
        for (int x = 0; x < g_grid_w; ++x) {
            uint8_t cell = grid[y * g_grid_w + x];
            int mat = CELL_MAT(cell);
            uint16_t col = (mat == MAT_EMPTY) ? 0x0000 : shade565(MAT565[mat], CELL_SHADE(cell));
            int px = g_origin_x + x * g_cell_px;
            for (int yy = 0; yy < g_cell_px; ++yy) {
                uint16_t *row = s_pix + (size_t)(py + yy) * SAND_PX + px;
                for (int xx = 0; xx < g_cell_px; ++xx) row[xx] = col;
            }
        }
    }
}

static void toast(const char *text)
{
    if (!s_canvas) return;
    lv_obj_t *parent = lv_layer_top();
    if (!s_toast) {
        s_toast = lv_label_create(parent);
        lv_obj_set_style_text_font(s_toast, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(s_toast, lv_color_hex(0xF1F3F5), 0);
        lv_obj_set_style_bg_color(s_toast, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(s_toast, LV_OPA_80, 0);
        lv_obj_set_style_pad_all(s_toast, 6, 0);
        lv_obj_set_style_radius(s_toast, 8, 0);
        lv_obj_align(s_toast, LV_ALIGN_BOTTOM_MID, 0, -54);
    }
    lv_label_set_text(s_toast, text);
    lv_obj_move_foreground(s_toast);
    s_toast_until = lv_tick_get() + 2600;
}

static int scene_encode(void)
{
    if (!s_scene_buf || g_grid_cells <= 0) return 0;
    uint8_t *out = s_scene_buf;
    int n = 0;
    out[n++] = (uint8_t)(g_grid_w & 0xFF);
    out[n++] = (uint8_t)(g_grid_w >> 8);
    out[n++] = (uint8_t)(g_grid_h & 0xFF);
    out[n++] = (uint8_t)(g_grid_h >> 8);
    const uint8_t *grid = sand_grid();
    int i = 0;
    while (i < g_grid_cells) {
        uint8_t cell = grid[i];
        int run = 1;
        while (i + run < g_grid_cells && grid[i + run] == cell && run < 255) ++run;
        if (n + 2 > SCENE_MAX) return 0;
        out[n++] = (uint8_t)run;
        out[n++] = cell;
        i += run;
    }
    return n;
}

static bool scene_decode(const uint8_t *in, int len)
{
    if (!s_pix || len < 4) return false;
    int w = in[0] | (in[1] << 8);
    int h = in[2] | (in[3] << 8);
    if (w != g_grid_w || h != g_grid_h) return false;
    uint8_t *cells = (uint8_t *)s_pix;      /* scratch: canvas is repainted right after */
    int pos = 4, out = 0;
    while (pos + 1 < len && out < g_grid_cells) {
        int run = in[pos++];
        uint8_t cell = in[pos++];
        while (run-- > 0 && out < g_grid_cells) cells[out++] = cell;
    }
    if (out != g_grid_cells) return false;
    sand_load_grid(cells);
    return true;
}

static const esp_partition_t *scene_partition(void)
{
    static const esp_partition_t *p;
    if (!p) p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "rec");
    return p;
}

static uint32_t scene_offset(int slot)
{
    const esp_partition_t *p = scene_partition();
    if (!p || slot < 1 || slot > 2) return 0;
    return p->size - 0x10000u + (uint32_t)(slot - 1) * SCENE_SLOT_BYTES;
}

static void scene_save(int slot)
{
    int n = scene_encode();
    uint32_t off = scene_offset(slot);
    const esp_partition_t *p = scene_partition();
    if (n <= 4 || !p || !off || n > (int)(SCENE_SLOT_BYTES - 8)) {
        ESP_LOGW("sand", "save failed n=%d p=%p off=%lu", n, (void *)p, (unsigned long)off);
        toast("save failed");
        return;
    }
    uint32_t hdr[2] = { SCENE_MAGIC, (uint32_t)n };
    esp_err_t e1 = esp_partition_erase_range(p, off, SCENE_SLOT_BYTES);
    esp_err_t e2 = esp_partition_write(p, off, hdr, sizeof hdr);
    esp_err_t e3 = esp_partition_write(p, off + sizeof hdr, s_scene_buf, (size_t)n);
    toast(e1 == ESP_OK && e2 == ESP_OK && e3 == ESP_OK ? "scene saved" : "save failed");
}

static void scene_load(int slot)
{
    uint32_t off = scene_offset(slot);
    const esp_partition_t *p = scene_partition();
    uint32_t hdr[2] = { 0, 0 };
    if (!p || !off || esp_partition_read(p, off, hdr, sizeof hdr) != ESP_OK ||
        hdr[0] != SCENE_MAGIC || hdr[1] <= 4 || hdr[1] > SCENE_MAX) {
        ESP_LOGW("sand", "load slot %d empty (magic=%08lx len=%lu)", slot,
                 (unsigned long)hdr[0], (unsigned long)hdr[1]);
        toast("no scene");
        return;
    }
    if (esp_partition_read(p, off + sizeof hdr, s_scene_buf, hdr[1]) != ESP_OK ||
        !scene_decode(s_scene_buf, (int)hdr[1])) {
        toast("bad scene");
        return;
    }
    toast("scene loaded");
}

static void sand_import_task(void *arg)
{
    (void)arg;
    int n = port_http_get("http://auto:8790/sand_scene.rle", (char *)s_scene_buf, SCENE_MAX);
    if (s_alive) {
        if (n > 4) {
            s_import_len = n;
            s_import_ready = true;
        } else {
            snprintf(s_import_msg, sizeof s_import_msg, "import failed");
        }
    }
    s_import_busy = false;
    vTaskDelete(NULL);
}

static void set_material(int idx);

static void paint_at(int sx, int sy)
{
    int gx = (sx - g_origin_x) / g_cell_px;
    int gy = (sy - g_origin_y) / g_cell_px;
    if (gx < 0 || gy < 0 || gx >= g_grid_w || gy >= g_grid_h) {
        s_last_x = -1;
        return;
    }
    int radius = TOUCH_DRAW_RADIUS_PX / g_cell_px;
    if (radius < 1) radius = 1;
    if (s_last_x < 0) { s_last_x = gx; s_last_y = gy; }
    sand_paint(s_last_x, s_last_y, gx, gy, radius, s_mat);
    s_last_x = gx;
    s_last_y = gy;
}

static void canvas_touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if (!indev) indev = lv_event_get_indev(e);
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    if (code == LV_EVENT_PRESSED) {
        s_last_x = -1;
        paint_at(p.x, p.y);
    } else if (code == LV_EVENT_PRESSING) {
        paint_at(p.x, p.y);
    } else if (code == LV_EVENT_RELEASED) {
        s_last_x = -1;
    }
}

static void sand_tick(lv_timer_t *t)
{
    (void)t;
    if (!s_canvas || !s_pix) return;

    if (s_import_ready) {
        if (scene_decode(s_scene_buf, s_import_len)) toast("image imported");
        else toast("bad scene");
        s_import_ready = false;
    }
    if (s_import_msg[0]) { toast(s_import_msg); s_import_msg[0] = 0; }
    if (s_toast && (int32_t)(lv_tick_get() - s_toast_until) > 0) {
        lv_obj_delete(s_toast);
        s_toast = NULL;
    }

    float gx = 0.0f, gy = 1.0f;
    float ax, ay;
    if (port_imu_accel(&ax, &ay)) {
        gx = ax / 1000.0f;
        gy = ay / 1000.0f;
    }
    sand_step(gx, gy);

    sand_clear_dirty();
    render_grid();
    lv_obj_invalidate(s_canvas);
}

static void set_material(int idx)
{
    if (idx < 0 || idx >= 12) return;
    s_mat = kMats[idx];
    for (int i = 0; i < 12; ++i) {
        lv_obj_set_style_border_width(s_btns[i], (i == idx) ? 3 : 0, 0);
    }
    if (s_name) lv_label_set_text(s_name, kMatName[idx]);
}

static void select_cb(lv_event_t *e)
{
    set_material((int)(intptr_t)lv_event_get_user_data(e));
}

static lv_color_t colour565(uint16_t c)
{
    return lv_color_make((uint8_t)(((c >> 11) & 0x1F) * 255 / 31),
                         (uint8_t)(((c >> 5) & 0x3F) * 255 / 63),
                         (uint8_t)((c & 0x1F) * 255 / 31));
}

static void clear_cb(lv_event_t *e)
{
    (void)e;
    sand_clear_all();
}

static lv_obj_t *mat_btn(lv_obj_t *root, int x, int idx, int mat)
{
    lv_obj_t *b = lv_button_create(root);
    lv_obj_set_size(b, 34, 34);
    lv_obj_set_pos(b, x, 18);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, colour565(MAT565[mat]), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, lv_color_white(), 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, select_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
    return b;
}

static void menu_close(void)
{
    if (s_menu) {
        lv_obj_delete(s_menu);
        s_menu = NULL;
    }
}

static void save1_cb(lv_event_t *e) { (void)e; scene_save(1); menu_close(); }
static void load1_cb(lv_event_t *e) { (void)e; scene_load(1); menu_close(); }
static void save2_cb(lv_event_t *e) { (void)e; scene_save(2); menu_close(); }
static void load2_cb(lv_event_t *e) { (void)e; scene_load(2); menu_close(); }
static void close_cb(lv_event_t *e) { (void)e; menu_close(); }

static void import_cb(lv_event_t *e)
{
    (void)e;
    if (s_import_busy) { toast("importing..."); return; }
    s_import_busy = true;
    toast("importing image...");
    if (xTaskCreate(sand_import_task, "sand_import", 6144, NULL, 3, NULL) != pdPASS) {
        s_import_busy = false;
        toast("import task failed");
    }
    menu_close();
}

static lv_obj_t *menu_button(lv_obj_t *m, int x, int y, const char *label,
                             lv_event_cb_t cb, uint32_t colour)
{
    lv_obj_t *b = lv_button_create(m);
    lv_obj_set_size(b, 64, 64);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x111417), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(colour), 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, label);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(colour), 0);
    lv_obj_center(l);
    return b;
}

static void menu_open(void)
{
    if (s_menu || !s_canvas) return;
    lv_obj_t *parent = lv_layer_top();
    s_menu = lv_obj_create(parent);
    lv_obj_set_size(s_menu, SAND_PX, SAND_PX);
    lv_obj_set_pos(s_menu, 0, 0);
    lv_obj_set_style_bg_color(s_menu, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_menu, LV_OPA_70, 0);
    lv_obj_set_style_border_width(s_menu, 0, 0);
    lv_obj_set_style_pad_all(s_menu, 0, 0);
    lv_obj_remove_flag(s_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_menu, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *title = lv_label_create(s_menu);
    lv_label_set_text(title, "SAND");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE0C080), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    menu_button(s_menu, 233 - 32, 233 - 125 - 32, "S1", save1_cb, 0x22D3A6);
    menu_button(s_menu, 233 + 108 - 32, 233 - 62 - 32, "L1", load1_cb, 0x7FB0FF);
    menu_button(s_menu, 233 + 108 - 32, 233 + 62 - 32, "S2", save2_cb, 0x22D3A6);
    menu_button(s_menu, 233 - 32, 233 + 125 - 32, "L2", load2_cb, 0x7FB0FF);
    menu_button(s_menu, 233 - 108 - 32, 233 + 62 - 32, "IMG", import_cb, 0xE0C080);
    menu_button(s_menu, 233 - 108 - 32, 233 - 62 - 32, "X", close_cb, 0x8A8D94);

    lv_obj_move_foreground(s_menu);
}

static void menu_btn_cb(lv_event_t *e)
{
    (void)e;
    if (s_menu) menu_close();
    else menu_open();
}

static void sand_build(lv_obj_t *root)
{
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    if (sand_init(SAND_CELL) != ESP_OK) {
        lv_obj_t *l = lv_label_create(root);
        lv_label_set_text(l, "sand: no memory");
        lv_obj_center(l);
        return;
    }

    s_pix = heap_caps_malloc(SAND_PX * SAND_PX * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!s_pix) {
        lv_obj_t *l = lv_label_create(root);
        lv_label_set_text(l, "sand: no PSRAM");
        lv_obj_center(l);
        return;
    }
    memset(s_pix, 0, SAND_PX * SAND_PX * sizeof(uint16_t));
    if (!s_scene_buf) s_scene_buf = heap_caps_malloc(SCENE_MAX, MALLOC_CAP_SPIRAM);
    s_alive = true;
    s_import_ready = false;
    s_import_msg[0] = 0;

    s_canvas = lv_canvas_create(root);
    lv_canvas_set_buffer(s_canvas, s_pix, SAND_PX, SAND_PX, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(s_canvas, 0, 0);
    lv_obj_add_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_canvas, canvas_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_touch_cb, LV_EVENT_RELEASED, NULL);

    for (int i = 0; i < 12; ++i) {
        int col = i % 6, row = i / 6;
        s_btns[i] = mat_btn(root, 128 + col * 42, i, kMats[i]);
        lv_obj_set_pos(s_btns[i], 128 + col * 42, 54 + row * 44);
        lv_obj_move_foreground(s_btns[i]);
    }
    s_btns[12] = lv_button_create(root);
    lv_obj_set_size(s_btns[12], 86, 28);
    lv_obj_set_pos(s_btns[12], 190, 146);
    lv_obj_set_style_radius(s_btns[12], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_btns[12], lv_color_hex(0x1E2A26), 0);
    lv_obj_set_style_bg_opa(s_btns[12], LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_btns[12], 1, 0);
    lv_obj_set_style_border_color(s_btns[12], lv_color_hex(0x22D3A6), 0);
    lv_obj_add_event_cb(s_btns[12], menu_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(s_btns[12]);
    lv_obj_t *cl = lv_label_create(s_btns[12]);
    lv_label_set_text(cl, "MENU");
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(cl, lv_color_hex(0x22D3A6), 0);
    lv_obj_center(cl);

    lv_obj_t *cb = lv_button_create(root);
    lv_obj_set_size(cb, 60, 28);
    lv_obj_set_pos(cb, 286, 146);
    lv_obj_set_style_radius(cb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cb, lv_color_hex(0x3A3F46), 0);
    lv_obj_set_style_bg_opa(cb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cb, 0, 0);
    lv_obj_add_event_cb(cb, clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(cb);
    lv_obj_t *cl2 = lv_label_create(cb);
    lv_label_set_text(cl2, "CLR");
    lv_obj_set_style_text_font(cl2, &lv_font_montserrat_12, 0);
    lv_obj_center(cl2);

    s_name_btn = lv_button_create(root);
    lv_obj_set_size(s_name_btn, 112, 28);
    lv_obj_set_pos(s_name_btn, 177, 12);
    lv_obj_set_style_radius(s_name_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_name_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_name_btn, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_name_btn, 1, 0);
    lv_obj_set_style_border_color(s_name_btn, lv_color_hex(0xE0C080), 0);
    lv_obj_add_event_cb(s_name_btn, menu_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(s_name_btn);
    s_name = lv_label_create(s_name_btn);
    lv_label_set_text(s_name, kMatName[0]);
    lv_obj_set_style_text_font(s_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_name, lv_color_hex(0xF1F3F5), 0);
    lv_obj_align(s_name, LV_ALIGN_CENTER, -8, 0);
    lv_obj_t *mic = lv_label_create(s_name_btn);
    lv_label_set_text(mic, LV_SYMBOL_LIST);
    lv_obj_set_style_text_font(mic, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(mic, lv_color_hex(0xE0C080), 0);
    lv_obj_align(mic, LV_ALIGN_RIGHT_MID, -6, 0);

    lv_obj_t *hint = lv_label_create(root);
    lv_label_set_text(hint, "tap top: menu");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8A8D94), 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 176);

    s_mat = MAT_SAND;
    lv_obj_set_style_border_width(s_btns[0], 3, 0);

    launcher_keep_awake_by(AWAKE_APP, true);
    s_last_x = s_last_y = -1;
    s_timer = lv_timer_create(sand_tick, 30, NULL);
}

static void sand_free(void)
{
    launcher_keep_awake_by(AWAKE_APP, false);
    s_alive = false;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    s_canvas = NULL;
    s_name = NULL;
    s_name_btn = NULL;
    for (int i = 0; i < 13; ++i) s_btns[i] = NULL;
    if (s_pix) { heap_caps_free(s_pix); s_pix = NULL; }
}

static lv_color_t sand_tint(void) { return lv_color_hex(0xE0C080); }

const badge_app_t app_sand = {
    .name = "Sand",
    .art = &app_icon_sand,
    .icon = LV_SYMBOL_TINT,
    .tint = sand_tint,
    .radio = RADIO_OFF,
    .keep_awake = false,
    .enter = sand_build,
    .leave = sand_free,
};
