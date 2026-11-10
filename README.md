# Agent Badge

Waveshare **ESP32-S3-Touch-AMOLED-1.75C**（32 MB flash / 8 MB PSRAM）上的个人用合并固件。

它把四个项目的“想要的部分”放进同一块表里：

| 来源 | 在这里是什么 |
|---|---|
| **The Badge**（curisama） | 底座：桌面/锁屏、应用框架、BLE HID 空鼠、WiFi、录音、电源管理、水球/行星/游戏/计算器等 |
| **Claude Desktop Buddy**（vthinkxie） | Pet 占位页（按当前决定先跳过；强依赖 Claude Desktop 的 BLE 审批链路） |
| **Codex Island**（WildBeast114514） | Codex 应用：Usage / Radar / Status，实时数据来自 Mac 上的 bridge |
| **Falling Sand**（nurdism） | Sand 应用：12 种材质、触摸绘制、倾斜重力、饼菜单、场景存读、PNG 导入 |

> 这是**个人使用**的合并固件。Codex Island 和 esp_sand_sim 的仓库没有根 LICENSE，
> 公开分发前需要先取得作者许可；The Badge 本体是 MIT。详见 [THIRD_PARTY.md](THIRD_PARTY.md)。

## 快速开始

```bash
cd agent-badge
./tools/dev.sh build
./tools/dev.sh flash          # 默认 /dev/cu.usbmodem101，按需改
```

首次启动后，WiFi 凭据会从旧 Codey 固件留在 NVS 里的 `codey/nets` 自动迁移；
迁移不到时，在 Badge 的 **Settings → WiFi** 里重新配网。

### Codex（需要 Mac 上的 bridge）

```bash
cd agent-badge
./tools/run_codex_bridge.sh              # 前台，默认 8790
# 或安装 LaunchAgent，登录后自启：
./tools/install_codex_bridge_launchd.sh
```

Badge 端使用 `http://auto:8790/...`：加入 WiFi 后通过 UDP 广播自动发现 Mac 的
局域网 IP；发现失败回退 `192.168.50.13`。

### Sand

- 第 3 页 → **Sand**
- 顶部「材质名 + ☰」按钮打开饼菜单：S1/L1、S2/L2 场景，IMG 导入，X 关闭
- 调色板 12 种材质；下方是绘制区；倾斜设备让沙子流动
- 把要导入的图放到 `agent-badge/tools/sand_import.png`，点 IMG 即可

## 目录

```
.
├── agent-badge/              # 合并固件工程（主项目）
│   ├── main/                 # 应用、HAL、Codex/Sand/Pet
│   ├── tools/                # dev.sh、Codex bridge、图标生成、PNG 导入
│   ├── docs/                 # The Badge 的截图/说明
│   ├── INTEGRATION.md        # 合并架构、取舍、已知限制
│   └── README.md             # 详细使用/构建说明
├── .toolchain/               # 工作区本地 ESP-IDF + 工具链（不提交）
└── THIRD_PARTY.md            # 许可证与来源说明
```

## 已知限制

- **Pet 未实现**：需要 Claude Desktop 的 Developer Mode + Hardware Buddy 配对。
- **Sand 的 USB 导入未移植**：Badge 的 TinyUSB 把录音分区做成只读 FAT，且没有
  teardown；改用 Mac bridge 的 PNG→RLE 导入作为等价方案。
- **Radar** 数据从 `codexradar.com` 页面内嵌 JSON 解析，站点改版可能导致 stale。
- **场景槽**放在 `rec` 分区最后 64 KB；录音接近 24 MB 满时理论上会覆盖。
- Codex bridge 需要 Mac 与 Badge 在同一局域网；Mac IP 变化时依赖 UDP 自动发现。

## 更多

- 详细的构建、桥接、应用操作：[agent-badge/README.md](agent-badge/README.md)
- 合并架构与取舍：[agent-badge/INTEGRATION.md](agent-badge/INTEGRATION.md)
- 第三方来源/许可：[THIRD_PARTY.md](THIRD_PARTY.md)
