# Agent Badge 固件

> 个人使用的合并固件：**The Badge** 底座 + **Codex** 实时面板 + **Falling Sand**。
> 目标板：Waveshare **ESP32-S3-Touch-AMOLED-1.75C**（32 MB flash / 8 MB PSRAM）。
> 详细合并取舍见 [INTEGRATION.md](INTEGRATION.md)。

## 硬件

| 项 | 规格 |
|---|---|
| SoC | ESP32-S3R8，8 MB OPI PSRAM |
| Flash | 32 MB（`esptool flash_id` 确认） |
| 屏幕 | 466×466 CO5300 QSPI AMOLED（列偏移 6） |
| 触摸 | CST9217 @ I2C 0x5A |
| PMU | AXP2101 @ 0x34（PWR 键走 PEK IRQ） |
| IMU | QMI8658 @ 0x6B |
| 麦克风 | ES7210 @ 0x40 |
| 扬声器 | ES8311 @ 0x18 + PA GPIO46 |
| 按键 | BOOT = GPIO0；PWR = AXP2101 |

## 功能

- **The Badge 底座**：桌面/锁屏、应用框架、BLE HID 空鼠、WiFi、录音、电源管理、
  水球、行星、游戏、计算器、计时器、设置等。
- **Codex 应用**（第 3 页）：
  - **Usage**：5h / 7d 百分比、今日 tokens、金额、reset 倒计时、Codex 状态
  - **Radar**：CodexRadar IQ 前 8 名（模型/effort/IQ）
  - **Status**：bridge、电量、plan、雷达条数、Codex 状态、同步时间
  - 在 Codex 界面左右滑切换三页
- **Sand 应用**（第 3 页）：
  - 12 种材质：沙/水/火/木/石/橡皮/土/酸/岩浆/烟/蒸汽/墙
  - 触摸绘制、倾斜重力、顶部两行调色板
  - 顶部「材质名 + ☰」打开饼菜单：S1/L1、S2/L2 场景存读、IMG 导入、X 关闭
  - 场景 RLE 存在 `rec` 分区尾部（32 KB × 2 槽）
  - PNG 导入：Mac 端把 `tools/sand_import.png` 转成 116×116 材质 RLE，Badge 通过 WiFi 下载
- **Pet**：占位页，暂未实现（需要 Claude Desktop 的 BLE 审批链路）。
- **WiFi**：首次启动自动从旧 Codey 固件的 NVS（`codey/nets`）迁移凭据；也可在
  Settings → WiFi 重新配网。
- **Mac IP 自动发现**：Badge 加入 WiFi 后向 UDP 8790 广播 `AGENTBADGE/1`，
  bridge 用它到 Badge 的网卡地址回复；失败回退 `192.168.50.13`。

## 操作

| 操作 | 行为 |
|---|---|
| 屏幕左右滑 | 首页翻页；Codex 内切换 Usage/Radar/Status |
| BOOT（GPIO0） | 屏幕开/关 |
| PWR 短按 | 回首页 |
| 底部把手上拉 | 回首页 |
| Sand 顶部按钮 / 调色板 MENU | 打开饼菜单 |
| Sand 场景 S1/L1 | 保存/读取槽 1；S2/L2 槽 2 |
| Sand IMG | 从 bridge 导入 `sand_import.png` |
| Sand CLR | 清空沙盘 |

## 构建 / 烧录

工具链已放在工作区 `.toolchain/`（ESP-IDF v5.5.4、CMake、Ninja、Python env），
不污染系统路径。

```bash
cd agent-badge
./tools/dev.sh build
./tools/dev.sh flash          # 默认 /dev/cu.usbmodem101
./tools/dev.sh monitor
```

其他目标：`menuconfig`、`clean`。串口工具在 `tools/serial_read.py`、
`tools/serial_reset_read.py`、`tools/serial_stream.py`。

分区表：`factory` 6 MB，`storage` 1 MB，`rec` 24 MB，`coredump` 64 KB，NVS 24 KB。
当前固件约 3.7 MB，factory 还剩约 38%。

## Codex bridge（Mac）

bridge 读取 `~/.codex` 的 auth/sessions，提供：

- `GET /codex.json`：usage + pet + radar（60 s 缓存）
- `GET /sand_scene.rle`：`tools/sand_import.png` → 116×116 材质 RLE
- UDP 8790：局域网自动发现

启动：

```bash
cd agent-badge
./tools/run_codex_bridge.sh
```

首次会创建 `.toolchain/venv-bridge` 并安装 vendored bridge；也可以手动：

```bash
./tools/setup_codex_bridge.sh
```

开机自启：

```bash
./tools/install_codex_bridge_launchd.sh
```

注意：
- bridge 需要 Mac 与 Badge 同一局域网；
- macOS 防火墙要允许 Python 接收入站连接；
- 如果 8790 被占用，先停掉手动 bridge 再装 LaunchAgent；
- Radar 从 `codexradar.com` 页面内嵌 JSON 解析，站点改版可能失效（Status 页会显示 stale）。
- 旧 Codey bridge 的 `HOME`/`CODEX_AUTH_PATH` 等环境变量处理仍保留在脚本里。

## Sand 使用

- 打开 Sand 后，顶部两行是材质；点选后下方绘制。
- 顶部按钮显示当前材质名，点它打开饼菜单。
- **保存场景**：画好后 S1/S2；**读取**：L1/L2。
- **换导入图**：替换 `tools/sand_import.png`，点 IMG；bridge 会实时转换。
- 场景存在 `rec` 分区最后 64 KB；录音接近 24 MB 满时理论上可能覆盖。
- 触摸/重力/材质物理来自 `main/sand/sand_sim.c`（esp_sand_sim 改编）。

## 代码结构

```
main/
├── apps/
│   ├── app_codex.c       # Usage / Radar / Status
│   ├── app_sand.c        # Sand UI、菜单、场景、导入
│   ├── app_pet.c         # Pet 占位
│   └── ...               # The Badge 原有应用
├── sand/
│   ├── sand_sim.c/h      # 物理/材质
│   └── sand_config.h
├── assets/               # 图标（含 Pet/Codex/Sand）
├── ble/                  # BLE HID
├── launcher.c            # 首页/分页
├── port_esp.c            # WiFi/HTTP/UDP/触摸/IMU/PMU HAL
└── main.c                # 启动、迁移、电源
tools/
├── dev.sh                # build/flash/monitor/menuconfig/clean
├── codex_bridge_http.py  # Mac bridge（usage/radar/PNG/UDP）
├── run_codex_bridge.sh
├── install_codex_bridge_launchd.sh
├── gen_agent_icons.py    # 生成 120×120 图标
├── sand_import.png       # 示例导入图
└── serial_*.py           # 串口日志
```

## 已知限制

- **Pet 未做**：需要 Claude Desktop（Developer Mode → Hardware Buddy）配对；
  只用 Claude Code 的话拿不到审批/transcript 事件。
- **USB 导入未移植**：Badge 的 TinyUSB 把录音分区做成只读 FAT，且 TinyUSB 0.15
  没有 teardown；Sand 原来的可写 FAT + 重启路径会和录音模式冲突。PNG 导入用
  bridge/HTTP 代替。
- **Sand 饼菜单/场景是 LVGL 重实现**，不是原版 `sand_menu.c` 的直接移植；原版
  的 PNG 场景/USB/FAT 也未移植。
- **Radar** 依赖外部页面结构；`Status` 页会显示 stale。
- **场景槽**在 `rec` 分区尾部，录音接近满时可能冲突。
- `main/secrets.h` 含 WiFi 密码，已被 `.gitignore` 排除；提交前确认不要 `git add -f`。

## 许可

- The Badge：**MIT**，见 [LICENSE](LICENSE) 与 [NOTICE](NOTICE)。
- Codex Island / esp_sand_sim：仓库未找到根 LICENSE，**仅个人使用**，公开分发前
  需取得作者许可；详见 [../THIRD_PARTY.md](../THIRD_PARTY.md)。
- Claude Desktop Buddy：MIT（未包含，Pet 暂跳过）。
- 构建期依赖（ESP-IDF/LVGL/TinyUSB/驱动）由 Component Manager 拉取，不随仓库分发。

## 致谢

- [curisama/The-Badge](https://github.com/curisama/The-Badge) — 底座
- [WildBeast114514/codex-island-waveshare](https://github.com/WildBeast114514/codex-island-waveshare) — Codex 页面思路与 bridge
- [nurdism/esp_sand_sim](https://github.com/nurdism/esp_sand_sim) — Sand 物理
- [vthinkxie/claude-desktop-buddy-esp32](https://github.com/vthinkxie/claude-desktop-buddy-esp32) — Pet 参考（未包含）
