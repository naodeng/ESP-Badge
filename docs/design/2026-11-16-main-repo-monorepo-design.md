# Jarvis_OS MOTO GPS 整合设计

设计日期：2026-11-16
状态：用户已确认产品范围与统一命名。

## 产品目标

以 ESP-Badge 主仓库为 Jarvis_OS 主项目，保留 Waveshare ESP32-S3-Touch-AMOLED-1.75C 的必要系统壳，并完整适配 MOTO GPS iOS App 的设备侧功能。交付范围是一个 Jarvis_OS 固件、一个 MOTO GPS iOS App、一个 Cloudflare 网关实现，以及开发调试工具。

## 三端职责

- **Jarvis_OS 固件**：圆屏 UI、触摸/按键、BLE 配对与消息协议、转向提示、路线进度/偏航状态、速度航向、周边地图显示和 App 控制命令。保留启动、电源、基础设置与导航所需驱动。
- **MOTO GPS iOS App**：定位、地点搜索、路线预览与规划、偏航重算、路况刷新、在线/离线地图缓存、Apple Music 控制；通过 BLE 将设备所需状态同步给 Jarvis_OS。
- **MOTO GPS 网关**：Node 本地运行实现和 Cloudflare Worker 实现都纳入仓库；生产 Worker 使用 AMap Web Service Secret 与私有 R2 瓦片缓存，UTC 03:17 定时刷新。

导航、地图、音乐等每个 App-to-device 能力都必须在 iOS BLE 协议与 Jarvis_OS GATT/消息处理之间有对应实现。位置搜索、路线计算、实时服务请求仍由 iOS 与网关负责。

## 统一命名与发布

- 主项目名称：`Jarvis_OS`。
- ESP-IDF `project()` 名称与正式固件产物前缀：`Jarvis_OS`，每次正式发布只产出一个固件文件。
- 不维护 stable/beta 两个产品固件、不使用两个 OTA 下载对象。源码可有本地 Debug/Release 构建配置，它们不是两个固件产品版本。
- iOS 伴侣仍显示为 `MOTO GPS`；Cloudflare Worker 仍叫 `moto-gps-gateway`。

## 代码范围与目录

```text
apps/ios/moto-gps/                 # MOTO GPS iOS App
firmware/jarvis_os/                # 唯一 Jarvis_OS ESP-IDF 工程
packages/moto-gps/shared/          # 导航核心、BLE/HTTP schema、离线地图资源
services/moto-gps-gateway/         # Node 与 Cloudflare Worker/R2 实现
tools/debug/jarvis-geektwin/       # 现有 BLE 调试器
tools/debug/moto-gps-simulator/    # WASM 圆屏模拟器
docs/                              # 架构、开发、部署、调试、流程和状态
licenses/moto-gps/                 # MOTO GPS LICENSE、NOTICE 和第三方许可
archive/legacy-jarvis/              # 从默认固件裁下的历史 App/工具源码
```

Jarvis GeekTwin 与 MOTO GPS WASM 模拟器仅供本地开发，不部署为产品网站，不进入 iOS 或固件发布包。MOTO GPS 的 Android App 和独立 ESP32 固件不纳入产品；现有独立 20-byte Travel BLE 协议由 MOTO GPS BLE 协议替代。

## 裁剪范围

- 正式固件只保留启动/待机时钟、亮度与必要设置、导航、BLE、触摸按键和导航所需传感器/显示支持。
- OTA、Pages/产品网站、GeekTwin 产品入口、Codex bridge、Weather、录音、音频娱乐、小游戏及其他与 MOTO GPS 无关的 App 不进入默认固件、安装包或发布流程；可恢复的旧源码归档到 `archive/legacy-jarvis/`。
- 保持分区表、NVS 命名空间和用户录音存储格式，不因移除入口而擦除设备数据。

## 当前部署基线

- Moto GPS Worker 已部署并使用 AMap provider 与 R2 地图缓存；具体地址属于部署者的 Cloudflare 账户配置，不写入仓库或 App 默认值。
- Cloudflare Secret `AMAP_WEB_SERVICE_KEY` 已存在；Secret 值不得复制进仓库。
- 私有 R2 桶 `moto-gps-map-cache` 已配置 `tiles/` 前缀 30 天过期。
- Worker Cron 为 `17 3 * * *` UTC；它属于地图源刷新，不是固件 OTA。

## 来源与许可

MOTO GPS 来源为 `mx3353672833-debug/moto-gps-waveshare`，基线 commit `2ddc164`。只迁入 iOS、网关、共享导航代码和两种调试工具所需源码；保留 PolyForm Noncommercial 1.0.0、NOTICE、第三方许可及 OSM 署名。soRound_os 来源与许可说明继续保留。生产 Key、签名凭据、Cloudflare 登录资料和设备配置均不入库。
