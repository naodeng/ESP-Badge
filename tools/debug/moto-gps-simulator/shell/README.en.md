# Moto GPS circular-display debug shell

This browser console is for local development. The shared LVGL navigation UI is rendered through WASM; the shell simulates navigation/network/playback state. It is not a product website and does not replace iPhone BLE.

## Build WASM

Requires ESP-IDF 5.5.x dependencies (to provide the pinned LVGL source), the Emscripten SDK, and CMake. Build firmware dependencies once from the repository root, or set `LVGL_DIR` to matching LVGL 9.6 source:

```sh
emcmake cmake -S tools/debug/moto-gps-simulator \
  -B /tmp/moto-gps-wasm \
  -DLVGL_DIR="$PWD/firmware/jarvis_os/managed_components/lvgl__lvgl" \
  -DLV_CONF_PATH="$PWD/firmware/jarvis_os/tests/host/lv_conf.h"
cmake --build /tmp/moto-gps-wasm -j
```

Generated files go to `tools/debug/moto-gps-simulator/shell/runtime/`, which Git ignores. The simulator does not use real location, AMap, or the Worker; routes and states come from local debug input.

## Run

```sh
python3 -m http.server 4173 \
  --directory tools/debug/moto-gps-simulator/shell
```

Open `http://127.0.0.1:4173/` in a desktop browser. Do not use `file://` for WASM. `?demo=1` shows a fixed scene; `?deviceState=offline`, `connecting`, `success`, `ready`, or `planning` lets you inspect connection states.

The canvas is 466 × 466. Check the physical display and simulator after shared UI changes. A successful simulator render does not verify touch, BLE, or ESP32 rendering.
