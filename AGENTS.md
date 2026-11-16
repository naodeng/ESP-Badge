# 项目协作约定

本文件适用于仓库根目录及其子目录。按任务读取背景：产品范围见 [PRODUCT.md](PRODUCT.md)，系统架构见 [design.md](design.md)，开发/部署/调试见 [docs/README.md](docs/README.md)，来源与许可见 [THIRD_PARTY.md](THIRD_PARTY.md) 和 [Jarvis_OS/UPSTREAM.md](Jarvis_OS/UPSTREAM.md)。

## 当前项目边界

- 主固件：`firmware/jarvis_os/`。
- iOS App：`apps/ios/moto-gps/`。
- Node.js 与 Cloudflare Worker 网关：`services/moto-gps-gateway/`。
- BLE、坐标、导航核心、UI 和离线地图共享代码：`packages/moto-gps/shared/`。
- 两个仅用于本地开发的调试工具：`tools/debug/`。
- 项目名与正式固件名固定为 `Jarvis_OS`。只生成一个发布固件，不维护 stable/beta 两个产品镜像。
- Android App、独立 Moto GPS ESP 固件、产品网站、OTA、旧 Codex/Travel/音频等 app 不属于当前产品。
- `Jarvis_OS/UPSTREAM_README.md` 与英文版是保留的 soRound_os 历史说明；当前使用说明以根目录文档为准。
- 当前设计以根目录 `design.md` 为准。旧设计与实施记录放在 `docs/design/history/`、`docs/plans/`，不得当成当前代码行为。

## 修改原则

- 先确认当前代码行为，再改对应模块；避免为局部改动批量格式化或改名无关文件。
- 固件页面遵循 `app_t` 生命周期；页面遮挡时维护 visibility、tick 暂停/恢复和退出清理语义。
- UI 保持 466 × 466 圆屏安全区和 soRound_os 黑、白、灰、红视觉约定。
- BLE 输入需校验帧长度、序号、连接会话和消息载荷；断连或超时后清除旧导航状态。
- iOS 是定位、路线、地图和服务端请求端；固件不连接 Wi-Fi 或外部导航服务。
- Worker 密钥只用 Wrangler Secret/Cloudflare 控制台配置，不写进代码、`wrangler.jsonc` 的 vars、日志或文档。
- 不要直接修改被忽略的 `managed_components/`、`build/`、本机 `sdkconfig` 或 `.toolchain/` 来留下不可复现的改动。组件依赖变更写入 `idf_component.yml`，并更新受版本控制的 `dependencies.lock`。
- 改分区、NVS 格式、BLE 配对持久化或录音存储前，确认已有用户配置和录音兼容性，并给出备份/迁移方案。本项目当前设备刷写只覆盖活动 app 槽。
- 保留上游归属、作者许可确认、第三方许可证、NOTICE、地图署名和历史来源说明。

## 本机文件与提交

以下内容是本机状态，不应提交：

- `.toolchain/`、固件 `build/`、`managed_components/`、本机 `sdkconfig` 和 `sdkconfig.old`。
- `secrets.h`、AMap Key、Apple 签名配置、Cloudflare 凭据、定位/路线数据、设备会话与本地缓存、Wi-Fi 凭据、Python 虚拟环境和生成的 `.egg-info/`。
- `node_modules/`、Wrangler 本地状态、Xcode `DerivedData/` 与 `xcuserdata/`。
- `.impeccable/`、`.superpowers/` 等本地设计/Agent 状态。

添加工具前检查账号、密钥、局域网地址、机器专属绝对路径和用户数据。只提交可重建源文件、必要素材、测试夹具与文档。

## 固件构建

使用 ESP-IDF 5.5.x。将构建目录和本机 `sdkconfig` 放在仓库外，避免覆盖现有忽略文件：

```sh
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
export IDF_TARGET=esp32s3
source .toolchain/esp-idf/export.sh
idf.py -C firmware/jarvis_os \
  -B /tmp/jarvis-os-build \
  -D SDKCONFIG=/tmp/jarvis-os-sdkconfig \
  build
```

触摸驱动补丁针对已验证的 `esp_lvgl_port` 版本；版本校验失败时检查依赖源和差异，再更新补丁。正式产物固定为 `Jarvis_OS.bin`。部署命令和设备应用分区偏移以 [部署说明](docs/deployment/README.md) 为准。

## 验收与报告

- 按用户要求构建/部署时报告具体 ESP-IDF 版本、产物大小与校验和、写入地址、串口启动结果。
- 将模拟器、主机测试、iOS Simulator、设备启动日志、iPhone BLE 实测和实际路线验收分开表述；一项通过不能代替另一项。
- 设备验证需单独报告实际步骤。固件启动成功不等于屏幕触摸、BLE 配对、后台定位、离线地图、媒体控制或长途导航已通过。
- 不要运行与修改无关的大规模测试；用户要求测试、构建验证或设备调试时，运行与目标对应的检查。
