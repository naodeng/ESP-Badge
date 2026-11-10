#include "codex_http.h"
#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "codex_http";

int codex_http_get(const char *url, char *out, size_t cap)
{
    if (!url || !out || cap < 2) return -1;
    out[0] = '\0';

    esp_http_client_config_t cfg = { 0 };
    cfg.url = url;
    cfg.timeout_ms = 10000;
    esp_http_client_handle_t cl = esp_http_client_init(&cfg);
    if (!cl) return -2;

    int result = -3;
    if (esp_http_client_open(cl, 0) == ESP_OK) {
        esp_http_client_fetch_headers(cl);
        int total = 0;
        int r;
        while ((size_t)total < cap - 1 &&
               (r = esp_http_client_read(cl, out + total, (int)(cap - 1 - total))) > 0) {
            total += r;
        }
        out[total] = '\0';
        int status = esp_http_client_get_status_code(cl);
        if (status == 200 && total > 0) result = total;
        ESP_LOGI(TAG, "GET %s -> %d, %d bytes", url, status, total);
    }
    esp_http_client_cleanup(cl);
    return result;
}
