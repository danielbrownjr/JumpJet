#!/usr/bin/env bash
set -euo pipefail
project=$(cd "${1:-.}" && pwd)
target=${2:-esp32s3}
build_dir=${3:-build}
idf_root=${IDF_PATH:-"${HOME}/esp/esp-idf"}
if [[ ! -f "$idf_root/export.sh" ]]; then
  echo "error: ESP-IDF export.sh not found at $idf_root/export.sh" >&2
  exit 2
fi
# shellcheck disable=SC1090
source "$idf_root/export.sh" >/dev/null
idf.py -C "$project" -B "$project/$build_dir" -D "IDF_TARGET=$target" build
