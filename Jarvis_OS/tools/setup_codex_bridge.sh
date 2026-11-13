#!/usr/bin/env bash
# Create the workspace-local venv and install the vendored Codex Island bridge.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TC="$(cd "$ROOT/../.toolchain" && pwd)"
VENV="$TC/venv-bridge"

PYTHON_BIN="${CODEX_BRIDGE_PYTHON:-}"
if [ -z "$PYTHON_BIN" ]; then
  for candidate in python3.13 python3.12 python3.11 \
      /opt/homebrew/bin/python3.13 /opt/homebrew/bin/python3.12 \
      /opt/homebrew/bin/python3.11 /usr/local/bin/python3.13 \
      /usr/local/bin/python3.12 /usr/local/bin/python3.11 python3; do
    if command -v "$candidate" >/dev/null 2>&1 \
        && "$candidate" -c 'import sys; raise SystemExit(sys.version_info < (3, 11))' 2>/dev/null; then
      PYTHON_BIN="$candidate"
      break
    fi
  done
fi

if [ -z "$PYTHON_BIN" ] || ! "$PYTHON_BIN" -c 'import sys; raise SystemExit(sys.version_info < (3, 11))' 2>/dev/null; then
  echo "Codex bridge requires Python 3.11 or newer. Install Python 3.11+ or set CODEX_BRIDGE_PYTHON." >&2
  exit 1
fi

if [ -x "$VENV/bin/python" ] \
    && ! "$VENV/bin/python" -c 'import sys; raise SystemExit(sys.version_info < (3, 11))' 2>/dev/null; then
  rm -rf "$VENV"
fi

if [ ! -x "$VENV/bin/python" ]; then
  "$PYTHON_BIN" -m venv "$VENV"
fi
"$VENV/bin/python" -m pip install -q --upgrade pip
"$VENV/bin/python" -m pip install -q -e "$ROOT/tools/codex_bridge"
echo "Codex bridge venv ready: $VENV ($($VENV/bin/python --version 2>&1))"
