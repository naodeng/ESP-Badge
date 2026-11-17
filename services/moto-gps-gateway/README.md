> **语言 / Language:** 中文 · [English](README.en.md)

# Moto GPS 网关

网关服务为 MOTO GPS iOS App 提供地点搜索、路线、行政区划和在线地图瓦片接口。Node.js 服务和 Cloudflare Worker 共用同一业务逻辑。

## 本地 Node 服务

要求 Node.js 20+。在仓库根目录运行：

```sh
cd services/moto-gps-gateway
npm ci
cp .env.example .env
node --env-file=.env src/server.js
```

`.env.example` 默认使用 `MOTO_PROVIDER=disabled`，不会访问真实地图服务。需要实时联调时再将本地忽略的 `.env` 改为 `MOTO_PROVIDER=amap` 并通过安全的本机方式提供 Key；不要写入 App、设备固件或 Git。

## Cloudflare Worker

Worker 配置、测试与 R2 binding 位于 `cloudflare/`。生产部署、Key、Cron、AMap 白名单和健康检查流程见[后端服务部署指南](../../docs/backend/deployment.md)。简要命令：

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
npm test
npx wrangler login
npx wrangler whoami
npx wrangler secret put AMAP_WEB_SERVICE_KEY
npm run deploy
```

Worker 默认不做用户身份认证。CORS 和限流不等于认证；公网使用前应确认接口访问范围和 Cloudflare 限额。Secret 由 Wrangler 交互式读取，不要粘贴到聊天或 commit。

## API

| 路径 | 用途 |
|---|---|
| `GET /healthz` | 配置能力与服务健康状态 |
| `GET /v1/places` | 地点搜索 |
| `POST /v1/route-options` | 候选路线 |
| `POST /v1/routes` | 路线、导航刷新和偏航重算 |
| `GET /v1/map/cities` | 城市/区县搜索 |
| `GET /v1/map/tiles/{z}/{x}/{y}` | 周边道路/建筑数据 |

共享 schema、BLE 协议和离线地图在 `packages/moto-gps/shared/`。完整设计见[系统架构](../../design.md)。
