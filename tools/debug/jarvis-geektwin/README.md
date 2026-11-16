# Jarvis GeekTwin 调试工具

此浏览器工具原本通过 Web Bluetooth 连接 soRound Twin 调试服务，解析 IMU 与电量帧并显示设备姿态。它不是产品网站，也不包含在 iOS 或固件发布包中。

## 本地开发

需要 Node.js 20+ 和支持 Web Bluetooth 的桌面 Chrome/Edge：

```sh
npm ci
npm run dev
```

通过 `http://localhost` 打开；浏览器权限只在用户发起连接时请求。

## 当前固件兼容性

这个工具使用旧的 Twin GATT 服务和 20-byte IMU 数据帧；当前 Jarvis_OS 正式固件暴露的是 Moto GPS GATT 协议，因此两者目前不能直接配对。需要调试 IMU BLE 时，应先决定如何在不干扰 Moto GPS 服务的情况下增加调试模式，再同步更新本工具的 UUID/协议和设备固件。不要把旧帧当成 Moto GPS 协议。

此工具保留在仓库用于后续适配，当前版本只适用于仍提供旧 Twin GATT 的设备固件。产品架构见[系统设计](../../../design.md)。
