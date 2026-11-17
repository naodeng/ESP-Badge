#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
toolchain_root="$repo_root"
if [[ ! -f "$toolchain_root/.toolchain/esp-idf/export.sh" ]]; then
  common_git_dir="$(git -C "$repo_root" rev-parse --path-format=absolute --git-common-dir)"
  common_repo_root="$(dirname "$common_git_dir")"
  if [[ -f "$common_repo_root/.toolchain/esp-idf/export.sh" ]]; then
    toolchain_root="$common_repo_root"
  fi
fi

if [[ -n "${IDF_EXPORT:-}" ]]; then
  idf_export="$IDF_EXPORT"
elif [[ -n "${IDF_PATH:-}" && -f "$IDF_PATH/export.sh" ]]; then
  idf_export="$IDF_PATH/export.sh"
else
  idf_export="$toolchain_root/.toolchain/esp-idf/export.sh"
fi
build_dir="/tmp/jarvis-os-screen-mirror-build"
sdkconfig_path="/tmp/jarvis-os-screen-mirror-sdkconfig"
defaults="$repo_root/firmware/jarvis_os/sdkconfig.defaults;$repo_root/firmware/jarvis_os/sdkconfig.mirror.defaults"

if [[ ! -f "$idf_export" ]]; then
  printf 'ESP-IDF export script not found: %s\n' "$idf_export" >&2
  exit 1
fi

export IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-$toolchain_root/.toolchain/espressif}"
export IDF_TARGET="${IDF_TARGET:-esp32s3}"
source "$idf_export"

idf.py -C "$repo_root/firmware/jarvis_os" \
  -B "$build_dir" \
  -D SDKCONFIG="$sdkconfig_path" \
  -D SDKCONFIG_DEFAULTS="$defaults" \
  build

printf '\nMirror firmware: %s/Jarvis_OS.bin\n' "$build_dir"
