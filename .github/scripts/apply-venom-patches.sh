#!/usr/bin/env bash
# Applies the Venom patches (the commits of <range>) on the checked out upstream commit, the same way for the nightly
# and for the hourly look-ahead on upstream master.
# Conflicts already solved once are solved again from .github/rerere (git rerere's cache, recorded with
# Tools\dev\export-rerere-cache.ps1 on the repo where the conflict was solved), so a conflict needs a hand only the
# first time, and the patch branch doesn't have to be rebased (rebasing it would drop the latest release's commit
# the nightly checks for).
# Conflicts rerere doesn't know go to auto-resolve.py, which solves the ones where a side only added lines; the rest
# stops the cherry-pick, with the conflicts left for a human in <out dir>/conflicts.diff.
# Usage, inside the upstream clone: apply-venom-patches.sh <range> <rerere dir> <out dir>
# Writes <out dir>/pick.log, <out dir>/resolved.txt (commits solved from the cache or automatically) and, on a
# conflict, <out dir>/conflicts.txt and conflicts.diff. Exit 0 = applied, 2 = conflict (cherry-pick aborted), anything
# else = error.
set -uo pipefail

range=$1
cache=$2
out=$3
here=$(cd "$(dirname "$0")" && pwd)
# The first Python that runs: on Windows python3 can be the Store's stub
python=
for c in python3 python py; do
  "$c" -c "" >/dev/null 2>&1 && { python=$c; break; }
done

# rerere tells conflicts apart by their text, line endings included: conflicted files are written with CRLF as on
# Windows, where the resolutions are recorded, also on the Linux runner
git config core.autocrlf false
git config core.eol crlf
git config rerere.enabled true
git config rerere.autoUpdate true
gitdir=$(git rev-parse --git-common-dir)
mkdir -p "$gitdir/rr-cache"
[ -d "$cache" ] && cp -r "$cache"/. "$gitdir/rr-cache/"

: > "$out/resolved.txt"
: > "$out/pick.log"
rm -f "$out/conflicts.txt" "$out/conflicts.diff" "$out/stopped.txt"

if git cherry-pick "$range" >> "$out/pick.log" 2>&1; then
  exit 0
fi

# Every stop leaves one commit to go on with: at most one stop per commit of the range
for _ in $(seq "$(git rev-list --count "$range")"); do
  git rev-parse -q --verify CHERRY_PICK_HEAD >/dev/null || break
  commit=$(git log -1 --format="%h %s" CHERRY_PICK_HEAD)
  mapfile -t left < <(git diff --name-only --diff-filter=U)
  if [ ${#left[@]} -gt 0 ]; then
    # Not in the cache: written again with the base between the sides, for the automatic rules
    git checkout --conflict=diff3 -- "${left[@]}"
    [ -n "$python" ] && "$python" "$here/auto-resolve.py" "${left[@]}" >> "$out/pick.log" 2>&1 || break
    git add -- "${left[@]}"
    echo "$commit (automatically: ${left[*]})" >> "$out/resolved.txt"
  else
    echo "$commit" >> "$out/resolved.txt"
  fi
  if GIT_EDITOR=true git cherry-pick --continue >> "$out/pick.log" 2>&1; then
    exit 0
  fi
done

git diff --name-only --diff-filter=U > "$out/conflicts.txt"
git diff > "$out/conflicts.diff" 2>/dev/null || true
[ -s "$out/conflicts.txt" ] || echo "(no conflicting file: see the log)" > "$out/conflicts.txt"
git log -1 --format="%h %s" CHERRY_PICK_HEAD > "$out/stopped.txt" 2>/dev/null || true
git cherry-pick --abort >/dev/null 2>&1 || true
exit 2
