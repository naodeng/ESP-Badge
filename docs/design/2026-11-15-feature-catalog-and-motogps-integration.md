# Jarvis 功能盘点与 MOTO GPS 手机 App 集成评估

## 2026-11-16 范围更新

本文件后续章节记录 2026-11-15 的初始盘点与方案比较。用户于 2026-11-16 确认新的主线：主项目与固件统一命名为 **Jarvis_OS**，面向 MOTO GPS iOS App 完整适配设备侧导航功能，并将 Node/Cloudflare 网关代码纳入主仓库。正式固件只保留一个版本。

- 产品必需：Jarvis_OS 固件、MOTO GPS iOS App、共享导航/协议代码、Cloudflare/Node 网关。
- 开发调试：保留 Jarvis GeekTwin 与 MOTO GPS WASM 模拟器，仅本地使用，不作为产品网站。
- 裁剪：Android App、MOTO 独立 ESP32 固件、OTA、产品网站、无关 Jarvis 应用，以及原独立 Travel BLE 服务。
- 保留的固件基础：启动/待机、显示、触摸/按键、基础设置、BLE 与导航所需传感器。
- 旧功能源码归档后不参与正式构建；分区、NVS 和录音存储保持兼容。
- `docs/design/2026-11-16-main-repo-monorepo-design.md` 与 `docs/plans/2026-11-16-monorepo-reorganization.md` 是当前实施依据；下方原有方案表仅作决策历史。

盘点日期：2026-11-15
状态：第一轮功能取舍已确定。2026-11-15 用户选择从默认固件移除 Mouse、Twin、Level、Maze、Fluid、Answers、Zodiac、Merit、Pixels、Stocks；本文件记录实施范围与当前 18 项启动器目录。

## 结论摘要

- 初始盘点时启动器有 28 个 App；移除所选 10 项后，默认固件保留 18 个 App，表盘目录有 63 款。另有启动、锁屏、快捷面板、网络、音频、存储、OTA 和 Codex Mac bridge 等跨 App 能力。
- MOTO GPS 不只是手机 App：仓库包含 iOS App、Android App、同一 Waveshare 1.75C 板型的配套 ESP32 固件，以及共享导航核心与 BLE 协议。个人研究、实验和自用符合其 PolyForm Noncommercial 1.0.0 明列的个人用途；源码或固件/App 再分发仍要保留许可、NOTICE 和各第三方许可。
- 手机 App 不能原样连接当前 Jarvis Travel 页。两边的 BLE 服务 UUID、消息格式和能力范围不同。最完整的复用方式是把上游导航固件模块接入 Jarvis，作为可选的 Navigation 构建包，并继续使用上游手机 App。
- 所选 10 项从默认固件源文件清单、应用注册表和演示目录撤下；实现与既有测试源码先保留，便于日后作为可选包重新评估。共享依赖保留；不改分区、不清理 NVS/录音数据、不刷设备。

上游核对对象：[moto-gps-waveshare](https://github.com/mx3353672833-debug/moto-gps-waveshare)，检查时 `main` 指向 `2ddc164dbcc7f1789317ca81899fbca9b73c32d4`。

## 1. Jarvis 当前功能目录

### 1.1 系统级能力

| 子系统 | 当前能力 | 盘点处理建议 |
|---|---|---|
| 开机与输入 | 开机自检、启动画面、启动器、触摸、两个实体键 | 平台基础，先列为 Core |
| 锁屏与显示 | 锁屏、AOD、熄屏、省电刷新、表盘预览 | 平台基础，先列为 Core |
| 快捷面板 | Wi-Fi、AOD、静音开关和亮度 | 依赖设置与电源状态，暂不拆开 |
| 表盘 | 当前目录计数 63：TYPE/ORBIT/SHIFT 共 15、HAND 6、STUDIO 41、表情时钟 1 | 可研究“基础表盘 + 可选主题组”；旧 ID 与保存格式需要兼容 |
| 配置与持久化 | NVS 设置、延迟保存、应用状态 | Core 依赖；隐藏应用时不清除设置 |
| 网络服务 | Wi-Fi、SNTP、HTTP/TLS、天气、Codex 客户端 | HTTP/TLS 与 Wi-Fi 仍是共享服务；行情/答案/星座客户端已退出默认构建 |
| 蓝牙 | 共用 NimBLE host；默认固件只注册 Travel GATT 服务 | Mouse、Twin 与媒体服务随对应应用撤下；Travel 仍使用 NimBLE |
| 音频与录音 | 麦克风、音频总线、音景/提示音、录音回放和存储卷 | 录音分区与真实数据需保留兼容，不能随 App 入口一起删 |
| OTA | HTTPS 下载、校验和升级页 | 作为系统维护能力单独评估，保持恢复路径 |
| Mac companion | Codex bridge | Mouse 媒体控制与 Now Playing 媒体桥已从默认产品范围移除 |

### 1.2 初始 28 项目录与本轮处理结果

以下序号是移除前目录序号；“当前保留”仅表示仍在默认固件中，不代表不可在后续继续调整。源码与测试源码对已移除项目暂时保留，但不进入当前设备构建或启动器。

| 原序号 | App | 功能摘要 | 可选分组候选 | 当前状态 |
|---:|---|---|---|---|
| 1 | WiFi | Wi-Fi 状态与连接配置 | Network | 当前保留 |
| 2 | I2C | 扫描板载 I²C 设备 | Device Tools | 当前保留 |
| 3 | System | 系统与内存遥测 | Device Tools | 当前保留 |
| 4 | Weather | 天气与地点选择 | Network | 当前保留 |
| 5 | Calendar | 月历 | Daily Tools | 当前保留 |
| 6 | Countdown | 倒计时 | Daily Tools | 当前保留 |
| 7 | Stopwatch | 秒表与圈次 | Daily Tools | 当前保留 |
| 8 | Settings | 亮度、网络、锁屏等系统设置 | Core | 当前保留 |
| 9 | OTA | 固件更新 | Core / Maintenance | 当前保留 |
| 10 | Audio | 麦克风实时频谱 | Audio | 当前保留 |
| 11 | Level | 倾角水平仪 | Sensors | **已移除** |
| 12 | Maze | 倾斜控制的迷宫游戏 | Hobby | **已移除** |
| 13 | Fluid | IMU 重力驱动的粒子流体 | Hobby / Sensors | **已移除** |
| 14 | Dice | 摇骰子与抛硬币 | Hobby | 当前保留 |
| 15 | Mouse | BLE 鼠标、演示翻页、媒体按键和音量控制 | Remote / Media | **已移除** |
| 16 | Twin | BLE 上传 IMU 与电量，网页同步显示 3D 设备模型 | Remote / Demo | **已移除** |
| 17 | Answers | 联网答案之书，带本地备用内容 | Network / Hobby | **已移除** |
| 18 | Zodiac | 星座内容查询 | Network / Hobby | **已移除** |
| 19 | Merit | 木鱼、计数、音效与成长状态 | Hobby / Audio | **已移除** |
| 20 | Pixels | IMU 驱动的像素动画 | Hobby / Sensors | **已移除** |
| 21 | Garden | 触摸互动生态箱；与专注完成状态联动 | Hobby | 当前保留 |
| 22 | Focus | 25 分钟专注计时和音景 | Daily Tools / Audio | 当前保留 |
| 23 | Capsules | 录音、列表、回放和删除 | Audio / Storage | 当前保留 |
| 24 | Watchfaces | 表盘选择器 | Core / Watchface Themes | 当前保留 |
| 25 | Stocks | 股票列表、行情和搜索 | Network | **已移除** |
| 26 | Mimi | 离线互动小宠物 | Hobby | 当前保留 |
| 27 | Codex | Usage、Radar、Activity、VibeBuddy；从 Mac bridge 只读同步 | Network / Mac Integration | 当前保留 |
| 28 | Travel | 手机 GPS 速度、航向、定位精度显示；目前不是路线导航 | Travel | 当前保留 |

Mouse 被移除后，默认固件不再提供 BLE 鼠标、演示翻页、媒体按键、音量/静音遥控；Mac Now Playing 媒体桥也不再属于当前产品。Audio、Focus 音景、Capsules 录音与本地回放仍在。移除 Stocks 不会清理设备现有 NVS 命名空间，代码也不触碰录音分区或文件。

## 2. 手机上游 App 与当前固件的适配差异

| 方面 | MOTO GPS 上游 | 当前 ESP-Badge / Jarvis |
|---|---|---|
| 手机端 | iOS 17+ 原生 SwiftUI App；另有 Kotlin + Jetpack Compose Android App | 当前仓库有 Web Travel 发送原型，没有对应的完整 iOS/Android 导航 App |
| 目标硬件 | Waveshare ESP32-S3-Touch-AMOLED-1.75C，与本设备板型一致 | 同一板型、ESP-IDF 与 LVGL 系统，但有自己的 app_t 生命周期和 NimBLE 服务管理 |
| 手机职责 | 定位、目的地搜索、路线预览、路线匹配、偏航重算、地图缓存/下载、媒体状态 | Travel 页目前只消费速度/航向/精度快照 |
| BLE 服务 | `7e57a000-b50c-4b6a-9c57-40a54e8e1000`，RX/TX、加密握手、分片、CRC、ACK、导航/地图/媒体/命令消息 | 当前 Jarvis 默认固件只注册 Travel 服务 `7f3a9001-2c91-4f8a-9d2e-51c034cd1300` 和 20 字节速度帧 |
| 服务端配置 | 真实搜索/路线需要自建网关和自己的高德 Key；iOS 安装需个人签名与权限设置 | 当前没有对应网关或手机导航状态机 |

所以“直接拿来用”有两种含义：**单独使用上游 App + 上游固件**，源项目已提供完整配对；**让上游 App 连当前 Jarvis 固件**，则需要先补齐上游 Navigation GATT 服务和消息处理，不能只换手机 App。

## 3. 复用方式与难度

| 方案 | 复用内容 | 与 Jarvis 的关系 | 难度 | 适用判断 |
|---|---|---|---|---|
| A. 上游原样配对 | 上游手机 App + 上游 Waveshare 固件 | 独立导航固件；刷入会替换当前 Jarvis 固件 | 低至中 | 适合先作源码/构建验证；不应直接刷当前设备 |
| B. Jarvis Navigation 可选包 | 上游 iOS/Android App、共享导航核心、BLE 协议、导航绘制器；适配到 Jarvis app_t 和 NimBLE host | 保留 Jarvis，在同一固件增加 Navigation profile | 高但可做 | 最符合“导航常用、小功能可挑选”的长期方向 |
| C. 只把上游手机定位接到当前 Travel | CoreLocation/Android Location 与现有 20 字节帧 | 不实现路线、地图或偏航重算 | 低至中 | 适合验证手机与设备的数据链，不能算完整导航 |
| D. 将上游固件整个移入 Jarvis | 大量复用上游 ESP32 实现 | 仍需迁移启动器、BLE host、共享屏幕与应用生命周期 | 高 | 不等于复制目录就完成集成，风险高于 B |

### 推荐的探索顺序

1. **只读基线**：固定上游 commit，复核 iOS/Android 和 ESP32 的构建依赖、第三方许可及测试入口。
2. **隔离构建**：生成手机工程并做不签名的模拟器/单测构建；编译上游 ESP32 固件，不刷板。配置自己的网关地址，不使用示例占位地址。
3. **协议验证**：先在主机模拟端验证上游 App 的连接、握手、导航快照、命令 ACK，再对照 Jarvis 的 NimBLE 服务注册边界。
4. **Navigation pack Spike**：在独立分支或可回退工作区里只接入连接状态与 `NavigationSnapshot`，继续用 Jarvis 页面生命周期；不先搬地图和媒体大图层。
5. **模块再筛选**：根据最终保留的 App 集合，拆出 build profiles 和依赖组；真机联调前再确认测试设备、备份和刷写范围。

这轮完成了源码/文档级结构核对，尚未下载完整上游工程、构建手机 App、刷写或在实机配对。

当前工作环境的 `xcode-select` 指向 Command Line Tools，`xcodebuild` 因未选中完整 Xcode 而退出；尚未发现 XcodeGen。要实际构建 iOS App，需要在有 Xcode/XcodeGen 的 Mac 上进行，或先准备对应工具链。

## 4. 可选打包方案

Jarvis 是静态 ESP-IDF 固件：当前 `main/CMakeLists.txt` 显式列源文件，`launcher.c` 的 `APPS[]` 需要和图标表一致。可选包应在**构建期**决定，不是设备运行时下载插件。

建议保留源码，将固件编译成可选择的 profiles：

| Build profile 候选 | 包含方向 |
|---|---|
| `full` | 当前 18 项默认功能，作为完整体验构建 |
| `core` | 启动、表盘基础、触摸/按键、锁屏、电源、设置、系统维护 |
| `ride` | `core` + Navigation + 必需的 BLE/IMU；其余由用户勾选 |
| `network` | Wi-Fi、天气、Codex 等网络应用 |
| `audio` | 麦克风频谱、Focus 音景、录音与本地回放 |
| `hobby` | Dice、生态箱、小宠物等当前保留的娱乐应用 |

落实时用一个 Kconfig/构建配置作为功能清单来源，再由它控制 CMake 源、`APPS[]`、图标、字体和资源，避免入口指向未编译的 App。每个 profile 都应有启动器目录回归。表盘要保留持久 ID；隐藏应用不能自动清除 NVS。录音分区与录音文件也应保持不变，除非另有专门迁移方案。

需要保留的共享依赖包括：Garden 与 Focus 共用成长/完成状态；Capsules 依赖麦克风、音频总线、录音任务和独立存储；天气缓存还会给部分表盘使用；Codex 页依赖本地 Mac bridge；Travel 使用共用 NimBLE host。`imu.c` 仍编入默认固件，作为后续导航辅助方向的设备驱动基础；当前 Travel 页面尚未使用 IMU。被移除应用的实现与测试源码暂时留在仓库，不在默认 ESP-IDF 源文件清单、`APPS[]`、演示目录或 BLE 服务注册中。

## 5. 许可与验证边界

上游 [LICENSE.md](https://github.com/mx3353672833-debug/moto-gps-waveshare/blob/main/LICENSE.md) 是 PolyForm Noncommercial 1.0.0，个人学习、实验、爱好用途被列为许可用途。若将代码、修改版源码或构建产物再分发，需按该许可保留许可文本/URL 和 [NOTICE](https://github.com/mx3353672833-debug/moto-gps-waveshare/blob/main/NOTICE)，同时保留第三方素材各自的许可；这不意味着 Jarvis 全仓库自动采用该许可证。

上游 iOS README 说明其需要 Xcode/XcodeGen、iOS 签名和自有 HTTPS 网关；Android README 表明协议测试覆盖模拟圆屏，但真实 Android 手机与真实圆屏配对/骑行尚未验证。构建、单元测试和模拟器结果都不能替代锁屏定位、实机 BLE、网络和骑行验证。
