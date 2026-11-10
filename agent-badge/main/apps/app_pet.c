/* Pet — Claude Desktop Buddy pet (skeleton).
 *
 * The final version will port the 18 buddy characters, the Pet/Info/Approval/
 * Transcript screens and the Claude BLE NUS protocol from
 * vthinkxie/claude-desktop-buddy-esp32 (MIT). This skeleton only proves the
 * app/launcher integration and gives the visual target.
 */
#include "app.h"
#include "port.h"
#include "assets/assets.h"

static lv_obj_t *s_face;
static lv_obj_t *s_status;
static lv_timer_t *s_anim;
static int s_phase;

static lv_color_t pet_tint(void) { return lv_color_hex(0xE8A87C); }

static void anim_cb(lv_timer_t *t)
{
    (void)t;
    s_phase = (s_phase + 1) % 120;
    /* a slow breathing ring / eye blink placeholder */
    int r = 86 + (s_phase < 60 ? s_phase : 120 - s_phase) / 6;
    lv_obj_set_size(s_face, r * 2, r * 2);
    lv_obj_center(s_face);
    if (s_phase == 0) lv_label_set_text(s_status, "idle · Claude BLE not connected");
}

static void pet_build(lv_obj_t *root)
{
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    s_face = lv_obj_create(root);
    lv_obj_remove_style_all(s_face);
    lv_obj_set_size(s_face, 172, 172);
    lv_obj_set_style_radius(s_face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_face, lv_color_hex(0xE8A87C), 0);
    lv_obj_set_style_bg_opa(s_face, LV_OPA_COVER, 0);
    lv_obj_center(s_face);

    /* eyes */
    for (int i = 0; i < 2; ++i) {
        lv_obj_t *eye = lv_obj_create(root);
        lv_obj_remove_style_all(eye);
        lv_obj_set_size(eye, 14, 20);
        lv_obj_set_style_radius(eye, 7, 0);
        lv_obj_set_style_bg_color(eye, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
        lv_obj_align(eye, LV_ALIGN_CENTER, i ? 26 : -26, -18);
    }
    lv_obj_t *mouth = lv_obj_create(root);
    lv_obj_remove_style_all(mouth);
    lv_obj_set_size(mouth, 34, 12);
    lv_obj_set_style_radius(mouth, 6, 0);
    lv_obj_set_style_bg_color(mouth, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(mouth, LV_OPA_COVER, 0);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 24);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Claude Pet");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF1F3F5), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -178);

    s_status = lv_label_create(root);
    lv_label_set_text(s_status, "idle · Claude BLE not connected");
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x8A8D94), 0);
    lv_obj_align(s_status, LV_ALIGN_CENTER, 0, 168);

    s_phase = 0;
    s_anim = lv_timer_create(anim_cb, 50, NULL);
}

static void pet_free(void)
{
    if (s_anim) { lv_timer_delete(s_anim); s_anim = NULL; }
    s_face = s_status = NULL;
}

const badge_app_t app_pet = {
    .name = "Pet",
    .art = &app_icon_pet,
    .icon = LV_SYMBOL_IMAGE,
    .tint = pet_tint,
    .radio = RADIO_OFF,   /* NUS bridge is added with the real port */
    .keep_awake = false,
    .enter = pet_build,
    .leave = pet_free,
};
