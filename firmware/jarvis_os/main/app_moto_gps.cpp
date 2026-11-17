extern "C" {
#include "app.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "imu.h"
LV_FONT_DECLARE(font_music_cjk_24);
}

#include "esp_lvgl_port.h"

#include <cmath>
#include <cstdint>

#include "ble_nav_transport_nimble.h"
#include "motion_heading_axis.hpp"
#include "moto_nav_presenter.hpp"
#include "moto_nav_ui.h"
#include "phone_nav_bridge.h"

namespace {

constexpr uint32_t kLvglLockTimeoutMs = 10'000;
constexpr TickType_t kNavigationTick = pdMS_TO_TICKS(25);

moto::nav::NavPresenter s_presenter;
PhoneNavBridge s_bridge(s_presenter);
BleNavTransport s_transport;
lv_obj_t* s_screen = nullptr;
bool s_renderer_started = false;
bool s_transport_attempted = false;
bool s_demo_task_started = false;
bool s_motion_task_started = false;

std::uint64_t monotonic_ms() {
  return static_cast<std::uint64_t>(esp_timer_get_time()) / 1'000U;
}

moto::ble::AckStatus receive_phone_message(
    const moto::ble::ReassembledMessage& message, void* context) {
  return static_cast<PhoneNavBridge*>(context)->on_message(message);
}

void update_phone_link(bool active, void* context) {
  static_cast<PhoneNavBridge*>(context)->on_link_state(active);
}

void navigation_demo_task(void* context) {
  auto* bridge = static_cast<PhoneNavBridge*>(context);
  while (true) {
    bridge->update_demo(monotonic_ms());
    vTaskDelay(kNavigationTick);
  }
}

void motion_heading_task(void* context) {
  auto* bridge = static_cast<PhoneNavBridge*>(context);
  float bias_sum = 0.0F;
  int stable_samples = 0;
  const std::uint64_t calibration_started_ms = monotonic_ms();
  float accel_x = 0.0F;
  float accel_y = 0.0F;
  float accel_z = 1.0F;
  float gyro_x = 0.0F;
  float gyro_y = 0.0F;
  float gyro_z = 0.0F;
  TickType_t next_tick = xTaskGetTickCount();

  while (stable_samples < 120 &&
         monotonic_ms() - calibration_started_ms < 4'000U) {
    if (imu_read_accel(&accel_x, &accel_y, &accel_z) &&
        imu_read_gyro(&gyro_x, &gyro_y, &gyro_z)) {
      const float rate = clockwise_gravity_heading_rate(
          accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
      const float angular = std::sqrt(gyro_x * gyro_x + gyro_y * gyro_y +
                                      gyro_z * gyro_z);
      if (std::isfinite(rate) && angular < 4.0F) {
        bias_sum += rate;
        ++stable_samples;
      } else {
        bias_sum = 0.0F;
        stable_samples = 0;
      }
    }
    xTaskDelayUntil(&next_tick, kNavigationTick);
  }

  float bias_dps = stable_samples > 0 ? bias_sum / stable_samples : 0.0F;
  ESP_LOGI("moto_heading", "gyro yaw assist ready; bias=%.3f dps samples=%d",
           static_cast<double>(bias_dps), stable_samples);
  next_tick = xTaskGetTickCount();
  while (true) {
    if (imu_read_accel(&accel_x, &accel_y, &accel_z) &&
        imu_read_gyro(&gyro_x, &gyro_y, &gyro_z)) {
      const float raw_rate = clockwise_gravity_heading_rate(
          accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
      const float corrected_rate = raw_rate - bias_dps;
      const float angular = std::sqrt(gyro_x * gyro_x + gyro_y * gyro_y +
                                      gyro_z * gyro_z);
      const float accel = std::sqrt(accel_x * accel_x + accel_y * accel_y +
                                    accel_z * accel_z);
      if (std::abs(corrected_rate) < 1.1F && angular < 2.0F &&
          accel > 0.82F && accel < 1.16F) {
        bias_dps += (raw_rate - bias_dps) * 0.0008F;
      }
      bridge->on_imu_sample(corrected_rate, monotonic_ms());
    }
    xTaskDelayUntil(&next_tick, kNavigationTick);
  }
}

void app_enter(lv_obj_t* parent) {
  if (parent == nullptr) return;
  for (lv_indev_t* indev = lv_indev_get_next(nullptr); indev != nullptr;
       indev = lv_indev_get_next(indev)) {
    if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
      lv_indev_set_gesture_min_distance(indev, 48);
    }
  }

  if (s_screen != parent) {
    moto_nav_ui_set_music_font_fallback(&font_music_cjk_24);
    moto_nav_ui_create_in(parent, 0U);
    s_screen = parent;
    s_bridge.install_ui_callbacks();

    if (!s_renderer_started) {
      s_renderer_started = s_bridge.start_renderer();
    }

    if (!s_transport_attempted) {
      s_transport_attempted = true;
      s_bridge.set_sender(BleNavTransport::send_from_bridge, &s_transport);
      s_transport.set_callbacks(receive_phone_message, update_phone_link,
                                &s_bridge);
      const esp_err_t result = s_transport.start();
      if (result != ESP_OK) {
        ESP_LOGE("moto_gps", "BLE navigation service failed: %s",
                 esp_err_to_name(result));
      }
    }

    if (!s_demo_task_started &&
        xTaskCreate(navigation_demo_task, "moto_demo", 6'144, &s_bridge, 2,
                    nullptr) == pdPASS) {
      s_demo_task_started = true;
    }

    if (!s_motion_task_started &&
        xTaskCreate(motion_heading_task, "moto_heading", 4'096, &s_bridge, 3,
                    nullptr) == pdPASS) {
      s_motion_task_started = true;
    }
  } else {
    s_bridge.install_ui_callbacks();
  }

  moto_nav_ui_set_visible(1U);
  launcher_set_title("MOTO GPS");
}

void app_visibility(bool visible) {
  moto_nav_ui_set_visible(visible ? 1U : 0U);
}

void app_exit() {
  // BLE and its renderer stay alive while the launcher is shown. The launcher
  // retains this screen object so the background renderer never holds stale
  // LVGL pointers.
  moto_nav_ui_set_visible(0U);
  for (lv_indev_t* indev = lv_indev_get_next(nullptr); indev != nullptr;
       indev = lv_indev_get_next(indev)) {
    if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
      lv_indev_set_gesture_min_distance(indev, 64);
    }
  }
}

void app_gesture(lv_dir_t direction) {
  if (direction == LV_DIR_LEFT) {
    moto_nav_ui_handle_gesture(MOTO_UI_GESTURE_LEFT);
  } else if (direction == LV_DIR_RIGHT) {
    moto_nav_ui_handle_gesture(MOTO_UI_GESTURE_RIGHT);
  }
  lv_indev_t* input = lv_indev_active();
  if (input != nullptr) lv_indev_wait_release(input);
}

}  // namespace

extern "C" bool board_port_lock(uint32_t timeout_ms) {
  return lvgl_port_lock(timeout_ms == UINT32_MAX ? kLvglLockTimeoutMs
                                                  : timeout_ms);
}

extern "C" void board_port_unlock(void) { lvgl_port_unlock(); }

extern "C" const app_t app_moto_gps = {
    "MOTO GPS", COL_TXT, app_enter, nullptr, app_exit, nullptr, 0,
    app_visibility,
    false, app_gesture, true};
