#!/usr/bin/env bash
# Run the HTTP bridge on port 8790. The badge fetches /codex.json over WiFi.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TC="$(cd "$ROOT/../.toolchain" && pwd)"
REAL_HOME="$HOME"
BRIDGE_PYTHON="$TC/venv-bridge/bin/python"
BRIDGE_SOURCE="$ROOT/tools/codex_bridge/src"

if [ ! -x "$BRIDGE_PYTHON" ] \
    || ! PYTHONPATH="$BRIDGE_SOURCE${PYTHONPATH:+:$PYTHONPATH}" "$BRIDGE_PYTHON" -c 'import sys,codex_island_bridge; raise SystemExit(sys.version_info < (3,11))' 2>/dev/null; then
  "$ROOT/tools/setup_codex_bridge.sh"
fi

# platformdirs insists on ~/Library/Application Support; keep its writes inside
# the repo. Codex auth/sessions still point at the real home.
export HOME="$TC/bridge-home"
mkdir -p "$HOME" "$TC/codex-bridge-data"
export CODEX_ISLAND_DATA_DIR="$TC/codex-bridge-data"
export CODEX_AUTH_PATH="${CODEX_AUTH_PATH:-$REAL_HOME/.codex/auth.json}"
export CODEX_SESSIONS_DIR="${CODEX_SESSIONS_DIR:-$REAL_HOME/.codex/sessions}"
export PYTHONPATH="$BRIDGE_SOURCE${PYTHONPATH:+:$PYTHONPATH}"
exec "$BRIDGE_PYTHON" "$ROOT/tools/codex_bridge_http.py"
