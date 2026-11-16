#include "app.h"
#include "ble_travel.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static travel_snapshot_t current;
static unsigned starts,stops;
static uint16_t pixels[466*466];
static lv_display_t *display;
uint8_t settings_lang(void){return 0;}
bool ble_travel_start(void){++starts;return true;}
void ble_travel_stop(void){++stops;}
void ble_travel_snapshot(travel_snapshot_t *snapshot){*snapshot=current;}
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *map){
    int w=lv_area_get_width(a);for(int y=a->y1;y<=a->y2;++y){memcpy(pixels+y*466+a->x1,map,(size_t)w*2);map+=(size_t)w*2;}lv_display_flush_ready(d);
}
static lv_obj_t *find_text(lv_obj_t *o,const char *text){
    if(lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),text))return o;
    for(uint32_t i=0;i<lv_obj_get_child_count(o);++i){lv_obj_t *found=find_text(lv_obj_get_child(o,i),text);if(found)return found;}
    return NULL;
}
static void assert_round_labels(lv_obj_t *o){
    if(lv_obj_check_type(o,&lv_label_class)){
        lv_area_t a;lv_obj_get_coords(o,&a);const int xs[]={a.x1,a.x2},ys[]={a.y1,a.y2};
        for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x)assert(hypot(xs[x]-232.5,ys[y]-232.5)<=225);
    }
    for(uint32_t i=0;i<lv_obj_get_child_count(o);++i)assert_round_labels(lv_obj_get_child(o,i));
}
int main(void){
    lv_init();display=lv_display_create(466,466);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,pixels,NULL,sizeof pixels,LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
    lv_obj_set_style_bg_color(lv_screen_active(),lv_color_black(),0);
    lv_obj_t *page=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(page);lv_obj_set_size(page,466,466);
    const app_t app=app_travel;app.enter(page);assert(starts==1);
    lv_obj_update_layout(lv_screen_active());assert_round_labels(page);
    assert(find_text(page,"CONNECT PHONE")&&find_text(page,"--")&&find_text(page,"--°"));
    current=(travel_snapshot_t){.active=true,.connected=true,.status=TRAVEL_LIVE,
        .data={.present=true,.speed_valid=true,.heading_valid=true,.accuracy_valid=true,
        .speed_cms=1234,.heading_cdeg=9050,.accuracy_dm=120}};
    app.tick();assert(find_text(page,"44.4")&&find_text(page,"091°")&&find_text(page,"±12.0 m"));
    current.data.speed_cms=149;app.tick();assert(find_text(page,"--°"));
    current.status=TRAVEL_STALE;app.tick();assert(find_text(page,"GPS DATA STALE")&&find_text(page,"--"));
    current.status=TRAVEL_LIVE;current.data.speed_cms=1000;current.data.demo=true;app.tick();
    lv_obj_t *demo=find_text(page,"36.0");assert(demo&&lv_color_eq(lv_obj_get_style_text_color(demo,0),lv_color_hex(COL_RED)));
    app.visibility(false);assert(stops==1);app.visibility(true);assert(starts==2);app.exit();assert(stops==2);
    lv_obj_delete(page);lv_deinit();puts("travel UI: round fit, live values, low-speed heading, stale clearing, demo marking and lifecycle PASS");
}
