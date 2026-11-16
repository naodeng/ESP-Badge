#include "companion_icons.h"
#include "companion_icons.inc"

// 小咪静态入口与 App 共用白团黑眼睛的识别特征。
// The launcher keeps Mimi's white orb and slanted black eyes recognizable.
static void mimi_icon(lv_event_t *e) {
    lv_area_t a; lv_obj_get_coords(lv_event_get_target_obj(e), &a);
    lv_layer_t *layer = lv_event_get_layer(e);
    lv_draw_rect_dsc_t circle; lv_draw_rect_dsc_init(&circle);
    circle.bg_color = lv_color_hex(0xf5f5f2); circle.bg_opa = LV_OPA_COVER;
    circle.radius = LV_RADIUS_CIRCLE;
    lv_area_t bounds = {a.x1+12, a.y1+12, a.x1+135, a.y1+135};
    lv_draw_rect(layer, &circle, &bounds);
    lv_draw_line_dsc_t eye; lv_draw_line_dsc_init(&eye);
    lv_obj_init_draw_line_dsc(lv_event_get_target_obj(e), LV_PART_MAIN, &eye);
    eye.color = lv_color_black(); eye.width = 12; eye.round_start = eye.round_end = true;
    eye.p1 = (lv_point_precise_t){a.x1+81,a.y1+42}; eye.p2 = (lv_point_precise_t){a.x1+87,a.y1+56};
    lv_draw_line(layer, &eye);
    eye.width = 10;
    eye.p1 = (lv_point_precise_t){a.x1+107,a.y1+37}; eye.p2 = (lv_point_precise_t){a.x1+112,a.y1+51};
    lv_draw_line(layer, &eye);
}

// Native travel speed and heading mark.
static void travel_icon(lv_event_t *e) {
    lv_area_t a;lv_obj_get_coords(lv_event_get_target_obj(e),&a);lv_layer_t *layer=lv_event_get_layer(e);
    lv_draw_arc_dsc_t arc;lv_draw_arc_dsc_init(&arc);arc.center=(lv_point_t){a.x1+74,a.y1+75};arc.radius=48;arc.width=7;
    arc.start_angle=215;arc.end_angle=325;arc.rounded=true;arc.color=lv_color_hex(0xf5f5f2);lv_draw_arc(layer,&arc);
    lv_draw_line_dsc_t needle;lv_draw_line_dsc_init(&needle);lv_obj_init_draw_line_dsc(lv_event_get_target_obj(e),LV_PART_MAIN,&needle);
    needle.width=9;needle.round_start=needle.round_end=true;needle.color=lv_color_hex(0xd1283a);
    needle.p1=(lv_point_precise_t){a.x1+74,a.y1+80};needle.p2=(lv_point_precise_t){a.x1+104,a.y1+47};lv_draw_line(layer,&needle);
    lv_draw_rect_dsc_t hub;lv_draw_rect_dsc_init(&hub);hub.bg_color=lv_color_hex(0xf5f5f2);hub.bg_opa=LV_OPA_COVER;hub.radius=LV_RADIUS_CIRCLE;
    lv_area_t dot={a.x1+69,a.y1+75,a.x1+79,a.y1+85};lv_draw_rect(layer,&hub,&dot);
}

// Native monochrome Codex mark with the system's single red signal point.
static void codex_icon(lv_event_t *e) {
    lv_area_t a; lv_obj_get_coords(lv_event_get_target_obj(e), &a);
    lv_layer_t *layer = lv_event_get_layer(e);
    int cx = a.x1 + 74, cy = a.y1 + 74;
    lv_draw_arc_dsc_t orbit; lv_draw_arc_dsc_init(&orbit);
    orbit.center = (lv_point_t){cx, cy}; orbit.radius = 48; orbit.width = 5;
    orbit.start_angle = 22; orbit.end_angle = 338; orbit.rounded = true;
    orbit.color = lv_color_hex(0xf5f5f2); lv_draw_arc(layer, &orbit);
    lv_draw_line_dsc_t signal; lv_draw_line_dsc_init(&signal);
    lv_obj_init_draw_line_dsc(lv_event_get_target_obj(e), LV_PART_MAIN, &signal);
    signal.width = 10; signal.color = lv_color_hex(0xf5f5f2);
    signal.round_start = signal.round_end = true;
    signal.p1 = (lv_point_precise_t){a.x1+43,a.y1+95};
    signal.p2 = (lv_point_precise_t){a.x1+103,a.y1+48};
    lv_draw_line(layer, &signal);
    lv_draw_rect_dsc_t marker; lv_draw_rect_dsc_init(&marker);
    marker.bg_color = lv_color_hex(0xd1283a); marker.bg_opa = LV_OPA_COVER; marker.radius = LV_RADIUS_CIRCLE;
    lv_area_t mark = {cx+24,cy-31,cx+34,cy-21}; lv_draw_rect(layer,&marker,&mark);
}

void companion_launcher_icon_set(lv_obj_t *icon, int index) {
    if (!icon || index < 0 || index >= LAUNCHER_ICON_COUNT + COMPANION_ICON_COUNT) return;
    lv_obj_t *original=lv_obj_get_child(icon,0), *image=lv_obj_get_child(icon,1);
    lv_obj_t *mimi=lv_obj_get_child(icon,2);
    bool is_mimi=index==COMPANION_ICON_MIMI;
    bool is_codex=index==COMPANION_ICON_CODEX;
    bool is_travel=index==COMPANION_ICON_TRAVEL;
    bool added=index>=LAUNCHER_ICON_COUNT;
    lv_obj_t *codex=lv_obj_get_child(icon,3), *travel=lv_obj_get_child(icon,4);
    ui_obj_set_hidden(original,added); ui_obj_set_hidden(image,!added||is_mimi||is_codex||is_travel);
    ui_obj_set_hidden(mimi,!is_mimi);
    ui_obj_set_hidden(codex,!is_codex);
    ui_obj_set_hidden(travel,!is_travel);
    if (!added) launcher_icon_set(original,(launcher_icon_t)index);
    else if(!is_mimi && !is_codex && !is_travel) {
        const lv_image_dsc_t *const sources[]={&companion_sprout,&companion_disc_3,&companion_mic,&companion_watchfaces};
        _Static_assert(sizeof sources/sizeof sources[0]+3==COMPANION_ICON_COUNT,"Keep appended icon sources in sync");
        lv_image_set_src(image,sources[index-LAUNCHER_ICON_COUNT]);
    }
}
lv_obj_t *companion_launcher_icon_create(lv_obj_t *parent, int index) {
    lv_obj_t *container=lv_obj_create(parent); lv_obj_remove_style_all(container);
    lv_obj_set_size(container,LAUNCHER_ICON_SIZE,LAUNCHER_ICON_SIZE);
    ui_obj_set_scrollable(container,false); ui_obj_set_clickable(container,false); ui_obj_set_event_bubble(container,true);
    lv_obj_t *original=launcher_icon_create(container,LAUNCHER_WIFI); lv_obj_center(original);
    lv_obj_t *image=lv_image_create(container); lv_obj_center(image);
    lv_obj_t *mimi=lv_obj_create(container); lv_obj_remove_style_all(mimi);
    lv_obj_set_size(mimi,LAUNCHER_ICON_SIZE,LAUNCHER_ICON_SIZE);
    ui_obj_set_scrollable(mimi,false);ui_obj_set_clickable(mimi,false);ui_obj_set_event_bubble(mimi,true);
    lv_obj_add_event_cb(mimi,mimi_icon,LV_EVENT_DRAW_MAIN,NULL);
    lv_obj_t *codex=lv_obj_create(container);lv_obj_remove_style_all(codex);
    lv_obj_set_size(codex,LAUNCHER_ICON_SIZE,LAUNCHER_ICON_SIZE);
    ui_obj_set_scrollable(codex,false);ui_obj_set_clickable(codex,false);ui_obj_set_event_bubble(codex,true);
    lv_obj_add_event_cb(codex,codex_icon,LV_EVENT_DRAW_MAIN,NULL);
    lv_obj_t *travel=lv_obj_create(container);lv_obj_remove_style_all(travel);lv_obj_set_size(travel,LAUNCHER_ICON_SIZE,LAUNCHER_ICON_SIZE);
    ui_obj_set_scrollable(travel,false);ui_obj_set_clickable(travel,false);ui_obj_set_event_bubble(travel,true);
    lv_obj_add_event_cb(travel,travel_icon,LV_EVENT_DRAW_MAIN,NULL);
    lv_obj_set_style_image_recolor(image,lv_color_hex(0xf5f5f2),0);
    lv_obj_set_style_image_recolor_opa(image,LV_OPA_COVER,0);
    companion_launcher_icon_set(container,index); return container;
}
