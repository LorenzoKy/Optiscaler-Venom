#!/usr/bin/env bash
# Applies the Venom patches (the commits of <range>) on the checked out upstream commit, the same way for the nightly
# and for the hourly look-ahead on upstream master.
# Conflicts already solved once are solved again from .github/rerere (git rerere's cache, recorded with
# Tools\dev\export-rerere-cache.ps1 on the repo where the conflict was solved), so a conflict needs a hand only the
# first time, and the patch branch doesn't have to be rebased (rebasing it would drop the latest release's commit
# the nightly checks for).
# Usage, inside the upstream clone: apply-venom-patches.sh <range> <rerere dir> <out dir>
# Writes <out dir>/pick.log, <out dir>/resolved.txt (commits solved from the cache) and, on a conflict,
# <out dir>/conflicts.txt. Exit 0 = applied, 2 = conflict (cherry-pick aborted), anything else = error.
set -uo pipefail

range=$1
cache=$2
out=$3

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
rm -f "$out/conflicts.txt"

if git cherry-pick "$range" >> "$out/pick.log" 2>&1; then
  exit 0
fi

# Every stop leaves one commit to go on with: at most one stop per commit of the range
for _ in $(seq "$(git rev-list --count "$range")"); do
  if [ -n "$(git diff --name-only --diff-filter=U)" ] || ! git rev-parse -q --verify CHERRY_PICK_HEAD >/dev/null; then
    break
  fi
  git log -1 --format="%h %s" CHERRY_PICK_HEAD >> "$out/resolved.txt"
  if GIT_EDITOR=true git cherry-pick --continue >> "$out/pick.log" 2>&1; then
    exit 0
  fi
done

git diff --name-only --diff-filter=U > "$out/conflicts.txt"
[ -s "$out/conflicts.txt" ] || echo "(no conflicting file: see the log)" > "$out/conflicts.txt"
git log -1 --format="%h %s" CHERRY_PICK_HEAD > "$out/stopped.txt" 2>/dev/null || true
git cherry-pick --abort >/dev/null 2>&1 || true
exit 2
