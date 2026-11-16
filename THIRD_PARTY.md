# 第三方来源与许可

本仓库只发布 Jarvis_OS 固件、Moto GPS iOS App、Moto GPS 网关与其共享协议/资源。分发源码、二进制或地图素材时，按对应上游要求保留归属、许可及署名文件。

| 来源 | 使用位置 | 说明 |
|---|---|---|
| soRound_os | `firmware/jarvis_os/` 中保留的系统与硬件适配代码；历史说明位于 `Jarvis_OS/` | 上游：[soBigRice/soRound_os](https://github.com/soBigRice/soRound_os)。上游快照未包含通用 LICENSE；项目所有者已确认取得作者源码复用和修改许可。该确认不等于上游发布了开放源代码许可证。 |
| Moto GPS Waveshare | `apps/ios/moto-gps/`、`services/moto-gps-gateway/`、`packages/moto-gps/shared/`、`tools/debug/` | 来源：[mx3353672833-debug/moto-gps-waveshare](https://github.com/mx3353672833-debug/moto-gps-waveshare)。按来源仓库的 PolyForm Noncommercial 1.0.0 条款使用。完整文本、NOTICE 和第三方声明见 [`licenses/moto-gps/`](licenses/moto-gps/)。 |
| OpenStreetMap / Protomaps 地图数据 | 共享离线地图与网关瓦片链路 | OSM 数据受 ODbL 管辖；地图显示、导出或再分发时保留相应署名。详情见 `packages/moto-gps/shared/offline_map/ATTRIBUTION.txt` 和地图目录 README。 |
| ESP-IDF 与组件 | 固件构建依赖 | 由 ESP-IDF Component Manager 按 `firmware/jarvis_os/dependencies.lock` 固定版本获取。依赖自身的许可证随组件元数据保留。 |
| LVGL | 固件 UI 与 WASM 调试模拟器 | LVGL 采用 MIT License；仓库保留组件声明和依赖锁定信息。 |
| Waveshare ESP LCD / touch drivers | 固件硬件接口 | 组件许可见 `licenses/moto-gps/LICENSES/` 和 ESP-IDF Component Manager 元数据。 |
| Montserrat 与 Source Han Sans | 固件/iOS/地图界面的字体素材 | 字体许可及来源副本见 `licenses/moto-gps/LICENSES/` 与 Moto GPS 第三方声明。 |

## 发布检查

1. 保留 soRound_os 上游说明、Moto GPS LICENSE/NOTICE、第三方许可和 OSM 署名。
2. 发布 soRound_os 衍生固件时遵守已取得的作者授权；不要将其误述为上游某个通用开源许可证下的发布。
3. 不提交 AMap Web Service Key、Cloudflare 凭据、Apple 签名材料、定位数据、Wi-Fi 凭据或设备用户数据。
4. 发布离线地图或在线地图截图/瓦片时，遵循实际地图数据源的许可和署名要求。
