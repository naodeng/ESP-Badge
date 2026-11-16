# Cloudflare Worker deployment

This directory is the Moto GPS gateway's Cloudflare target, using Wrangler, Workers, and a private R2 cache. The current deployment runbook for the Jarvis_OS monorepo is the root [deployment guide](../../../docs/deployment/README.md).

## Configuration

- Worker: `moto-gps-gateway`.
- R2 binding: `MAP_CACHE` → private bucket `moto-gps-map-cache`.
- AMap provider: enabled by `MOTO_PROVIDER=amap`.
- Secret: `AMAP_WEB_SERVICE_KEY`; enter it interactively through Wrangler.
- Cron: `17 3 * * *` UTC, for map-source metadata refresh.
- User authentication is not configured; CORS and rate limits are not authentication.

## Commands

Run from the repository root:

```sh
cd services/moto-gps-gateway/cloudflare
npm ci
npm test
npm run build
npx wrangler whoami
npx wrangler secret put AMAP_WEB_SERVICE_KEY
npm run deploy
```

Local Secrets may be stored in the ignored `.dev.vars`. Never commit them. Before deployment, confirm the Cloudflare account, R2 bucket, and production Secret ownership. The config describes desired state; after deployment, check the Worker version, R2 binding, Secret name, and Cron trigger in Cloudflare.

See the root [deployment guide](../../../docs/deployment/README.md) for the endpoint, iOS default URL, health check, and API validation.
