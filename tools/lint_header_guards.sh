#!/usr/bin/env bash
# tools/lint_header_guards.sh
# Verifies every header has *some* include-guard mechanism.
#
# Accepts either modern `#pragma once` (preferred in this repo) or the
# traditional `#ifndef FOO_H ... #define FOO_H` pattern. We don't enforce
# #pragma once vs #ifndef — that's a style choice — but we *do* require
# that a guard exists so we don't ship TUs with double-definition risk.

set -euo pipefail

fail=0
checked=0
pragmas=0
ifndef=0
other=0

while IFS= read -r -d '' hdr; do
    checked=$((checked + 1))
    if grep -qE '^[[:space:]]*#pragma[[:space:]]+once\b' "$hdr"; then
        pragmas=$((pragmas + 1))
        continue
    fi
    if grep -qE '^[[:space:]]*#ifndef[[:space:]]+[A-Z0-9_]+' "$hdr" \
       && grep -qE '^[[:space:]]*#define[[:space:]]+[A-Z0-9_]+' "$hdr"; then
        ifndef=$((ifndef + 1))
        continue
    fi
    echo "MISSING guard: $hdr  (need either '#pragma once' or '#ifndef FOO_H' / '#define FOO_H')"
    fail=$((fail + 1))
done < <(find src tests -type f \( -name '*.h' -o -name '*.hpp' \) -print0 2>/dev/null)

if [ "$checked" -eq 0 ]; then
    echo "lint_header_guards: no headers found, nothing to check"
    exit 0
fi

if [ "$fail" -gt 0 ]; then
    echo "lint_header_guards: $fail of $checked headers failed  (pragma-once=$pragmas  ifndef=$ifndef)"
    exit 1
fi
echo "lint_header_guards: $checked headers OK  (pragma-once=$pragmas  ifndef=$ifndef)"
