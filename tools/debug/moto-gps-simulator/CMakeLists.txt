cmake_minimum_required(VERSION 3.16)
project(moto_gps_simulator LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this target with emcmake")
endif()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(LVGL_DIR "${REPO_ROOT}/firmware/jarvis_os/managed_components/lvgl__lvgl"
    CACHE PATH "LVGL source installed from the pinned ESP-IDF dependency")
set(LV_CONF_PATH "${REPO_ROOT}/firmware/jarvis_os/tests/host/lv_conf.h"
    CACHE FILEPATH "LVGL host configuration")

if(NOT EXISTS "${LVGL_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "LVGL not found at ${LVGL_DIR}. Build firmware dependencies first or set LVGL_DIR.")
endif()
if(NOT EXISTS "${LV_CONF_PATH}")
    message(FATAL_ERROR "LV_CONF_PATH not found: ${LV_CONF_PATH}")
endif()

add_subdirectory("${LVGL_DIR}" "${CMAKE_BINARY_DIR}/third_party/lvgl")
add_subdirectory("${REPO_ROOT}/packages/moto-gps/shared/nav_presenter"
                 "${CMAKE_BINARY_DIR}/moto_nav_presenter")
add_subdirectory("${REPO_ROOT}/packages/moto-gps/shared/nav_app"
                 "${CMAKE_BINARY_DIR}/moto_nav_app")
add_subdirectory(wasm)
