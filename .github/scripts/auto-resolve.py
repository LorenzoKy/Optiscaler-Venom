#!/usr/bin/env python3
# Solves the conflicts of a file written with diff3 markers (git checkout --conflict=diff3) when one side only added
# lines, so no line of either side is lost or changed:
# - both sides added lines at the same place (empty base): upstream's lines, then the Venom ones
# - one side kept the base as it was and only added lines before and/or after it: those lines around the other side
# Anything else (both sides changed the same lines) stays for a human: the file is left untouched.
# Usage: auto-resolve.py <file>...  Exit 0 = every conflict of every file solved and written, 1 = some left.
# Prints one line per file to stdout: "<file>: <n> solved" or "<file>: <n> of <m> left".
import re
import sys

START = re.compile(rb"^<<<<<<< ")
BASE = re.compile(rb"^\|\|\|\|\|\|\| ")
SPLIT = re.compile(rb"^=======\r?\n?$")
END = re.compile(rb"^>>>>>>> ")


def find(whole, part):
    """Positions where part is in whole, line by line"""
    return [i for i in range(len(whole) - len(part) + 1) if whole[i : i + len(part)] == part]


def solve(ours, base, theirs):
    if ours == theirs or theirs == base:
        return ours
    if ours == base:
        return theirs
    if not base:
        # The same lines added on both sides, and more on one: the longer one
        if find(theirs, ours):
            return theirs
        if find(ours, theirs):
            return ours
        return ours + theirs
    # Blank lines alone can't tell where the base is
    if all(not line.strip() for line in base):
        return None
    for kept, changed in ((theirs, ours), (ours, theirs)):
        at = find(kept, base)
        if len(at) == 1:
            return kept[: at[0]] + changed + kept[at[0] + len(base) :]
    return None


def resolve(path):
    lines = open(path, "rb").read().splitlines(keepends=True)
    out, solved, total, i = [], 0, 0, 0
    while i < len(lines):
        if not START.match(lines[i]):
            out.append(lines[i])
            i += 1
            continue
        start, parts, side = i, {"ours": [], "base": [], "theirs": []}, "ours"
        i += 1
        while i < len(lines) and not END.match(lines[i]):
            if BASE.match(lines[i]) and side == "ours":
                side = "base"
            elif SPLIT.match(lines[i]) and side in ("ours", "base"):
                side = "theirs"
            else:
                parts[side].append(lines[i])
            i += 1
        if i == len(lines) or side != "theirs":
            raise SystemExit(f"{path}: a conflict without diff3 markers")
        total += 1
        result = solve(parts["ours"], parts["base"], parts["theirs"])
        if result is None:
            out.extend(lines[start : i + 1])
        else:
            out.extend(result)
            solved += 1
        i += 1
    if solved == total:
        open(path, "wb").write(b"".join(out))
        print(f"{path}: {solved} solved")
        return True
    print(f"{path}: {total - solved} of {total} left")
    return False


ok = True
for path in sys.argv[1:]:
    ok = resolve(path) and ok
sys.exit(0 if ok else 1)
