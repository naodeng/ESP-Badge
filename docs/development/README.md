# 开发指南

## 仓库入口

| 目标 | 工程路径 |
|---|---|
| Jarvis_OS ESP-IDF 固件 | `firmware/jarvis_os/` |
| MOTO GPS iOS App | `apps/ios/moto-gps/` |
| Moto GPS 网关 | `services/moto-gps-gateway/` |
| BLE/导航/地图共享模块 | `packages/moto-gps/shared/` |
| 本地调试工具 | `tools/debug/` |

## 固件环境

使用 ESP-IDF 5.5.x、CMake、Ninja 和 Python。Codex Desktop 工作区可使用仓库本机 `.toolchain/`；工具链不纳入版本控制。构建时把输出目录和本机 sdkconfig 放到仓库外：

```sh
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
export IDF_TARGET=esp32s3
source .toolchain/esp-idf/export.sh
idf.py -C firmware/jarvis_os \
  -B /tmp/jarvis-os-build \
  -D SDKCONFIG=/tmp/jarvis-os-sdkconfig \
  build
```

正式源配置为 `firmware/jarvis_os/sdkconfig.defaults`，组件版本由 `dependencies.lock` 固定。不要直接编辑 `managed_components/`；依赖变更必须修改组件清单和锁文件。构建时若碰到组件管理器的进程权限限制，需要在受控权限下重跑构建，不要手动修改组件缓存。

## 查看设备实屏

使用 [Jarvis_OS USB 实屏预览说明](../../tools/debug/jarvis-screen-viewer/README.md) 在本地浏览器查看真实设备画面、保存 PNG 或录制 WebM。该工具需要单独构建的调试固件，正式构建默认关闭镜像功能。

## iOS App

要求 Xcode 16 或更新版本、iOS 17 SDK 与 XcodeGen。进入 `apps/ios/moto-gps/`：

```sh
xcodegen generate
open MotoGPS.xcodeproj
```

命令行构建示例：

```sh
xcodebuild -project MotoGPS.xcodeproj -scheme MotoGPS \
  -destination 'generic/platform=iOS Simulator' build
```

签名和真机部署使用自己的 Apple Team。Simulator 无法验证 CoreBluetooth 与真实 ESP32 连接。

## 网关本地开发

Node 版本和入口脚本见 `services/moto-gps-gateway/package.json`；本地运行参数放在忽略的 `.env`。Worker 开发另见 [部署说明](../deployment/README.md)。不要将 AMap Key 放入代码或 shell 命令历史。

## 共享协议

设备协议、fixture 和导航数据结构位于 `packages/moto-gps/shared/`。调整帧格式时，同步更新固件 transport、iOS BLE、fixture 与协议说明。迁移必须有版本兼容规则，不能只更新一端。

## 文档和归档

- 当前行为写回根目录 `README.md`、`PRODUCT.md`、`design.md` 或本目录。
- 阶段计划放在 `docs/plans/`，完成后标注结果或移动到历史目录。
- 旧产品功能文档保留为历史资料时，需注明“不代表当前产品”，不要作为使用入口。
