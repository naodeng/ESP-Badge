#include "codex_data.h"

#include "cJSON.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static cJSON *member(const cJSON *object, const char *name) {
    if (!cJSON_IsObject(object)) return NULL;
    return cJSON_GetObjectItemCaseSensitive(object, name);
}

static bool number_value(const cJSON *value, int64_t *out) {
    if (!cJSON_IsNumber(value) || !isfinite(value->valuedouble)) return false;
    if (value->valuedouble < (double)INT64_MIN || value->valuedouble >= 9223372036854775808.0 ||
        trunc(value->valuedouble) != value->valuedouble) return false;
    *out = (int64_t)value->valuedouble;
    return true;
}

static void bounded_string(char *out, size_t capacity, const cJSON *value, const char *fallback) {
    const char *source = cJSON_IsString(value) && value->valuestring && value->valuestring[0]
        ? value->valuestring : fallback;
    snprintf(out, capacity, "%s", source ? source : "");
}

static int optional_int(const cJSON *object, const char *name, int fallback, int minimum, int maximum) {
    int64_t number;
    if (!number_value(member(object, name), &number) || number < minimum || number > maximum) return fallback;
    return (int)number;
}

static int64_t optional_i64(const cJSON *object, const char *name, int64_t fallback, int64_t minimum) {
    int64_t number;
    if (!number_value(member(object, name), &number) || number < minimum) return fallback;
    return number;
}

void codex_snapshot_init(codex_snapshot_t *snapshot) {
    if (!snapshot) return;
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->p5_percent = -1;
    snapshot->p7_percent = -1;
    snapshot->reset_s = -1;
    snapshot->tokens = -1;
    snapshot->cost_cents = -1;
    snapshot->radar_stale = true;
    snprintf(snapshot->plan, sizeof(snapshot->plan), "--");
    snprintf(snapshot->activity_state, sizeof(snapshot->activity_state), "idle");
}

bool codex_snapshot_parse(const char *json, size_t length, codex_snapshot_t *out) {
    if (!json || !length || !out || length > 16384) return false;

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(json, length, &parse_end, 0);
    if (!root) return false;
    const char *end = json + length;
    while (parse_end && parse_end < end && (*parse_end == ' ' || *parse_end == '\t' ||
            *parse_end == '\r' || *parse_end == '\n')) ++parse_end;
    if (!cJSON_IsObject(root) || !parse_end || parse_end != end ||
        optional_int(root, "v", -1, 1, 1) != 1) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *usage = member(root, "usage");
    cJSON *activity = member(root, "activity");
    cJSON *radar = member(root, "radar");
    if (!cJSON_IsObject(usage) || !cJSON_IsObject(activity) || !cJSON_IsObject(radar)) {
        cJSON_Delete(root);
        return false;
    }

    codex_snapshot_t parsed;
    codex_snapshot_init(&parsed);
    parsed.p5_percent = optional_int(usage, "p5", -1, 0, 100);
    parsed.p7_percent = optional_int(usage, "p7", -1, 0, 100);
    parsed.reset_s = optional_int(usage, "reset_s", -1, 0, INT32_MAX);
    parsed.tokens = optional_i64(usage, "tok", -1, 0);
    parsed.cost_cents = optional_i64(usage, "cost_c", -1, 0);
    bounded_string(parsed.plan, sizeof(parsed.plan), member(usage, "plan"), "--");
    bounded_string(parsed.activity_state, sizeof(parsed.activity_state),
                    member(activity, "state"), "idle");
    parsed.active_tasks = optional_int(activity, "active_tasks", 0, 0, INT32_MAX);
    parsed.snapshot_ts_s = optional_i64(root, "ts", 0, 0);
    parsed.radar_updated_s = optional_i64(radar, "updated", 0, 0);
    cJSON *radar_stale = member(radar, "stale");
    parsed.radar_stale = cJSON_IsBool(radar_stale) ? cJSON_IsTrue(radar_stale) : parsed.radar_updated_s == 0;

    cJSON *rows = member(radar, "rows");
    if (cJSON_IsArray(rows)) {
        const cJSON *row = NULL;
        cJSON_ArrayForEach(row, rows) {
            if (parsed.radar_count == CODEX_RADAR_MAX) break;
            cJSON *model = member(row, "m");
            cJSON *epoch = member(row, "e");
            int64_t iq;
            if (!cJSON_IsString(model) || !model->valuestring || !model->valuestring[0] ||
                !cJSON_IsString(epoch) || !epoch->valuestring || !epoch->valuestring[0] ||
                !number_value(member(row, "iq"), &iq) || iq < 0 || iq > INT32_MAX) continue;
            codex_radar_row_t *target = &parsed.radar[parsed.radar_count++];
            bounded_string(target->model, sizeof(target->model), model, "");
            bounded_string(target->epoch, sizeof(target->epoch), epoch, "");
            target->iq = (int)iq;
        }
    }
    parsed.has_snapshot = true;
    parsed.online = true;
    parsed.stale = false;
    *out = parsed;
    cJSON_Delete(root);
    return true;
}

void codex_snapshot_mark_offline(codex_snapshot_t *snapshot, const char *reason) {
    if (!snapshot) return;
    snapshot->online = false;
    bounded_string(snapshot->error, sizeof(snapshot->error), NULL,
                   reason && reason[0] ? reason : "bridge offline");
}

bool codex_snapshot_is_stale(const codex_snapshot_t *snapshot, int64_t now_ms) {
    if (!snapshot || !snapshot->has_snapshot || snapshot->last_success_ms <= 0 ||
        now_ms < snapshot->last_success_ms) return false;
    return now_ms - snapshot->last_success_ms >= 60000;
}
