#include "identity_ui.h"
#include "identity_geometry.h"
#include "src/core/lv_obj_private.h"
#include "src/core/lv_obj_class_private.h"
#include <math.h>

typedef struct {lv_obj_t obj; uint32_t elapsed_ms;} logo_t;
typedef struct {lv_obj_t obj; uint32_t elapsed_ms;} reactor_t;
typedef struct {lv_obj_t obj; lv_obj_t *logo,*name,*status; bool elapsed,ready;} boot_t;
static lv_obj_t *s_boot;
static const char *s_jarvis_matrix[]={"00111","00010","00010","00010","00010","10010","01100"};
enum { REACTOR_SIZE=176, REACTOR_RADIUS=74, REACTOR_DOT_COUNT=72, BOOT_REACTOR_SIZE=240 };
#define REACTOR_CYAN 0x00d9f2
#define REACTOR_ICE  0xbefaff
#define REACTOR_DIM  0x174550
#define REACTOR_RIM  0x0b3038

static int32_t phase(uint32_t ms, uint32_t start, uint32_t duration) {
    if(ms <= start)return 0;
    if(ms >= start+duration)return LV_BEZIER_VAL_MAX;
    return (int32_t)((ms-start)*LV_BEZIER_VAL_MAX/duration);
}
static int32_t eased(int32_t value) {
    return lv_cubic_bezier(value,LV_BEZIER_VAL_FLOAT(.22f),LV_BEZIER_VAL_FLOAT(.61f),
                          LV_BEZIER_VAL_FLOAT(.36f),LV_BEZIER_VAL_MAX);
}
static void draw_dot(lv_obj_t *obj,lv_layer_t *layer,const lv_draw_line_dsc_t *style,
                     int cx,int cy,int diameter,uint32_t color,uint8_t opacity) {
    if(diameter<1)diameter=1;
    lv_draw_rect_dsc_t dot;lv_draw_rect_dsc_init(&dot);
    dot.base.obj=obj;dot.bg_opa=(lv_opa_t)((uint32_t)style->opa*opacity/255u);
    dot.bg_color=lv_color_hex(color);dot.radius=LV_RADIUS_CIRCLE;
    int x=cx-diameter/2,y=cy-diameter/2;
    lv_area_t bounds={x,y,x+diameter-1,y+diameter-1};
    lv_draw_rect(layer,&dot,&bounds);
}
static void logo_draw(lv_obj_t *obj,lv_layer_t *layer) {
    logo_t *logo=(logo_t *)obj;
    lv_area_t area;lv_obj_get_coords(obj,&area);
    float scale=(float)lv_obj_get_width(obj)/IDENTITY_VIEWBOX;
    lv_draw_line_dsc_t style;lv_draw_line_dsc_init(&style);
    lv_obj_init_draw_line_dsc(obj,LV_PART_MAIN,&style);
    // The 36px settings-row mark needs wider gaps to stay visibly dotted.
    int ring_count=scale<.75f?IDENTITY_RING_DOT_COUNT/2:IDENTITY_RING_DOT_COUNT;
    float ring_dot_units=ring_count<IDENTITY_RING_DOT_COUNT?3.f:IDENTITY_RING_DOT_DIAMETER;
    int ring_diameter=(int)lroundf(ring_dot_units*scale);
    int32_t ring_progress=eased(phase(logo->elapsed_ms,100,800));
    int ring_lit=(int)((uint32_t)ring_count*ring_progress/LV_BEZIER_VAL_MAX);
    for(int i=0;i<ring_count;++i) {
        // Begin at the lower-right scan point and travel clockwise around the face.
        float angle=(45.f+(float)i*360.f/(float)ring_count)*
                    0.01745329251994329577f;
        int x=area.x1+(int)lroundf((IDENTITY_RING_CX+IDENTITY_RING_RADIUS*cosf(angle))*scale);
        int y=area.y1+(int)lroundf((IDENTITY_RING_CY+IDENTITY_RING_RADIUS*sinf(angle))*scale);
        draw_dot(obj,layer,&style,x,y,ring_diameter,i<ring_lit?IDENTITY_WHITE:0x34393c,255);
    }

    int matrix_diameter=(int)lroundf(IDENTITY_J_DOT_DIAMETER*scale);
    int matrix_index=0;
    int matrix_count=0;
    for(int row=0;row<7;++row)for(int column=0;column<5;++column)
        if(s_jarvis_matrix[row][column]=='1')++matrix_count;
    int32_t matrix_progress=eased(phase(logo->elapsed_ms,220,560));
    int matrix_lit=(int)((uint32_t)matrix_count*matrix_progress/LV_BEZIER_VAL_MAX);
    for(int row=0;row<7;++row)for(int column=0;column<5;++column) {
        if(s_jarvis_matrix[row][column]!='1')continue;
        int x=area.x1+(int)lroundf((IDENTITY_RING_CX+(column-2)*IDENTITY_J_PITCH)*scale);
        int y=area.y1+(int)lroundf((IDENTITY_RING_CY+(row-3)*IDENTITY_J_PITCH)*scale);
        draw_dot(obj,layer,&style,x,y,matrix_diameter,
                 matrix_index<matrix_lit?IDENTITY_WHITE:0x34393c,255);
        ++matrix_index;
    }

    int32_t alpha=phase(logo->elapsed_ms,80,160);
    if(!alpha)return;
    float angle=(1.f-(float)eased(phase(logo->elapsed_ms,120,880))/LV_BEZIER_VAL_MAX)*
                1.57079632679489661923f;
    float dx=IDENTITY_DOT_CX-IDENTITY_RING_CX,dy=IDENTITY_DOT_CY-IDENTITY_RING_CY;
    int x=area.x1+(int)lroundf((IDENTITY_RING_CX+dx*cosf(angle)-dy*sinf(angle))*scale);
    int y=area.y1+(int)lroundf((IDENTITY_RING_CY+dx*sinf(angle)+dy*cosf(angle))*scale);
    int diameter=(int)lroundf(2.f*IDENTITY_DOT_RADIUS*scale);
    draw_dot(obj,layer,&style,x,y,diameter,IDENTITY_RED,
             (uint8_t)(255u*(uint32_t)alpha/LV_BEZIER_VAL_MAX));
}

static bool inside_triangle(int x,int y,int ax,int ay,int bx,int by,int cx,int cy) {
    int ab=(x-ax)*(by-ay)-(y-ay)*(bx-ax);
    int bc=(x-bx)*(cy-by)-(y-by)*(cx-bx);
    int ca=(x-cx)*(ay-cy)-(y-cy)*(ax-cx);
    return (ab>=0&&bc>=0&&ca>=0)||(ab<=0&&bc<=0&&ca<=0);
}

static bool inside_polygon(int x,int y,const int (*points)[2],size_t count) {
    bool inside=false;
    for(size_t i=0,j=count-1;i<count;j=i++) {
        int xi=points[i][0],yi=points[i][1];
        int xj=points[j][0],yj=points[j][1];
        if(((yi>y)!=(yj>y)) &&
           (x<(xj-xi)*(y-yi)/(yj-yi)+xi))inside=!inside;
    }
    return inside;
}

static int reactor_region(int x,int y) {
    static const int top[][2]={{-22,-59},{22,-59},{48,-40},{-48,-40}};
    static const int left[][2]={{-60,-28},{-44,-38},{-12,20},{-14,51},{-26,54},{-56,15}};
    static const int right[][2]={{60,-28},{44,-38},{12,20},{14,51},{26,54},{56,15}};
    if(inside_triangle(x,y,-25,-15,25,-15,0,31))return 3;
    if(inside_polygon(x,y,top,sizeof(top)/sizeof(top[0])))return 1;
    if(inside_polygon(x,y,left,sizeof(left)/sizeof(left[0])))return 2;
    if(inside_polygon(x,y,right,sizeof(right)/sizeof(right[0])))return 2;
    return 0;
}

static void reactor_draw(lv_obj_t *obj,lv_layer_t *layer) {
    reactor_t *reactor=(reactor_t *)obj;
    lv_area_t area;lv_obj_get_coords(obj,&area);
    float scale=(float)lv_obj_get_width(obj)/REACTOR_SIZE;
    int cx=area.x1+lv_obj_get_width(obj)/2,cy=area.y1+lv_obj_get_height(obj)/2;
    lv_draw_line_dsc_t style;lv_draw_line_dsc_init(&style);
    lv_obj_init_draw_line_dsc(obj,LV_PART_MAIN,&style);

    int32_t ring_progress=eased(phase(reactor->elapsed_ms,100,1450));
    int ring_lit=(int)((uint32_t)REACTOR_DOT_COUNT*ring_progress/LV_BEZIER_VAL_MAX);
    for(int i=0;i<REACTOR_DOT_COUNT;++i) {
        float angle=(-90.f+(float)i*360.f/REACTOR_DOT_COUNT)*0.01745329251994329577f;
        int x=cx+(int)lroundf(REACTOR_RADIUS*scale*cosf(angle));
        int y=cy+(int)lroundf(REACTOR_RADIUS*scale*sinf(angle));
        int diameter=(int)lroundf(3.6f*scale);
        draw_dot(obj,layer,&style,x,y,diameter,i<ring_lit?REACTOR_CYAN:REACTOR_DIM,255);
    }

    int32_t body_progress=eased(phase(reactor->elapsed_ms,220,1200));
    int point_count=0;
    for(int y=-56;y<=56;y+=8)for(int x=-56;x<=56;x+=8)
        if(x*x+y*y<=61*61 && reactor_region(x,y))++point_count;
    int point_lit=(int)((uint32_t)point_count*body_progress/LV_BEZIER_VAL_MAX);
    int point_index=0;
    for(int y=-56;y<=56;y+=8)for(int x=-56;x<=56;x+=8) {
        int region=reactor_region(x,y);
        if(x*x+y*y>61*61 || !region)continue;
        uint32_t color=point_index<point_lit?(region==3?REACTOR_ICE:REACTOR_CYAN):REACTOR_DIM;
        int diameter=(int)lroundf((region==3?4.6f:3.6f)*scale);
        draw_dot(obj,layer,&style,cx+(int)lroundf(x*scale),cy+(int)lroundf(y*scale),
                 diameter,color,255);
        ++point_index;
    }

    int32_t rim_progress=eased(phase(reactor->elapsed_ms,520,900));
    int rim_lit=(int)(48u*(uint32_t)rim_progress/LV_BEZIER_VAL_MAX);
    for(int i=0;i<48;++i) {
        float angle=(-90.f+(float)i*360.f/48.f)*0.01745329251994329577f;
        int x=cx+(int)lroundf(55.f*scale*cosf(angle));
        int y=cy+(int)lroundf(55.f*scale*sinf(angle));
        draw_dot(obj,layer,&style,x,y,(int)lroundf(2.6f*scale),
                 i<rim_lit?REACTOR_DIM:REACTOR_RIM,255);
    }

    int core_diameter=(int)lroundf(5.6f*scale);
    draw_dot(obj,layer,&style,cx-(int)lroundf(25.f*scale),cy-(int)lroundf(15.f*scale),
             core_diameter,REACTOR_ICE,(uint8_t)(255u*(uint32_t)body_progress/LV_BEZIER_VAL_MAX));
    draw_dot(obj,layer,&style,cx+(int)lroundf(25.f*scale),cy-(int)lroundf(15.f*scale),
             core_diameter,REACTOR_ICE,(uint8_t)(255u*(uint32_t)body_progress/LV_BEZIER_VAL_MAX));
    draw_dot(obj,layer,&style,cx,cy+(int)lroundf(31.f*scale),core_diameter,REACTOR_ICE,
             (uint8_t)(255u*(uint32_t)body_progress/LV_BEZIER_VAL_MAX));
    int32_t pulse=eased(phase(reactor->elapsed_ms,550,650));
    draw_dot(obj,layer,&style,cx,cy+(int)lroundf(1.f*scale),
             (int)lroundf((4.f+2.f*pulse/LV_BEZIER_VAL_MAX)*scale),REACTOR_ICE,
             (uint8_t)(255u*(uint32_t)pulse/LV_BEZIER_VAL_MAX));
}

static void reactor_event(const lv_obj_class_t *cls,lv_event_t *event) {
    lv_event_code_t code=lv_event_get_code(event);
    if(code==LV_EVENT_COVER_CHECK){lv_event_set_cover_res(event,LV_COVER_RES_NOT_COVER);return;}
    if(code==LV_EVENT_DRAW_MAIN){reactor_draw(lv_event_get_target_obj(event),lv_event_get_layer(event));return;}
    if(code!=LV_EVENT_DRAW_MAIN_END)lv_obj_event_base(cls,event);
}

static const lv_obj_class_t reactor_class={
    .base_class=&lv_obj_class,.event_cb=reactor_event,.instance_size=sizeof(reactor_t),.name="jarvis_arc_reactor"
};

static lv_obj_t *reactor_create(lv_obj_t *parent,int size) {
    if(size<=0)return NULL;
    lv_obj_t *obj=lv_obj_class_create_obj(&reactor_class,parent);if(!obj)return NULL;
    lv_obj_class_init_obj(obj);lv_obj_remove_style_all(obj);lv_obj_set_size(obj,size,size);
    lv_obj_set_scrollable(obj,false);lv_obj_set_clickable(obj,false);lv_obj_set_gesture_bubble(obj,false);
    ((reactor_t *)obj)->elapsed_ms=0;
    return obj;
}
static void logo_event(const lv_obj_class_t *cls,lv_event_t *event) {
    lv_event_code_t code=lv_event_get_code(event);
    if(code==LV_EVENT_COVER_CHECK){lv_event_set_cover_res(event,LV_COVER_RES_NOT_COVER);return;}
    if(code==LV_EVENT_DRAW_MAIN){logo_draw(lv_event_get_target_obj(event),lv_event_get_layer(event));return;}
    if(code!=LV_EVENT_DRAW_MAIN_END)lv_obj_event_base(cls,event);
}
static const lv_obj_class_t logo_class={
    .base_class=&lv_obj_class,.event_cb=logo_event,.instance_size=sizeof(logo_t),.name="jarvis_dot_matrix"
};
static const lv_obj_class_t boot_class={
    .base_class=&lv_obj_class,.instance_size=sizeof(boot_t),.name="jarvis_boot"
};
lv_obj_t *identity_logo_create(lv_obj_t *parent,int size) {
    if(size<=0)return NULL;
    lv_obj_t *obj=lv_obj_class_create_obj(&logo_class,parent);if(!obj)return NULL;
    lv_obj_class_init_obj(obj);lv_obj_remove_style_all(obj);lv_obj_set_size(obj,size,size);
    lv_obj_set_scrollable(obj,false);lv_obj_set_clickable(obj,false);lv_obj_set_gesture_bubble(obj,true);
    ((logo_t *)obj)->elapsed_ms=1000;
    return obj;
}
static void boot_advance(void *var,int32_t ms) {
    boot_t *boot=var;reactor_t *reactor=(reactor_t *)boot->logo;
    uint32_t reactor_ms=ms<1800?(uint32_t)ms:1800;
    if(reactor->elapsed_ms!=reactor_ms){reactor->elapsed_ms=reactor_ms;lv_obj_invalidate(boot->logo);}
    int32_t fade=phase((uint32_t)ms,1820,480);
    lv_obj_set_style_text_opa(boot->name,(lv_opa_t)(255*fade/LV_BEZIER_VAL_MAX),0);
    int baseline=349-7*eased(fade)/LV_BEZIER_VAL_MAX;
    lv_obj_set_y(boot->name,baseline-font_identity_26.line_height+font_identity_26.base_line);
    lv_obj_set_style_text_opa(boot->status,(lv_opa_t)(255*phase((uint32_t)ms,2500,300)/LV_BEZIER_VAL_MAX),0);
}
static void boot_completed(lv_anim_t *animation) {
    boot_t *boot=animation->var;boot->elapsed=true;
    if(boot->ready){lv_display_trigger_activity(NULL);lv_obj_delete((lv_obj_t *)boot);}
}
static void boot_deleted(lv_event_t *event) {
    if(lv_event_get_target_obj(event)==s_boot)s_boot=NULL;
}
bool identity_boot_active(void){return s_boot!=NULL;}
void identity_boot_message(lv_obj_t *obj,const char *message,bool error) {
    if(!obj)return;
    boot_t *boot=(boot_t *)obj;
    if(error){boot->ready=false;lv_obj_set_hidden(obj,false);lv_obj_move_foreground(obj);}
    lv_label_set_text(boot->status,message?message:"");
    lv_obj_set_style_text_color(boot->status,lv_color_hex(error?IDENTITY_RED:0x9a9a9e),0);
}
void identity_boot_reveal(lv_obj_t *obj) {
    if(obj)lv_obj_set_hidden(obj,true);
}
void identity_boot_release(lv_obj_t *obj) {
    if(!obj)return;
    boot_t *boot=(boot_t *)obj;boot->ready=true;
    if(boot->elapsed){lv_display_trigger_activity(NULL);lv_obj_delete(obj);}
}
lv_obj_t *identity_boot_create(lv_obj_t *parent) {
    lv_obj_t *obj=lv_obj_class_create_obj(&boot_class,parent);if(!obj)return NULL;
    lv_obj_class_init_obj(obj);lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj,466,466);lv_obj_set_pos(obj,0,0);
    lv_obj_set_style_bg_color(obj,lv_color_black(),0);lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    // Swallow touches/gestures while the original lockscreen continues underneath.
    lv_obj_set_scrollable(obj,false);lv_obj_set_gesture_bubble(obj,false);lv_obj_set_event_bubble(obj,false);
    lv_obj_set_clickable(obj,true);
    s_boot=obj;lv_obj_add_event_cb(obj,boot_deleted,LV_EVENT_DELETE,NULL);
    boot_t *boot=(boot_t *)obj;
    boot->logo=reactor_create(obj,BOOT_REACTOR_SIZE);boot->name=lv_label_create(obj);boot->status=lv_label_create(obj);
    if(!boot->logo||!boot->name||!boot->status){lv_obj_delete(obj);return NULL;}
    lv_obj_set_pos(boot->logo,(466-BOOT_REACTOR_SIZE)/2,82);
    lv_obj_set_gesture_bubble(boot->logo,false);
    lv_obj_set_clickable(boot->name,false);lv_obj_set_gesture_bubble(boot->name,false);
    lv_label_set_text(boot->name,"JARVIS OS");lv_obj_set_width(boot->name,300);lv_obj_set_x(boot->name,83);
    lv_obj_set_style_text_font(boot->name,&font_identity_26,0);
    lv_obj_set_style_text_align(boot->name,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(boot->name,lv_color_hex(IDENTITY_WHITE),0);
    lv_label_set_text(boot->status,"System check...");lv_obj_set_size(boot->status,276,LV_SIZE_CONTENT);
    lv_obj_set_pos(boot->status,95,371);lv_obj_set_style_text_font(boot->status,&font_identity_18,0);
    lv_obj_set_style_text_align(boot->status,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(boot->status,lv_color_hex(0x9a9a9e),0);
    lv_obj_set_clickable(boot->status,false);lv_obj_set_gesture_bubble(boot->status,false);
    lv_obj_set_event_bubble(boot->status,false);
    lv_anim_t animation;lv_anim_init(&animation);lv_anim_set_var(&animation,obj);
    lv_anim_set_exec_cb(&animation,boot_advance);lv_anim_set_values(&animation,0,IDENTITY_BOOT_MS);
    lv_anim_set_duration(&animation,IDENTITY_BOOT_MS);
    // LVGL deletes animations attached to the object, including an early parent deletion.
    lv_anim_set_completed_cb(&animation,boot_completed);
    if(!lv_anim_start(&animation)){lv_obj_delete(obj);return NULL;}
    return obj;
}
