#!/usr/bin/env bash
# Run the HTTP bridge on port 8790. The badge fetches /codex.json over WiFi.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TC="$ROOT/../.toolchain"
REAL_HOME="$HOME"

if [ ! -x "$TC/venv-bridge/bin/python" ]; then
  "$ROOT/tools/setup_codex_bridge.sh"
fi

# platformdirs insists on ~/Library/Application Support; keep its writes inside
# the repo. Codex auth/sessions still point at the real home.
export HOME="$TC/bridge-home"
mkdir -p "$HOME" "$TC/codex-bridge-data"
export CODEX_ISLAND_DATA_DIR="$TC/codex-bridge-data"
export CODEX_AUTH_PATH="${CODEX_AUTH_PATH:-$REAL_HOME/.codex/auth.json}"
export CODEX_SESSIONS_DIR="${CODEX_SESSIONS_DIR:-$REAL_HOME/.codex/sessions}"
exec "$TC/venv-bridge/bin/python" "$ROOT/tools/codex_bridge_http.py"
