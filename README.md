# Jarvis_OS

Jarvis_OS 是运行在 Waveshare ESP32-S3-Touch-AMOLED-1.75C 上的圆屏系统。它保留 soRound_os 的应用与设备能力，并在原生界面中加入 Codex 助手页和表情时钟表盘。

## 设备与功能

- **硬件**：ESP32-S3R8、32 MB Flash、8 MB PSRAM、466 × 466 圆形 AMOLED。
- **soRound_os 应用**：启动器、表盘、设置、网络、音频与录音、传感器、工具和游戏。完整清单见[应用目录](Jarvis_OS/GeekTool-IDF/COMPANION_APPS.md)。
- **Codex**：Usage、Radar、Activity、VibeBuddy 四页。Usage 由 Mac bridge 提供，只读显示；设备通过局域网自动发现 bridge，离线时保留最近有效数据。VibeBuddy 是 Codex 内的一页，不是独立宠物应用。
- **表情时钟**：可在表盘选择器中启用，含 13 种表情、时间、日期、电量和眨眼动画；锁屏唤醒时会切换表情。
- **本地优先**：Codex bridge 在 Mac 上运行，设备通过 Wi-Fi 获取摘要；设置和设备状态保存在本机。

## 项目结构

```text
.
├── Jarvis_OS/GeekTool-IDF/  # ESP-IDF 固件、硬件测试和模块文档
├── Jarvis_OS/tools/         # Mac Codex bridge 和项目工具
├── docs/plans/              # 实施计划归档
├── docs/design/history/     # 已完成或被后续实现替代的设计记录
├── README.md                # 使用与构建入口
├── AGENTS.md                # AI 与贡献者的项目约定
├── design.md                # 当前系统架构与交互
├── PRODUCT.md               # 产品范围与体验约束
└── THIRD_PARTY.md           # 上游来源和许可说明
```

## 构建固件

需要 ESP-IDF 5.5.x、CMake、Ninja 和 Python。使用已安装的 ESP-IDF 时，先加载其 `export.sh`；共享开发工作区也可使用本机 `.toolchain/` 工具链（此目录不随仓库提交）：

```bash
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
source .toolchain/esp-idf/export.sh
idf.py -C Jarvis_OS/GeekTool-IDF set-target esp32s3
python Jarvis_OS/GeekTool-IDF/tools/apply_lvgl_touch_patch.py
idf.py -C Jarvis_OS/GeekTool-IDF build
```

`dependencies.lock` 固定已验证的 ESP-IDF 组件版本。`set-target` 会解析项目组件；触摸补丁脚本随后处理 CST9217 的瞬时无效响应。升级 `esp_lvgl_port` 后若脚本提示版本不匹配，应先复核补丁再调整版本检查。

只更新应用分区可使用 `app-flash`。完整 `flash` 会写入启动器、分区表和系统资源；改变分区布局前，先用设备 USB 功能导出需要保留的录音。

```bash
# 仅在分区布局已匹配时更新应用
idf.py -C Jarvis_OS/GeekTool-IDF -p /dev/cu.usbmodem1101 app-flash

# 完整安装并打开串口日志
idf.py -C Jarvis_OS/GeekTool-IDF -p /dev/cu.usbmodem1101 flash monitor
```

## 连接 Codex

在与设备处于同一局域网、保存 Codex 本地数据的 Mac 上启动 bridge：

```bash
./Jarvis_OS/tools/run_codex_bridge.sh
```

设备通过 UDP 发现服务，再从 HTTP `8790` 端口读取摘要。用量由 bridge 同步，不在设备上手动录入。安装、运行和故障排查见 [Codex bridge 说明](Jarvis_OS/tools/codex_bridge/README.md)。

## 验证

```bash
cmake -S Jarvis_OS/GeekTool-IDF/tests/host -B /tmp/jarvis-host
cmake --build /tmp/jarvis-host
ctest --test-dir /tmp/jarvis-host --output-on-failure

cmake -S Jarvis_OS/GeekTool-IDF/tests/ota -B /tmp/jarvis-ota
cmake --build /tmp/jarvis-ota
ctest --test-dir /tmp/jarvis-ota --output-on-failure
```

主机回归不能代替真实设备上的触摸、音频、IMU、无线、睡眠和 Flash 验证。

## 项目文档

- [产品范围与体验约束](PRODUCT.md)
- [系统架构与交互](design.md)
- [代码修改与验证约定](AGENTS.md)
- [应用清单](Jarvis_OS/GeekTool-IDF/COMPANION_APPS.md)
- [上游来源](Jarvis_OS/UPSTREAM.md)与[第三方许可](THIRD_PARTY.md)
- [文档索引](docs/README.md)
