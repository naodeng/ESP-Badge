# Moto GPS Gateway

The gateway provides places, routes, administrative areas, and online map tiles for the MOTO GPS iOS app. The Node.js server and Cloudflare Worker share the same request logic.

## Local Node server

Requires Node.js 20+. Run from the repository root:

```sh
npm ci --prefix services/moto-gps-gateway
npm start --prefix services/moto-gps-gateway
```

Local development uses fixture/disabled settings by default. Put a real AMap key only in an ignored `.env` file or process environment; never put it in the app, firmware, or Git.

## Cloudflare Worker

Worker configuration, tests, and the R2 binding are under `cloudflare/`. See the root [deployment guide](../../docs/deployment/README.md) for production secrets, Cron, and checks.

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
npm test
npx wrangler login
npx wrangler secret put AMAP_WEB_SERVICE_KEY
npm run deploy
```

The Worker has no user authentication by default. CORS and rate limits do not authenticate callers. Confirm public access scope and Cloudflare limits before production use. Wrangler reads the Secret interactively; do not paste it into chat or commit it.

## API

| Path | Purpose |
|---|---|
| `GET /healthz` | Configuration capabilities and health |
| `GET /v1/places` | Place search |
| `POST /v1/route-options` | Route candidates |
| `POST /v1/routes` | Route, navigation refresh, and rerouting |
| `GET /v1/map/cities` | City/district search |
| `GET /v1/map/tiles/{z}/{x}/{y}` | Nearby roads/buildings |

Shared schemas, BLE protocol, and offline map resources live in `packages/moto-gps/shared/`. See the root [system design](../../design.md).
