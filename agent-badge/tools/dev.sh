#!/usr/bin/env bash
# Workspace-local ESP-IDF wrapper for this project.
#   ./tools/dev.sh build
#   ./tools/dev.sh flash [/dev/cu.usbmodem101]
#   ./tools/dev.sh monitor [/dev/cu.usbmodem101]
#   ./tools/dev.sh menuconfig | clean
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TC="$ROOT/../.toolchain"

export PATH="$TC/bin:$PATH"
export IDF_TOOLS_PATH="$TC/espressif"
export IDF_COMPONENT_CACHE_PATH="$TC/idf-component-cache"
# shellcheck disable=SC1090
. "$TC/esp-idf/export.sh" >/dev/null

cd "$ROOT"
cmd="${1:-build}"; shift || true
case "$cmd" in
  build)      idf.py build ;;
  flash)      idf.py -p "${1:-/dev/cu.usbmodem101}" flash ;;
  monitor)    idf.py -p "${1:-/dev/cu.usbmodem101}" monitor ;;
  menuconfig) idf.py menuconfig ;;
  clean)      idf.py fullclean ;;
  *) echo "usage: $0 {build|flash|monitor|menuconfig|clean} [port]" >&2; exit 2 ;;
esac
