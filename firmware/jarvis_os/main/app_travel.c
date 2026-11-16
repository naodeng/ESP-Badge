#include "app.h"
#include "ble_travel.h"
#include "tools_ui.h"
#include "ui_update.h"
#include <stdio.h>

static lv_obj_t *g_content,*g_speed,*g_unit,*g_heading,*g_status,*g_accuracy;
static bool s_start_failed;
static void travel_render(void){
    if(!g_content)return;
    travel_snapshot_t s;ble_travel_snapshot(&s);
    const char *status=s_start_failed?tools_text("BLE UNAVAILABLE","蓝牙不可用"):
        !s.connected?tools_text("CONNECT PHONE","等待手机连接"):
        s.status==TRAVEL_WAITING?tools_text("WAITING FOR GPS","等待定位数据"):
        s.status==TRAVEL_STALE?tools_text("GPS DATA STALE","定位数据已过期"):
        s.data.demo?tools_text("DEMO DATA","模拟数据"):
        tools_text("GPS CONNECTED","定位已连接");
    ui_text(g_status,status);
    bool live=s.connected&&s.status==TRAVEL_LIVE;
    char text[20];
    if(live&&s.data.speed_valid)snprintf(text,sizeof text,"%.1f",s.data.speed_cms/27.7778);
    else snprintf(text,sizeof text,"--");
    ui_text(g_speed,text);
    if(live&&travel_data_heading_valid(&s.data))snprintf(text,sizeof text,"%03u°",(unsigned)(s.data.heading_cdeg+50)/100%360);
    else snprintf(text,sizeof text,"--°");
    ui_text(g_heading,text);
    if(live&&s.data.accuracy_valid)snprintf(text,sizeof text,"±%.1f m",s.data.accuracy_dm/10.0);
    else snprintf(text,sizeof text,"--");
    ui_text(g_accuracy,text);
    uint32_t color=live&&s.data.demo?COL_RED:live?TOOLS_WHITE:TOOLS_GRAY;
    if(!lv_color_eq(lv_obj_get_style_text_color(g_speed,0),lv_color_hex(color)))lv_obj_set_style_text_color(g_speed,lv_color_hex(color),0);
}
static void travel_enter(lv_obj_t *parent){
    s_start_failed=!ble_travel_start();
    g_content=tools_surface(parent,0,90,466,340);
    g_status=tools_label(g_content,tools_text("CONNECT PHONE","等待手机连接"),&font_tools_20,233,24,TOOLS_GRAY,1);
    g_speed=tools_label(g_content,"--",&font_tools_60,233,131,TOOLS_WHITE,0);
    g_unit=tools_label(g_content,"km/h",&font_tools_24,233,189,TOOLS_GRAY,1);
    g_heading=tools_label(g_content,"--°",&font_tools_36,233,248,TOOLS_WHITE,0);
    g_accuracy=tools_label(g_content,"--",&font_tools_20,233,299,TOOLS_GRAY,1);
    travel_render();
}
static void travel_tick(void){travel_render();}
static void travel_exit(void){ble_travel_stop();g_content=g_speed=g_unit=g_heading=g_status=g_accuracy=NULL;s_start_failed=false;}
static void travel_visibility(bool visible){if(visible)s_start_failed=!ble_travel_start();else ble_travel_stop();}
const app_t app_travel={"Travel",COL_TXT,travel_enter,travel_tick,travel_exit,NULL,250,travel_visibility,false};
