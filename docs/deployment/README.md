# 部署与发布

## 1. 构建单一 Jarvis_OS 固件

采用 ESP-IDF 5.5.x 和隔离的临时构建配置：

```sh
export IDF_TOOLS_PATH="$PWD/.toolchain/espressif"
export IDF_TARGET=esp32s3
source .toolchain/esp-idf/export.sh
idf.py -C firmware/jarvis_os \
  -B /tmp/jarvis-os-release-build \
  -D SDKCONFIG=/tmp/jarvis-os-release-sdkconfig \
  build

mkdir -p release
cp /tmp/jarvis-os-release-build/Jarvis_OS.bin release/Jarvis_OS.bin
shasum -a 256 release/Jarvis_OS.bin > release/Jarvis_OS.bin.sha256
```

正式输出只有 `release/Jarvis_OS.bin` 一个固件文件；SHA-256 旁文件用于安装前核对。二进制被 `.gitignore` 忽略，可通过上述命令重建。构建不应使用或覆盖仓库内忽略的本机 `sdkconfig`。

## 2. 在当前设备安全安装

当前设备已识别为 ESP32-S3，分区布局包含 `ota_0` (`0x20000`，8 MiB)、`ota_1`、NVS、storage 和录音分区。此次更新只写 `ota_0` 应用地址：

```sh
python -m esptool --chip esp32s3 -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_size 32MB --flash_freq 80m \
  0x20000 release/Jarvis_OS.bin
```

运行前先按 `ls /dev/cu.usbmodem*` 确认端口；如设备端口不同，先确认是目标 Waveshare 板卡。**不要把 `idf.py flash` 或 `flash_args` 用于这次保留用户数据的更新**：常规完整烧录会同时写 bootloader、分区表和其他数据区域。本命令只包含一个写入地址，不擦除 NVS、配对信息、用户设置、storage 或录音。

刷写结束后连接串口监视器，检查芯片启动、`Project name: Jarvis_OS`、显示/触摸/IMU 初始化和启动自检结果。任何启动循环或 panic 都应先停止重复刷写并保存串口日志。

## 3. 部署 Moto GPS Worker

源码和 Wrangler 配置在 `services/moto-gps-gateway/cloudflare/`。已配置 Worker 名称 `moto-gps-gateway`、私有 R2 桶 `moto-gps-map-cache` 和 UTC Cron `17 3 * * *`。该 Cron 只执行地图源元数据刷新，与固件 OTA 无关。

在已授权目标 Cloudflare 账户的终端中，先进入 Worker 目录：

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
npx wrangler whoami
npm run build
npx wrangler secret put AMAP_WEB_SERVICE_KEY \
  --config wrangler.jsonc
npm run deploy
```

`secret put` 会交互式读取 Key。不要把 Key 粘贴到聊天或直接放在命令参数、配置文件和日志中。已有部署的资源/Secret 应先核对，避免无意覆盖控制台配置。

Worker 默认接口没有用户身份认证，访问范围为公开 HTTPS；配置中的 CORS 和 IP 限流不等于身份鉴权。若要限制消费者，需另行规划认证机制。R2 保持私有，`tiles/` 生命周期按部署说明在 Cloudflare 控制台设为 30 天。

Wrangler 配置和本仓库源码不会证明线上部署、Secret、桶权限或 Cron 已生效。部署后在 Cloudflare 控制台检查 Worker 版本、Settings → Triggers 中的 Cron、R2 绑定和 Secret 名称，再请求 `/healthz` 与真实搜索/路线接口。此轮只读线上状态检查未能完成，发布前需在控制台确认。

## 4. 将 App 指向网关

当前默认 HTTPS 根地址：

```text
https://moto-gps-gateway.naodeng.workers.dev/
```

构建默认值来自 `apps/ios/moto-gps/project.yml` 的 `MOTOGPSGatewayBaseURL`。用户也可在 App 的“网关设置”里保存自己的根地址。若自建 Worker 或设置了路径前缀，App 地址必须使用对应 Worker 域名和 API 根路径。

App Store/TestFlight 签名、Bundle ID、团队和上传凭据属于发布者的 Apple 账户配置，不纳入仓库或本地调试文档。

## 5. 网关本地服务

Node 服务可在本机运行用于开发：

```sh
npm ci --prefix services/moto-gps-gateway
npm start --prefix services/moto-gps-gateway
```

本地环境变量放在该服务忽略的 `.env` 文件。Worker 的 `.dev.vars` 和生产 Secret 相互独立。
