#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp)
trap 'rm -f "$out"' EXIT
cc -std=c11 -Wall -Wextra -Werror \
  -fsanitize=undefined -fno-sanitize-recover=undefined \
  -I"$root/components/jj_interlock/include" \
  -I"$root/components/jj_authority/include" \
  "$root/components/jj_interlock/jj_interlock.c" \
  "$root/components/jj_authority/jj_authority.c" \
  "$root/tests/jj_authority_host_test.c" -lm -o "$out"
"$out"
