// 锁屏控制:BOOT 短按=锁/解切换,长按 2 秒=关机;屏保上滑也可解锁。
// 锁屏后放电超时熄屏、充电常显(M3b)。实体键由 buttons.c 统一轮询。
#include "lock.h"
#include "identity_ui.h"
#include "watchface.h"
#include "display.h"
#include "power.h"
#include "settings.h"
#include "app.h"
#include "buttons.h"

#define AOD_MS     12000   // 空闲进入低功耗"平静"态(变暗 + 停止闪烁 + 按分钟刷新)
#define SLEEP_MS   30000   // 仅"自动熄屏"模式:空闲再久则熄屏
#define BR_DIM     0x20    // 平静态亮度
#define UNLOCK_SWIPE_START_Y 260
#define UNLOCK_SWIPE_MIN_PX 80

static bool s_locked = false;
static lv_point_t s_unlock_start;
static bool s_unlock_tracking;
typedef enum { SCR_FULL, SCR_CALM, SCR_OFF } scr_t;
static scr_t s_scr = SCR_FULL;

static void set_screen(scr_t s) {
    if (s == s_scr) return;
    s_scr = s;
    switch (s) {
        // FULL=全亮活动态(表盘正常闪/秒点);CALM=变暗 + AOD(停闪、只按分钟刷新);
        // OFF=熄屏 + 停表盘刷新(不再重画/推屏 → CPU 长空闲,配合 tickless 进浅睡)
        case SCR_FULL: watchface_set_sleep(false); watchface_set_aod(false); display_sleep(false); display_set_brightness(settings_brightness()); break;
        case SCR_CALM: watchface_set_sleep(false); watchface_set_aod(true);  display_sleep(false); display_set_brightness(BR_DIM);                break;
        case SCR_OFF:  display_set_brightness(0); display_sleep(true); watchface_set_sleep(true);                                                 break;
    }
}

void lock_set(bool locked) {
    buttons_reset_control();
    s_locked = locked;
    set_screen(SCR_FULL);
    lv_display_trigger_activity(NULL);    // 重置空闲计时
    if (locked) watchface_show();
    else        watchface_hide();
}

bool lock_is_locked(void) { return s_locked; }

static void watchface_touch_cb(lv_event_t *event) {
    const lv_event_code_t code = lv_event_get_code(event);
    lv_indev_t *input = lv_indev_active();
    if (code == LV_EVENT_PRESSED) {
        if (input != NULL) {
            lv_indev_get_point(input, &s_unlock_start);
            s_unlock_tracking = true;
        }
        return;
    }
    if (code == LV_EVENT_RELEASED) {
        s_unlock_tracking = false;
        return;
    }
    if (code != LV_EVENT_GESTURE || input == NULL || !s_unlock_tracking) return;

    lv_point_t end;
    lv_indev_get_point(input, &end);
    const bool deliberate_up_swipe = s_unlock_tracking &&
        s_unlock_start.y >= UNLOCK_SWIPE_START_Y &&
        s_unlock_start.y - end.y >= UNLOCK_SWIPE_MIN_PX;
    if (s_locked && deliberate_up_swipe &&
        lv_indev_get_gesture_dir(input) == LV_DIR_TOP) {
        s_unlock_tracking = false;
        lock_set(false);
    }
}

/* 省电:仅锁屏时按空闲超时进入平静/熄屏;触摸/按键唤醒。
   空闲即进入"平静"(变暗+停闪+按分钟刷新),无论充放电都生效(插着 USB 也能看到变暗)。
   仅"自动熄屏"模式且在放电时,空闲更久才真正熄屏;充电时当桌面钟常显不熄。 */
static void powersave_cb(lv_timer_t *t) {
    if(identity_boot_active()){set_screen(SCR_FULL);return;}
    if (!s_locked) { set_screen(SCR_FULL); return; }
    uint32_t idle = lv_display_get_inactive_time(NULL);
    if (idle <= AOD_MS) { set_screen(SCR_FULL); return; }       // 刚操作过 → 全亮活动态

    int soc; pwr_state_t st = PWR_UNKNOWN;
    bool discharging = power_read(&soc, &st) && (st == PWR_DISCHARGING);
    if (idle > SLEEP_MS && settings_idle_mode() == IDLE_OFF && discharging)
        set_screen(SCR_OFF);
    else
        set_screen(SCR_CALM);
}

/* BOOT 全局锁屏/关机;PWR 仅在桌面或 App 可见时发布控制事件。 */
static void button_cb(lv_timer_t *t) {
    (void)t;
    bool control_active=!identity_boot_active()&&(launcher_home_visible()||launcher_app_visible());
    button_event_t ev = buttons_poll(control_active);
    if (ev == BUTTON_SHORT && !identity_boot_active()) {
        lock_set(!s_locked);
    }
    else if (ev == BUTTON_LONG) power_off();
}

bool lock_init(void) {
    watchface_init();
    lv_obj_add_event_cb(watchface_root(), watchface_touch_cb,
                        LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(watchface_root(), watchface_touch_cb,
                        LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(watchface_root(), watchface_touch_cb,
                        LV_EVENT_GESTURE, NULL);

    buttons_init();
    lv_timer_t *buttons=lv_timer_create(button_cb,20,NULL);
    lv_timer_t *power=lv_timer_create(powersave_cb,300,NULL);
    return buttons && power;
}
