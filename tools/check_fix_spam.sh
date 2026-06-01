#!/usr/bin/env bash
# tools/check_fix_spam.sh
# Reads commit-msg on stdin. Rejects if the last 3 commits are all "fix:" —
# this forces the author to pause and squash or rethink before adding
# another band-aid commit.

set -euo pipefail

msg=$(cat)
subject=$(printf '%s' "$msg" | head -n1)
self_type=$(printf '%s' "$subject" | sed -nE 's/^([a-z]+)(\([^)]+\))?!?:.*/\1/p')

# Only enforce for fix: (refactor:, chore:, style: also trigger but are rarer)
if [ "$self_type" != "fix" ]; then
    exit 0
fi

recent=$(git log -3 --pretty=%s 2>/dev/null || true)
if [ -z "$recent" ]; then
    exit 0
fi

# Count "fix:" subjects in the last 3 (which now include the incoming one,
# because commit-msg runs before the new commit is recorded)
count=1  # the incoming one
while IFS= read -r s; do
    case "$s" in
        fix:*|fix\(*) count=$((count + 1)) ;;
    esac
done <<EOF
$recent
EOF

if [ "$count" -ge 3 ]; then
    echo "fix-spam: last 3 commits are all 'fix:'. Squash or re-think."
    echo "Either: git rebase -i HEAD~N  to squash, or write a 'refactor:'"
    echo "commit that addresses the root cause instead of band-aiding."
    exit 1
fi
