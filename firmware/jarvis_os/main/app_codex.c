#include "app.h"
#include "codex_client.h"
#include "power.h"
#include "settings.h"
#include "tools_ui.h"
#include "ui_update.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

enum { CODEX_PAGE_USAGE, CODEX_PAGE_RADAR, CODEX_PAGE_ACTIVITY, CODEX_PAGE_BUDDY, CODEX_PAGE_COUNT };

static lv_obj_t *s_root;
static lv_obj_t *s_pages[CODEX_PAGE_COUNT];
static lv_obj_t *s_usage_canvas;
static lv_obj_t *s_window_value[2];
static lv_obj_t *s_window_name[2];
static int s_usage_percent[2] = {-1, -1};
static lv_obj_t *s_usage_today;
static lv_obj_t *s_usage_cost;
static lv_obj_t *s_usage_reset;
static lv_obj_t *s_usage_meta;
static lv_obj_t *s_radar_meta;
static lv_obj_t *s_radar_empty;
static lv_obj_t *s_radar_rows[CODEX_RADAR_MAX][4];
static lv_obj_t *s_activity_state;
static lv_obj_t *s_activity_tasks;
static lv_obj_t *s_activity_bridge;
static lv_obj_t *s_activity_plan;
static lv_obj_t *s_activity_battery;
static lv_obj_t *s_activity_radar;
static lv_obj_t *s_activity_sync;
static lv_obj_t *s_buddy_art;
static lv_obj_t *s_buddy_state;
static lv_obj_t *s_buddy_tasks;
static lv_obj_t *s_page_index;
static lv_obj_t *s_page_control;
static lv_obj_t *s_page_marks[CODEX_PAGE_COUNT];
static unsigned s_page_target_index[CODEX_PAGE_COUNT];
static unsigned s_page;
static bool s_visible;

LV_FONT_DECLARE(font_cn16);
LV_FONT_DECLARE(font_codex_cn16);
typedef struct { const lv_font_t *source; lv_font_t copy; bool ready; } font_fallback_t;
static font_fallback_t s_font_fallbacks[]={
    {&font_tools_19},{&font_tools_20},{&font_tools_21},{&font_tools_22},
    {&font_tools_24},{&font_tools_36},{&font_tools_60},
};
static lv_font_t s_codex_ascii_font;
static bool s_codex_ascii_ready;

static const lv_font_t *codex_ascii_fallback(void) {
    if(!s_codex_ascii_ready){
        s_codex_ascii_font=lv_font_montserrat_14;
        s_codex_ascii_font.fallback=&font_codex_cn16;
        s_codex_ascii_ready=true;
    }
    return &s_codex_ascii_font;
}

static const lv_font_t *codex_font(const lv_font_t *source) {
    if(source==&lv_font_montserrat_14)return codex_ascii_fallback();
    for(size_t i=0;i<sizeof s_font_fallbacks/sizeof s_font_fallbacks[0];++i) {
        font_fallback_t *entry=&s_font_fallbacks[i];
        if(entry->source!=source)continue;
        if(!entry->ready){entry->copy=*source;entry->copy.fallback=codex_ascii_fallback();entry->ready=true;}
        return &entry->copy;
    }
    return source;
}

static lv_obj_t *codex_surface(lv_obj_t *parent, int x, int y, int width, int height) {
    lv_obj_t *surface = tools_surface(parent, x, y, width, height);
    ui_obj_set_gesture_bubble(surface, true);
    return surface;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       int cx, int cy, int width, uint32_t color, int spacing) {
    lv_obj_t *result = tools_label(parent, text, codex_font(font), cx, cy, color, spacing);
    ui_obj_set_gesture_bubble(result, true);
    if (width > 0) {
        lv_obj_set_width(result, width);
        lv_label_set_long_mode(result, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(result, LV_TEXT_ALIGN_CENTER, 0);
        tools_label_center(result, cx, cy);
    }
    return result;
}

static const char *bridge_status(const codex_snapshot_t *snapshot) {
    if(!snapshot->error[0])return tools_text("WAITING FOR BRIDGE","等待连接桥接程序");
    if(!strcmp(snapshot->error,"Wi-Fi unavailable"))return tools_text("WI-FI UNAVAILABLE","设备 Wi-Fi 未连接");
    if(!strcmp(snapshot->error,"Bridge not found"))return tools_text("BRIDGE NOT FOUND","未发现 Bridge");
    if(!strcmp(snapshot->error,"Out of memory"))return tools_text("MEMORY UNAVAILABLE","内存不足");
    if(!strcmp(snapshot->error,"Invalid bridge response"))return tools_text("INVALID BRIDGE DATA","Bridge 数据格式错误");
    if(!strcmp(snapshot->error,"Client unavailable"))return tools_text("CLIENT UNAVAILABLE","客户端启动失败");
    if(!strcmp(snapshot->error,"Bridge request failed"))return tools_text("BRIDGE REQUEST FAILED","Bridge 请求失败");
    return tools_text("BRIDGE OFFLINE","Bridge 离线");
}

static void usage_track(lv_layer_t *layer, int cx, int cy, int width, int height,
                        int percent, uint32_t color) {
    int left=cx-width/2,top=cy-height/2;
    lv_draw_rect_dsc_t track;lv_draw_rect_dsc_init(&track);
    track.bg_color=lv_color_hex(TOOLS_DIM);track.bg_opa=LV_OPA_COVER;track.radius=LV_RADIUS_CIRCLE;
    lv_area_t area={left,top,left+width-1,top+height-1};
    lv_draw_rect(layer,&track,&area);
    if(percent<0)return;
    int filled=width*LV_CLAMP(0,percent,100)/100;
    if(filled<=0)return;
    track.bg_color=lv_color_hex(color);
    track.radius=LV_MIN(height/2,filled/2);
    area.x2=left+filled-1;
    lv_draw_rect(layer,&track,&area);
}

static void usage_draw(lv_event_t *event) {
    lv_area_t area;
    lv_obj_get_coords(lv_event_get_target_obj(event), &area);
    lv_layer_t *layer = lv_event_get_layer(event);
    const int y = area.y1 + 143;
    const int x[2] = {area.x1 + 142, area.x1 + 324};
    for (int i = 0; i < 2; ++i) {
        uint32_t color = s_usage_percent[i] >= 90 ? COL_RED : TOOLS_WHITE;
        usage_track(layer,x[i],y,116,4,s_usage_percent[i],color);
    }
}

static void buddy_draw(lv_event_t *event) {
    lv_area_t area;
    lv_obj_get_coords(lv_event_get_target_obj(event), &area);
    lv_layer_t *layer = lv_event_get_layer(event);
    int cx = area.x1 + lv_area_get_width(&area) / 2;
    int cy = area.y1 + 72;
    tools_circle(layer, cx, cy, 48, 2, TOOLS_LINE);
    tools_circle(layer, cx, cy, 37, 2, TOOLS_GRAY);
    tools_dot(layer, cx - 14, cy - 6, 8, TOOLS_WHITE);
    tools_dot(layer, cx + 14, cy - 6, 8, TOOLS_WHITE);
    tools_line(layer, cx - 12, cy + 16, cx - 4, cy + 21, 3, TOOLS_WHITE);
    tools_line(layer, cx - 4, cy + 21, cx + 5, cy + 21, 3, TOOLS_WHITE);
    tools_line(layer, cx + 5, cy + 21, cx + 12, cy + 16, 3, TOOLS_WHITE);
    tools_dot(layer, cx, cy - 38, 7, COL_RED);
}

static const char *state_title(const char *state) {
    if (!strcmp(state, "running")) return tools_text("WORKING", "正在工作");
    if (!strcmp(state, "waiting")) return tools_text("NEEDS INPUT", "等待输入");
    if (!strcmp(state, "review")) return tools_text("READY FOR REVIEW", "等待审查");
    if (!strcmp(state, "failed")) return tools_text("ACTION FAILED", "任务失败");
    return tools_text("STANDING BY", "待命");
}

static void format_time(int64_t epoch, char *out, size_t capacity) {
    if (epoch <= 0) {
        snprintf(out, capacity, "--:--");
        return;
    }
    time_t timestamp = (time_t)epoch;
    struct tm local;
    if (!localtime_r(&timestamp, &local)) {
        snprintf(out, capacity, "--:--");
        return;
    }
    snprintf(out, capacity, "%02d:%02d", local.tm_hour, local.tm_min);
}

static void format_tokens(int64_t tokens, char *out, size_t capacity) {
    if (tokens < 0) snprintf(out, capacity, "--");
    else if (tokens >= 1000000) snprintf(out, capacity, "%.1fM", tokens / 1000000.0);
    else if (tokens >= 1000) snprintf(out, capacity, "%.1fK", tokens / 1000.0);
    else snprintf(out, capacity, "%lld", (long long)tokens);
}

static void format_reset(int seconds, char *out, size_t capacity) {
    if (seconds < 0) {
        snprintf(out, capacity, "%s", tools_text("5H reset unavailable", "5 小时额度重置时间未知"));
        return;
    }
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    if(settings_lang()) {
        if(hours>0)snprintf(out,capacity,"5 小时后重置  %d 小时 %02d 分",hours,minutes);
        else snprintf(out,capacity,"5 小时后重置  %d 分",minutes);
    } else if (hours > 0) snprintf(out, capacity, "5H reset in %dh %02dm", hours, minutes);
    else snprintf(out, capacity, "5H reset in %dm", minutes);
}

static void update_usage(const codex_snapshot_t *snapshot) {
    char tokens[24], cost[24], reset[48], time_text[12], meta[48];
    lv_label_set_text_fmt(s_window_value[0], snapshot->p5_percent < 0 ? "--" : "%d%%", snapshot->p5_percent);
    lv_label_set_text_fmt(s_window_value[1], snapshot->p7_percent < 0 ? "--" : "%d%%", snapshot->p7_percent);
    s_usage_percent[0] = snapshot->p5_percent;
    s_usage_percent[1] = snapshot->p7_percent;
    format_tokens(snapshot->tokens, tokens, sizeof(tokens));
    if (snapshot->cost_cents < 0) snprintf(cost, sizeof(cost), "--");
    else snprintf(cost, sizeof(cost), "$%lld.%02lld", (long long)(snapshot->cost_cents / 100),
                  (long long)(snapshot->cost_cents % 100));
    format_reset(snapshot->reset_s, reset, sizeof(reset));
    format_time(snapshot->snapshot_ts_s, time_text, sizeof(time_text));
    lv_label_set_text_fmt(s_usage_today, "%s  ·  %s tokens", tools_text("TODAY", "今日"), tokens);
    lv_label_set_text_fmt(s_usage_cost, "%s  %s", tools_text("COST", "费用"), cost);
    lv_label_set_text(s_usage_reset, reset);
    if (!snapshot->has_snapshot) {
        snprintf(meta, sizeof(meta), "%s", bridge_status(snapshot));
    } else if (!snapshot->online) {
        snprintf(meta, sizeof(meta), "%s", snapshot->stale
                 ? tools_text("OFFLINE  ·  CACHED", "离线 · 显示缓存")
                 : tools_text("OFFLINE", "离线"));
    } else if (snapshot->stale) {
        snprintf(meta, sizeof(meta), "%s", tools_text("CACHED  ·  STALE", "缓存数据 · 已过期"));
    } else {
        snprintf(meta, sizeof(meta), "%s  ·  %s", snapshot->plan, time_text);
    }
    lv_label_set_text(s_usage_meta, meta);
    lv_obj_set_style_text_color(s_usage_meta,
        snapshot->online && !snapshot->stale ? lv_color_hex(TOOLS_GRAY) : lv_color_hex(COL_RED), 0);
    lv_obj_invalidate(s_usage_canvas);
}

static void update_radar(const codex_snapshot_t *snapshot) {
    char stamp[12], text[64];
    format_time(snapshot->radar_updated_s, stamp, sizeof(stamp));
    if (!snapshot->has_snapshot) {
        lv_label_set_text(s_radar_meta, bridge_status(snapshot));
        lv_label_set_text(s_radar_empty, tools_text("NO RADAR DATA YET", "暂无 Radar 数据"));
    } else {
        snprintf(text, sizeof(text), "%s  ·  %s%s", stamp,
                 snapshot->radar_stale ? tools_text("STALE", "已过期") : tools_text("LIVE", "实时"),
                 snapshot->stale ? "  ·  CACHED" : "");
        lv_label_set_text(s_radar_meta, text);
        lv_label_set_text(s_radar_empty, snapshot->stale
                          ? tools_text("SHOWING LAST SAVED RADAR", "显示上次缓存的 Radar")
                          : tools_text("NO RADAR ENTRIES", "暂无 Radar 项目"));
    }
    lv_obj_set_style_text_color(s_radar_meta,
        snapshot->radar_stale || snapshot->stale ? lv_color_hex(TOOLS_GRAY) : lv_color_hex(TOOLS_WHITE), 0);
    ui_obj_set_hidden(s_radar_empty, snapshot->radar_count != 0);
    for (size_t i = 0; i < CODEX_RADAR_MAX; ++i) {
        if (i >= snapshot->radar_count) {
            for(int column=0;column<4;++column)lv_label_set_text(s_radar_rows[i][column],"");
            continue;
        }
        lv_label_set_text_fmt(s_radar_rows[i][0],"%02u",(unsigned)i+1);
        lv_label_set_text(s_radar_rows[i][1],snapshot->radar[i].model);
        lv_label_set_text(s_radar_rows[i][2],snapshot->radar[i].epoch);
        lv_label_set_text_fmt(s_radar_rows[i][3],"%d",snapshot->radar[i].iq);
    }
}

static void update_activity(const codex_snapshot_t *snapshot) {
    const char *state = state_title(snapshot->activity_state);
    lv_label_set_text(s_activity_state, state);
    lv_obj_set_style_text_color(s_activity_state,
        !strcmp(snapshot->activity_state, "failed") || !snapshot->online
            ? lv_color_hex(COL_RED) : lv_color_hex(TOOLS_WHITE), 0);
    lv_label_set_text_fmt(s_activity_tasks, "%d  %s", snapshot->active_tasks,
                          tools_text("active task(s)", "个活动任务"));
    lv_label_set_text_fmt(s_activity_bridge, "%s  ·  %s",
        tools_text("BRIDGE", "桥接程序"), snapshot->online ? "ONLINE" : "OFFLINE");
    lv_label_set_text_fmt(s_activity_plan, "%s  ·  %s", tools_text("PLAN", "方案"),
                          snapshot->plan[0] ? snapshot->plan : "--");
    int battery = -1;
    pwr_state_t power_state = PWR_UNKNOWN;
    bool battery_valid = power_read(&battery, &power_state);
    if (battery_valid) lv_label_set_text_fmt(s_activity_battery, "%s  ·  %d%%",
        tools_text("BATTERY", "电量"), LV_CLAMP(0, battery, 100));
    else lv_label_set_text_fmt(s_activity_battery, "%s  ·  --", tools_text("BATTERY", "电量"));
    (void)power_state;

    char stamp[12];
    format_time(snapshot->radar_updated_s, stamp, sizeof(stamp));
    lv_label_set_text_fmt(s_activity_radar, "%s  ·  %u %s", tools_text("RADAR", "Radar"),
                          (unsigned)snapshot->radar_count, tools_text("models", "个模型"));
    lv_label_set_text_fmt(s_activity_sync, "%s  ·  %s%s", tools_text("SYNC", "同步"), stamp,
                          snapshot->stale ? "  ·  STALE" : "");
}

static void update_buddy(const codex_snapshot_t *snapshot) {
    const char *state = state_title(snapshot->activity_state);
    lv_label_set_text(s_buddy_state, state);
    lv_label_set_text_fmt(s_buddy_tasks, "%d  %s", snapshot->active_tasks,
                          tools_text("active task(s)", "个活动任务"));
    lv_obj_set_style_text_color(s_buddy_state,
        !snapshot->online || !strcmp(snapshot->activity_state, "failed")
            ? lv_color_hex(COL_RED) : lv_color_hex(TOOLS_WHITE), 0);
}

static void paint(void) {
    if (!s_root || !s_visible) return;
    codex_snapshot_t snapshot;
    codex_client_snapshot(&snapshot);
    update_usage(&snapshot);
    update_radar(&snapshot);
    update_activity(&snapshot);
    update_buddy(&snapshot);
}

static void page_update(void) {
    if (!s_root) return;
    for (unsigned i = 0; i < CODEX_PAGE_COUNT; ++i) {
        ui_obj_set_hidden(s_pages[i], i != s_page);
        if (s_page_marks[i]) {
            bool active = i == s_page;
            lv_obj_set_size(s_page_marks[i], active ? 20 : 8, 8);
            lv_obj_set_style_bg_color(s_page_marks[i],
                                      lv_color_hex(active ? TOOLS_WHITE : TOOLS_GRAY), 0);
        }
    }
    bool can_next = s_page + 1 < CODEX_PAGE_COUNT;
    lv_label_set_text_fmt(s_page_index, "%u / %u", s_page + 1, CODEX_PAGE_COUNT);
    ui_obj_set_clickable(s_page_control, can_next);
    lv_obj_set_style_opa(s_page_control, LV_OPA_COVER, 0);
    paint();
}

bool app_codex_next_page(void) {
    if (!s_root || s_page + 1 >= CODEX_PAGE_COUNT) return false;
    ++s_page;
    page_update();
    return true;
}

static void page_control_click(lv_event_t *event) {
    (void)event;
    (void)app_codex_next_page();
}

static void page_indicator_click(lv_event_t *event) {
    const unsigned *target = lv_event_get_user_data(event);
    if (!target || *target >= CODEX_PAGE_COUNT || *target == s_page) return;
    s_page = *target;
    page_update();
}

static bool codex_handle_swipe(lv_dir_t direction) {
    if (direction == LV_DIR_LEFT) return app_codex_next_page();
    if (direction == LV_DIR_RIGHT) {
        if (s_page > 0) {
            --s_page;
            page_update();
        } else {
            go_home();
        }
        return true;
    }
    return false;
}

static void codex_gesture(lv_event_t *event) {
    (void)event;
    lv_indev_t *input = lv_indev_active();
    if (input) (void)codex_handle_swipe(lv_indev_get_gesture_dir(input));
}

static void create_usage(lv_obj_t *parent) {
    s_usage_canvas = codex_surface(parent, 0, 0, 466, 326);
    ui_obj_set_event_bubble(s_usage_canvas, true);
    lv_obj_add_event_cb(s_usage_canvas, usage_draw, LV_EVENT_DRAW_MAIN, NULL);
    label(parent, tools_text("USAGE WINDOWS", "用量窗口"), &font_tools_19, 233, 22, 330, TOOLS_GRAY, 2);
    for (int i = 0; i < 2; ++i) {
        int cx = i == 0 ? 142 : 324;
        s_window_name[i] = label(parent,
                                 tools_text(i == 0 ? "5H USED" : "7D USED",
                                            i == 0 ? "5小时已用" : "7天已用"),
                                 &font_tools_19, cx, 64, 124, TOOLS_GRAY, 1);
        s_window_value[i] = label(parent, "--", &font_tools_24, cx, 103, 110, TOOLS_WHITE, 0);
        ui_obj_set_clickable(s_window_value[i], false);
    }
    ui_obj_set_clickable(s_usage_canvas, false);
    s_usage_today = label(parent, "TODAY  ·  -- tokens", &font_tools_20, 233, 194, 370, TOOLS_WHITE, 0);
    s_usage_cost = label(parent, "COST  --", &font_tools_19, 233, 220, 330, TOOLS_GRAY, 0);
    s_usage_reset = label(parent, "5H reset unavailable", &font_tools_19, 233, 250, 360, TOOLS_GRAY, 0);
    s_usage_meta = label(parent, "WAITING FOR BRIDGE", &font_tools_19, 233, 282, 360, TOOLS_GRAY, 1);
}

static void create_radar(lv_obj_t *parent) {
    label(parent, "CODEX RADAR", &font_tools_19, 233, 22, 330, TOOLS_WHITE, 2);
    s_radar_meta = label(parent, "WAITING FOR BRIDGE", &font_tools_19, 233, 43, 360, TOOLS_GRAY, 0);
    s_radar_empty = label(parent, "NO RADAR DATA YET", &font_tools_20, 233, 164, 330, TOOLS_GRAY, 0);
    for (size_t i = 0; i < CODEX_RADAR_MAX; ++i) {
        static const int x[]={80,106,250,333},width[]={22,140,78,44};
        for(int column=0;column<4;++column) {
            lv_obj_t *cell=lv_label_create(parent);s_radar_rows[i][column]=cell;
            lv_obj_set_style_text_font(cell,codex_font(&lv_font_montserrat_14),0);
            uint32_t color=column==1?TOOLS_WHITE:column==3?COL_RED:TOOLS_GRAY;
            lv_obj_set_style_text_color(cell,lv_color_hex(color),0);
            lv_obj_set_style_text_align(cell,column==2?LV_TEXT_ALIGN_CENTER:column==3?LV_TEXT_ALIGN_RIGHT:LV_TEXT_ALIGN_LEFT,0);
            lv_obj_set_width(cell,width[column]);lv_label_set_long_mode(cell,LV_LABEL_LONG_MODE_DOTS);
            lv_obj_set_pos(cell,x[column],54+(int)i*17);ui_obj_set_clickable(cell,false);
            ui_obj_set_gesture_bubble(cell,true);
        }
    }
}

static void create_activity(lv_obj_t *parent) {
    label(parent, "ACTIVITY", &font_tools_19, 233, 22, 330, TOOLS_GRAY, 2);
    s_activity_state = label(parent, "STANDING BY", &font_tools_36, 233, 77, 360, TOOLS_WHITE, 0);
    s_activity_tasks = label(parent, "0 active task(s)", &font_tools_22, 233, 118, 360, TOOLS_GRAY, 0);
    lv_obj_t *separator = codex_surface(parent, 92, 143, 282, 1);
    lv_obj_set_style_bg_color(separator, lv_color_hex(TOOLS_LINE), 0);
    lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
    s_activity_bridge = label(parent, "BRIDGE  ·  OFFLINE", &font_tools_19, 233, 169, 340, TOOLS_WHITE, 1);
    s_activity_plan = label(parent, "PLAN  ·  --", &font_tools_19, 233, 202, 340, TOOLS_GRAY, 0);
    s_activity_battery = label(parent, "BATTERY  ·  --", &font_tools_19, 233, 233, 340, TOOLS_GRAY, 0);
    s_activity_radar = label(parent, "RADAR  ·  0 models", &font_tools_19, 233, 264, 340, TOOLS_GRAY, 0);
    s_activity_sync = label(parent, "SYNC  ·  --:--", &font_tools_19, 233, 295, 340, TOOLS_GRAY, 0);
}

static void create_buddy(lv_obj_t *parent) {
    label(parent, "VIBEBUDDY", &font_tools_19, 233, 22, 330, TOOLS_GRAY, 2);
    s_buddy_art = codex_surface(parent, 143, 22, 180, 145);
    ui_obj_set_clickable(s_buddy_art, false);
    lv_obj_add_event_cb(s_buddy_art, buddy_draw, LV_EVENT_DRAW_MAIN, NULL);
    s_buddy_state = label(parent, "STANDING BY", &font_tools_24, 233, 203, 350, TOOLS_WHITE, 1);
    s_buddy_tasks = label(parent, "0 active task(s)", &font_tools_20, 233, 240, 350, TOOLS_GRAY, 0);
    label(parent, "CODEX COMPANION", &font_tools_19, 233, 282, 350, TOOLS_GRAY, 2);
}

static void create_page_control(lv_obj_t *parent) {
    s_page_control = codex_surface(parent, 161, 318, 144, 56);
    s_page_index = label(s_page_control, "1 / 4", &font_tools_19, 72, 12, 72, TOOLS_WHITE, 0);
    ui_obj_set_clickable(s_page_control, true);
    lv_obj_add_event_cb(s_page_control, page_control_click, LV_EVENT_CLICKED, NULL);

    for (unsigned i = 0; i < CODEX_PAGE_COUNT; ++i) {
        lv_obj_t *target = lv_obj_create(s_page_control);
        lv_obj_remove_style_all(target);
        lv_obj_set_size(target, 32, 24);
        lv_obj_set_pos(target, 8 + (int)i * 32, 28);
        ui_obj_set_scrollable(target, false);
        ui_obj_set_clickable(target, true);
        ui_obj_set_event_bubble(target, false);
        ui_obj_set_gesture_bubble(target, true);
        s_page_target_index[i] = i;
        lv_obj_add_event_cb(target, page_indicator_click, LV_EVENT_CLICKED, &s_page_target_index[i]);

        lv_obj_t *mark = lv_obj_create(target);
        lv_obj_remove_style_all(mark);
        lv_obj_set_size(mark, i == s_page ? 20 : 8, 8);
        lv_obj_align(mark, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(mark, lv_color_hex(i == s_page ? TOOLS_WHITE : TOOLS_GRAY), 0);
        lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(mark, LV_RADIUS_CIRCLE, 0);
        ui_obj_set_scrollable(mark, false);
        ui_obj_set_clickable(mark, false);
        s_page_marks[i] = mark;
    }
}

static void codex_enter(lv_obj_t *parent) {
    launcher_set_title("Codex");
    s_page = CODEX_PAGE_USAGE;
    s_visible = true;
    s_root = codex_surface(parent, 0, 88, 466, 378);
    // Catch horizontal swipes here; children bubble their gestures to this page.
    // The root must not bubble onward or the launcher would consume right-swipe first.
    ui_obj_set_gesture_bubble(s_root, false);
    ui_obj_set_clickable(s_root, true);
    lv_obj_set_style_bg_color(s_root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    for (unsigned i = 0; i < CODEX_PAGE_COUNT; ++i) {
        s_pages[i] = codex_surface(s_root, 0, 0, 466, 326);
    }
    create_usage(s_pages[CODEX_PAGE_USAGE]);
    create_radar(s_pages[CODEX_PAGE_RADAR]);
    create_activity(s_pages[CODEX_PAGE_ACTIVITY]);
    create_buddy(s_pages[CODEX_PAGE_BUDDY]);
    create_page_control(s_root);
    lv_obj_add_event_cb(s_root, codex_gesture, LV_EVENT_GESTURE, NULL);
    codex_client_set_visible(true);
    page_update();
}

static void codex_tick(void) {
    paint();
}

static void codex_visibility(bool visible) {
    s_visible = visible;
    codex_client_set_visible(visible);
    if (visible) paint();
}

static bool codex_back(void) {
    if (s_page == 0) return false;
    --s_page;
    page_update();
    return true;
}

static void codex_exit(void) {
    codex_client_set_visible(false);
    s_visible = false;
    s_root = NULL;
    memset(s_pages, 0, sizeof(s_pages));
    s_usage_canvas = NULL;
    memset(s_window_value, 0, sizeof(s_window_value));
    memset(s_window_name, 0, sizeof(s_window_name));
    memset(s_radar_rows, 0, sizeof(s_radar_rows));
    s_usage_today = s_usage_cost = s_usage_reset = s_usage_meta = NULL;
    s_radar_meta = s_radar_empty = NULL;
    s_activity_state = s_activity_tasks = s_activity_bridge = s_activity_plan = NULL;
    s_activity_battery = s_activity_radar = s_activity_sync = NULL;
    s_buddy_art = s_buddy_state = s_buddy_tasks = NULL;
    s_page_index = s_page_control = NULL;
    memset(s_page_marks, 0, sizeof(s_page_marks));
}

const app_t app_codex = {
    .name = "Codex",
    .color = COL_TXT,
    .enter = codex_enter,
    .tick = codex_tick,
    .exit = codex_exit,
    .back = codex_back,
    .tick_period_ms = 1000,
    .visibility = codex_visibility,
};
