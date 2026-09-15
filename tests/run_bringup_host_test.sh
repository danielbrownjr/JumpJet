#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
core=${DRAGON_CORE_PATH:?Set DRAGON_CORE_PATH to the pinned dragon-core checkout}
idf=${IDF_PATH:?Set IDF_PATH to the ESP-IDF v5.3 checkout used by firmware CI}
pin=$(sed -n 's/.*version: \([0-9a-f]\{40\}\)/\1/p' "$root/main/idf_component.yml" | sort -u)
[[ $(git -C "$core" rev-parse HEAD) == "$pin" ]] || { echo 'dragon-core test/firmware pin mismatch' >&2; exit 1; }
[[ -z $(git -C "$core" status --porcelain -- components/dc_prusa components/dc_wifi) ]] || { echo 'modified test dependency' >&2; exit 1; }
[[ $(git -C "$idf" rev-parse HEAD) == e0991facf5ecb362af6aac1fae972139eb38d2e4 ]] || { echo 'ESP-IDF test/CI v5.3 mismatch' >&2; exit 1; }
json="$idf/components/json/cJSON"
expected_json=$(git -C "$idf" ls-tree HEAD components/json/cJSON | awk '{print $3}')
[[ $(git -C "$json" rev-parse HEAD) == "$expected_json" ]] || { echo 'cJSON test/IDF pin mismatch' >&2; exit 1; }
[[ -z $(git -C "$json" status --porcelain) ]] || { echo 'modified cJSON test dependency' >&2; exit 1; }
out=$(mktemp)
trap 'rm -f "$out"' EXIT
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined \
  -I"$root/tests/stubs" -I"$core/components/dc_prusa/include" -I"$json" \
  -I"$root/components/jj_board/include" -I"$root/components/jj_bringup/include" \
  -I"$root/components/jj_portal/include" \
  "$root/components/jj_board/jj_board.c" "$root/components/jj_bringup/jj_bringup_json.c" \
  "$root/components/jj_portal/jj_config.c" "$root/tests/jj_bringup_host_test.c" \
  "$json/cJSON.c" -lm -o "$out"
"$out"
for config in disabled enabled_no_boot_init enabled_boot_init; do
  defs=()
  case "$config" in
    disabled) defs=(-DCONFIG_SPIRAM=0 -DCONFIG_SPIRAM_BOOT_INIT=0);;
    enabled_no_boot_init) defs=(-DCONFIG_SPIRAM=1 -DCONFIG_SPIRAM_BOOT_INIT=0);;
    enabled_boot_init) defs=(-DCONFIG_SPIRAM=1 -DCONFIG_SPIRAM_BOOT_INIT=1);;
  esac
  cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined \
    "${defs[@]}" -I"$root/tests/stubs/bringup" -I"$root/tests/stubs" \
    -I"$core/components/dc_wifi/include" -I"$json" \
    -I"$root/components/jj_board/include" -I"$root/components/jj_bringup/include" \
    "$root/components/jj_board/jj_board.c" "$root/components/jj_bringup/jj_bringup_json.c" \
    "$root/components/jj_bringup/jj_bringup.c" "$root/tests/jj_bringup_adapter_host_test.c" \
    "$json/cJSON.c" -lm -o "$out"
  printf '%s: ' "$config"
  "$out"
done
