# 系统总览

Jarvis_OS 是主产品；Moto GPS iOS App、服务端与设备端适配在同一个主仓库中开发和交付。

```text
手机定位 / 搜索 / 路线
          │
          ├── HTTPS ──> Moto GPS 网关 ──> AMap / Protomaps / 私有 R2
          │
          └── 加密 BLE ──> Jarvis_OS ──> 圆屏导航指示 / 速度 / 连接状态
                                  ▲
                                  │
                       packages/moto-gps/shared
```

## 固件

工程入口为 `firmware/jarvis_os/`，设备为 Waveshare ESP32-S3-Touch-AMOLED-1.75C。正式启动器保留基础系统设置、表盘和 MOTO GPS 页面。设备端负责圆屏渲染、触摸/按键、IMU 辅助航向与 BLE 消息处理，不直接请求互联网服务。

## iOS App

`apps/ios/moto-gps/` 是定位、路线和地图的主交互端。App 用 HTTPS 访问网关，用加密 BLE 将设备所需导航状态发给 Jarvis_OS。仓库和安装包不提供共享的默认网关地址；每位使用者在 App 网关设置中填写自己有权使用的服务地址。

## 网关

`services/moto-gps-gateway/` 包含 Node 本地服务器和 `cloudflare/` Worker 部署。AMap Key 仅放在 Worker Secret 或本地忽略的 `.env`/`.dev.vars` 中。Cloudflare 版本使用私有 R2 缓存地图资源，Cron 配置在 `wrangler.jsonc`。

## 共享代码和调试

`packages/moto-gps/shared/` 管理 BLE 协议、坐标、导航核心、Presenter、共享 UI、示例路线和离线地图。协议 fixture 是两端联调时的格式参照。

`tools/debug/` 中的 GeekTwin 与 Moto GPS 模拟器仅供开发使用，不作为生产网站发布。GeekTwin 目前仍使用旧的 IMU BLE 协议；当前 Jarvis_OS 固件的 MOTO GATT 协议不同，因此需要专门适配后才能用于当前固件。

更完整的模块边界、生命周期与数据兼容约束见根目录 [design.md](../../design.md)。
