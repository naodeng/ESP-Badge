# 调试与验收

## Jarvis_OS 真机启动

1. 连接设备并确认目标串口，例如 `/dev/cu.usbmodem1101`。
2. 核对发布固件 `release/Jarvis_OS.bin` 的 SHA-256。
3. 只在 `0x20000` 写入应用镜像；不要烧写 bootloader、分区表或 storage 镜像。
4. 打开串口监视器，保存从复位到首页稳定运行的日志。
5. 检查系统名、显示驱动、触摸、RTC、IMU、自检结果和 panic/reset reason。
6. 手动验证启动器、锁屏/唤醒、设置页、表盘和 MOTO GPS 页。记录每项实际执行结果。

ESP-IDF 的 monitor 入口：

```sh
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
source .toolchain/esp-idf/export.sh
idf.py -C firmware/jarvis_os \
  -B /tmp/jarvis-os-release-build \
  -D SDKCONFIG=/tmp/jarvis-os-release-sdkconfig \
  -p /dev/cu.usbmodem1101 monitor
```

若端口不同，替换成实际设备端口。监视串口与刷写是分开的操作；捕获日志后用 `Ctrl+]` 退出。

## iPhone BLE 联调

- 使用真机 iPhone；Simulator 不提供到 ESP32 外设的真实 BLE 链路。
- 首次连接授权蓝牙并完成系统配对，确认设备名为 `Jarvis_OS`。
- 验证 App 内连接状态、设备 Ready、开始导航、转向/进度、暂停/结束、断连和重连。
- 检查旧连接的导航快照不会在新会话中重现。
- 导航运动传感器和后台定位需在静止、安全环境中先验证。

## Moto GPS 模拟器

目录：`tools/debug/moto-gps-simulator/`。该浏览器/WASM 工具供开发时查看共享圆屏 UI、路线样例和场景状态，不是生产网站。构建与运行细节见目录内 `shell/README.md` 和 `wasm/README.md`。模拟器通过不代表固件硬件显示、真实 BLE 或真实地图接口已通过。

## Jarvis GeekTwin

目录：`tools/debug/jarvis-geektwin/`。它用于检查 IMU/电量帧并渲染数字孪生。当前工具仍使用旧 soRound Twin BLE 服务，与 Jarvis_OS 的 Moto GPS GATT 协议不同；当前发布固件不提供 Twin GATT 服务，所以它暂时不能直接连到这版固件。不要把旧 20-byte 帧误认为 Moto GPS 协议。需要用该工具调 IMU 时，应先在单一固件中完成明确的调试模式/协议适配。

## 网关调试

- Node 与 Worker 的日志、测试与本地 fixture 查看 `services/moto-gps-gateway/README.md`。
- 不要在公开 issue 或日志中输出 AMap Key、完整用户定位、真实地址或路线响应。
- 模拟路线和 `/healthz` 能力标记不等于真实 AMap 请求成功。线上搜索、路线、地图源、R2 与 Cron 分开验收。

## 结果记录模板

```text
日期/固件 SHA-256：
设备/串口：
刷写地址：0x20000（仅 app）
串口启动：
显示/触摸/IMU：
锁屏与唤醒：
MOTO GPS 页：
iPhone BLE：未测 / 通过 / 失败
网关真实请求：未测 / 通过 / 失败
未验证或问题：
```
