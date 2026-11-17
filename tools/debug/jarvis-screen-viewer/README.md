# Jarvis_OS USB 屏幕查看器

这个本地调试工具从真实设备的 LVGL 刷新链路读取屏幕更新，在浏览器里显示 466 × 466 圆屏画面。它通过 ESP32-S3 的 USB Serial/JTAG 工作，不启用 Wi-Fi。

## 要求

- ESP-IDF 5.5.x，使用仓库内 `.toolchain/esp-idf/` 环境。
- ESP32-S3 设备通过 USB 连接电脑。
- 最新版 Chrome 或 Edge。浏览器必须通过 `localhost` 打开页面，Web Serial 不支持直接用 `file://` 打开。

## 构建和安装调试固件

先在仓库根目录运行：

```sh
bash tools/debug/jarvis-screen-viewer/build-mirror-firmware.sh
```

构建目录和本机 sdkconfig 写入 `/tmp`。该配置把 USB Serial/JTAG 设为主控制台以接收预览命令，并启用 `JARVIS_SCREEN_MIRROR`。标准 `sdkconfig.defaults` 保持不变，普通构建不会打开镜像功能。

把生成的 `/tmp/jarvis-os-screen-mirror-build/Jarvis_OS.bin` 按[部署说明](../../../docs/deployment/README.md)写入设备的应用分区。**仅安装应用镜像，不要使用完整 `idf.py flash`，也不要覆盖 bootloader、分区表、NVS、storage 或录音分区。**本地镜像传输会与调试日志共用 USB 串口，预览期间不要同时打开 `idf.py monitor`；本页面会保留普通日志。

## 打开预览

在仓库根目录启动本地页面服务：

```sh
python3 -m http.server 8765 --directory tools/debug/jarvis-screen-viewer
```

在 Chrome 或 Edge 打开 [http://localhost:8765](http://localhost:8765)，点击“连接设备”并选择 ESP32-S3 USB 串口，然后点击“开始预览”。设备会完整刷新一次屏幕，之后只传输 LVGL 的局部更新。

- **保存截图**：请求一次完整刷新，收到完整画面后下载 PNG。
- **开始录制**：记录浏览器预览为 WebM；再次点击后停止并下载。
- **停止预览**：停止向电脑发送屏幕更新；设备 USB 串口仍可用于日志。

屏幕块会先做 RGB565 游程压缩，再通过 USB 串口传输；纯色和大面积背景会显著减少数据量。动画大幅重绘时，设备会丢弃积压的旧画面并自动重新同步当前页面；查看器也会在发现区域校验失败时自动请求重同步。录制按浏览器预览的实际更新画面输出，不承诺固定实时帧率。

## 恢复普通固件

结束预览后，按仓库[部署说明](../../../docs/deployment/README.md)把常规 `Jarvis_OS.bin` 只写入当前活动 app 槽。镜像选项默认关闭；不需要改动本机 `sdkconfig` 或固件用户数据。
