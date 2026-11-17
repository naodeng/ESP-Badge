# MOTO GPS 手机 App 配置指南

本文说明 iPhone App 的网关地址、Xcode 签名、系统权限和真机安装配置。App 的源码位于 `apps/ios/moto-gps/`；Cloudflare Worker 的 AMap Key 只配置在服务端，不放进 App。

## 1. 配置项一览

| 配置 | 当前来源 | 作用 |
| --- | --- | --- |
| 网关初始状态 | 仓库和安装包不内置共享地址；首次使用时在 App“网关设置”中填写自己的 Worker 根地址 | 地点搜索、路线和地图请求的 HTTPS 服务地址 |
| 网关运行时覆盖 | App 的“网关设置”，保存在 iPhone `UserDefaults` 的 `MotoGPS.GatewayBaseURL.v1` | 可切换到自建或测试网关；保存后立即生效 |
| AMap Web 服务 Key | Cloudflare Worker Secret `AMAP_WEB_SERVICE_KEY` | Worker 调用高德搜索和路线服务；不能放进 iOS 包 |
| Bundle ID / Apple Team | Xcode 的 `MotoGPS` target 签名设置 | 将开发版安装到开发者自己的 iPhone |

网关地址由每位部署者/使用者自行配置。项目不会把维护者的个人 Worker 地址打包给其他人。

## 2. 设置网关地址

在 App 首页点左上角齿轮，打开 **网关设置**。填写完整的 HTTPS **根地址**，例如：

```text
https://your-worker.example.workers.dev/
```

如果 Worker 使用了路径前缀，根地址也要包含此前缀，例如：

```text
https://example.com/moto-gps/api/
```

不要填写 `/healthz`、`/v1/places` 或 `/v1/routes` 等具体接口地址；不要在地址中放 Key、用户名、密码、查询参数或 `#`。App 会规范化末尾斜杠，并在需要时自行拼接 API 路径。

点击 **测试连接** 后，App 发送 `GET <网关根地址>/healthz`。测试只检查 Worker 是否可访问、返回的健康状态是否有效，以及是否报告启用了实时导航配置；它**不会向高德发起请求，也不能证明 AMap Key 可用**。高德搜索和路线要通过实际地点搜索或路线规划验证。

直接打开根地址 `/` 可能返回 `NOT_FOUND` JSON，因为 Worker 没有为根路径定义 API。这个响应表示浏览器已经连到 Worker，但不是健康检查结果；请检查完整的 `/healthz` 路径。

将你自己的 Worker 地址保存后，如果 iPhone 内测试失败，应核对 App 中保存的地址及 iPhone 当前网络。原生 iOS 的 `URLSession` 不受浏览器 CORS 白名单约束，通常不需要为 iPhone 配置 `WEB_ORIGIN`。

网关地址保存在手机本地，保存后搜索、路线和周边地图会切换到新地址。更换地址会清除当前搜索并暂停地图下载；已下载的离线地图会保留。导航进行中不能更换地址。

## 3. Xcode 签名与真机安装

要求 Xcode 16 或更新版本、iOS 17 SDK 和 XcodeGen。项目生成入口：

```sh
cd apps/ios/moto-gps
xcodegen generate
open MotoGPS.xcodeproj
```

在 Xcode 中选择 `MotoGPS` App target → **Signing & Capabilities**：

1. 打开 **Automatically manage signing**。
2. 选择自己的 Apple Developer Team。
3. 将 Bundle Identifier 改为该 Team 可签名的唯一标识。
4. 将 iPhone 连接 Mac，在设备选择器中选中该手机并运行 `MotoGPS` scheme。
5. 如果 iOS 要求启用开发者模式或信任开发者，按系统提示完成。

仓库模板 `project.yml` 的 Bundle ID 是 `org.example.motogps`，Development Team 为空，不能直接代表你的 Apple 账户。`project.yml` 是 XcodeGen 的生成来源；重新执行 `xcodegen generate` 会重新生成 `.xcodeproj`，需把本机签名设置写入本地配置或重新在 Xcode 中选择。不要把签名证书、私钥或上传凭据提交到仓库。

模拟器可编译和检查大多数界面，但不能验证与 ESP32 的真实 BLE、真实 GPS、后台定位和 Apple Music 控制。BLE 必须用 iPhone 真机验收。

## 4. iPhone 权限

首次使用对应功能时，iOS 会请求权限。权限可在 **设置 → 隐私与安全性** 或 **设置 → MOTO GPS** 中调整。

| 权限 | 用途 | 拒绝后的影响 |
| --- | --- | --- |
| 蓝牙 | 查找并连接 Jarvis_OS 圆屏，发送导航状态 | App 无法连接圆屏 |
| 定位（使用 App 期间） | 按当前位置排序地点、规划路线 | 搜索位置偏差或无法规划当前路线 |
| 定位（始终/后台） | 导航期间锁屏或切换 App 后继续更新位置 | App 进入后台后导航位置可能停止更新 |
| Apple Music / 媒体资料库 | 在圆屏显示曲目并控制播放 | 不影响搜索和导航 |

App 的导航位置和设备 BLE 状态分属不同链路：**网关测试成功不代表 BLE 已连接；BLE 已连接也不代表高德路线服务可用**。隐私和数据流说明见 App 内“隐私与数据”页面及 `App/DataUseView.swift`。

## 5. 网关与 Key 的职责边界

- iPhone 只保存网关地址，不保存 AMap Key。
- Worker Secret 名称是 `AMAP_WEB_SERVICE_KEY`。通过 Wrangler 或 Cloudflare 控制台安全录入，禁止写入 `project.yml`、`Info.plist`、源码、截图或聊天记录。
- 当前 AMap Key 没有配置 IP 白名单，因此不限制 Worker 出口 IP。若以后启用白名单，应填写服务端实际稳定的出口 IP；不是 iPhone 的公网 IP，也不是开发 Mac 的 IP。当前 Worker 配置没有声明固定出口地址。
- `WEB_ORIGIN` 是浏览器跨域响应配置。原生 iOS App 用 `URLSession` 访问 API，不需要按 iPhone 域名加 CORS 白名单。

后端配置和部署过程见[后端服务部署指南](../backend/deployment.md)。

## 6. 常见问题

| 现象 | 检查方法 |
| --- | --- |
| 地址无法保存 | 使用完整 HTTPS 根地址；移除 `/healthz`、`/v1/...`、查询参数和片段 |
| App 显示“连接失败” | 在设置页核对你自己的 Worker 根地址；用 iPhone Safari 打开 `<你的网关根地址>/healthz`。不要用根路径 `/` 的 `NOT_FOUND` 判断健康检查失败。如果其他 Cloudflare Worker 可访问而这个地址不行，检查 Worker 域名、部署状态和手机网络；也可给自己的 Worker 绑定自有域名，并将 App 根地址切换到该域名。健康路径能打开但 App 失败时，再检查 App 保存地址和已安装版本 |
| 显示已连接但地点搜索失败 | 这是 AMap/搜索服务链路问题，不是 BLE。检查 Worker Secret、Key 平台类型、配额和 Worker 运行日志 |
| App 测试通过，但实际路线失败 | `/healthz` 是配置健康检查，不会验证 AMap Key 的实时请求。实际搜索或规划路线一次，并查看 Worker/AMap 错误码 |
| 圆屏一直未找到 | 先在设备打开 MOTO GPS 页面，再在 App“我的圆屏”中点“连接圆屏”；详见[手机与圆屏连接指南](../connection-guide.md) |

开发构建与设备调试入口见[开发指南](../development/README.md)和[调试与验收指南](../debugging/README.md)。
