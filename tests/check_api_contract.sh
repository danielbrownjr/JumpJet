#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
portal="$root/components/jj_portal/jj_portal.c"
app="$root/main/app_main.c"
header="$root/components/jj_interlock/include/jj_interlock.h"

for token in '"mode"' '"controller"' '"control"' '"target_c"' \
             '"requested"' '"allowed"' '"delivered"' \
             '"dominant_constraint"' '"health"'; do
    grep -q "$token" "$portal"
done

# Diagnostics must preserve the semantic order in the product contract.
previous=0
for token in '"mode"' '"controller"' '"control"' '"target_c"' \
             '"requested"' '"allowed"' '"delivered"' \
             '"dominant_constraint"' '"health"'; do
    line=$(grep -n -m1 "$token" "$portal" | cut -d: -f1)
    [ "$line" -ge "$previous" ]
    previous=$line
done

grep -q '"available", false' "$portal"
grep -q '"delivered_percent", 0' "$portal"
grep -q 'JJ_MANUAL_TARGET_MIN_C     30.0f' "$header"
grep -q 'JJ_MANUAL_TARGET_DEFAULT_C 45.0f' "$header"
grep -q 'JJ_MANUAL_TARGET_MAX_C     50.0f' "$header"
grep -q 'strcmp(state, "PRINTING") == 0' "$app"

for route in '/api/v2/control/acquire' '/api/v2/control/refresh' \
             '/api/v2/control/heartbeat' '/api/v2/control/mutate'; do
    grep -q "$route" "$portal"
done
for error in 'control_authority_required' 'control_authority_lost' \
             'control_generation_stale' 'state_revision_conflict' \
             'control_request_ineligible' 'hardware_fault_latched'; do
    grep -q "$error" "$root/components/jj_authority/jj_authority.c"
done
grep -q 'application/json' "$portal"
grep -q 'authority->active_authority == JJ_AUTHORITY_REMOTE' "$portal"
grep -q 'authority->active_authority == JJ_AUTHORITY_AUTOMATIC' "$portal"

if rg -n 'power_on|heater_control|fan_control|"heating"|"fan"' "$portal" \
    | grep 'cJSON_CreateString' >/dev/null; then
    echo "unavailable physical capability is advertised" >&2
    exit 1
fi

echo "API/product contract check: PASS"
