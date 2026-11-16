#pragma once

#include "watchface_ui.h"

enum { WATCHFACE_GROK_EXPRESSION_COUNT = 13 };

// Pick from every expression except the previous one.
uint8_t watchface_grok_next_expression(uint8_t previous, uint32_t random_value);
uint8_t watchface_grok_blink_amount(uint16_t ms_in_second);
void watchface_grok_render(lv_layer_t *layer, const lv_area_t *area,
                           const watchface_data_t *data, bool preview);
