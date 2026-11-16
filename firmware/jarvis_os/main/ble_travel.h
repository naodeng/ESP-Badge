#pragma once
#include "travel_data.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool active, connected;
    travel_data_t data;
    travel_status_t status;
} travel_snapshot_t;

bool ble_travel_start(void);
void ble_travel_stop(void);
void ble_travel_snapshot(travel_snapshot_t *snapshot);
