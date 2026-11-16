#include "watchface_grok.h"

#include <stdio.h>
#include <string.h>

#define FACE_WHITE 0xf3f1eb
#define FACE_GRAY  0x929294
#define FACE_DIM   0x29292e
#define FACE_RED   0xd1283a

typedef struct { lv_layer_t *layer; int x, y, size; bool preview; } face_draw_t;

static int px(const face_draw_t *d, int value) {
    return value * d->size / 466;
}

uint8_t watchface_grok_next_expression(uint8_t previous, uint32_t random_value) {
    uint8_t base = (uint8_t)(random_value % (WATCHFACE_GROK_EXPRESSION_COUNT - 1));
    if (previous >= WATCHFACE_GROK_EXPRESSION_COUNT) return base;
    return (uint8_t)((previous + 1 + base) % WATCHFACE_GROK_EXPRESSION_COUNT);
}

uint8_t watchface_grok_blink_amount(uint16_t ms_in_second) {
    if (ms_in_second < 70) return (uint8_t)(ms_in_second * 255u / 70u);
    if (ms_in_second <= 110) return 255;
    if (ms_in_second < 220) return (uint8_t)((220u - ms_in_second) * 255u / 110u);
    return 0;
}

static void pill(const face_draw_t *d, int cx, int cy, int width, int height, uint32_t color) {
    lv_draw_rect_dsc_t rect; lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_hex(color); rect.bg_opa = LV_OPA_COVER; rect.radius = LV_RADIUS_CIRCLE;
    int w=px(d,width),h=px(d,height),x=d->x+px(d,cx),y=d->y+px(d,cy);
    lv_area_t a={x-w/2,y-h/2,x+(w-1)/2,y+(h-1)/2};
    lv_draw_rect(d->layer,&rect,&a);
}

static void dot(const face_draw_t *d, int cx, int cy, int diameter, uint32_t color) {
    pill(d,cx,cy,diameter,diameter,color);
}

static void cover(const face_draw_t *d,int cx,int top,int width,int height) {
    if(width<=0||height<=0)return;
    lv_draw_rect_dsc_t rect;lv_draw_rect_dsc_init(&rect);
    rect.bg_color=lv_color_black();rect.bg_opa=LV_OPA_COVER;rect.radius=0;
    int w=px(d,width),h=px(d,height),x=d->x+px(d,cx)-w/2,y=d->y+px(d,top);
    lv_area_t a={x,y,x+w-1,y+h-1};lv_draw_rect(d->layer,&rect,&a);
}

static void eyelids(const face_draw_t *d,uint8_t amount) {
    if(!amount)return;
    const int left=151,right=315,top=118,height=142;
    int cover_height=height*amount/255;
    cover(d,left,top,104,cover_height);cover(d,right,top,104,cover_height);
    if(amount==255){pill(d,left,190,72,5,FACE_WHITE);pill(d,right,190,72,5,FACE_WHITE);}
}

static void line(const face_draw_t *d, int x1, int y1, int x2, int y2, int width, uint32_t color) {
    lv_draw_line_dsc_t desc; lv_draw_line_dsc_init(&desc);
    desc.p1=(lv_point_precise_t){d->x+px(d,x1),d->y+px(d,y1)};
    desc.p2=(lv_point_precise_t){d->x+px(d,x2),d->y+px(d,y2)};
    desc.width=LV_MAX(1,px(d,width));desc.color=lv_color_hex(color);desc.round_start=true;desc.round_end=true;
    lv_draw_line(d->layer,&desc);
}

static void arc(const face_draw_t *d, int cx, int cy, int radius, int start, int end, int width, uint32_t color) {
    lv_draw_arc_dsc_t desc;lv_draw_arc_dsc_init(&desc);
    desc.center=(lv_point_t){d->x+px(d,cx),d->y+px(d,cy)};
    desc.radius=LV_MAX(1,px(d,radius));desc.start_angle=(uint16_t)start;desc.end_angle=(uint16_t)end;
    desc.width=LV_MAX(1,px(d,width));desc.color=lv_color_hex(color);desc.rounded=true;
    lv_draw_arc(d->layer,&desc);
}

static const lv_font_t *font(const face_draw_t *d,int size) {
    if(size==72)return d->preview?&font_wf_36:&font_wf_72;
    if(size==18)return d->preview?&font_wf_9:&font_wf_18;
    return d->preview?&font_wf_7:&font_wf_14;
}

static void text(const face_draw_t *d,const char *value,int cy,int size,uint32_t color) {
    const lv_font_t *f=font(d,size);int width=px(d,370),spacing=px(d,1);lv_point_t bounds;
    lv_text_get_size(&bounds,value,f,spacing,0,width,LV_TEXT_FLAG_NONE);
    lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);
    label.font=f;label.text=value;label.text_local=1;label.color=lv_color_hex(color);label.align=LV_TEXT_ALIGN_CENTER;label.letter_space=spacing;
    int top=d->y+px(d,cy)-bounds.y/2,left=d->x+(d->size-width)/2;
    lv_area_t a={left,top,left+width-1,top+bounds.y-1};lv_draw_label(d->layer,&label,&a);
}

static void eyes(const face_draw_t *d,uint8_t expression) {
    const int left=151,right=315,mid=190;
    int lw=78,rw=78,lh=106,rh=106,ly=mid,ry=mid;
    switch(expression){
        case 1: lh=rh=126;lw=rw=82;break;                 // listening
        case 2: lh=76;rh=82;ly-=4;ry+=4;break;            // thinking
        case 3: lh=104;rh=84;break;                       // speaking
        case 4: arc(d,left,mid+18,36,200,340,13,FACE_WHITE);arc(d,right,mid+18,36,200,340,13,FACE_WHITE);return;
        case 5: lh=rh=130;lw=rw=86;break;                 // excited
        case 6: lh=126;rh=82;ly-=5;ry+=5;break;           // curious
        case 7: lh=92;rh=110;ly+=3;ry-=4;break;           // confused
        case 8: lh=rh=94;break;                           // angry
        case 9: lh=rh=96;lw=rw=68;break;                 // surprised
        case 10: lh=rh=100;ly+=5;ry+=5;break;             // sad
        case 11: lh=rh=48;ly+=24;ry+=24;break;            // sleepy
        case 12:
            for(int i=0;i<2;++i){int cx=i?right:left;arc(d,cx,mid,34,28,286,9,FACE_WHITE);arc(d,cx,mid,18,214,510,6,FACE_GRAY);}return;
        default: break;
    }
    pill(d,left,ly,lw,lh,FACE_WHITE);pill(d,right,ry,rw,rh,FACE_WHITE);
    if(expression==5){pill(d,left-6,ly-17,10,16,FACE_GRAY);pill(d,right-6,ry-17,10,16,FACE_GRAY);}
    if(expression==9){pill(d,left,ly,24,38,0x000000);pill(d,right,ry,24,38,0x000000);}
    if(expression==2)line(d,left-42,mid-68,left+24,mid-75,7,FACE_GRAY);
    if(expression==7)line(d,left-34,mid-68,left+26,mid-68,6,FACE_GRAY);
    if(expression==8){line(d,left-42,mid-72,left+34,mid-54,8,FACE_GRAY);line(d,right-34,mid-54,right+42,mid-72,8,FACE_GRAY);}
    if(expression==10){line(d,left-38,mid-57,left+28,mid-45,7,FACE_GRAY);line(d,right-28,mid-45,right+38,mid-57,7,FACE_GRAY);}
    if(expression==11){line(d,left-42,mid+12,left+42,mid+12,6,FACE_GRAY);line(d,right-42,mid+12,right+42,mid+12,6,FACE_GRAY);}
}

void watchface_grok_render(lv_layer_t *layer,const lv_area_t *area,
                           const watchface_data_t *data,bool preview) {
    if(!layer||!area||!data)return;
    face_draw_t d={.layer=layer,.x=area->x1,.y=area->y1,.size=lv_area_get_width(area),.preview=preview};
    uint8_t expression=data->grok_expression%WATCHFACE_GROK_EXPRESSION_COUNT;
    static const char *const moods[]={"IDLE","LISTENING","THINKING","SPEAKING","HAPPY","EXCITED","CURIOUS","CONFUSED","ANGRY","SURPRISED","SAD","SLEEPY","DIZZY"};
    text(&d,"JARVIS",62,18,FACE_GRAY);
    dot(&d,233,82,6,FACE_RED);
    eyes(&d,expression);
    if(!preview&&!data->aod)eyelids(&d,data->grok_blink);
    text(&d,moods[expression],263,14,FACE_GRAY);
    char time_text[8],date_text[20],battery_text[20];
    snprintf(time_text,sizeof time_text,"%02d:%02d",data->time.tm_hour,data->time.tm_min);
    text(&d,time_text,324,72,FACE_WHITE);
    if(data->time.tm_year>0){
        static const char *const months[]={"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
        static const char *const days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
        snprintf(date_text,sizeof date_text,"%s  %02d %s",days[LV_CLAMP(0,data->time.tm_wday,6)],data->time.tm_mday,months[LV_CLAMP(0,data->time.tm_mon,11)]);
    }else snprintf(date_text,sizeof date_text,"SYNCING TIME");
    text(&d,date_text,375,18,FACE_GRAY);
    if(data->battery_valid)snprintf(battery_text,sizeof battery_text,"%d%%  BATTERY%s",LV_CLAMP(0,data->battery,100),data->charging?" · CHARGING":"");
    else snprintf(battery_text,sizeof battery_text,"--  BATTERY");
    text(&d,battery_text,414,14,data->battery_valid&&data->battery<=15?FACE_RED:FACE_GRAY);
}
