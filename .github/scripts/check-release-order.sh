#!/usr/bin/env bash
# Checks that a new release will be listed above every other release on GitHub's Releases page.
# GitHub orders releases by the UTC day of the commit their tag points to (newest day first), and releases of the
# same day by tag name, in descending byte order: the time of day doesn't count. So the new tag's commit must be of
# today (or of the newest day already listed) and, on the same day, the tag name must sort after the others':
# opti-venom-v0.3-beta2 after opti-venom-v0.3-beta1, the stable opti-venom-v0.3.0 after both ('.' > '-'),
# a nightly (opti-venom-nightly-...) never after a release of its day.
# Usage: check-release-order.sh <repo> <tag> <commit date, ISO 8601>
# Exit 0: it will be on top. Exit 1: it won't, the reason is printed as a GitHub annotation of level ORDER_LEVEL
# (default error).
set -euo pipefail
level=${ORDER_LEVEL:-error}
export LC_ALL=C

repo=$1
tag=$2
day=$(TZ=UTC date -d "$3" +%F)

# GitHub's key for a release: created_at is the date of its tag's commit
others=$(gh api "repos/$repo/releases?per_page=100" --paginate \
  --jq ".[] | select(.draft | not) | select(.tag_name != \"$tag\") | \"\(.created_at[0:10]) \(.tag_name)\"")

ok=true
while read -r otherDay otherTag; do
  [ -n "${otherDay:-}" ] || continue

  if [[ "$otherDay" > "$day" ]]; then
    echo "::$level::$tag (commit of $day UTC) would be listed below $otherTag (commit of $otherDay UTC): tag a commit made now"
    ok=false
  elif [[ "$otherDay" == "$day" && ! "$tag" > "$otherTag" ]]; then
    echo "::$level::$tag would be listed below $otherTag: on the same UTC day GitHub orders releases by tag name. Use a tag that sorts after it (stable: opti-venom-vX.Y.0, betas: opti-venom-vX.Y-betaN)"
    ok=false
  fi
done <<< "$others"

$ok && echo "$tag will be listed first (day $day UTC)"
$ok
