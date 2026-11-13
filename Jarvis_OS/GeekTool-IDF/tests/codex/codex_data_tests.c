#include "codex_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char valid_json[] =
    "{\"v\":1,\"ts\":1794470400,"
    "\"usage\":{\"p5\":32,\"p7\":70,\"reset_s\":19800,\"tok\":128700,\"cost_c\":0,\"plan\":\"Plus\"},"
    "\"activity\":{\"state\":\"running\",\"active_tasks\":1},"
    "\"radar\":{\"updated\":1794469900,\"stale\":false,"
    "\"rows\":[{\"m\":\"gpt-5-codex\",\"e\":\"high\",\"iq\":1372}]}}";

static void test_parses_bridge_snapshot(void) {
    codex_snapshot_t snapshot;
    codex_snapshot_init(&snapshot);
    assert(codex_snapshot_parse(valid_json, sizeof valid_json - 1, &snapshot));
    assert(snapshot.has_snapshot);
    assert(snapshot.online);
    assert(snapshot.p5_percent == 32);
    assert(snapshot.p7_percent == 70);
    assert(snapshot.reset_s == 19800);
    assert(snapshot.tokens == 128700);
    assert(snapshot.cost_cents == 0);
    assert(strcmp(snapshot.plan, "Plus") == 0);
    assert(strcmp(snapshot.activity_state, "running") == 0);
    assert(snapshot.active_tasks == 1);
    assert(snapshot.radar_count == 1);
    assert(strcmp(snapshot.radar[0].model, "gpt-5-codex") == 0);
    assert(strcmp(snapshot.radar[0].epoch, "high") == 0);
    assert(snapshot.radar[0].iq == 1372);
}

static void test_null_windows_are_unavailable(void) {
    const char json[] =
        "{\"v\":1,\"usage\":{\"p5\":null,\"p7\":null},"
        "\"activity\":{},\"radar\":{\"rows\":[]}}";
    codex_snapshot_t snapshot;
    codex_snapshot_init(&snapshot);
    assert(codex_snapshot_parse(json, sizeof json - 1, &snapshot));
    assert(snapshot.p5_percent == -1);
    assert(snapshot.p7_percent == -1);
    assert(snapshot.reset_s == -1);
    assert(snapshot.tokens == -1);
    assert(snapshot.cost_cents == -1);
    assert(strcmp(snapshot.plan, "--") == 0);
    assert(strcmp(snapshot.activity_state, "idle") == 0);
}

static void test_parse_failure_preserves_snapshot(void) {
    codex_snapshot_t snapshot;
    codex_snapshot_init(&snapshot);
    assert(codex_snapshot_parse(valid_json, sizeof valid_json - 1, &snapshot));
    snapshot.last_success_ms = 123456;
    codex_snapshot_t before = snapshot;
    const char malformed[] = "{\"v\":1,\"usage\":{";
    assert(!codex_snapshot_parse(malformed, sizeof malformed - 1, &snapshot));
    assert(memcmp(&snapshot, &before, sizeof snapshot) == 0);
    const char wrong_shape[] = "[]";
    assert(!codex_snapshot_parse(wrong_shape, sizeof wrong_shape - 1, &snapshot));
    assert(memcmp(&snapshot, &before, sizeof snapshot) == 0);
    const char trailing_data[] = "{\"v\":1,\"usage\":{},\"activity\":{},\"radar\":{}} garbage";
    assert(!codex_snapshot_parse(trailing_data, sizeof trailing_data - 1, &snapshot));
    assert(memcmp(&snapshot, &before, sizeof snapshot) == 0);
}

static void test_non_integer_percentages_are_unavailable(void) {
    const char json[] =
        "{\"v\":1,\"usage\":{\"p5\":35.5,\"p7\":100.5},"
        "\"activity\":{},\"radar\":{\"rows\":[]}}";
    codex_snapshot_t snapshot;
    codex_snapshot_init(&snapshot);
    assert(codex_snapshot_parse(json, sizeof json - 1, &snapshot));
    assert(snapshot.p5_percent == -1);
    assert(snapshot.p7_percent == -1);
}

static void test_radar_is_bounded_to_sixteen_rows(void) {
    char json[4096];
    size_t used = (size_t)snprintf(json, sizeof json,
        "{\"v\":1,\"usage\":{},\"activity\":{},\"radar\":{\"rows\":[");
    for (int i = 0; i < 20; ++i) {
        int written = snprintf(json + used, sizeof json - used,
            "%s{\"m\":\"model-%d\",\"e\":\"high\",\"iq\":%d}",
            i ? "," : "", i, 1200 + i);
        assert(written > 0 && (size_t)written < sizeof json - used);
        used += (size_t)written;
    }
    int written = snprintf(json + used, sizeof json - used, "]}}");
    assert(written > 0 && (size_t)written < sizeof json - used);
    used += (size_t)written;

    codex_snapshot_t snapshot;
    codex_snapshot_init(&snapshot);
    assert(codex_snapshot_parse(json, used, &snapshot));
    assert(snapshot.radar_count == CODEX_RADAR_MAX);
    assert(strcmp(snapshot.radar[15].model, "model-15") == 0);
}

static void test_offline_keeps_cached_data_and_reports_staleness(void) {
    codex_snapshot_t snapshot;
    codex_snapshot_init(&snapshot);
    assert(codex_snapshot_parse(valid_json, sizeof valid_json - 1, &snapshot));
    snapshot.last_success_ms = 1000;
    codex_snapshot_mark_offline(&snapshot, "bridge offline");
    assert(snapshot.has_snapshot);
    assert(!snapshot.online);
    assert(snapshot.p5_percent == 32);
    assert(snapshot.radar_count == 1);
    assert(strcmp(snapshot.error, "bridge offline") == 0);
    assert(!codex_snapshot_is_stale(&snapshot, 60999));
    assert(codex_snapshot_is_stale(&snapshot, 61000));
}

int main(void) {
    test_parses_bridge_snapshot();
    test_null_windows_are_unavailable();
    test_non_integer_percentages_are_unavailable();
    test_parse_failure_preserves_snapshot();
    test_radar_is_bounded_to_sixteen_rows();
    test_offline_keeps_cached_data_and_reports_staleness();
    puts("Codex snapshot tests passed");
    return 0;
}
