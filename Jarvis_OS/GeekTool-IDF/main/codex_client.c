#include "codex_client.h"

#include "esp_netif.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "network_http.h"
#include "wifi_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CODEX_BRIDGE_PORT 8790
#define CODEX_POLL_MS 10000
#define CODEX_BODY_CAP 8192
#define CODEX_WORKER_STACK 6144

static const char *TAG = "codex";
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static codex_snapshot_t s_snapshot;
static TaskHandle_t s_worker;
static bool s_initialized;
static volatile bool s_visible;
static volatile uint32_t s_generation;

static void snapshot_init_once(void) {
    portENTER_CRITICAL(&s_lock);
    if (!s_initialized) {
        codex_snapshot_init(&s_snapshot);
        s_initialized = true;
    }
    portEXIT_CRITICAL(&s_lock);
}

static uint32_t generation_get(void) {
    portENTER_CRITICAL(&s_lock);
    uint32_t generation = s_generation;
    portEXIT_CRITICAL(&s_lock);
    return generation;
}

static bool request_active(void *user) {
    uint32_t generation = (uint32_t)(uintptr_t)user;
    return s_visible && generation == generation_get();
}

static void snapshot_offline(const char *reason) {
    char message[CODEX_ERROR_MAX];
    snprintf(message,sizeof message,"%s",reason&&reason[0]?reason:"Bridge offline");
    portENTER_CRITICAL(&s_lock);
    bool changed=strcmp(s_snapshot.error,message)!=0;
    codex_snapshot_mark_offline(&s_snapshot, message);
    portEXIT_CRITICAL(&s_lock);
    if(changed)ESP_LOGW(TAG,"%s",message);
}

static bool discover_bridge(char *ip, size_t capacity, uint32_t generation) {
    if (!ip || capacity < 8 || !request_active((void *)(uintptr_t)generation)) return false;
    int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) return false;

    int enabled = 1;
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 900000};
    (void)setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
    (void)setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in targets[2] = {0};
    targets[0].sin_family = AF_INET;
    targets[0].sin_port = htons(CODEX_BRIDGE_PORT);
    targets[0].sin_addr.s_addr = INADDR_BROADCAST;
    size_t target_count = 1;

    // Some access points drop 255.255.255.255 while still allowing subnet
    // broadcasts. Try the Wi-Fi interface's directed broadcast as a fallback.
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info = {0};
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK &&
        ip_info.ip.addr != 0 && ip_info.netmask.addr != 0) {
        uint32_t directed_broadcast = ip_info.ip.addr | ~ip_info.netmask.addr;
        if (directed_broadcast != INADDR_BROADCAST) {
            targets[target_count] = targets[0];
            targets[target_count].sin_addr.s_addr = directed_broadcast;
            ++target_count;
        }
    }

    static const char hello[] = "AGENTBADGE/1";
    char reply[48] = {0};
    struct sockaddr_in peer = {0};
    socklen_t peer_len = sizeof(peer);
    int received = -1;
    for (size_t i = 0; i < target_count && request_active((void *)(uintptr_t)generation); ++i) {
        bool sent = sendto(socket_fd, hello, sizeof(hello) - 1, 0,
                           (struct sockaddr *)&targets[i], sizeof(targets[i])) ==
                    (ssize_t)(sizeof(hello) - 1);
        if (!sent) continue;
        peer_len = sizeof(peer);
        received = recvfrom(socket_fd, reply, sizeof(reply) - 1, 0,
                            (struct sockaddr *)&peer, &peer_len);
        if (received > 0) break;
    }
    close(socket_fd);
    if (received <= 0) return false;
    reply[received] = 0;
    for (int i = 0; i < received; ++i) {
        if (reply[i] == '\r' || reply[i] == '\n' || reply[i] == ' ' || reply[i] == '\t') {
            reply[i] = 0;
            break;
        }
    }
    struct in_addr parsed_ip;
    if (!reply[0] || inet_pton(AF_INET, reply, &parsed_ip) != 1) return false;
    snprintf(ip, capacity, "%s", reply);
    return true;
}

static void fetch_snapshot(uint32_t generation) {
    if (!wifi_service_ready()) {
        snapshot_offline("Wi-Fi unavailable");
        return;
    }

    char bridge_ip[48];
    if (!discover_bridge(bridge_ip, sizeof(bridge_ip), generation)) {
        if (request_active((void *)(uintptr_t)generation)) snapshot_offline("Bridge not found");
        return;
    }
    ESP_LOGI(TAG,"bridge discovered at %s",bridge_ip);

    char url[96];
    snprintf(url, sizeof(url), "http://%s:%d/codex.json", bridge_ip, CODEX_BRIDGE_PORT);
    char *body = heap_caps_malloc(CODEX_BODY_CAP, MALLOC_CAP_8BIT);
    if (!body) {
        snapshot_offline("Out of memory");
        return;
    }

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
        .buffer_size = 2048,
        .buffer_size_tx = 512,
        .keep_alive_enable = false,
    };
    network_http_result_t result = network_http_get(TAG, &config, body, CODEX_BODY_CAP,
                                                     8000, request_active,
                                                     (void *)(uintptr_t)generation);
    if (result.stage == NET_HTTP_OK && request_active((void *)(uintptr_t)generation)) {
        codex_snapshot_t parsed;
        codex_snapshot_init(&parsed);
        if (codex_snapshot_parse(body, result.bytes, &parsed)) {
            parsed.last_success_ms = esp_timer_get_time() / 1000;
            portENTER_CRITICAL(&s_lock);
            s_snapshot = parsed;
            portEXIT_CRITICAL(&s_lock);
            ESP_LOGI(TAG, "bridge snapshot: %u Radar rows, %d active task(s)",
                     (unsigned)parsed.radar_count, parsed.active_tasks);
        } else {
            snapshot_offline("Invalid bridge response");
        }
    } else if (result.stage != NET_HTTP_CANCELED && request_active((void *)(uintptr_t)generation)) {
        snapshot_offline(result.stage == NET_HTTP_OFFLINE ? "Wi-Fi unavailable" : "Bridge request failed");
    }
    free(body);
}

static void client_worker(void *arg) {
    (void)arg;
    for (;;) {
        if (!s_visible) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }
        uint32_t generation = generation_get();
        fetch_snapshot(generation);
        if (s_visible) (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CODEX_POLL_MS));
    }
}

static bool worker_start(void) {
    if (s_worker) return true;
    if (xTaskCreate(client_worker, "codex_client", CODEX_WORKER_STACK, NULL, 3, &s_worker) == pdPASS)
        return true;
    s_worker = NULL;
    return false;
}

void codex_client_set_visible(bool visible) {
    snapshot_init_once();
    portENTER_CRITICAL(&s_lock);
    s_visible = visible;
    if (!visible) ++s_generation;
    portEXIT_CRITICAL(&s_lock);
    ESP_LOGI(TAG,"client visible=%d",visible);
    if (visible && !worker_start()) snapshot_offline("Client unavailable");
    TaskHandle_t worker = s_worker;
    if (worker) xTaskNotifyGive(worker);
}

void codex_client_request_refresh(void) {
    TaskHandle_t worker = s_worker;
    if (s_visible && worker) xTaskNotifyGive(worker);
}

void codex_client_snapshot(codex_snapshot_t *out) {
    if (!out) return;
    snapshot_init_once();
    portENTER_CRITICAL(&s_lock);
    *out = s_snapshot;
    portEXIT_CRITICAL(&s_lock);
    out->stale = codex_snapshot_is_stale(out, esp_timer_get_time() / 1000);
}
