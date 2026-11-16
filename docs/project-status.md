# Jarvis_OS 项目状态

更新日期：2026-11-16

## 当前范围

- 单一项目名和固件名：`Jarvis_OS`。
- 产品组成：Jarvis_OS 固件、Moto GPS iOS App、Node/Cloudflare 网关、共享 Moto GPS 协议/导航代码。
- 开发工具：Jarvis GeekTwin 与 Moto GPS WASM 模拟器；它们不发布为网站。
- 不包括 Android、独立 Moto GPS ESP 固件、OTA、Codex/Travel/音频/天气等旧固件应用。
- 上游来源、许可、NOTICE 和地图署名仍保留。

## 本轮构建和设备工作

- ESP-IDF 5.5.4，ESP32-S3，隔离临时配置干净构建成功。
- 固件文件名 `Jarvis_OS.bin`，大小 `0x321EC0`（3,284,672 bytes），8 MiB app 槽余量约 61%。
- 发布文件 SHA-256：`f6c8b21f8fea49a45e8669f4f825a8e6cfb90169dca214dd237b10a08803c894`。
- 已刷写至已核验设备 `0x20000` 的 `ota_0` 应用槽。esptool 校验成功；刷写没有覆盖分区表、NVS、设置、配对信息、storage 或录音。
- 串口启动达到 `startup core=OK`、`startup check=OK`、`Jarvis_OS up`；CO5300、CST9217 和 QMI8658 初始化成功；LVGL watchdog 约 10 秒仍有 heartbeat，未看到 panic/reboot。
- 串口可见硬件初始化与首页首帧；本轮未手动触摸页面，也未连接 iPhone 验证导航 BLE。

## 验收状态

| 项目 | 状态 |
|---|---|
| Jarvis_OS 固件干净构建 | 已通过 |
| 固件尺寸进入 8 MiB 应用槽 | 已确认 |
| 设备 app 槽刷写与镜像校验 | 已通过，地址 `0x20000` |
| 设备串口启动与启动自检 | 已通过，约 10 秒无 panic |
| 屏幕实物显示和首帧 | 启动日志通过；未做相机/目视检查 |
| 触摸手势手动操作 | 未测 |
| iOS Simulator 构建/启动 | 本轮之前已完成；不验证真实 BLE |
| Node/Cloudflare 本地测试 | 项目调整前已运行通过；路径搬迁后未重跑 |
| 当前固件 BLE 与真实 iPhone 配对 | 待真机验收 |
| 实物屏幕检查、触摸手势、锁屏/唤醒 | 需人工目视与触摸操作；启动日志不能代替此项 |
| 真实 AMap 请求与 Worker Secret | 需在部署账户复核 |
| Worker 线上版本、R2 绑定和 Cron | 本轮只读云端检查未成功；需在 Cloudflare 控制台确认 |
| GeekTwin 连接当前固件 | 尚不兼容；旧 IMU GATT 与当前 Moto GPS GATT 不同 |

## 发布记录

最终二进制、哈希和刷写结果以仓库的 `release/` 文件及本页后续更新为准。不要把临时构建目录或本机 `sdkconfig` 当成正式发布材料。
