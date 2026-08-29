#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
portal="$root/components/jj_portal/jj_portal.c"

grep -q 'guard_operation = guard_operation' "$portal"
grep -q 'validate_image = validate_image' "$portal"
grep -q 'esp_err_t err = heater_off_guard(message, message_size);' "$portal"
grep -q 'strcmp(image->project_name, JJ_IDENTITY_PRODUCT_ID)' "$portal"
grep -q '"commissioned", false' "$portal"
grep -q '"heater_available", false' "$portal"

echo "OTA/status product contract check: PASS"
