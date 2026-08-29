#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
identity="$root/components/jj_identity/identity.cmake"
product_id=$(sed -n 's/^set(JJ_IDENTITY_PRODUCT_ID "\([^"]*\)")$/\1/p' "$identity")

[ -n "$product_id" ] || { echo "identity source is missing/empty" >&2; exit 1; }
[ "${#product_id}" -le 31 ] || { echo "identity exceeds ESP-IDF project_name" >&2; exit 1; }

cmake -DTEST_PRODUCT_ID="$product_id" -P "$root/tests/identity_validation.cmake" >/dev/null
if cmake -P "$root/tests/identity_validation.cmake" >/dev/null 2>&1; then
    echo "undefined identity unexpectedly passed validation" >&2
    exit 1
fi
if cmake -DTEST_PRODUCT_ID= -P "$root/tests/identity_validation.cmake" >/dev/null 2>&1; then
    echo "empty identity unexpectedly passed validation" >&2
    exit 1
fi
too_long=abcdefghijklmnopqrstuvwxyz123456
if cmake -DTEST_PRODUCT_ID="$too_long" -P "$root/tests/identity_validation.cmake" >/dev/null 2>&1; then
    echo "overlength identity unexpectedly passed validation" >&2
    exit 1
fi

# Runtime code must consume the generated identity macro rather than repeat the
# canonical literal. The identity source itself is the single permitted match.
duplicates=$(rg -l --fixed-strings "\"$product_id\"" \
    "$root/CMakeLists.txt" "$root/main" "$root/components" \
    -g '*.c' -g '*.h' -g '*.in' -g '*.cmake' | tr '\\' '/' | \
    grep -Ev '/components/jj_identity/identity\.cmake$' || true)
if [ -n "$duplicates" ]; then
    echo "duplicate runtime product identity literal:" >&2
    printf '%s\n' "$duplicates" >&2
    exit 1
fi

grep -q 'project(${JJ_IDENTITY_PRODUCT_ID})' "$root/CMakeLists.txt"
grep -q 'strcmp(image->project_name, JJ_IDENTITY_PRODUCT_ID)' \
    "$root/components/jj_portal/jj_portal.c"

echo "identity contract check: PASS"
