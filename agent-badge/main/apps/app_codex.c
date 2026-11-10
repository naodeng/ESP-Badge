/* Codex — live Codex usage, Radar and Status for Agent Badge.
 *
 * Data comes from the macOS bridge (tools/run_codex_bridge.sh) over WiFi:
 *   GET http://<mac>:8790/codex.json
 * The badge joins WiFi, fetches, then leaves the radio off (port_http_get).
 * Pages: Usage rings · Codex Radar (IQ) · Status.
 */
#include "app.h"
#include "port.h"
#include "assets/assets.h"
#include "cJSON.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Change this when the Mac's LAN address changes. */
#define CODEX_BRIDGE_URL "http://auto:8790/codex.json"
#define RADAR_MAX 8
#define TILE_N 3

static lv_obj_t *s_tv;
static lv_obj_t *s_arc5, *s_arc7;
static lv_obj_t *s_p5, *s_p7, *s_tok, *s_cost, *s_reset, *s_status, *s_pet, *s_plan;
static lv_obj_t *s_radar_meta, *s_radar_lbl[RADAR_MAX];
static lv_obj_t *s_st_bridge, *s_st_batt, *s_st_plan, *s_st_radar, *s_st_pet, *s_st_sync;
static lv_timer_t *s_tick = NULL;

static volatile bool s_active = false;
static volatile bool s_want = false;
static bool s_task_started = false;
static volatile int64_t s_last_fetch = 0;

static int  s_p5v = -1, s_p7v = -1;
static long s_reset_s = 0;
static long long s_tok_n = 0;
static int  s_cost_c = 0;
static char s_state[20] = "idle";
static char s_plan_s[16] = "";
static bool s_have = false;
static char s_err[48] = "";
static int  s_synced_at = 0;

static char s_radar_m[RADAR_MAX][28];
static char s_radar_e[RADAR_MAX][12];
static int  s_radar_iq[RADAR_MAX];
static int  s_radar_n = 0;
static int  s_radar_updated = 0;
static bool s_radar_stale = true;

static lv_color_t codex_tint(void) { return lv_color_hex(0x22D3A6); }

static void fmt_tokens(long long n, char *out, size_t cap)
{
    if (n >= 1000000)      snprintf(out, cap, "%.1fM", n / 1000000.0);
    else if (n >= 1000)    snprintf(out, cap, "%.1fk", n / 1000.0);
    else                   snprintf(out, cap, "%lld", n);
}

static void hhmm(int epoch, char *out, size_t cap)
{
    if (epoch <= 0) { snprintf(out, cap, "--"); return; }
    time_t t = (time_t)epoch;
    struct tm ti;
    localtime_r(&t, &ti);
    snprintf(out, cap, "%02d:%02d", ti.tm_hour, ti.tm_min);
}

static void short_model(const char *m, char *out, size_t cap)
{
    const char *dash = strrchr(m, '-');
    snprintf(out, cap, "%s", dash && dash[1] ? dash + 1 : m);
}

static void paint_locked(void)
{
    if (!s_p5) return;

    lv_label_set_text_fmt(s_p5, s_p5v < 0 ? "--" : "%d%%", s_p5v);
    lv_label_set_text_fmt(s_p7, s_p7v < 0 ? "--" : "%d%%", s_p7v);
    lv_arc_set_value(s_arc5, s_p5v < 0 ? 0 : s_p5v);
    lv_arc_set_value(s_arc7, s_p7v < 0 ? 0 : s_p7v);

    char tok[24]; fmt_tokens(s_tok_n, tok, sizeof tok);
    lv_label_set_text_fmt(s_tok, "today %s", tok);
    lv_label_set_text_fmt(s_cost, "$%d.%02d", s_cost_c / 100, s_cost_c % 100);
    lv_label_set_text_fmt(s_plan, "%s", s_plan_s[0] ? s_plan_s : "--");

    if (s_reset_s > 0) {
        long h = s_reset_s / 3600, m = (s_reset_s % 3600) / 60;
        if (h > 0) lv_label_set_text_fmt(s_reset, "reset %ldh%02ldm", h, m);
        else       lv_label_set_text_fmt(s_reset, "reset %ldm", m);
    } else {
        lv_label_set_text(s_reset, "reset --");
    }

    lv_label_set_text_fmt(s_pet, "Codex %s", s_state);
    if (s_err[0]) {
        lv_label_set_text(s_status, s_err);
        lv_obj_set_style_text_color(s_status, lv_color_hex(0xFF7A7A), 0);
    } else if (s_have) {
        lv_label_set_text(s_status, "updated from bridge");
        lv_obj_set_style_text_color(s_status, lv_color_hex(0x5BD48A), 0);
    } else {
        lv_label_set_text(s_status, "waiting for WiFi / bridge");
        lv_obj_set_style_text_color(s_status, lv_color_hex(0x8A8D94), 0);
    }

    /* Radar */
    char stamp[12]; hhmm(s_radar_updated, stamp, sizeof stamp);
    if (s_radar_meta) {
        lv_label_set_text_fmt(s_radar_meta, "IQ · updated %s%s", stamp,
                              s_radar_stale ? " (stale)" : "");
        lv_obj_set_style_text_color(s_radar_meta,
                                    s_radar_stale ? lv_color_hex(0x8A8D94) : lv_color_hex(0x5BD48A), 0);
    }
    for (int i = 0; i < RADAR_MAX; ++i) {
        if (!s_radar_lbl[i]) continue;
        if (i < s_radar_n) {
            char sm[20]; short_model(s_radar_m[i], sm, sizeof sm);
            lv_label_set_text_fmt(s_radar_lbl[i], "%d  %s  %s  %d",
                                  i + 1, sm, s_radar_e[i], s_radar_iq[i]);
        } else {
            lv_label_set_text(s_radar_lbl[i], "");
        }
    }

    /* Status */
    if (s_st_bridge) {
        char sync[12]; hhmm(s_synced_at, sync, sizeof sync);
        int batt = port_battery_percent();
        char batt_s[12]; if (batt >= 0) snprintf(batt_s, sizeof batt_s, "%d%%", batt);
        else snprintf(batt_s, sizeof batt_s, "--");
        lv_label_set_text(s_st_bridge, s_have ? "online" : "offline");
        lv_obj_set_style_text_color(s_st_bridge, s_have ? lv_color_hex(0x5BD48A) : lv_color_hex(0xFF7A7A), 0);
        lv_label_set_text(s_st_batt, batt_s);
        lv_label_set_text_fmt(s_st_plan, "%s", s_plan_s[0] ? s_plan_s : "--");
        lv_label_set_text_fmt(s_st_radar, "%d models%s", s_radar_n, s_radar_stale ? " · stale" : "");
        lv_label_set_text_fmt(s_st_pet, "%s", s_state);
        lv_label_set_text(s_st_sync, sync);
    }
}

static void parse_and_paint(const char *json, int len)
{
    (void)len;
    cJSON *root = cJSON_Parse(json);
    if (!root) { snprintf(s_err, sizeof s_err, "JSON parse error"); return; }

    cJSON *u = cJSON_GetObjectItemCaseSensitive(root, "usage");
    cJSON *p5 = u ? cJSON_GetObjectItemCaseSensitive(u, "p5") : NULL;
    cJSON *p7 = u ? cJSON_GetObjectItemCaseSensitive(u, "p7") : NULL;
    cJSON *rs = u ? cJSON_GetObjectItemCaseSensitive(u, "reset_s") : NULL;
    cJSON *tk = u ? cJSON_GetObjectItemCaseSensitive(u, "tok") : NULL;
    cJSON *ct = u ? cJSON_GetObjectItemCaseSensitive(u, "cost_c") : NULL;
    cJSON *pl = u ? cJSON_GetObjectItemCaseSensitive(u, "plan") : NULL;
    cJSON *pet = cJSON_GetObjectItemCaseSensitive(root, "pet");
    cJSON *ps = pet ? cJSON_GetObjectItemCaseSensitive(pet, "state") : NULL;
    cJSON *radar = cJSON_GetObjectItemCaseSensitive(root, "radar");
    cJSON *rows = radar ? cJSON_GetObjectItemCaseSensitive(radar, "rows") : NULL;
    cJSON *rupd = radar ? cJSON_GetObjectItemCaseSensitive(radar, "updated") : NULL;
    cJSON *rstale = radar ? cJSON_GetObjectItemCaseSensitive(radar, "stale") : NULL;

    port_lock();
    if (!s_active) { port_unlock(); cJSON_Delete(root); return; }
    s_p5v = cJSON_IsNumber(p5) ? p5->valueint : -1;
    s_p7v = cJSON_IsNumber(p7) ? p7->valueint : -1;
    s_reset_s = cJSON_IsNumber(rs) ? (long)rs->valuedouble : 0;
    s_tok_n = cJSON_IsNumber(tk) ? (long long)tk->valuedouble : 0;
    s_cost_c = cJSON_IsNumber(ct) ? ct->valueint : 0;
    if (cJSON_IsString(pl) && pl->valuestring) {
        snprintf(s_plan_s, sizeof s_plan_s, "%s", pl->valuestring);
    }
    if (cJSON_IsString(ps) && ps->valuestring) {
        snprintf(s_state, sizeof s_state, "%s", ps->valuestring);
    }
    s_radar_n = 0;
    cJSON *row = NULL;
    cJSON_ArrayForEach(row, rows) {
        if (s_radar_n >= RADAR_MAX) break;
        cJSON *m = cJSON_GetObjectItemCaseSensitive(row, "m");
        cJSON *e = cJSON_GetObjectItemCaseSensitive(row, "e");
        cJSON *iq = cJSON_GetObjectItemCaseSensitive(row, "iq");
        if (!cJSON_IsString(m) || !cJSON_IsString(e) || !cJSON_IsNumber(iq)) continue;
        snprintf(s_radar_m[s_radar_n], sizeof s_radar_m[0], "%s", m->valuestring);
        snprintf(s_radar_e[s_radar_n], sizeof s_radar_e[0], "%s", e->valuestring);
        s_radar_iq[s_radar_n] = iq->valueint;
        ++s_radar_n;
    }
    s_radar_updated = cJSON_IsNumber(rupd) ? rupd->valueint : 0;
    s_radar_stale = cJSON_IsTrue(rstale);
    s_synced_at = (int)time(NULL);
    s_have = true;
    s_err[0] = '\0';
    paint_locked();
    port_unlock();
    cJSON_Delete(root);
}

static void fetch_task(void *arg)
{
    (void)arg;
    char buf[4096];
    int backoff = 2;
    for (;;) {
        int64_t now = (int64_t)(esp_timer_get_time() / 1000000LL);
        if (s_active && (s_want || now - s_last_fetch > 60)) {
            s_want = false;
            int n = port_http_get(CODEX_BRIDGE_URL, buf, sizeof buf);
            s_last_fetch = (int64_t)(esp_timer_get_time() / 1000000LL);
            if (n > 0) {
                parse_and_paint(buf, n);
                backoff = 2;
            } else if (s_active) {
                port_lock();
                if (!s_have) {
                    snprintf(s_err, sizeof s_err, n == -4 ? "no WiFi on badge" : "bridge unreachable");
                    paint_locked();
                }
                port_unlock();
                s_last_fetch = (int64_t)(esp_timer_get_time() / 1000000LL) - (60 - backoff);
                if (backoff < 30) backoff += 2;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    if (s_reset_s > 0) --s_reset_s;
    if (!s_active || !s_p5) return;
    if (s_reset_s > 0) {
        long h = s_reset_s / 3600, m = (s_reset_s % 3600) / 60;
        if (h > 0) lv_label_set_text_fmt(s_reset, "reset %ldh%02ldm", h, m);
        else       lv_label_set_text_fmt(s_reset, "reset %ldm", m);
    }
}

static lv_obj_t *tile(lv_obj_t *tv, int col, lv_dir_t dir)
{
    lv_obj_t *t = lv_tileview_add_tile(tv, col, 0, dir);
    lv_obj_set_style_bg_color(t, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(t, LV_SCROLLBAR_MODE_OFF);
    return t;
}

static void tile_hint(lv_obj_t *t, const char *text)
{
    lv_obj_t *l = lv_label_create(t);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x5C6068), 0);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -14);
}

static void build_radar(lv_obj_t *t)
{
    lv_obj_t *title = lv_label_create(t);
    lv_label_set_text(title, "Codex Radar");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x7FB0FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 34);

    s_radar_meta = lv_label_create(t);
    lv_label_set_text(s_radar_meta, "IQ · updated --");
    lv_obj_set_style_text_font(s_radar_meta, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_radar_meta, lv_color_hex(0x8A8D94), 0);
    lv_obj_align(s_radar_meta, LV_ALIGN_TOP_MID, 0, 74);

    for (int i = 0; i < RADAR_MAX; ++i) {
        s_radar_lbl[i] = lv_label_create(t);
        lv_label_set_text(s_radar_lbl[i], "");
        lv_obj_set_style_text_font(s_radar_lbl[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(s_radar_lbl[i], lv_color_hex(0xE8ECF0), 0);
        lv_obj_align(s_radar_lbl[i], LV_ALIGN_TOP_LEFT, 40, 100 + i * 38);
    }
    tile_hint(t, "‹ swipe: status");
}

static lv_obj_t *stat_row(lv_obj_t *t, const char *name, int y)
{
    lv_obj_t *n = lv_label_create(t);
    lv_label_set_text(n, name);
    lv_obj_set_style_text_font(n, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(n, lv_color_hex(0x8A8D94), 0);
    lv_obj_align(n, LV_ALIGN_TOP_LEFT, 54, y);
    lv_obj_t *v = lv_label_create(t);
    lv_label_set_text(v, "--");
    lv_obj_set_style_text_font(v, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(0xF1F3F5), 0);
    lv_obj_align(v, LV_ALIGN_TOP_RIGHT, -54, y);
    return v;
}

static void build_status(lv_obj_t *t)
{
    lv_obj_t *title = lv_label_create(t);
    lv_label_set_text(title, "Status");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x22D3A6), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    s_st_bridge = stat_row(t, "Bridge", 100);
    s_st_batt   = stat_row(t, "Battery", 140);
    s_st_plan   = stat_row(t, "Plan", 180);
    s_st_radar  = stat_row(t, "Radar", 220);
    s_st_pet    = stat_row(t, "Codex", 260);
    s_st_sync   = stat_row(t, "Synced", 300);

    lv_obj_t *host = lv_label_create(t);
    lv_label_set_text(host, "auto · LAN discovery · :8790");
    lv_obj_set_style_text_font(host, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(host, lv_color_hex(0x5C6068), 0);
    lv_obj_align(host, LV_ALIGN_TOP_MID, 0, 350);
    tile_hint(t, "swipe back ›");
}

static void codex_build(lv_obj_t *stage)
{
    lv_obj_remove_flag(stage, LV_OBJ_FLAG_SCROLLABLE);

    s_tv = lv_tileview_create(stage);
    lv_obj_set_size(s_tv, 466, 466);
    lv_obj_set_pos(s_tv, 0, 0);
    lv_obj_set_scrollbar_mode(s_tv, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(s_tv, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_tv, LV_OPA_COVER, 0);

    lv_obj_t *t0 = tile(s_tv, 0, LV_DIR_RIGHT);
    lv_obj_t *t1 = tile(s_tv, 1, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    lv_obj_t *t2 = tile(s_tv, 2, LV_DIR_LEFT);

    /* ── page 0: usage (existing layout) ───────────────────── */
    lv_obj_t *root = t0;
    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Codex");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x22D3A6), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -186);

    s_arc5 = lv_arc_create(root);
    lv_obj_set_size(s_arc5, 214, 214);
    lv_obj_align(s_arc5, LV_ALIGN_CENTER, 0, -44);
    lv_arc_set_rotation(s_arc5, 135);
    lv_arc_set_bg_angles(s_arc5, 0, 270);
    lv_arc_set_range(s_arc5, 0, 100);
    lv_obj_remove_style(s_arc5, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_arc5, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc5, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc5, lv_color_hex(0x20242A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc5, lv_color_hex(0x22D3A6), LV_PART_INDICATOR);

    s_arc7 = lv_arc_create(root);
    lv_obj_set_size(s_arc7, 158, 158);
    lv_obj_align(s_arc7, LV_ALIGN_CENTER, 0, -44);
    lv_arc_set_rotation(s_arc7, 135);
    lv_arc_set_bg_angles(s_arc7, 0, 270);
    lv_arc_set_range(s_arc7, 0, 100);
    lv_obj_remove_style(s_arc7, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_arc7, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc7, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc7, lv_color_hex(0x20242A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc7, lv_color_hex(0x7FB0FF), LV_PART_INDICATOR);

    s_p5 = lv_label_create(root);
    lv_label_set_text(s_p5, "--");
    lv_obj_set_style_text_font(s_p5, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_p5, lv_color_hex(0xF1F3F5), 0);
    lv_obj_align(s_p5, LV_ALIGN_CENTER, 0, -64);

    lv_obj_t *l5 = lv_label_create(root);
    lv_label_set_text(l5, "5h window");
    lv_obj_set_style_text_font(l5, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l5, lv_color_hex(0x8A8D94), 0);
    lv_obj_align(l5, LV_ALIGN_CENTER, 0, -18);

    s_p7 = lv_label_create(root);
    lv_label_set_text(s_p7, "--");
    lv_obj_set_style_text_font(s_p7, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_p7, lv_color_hex(0x7FB0FF), 0);
    lv_obj_align(s_p7, LV_ALIGN_CENTER, 106, 24);

    s_reset = lv_label_create(root);
    lv_label_set_text(s_reset, "reset --");
    lv_obj_set_style_text_font(s_reset, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_reset, lv_color_hex(0xB8BCC2), 0);
    lv_obj_align(s_reset, LV_ALIGN_CENTER, 0, 86);

    s_tok = lv_label_create(root);
    lv_label_set_text(s_tok, "today --");
    lv_obj_set_style_text_font(s_tok, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_tok, lv_color_hex(0xB8BCC2), 0);
    lv_obj_align(s_tok, LV_ALIGN_CENTER, -82, 128);

    s_cost = lv_label_create(root);
    lv_label_set_text(s_cost, "$0.00");
    lv_obj_set_style_text_font(s_cost, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_cost, lv_color_hex(0xB8BCC2), 0);
    lv_obj_align(s_cost, LV_ALIGN_CENTER, 82, 128);

    s_pet = lv_label_create(root);
    lv_label_set_text(s_pet, "Codex idle");
    lv_obj_set_style_text_font(s_pet, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_pet, lv_color_hex(0x5BD48A), 0);
    lv_obj_align(s_pet, LV_ALIGN_CENTER, 0, 162);

    s_plan = lv_label_create(root);
    lv_label_set_text(s_plan, "--");
    lv_obj_set_style_text_font(s_plan, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_plan, lv_color_hex(0x6E737A), 0);
    lv_obj_align(s_plan, LV_ALIGN_CENTER, 0, 190);

    s_status = lv_label_create(root);
    lv_label_set_text(s_status, "waiting for WiFi / bridge");
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x8A8D94), 0);
    lv_obj_align(s_status, LV_ALIGN_CENTER, 0, 214);
    tile_hint(root, "swipe: radar ›");

    build_radar(t1);
    build_status(t2);

    launcher_keep_awake_by(AWAKE_APP, true);
    s_active = true;
    s_want = true;
    s_err[0] = '\0';
    paint_locked();

    if (s_tick) { lv_timer_delete(s_tick); s_tick = NULL; }
    s_tick = lv_timer_create(tick_cb, 1000, NULL);

    if (!s_task_started) {
        s_task_started = true;
        xTaskCreate(fetch_task, "codex_fetch", 10240, NULL, 3, NULL);
    }
}

static void codex_free(void)
{
    launcher_keep_awake_by(AWAKE_APP, false);
    s_active = false;
    s_tv = NULL;
    s_p5 = s_p7 = s_tok = s_cost = s_reset = s_status = s_pet = s_plan = NULL;
    s_radar_meta = NULL;
    for (int i = 0; i < RADAR_MAX; ++i) s_radar_lbl[i] = NULL;
    s_st_bridge = s_st_batt = s_st_plan = s_st_radar = s_st_pet = s_st_sync = NULL;
    s_arc5 = s_arc7 = NULL;
    if (s_tick) { lv_timer_delete(s_tick); s_tick = NULL; }
    /* the fetch task stays alive and only acts when s_active is true */
}

const badge_app_t app_codex = {
    .name = "Codex",
    .art = &app_icon_codex,
    .icon = LV_SYMBOL_WIFI,
    .tint = codex_tint,
    .radio = RADIO_WIFI,
    .keep_awake = false,
    .enter = codex_build,
    .leave = codex_free,
};
