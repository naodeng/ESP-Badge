# 部署与发布

## 1. 构建单一 Jarvis_OS 固件

采用 ESP-IDF 5.5.x 和隔离的临时构建配置：

```sh
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
export IDF_TARGET=esp32s3
source .toolchain/esp-idf/export.sh
idf.py -C firmware/jarvis_os \
  -B /tmp/jarvis-os-release-build \
  -D SDKCONFIG=/tmp/jarvis-os-release-sdkconfig \
  build

mkdir -p release
cp /tmp/jarvis-os-release-build/Jarvis_OS.bin release/Jarvis_OS.bin
shasum -a 256 release/Jarvis_OS.bin > release/Jarvis_OS.bin.sha256
```

正式输出只有 `release/Jarvis_OS.bin` 一个固件文件；SHA-256 旁文件用于安装前核对。二进制被 `.gitignore` 忽略，可通过上述命令重建。构建不应使用或覆盖仓库内忽略的本机 `sdkconfig`。

## 2. 在当前设备安全安装

当前设备已识别为 ESP32-S3，分区布局包含 `ota_0` (`0x20000`，8 MiB)、`ota_1`、NVS、storage 和录音分区。此次更新只写 `ota_0` 应用地址：

```sh
python -m esptool --chip esp32s3 -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_size 32MB --flash_freq 80m \
  0x20000 release/Jarvis_OS.bin
```

运行前先按 `ls /dev/cu.usbmodem*` 确认端口；如设备端口不同，先确认是目标 Waveshare 板卡。**不要把 `idf.py flash` 或 `flash_args` 用于这次保留用户数据的更新**：常规完整烧录会同时写 bootloader、分区表和其他数据区域。本命令只包含一个写入地址，不擦除 NVS、配对信息、用户设置、storage 或录音。

刷写结束后连接串口监视器，检查芯片启动、`Project name: Jarvis_OS`、显示/触摸/IMU 初始化和启动自检结果。任何启动循环或 panic 都应先停止重复刷写并保存串口日志。

## 3. 部署 Moto GPS 后端

Node.js 本地服务、Cloudflare Worker、AMap Secret、R2、Cron、健康检查及 Key 白名单的详细步骤见[后端服务部署指南](../backend/deployment.md)。

仓库和 App 安装包不提供共享的默认网关地址。部署者应从自己的 Cloudflare Dashboard 获取 Worker 地址，再在 iPhone App 的“网关设置”中填写。健康检查只确认 Worker 状态，不验证高德 Key 的实时搜索或路线调用。

## 4. 配置 iPhone App

网关地址配置、运行时更换、Xcode 签名和 iOS 权限见[手机 App 配置指南](../mobile-app/configuration.md)。首次使用时在首页齿轮 → **网关设置**填写自己的 HTTPS 根地址。

手机与 Jarvis_OS 圆屏配对和导航同步步骤见[MOTO GPS 连接指南](../connection-guide.md)。
