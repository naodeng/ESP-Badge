# 第三方来源与许可

这个仓库是个人使用的合并固件，**不是**一个单一许可证的项目。

## 直接包含/改编的代码

| 组件 | 位置 | 许可 | 说明 |
|---|---|---|---|
| **The Badge** — curisama | `agent-badge/` 底座 | **MIT** | 许可证全文见 `agent-badge/LICENSE`；`agent-badge/NOTICE` 列出了构建期依赖 |
| **Codex Island** — WildBeast114514 | `agent-badge/tools/codex_bridge/`、Codex 应用思路 | **未找到根 LICENSE** | 仅个人使用；公开分发前需联系作者取得许可 |
| **esp_sand_sim** — nurdism | `agent-badge/main/sand/sand_sim.c/h` | **未找到根 LICENSE** | 仅个人使用；公开分发前需联系作者取得许可 |
| **Claude Desktop Buddy** — vthinkxie | 未包含（Pet 先跳过） | MIT | 后续若移植，保留其版权与许可声明 |

## 构建期依赖

ESP-IDF、LVGL、TinyUSB、esp_lvgl_adapter、CO5300/CST9217 驱动、QMI8658 驱动等
由 ESP-IDF Component Manager 在构建时拉取，不随仓库分发；具体条目与许可见
`agent-badge/NOTICE`。

## 字体与图像

- Montserrat 字体随 LVGL 分发（SIL OFL 1.1）。
- 行星贴图来自 NASA/JPL（公共领域/NASA 使用条款），详见 `agent-badge/NOTICE`。
- Pet/Codex/Sand 的 120×120 图标由 `agent-badge/tools/gen_agent_icons.py` 生成。

## 重要

如果你要把这个仓库公开，请先：

1. 取得 Codex Island 与 esp_sand_sim 作者的许可，或移除对应代码；
2. 保留 The Badge 的 MIT 许可证与版权声明；
3. 不要把 `agent-badge/main/secrets.h`（WiFi 密码）提交进仓库。
