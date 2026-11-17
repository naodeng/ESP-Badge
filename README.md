# Jarvis_OS

Jarvis_OS 是一个面向 Waveshare ESP32-S3-Touch-AMOLED-1.75C 的圆屏系统，并在同一主仓库维护 Moto GPS iOS App、网关后端和设备侧 BLE 导航适配。

## 产品范围

- **Jarvis_OS 固件**：圆屏启动与锁屏、时钟表盘、亮度和必要系统设置、Moto GPS 导航显示、BLE 与触摸/按键驱动。
- **Moto GPS iOS App**：定位、地点搜索、路线与地图、离线缓存、Apple Music 控制；通过 BLE 向设备发送导航状态。
- **Moto GPS 网关**：Node.js 本地服务和 Cloudflare Worker 实现，为 App 提供搜索、路线和地图接口。
- **开发调试**：Jarvis GeekTwin 硬件调试器、Moto GPS WASM 模拟器和 Jarvis_OS USB 实屏预览。它们只用于本地开发，不作为产品网站发布。

本项目不包含 Android App、独立 Moto GPS ESP 固件、产品网站、OTA 服务或旧 Codex / Travel 等应用。上游来源说明及必要许可证仍保留。

## 仓库结构

```text
apps/ios/moto-gps/             iOS 伴侣 App
firmware/jarvis_os/            唯一 ESP-IDF 固件工程
services/moto-gps-gateway/     Node.js 与 Cloudflare Worker 网关
packages/moto-gps/shared/      BLE 协议、导航核心、共享资源和地图数据
tools/debug/                   三个本地调试工具
licenses/moto-gps/             Moto GPS 来源、许可证和第三方声明
docs/                          架构、开发、部署、调试与过程文档
archive/legacy-jarvis/         已从当前产品和固件中移除的历史工具/原型
Jarvis_OS/                     保留的 soRound_os 上游来源说明
release/                       单一发布固件及校验和（产物被 Git 忽略）
```

## 构建固件

需要 ESP-IDF 5.5.x。以下使用仓库本机工具链，并把构建目录与本机 `sdkconfig` 放在 `/private/tmp`：

```sh
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
export IDF_TARGET=esp32s3
source .toolchain/esp-idf/export.sh
idf.py -C firmware/jarvis_os \
  -B /private/tmp/jarvis-os-release-build \
  -D SDKCONFIG=/private/tmp/jarvis-os-release-sdkconfig \
  build
```

发布时只生成并分发 `release/Jarvis_OS.bin` 一个固件文件。当前板卡保留既有双 OTA 槽分区布局，但本固件没有 OTA 应用入口；安装只写活动应用槽，**不写 bootloader、分区表、NVS、storage、录音或用户数据分区**。首次刷写或分区迁移必须另行核对设备型号和数据备份。完整流程见[部署文档](docs/deployment/README.md)。

## 开发入口

- [项目架构](docs/architecture/system-overview.md)
- [开发环境与构建](docs/development/README.md)
- [固件部署和设备安装](docs/deployment/README.md)
- [手机 App 配置](docs/mobile-app/configuration.md)
- [后端服务部署](docs/backend/deployment.md)
- [MOTO GPS 连接指南](docs/connection-guide.md)
- [调试与设备验收](docs/debugging/README.md)
- [Jarvis_OS USB 实屏预览](tools/debug/jarvis-screen-viewer/README.md)
- [项目过程与迭代约定](docs/process/README.md)
- [当前状态和已知验证边界](docs/project-status.md)
- [第三方来源与许可](THIRD_PARTY.md)

后端 Worker 的 Secret 只通过 Wrangler/Cloudflare 控制台输入，不写入聊天、源码或配置文件。仓库不内置维护者的个人 Worker 地址；每位使用者应在 iOS App 中配置自己有权使用的网关。部署流程见[部署文档](docs/deployment/README.md)。
