#!/usr/bin/env python3
"""Promote a synthesis build artifact to a public GitHub Release.

This script identifies a build (from local output_files or GitHub Actions CI),
verifies compile effort tier and TimeQuest timing closure, packages the assets
under MiSTer naming conventions, auto-generates release notes, and creates
a GitHub Release (draft by default) via the `gh` CLI.

Usage examples:
  # Promote the latest successful master synthesis build to a draft release:
  python3 scripts/release/promote.py

  # Test without making network or release calls:
  python3 scripts/release/promote.py --dry-run

  # Promote a specific commit SHA and publish directly:
  python3 scripts/release/promote.py --sha ef8da61 --publish

  # Promote a local RBF file directly:
  python3 scripts/release/promote.py --file output_files/Amstrad_20260922_7f1712b.rbf
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


def run_cmd(cmd, cwd=None, check=True):
    """Run a shell command and return CompletedProcess."""
    res = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    if check and res.returncode != 0:
        cmd_str = " ".join(cmd) if isinstance(cmd, list) else cmd
        raise RuntimeError(
            f"Command failed (exit {res.returncode}): {cmd_str}\n"
            f"STDOUT: {res.stdout.strip()}\n"
            f"STDERR: {res.stderr.strip()}"
        )
    return res


def sha256_file(filepath):
    """Compute SHA-256 hash of a file."""
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def detect_repo(explicit_repo=None):
    """Detect GitHub repository name (owner/repo)."""
    if explicit_repo:
        return explicit_repo
    res = run_cmd(["git", "remote", "get-url", "origin"], check=False)
    if res.returncode == 0:
        url = res.stdout.strip()
        # Parse git@github.com:owner/repo.git or https://github.com/owner/repo.git
        m = re.search(r"github\.com[:/]([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+?)(?:\.git)?$", url)
        if m:
            return m.group(1)
    return "renaudguerin/Amstrad_MiSTer"


def find_ci_run_for_release(repo, target_sha=None, run_id=None):
    """Find the target GitHub Actions workflow run with full synthesis artifacts."""
    if run_id:
        cmd = ["gh", "run", "view", str(run_id), "-R", repo, "--json", "databaseId,headSha,headBranch,conclusion,status,displayTitle"]
        res = run_cmd(cmd)
        run_data = json.loads(res.stdout)
        return run_data

    # Query recent successful runs on master branch
    cmd = [
        "gh", "run", "list",
        "-R", repo,
        "--branch", "master",
        "--workflow", "Build core",
        "--status", "success",
        "--limit", "15",
        "--json", "databaseId,headSha,headBranch,conclusion,status,displayTitle,createdAt"
    ]
    res = run_cmd(cmd)
    runs = json.loads(res.stdout)
    if not runs:
        raise RuntimeError(f"No successful 'Build core' runs found on branch 'master' in {repo}")

    if target_sha:
        target_sha_lower = target_sha.lower()
        for r in runs:
            if r["headSha"].lower().startswith(target_sha_lower):
                return r
        raise RuntimeError(f"Could not find a successful CI run for commit SHA {target_sha} on branch 'master'")

    # Without target_sha: inspect runs from newest to oldest to find one that generated a full synthesis artifact
    for r in runs:
        art_cmd = ["gh", "api", f"repos/{repo}/actions/runs/{r['databaseId']}/artifacts", "--jq", ".artifacts[].name"]
        art_res = run_cmd(art_cmd, check=False)
        if art_res.returncode == 0 and any("full" in line for line in art_res.stdout.splitlines()):
            return r

    # Fallback to the latest run if none explicitly checked
    return runs[0]


def locate_or_download_artifacts(repo, run, work_dir, explicit_file=None, explicit_reports_dir=None):
    """Locate build artifacts locally or download them via gh CLI.

    Returns (rbf_path, reports_dir, short_sha, build_date).
    """
    head_sha = run.get("headSha", "") if run else ""
    short_sha = head_sha[:7] if head_sha else ""

    # Case 1: Explicit local file provided
    if explicit_file:
        rbf_path = Path(explicit_file).resolve()
        if not rbf_path.is_file():
            raise FileNotFoundError(f"Specified RBF file not found: {explicit_file}")
        # Try to deduce short sha and date from filename e.g. Amstrad_20260922_cdcb3c3.rbf
        m = re.search(r"Amstrad_(\d{8})_([0-9a-fA-F]{7})", rbf_path.name)
        if m:
            build_date = m.group(1)
            file_sha = m.group(2)
            short_sha = short_sha or file_sha
        else:
            m_date = re.search(r"(\d{8})", rbf_path.name)
            build_date = m_date.group(1) if m_date else run_cmd(["date", "-u", "+%Y%m%d"]).stdout.strip()
            short_sha = short_sha or "local"

        reports_dir = Path(explicit_reports_dir).resolve() if explicit_reports_dir else None
        if not reports_dir:
            # Check adjacent directories like reports-<sha>/
            candidate = rbf_path.parent / f"reports-{short_sha}"
            if candidate.is_dir():
                reports_dir = candidate
            elif (rbf_path.parent / "reports").is_dir():
                reports_dir = rbf_path.parent / "reports"

        return rbf_path, reports_dir, short_sha, build_date

    # Case 2: Check local output_files directories before hitting network
    local_search_paths = [
        Path.cwd() / "output_files",
        Path.cwd().parent / "output_files",
        Path.home() / "code" / "Amstrad_MiSTer" / "output_files"
    ]
    if short_sha:
        for p in local_search_paths:
            if not p.is_dir():
                continue
            matching_rbfs = list(p.glob(f"Amstrad_*_{short_sha}.rbf"))
            if matching_rbfs:
                rbf_path = matching_rbfs[0]
                m_date = re.search(r"Amstrad_(\d{8})_", rbf_path.name)
                build_date = m_date.group(1) if m_date else run_cmd(["date", "-u", "+%Y%m%d"]).stdout.strip()
                rep_dir = p / f"reports-{short_sha}"
                reports_dir = rep_dir if rep_dir.is_dir() else (p / "reports" if (p / "reports").is_dir() else None)
                print(f"Found local build artifact matching commit {short_sha}: {rbf_path}")
                return rbf_path, reports_dir, short_sha, build_date

    # Case 3: Download from GitHub Actions
    run_id = run["databaseId"]
    print(f"Fetching artifacts for GitHub Actions run #{run_id} ({run.get('displayTitle', '')})...")
    art_cmd = ["gh", "api", f"repos/{repo}/actions/runs/{run_id}/artifacts", "--jq", ".artifacts[].name"]
    art_res = run_cmd(art_cmd)
    artifact_names = [line.strip() for line in art_res.stdout.splitlines() if line.strip()]

    # Look for full-effort synthesis artifact
    full_artifacts = [name for name in artifact_names if "full" in name]
    target_artifact = full_artifacts[0] if full_artifacts else (artifact_names[0] if artifact_names else None)

    if not target_artifact:
        raise RuntimeError(f"Run #{run_id} has no uploaded artifacts to download.")

    print(f"Downloading artifact '{target_artifact}' from run #{run_id}...")
    download_dir = work_dir / "ci_download"
    download_dir.mkdir(parents=True, exist_ok=True)
    run_cmd(["gh", "run", "download", str(run_id), "-R", repo, "-n", target_artifact, "--dir", str(download_dir)])

    # Search for RBF in downloaded files
    rbfs = list(download_dir.glob("**/*.rbf"))
    if not rbfs:
        raise RuntimeError(f"No .rbf file found in downloaded artifact '{target_artifact}'")
    rbf_path = rbfs[0]

    # Extract date & sha from filename
    m = re.search(r"Amstrad_(\d{8})_([0-9a-fA-F]{7})", rbf_path.name)
    if m:
        build_date = m.group(1)
        short_sha = short_sha or m.group(2)
    else:
        build_date = run_cmd(["date", "-u", "+%Y%m%d"]).stdout.strip()
        short_sha = short_sha or head_sha[:7]

    rep_dirs = [d for d in download_dir.glob("**/reports") if d.is_dir()]
    reports_dir = rep_dirs[0] if rep_dirs else None

    return rbf_path, reports_dir, short_sha, build_date


def verify_build_quality(rbf_path, reports_dir, repo_root, skip_timing=False):
    """Verify compile tier, TimeQuest timing closure, and SHA256."""
    results = {
        "rbf_sha256": sha256_file(rbf_path),
        "rbf_size": rbf_path.stat().st_size,
        "timing_passed": False,
        "timing_summary": "Timing not checked",
        "effort_mode": "unknown"
    }

    if not reports_dir or not reports_dir.is_dir():
        print("Warning: Reports directory not found; skipping detailed timing closure verification.")
        return results

    # Check synthesis effort in quartus-cache.txt
    cache_txt = reports_dir / "quartus-cache.txt"
    if cache_txt.is_file():
        content = cache_txt.read_text().strip()
        results["effort_mode"] = content
        if "smoke" in content and "full" not in content:
            raise ValueError(f"Artifact was compiled at 'smoke' effort tier ({content}). Public releases require 'full' effort!")

    # Check TimeQuest timing closure
    sta_summary = reports_dir / "Amstrad.sta.summary"
    if sta_summary.is_file():
        timing_script = repo_root / "scripts" / "ci" / "check-quartus-timing.sh"
        if timing_script.is_file() and os.access(timing_script, os.X_OK):
            check_res = run_cmd([str(timing_script), str(sta_summary)], check=False)
            if check_res.returncode == 0:
                results["timing_passed"] = True
                results["timing_summary"] = check_res.stdout.strip()
            else:
                msg = f"TimeQuest timing closure check FAILED:\n{check_res.stderr.strip() or check_res.stdout.strip()}"
                if skip_timing:
                    print(f"WARNING: {msg}")
                    results["timing_summary"] = "FAILED (forced via --skip-timing-check)"
                else:
                    raise RuntimeError(msg)
        else:
            # Fallback inline check
            text = sta_summary.read_text()
            if "Slack : -" in text:
                if not skip_timing:
                    raise RuntimeError("Negative slack detected in Amstrad.sta.summary!")
                results["timing_summary"] = "FAILED (negative slack)"
            else:
                results["timing_passed"] = True
                results["timing_summary"] = "Setup and Hold slack positive across all clocks"

    return results


def package_release_assets(rbf_path, output_dir, build_date, short_sha,
                           include_reports=False, reports_dir=None,
                           include_boot_rom=False, repo_root=None,
                           include_sha_rbf=False,
                           include_checksums=False):
    """Package release files. By default, packages solely Amstrad_YYYYMMDD.rbf."""
    output_dir.mkdir(parents=True, exist_ok=True)
    assets = []

    # 1. Primary RBF named Amstrad_YYYYMMDD.rbf for direct MiSTer OSD drop-in
    primary_rbf = output_dir / f"Amstrad_{build_date}.rbf"
    shutil.copy2(rbf_path, primary_rbf)
    assets.append(primary_rbf)

    # Optional: SHA-bearing RBF alias
    if include_sha_rbf:
        sha_rbf = output_dir / f"Amstrad_{build_date}_{short_sha}.rbf"
        if sha_rbf != primary_rbf:
            shutil.copy2(rbf_path, sha_rbf)
            assets.append(sha_rbf)

    # Optional: System boot ROM
    if include_boot_rom and repo_root:
        rom_src = repo_root / "roms" / "boot.rom"
        if not rom_src.is_file():
            # Fallback to releases/boot.rom if old checkout
            rom_src = repo_root / "releases" / "boot.rom"
        if rom_src.is_file():
            target_rom = output_dir / "boot.rom"
            shutil.copy2(rom_src, target_rom)
            assets.append(target_rom)
        else:
            print("Warning: boot.rom not found in roms/ or releases/; skipping companion boot ROM attachment.")

    # Optional: Reports zip
    if include_reports and reports_dir and reports_dir.is_dir():
        zip_path = output_dir / f"Amstrad_{build_date}_{short_sha}_reports.zip"
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for root, _, files in os.walk(reports_dir):
                for f in files:
                    file_p = Path(root) / f
                    arcname = file_p.relative_to(reports_dir)
                    zf.write(file_p, arcname)
        assets.append(zip_path)

    # Optional: SHA256SUMS.txt
    if include_checksums:
        sums_file = output_dir / "SHA256SUMS.txt"
        with open(sums_file, "w") as sf:
            for a in assets:
                h = sha256_file(a)
                sf.write(f"{h}  {a.name}\n")
        assets.append(sums_file)

    return assets


def generate_notes(repo, target_sha, short_sha, build_date, run_id, verification_info, assets, custom_notes_path=None):
    """Generate Markdown release notes."""
    if custom_notes_path and Path(custom_notes_path).is_file():
        return Path(custom_notes_path).read_text()

    # Query recent commits since previous release tag if available
    prev_tag_res = run_cmd(["git", "describe", "--tags", "--abbrev=0"], check=False)
    prev_tag = prev_tag_res.stdout.strip() if prev_tag_res.returncode == 0 else None

    if prev_tag:
        log_res = run_cmd(["git", "log", f"{prev_tag}..{target_sha}", "--oneline", "--no-merges"], check=False)
        changelog = log_res.stdout.strip() if log_res.returncode == 0 else ""
    else:
        log_res = run_cmd(["git", "log", "-n", "15", "--oneline", "--no-merges", target_sha], check=False)
        changelog = log_res.stdout.strip() if log_res.returncode == 0 else ""

    commit_url = f"https://github.com/{repo}/commit/{target_sha}"
    run_url = f"https://github.com/{repo}/actions/runs/{run_id}" if run_id else None

    lines = [
        f"# Amstrad CPC & Plus / GX4000 Release — {build_date[:4]}-{build_date[4:6]}-{build_date[6:]}",
        "",
        f"**Commit:** [`{short_sha}`]({commit_url})  ",
        f"**Build Date:** {build_date}  ",
    ]
    if run_url:
        lines.append(f"**CI Synthesis Run:** [Actions Run #{run_id}]({run_url})  ")
    if verification_info.get("timing_summary"):
        lines.append(f"**Timing Closure:** `{verification_info['timing_summary']}`  ")

    has_boot_rom = any(a.name == "boot.rom" for a in assets)
    boot_rom_src = "attached below" if has_boot_rom else "available in repository `roms/boot.rom`"

    lines.extend([
        "",
        "## Core Capabilities & Features",
        "",
        "- **Cycle-Accurate Classic CRTC (Types 0 & 1)**:",
        "  - Dedicated per-type rule engines for Hitachi HD6845S / UM6845 (Type 0) and UM6845R (Type 1).",
        "  - Precise horizontal/vertical counting, interlace video modes (IVM), raster flash detection (RFD), and skew compensation.",
        "  - Sub-cycle CPU write race modeling (R5/R0 write retention, edge writes, R2.JIT) grounded in *The Amstrad CPC CRTC Compendium* (ACCC v1.11).",
        "- **Amstrad Plus & GX4000 Support**:",
        "  - Direct CPR cartridge file loading via OSD with atomic SDRAM memory service.",
        "  - AMS40489 ASIC features: 16 hardware sprites, 12-bit palette (4,096 colors), 3-channel audio DMA, programmable raster interrupts (PRI), and split-screen / soft scroll registers.",
        "  - Most commercial cartridge titles are playable (e.g., *Burnin' Rubber*, *Navy Seals*, *RoboCop 2*, *Pang*, *Plotting*).",
        "",
        "## Installation Instructions",
        "",
        "1. Download `Amstrad_" + build_date + ".rbf` from the assets below and copy it to:",
        "   ```",
        "   /media/fat/_Computer/Amstrad_" + build_date + ".rbf",
        "   ```",
        f"2. Ensure `boot.rom` ({boot_rom_src}) is placed at:",
        "   ```",
        "   /media/fat/Games/Amstrad/boot.rom",
        "   ```",
        "   *(The core requires this file to boot).* ",
        "3. For Amstrad Plus games: place `.CPR` cartridge images in `/media/fat/Games/Amstrad/` and mount them from the OSD menu.",
        "",
        "## Changes in this Release",
        "",
        "```",
        changelog if changelog else "Release build.",
        "```",
        "",
        "## Release Assets & Checksums",
        "",
        "| File | SHA-256 Checksum |",
        "| :--- | :--- |"
    ])

    for a in assets:
        h = sha256_file(a)
        lines.append(f"| `{a.name}` | `{h}` |")

    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Promote synthesis build to a public GitHub Release.")
    parser.add_argument("--sha", help="Target commit SHA (default: latest successful master build with full synthesis)")
    parser.add_argument("--run-id", type=int, help="Specific GitHub Actions workflow run ID")
    parser.add_argument("--file", help="Path to local .rbf bitstream file to promote")
    parser.add_argument("--reports-dir", help="Path to local reports directory (optional with --file)")
    parser.add_argument("--tag", help="Release tag (default: vYYYY.MM.DD[-N])")
    parser.add_argument("--title", help="Release title (default: Amstrad CPC/Plus Release YYYY-MM-DD (<shortsha>))")
    parser.add_argument("--notes", help="Path to custom release notes Markdown file")
    parser.add_argument("--output-dir", help="Output staging directory for packaged assets")
    parser.add_argument("--repo", help="GitHub repo in owner/repo format (default: auto-detected)")
    parser.add_argument("--publish", action="store_true", help="Publish release immediately (default is draft)")
    parser.add_argument("--draft", action="store_true", default=True, help="Create release as draft (default: True)")
    parser.add_argument("--dry-run", action="store_true", help="Inspect actions and notes without creating release on GitHub")
    parser.add_argument("--skip-timing-check", action="store_true", help="Allow release even if TimeQuest timing check fails")
    parser.add_argument("--include-reports", action="store_true", help="Also attach synthesis reports zip (default: False)")
    parser.add_argument("--include-boot-rom", action="store_true", help="Also attach companion boot.rom (default: False)")
    parser.add_argument("--include-sha-rbf", action="store_true", help="Also attach commit-SHA-tagged RBF alias (default: False)")
    parser.add_argument("--include-checksums", action="store_true", help="Also attach SHA256SUMS.txt manifest (default: False)")

    args = parser.parse_args()
    if args.publish:
        args.draft = False

    repo = detect_repo(args.repo)
    repo_root = Path(__file__).resolve().parent.parent.parent

    print(f"=== Amstrad MiSTer Release Promotion ===")
    print(f"Repository: {repo}")

    # 1. Locate Target CI Run / Artifact
    run = None
    if not args.file:
        run = find_ci_run_for_release(repo, target_sha=args.sha, run_id=args.run_id)
        print(f"Selected CI Run: #{run['databaseId']} ({run.get('displayTitle', '')})")
        print(f"Target commit: {run['headSha']}")
        target_sha = run['headSha']
    else:
        target_sha = args.sha or run_cmd(["git", "rev-parse", "HEAD"]).stdout.strip()
        print(f"Using local file: {args.file}")

    # 2. Setup Staging Directory
    staging_dir = Path(args.output_dir) if args.output_dir else Path(tempfile.mkdtemp(prefix="amstrad_rel_"))

    # 3. Locate / Download Artifacts
    rbf_path, reports_dir, short_sha, build_date = locate_or_download_artifacts(
        repo=repo,
        run=run,
        work_dir=staging_dir,
        explicit_file=args.file,
        explicit_reports_dir=args.reports_dir
    )
    print(f"Resolved RBF: {rbf_path} ({rbf_path.stat().st_size} bytes)")
    if reports_dir:
        print(f"Resolved Reports: {reports_dir}")

    # 4. Verify Quality Gates
    print("Verifying build quality and timing closure...")
    verif = verify_build_quality(rbf_path, reports_dir, repo_root, skip_timing=args.skip_timing_check)
    print(f"SHA-256: {verif['rbf_sha256']}")
    print(f"Timing:  {verif['timing_summary']}")

    # 5. Package Assets
    assets_dir = staging_dir / "release_assets"
    assets = package_release_assets(
        rbf_path=rbf_path,
        output_dir=assets_dir,
        build_date=build_date,
        short_sha=short_sha,
        include_reports=args.include_reports,
        reports_dir=reports_dir,
        include_boot_rom=args.include_boot_rom,
        repo_root=repo_root,
        include_sha_rbf=args.include_sha_rbf,
        include_checksums=args.include_checksums,
    )
    print(f"Packaged {len(assets)} release asset(s) in {assets_dir}:")
    for a in assets:
        print(f"  - {a.name} ({a.stat().st_size} bytes)")

    # 6. Tag & Title Formatting
    if not args.tag:
        base_tag = f"v{build_date[:4]}.{build_date[4:6]}.{build_date[6:]}"
        # Check if tag exists
        existing_tags = run_cmd(["git", "tag", "-l", f"{base_tag}*"]).stdout.splitlines()
        if base_tag not in existing_tags:
            tag = base_tag
        else:
            n = 1
            while f"{base_tag}.{n}" in existing_tags:
                n += 1
            tag = f"{base_tag}.{n}"
    else:
        tag = args.tag

    title = args.title or f"Amstrad CPC / Plus Release {build_date[:4]}-{build_date[4:6]}-{build_date[6:]} ({short_sha})"

    # 7. Generate Release Notes
    notes_content = generate_notes(
        repo=repo,
        target_sha=target_sha,
        short_sha=short_sha,
        build_date=build_date,
        run_id=run["databaseId"] if run else None,
        verification_info=verif,
        assets=assets,
        custom_notes_path=args.notes
    )

    notes_file = staging_dir / "RELEASE_NOTES.md"
    notes_file.write_text(notes_content)

    print(f"\nRelease Details:")
    print(f"  Tag:   {tag}")
    print(f"  Title: {title}")
    print(f"  Mode:  {'Draft' if args.draft else 'Public'}")
    print(f"  Notes preview ({len(notes_content.splitlines())} lines written to {notes_file})")

    # 8. Publication or Dry Run
    if args.dry_run:
        print("\n=== DRY RUN MODE: Release not published to GitHub ===")
        print(f"Assets to upload:")
        for a in assets:
            print(f"  - {a}")
        print("\n--- Notes Preview ---")
        print(notes_content)
        return 0

    print(f"\nCreating GitHub Release {tag} in {repo} via gh CLI...")
    rel_cmd = [
        "gh", "release", "create", tag,
        "-R", repo,
        "--title", title,
        "--notes-file", str(notes_file),
    ]
    if args.draft:
        rel_cmd.append("--draft")
    for a in assets:
        rel_cmd.append(str(a))

    res = run_cmd(rel_cmd)
    release_url = res.stdout.strip()
    print(f"\nSUCCESS: Release created successfully!")
    print(f"Release URL: {release_url}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
