# Jarvis_OS 产品范围

## 定位

Jarvis_OS 是运行在 Waveshare ESP32-S3-Touch-AMOLED-1.75C 上的圆屏系统。该主仓库同时维护 Moto GPS iOS App、服务端网关及设备端导航显示适配，三端作为同一个 Moto GPS 产品链路交付。

## 组成

### 固件

- ESP32-S3R8、32 MB Flash、8 MB PSRAM、466 × 466 圆形 AMOLED。
- ESP-IDF 5.5.x 与 LVGL。
- 保留系统启动、锁屏、时钟表盘、亮度和语言等基础设置，以及显示、触摸、RTC、IMU、BLE 驱动。
- Moto GPS 页面接收 iOS App 的路线、转向、进度、速度、连接状态和控制消息。
- 只发布一个 `Jarvis_OS` 固件；无 OTA 页面或设备端升级服务。

### Moto GPS iOS App

- 获取手机定位、搜索地点、规划路线、显示在线和离线地图。
- 通过 HTTPS 网关请求搜索、路线与地图数据。
- 通过加密 BLE 与 Jarvis_OS 交换导航数据和设备状态。
- 提供地图缓存、离线区域管理和 Apple Music 基础控制。

### Moto GPS 后端

- Node.js 服务用于本地开发和自托管。
- Cloudflare Worker 用于现有远端部署；AMap Web Service Key 作为 Worker Secret，地图缓存使用私有 R2。
- API 未启用用户身份认证时为公开接口，依靠 CORS 与边缘限流做基础保护；CORS 不构成访问认证。

### 调试工具

- Jarvis GeekTwin：用于调试设备 IMU 数值和 BLE 数字孪生链路。
- Moto GPS 模拟器：用于浏览器中预览圆屏 Moto GPS 导航 UI 和共享场景数据。
- Jarvis_OS USB 实屏预览：通过 USB Serial/JTAG 在本地浏览器查看设备当前画面，并保存截图或录制 WebM。它需要单独构建的调试固件。正式固件默认关闭屏幕镜像。
- 工具仅用于开发，不属于产品服务或网站部署内容。GeekTwin 使用旧的 IMU BLE 协议；当前最终固件的 MOTO GATT 服务与它不同，直接连接能力需要后续兼容适配。

## 不包含

- Android App。
- 独立 Moto GPS ESP32 固件。
- 产品网站、Pages 网站和公开调试站点。
- OTA 服务、Codex bridge、Wi-Fi 网络应用、旧 Travel BLE、天气、录音/音频和其他旧 soRound 应用。
- 上述历史实现不属于当前固件和发布包；上游归属、许可证与用户数据格式等必要材料继续保留。

## 体验与数据约束

- 界面按 466 × 466 圆屏安全区域设计，延续黑、白、灰、红的系统色彩约定。
- 连接、无数据、缓存、过期、离线和失败状态需明确表达，不以演示数据冒充在线状态。
- iOS App 是定位、地图与网关的责任端；固件只处理设备 UI、传感器辅助和 BLE 协议，不连接 Wi-Fi 或地图服务。
- 更新安装只覆盖应用槽。分区表、NVS、用户设置、配对信息和录音数据应保持兼容。

## 验收范围

- 固件可以在 ESP32-S3 目标上完整构建，启动日志无崩溃并初始化显示、触摸和 IMU。
- iPhone 真机能够配对 Jarvis_OS，完成连接、导航状态接收、断开与重连。
- 网关真实 AMap 请求、地图缓存、R2 与 Cron 配置按部署环境独立验收。
- iOS Simulator 可验证界面与不依赖 BLE 的流程；BLE、后台定位和音乐控制需真机验证。
