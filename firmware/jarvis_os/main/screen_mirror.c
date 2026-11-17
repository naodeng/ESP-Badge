#include "screen_mirror.h"

#if defined(CONFIG_JARVIS_SCREEN_MIRROR) && CONFIG_JARVIS_SCREEN_MIRROR

#include "board_config.h"
#include "driver/usb_serial_jtag.h"
#include "esp_heap_caps.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#define MIRROR_SLOT_COUNT 12u
#define MIRROR_MAX_ROWS 40u
#define MIRROR_SLOT_BYTES (LCD_H_RES * MIRROR_MAX_ROWS * 2u)
#define MIRROR_ENCODED_MAX_BYTES (MIRROR_SLOT_BYTES + (((MIRROR_SLOT_BYTES / 2u) + 127u) / 128u) + 1u)
#define MIRROR_LOG_CHUNK_BYTES 360u
#define MIRROR_END_SLOT UINT8_MAX
#define MIRROR_CLIENT_TIMEOUT_US (6000000LL)

static const char *TAG = "screen_mirror";
static portMUX_TYPE s_resync_mux = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
    uint32_t generation;
    uint32_t frame;
    uint16_t rectangle, x, y, width, height;
    size_t length;
    bool dropped;
    uint8_t pixels[MIRROR_SLOT_BYTES];
} mirror_slot_t;

typedef struct {
    uint8_t slot;
    uint32_t generation;
    uint32_t frame;
    uint16_t rectangle_count;
    bool keyframe_start;
    bool dropped;
} mirror_item_t;

static mirror_slot_t *s_slots;
static uint8_t *s_encoded_buffer;
static QueueHandle_t s_free_slots;
static QueueHandle_t s_pending;
static lv_display_t *s_display;
static volatile bool s_active;
static volatile bool s_ready;
static volatile bool s_resync_pending;
static volatile int64_t s_last_client_us;
static volatile uint32_t s_unreported_drops;
static volatile uint32_t s_tx_generation;
static uint32_t s_current_frame;
static uint16_t s_current_rectangle;
static bool s_keyframe_pending;
static bool s_current_frame_dropped;

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_MAX;
    for(size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for(unsigned bit = 0; bit < 8; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static size_t base64_encode(const uint8_t *source, size_t length, char *destination)
{
    size_t in = 0, out = 0;
    while(in + 3 <= length) {
        uint32_t value = ((uint32_t)source[in] << 16) |
                         ((uint32_t)source[in + 1] << 8) | source[in + 2];
        destination[out++] = B64[(value >> 18) & 63];
        destination[out++] = B64[(value >> 12) & 63];
        destination[out++] = B64[(value >> 6) & 63];
        destination[out++] = B64[value & 63];
        in += 3;
    }
    if(in < length) {
        uint32_t value = (uint32_t)source[in] << 16;
        bool has_second = in + 1 < length;
        if(has_second) value |= (uint32_t)source[in + 1] << 8;
        destination[out++] = B64[(value >> 18) & 63];
        destination[out++] = B64[(value >> 12) & 63];
        destination[out++] = has_second ? B64[(value >> 6) & 63] : '=';
        destination[out++] = '=';
    }
    destination[out] = '\0';
    return out;
}

static bool same_pixel(const uint8_t *pixels, size_t left, size_t right)
{
    return pixels[left * 2u] == pixels[right * 2u] &&
           pixels[left * 2u + 1u] == pixels[right * 2u + 1u];
}

// RGB565 packet RLE: 0x00..0x7f is 1..128 literal pixels; 0x80..0xff is
// 3..130 copies of the following pixel. Literal encoding is bounded to less
// than 0.4% overhead, so high-entropy artwork safely falls back to raw data.
static size_t encode_rle(const uint8_t *source, size_t length, uint8_t *destination,
                         size_t capacity)
{
    if((length & 1u) != 0) return 0;
    size_t pixel_count = length / 2u;
    size_t pixel = 0;
    size_t output = 0;
    while(pixel < pixel_count) {
        size_t run = 1;
        while(run < 130u && pixel + run < pixel_count && same_pixel(source, pixel, pixel + run)) ++run;
        if(run >= 3u) {
            if(output + 3u > capacity) return 0;
            destination[output++] = (uint8_t)(0x80u | (run - 3u));
            destination[output++] = source[pixel * 2u];
            destination[output++] = source[pixel * 2u + 1u];
            pixel += run;
            continue;
        }

        size_t literal_start = pixel;
        size_t literal_pixels = 0;
        while(pixel < pixel_count && literal_pixels < 128u) {
            run = 1;
            while(run < 130u && pixel + run < pixel_count && same_pixel(source, pixel, pixel + run)) ++run;
            if(run >= 3u && literal_pixels != 0) break;
            size_t take = run >= 3u ? 1u : run;
            size_t remaining = 128u - literal_pixels;
            if(take > remaining) take = remaining;
            pixel += take;
            literal_pixels += take;
            if(run >= 3u) break;
        }
        size_t literal_bytes = literal_pixels * 2u;
        if(literal_pixels == 0 || output + 1u + literal_bytes > capacity) return 0;
        destination[output++] = (uint8_t)(literal_pixels - 1u);
        memcpy(destination + output, source + literal_start * 2u, literal_bytes);
        output += literal_bytes;
    }
    return output;
}

static bool can_send(uint32_t generation)
{
    return __atomic_load_n(&s_active, __ATOMIC_ACQUIRE) &&
           !__atomic_load_n(&s_resync_pending, __ATOMIC_ACQUIRE) &&
           generation == __atomic_load_n(&s_tx_generation, __ATOMIC_RELAXED) &&
           usb_serial_jtag_is_connected();
}

static void send_slot(const mirror_slot_t *slot, uint32_t generation)
{
    char encoded[481];
    size_t encoded_length = encode_rle(slot->pixels, slot->length,
                                       s_encoded_buffer, MIRROR_ENCODED_MAX_BYTES);
    const uint8_t *source = s_encoded_buffer;
    char encoding = 'R';
    if(encoded_length == 0 || encoded_length >= slot->length) {
        source = slot->pixels;
        encoded_length = slot->length;
        encoding = 'N';
    }
    size_t offset = 0;
    uint32_t checksum = crc32(slot->pixels, slot->length);
    while(offset < encoded_length) {
        if(!can_send(generation)) return;
        size_t count = encoded_length - offset;
        if(count > MIRROR_LOG_CHUNK_BYTES) count = MIRROR_LOG_CHUNK_BYTES;
        base64_encode(source + offset, count, encoded);
        ESP_LOGI(TAG, "MIRROR|P|%u|%u|%u|%u|%u|%u|%u|%u|%u|%08x|%c|%s",
                 (unsigned)slot->frame, (unsigned)slot->rectangle,
                 (unsigned)slot->x, (unsigned)slot->y,
                 (unsigned)slot->width, (unsigned)slot->height,
                 (unsigned)offset, (unsigned)encoded_length, (unsigned)slot->length,
                 (unsigned)checksum, encoding, encoded);
        offset += count;
    }
}

static void send_frame_end(const mirror_item_t *item)
{
    if(!can_send(item->generation)) return;
    ESP_LOGI(TAG, "MIRROR|E|%u|%u|%u", (unsigned)item->frame,
             (unsigned)item->rectangle_count, item->dropped ? 1u : 0u);
}

static void schedule_resync(bool report_drop)
{
    portENTER_CRITICAL(&s_resync_mux);
    if(!s_resync_pending) {
        __atomic_add_fetch(&s_tx_generation, 1u, __ATOMIC_RELAXED);
        __atomic_store_n(&s_resync_pending, true, __ATOMIC_RELEASE);
    }
    if(report_drop) __atomic_add_fetch(&s_unreported_drops, 1u, __ATOMIC_RELAXED);
    portEXIT_CRITICAL(&s_resync_mux);
}

static void stop_stream(void)
{
    portENTER_CRITICAL(&s_resync_mux);
    __atomic_store_n(&s_active, false, __ATOMIC_RELEASE);
    __atomic_add_fetch(&s_tx_generation, 1u, __ATOMIC_RELAXED);
    __atomic_store_n(&s_resync_pending, false, __ATOMIC_RELEASE);
    portEXIT_CRITICAL(&s_resync_mux);
}

static void drain_pending_queue(void)
{
    mirror_item_t item;
    while(xQueueReceive(s_pending, &item, 0) == pdTRUE) {
        if(item.slot != MIRROR_END_SLOT) xQueueSend(s_free_slots, &item.slot, 0);
    }
}

static bool request_full_refresh(void)
{
    if(!s_display || !lvgl_port_lock(3000)) {
        ESP_LOGW(TAG, "MIRROR|S|LOCK_TIMEOUT");
        return false;
    }
    // The LVGL lock excludes a flush callback that started before resync was
    // requested. Drop its now-stale items before admitting the keyframe.
    drain_pending_queue();
    s_current_rectangle = 0;
    s_current_frame_dropped = false;
    s_keyframe_pending = true;
    lv_obj_invalidate(lv_screen_active());
    lv_obj_invalidate(lv_layer_top());
    portENTER_CRITICAL(&s_resync_mux);
    __atomic_store_n(&s_resync_pending, false, __ATOMIC_RELEASE);
    portEXIT_CRITICAL(&s_resync_mux);
    lvgl_port_unlock();
    return true;
}

static void mirror_tx_task(void *arg)
{
    (void)arg;
    mirror_item_t item;
    for(;;) {
        if(!__atomic_load_n(&s_active, __ATOMIC_ACQUIRE)) {
            drain_pending_queue();
            vTaskDelay(pdMS_TO_TICKS(50));
        } else if(__atomic_load_n(&s_resync_pending, __ATOMIC_ACQUIRE)) {
            drain_pending_queue();
            if(!request_full_refresh()) vTaskDelay(pdMS_TO_TICKS(100));
        } else if(xQueueReceive(s_pending, &item, pdMS_TO_TICKS(100)) == pdTRUE) {
            if(item.slot == MIRROR_END_SLOT) {
                send_frame_end(&item);
                continue;
            }
            mirror_slot_t *slot = &s_slots[item.slot];
            if(can_send(item.generation)) {
                if(item.keyframe_start) ESP_LOGI(TAG, "MIRROR|K|%u", (unsigned)item.frame);
                send_slot(slot, item.generation);
            }
            xQueueSend(s_free_slots, &item.slot, portMAX_DELAY);
        }

        uint32_t drops = __atomic_exchange_n(&s_unreported_drops, 0, __ATOMIC_RELAXED);
        if(drops && usb_serial_jtag_is_connected()) {
            ESP_LOGW(TAG, "MIRROR|X|%u", (unsigned)drops);
        }
    }
}

static void accept_command(const char *command)
{
    if(strcmp(command, "START") == 0) {
        __atomic_store_n(&s_last_client_us, esp_timer_get_time(), __ATOMIC_RELAXED);
        __atomic_store_n(&s_active, true, __ATOMIC_RELEASE);
        ESP_LOGI(TAG, "MIRROR|S|STARTING");
        schedule_resync(false);
    } else if(strcmp(command, "SNAP") == 0) {
        __atomic_store_n(&s_last_client_us, esp_timer_get_time(), __ATOMIC_RELAXED);
        __atomic_store_n(&s_active, true, __ATOMIC_RELEASE);
        schedule_resync(false);
    } else if(strcmp(command, "PING") == 0) {
        __atomic_store_n(&s_last_client_us, esp_timer_get_time(), __ATOMIC_RELAXED);
    } else if(strcmp(command, "STOP") == 0) {
        stop_stream();
        ESP_LOGI(TAG, "MIRROR|S|STOPPED");
    }
}

static void mirror_rx_task(void *arg)
{
    (void)arg;
    char line[24];
    size_t length = 0;
    for(;;) {
        int value = fgetc(stdin);
        if(value == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if(value == '\r' || value == '\n') {
            if(length) {
                line[length] = '\0';
                accept_command(line);
                length = 0;
            }
        } else if(length + 1 < sizeof(line)) {
            line[length++] = (char)value;
        } else {
            length = 0;
        }
    }
}

static void mirror_watchdog_task(void *arg)
{
    (void)arg;
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if(__atomic_load_n(&s_active, __ATOMIC_ACQUIRE)) {
            int64_t last = __atomic_load_n(&s_last_client_us, __ATOMIC_RELAXED);
            if(esp_timer_get_time() - last > MIRROR_CLIENT_TIMEOUT_US) {
                stop_stream();
                ESP_LOGI(TAG, "MIRROR|S|CLIENT_TIMEOUT");
            }
        }
    }
}

void screen_mirror_init(lv_display_t *display)
{
    s_display = display;
    s_current_frame = 1;
    s_current_rectangle = 0;
    s_slots = heap_caps_calloc(MIRROR_SLOT_COUNT, sizeof(*s_slots),
                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_encoded_buffer = heap_caps_malloc(MIRROR_ENCODED_MAX_BYTES,
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_free_slots = xQueueCreate(MIRROR_SLOT_COUNT, sizeof(uint8_t));
    s_pending = xQueueCreate(MIRROR_SLOT_COUNT + 4u, sizeof(mirror_item_t));
    if(!s_slots || !s_encoded_buffer || !s_free_slots || !s_pending) {
        ESP_LOGE(TAG, "Screen mirror allocation failed; mirror disabled");
        if(s_slots) heap_caps_free(s_slots);
        if(s_encoded_buffer) heap_caps_free(s_encoded_buffer);
        if(s_free_slots) vQueueDelete(s_free_slots);
        if(s_pending) vQueueDelete(s_pending);
        s_slots = NULL;
        s_encoded_buffer = NULL;
        s_free_slots = NULL;
        s_pending = NULL;
        return;
    }

    for(uint8_t i = 0; i < MIRROR_SLOT_COUNT; ++i) xQueueSend(s_free_slots, &i, 0);
    TaskHandle_t tx_task = NULL, rx_task = NULL, watchdog_task = NULL;
    BaseType_t tx_ok = xTaskCreate(mirror_tx_task, "screen_mirror_tx", 4096, NULL, 2, &tx_task);
    BaseType_t rx_ok = tx_ok == pdPASS ?
        xTaskCreate(mirror_rx_task, "screen_mirror_rx", 3072, NULL, 2, &rx_task) : pdFAIL;
    BaseType_t watchdog_ok = rx_ok == pdPASS ?
        xTaskCreate(mirror_watchdog_task, "screen_mirror_watchdog", 2048, NULL, 2, &watchdog_task) : pdFAIL;
    if(tx_ok != pdPASS || rx_ok != pdPASS || watchdog_ok != pdPASS) {
        ESP_LOGE(TAG, "Screen mirror task creation failed");
        if(tx_task) vTaskDelete(tx_task);
        if(rx_task) vTaskDelete(rx_task);
        if(watchdog_task) vTaskDelete(watchdog_task);
        if(s_slots) heap_caps_free(s_slots);
        if(s_encoded_buffer) heap_caps_free(s_encoded_buffer);
        if(s_free_slots) vQueueDelete(s_free_slots);
        if(s_pending) vQueueDelete(s_pending);
        s_slots = NULL;
        s_encoded_buffer = NULL;
        s_free_slots = NULL;
        s_pending = NULL;
        return;
    }
    s_ready = true;
    ESP_LOGI(TAG, "MIRROR|S|READY");
}

void screen_mirror_flush(lv_display_t *display, const lv_area_t *area,
                         const uint8_t *pixels, bool is_last)
{
    (void)display;
    if(!s_ready || !__atomic_load_n(&s_active, __ATOMIC_ACQUIRE)) return;

    if(__atomic_load_n(&s_resync_pending, __ATOMIC_ACQUIRE)) {
        if(is_last) {
            ++s_current_frame;
            s_current_rectangle = 0;
            s_current_frame_dropped = false;
        }
        return;
    }

    uint32_t frame = s_current_frame;
    uint32_t generation = __atomic_load_n(&s_tx_generation, __ATOMIC_RELAXED);
    int width = area->x2 - area->x1 + 1;
    int height = area->y2 - area->y1 + 1;
    uint16_t rectangle = s_current_rectangle++;
    bool valid = width > 0 && height > 0 && area->x1 >= 0 && area->y1 >= 0 &&
                 area->x2 < LCD_H_RES && area->y2 < LCD_V_RES && height <= MIRROR_MAX_ROWS &&
                 (size_t)width * (size_t)height * 2u <= MIRROR_SLOT_BYTES;
    uint8_t slot_index = MIRROR_END_SLOT;

    if(valid && xQueueReceive(s_free_slots, &slot_index, 0) == pdTRUE) {
        mirror_slot_t *slot = &s_slots[slot_index];
        size_t row_bytes = (size_t)width * 2u;
        size_t source_stride = lv_draw_buf_width_to_stride((uint32_t)width,
                                                           LV_COLOR_FORMAT_RGB565_SWAPPED);
        slot->generation = generation;
        slot->frame = frame;
        slot->rectangle = rectangle;
        slot->x = (uint16_t)area->x1;
        slot->y = (uint16_t)area->y1;
        slot->width = (uint16_t)width;
        slot->height = (uint16_t)height;
        slot->length = row_bytes * (size_t)height;
        for(int row = 0; row < height; ++row) {
            memcpy(slot->pixels + (size_t)row * row_bytes,
                   pixels + (size_t)row * source_stride, row_bytes);
        }
        mirror_item_t item = {
            .slot = slot_index,
            .generation = generation,
            .frame = frame,
            .rectangle_count = 0,
            .keyframe_start = s_keyframe_pending,
            .dropped = false,
        };
        if(xQueueSend(s_pending, &item, 0) != pdTRUE) {
            xQueueSend(s_free_slots, &slot_index, 0);
            s_current_frame_dropped = true;
            schedule_resync(true);
        } else if(item.keyframe_start) s_keyframe_pending = false;
    } else {
        s_current_frame_dropped = true;
        schedule_resync(true);
    }

    if(is_last && !__atomic_load_n(&s_resync_pending, __ATOMIC_ACQUIRE)) {
        mirror_item_t end = {
            .slot = MIRROR_END_SLOT,
            .generation = generation,
            .frame = frame,
            .rectangle_count = s_current_rectangle,
            .keyframe_start = false,
            .dropped = s_current_frame_dropped,
        };
        if(xQueueSend(s_pending, &end, 0) != pdTRUE) {
            schedule_resync(true);
        }
    }

    if(is_last) {
        ++s_current_frame;
        s_current_rectangle = 0;
        s_current_frame_dropped = false;
    }
}

#else

void screen_mirror_init(lv_display_t *display)
{
    (void)display;
}

void screen_mirror_flush(lv_display_t *display, const lv_area_t *area,
                         const uint8_t *pixels, bool is_last)
{
    (void)display;
    (void)area;
    (void)pixels;
    (void)is_last;
}

#endif
