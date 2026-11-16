#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define TRAVEL_FRAME_SIZE 20
#define TRAVEL_MAX_AGE_MS 5000u
// Pure parser; callers serialize access. No location coordinates or persistent storage.
typedef enum { TRAVEL_WAITING, TRAVEL_LIVE, TRAVEL_STALE } travel_status_t;
typedef struct {
    bool present, speed_valid, heading_valid, accuracy_valid, demo;
    uint16_t sequence, speed_cms, heading_cdeg, accuracy_dm, age_ms;
    uint32_t session, received_ms;
} travel_data_t;
void travel_data_reset(travel_data_t *state);
bool travel_data_accept(travel_data_t *state, const uint8_t *frame, size_t len, uint32_t now_ms);
travel_status_t travel_data_status(const travel_data_t *state, uint32_t now_ms);
bool travel_data_heading_valid(const travel_data_t *state);
