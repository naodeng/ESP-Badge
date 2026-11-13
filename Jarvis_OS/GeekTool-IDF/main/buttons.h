#pragma once
#include <stdbool.h>

// LVGL 线程统一轮询:BOOT 短按锁/解,长按关机;PWR 短按/长按由当前页面消费。
typedef enum { BUTTON_NONE, BUTTON_SHORT, BUTTON_LONG } button_event_t;
void buttons_init(void);
button_event_t buttons_poll(bool control_active); // 20ms 全局轮询;隐藏时丢弃 PWR 事件
void buttons_reset_control(void);                 // 页面切换/锁屏时清掉旧事件
bool buttons_control_pressed(void);               // PWR 短按一次消费,不会跨页面补发
bool buttons_control_long_pressed(void);          // PWR 长按一次消费,不会持续重复
