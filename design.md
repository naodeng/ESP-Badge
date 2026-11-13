# Jarvis_OS 系统设计

本文描述当前固件的模块边界、页面结构和数据流。历史方案不作为当前行为的依据；相关记录放在 [`docs/design/history/`](docs/design/history/)。

## 系统边界

| 层级 | 职责 | 主要实现 |
|---|---|---|
| 硬件与显示 | AMOLED、触摸、I²C、RTC、IMU、电源和音频 | `display.c`、`power.c`、`rtc.c`、`imu.c`、`audio_bus.c` |
| 启动与输入 | 启动自检、启动器、锁屏、省电、两个实体按键 | `main.c`、`startup.c`、`launcher.c`、`lock.c`、`buttons.c` |
| 表盘与覆盖层 | 表盘选择与渲染、快速控制面板、启动画面 | `watchface*.c`、`quickpanel.c`、`identity_ui.c` |
| 应用 | soRound_os 工具、游戏、录音和新增 Codex 页面 | `app_*.c`、应用专用状态与 UI 模块 |
| 状态与服务 | NVS 设置、网络、媒体、bridge、OTA 和文件资源 | `settings.c`、`wifi_service.c`、`codex_client.c`、`ota_update.c` 等 |

运行环境是 ESP-IDF 与 LVGL。硬件与依赖版本以 `Jarvis_OS/GeekTool-IDF/` 的项目配置为准；系统源码不依赖 Android Wear。根目录 [`PRODUCT.md`](PRODUCT.md) 描述产品目标，详细应用实现见 [固件文档](Jarvis_OS/GeekTool-IDF/)。

## 启动与页面生命周期

1. `app_main()` 初始化 NVS、时区、动态调频、音频总线、I²C、IMU、RTC、显示、设置和触摸。
2. 创建启动身份画面和启动器，执行核心自检；设备时间、联网状态与启动结果会反馈到启动界面。
3. 自检通过后显示主页；锁屏表盘由 `watchface.c` 管理，应用由 `launcher.c` 按需进入。
4. 快速控制面板作为覆盖层显示，不退出当前应用。应用的 tick 与 visibility 回调跟随可见状态调度。

应用通过 `app_t` 提供 `enter`、`tick`、`exit`、`back` 等生命周期回调。切页或遮挡页面时应暂停不必要的视觉更新；退出应用时释放页面对象和专属资源。

## 导航与电源行为

- 主页通过横向滑动浏览应用，点图标进入应用；应用内右滑通常执行返回，应用可以先消费返回以退出内部子页。
- BOOT 短按锁定或解锁，长按执行关机。解锁使用 BOOT 键，避免表盘上的上滑手势误解锁。
- PWR 键由当前可见应用消费。在 Codex 页面长按向前翻页；其他应用按自身定义响应。
- 锁屏空闲约 12 秒后进入低亮度 AOD；启用自动熄屏并处于放电状态时，空闲超过约 30 秒后关闭显示。触摸或按键会唤醒屏幕。
- 这些时长和策略来自 `lock.c`，更改时同步更新设备行为、文档和电源回归。

## Codex 数据与页面

```text
Mac 本地 Codex 数据
        │
        ▼
Codex bridge ── UDP 服务发现 / HTTP :8790
        │                    /codex.json
        ▼
设备 Wi-Fi → codex_client → codex_data → app_codex
                                  │            ├─ Usage
                                  │            ├─ Radar
                                  │            ├─ Activity
                                  │            └─ VibeBuddy
                                  └─ 最近一次有效快照
```

- bridge 由 `Jarvis_OS/tools/run_codex_bridge.sh` 启动；固件在 Wi-Fi 可用时用 UDP 发现 bridge，再请求 HTTP `8790` 的 JSON 摘要。
- `codex_data.c` 负责校验和事务式解析数据；无效响应不能覆盖最近一次有效快照。离线或数据过期时，页面保留缓存并显示对应状态。
- `app_codex.c` 负责四个页面的 LVGL 展示与页码切换。左滑下一页、右滑上一页；第一页右滑离开 Codex。底部页码也可点按前进，PWR 长按继续支持下一页。
- Usage 的百分比和进度来自 bridge，是只读信息；设备上没有手动滑动添加用量的控件。
- Radar 最多展示 16 行。不可用数据用 `--` 或明确状态表示，不以占位值伪装在线数据。

数据结构和本机运行说明见 [`Jarvis_OS/tools/codex_bridge/README.md`](Jarvis_OS/tools/codex_bridge/README.md)。该服务与设备应处于可互通的局域网；本设计不要求另建 BLE 数据连接。

## 表情时钟

表情时钟通过 `watchface.c` 与表盘目录注册，绘制实现位于 `watchface_grok.c`。它包含 13 种矢量表情，按表盘快照显示系统时间、电量和眨眼状态。进入表盘、锁屏显示或完整睡眠唤醒时选择新表情；AOD 时停止眨眼并降低刷新频率，熄屏时停止表盘 timer。

它是 soRound_os 的原生表盘与锁屏显示，不是独立宠物应用。修改表盘选择、睡眠或锁屏流程时，保留原有表盘 ID、NVS 选择值和省电生命周期。

## 存储与升级

当前分区定义见 [`Jarvis_OS/GeekTool-IDF/partitions.csv`](Jarvis_OS/GeekTool-IDF/partitions.csv)：

| 分区 | 用途 |
|---|---|
| `nvs` | 设备设置和小型状态 |
| `ota_0` / `ota_1` | 双 OTA 固件槽 |
| `storage` | 系统资源 FAT 区 |
| `capsule_meta` / `capsules` | 录音元数据和录音文件 |

只更新 app 镜像时使用 `app-flash`；完整烧录会更新启动器和分区表。更改分区、文件系统格式或录音读写路径前，先明确现有设备数据的保留方式，并提供可执行的迁移步骤。

## 界面约束

- 目标画布为 466 × 466 圆屏，靠近四角的矩形区域会被圆形玻璃边界裁掉；重要文字和触控目标必须置于圆形安全范围。
- 新页面延续 soRound_os 的黑色背景、白灰文字和红色状态强调；Codex 与表情时钟复用现有字体、触摸和导航约定。
- 页面按一眼可读来组织。辅助入口要有足够大的触摸面积，状态文字避免与仪表图形重叠。
- 异步数据需区分在线、缓存、过期、断连和错误；不要仅靠颜色传达唯一含义。

## 主要代码与验证

- 固件和 app 注册：[`Jarvis_OS/GeekTool-IDF/main/`](Jarvis_OS/GeekTool-IDF/main/)
- Codex UI：`main/app_codex.c`；bridge 客户端：`main/codex_client.c`；快照解析：`main/codex_data.c`
- 表情时钟：`main/watchface.c`、`main/watchface_grok.c`
- 应用清单：[`COMPANION_APPS.md`](Jarvis_OS/GeekTool-IDF/COMPANION_APPS.md)
- 原生 Codex UI 测试：`tests/codex/codex_ui_tests.c`
- 完整测试命令：[`README.md`](README.md) 与 [`AGENTS.md`](AGENTS.md)
