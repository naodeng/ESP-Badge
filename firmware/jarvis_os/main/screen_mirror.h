#pragma once

#include "lvgl.h"

void screen_mirror_init(lv_display_t *display);
void screen_mirror_flush(lv_display_t *display, const lv_area_t *area,
                         const uint8_t *pixels, bool is_last);
