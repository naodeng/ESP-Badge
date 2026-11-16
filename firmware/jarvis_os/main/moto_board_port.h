#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool board_port_lock(uint32_t timeout_ms);
void board_port_unlock(void);

#ifdef __cplusplus
}
#endif
