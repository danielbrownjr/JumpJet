#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
allowlist="$root/tools/production-actuation-allowlist.txt"

[ -f "$allowlist" ] || { echo "missing production actuation allowlist" >&2; exit 1; }
command -v rg >/dev/null 2>&1 || { echo "actuation check requires rg" >&2; exit 1; }

# These are direct output-changing primitives used by ESP-IDF, Arduino, and
# common ESP32 PWM/MCPWM paths. Any production use requires an exact-file entry.
matches=$(rg -n --no-heading \
    '(^|[^A-Za-z0-9_])(gpio_set_level|ledcWrite|ledc_set_duty|ledc_update_duty|ledc_stop|mcpwm_comparator_set_compare_value|mcpwm_generator_set_action[^[:space:](]*)[[:space:]]*\(' \
    "$root/main" "$root/components" \
    -g '*.c' -g '*.cc' -g '*.cpp' -g '*.ino' || true)

files=$(printf '%s\n' "$matches" | sed '/^$/d; s/:.*//' | sort -u)
for file in $files; do
    relative=${file#"$root/"}
    if ! grep -Fxq "$relative" "$allowlist"; then
        echo "production actuation outside allowlist: $relative" >&2
        printf '%s\n' "$matches" | grep -F "$file:" >&2 || true
        exit 1
    fi
done

# Preserve one auditable heater-power boundary when that phase arrives.
heater_entries=$(grep -Ev '^[[:space:]]*(#|$)' "$allowlist" | grep -Eic 'heater' || true)
if [ "$heater_entries" -gt 1 ]; then
    echo "heater actuation must remain confined to one allowlisted file" >&2
    exit 1
fi

echo "production actuation allowlist check: PASS"
