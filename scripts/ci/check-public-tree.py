#!/usr/bin/env python3
"""Fail when the published tree gains working documents or broken local links."""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PUBLIC_DOCS = {
    "docs/README.md", "docs/building.md", "docs/roadmap.md",
    "docs/known-issues.md", "docs/images/banner.jpg",
}
TOP = {
    ".gitignore", ".worktreeinclude", "AGENTS.md", "README.md", "CREDITS.md",
    "Amstrad.qpf", "Amstrad.qsf", "Amstrad.sdc", "Amstrad.srf", "Amstrad.sv",
    "Amstrad_Q13.qpf", "Amstrad_Q13.qsf", "Amstrad_Q13.srf",
    "files.qip", "clean.bat",
}
PUBLIC_GITHUB = {
    ".github/FUNDING.yml",
    ".github/ISSUE_TEMPLATE/bug_report.md",
    ".github/ISSUE_TEMPLATE/feature_request.md",
    ".github/actions/setup-verilator/action.yml",
    ".github/workflows/build.yml",
    ".github/workflows/local-build.yml",
    ".github/workflows/public-tree.yml",
}
PUBLIC_SCRIPTS = {
    "scripts/install_ghdl_macos.sh",
    "scripts/diagnostics/README.md",
    "scripts/diagnostics/classic_b25.asm",
    "scripts/diagnostics/classic_b25.py",
    "scripts/diagnostics/classic_b25_sim.py",
    "scripts/diagnostics/fdc_read_probe.asm",
    "scripts/diagnostics/fdc_read_probe.py",
    "scripts/diagnostics/pa7_dma_overlap.asm",
    "scripts/diagnostics/pa7_dma_overlap.py",
    "scripts/diagnostics/plus_hw_probes.asm",
    "scripts/diagnostics/plus_hw_probes.py",
    "scripts/diagnostics/plus_hw_probes_sim.cpp",
    "scripts/diagnostics/plus_hw_probes_sim.py",
    "scripts/diagnostics/pri_alias_probe.py",
    "scripts/hardware-loop/.gitignore",
    "scripts/hardware-loop/driver.py",
    "scripts/hardware-loop/shaker_ssm_inventory.py",
    "scripts/hardware-loop/sna_pull.py",
    "scripts/hardware-loop/ssm_dt_trace.py",
    "scripts/hardware-loop/ssm_dt_trace_schema.json",
    "scripts/hardware-loop/ssm_ring.py",
    "scripts/hardware-loop/test_driver.py",
    "scripts/hardware-loop/test_shaker_ssm_inventory.py",
    "scripts/hardware-loop/test_sna_pull.py",
}
PUBLIC_SIM_DOCS = {"sim/README.md", "sim/TESTS.md", "sim/soak-golden-history.md"}
PUBLIC_ROMS = {"roms/README.md", "roms/boot.rom", "roms/cpc464nd.eZ0"}
PUBLIC_RTL_ASSETS = {
    "rtl/GA40010/40010-simplified_V03.pdf", "rtl/u765/test.dsk",
    "rtl/T80/README",
}
LINK = re.compile(r"!?\[[^]]*\]\(([^)]+)\)")


def allowed(path):
    if path in TOP | PUBLIC_DOCS | PUBLIC_GITHUB | PUBLIC_SCRIPTS | PUBLIC_SIM_DOCS | PUBLIC_ROMS | PUBLIC_RTL_ASSETS:
        return True
    if path.startswith("rtl/"):
        return Path(path).suffix in {".v", ".sv", ".vhd", ".qip", ".cpp"} or Path(path).name in {"Makefile", ".gitignore"}
    if path.startswith("sys/"):
        return Path(path).suffix in {".v", ".sv", ".vhd", ".vh", ".qip", ".tcl", ".sdc"}
    if path.startswith("scripts/ci/"):
        return Path(path).suffix in {".sh", ".py", ".tcl"}
    if path.startswith("sim/"):
        return Path(path).suffix in {".cpp", ".v", ".sv", ".vhd", ".py", ".mk", ".vlt"} or Path(path).name in {"Makefile", ".gitignore"}
    return False


def main():
    tracked = subprocess.check_output(
        ["git", "ls-files", "-z"], cwd=ROOT
    ).decode().split("\0")
    tracked = [path for path in tracked if path]
    errors = [f"unapproved public path: {path}" for path in tracked if not allowed(path)]
    for name in tracked:
        if not name.endswith(".md"):
            continue
        source = ROOT / name
        for target in LINK.findall(source.read_text(errors="replace")):
            target = target.split("#", 1)[0].strip()
            if not target or "://" in target or target.startswith(("mailto:", "#")):
                continue
            target = target.split(" ", 1)[0]
            resolved = (source.parent / target).resolve()
            if not resolved.is_file() or not resolved.is_relative_to(ROOT):
                errors.append(f"broken local link: {name} -> {target}")
    for error in errors:
        print(f"public-tree: {error}", file=sys.stderr)
    if errors:
        return 1
    print(f"public-tree: OK ({len(tracked)} tracked paths)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
