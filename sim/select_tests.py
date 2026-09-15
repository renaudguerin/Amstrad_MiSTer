#!/usr/bin/env python3
"""Pick the simulation benches a change needs; rules and index in sim/TESTS.md."""

import argparse
import fnmatch
import os
import re
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INDEX = ROOT / "sim" / "TESTS.md"
SOURCE_SUFFIXES = {".v", ".sv", ".vh", ".cpp", ".h", ".vhd", ".vlt"}
# Shared build inputs: any change here selects every fast bench.
INFRA = [
    "sim/Makefile",
    "sim/plus/Makefile",
    "rtl/GA40010/Makefile",
    "rtl/u765/Makefile",
    ".github/actions/setup-verilator/*",
    "scripts/ci/install-verilator.sh",
]


class Bench:
    def __init__(self, directory, target, tier, covers, checks):
        self.dir, self.target, self.tier = directory, target, tier
        self.covers, self.checks = covers, checks
        self.reasons = []

    @property
    def name(self):
        return f"{self.dir} {self.target}"


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True, text=True,
                          capture_output=True).stdout


def load_index():
    benches = []
    for line in INDEX.read_text().splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) != 4 or not re.fullmatch(r"`\S+ \S+`", cells[0]):
            continue
        directory, target = cells[0].strip("`").split()
        covers = [g for g in cells[2].replace("`", " ").split() if g != "—"]
        benches.append(Bench(directory, target, cells[1], covers, cells[3]))
    return benches


def matches(path, globs):
    return any(fnmatch.fnmatchcase(path, g) for g in globs)


def dependencies(bench):
    """Repository files the bench compiles, from a dry run plus C++ includes."""
    out = subprocess.run(["make", "-n", "-B", "-w", "-C", bench.dir, bench.target],
                         cwd=ROOT, text=True, capture_output=True).stdout
    stack, found = [ROOT / bench.dir], set()
    for line in out.splitlines():
        entering = re.search(r"Entering directory [`'](.+)'", line)
        if entering:
            stack.append(Path(entering.group(1)))
            continue
        if re.search(r"Leaving directory", line):
            if len(stack) > 1:
                stack.pop()
            continue
        try:
            words = shlex.split(line)
        except ValueError:
            words = line.split()
        for word in words:
            if Path(word).suffix in SOURCE_SUFFIXES and not word.startswith(("+", "-")):
                found.add((stack[-1] / word).resolve())
    pending = [p for p in found if p.suffix in {".cpp", ".h"}]
    while pending:
        source = pending.pop()
        if not source.is_file():
            continue
        for include in re.findall(r'#include "([^"]+)"', source.read_text(errors="replace")):
            path = (source.parent / include).resolve()
            if path.is_file() and path not in found:
                found.add(path)
                pending.append(path)
    rel = set()
    for path in found:
        if path.is_file() and path.is_relative_to(ROOT):
            rel.add(path.relative_to(ROOT).as_posix())
    return rel


def changed_files(base, head):
    if head:
        names = git("diff", "--name-only", "--no-renames", base, head).split()
    else:
        names = git("diff", "--name-only", "--no-renames", base).split()
        names += git("ls-files", "--others", "--exclude-standard").split()
    return sorted(set(names))


def default_base():
    for ref in ("origin/master", "master"):
        try:
            return git("merge-base", "HEAD", ref).strip()
        except subprocess.CalledProcessError:
            continue
    sys.exit("select_tests: no merge base with master; pass --base")


def select(benches, changed):
    infra = [f for f in changed if matches(f, INFRA)]
    with ThreadPoolExecutor() as pool:
        deps = dict(zip(benches, pool.map(dependencies, benches)))
    covered = {f for f in changed for b in benches if matches(f, b.covers)}
    for b in benches:
        if not b.covers:  # on-demand variants such as b6-video-boundary-strict
            continue
        b.reasons = [f for f in changed if matches(f, b.covers)]
        b.reasons += [f for f in changed if f in deps[b] and f.startswith("sim/")
                      and f not in b.reasons]
        if b.tier == "fast":
            b.reasons += infra
    uncovered = [f for f in changed
                 if f.startswith(("rtl/", "sys/")) and f not in covered]
    for f in uncovered:
        for b in benches:
            if b.tier == "fast" and f in deps[b]:
                b.reasons.append(f"{f} (compiled, not covered)")
    return [b for b in benches if b.reasons], uncovered


def check(benches):
    errors = []
    tracked = git("ls-files").split()
    for directory in sorted({b.dir for b in benches} | {"sim", "sim/plus"}):
        listed = set(subprocess.run(["make", "-s", "-C", directory, "list-tests"], cwd=ROOT,
                                    check=True, text=True, capture_output=True).stdout.split())
        indexed = {b.target for b in benches if b.dir == directory}
        errors += [f"{directory} {t}: test target has no row" for t in sorted(listed - indexed)]
        errors += [f"{directory} {t}: row names no test target" for t in sorted(indexed - listed)]
    for b in benches:
        if b.tier not in ("fast", "slow"):
            errors.append(f"{b.name}: tier must be fast or slow")
        errors += [f"{b.name}: {g} matches no tracked file"
                   for g in b.covers if not any(fnmatch.fnmatchcase(t, g) for t in tracked)]
    for e in errors:
        print(f"select_tests: {e}", file=sys.stderr)
    return not errors


def make_commands(benches):
    jobs = os.cpu_count() or 4
    version = subprocess.run(["make", "--version"], text=True, capture_output=True).stdout
    sync = [] if re.match(r"GNU Make 3\.", version) else ["-Otarget"]
    by_dir = {}
    for b in benches:
        by_dir.setdefault(b.dir, []).append(b.target)
    return [["make", "-C", d, f"-j{jobs}", *sync, *targets] for d, targets in by_dir.items()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", help="compare against this commit (default: merge base with master)")
    parser.add_argument("--head", help="compare base..head instead of the working tree")
    parser.add_argument("--files", nargs="+", help="use these changed paths instead of git")
    parser.add_argument("--all", action="store_true", help="select every bench in the chosen tiers")
    parser.add_argument("--slow", action="store_true", help="also run selected slow benches")
    parser.add_argument("--run", action="store_true", help="run the selection")
    parser.add_argument("--check", action="store_true", help="validate the index and exit")
    args = parser.parse_args()

    benches = load_index()
    if args.check:
        ok = check(benches)
        print(f"select_tests: index {'OK' if ok else 'FAILED'} ({len(benches)} rows)")
        return 0 if ok else 1

    if args.all:
        selected = [b for b in benches if b.covers and (b.tier == "fast" or args.slow)]
        uncovered = []
        for b in selected:
            b.reasons = ["--all"]
        print("select_tests: all benches")
    else:
        if args.files:
            changed = sorted(args.files)
            print(f"select_tests: {len(changed)} given paths")
        else:
            base = args.base or default_base()
            changed = changed_files(base, args.head)
            span = f"{base[:12]}..{args.head[:12]}" if args.head else f"{base[:12]}..working tree"
            print(f"select_tests: {len(changed)} changed files ({span})")
        selected, uncovered = select(benches, changed)

    to_run = [b for b in selected if b.tier == "fast" or args.slow]
    skipped = [b for b in selected if b not in to_run]
    for title, group in (("Selected", to_run), ("Relevant slow benches, not run (add --slow)", skipped)):
        if group:
            print(f"{title}:")
            for b in group:
                print(f"  {b.name:45} {', '.join(b.reasons[:3])}{' ...' if len(b.reasons) > 3 else ''}")
    for f in uncovered:
        print(f"select_tests: note: no row covers {f}; add it to the bench that protects it")
    commands = make_commands(to_run)
    if not commands:
        print("select_tests: no simulation needed")
        return 0
    for command in commands:
        print("  " + shlex.join(command))
    if not args.run:
        return 0

    failed = []
    for command in commands:
        if subprocess.run(command, cwd=ROOT).returncode:
            failed.append(command[2])
    names = ", ".join(b.target for b in to_run)
    if failed:
        print(f"select_tests: FAILED in {', '.join(failed)} ({len(to_run)} benches: {names})")
        return 1
    print(f"select_tests: PASS {len(to_run)} benches: {names}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
