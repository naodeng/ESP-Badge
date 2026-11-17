> **语言 / Language:** 中文 · [English](README.en.md)

# Cloudflare Worker 部署

本目录是 Moto GPS 网关的 Cloudflare 部署目标，使用 Wrangler、Worker 与私有 R2 缓存。完整部署、健康检查、AMap Key 白名单和排错流程见主仓库[后端服务部署指南](../../../docs/backend/deployment.md)。固件安装另见[固件部署总览](../../../docs/deployment/README.md)。

## 配置

- Worker 名称：`moto-gps-gateway`。
- R2 binding：`MAP_CACHE` → `moto-gps-map-cache`（私有桶）。
- AMap provider：由 `MOTO_PROVIDER=amap` 启用。
- Secret：`AMAP_WEB_SERVICE_KEY`；用 Wrangler 的交互输入配置。
- Cron：UTC `17 3 * * *`，负责检查地图源元数据。
- 当前配置没有用户身份认证；CORS 和 Rate Limiting 不等于认证。

## 命令

从仓库根目录：

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
npm test
npm run build
npx wrangler whoami
npx wrangler secret put AMAP_WEB_SERVICE_KEY
npm run deploy
```

本地 Secret 可放在被忽略的 `.dev.vars`；不要提交它。部署前确认 Cloudflare 账户、R2 桶和线上 Secret 归属。配置文件只描述期望配置，部署后仍需在 Cloudflare 控制台检查 Worker version、R2 binding、Secret 名称和 Cron Trigger。

真实 endpoint、iOS 默认地址与健康检查含义见主仓库[手机 App 配置指南](../../../docs/mobile-app/configuration.md)。
