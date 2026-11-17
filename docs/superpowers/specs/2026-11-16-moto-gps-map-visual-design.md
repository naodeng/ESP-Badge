# MOTO GPS 圆屏地图视觉改进

状态：用户已确认；按此说明完成实现
范围：仅调整 Jarvis_OS 圆屏导航地图样式

## 意图

让 466 × 466 圆屏上的道路与导航路线更容易辨认，并增加白天、夜间两种地图色板。昼间底图采用用户确认的较亮概念；夜间底图维持近黑色。概念图见 [`moto-gps-day-night-map-concept.png`](../../design/mockups/moto-gps-day-night-map-concept.png)。该图是视觉稿，不是设备截图或真实地图数据。

## 已选方案

保留当前 OSM / Protomaps 道路和建筑数据、离线场景及 BLE `MapScene` 协议，只调整设备端渲染。道路按现有等级拉开线宽与亮度；规划路线保持白色并使用深色描边，当前位置使用红色箭头；建筑轮廓保持低对比，不盖过导航路线。保持现有页面布局和圆屏安全区域。

另外两种方案不纳入本轮：

1. 增加水面、绿地、地名或 POI：需要新增地图数据、存储和协议容量，超出已选的“保留数据与协议”范围。
2. 移植 MotoNav 的 Android 地图截图/栅格处理链路：当前产品使用 iOS App 与 OSM / Protomaps 场景，不维护 Android App；移植会改变地图数据链路和供应商依赖。

## 昼夜切换（已确认）

固件按设备本地时间自动切换：06:00–17:59 使用昼间色板，18:00–05:59 使用夜间色板；时间未校准时使用夜间色板。页面打开期间每 60 秒检查一次时间，跨过切换边界后会在下一次检查时更新，最多延迟约一分钟；不需要增加 BLE 字段、手机设置页或新的用户操作。

## 目标与边界

- 昼间：明亮的中性灰底图，主干道更醒目，次级道路较细，建筑轮廓可辨但克制。
- 夜间：近黑底图，降低道路和建筑亮度，规划路线与当前位置仍清楚可见。
- 保持当前导航卡片、速度显示、动画、地图投影与数据更新行为。
- 不添加地图要素、不改 BLE 协议、不改 iOS 地图、网关、离线数据库或地图来源。
- 不复制 MotoNav 源代码、截图、瓦片或地图素材；只独立实现通用视觉层次。

## 来源与许可记录

用户确认本项目用于个人非商业用途，并同意注明参考来源。根目录 `THIRD_PARTY.md` 已添加 MotoNav 参考说明，注明链接、参考范围和未复制代码/素材。MotoNav 仓库的自有内容条款限个人非商业用途，并规定免费分享修改版时保留条款、归属并注明修改；地图、SDK 等第三方数据仍按各自条款处理。若后续改为直接复用其代码，必须另行保留完整许可文本、作者归属并标记修改内容。详见 MotoNav 的 [LICENSE.md](https://github.com/CharlesXu/MotoNav/blob/main/LICENSE.md)。

参考仓库：[CharlesXu/MotoNav](https://github.com/CharlesXu/MotoNav)；其地图来源与取图设计见 [README](https://github.com/CharlesXu/MotoNav/blob/main/README.md)、[MapFeeder.kt](https://github.com/CharlesXu/MotoNav/blob/main/android/app/src/main/java/com/example/motonav/MapFeeder.kt) 和 [AmapSdkRaster.kt](https://github.com/CharlesXu/MotoNav/blob/main/android/app/src/main/java/com/example/motonav/AmapSdkRaster.kt)。

## 验收标准

- 白天地图明显比夜间明亮，但顶部导航信息、速度文字和路线仍清晰。
- 主路、次路、支路和建筑轮廓可区分；路线始终是地图上最醒目的线形信息。
- 日夜切换不改变 BLE 消息、地图数据、导航状态、圆屏安全区域或页面布局。
- 无有效系统时间时页面稳定使用夜间色板，不闪烁、不反复切换。

## 实施记录

实现限于共享固件导航 UI 和根目录 `THIRD_PARTY.md`。未修改地图数据、BLE 协议、iOS App、网关或地图来源；未复制 MotoNav 代码或素材。当前记录不代表已在设备上构建或验证。
