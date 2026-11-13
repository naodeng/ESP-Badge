# JARVIS OS 点阵标志与开机动画

这组设计资产给 466 × 466 圆屏使用。系统标志和开机图标分开绘制：关于页保留白色 J 点阵标志；开机动画使用青蓝能量反应堆点阵，包含圆形外环、分区面板与倒三角核心。

## 设计资产

- `logo.svg` / `logo-dark.svg`：透明背景的浅色、深色标志。
- `about.svg` / `about-en.svg`：关于本机页面的中英文参考稿。
- `boot.svg`：4 秒参考时间轴。反应堆约 1.8 秒形成，随后显示系统名和自检状态。
- `index.html`：可播放、暂停、拖动时间轴、切换底色与语言的单页预览；原生终帧在 `native/startup-20261112/boot-final.png`。
- `generate.py`：从同一组几何参数生成 SVG、预览页和 `main/identity_geometry.h`。
- `../../main/identity_ui.c`：LVGL 点阵原生绘制和启动动画；不依赖 SVG 解码或播放视频。
- `../../tools/gen_identity_fonts.swift`：生成启动页与关于页使用的 JARVIS OS 字形子集。

重新生成资产：

```sh
cd Jarvis_OS/GeekTool-IDF
python3 artwork/identity/generate.py
swift -module-cache-path /private/tmp/jarvis-swift-cache tools/gen_identity_fonts.swift
```

## 动画节奏

| 时间 | 画面 |
| --- | --- |
| 0.10 秒起 | 青蓝外环从顶部开始顺时针扫描 |
| 0.22–1.80 秒 | 分区面板与倒三角核心按行点亮，三个核心节点依次显现 |
| 1.82–2.30 秒 | `JARVIS OS` 淡入并上移 |
| 2.50–2.80 秒 | 系统检查状态淡入 |
| 4 秒以后 | 保持最终画面；最短动画时间和启动自检都完成后才显示表盘 |

启动时长、自检门控、错误提示和离线提示沿用现有启动流程。开机图标独立于关于页的 JARVIS 品牌标志。

## 实现位置

- 原生标志与覆盖层：`main/identity_ui.c`、`main/identity_ui.h`
- 共享几何：`main/identity_geometry.h`（由 `generate.py` 生成）
- 关于本机名称：`main/app_settings.c`
- 原生字形：`main/font_identity.c`（由 `gen_identity_fonts.swift` 生成）
- 原生渲染回归：`tests/host/identity_ui_tests.c`
