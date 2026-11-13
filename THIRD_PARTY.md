# 第三方来源与许可

本仓库基于 soRound_os，并集成 Codex bridge 与表情时钟。源码、图片、字体和构建依赖保留各自的来源与许可文件。

| 组件 | 仓库内容 | 许可 / 说明 |
|---|---|---|
| soRound_os | `Jarvis_OS/` 的固件、工具、配套应用和文档 | 来源：[soBigRice/soRound_os](https://github.com/soBigRice/soRound_os)。上游快照未包含 LICENSE；项目所有者确认已获得作者对源码复用和修改的许可。该许可确认不代表上游发布了通用开源许可证。 |
| GrokBot 表情数据 | `Jarvis_OS/GeekTool-IDF/artwork/mimi/vendor/GrokBot/` 及其派生表达式 | BSD-3-Clause；保留目录内 LICENSE 与来源说明。 |
| bloub | `Jarvis_OS/GeekTool-IDF/artwork/mimi/vendor/bloub/` | MIT；保留目录内 LICENSE。 |
| Codex bridge | `Jarvis_OS/tools/codex_bridge/` 和 `Jarvis_OS/tools/codex_bridge_http.py` | 项目内的本地辅助程序；运行时从用户本机读取 Codex 登录与会话元数据，只向设备发送摘要。 |
| ESP-IDF、LVGL、Waveshare BSP | 构建期组件 | 由 ESP-IDF Component Manager 获取；各组件的许可随依赖保留。 |
| Montserrat | LVGL 内置字体 | SIL Open Font License 1.1；本仓库未另行复制字体文件。 |

新写的 VibeBuddy 页面和 Grok 风格时钟使用本地 LVGL 矢量绘制；不打包 VibeBuddy 上游图像文件。

## 再分发时

1. 保留上表所列素材的版权、许可和来源文件。
2. 发布 soRound_os 衍生固件时遵守已取得的作者许可条款；不要把它描述为某种上游开源许可证下的发布。
3. 不要提交本机 Wi-Fi 密码、Codex 凭据或会话数据。
