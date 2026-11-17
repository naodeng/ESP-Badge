# Moto GPS 后端服务部署指南

本指南针对主仓库中的 `services/moto-gps-gateway/`，包含 Cloudflare Worker 生产部署和 Node.js 本地调试。后端为 MOTO GPS iPhone App 提供地点搜索、路线规划和周边地图接口；圆屏本身不直接访问互联网。

## 1. 服务和配置

| 项目 | 当前配置 |
| --- | --- |
| Worker 名称 | `moto-gps-gateway` |
| 部署后地址 | 由部署者的 Cloudflare 账户生成；不写入仓库或 App 默认配置 |
| AMap Key | Cloudflare Secret `AMAP_WEB_SERVICE_KEY` |
| Provider | `MOTO_PROVIDER=amap` |
| 地图模式 | `MOTO_MAP_PMTILES_URL=auto`，地图缓存绑定 R2 |
| R2 binding | `MAP_CACHE` → `moto-gps-map-cache`（私有桶） |
| 定时任务 | UTC `17 3 * * *`，刷新地图源元数据 |
| 部署配置 | `services/moto-gps-gateway/cloudflare/wrangler.jsonc` |

主要 API：

| 路径 | 方法 | 用途 |
| --- | --- | --- |
| `/healthz` | GET | Worker 健康检查和能力状态 |
| `/v1/places` | GET | 高德地点搜索 |
| `/v1/routes` | POST | 路线规划 |
| `/v1/route-options` | POST | 路线选项 |
| `/v1/map/cities` | GET | 离线地图城市查询 |
| `/v1/map/tiles/{z}/{x}/{y}` | GET | 周边地图瓦片 |

所有地址均相对于 App 配置的网关根地址。若设置 `MOTO_BASE_PATH`，App 的根地址必须包含相同前缀。

当前 Worker 按来源 IP 限流：路线/路线选项每分钟 30 次、地点搜索每分钟 60 次、城市查询每分钟 30 次、地图瓦片每分钟 600 次。该限流用于控制调用量，不识别 App 用户身份。

## 2. 首次部署或更新 Worker

需要 Node.js 22+、npm、Cloudflare 账户访问权限。项目通过 `package-lock.json` 固定 Wrangler 4.142.0，命令应从 Worker 目录执行：

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
npx wrangler whoami
npm test
```

Worker 运行模式由 `MOTO_PROVIDER` 控制：生产配置为 `amap`；本地 `.dev.vars.example` 使用不调用高德的 `fixture`；Node 示例默认使用 `disabled`。

`whoami` 会确认 Wrangler 当前使用的 Cloudflare 身份。若尚未授权，运行 `npx wrangler login` 完成浏览器登录后再执行 `whoami`。确认身份和账户无误后，在终端交互提示中录入 AMap Web 服务 Key：

```sh
npx wrangler secret put AMAP_WEB_SERVICE_KEY --config wrangler.jsonc
```

只有首次部署或轮换 Key 时才执行 `secret put`；常规代码发布无需重复覆盖已有 Secret。不要把 Key 放进命令参数、环境变量输出、源码、`.dev.vars` 示例、日志或聊天。`wrangler secret put` 是线上变更，会立即创建并部署包含新 Secret 的 Worker 版本。代码变更发布使用：

```sh
npm run deploy
```

部署前核对命令所在目录、Wrangler 当前身份和目标 Worker。配置文件表示期望状态；它不证明线上 Key、R2 桶或 Cron 实际已配置。

## 3. Cloudflare 资源核对

部署后在 Cloudflare Dashboard → **Workers & Pages** → `moto-gps-gateway` 检查：

1. **Deployments / Versions**：最新版本已成功部署。
2. **Settings / Bindings**：R2 binding `MAP_CACHE` 指向 `moto-gps-map-cache`。
3. **Settings / Variables and Secrets**：有 `AMAP_WEB_SERVICE_KEY` Secret。只确认名称，不导出或复制值。
4. **Settings / Triggers**：Cron 是 UTC `17 3 * * *`。
5. **Observability / Logs**：发生请求时检查 Worker 的错误和限流记录。

地图桶保持私有。若使用对象生命周期规则，按原有部署约定令 `tiles/` 前缀对象 30 天后过期；不要开放整个桶的公共读取。

Worker 对搜索、路线、地图城市和瓦片分别配置 IP 限流；这不构成用户身份认证。当前 Worker API 没有 App 用户登录或访问令牌，知道 URL 的客户端都能请求公开接口。若需要限制调用者，应另行设计认证和密钥轮换。

## 4. 健康检查与真实服务验证

先检查 Worker，不会调用高德：

```sh
MOTO_GPS_GATEWAY_URL="https://your-worker.example.workers.dev"
curl -i "${MOTO_GPS_GATEWAY_URL%/}/healthz"
```

Worker 没有为 `/` 根路径提供网页或 API。打开 Worker 根路径返回 JSON `NOT_FOUND` 属于预期行为，只能说明该浏览器连到了 Worker；健康检查必须访问 `/healthz`。

成功响应应为 HTTP 200，JSON 至少包含 `status: "ok"` 和 `ready_for_live_navigation: true`。`ready_for_live_navigation` 表示 Worker 配置为 `MOTO_PROVIDER=amap`；该标志不是对高德 Key 的实时验证。

App 的“测试连接”也只请求网关根地址下的 `/healthz`。它不会发送地点搜索，不会验证 Key、AMap 配额或路线服务。若 iPhone 的测试按钮失败，但从其他网络访问健康检查成功，应先核对 App 显示的根地址和 iPhone 当前网络；原生 iOS 请求不受 CORS 规则限制。

若 iPhone Safari 也打不开同一条 `/healthz`，这表示请求没有成功得到健康响应。若手机能访问其他 Cloudflare Worker 的自有域名，但不能访问这里的 `workers.dev` 地址，重点检查域名入口差异，而非手机整体网络；可为 Moto GPS Worker 绑定自有域名，再先从 iPhone Safari 验证新域名的 `/healthz`，然后把 App 根地址切换过去。不要因此修改 AMap IP 白名单。

验证实际 AMap 时，从 App 执行一次地点搜索和一次路线规划，随后检查 Worker 日志和返回结果。单次功能验证即可，不要对高德服务做压力测试。错误码可按[高德 Web 服务错误码文档](https://lbs.amap.com/api/webservice/guide/tools/info)排查，例如：

| 高德错误码 | 常见含义 | 处理方向 |
| --- | --- | --- |
| `10001` | Key 错误、失效或过期 | 确认使用 Web 服务 Key，重录正确 Secret |
| `10002` | Key 无此服务权限或接口地址错误 | 检查 Key 绑定平台和接口权限 |
| `10003` / `10004` | 日调用量或单位时间调用超限 | 检查 Key 配额并降低请求频率 |
| `10005` | 请求服务器出口 IP 不在 IP 白名单 | 检查 Key 白名单和服务端出口地址 |
| `10009` | Key 绑定平台与请求 API 不匹配 | Worker 调用 Web 服务时应使用 Web 服务类型 Key |
| `10010` | 单个 IP 请求超限，可能触发封停 | 按高德控制台提示处理或联系高德支持 |

### AMap Key 的 IP 白名单

高德 Web 服务 Key 的可选 IP 白名单匹配的是**调用高德的服务器出口 IP**，不是用户 iPhone 的 IP。当前 Key 未配置白名单，所以不会由白名单拒绝请求。只有启用白名单后，来源 IP 不匹配才会出现高德 `10005 INVALID_USER_IP`。[高德官方说明](https://lbs.amap.com/faq/webservice/webservice-api/basic-configuration/43238)

本项目的 Cloudflare Worker 使用 `fetch()` 调用高德，`wrangler.jsonc` 没配置固定出口或专用 egress。不要把家中宽带、开发 Mac 或 iPhone 的 IP 填入 Key 白名单。若之后需要启用白名单，先为 Worker 配置并验证稳定出口方案，再在高德控制台设置；Cloudflare 对 Worker `fetch` 与 Dedicated CDN Egress IP 的适用条件见[官方说明](https://developers.cloudflare.com/smart-shield/configuration/dedicated-egress-ips/other-products/)。

`WEB_ORIGIN` 配置的是浏览器 CORS 响应来源，不是高德 Key 白名单。当前原生 iOS App 直接通过 HTTPS `URLSession` 调用 Worker；普通 iOS 网络请求不执行浏览器 CORS 校验。

## 5. 本地开发

### Node.js HTTP 服务

```sh
cd services/moto-gps-gateway
npm ci
cp .env.example .env
```

默认 `.env.example` 使用 `MOTO_PROVIDER=disabled`，不会请求高德。需要本地联调时，在被 Git 忽略的 `.env` 中设置 `MOTO_PROVIDER=amap`，并通过安全的本机方式填写 `AMAP_WEB_SERVICE_KEY`，然后显式加载该文件：

```sh
node --env-file=.env src/server.js
```

本地网关默认监听 `http://127.0.0.1:8787`。iPhone 无法访问 Mac 的 loopback 地址；若要让真机联调本地 Node 服务，需要另外配置 Mac 局域网监听、HTTPS 和网络访问策略。优先使用已部署的 HTTPS Worker。

### Wrangler 本地模拟

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
cp .dev.vars.example .dev.vars
npm run dev
```

示例 `.dev.vars` 使用 `fixture` Provider，不访问高德；需要实时 AMap 时才在本地忽略文件中改为 `MOTO_PROVIDER=amap` 并安全提供 Key。Node 的 `.env` 与 Wrangler 的 `.dev.vars` 是两个独立配置文件。

## 6. App 地址配置

仓库和 App 安装包不包含个人 Worker 地址。用户首次使用时，在 App 首页齿轮 → **网关设置**填写自己的 HTTPS 网关根地址；之后可在此处更换，不需要重新安装 App。App 配置细节见[手机 App 配置指南](../mobile-app/configuration.md)。
