#!/usr/bin/env bash
# Create the workspace-local venv and install the vendored Codex Island bridge.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TC="$ROOT/../.toolchain"
python3 -m venv "$TC/venv-bridge" 2>/dev/null || true
"$TC/venv-bridge/bin/pip" install -q -e "$ROOT/tools/codex_bridge"
echo "Codex bridge venv ready: $TC/venv-bridge"
