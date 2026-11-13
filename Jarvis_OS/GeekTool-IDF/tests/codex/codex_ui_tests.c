#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codex_client.h"
#include "power.h"
#include "settings.h"
#include "tools_ui.h"
#include "src/misc/lv_text_private.h"

static codex_snapshot_t fixture;
static bool client_visible;
static unsigned visibility_changes;
static unsigned battery=82;
static unsigned language;
static unsigned home_returns;
static char launcher_title[32];

uint8_t settings_lang(void){return (uint8_t)language;}
void launcher_set_title(const char *title){snprintf(launcher_title,sizeof launcher_title,"%s",title);}
void go_home(void){++home_returns;}
bool power_read(int *soc,pwr_state_t *state){*soc=(int)battery;*state=PWR_DISCHARGING;return true;}
void codex_client_set_visible(bool visible){client_visible=visible;++visibility_changes;}
void codex_client_request_refresh(void){}
void codex_client_snapshot(codex_snapshot_t *out){*out=fixture;}

#include "../../main/app_codex.c"

static uint16_t buffer[466*466];
static uint16_t pixels[466*466];
static lv_display_t *display;
static void flush(lv_display_t *d,const lv_area_t *area,uint8_t *bytes){
    int width=lv_area_get_width(area);
    for(int y=area->y1;y<=area->y2;++y){memcpy(pixels+y*466+area->x1,bytes,(size_t)width*2);bytes+=width*2;}
    lv_display_flush_ready(d);
}

static void capture(const char *directory,const char *name){
    if(!directory)return;
    lv_obj_update_layout(lv_screen_active());lv_refr_now(display);
    char path[512];snprintf(path,sizeof path,"%s/codex-%s.ppm",directory,name);
    FILE *file=fopen(path,"wb");assert(file);fprintf(file,"P6\n466 466\n255\n");
    for(size_t i=0;i<466u*466u;++i){uint16_t p=pixels[i];uint8_t rgb[]={
        (uint8_t)(((p>>11)&31)*255/31),(uint8_t)(((p>>5)&63)*255/63),(uint8_t)((p&31)*255/31)};
        assert(fwrite(rgb,1,sizeof rgb,file)==sizeof rgb);
    }
    assert(fclose(file)==0);
}

static void fixture_init(void){
    memset(&fixture,0,sizeof fixture);
    fixture.p5_percent=32;fixture.p7_percent=70;fixture.reset_s=18500;
    fixture.tokens=128700;fixture.cost_cents=500;fixture.snapshot_ts_s=1794512400;
    fixture.radar_updated_s=1794512340;fixture.radar_count=CODEX_RADAR_MAX;
    fixture.has_snapshot=fixture.online=true;fixture.plan[0]='p';strcpy(fixture.plan,"plus");
    strcpy(fixture.activity_state,"review");fixture.active_tasks=2;
    for(size_t i=0;i<CODEX_RADAR_MAX;++i){
        snprintf(fixture.radar[i].model,sizeof fixture.radar[i].model,"gpt-5.%u-codex",(unsigned)(i%10));
        snprintf(fixture.radar[i].epoch,sizeof fixture.radar[i].epoch,"%s",i%2?"high":"medium");
        fixture.radar[i].iq=1372-(int)i;
    }
}

static void verify_glyphs(lv_obj_t *root){
    if(ui_obj_is_hidden(root))return;
    if(lv_obj_check_type(root,&lv_label_class)){
        const char *text=lv_label_get_text(root);uint32_t offset=0;
        const lv_font_t *font=lv_obj_get_style_text_font(root,0);
        while(text[offset]){
            uint32_t codepoint=lv_text_encoded_next(text,&offset);lv_font_glyph_dsc_t glyph={0};
            if(!lv_font_get_glyph_dsc(font,&glyph,codepoint,0)){fprintf(stderr,"missing Codex glyph U+%04lx in: %s\n",(unsigned long)codepoint,text);abort();}
            assert(!glyph.is_placeholder);
        }
    }
    for(uint32_t i=0;i<lv_obj_get_child_count(root);++i)verify_glyphs(lv_obj_get_child(root,i));
}

static void verify_codex_font_latin(void){
    const lv_font_t *fonts[]={&font_tools_19,&font_tools_20,&font_tools_22,&font_tools_24};
    for(size_t i=0;i<sizeof fonts/sizeof fonts[0];++i){
        for(uint32_t code='A';code<='Z';++code){lv_font_glyph_dsc_t glyph={0};assert(lv_font_get_glyph_dsc(fonts[i],&glyph,code,0));assert(!glyph.is_placeholder);}
        for(uint32_t code='a';code<='z';++code){lv_font_glyph_dsc_t glyph={0};assert(lv_font_get_glyph_dsc(fonts[i],&glyph,code,0));assert(!glyph.is_placeholder);}
    }
}

int main(int argc,char **argv){
    const char *directory=argc>1?argv[1]:NULL;
    lv_init();verify_codex_font_latin();display=lv_display_create(466,466);assert(display);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer,NULL,sizeof buffer,LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);lv_obj_set_style_bg_color(lv_screen_active(),lv_color_black(),0);
    fixture_init();
    for(language=0;language<2;++language){
        lv_obj_t *parent=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(parent);lv_obj_set_size(parent,466,466);
        home_returns=0;
        visibility_changes=0;app_codex.enter(parent);
        assert(!strcmp(launcher_title,"Codex")&&s_visible&&client_visible&&visibility_changes==1);
        assert(s_page==CODEX_PAGE_USAGE&&!ui_obj_is_hidden(s_pages[CODEX_PAGE_USAGE]));
        assert(lv_obj_is_gesture_bubble(s_usage_today));
        assert(lv_obj_is_gesture_bubble(s_page_control));
        assert(!lv_obj_is_gesture_bubble(s_root));
        assert(lv_obj_is_clickable(s_root));
        assert(lv_obj_is_gesture_bubble(s_pages[CODEX_PAGE_USAGE]));
        assert(lv_obj_is_clickable(s_page_control));
        assert(codex_handle_swipe(LV_DIR_RIGHT)&&home_returns==1);
        assert(!lv_obj_is_clickable(s_usage_canvas));
        assert(!lv_obj_is_scrollable(s_usage_canvas));
        assert(!lv_obj_is_clickable(s_window_value[0]));
        assert(!lv_obj_is_clickable(s_window_value[1]));
        assert(!strcmp(lv_label_get_text(s_window_value[0]),"32%"));
        assert(!strcmp(lv_label_get_text(s_window_value[1]),"70%"));
        assert(strstr(lv_label_get_text(s_usage_today),"128.7K"));
        assert(strstr(lv_label_get_text(s_usage_cost),"5.00"));
        if(language)assert(strstr(lv_label_get_text(s_usage_reset),"5 小时后重置"));
        else assert(strstr(lv_label_get_text(s_usage_reset),"5H reset in"));
        assert(lv_obj_get_style_text_font(s_usage_reset,0)->fallback!=NULL);
        capture(directory,language?"usage-zh":"usage-en");
        verify_glyphs(s_root);

        assert(codex_handle_swipe(LV_DIR_LEFT)&&s_page==CODEX_PAGE_RADAR);
        assert(!ui_obj_is_hidden(s_pages[CODEX_PAGE_RADAR]));
        for(size_t i=0;i<CODEX_RADAR_MAX;++i){
            assert(s_radar_rows[i][0]&&s_radar_rows[i][1]&&s_radar_rows[i][2]&&s_radar_rows[i][3]);
            char expected[CODEX_MODEL_MAX];snprintf(expected,sizeof expected,"gpt-5.%u-codex",(unsigned)(i%10));
            assert(!strcmp(lv_label_get_text(s_radar_rows[i][1]),expected));
            assert(!strcmp(lv_label_get_text(s_radar_rows[i][2]),i%2?"high":"medium"));
            char iq[8];snprintf(iq,sizeof iq,"%d",1372-(int)i);
            assert(!strcmp(lv_label_get_text(s_radar_rows[i][3]),iq));
            assert(!lv_obj_is_clickable(s_radar_rows[i][1]));
            assert(lv_obj_is_gesture_bubble(s_radar_rows[i][1]));
        }
        assert(lv_obj_get_y(s_radar_rows[15][0])>lv_obj_get_y(s_radar_rows[0][0]));
        capture(directory,language?"radar-zh":"radar-en");
        verify_glyphs(s_root);

        fixture.online=false;fixture.stale=true;fixture.radar_stale=true;paint();
        assert(!strcmp(lv_label_get_text(s_window_value[0]),"32%"));
        assert(strstr(lv_label_get_text(s_usage_meta),language?"离线":"OFFLINE"));
        fixture.online=true;fixture.stale=false;fixture.radar_stale=false;
        fixture.has_snapshot=false;fixture.online=false;snprintf(fixture.error,sizeof fixture.error,"Bridge not found");paint();
        assert(strstr(lv_label_get_text(s_usage_meta),language?"未发现 Bridge":"BRIDGE NOT FOUND"));
        assert(strstr(lv_label_get_text(s_radar_meta),language?"未发现 Bridge":"BRIDGE NOT FOUND"));
        fixture.has_snapshot=true;fixture.online=true;fixture.error[0]=0;paint();
        lv_obj_send_event(s_page_control,LV_EVENT_CLICKED,NULL);assert(s_page==CODEX_PAGE_ACTIVITY);
        assert(strstr(lv_label_get_text(s_activity_state),language?"审查":"REVIEW"));
        capture(directory,language?"activity-zh":"activity-en");
        assert(codex_handle_swipe(LV_DIR_LEFT)&&s_page==CODEX_PAGE_BUDDY);
        assert(s_buddy_art&&!lv_obj_is_clickable(s_buddy_art));
        assert(strstr(lv_label_get_text(s_buddy_state),language?"审查":"REVIEW"));
        capture(directory,language?"buddy-zh":"buddy-en");
        assert(!lv_obj_is_clickable(s_page_control));
        assert(!codex_handle_swipe(LV_DIR_LEFT)&&s_page==CODEX_PAGE_BUDDY);
        assert(!app_codex_next_page()&&s_page==CODEX_PAGE_BUDDY);
        assert(codex_back()&&s_page==CODEX_PAGE_ACTIVITY);
        assert(app_codex_next_page()&&s_page==CODEX_PAGE_BUDDY);
        assert(!app_codex_next_page()&&s_page==CODEX_PAGE_BUDDY);
        assert(codex_back()&&s_page==CODEX_PAGE_ACTIVITY);
        codex_visibility(false);assert(!client_visible&&!s_visible);
        codex_visibility(true);assert(client_visible&&s_visible);
        lv_obj_update_layout(lv_screen_active());lv_refr_now(display);
        app_codex.exit();assert(!client_visible&&!s_root);
        lv_obj_delete(parent);
    }
    lv_deinit();puts("Codex native UI: bilingual glyphs, read-only usage, full 16-row Radar layout, cached offline values, four-card navigation and visible-only bridge lifecycle passed");
    return 0;
}
