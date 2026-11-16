set(MOTO_SHARED_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../packages/moto-gps/shared")

idf_component_register(
    SRCS
        "main.c"
        "display.c"
        "launcher.c"
        "power.c"
        "watchface.c"
        "watchface_catalog.c"
        "watchface_ui.c"
        "watchface_grok.c"
        "watchface_instrument.c"
        "watchface_scenes.c"
        "watchface_graphic.c"
        "watchface_atelier.c"
        "watchface_type.c"
        "watchface_sport.c"
        "watchface_geometry.c"
        "watchface_studio_draw.c"
        "watchface_backgrounds.c"
        "font_watchface.c"
        "font_watchface_hands.c"
        "font_watchface_studio.c"
        "font_watchface_type.c"
        "lock.c"
        "glyph.c"
        "buttons.c"
        "i18n.c"
        "font_cn16.c"
        "quickpanel.c"
        "tools_ui.c"
        "font_tools.c"
        "launcher_icons.c"
        "companion_icons.c"
        "font_companion.c"
        "control_ui.c"
        "font_control.c"
        "font_location_24.c"
        "identity_ui.c"
        "font_identity.c"
        "startup.c"
        "settings.c"
        "ui_persistence.c"
        "app_settings.c"
        "app_watchfaces.c"
        "watchface_picker.c"
        "weather_artwork.c"
        "weather_frame.c"
        "weather_refresh.c"
        "img_store.c"
        "resource_store.c"
        "rtc.c"
        "imu.c"
        "app_moto_gps.cpp"
        "ble_nav_transport_nimble.cpp"
        "phone_nav_bridge.cpp"
        "${MOTO_SHARED_ROOT}/ble_protocol/src/ble_protocol.cpp"
        "${MOTO_SHARED_ROOT}/coordinates/coordinates.cpp"
        "${MOTO_SHARED_ROOT}/nav_core/nav_core.cpp"
        "${MOTO_SHARED_ROOT}/nav_presenter/src/moto_nav_presenter.cpp"
        "${MOTO_SHARED_ROOT}/nav_ui/src/moto_nav_ui.cpp"
        "${MOTO_SHARED_ROOT}/nav_ui/assets/moto_font_nav_16.c"
    INCLUDE_DIRS
        "."
        "${MOTO_SHARED_ROOT}"
        "${MOTO_SHARED_ROOT}/ble_protocol/include"
        "${MOTO_SHARED_ROOT}/coordinates/include"
        "${MOTO_SHARED_ROOT}/nav_presenter/include"
        "${MOTO_SHARED_ROOT}/nav_ui/include"
        "${MOTO_SHARED_ROOT}/demo_fixture"
    REQUIRES
        esp_lcd
        esp_driver_spi
        esp_driver_i2c
        esp_driver_gpio
        esp_lvgl_port
        nvs_flash
        esp_app_format
        esp_timer
        esp_psram
        spi_flash
        esp_hw_support
        esp_system
        esp_common
        bt
        fatfs
        esp_jpeg
        wear_levelling
)

# Keep raw-image generation available to developers, but never include it in
# the default flash target: field updates write the app slot only and preserve
# the user's storage partition.
fatfs_create_rawflash_image(storage ../images)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation -Wno-missing-field-initializers)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
