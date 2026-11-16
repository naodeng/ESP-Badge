#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { STARTUP_OK, STARTUP_TOUCH, STARTUP_APPS, STARTUP_STORAGE,
    STARTUP_MEMORY, STARTUP_UI, STARTUP_IMAGE } startup_problem_t;
typedef struct { startup_problem_t problem; esp_err_t error; } startup_result_t;

// Runs outside the LVGL lock. Network reachability and an unset RTC are not
// firmware validity requirements; UI/DMA and started image work must settle.
startup_result_t startup_selftest(bool touch_ready, bool launcher_ready, uint32_t first_heartbeat, uint32_t first_transfer, int64_t cover_started_us);
startup_result_t startup_wait_home(uint32_t first_heartbeat,uint32_t frame_request);
const char *startup_problem_name(startup_problem_t problem);
// Only PENDING_VERIFY images may be confirmed or rolled back. Confirmation is
// performed after the boot minimum, core check and home frame completion.
