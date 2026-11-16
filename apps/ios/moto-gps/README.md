> **语言 / Language:** 中文 · [English](README.en.md)

# MOTO GPS iOS App

该 App 与 Jarvis_OS 固件配套，负责 iPhone 定位、地点搜索、路线规划、在线/离线地图、导航状态同步和 Apple Music 基础控制。路线、搜索与地图请求通过 HTTPS 网关；设备连接使用加密 BLE。

## 开发

要求 Xcode 16+、iOS 17 SDK 和 XcodeGen。进入本目录：

```sh
xcodegen generate
open MotoGPS.xcodeproj
```

命令行 Simulator 构建：

```sh
xcodebuild -project MotoGPS.xcodeproj -scheme MotoGPS \
  -destination 'generic/platform=iOS Simulator' build
```

Swift Package 测试：

```sh
swift test
```

Simulator 可以检查不依赖蓝牙、真实定位、后台执行或音乐控制的界面流程；CoreBluetooth 与 ESP32 BLE 必须用 iPhone 真机验收。

## 网关与签名

默认网关根地址配置在 `project.yml` 的 `MOTOGPSGatewayBaseURL`。App 也提供网关设置覆盖运行时地址。Worker Key 不进入此 App，也不存储在设备上。网关部署见[仓库部署文档](../../../docs/deployment/README.md)。

Bundle ID、Apple Team、签名证书和 TestFlight 上传凭据使用开发者自己的 Apple 账户配置，不随仓库提供。隐私文案和权限描述位于 `App/DataUseView.swift`、`App/Info.plist` 与 `App/PrivacyInfo.xcprivacy`。

项目结构与 BLE 协议参见[系统架构](../../../design.md)及 `../../../packages/moto-gps/shared/protocol/`。
