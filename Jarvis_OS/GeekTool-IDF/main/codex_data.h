#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CODEX_RADAR_MAX 16
#define CODEX_MODEL_MAX 32
#define CODEX_EPOCH_MAX 12
#define CODEX_PLAN_MAX 24
#define CODEX_STATE_MAX 16
#define CODEX_ERROR_MAX 48

typedef struct {
    char model[CODEX_MODEL_MAX];
    char epoch[CODEX_EPOCH_MAX];
    int iq;
} codex_radar_row_t;

typedef struct {
    int p5_percent;
    int p7_percent;
    int reset_s;
    int64_t tokens;
    int64_t cost_cents;
    char plan[CODEX_PLAN_MAX];
    char activity_state[CODEX_STATE_MAX];
    int active_tasks;
    int64_t snapshot_ts_s;
    int64_t radar_updated_s;
    bool radar_stale;
    codex_radar_row_t radar[CODEX_RADAR_MAX];
    size_t radar_count;
    bool has_snapshot;
    bool online;
    bool stale;
    int64_t last_success_ms;
    char error[CODEX_ERROR_MAX];
} codex_snapshot_t;

void codex_snapshot_init(codex_snapshot_t *snapshot);
// Parse the bridge schema transactionally. On failure, out is left untouched.
bool codex_snapshot_parse(const char *json, size_t length, codex_snapshot_t *out);
// Connectivity changes must not discard the last valid usage/activity/radar values.
void codex_snapshot_mark_offline(codex_snapshot_t *snapshot, const char *reason);
bool codex_snapshot_is_stale(const codex_snapshot_t *snapshot, int64_t now_ms);
