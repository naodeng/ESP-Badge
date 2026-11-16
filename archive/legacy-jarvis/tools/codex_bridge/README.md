# Codex Island Bridge

Mac-side Codex data collector and Nordic UART Service BLE bridge. It reads account data from `~/.codex/auth.json` and incrementally scans local `rollout-*.jsonl` session files. Credentials and conversation text are not included in cache files or outgoing snapshots.

The provider emits an Activity snapshot with `idle`, `running`, `waiting`, `review`, or `failed` state plus an active-task count. Activity is lifecycle status inferred from session events; it does not inspect conversation content.

## Install and inspect

```bash
cd Jarvis_OS/tools/codex_bridge
python3 -m venv .venv
.venv/bin/pip install -e .
.venv/bin/codex-island-bridge print
.venv/bin/codex-island-bridge activity-status
.venv/bin/codex-island-bridge devices
.venv/bin/codex-island-bridge distributed-test
.venv/bin/codex-island-bridge once
```

The `activity-status` command prints the same compact state used by the BLE transport. `CODEX_ACTIVITY_INTERVAL` controls the sampling interval; it defaults to two seconds. The BLE message kind is `activity` and carries the state and active-task count.

## Badge HTTP bridge

The firmware Codex app uses `../codex_bridge_http.py`, started by `../run_codex_bridge.sh`, to serve Usage, Activity, and Radar over local WiFi. This keeps the Badge's Codex app independent of a second BLE data connection. The former Sand image import endpoint has been removed with the Sand app.

## Data and Radar

Usage windows follow the API's reported duration. If the account does not expose a five-hour window, the bridge sends `p5: null` instead of relabeling a weekly window. Today cost is an estimate based on the public model price table; unknown model slugs are omitted from the estimate.

Radar can use an authorized JSON endpoint with `CODEX_RADAR_API_URL` and optionally `CODEX_RADAR_API_TOKEN`. The public-page parser must be enabled explicitly with `CODEX_RADAR_ALLOW_HTML=1`; it is rate-limited and normally refreshes hourly. `CODEX_RADAR_PRIMARY_KEY` selects the trend model, with highest-IQ fallback if it disappears.

Distributed Radar reads its public table every five minutes, preserves source order, and marks cached data stale after 30 minutes without a successful refresh. Set `CODEX_DISTRIBUTED_RADAR_URL` to override its endpoint.

## Protocol migration

The Activity rename changes the local BLE message kind from `pet` to `activity` and the CLI command from `pet-status` to `activity-status`. There is no compatibility alias; update the bridge and receiving firmware together.
