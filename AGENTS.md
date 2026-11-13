# 项目协作约定

本文件适用于仓库根目录及其子目录。按任务读取背景：产品范围见 [PRODUCT.md](PRODUCT.md)，架构与交互见 [design.md](design.md)，构建和设备操作见 [README.md](README.md)，上游来源或许可相关改动见 [THIRD_PARTY.md](THIRD_PARTY.md) 与 [Jarvis_OS/UPSTREAM.md](Jarvis_OS/UPSTREAM.md)。

## 项目边界

- 主固件位于 `Jarvis_OS/GeekTool-IDF/`，应用入口和系统服务在 `main/`。
- Mac Codex bridge 位于 `Jarvis_OS/tools/`。
- `Jarvis_OS/UPSTREAM_README.md` 与 `UPSTREAM_README.en.md` 是保留的 soRound_os 历史说明；当前 Jarvis 使用说明以仓库根目录 README 为准。
- 当前系统架构以根目录 `design.md` 为准；旧方案和实施记录分别归档在 `docs/design/history/` 与 `docs/plans/`。
- 上游来源和许可边界以 `THIRD_PARTY.md`、`Jarvis_OS/UPSTREAM.md` 为准。保留已有归属和许可文件。

## 修改原则

- 先确认当前代码行为，再改对应模块；避免为局部改动批量格式化或重命名无关文件。
- 固件应用遵循 `app_t` 生命周期；涉及页面遮挡时维护现有 visibility、tick 和退出清理语义。
- UI 保持 466 × 466 圆屏安全区域及 soRound_os 的黑、白、灰、红视觉约定。
- Codex 数据来自本机 bridge；Usage 只读，离线缓存和错误状态不能被误报为实时数据。
- 不要直接修改被忽略的 `managed_components/`、`build/` 或 `.toolchain/` 来留下不可复现的改动。需要改变构建依赖时，把改动放入可追踪的组件、补丁或脚本，并记录来源与版本。
- 改分区、升级存储格式或写入设备前，确认录音和用户配置的兼容性；分区更改需要明确的迁移与备份说明。

## 本地文件与提交

以下内容是本机状态，不应提交：

- `.toolchain/`、固件 `build/`、`managed_components/` 和本机 `sdkconfig`。`dependencies.lock` 由 ESP-IDF 生成并纳入版本控制，用于固定组件版本。
- `secrets.h`、Codex 登录数据、会话文件、bridge 运行缓存、Wi-Fi 凭据、Python 虚拟环境和生成的 `.egg-info/`。
- `.impeccable/`、`.superpowers/` 等本地设计工具状态。

添加新工具前检查文件是否包含账号、密钥、局域网配置或机器专属绝对路径。只提交可重建的源文件、必要素材和文档。

## 构建与验证

使用 ESP-IDF 5.5.x 构建：

```bash
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
source .toolchain/esp-idf/export.sh
idf.py -C Jarvis_OS/GeekTool-IDF set-target esp32s3
python Jarvis_OS/GeekTool-IDF/tools/apply_lvgl_touch_patch.py
idf.py -C Jarvis_OS/GeekTool-IDF build
```

触摸补丁针对 `esp_lvgl_port` 的已验证版本；版本校验失败时先检查依赖源和代码差异，再更新补丁。

针对 Codex 页面运行原生回归：

```bash
cmake -S Jarvis_OS/GeekTool-IDF/tests/host -B /tmp/jarvis-host
cmake --build /tmp/jarvis-host --target codex_ui_tests
ctest --test-dir /tmp/jarvis-host -R '^codex_native_ui_interaction_and_fit$' --output-on-failure
```

设备验证要单独报告实际执行的步骤。主机测试通过不等于触摸、音频、无线、睡眠和录音迁移已在真机通过。
