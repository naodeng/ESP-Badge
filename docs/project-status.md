# Jarvis_OS 项目状态

更新日期：2026-11-17

## 当前范围

- 单一项目名和固件名：`Jarvis_OS`。
- 产品组成：Jarvis_OS 固件、Moto GPS iOS App、Node/Cloudflare 网关、共享 Moto GPS 协议/导航代码。
- 开发工具：Jarvis GeekTwin、Moto GPS WASM 模拟器与 Jarvis_OS USB 实屏预览；它们不发布为网站。
- USB 实屏预览使用独立调试固件和本地浏览器，正式固件默认关闭镜像功能。预览可以查看设备当前画面、保存 PNG 和录制 WebM。
- 不包括 Android、独立 Moto GPS ESP 固件、OTA、Codex/Travel/音频/天气等旧固件应用。
- 上游来源、许可、NOTICE 和地图署名仍保留。

## 本轮构建和设备工作

- ESP-IDF 5.5.4，ESP32-S3，使用仓库外 `/tmp` 构建目录和 sdkconfig 干净构建成功；地图昼夜色板及页面可见状态定时器已编译进固件。
- 本轮修复了 MOTO GPS BLE 广播包超过传统广播长度上限的问题：服务 UUID 和 Flags 留在主广播包，`Jarvis_OS` 完整名称放入扫描响应。
- MOTO GPS 页面隐藏启动器全局标题；本轮进一步移除了所有 App 上的全局返回箭头，改由 PWR 长按返回，避免遮挡页面标题。
- 本轮新增屏保上滑解锁：从屏幕下半部向上移动至少约 80 像素才解锁；轻触和短滑不解锁，BOOT 短按仍可备用解锁。
- 本轮音乐页切换为 Source Han Sans SC 常用 GB2312 字符字库（8,178 个字形，18px/2bpp）；用户提供的“说好幸福呢 / 浪花兄弟”所含汉字均已核对进入字形索引。仍需用户在实机确认显示效果。
- 本轮为 MOTO GPS 所有页面子控件启用手势冒泡，确保从指南针刻度和文字区域滑动也能翻页；MOTO GPS 内滑动触发门槛降至 48 px，退出 App 后恢复为系统默认 64 px。
- 最新构建固件文件名 `Jarvis_OS.bin`，大小 `0x3c01b0`（3,932,592 bytes），8 MiB app 槽余量约 53%。
- 最新发布文件 SHA-256：`dfe60afa69ad84d0bb317489d8e8f0eeaf6f4340caec793ab20e0a5182227187`。
- 最终镜像已更新 `release/Jarvis_OS.bin` 并刷入设备 `ota_0` 的应用地址 `0x20000`；esptool 写入后 `Hash of data verified`。本次只写应用镜像，没有覆盖 bootloader、分区表、NVS、设置、配对信息、storage 或录音。
- 刷入最终镜像前的中间构建串口曾达到 `Project name: Jarvis_OS`、CO5300 面板就绪、CST9217 466×466 触摸就绪、`startup core=OK`、`startup check=OK` 和 `Jarvis_OS up`；最终镜像刷入后，串口监视器连接返回 `resource busy`，所以最终镜像的启动日志尚未复核。中间构建当时监视器误用了仓库忽略的旧 ELF，出现构建校验提示；esptool 对镜像数据的校验通过。
- 前一版本已由用户确认 MOTO GPS 标题正常，并验证过 BLE 广播、加密和通知订阅。本次刷写后尚待复核 BLE、标题、音乐字形、昼夜地图、锁屏手势及页面可见状态切换。
- 已只读检查线上 Worker `/healthz`：HTTP 200，`provider=amap`、实时导航配置为 ready、地图源使用 R2 auto 模式。该接口不验证 AMap Key 的实际搜索/路线调用。
- 用户报告 iPhone App“网关设置 → 测试连接”失败，并确认手机可访问另一个自有域名 Worker。截图打开 Moto GPS Worker 根路径 `/` 并收到 `NOT_FOUND`，这是预期响应，说明浏览器连到 Worker；根路径不是 App 测试的 `/healthz`。开发机访问 `/healthz` 曾返回 HTTP 200。排查时需区分 `workers.dev` 与自定义域名入口；AMap Key 的 IP 白名单与 Worker 健康检查无关。仓库和 App 不再公开维护者的个人 Worker 地址作为默认值。
- 真实导航路线和转向数据同步尚未验收。

## 验收状态

| 项目 | 状态 |
|---|---|
| Jarvis_OS 固件干净构建 | 已通过 |
| USB 实屏预览工具 | 已实现并构建；切页后的稳定性仍需真机复核 |
| 本轮 UI/按键/锁屏/字体/手势修改构建 | 已通过，ESP-IDF 5.5.4 |
| 本轮固件刷写与镜像校验 | 已通过，最终镜像写入 `0x20000`，esptool hash verified |
| 固件尺寸进入 8 MiB 应用槽 | 已确认，余量约 53% |
| 中间构建设备串口启动与启动自检 | 已通过，CO5300/CST9217 就绪，`startup core=OK`、`startup check=OK`；最终镜像串口日志待端口恢复后复核 |
| App 顶部标题无遮挡 | 上一版本确认过 MOTO GPS 标题；最新版本移除全局返回箭头，待用户实机复核 |
| 音乐页中文曲名/艺人 | 用户反馈“说好幸福呢 / 浪花兄弟”有乱码；新字库已构建并刷入，字形索引覆盖该曲名和艺人，待实机目视确认 |
| 指南针页滑动翻页 | 已修复子控件截断冒泡并降低 MOTO GPS 手势门槛，待实机手动确认 |
| 昼夜地图样式与页面可见状态 | 上一版本已构建并刷写；最新版本待目视复核 |
| MOTO GPS BLE 广播、加密和通知订阅 | 上一版本通过；最新版本待复测 |
| 触摸手势手动操作 | 最新版本已刷写；上滑解锁、PWR 长按返回与指南针页右滑待用户实机复测 |
| iOS Simulator 构建/启动 | 本轮之前已完成；不验证真实 BLE |
| Node/Cloudflare 本地测试 | 项目调整前已运行通过；路径搬迁后未重跑 |
| 上一版本固件 BLE 与真实 iPhone 配对 | 已通过；手机显示已连接，通知订阅成功 |
| 真实导航路线和转向同步 | 待真机验收 |
| 屏保上滑解锁、BOOT 解锁、PWR 长按返回 | 本轮固件已刷写，待用户实机验收 |
| Worker 线上 `/healthz` | 已通过，HTTP 200，`provider=amap`、R2 auto |
| App 内网关测试（iPhone） | 用户报告失败；手机能访问其他自有域名 Worker，当前需评估 Moto GPS 是否切换到自有域名；截图根路径 `NOT_FOUND` 为预期，开发机 `/healthz` 为 HTTP 200 |
| 真实 AMap 搜索/路线请求 | 尚未通过 App 做真实请求验收 |
| Worker Secret、R2 绑定和 Cron 控制台核对 | 线上健康检查能确认 Worker 能加载配置；Secret 值、桶策略和 Cron 仍需在 Cloudflare 控制台复核 |
| GeekTwin 连接当前固件 | 尚不兼容；旧 IMU GATT 与当前 Moto GPS GATT 不同 |

## 发布记录

最终二进制、哈希和刷写结果以仓库的 `release/` 文件及本页后续更新为准。不要把临时构建目录或本机 `sdkconfig` 当成正式发布材料。
