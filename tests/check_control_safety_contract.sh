#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
app="$root/main/app_main.c"
authority="$root/components/jj_authority/jj_authority.c"
portal="$root/components/jj_portal/jj_portal.c"
interlock="$root/components/jj_interlock/jj_interlock.c"

prusa_line=$(grep -n -m1 'dc_prusa_get_status(&printer)' "$app" | cut -d: -f1)
control_line=$(grep -n -m1 'jj_authority_control_step(' "$app" | cut -d: -f1)
[ "$prusa_line" -lt "$control_line" ]

if grep -q 'jj_authority_snapshot' "$app"; then
    echo "control task must not snapshot authority before cached inputs" >&2
    exit 1
fi
if grep -q 'jj_interlock_step' "$app" "$portal"; then
    echo "ordinary production interlock evaluation must remain control-task owned" >&2
    exit 1
fi

if rg -n '(ESP_LOG[A-Z]*|\b(printf|snprintf|vfprintf|malloc|calloc|realloc|free|xSemaphoreTake|vTaskDelay)[[:space:]]*\(|esp_http)' \
    "$authority" >/dev/null; then
    echo "authority critical-section unit contains forbidden unbounded work" >&2
    exit 1
fi

grep -q 'Lock ordering is AUTHORITY_LOCK -> jj_interlock' "$authority"
grep -q 'return JJ_REMOTE_ACK_WHEN_HEALTHY' "$interlock"
grep -q 'return JJ_REMOTE_ACK_AFTER_REVALIDATION' "$interlock"
grep -q 'return JJ_REMOTE_ACK_NEVER' "$interlock"

echo "control ordering/locking/fault-policy contract check: PASS"
