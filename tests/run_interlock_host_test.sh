#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp)
trap 'rm -f "$out"' EXIT
cc -std=c11 -Wall -Wextra -Werror \
  -I"$root/components/jj_interlock/include" \
  "$root/components/jj_interlock/jj_interlock.c" \
  "$root/tests/jj_interlock_host_test.c" -lm -o "$out"
"$out"
