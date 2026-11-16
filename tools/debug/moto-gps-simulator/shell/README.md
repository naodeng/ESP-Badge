> **语言 / Language:** 中文 · [English](README.en.md)

# Moto GPS 圆屏调试外壳

这是本地开发用浏览器控制台。圆屏导航 UI 由共享 LVGL 代码编译为 WASM 后绘制；外壳只模拟导航/网络/播放状态，不是产品网站，也不代替 iPhone BLE。

## 构建 WASM

要求 ESP-IDF 5.5.x 的依赖已安装（用于获取锁定版本的 LVGL）、Emscripten SDK 与 CMake。先在仓库根目录构建过一次固件，或将 `LVGL_DIR` 指向匹配的 LVGL 9.6 源码：

```sh
emcmake cmake -S tools/debug/moto-gps-simulator \
  -B /tmp/moto-gps-wasm \
  -DLVGL_DIR="$PWD/firmware/jarvis_os/managed_components/lvgl__lvgl" \
  -DLV_CONF_PATH="$PWD/firmware/jarvis_os/tests/host/lv_conf.h"
cmake --build /tmp/moto-gps-wasm -j
```

生成文件在 `tools/debug/moto-gps-simulator/shell/runtime/`，此目录被 Git 忽略。该模拟器不访问真实位置、AMap 或 Worker；其路线、状态和场景来自本地调试输入。

## 运行

```sh
python3 -m http.server 4173 \
  --directory tools/debug/moto-gps-simulator/shell
```

用桌面浏览器打开 `http://127.0.0.1:4173/`。不要用 `file://` 打开 WASM。`?demo=1` 可显示固定示例场景；`?deviceState=offline`、`connecting`、`success`、`ready` 和 `planning` 可手动检查连接状态页面。

固定画布为 466 × 466。更改共享 UI 时同时检查实体圆屏与该模拟器；模拟器显示正常不代表触摸、BLE 或 ESP32 渲染已验证。
