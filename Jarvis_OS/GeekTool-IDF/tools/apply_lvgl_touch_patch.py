#!/usr/bin/env python3
"""Apply Jarvis_OS' CST9217 transient-read handling to esp_lvgl_port 2.9.0."""

from pathlib import Path
import sys


COMPONENT_COMMIT = "f0ef9497efce684997ce391edd19733483e250a5"
COMPONENT_ROOT = Path(__file__).resolve().parents[1] / "managed_components/espressif__esp_lvgl_port"
MANIFEST = COMPONENT_ROOT / "idf_component.yml"
SOURCE = COMPONENT_ROOT / "src/lvgl9/esp_lvgl_port_touch.c"

UPSTREAM = """    /* Read data from touch controller into memory */
    ESP_ERROR_CHECK(esp_lcd_touch_read_data(touch_ctx->handle));

    /* Read data from touch controller */
    ESP_ERROR_CHECK(esp_lcd_touch_get_data(touch_ctx->handle, touch_data, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS));"""

PATCHED = """    /* Read data from touch controller into memory. A transient invalid ACK
     * from CST9217 is a dropped sample, not a reason to abort the whole app. */
    esp_err_t err = esp_lcd_touch_read_data(touch_ctx->handle);
    if (err == ESP_ERR_INVALID_RESPONSE) {
        err = esp_lcd_touch_read_data(touch_ctx->handle);
    }
    if (err != ESP_OK) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    /* Read data from touch controller */
    err = esp_lcd_touch_get_data(touch_ctx->handle, touch_data, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    if (err != ESP_OK) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }"""


def main() -> int:
    if not MANIFEST.is_file() or not SOURCE.is_file():
        print("esp_lvgl_port is missing; configure the ESP-IDF project first.", file=sys.stderr)
        return 1

    manifest = MANIFEST.read_text(encoding="utf-8")
    if COMPONENT_COMMIT not in manifest:
        print(
            "Unsupported esp_lvgl_port source revision. Review this patch before applying it.",
            file=sys.stderr,
        )
        return 1

    source = SOURCE.read_text(encoding="utf-8")
    if PATCHED in source:
        print("CST9217 transient-read handling is already applied.")
        return 0
    if UPSTREAM not in source:
        print("Expected upstream touch-read block was not found; no files changed.", file=sys.stderr)
        return 1

    SOURCE.write_text(source.replace(UPSTREAM, PATCHED, 1), encoding="utf-8")
    print("Applied CST9217 transient-read handling to esp_lvgl_port.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
