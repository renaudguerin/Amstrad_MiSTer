---
name: release-promote
description: Promote a verified FPGA synthesis build into a public GitHub Release with MiSTer assets, system boot ROM, timing verification, and changelog.
---

# release-promote

Promote a verified core synthesis build into a public GitHub Release. This workflow packages
pre-compiled `.rbf` bitstreams under standard MiSTer naming conventions, bundles the companion
`boot.rom`, verifies compile effort and TimeQuest timing closure, generates structured release
notes, and creates a GitHub Release.

Works across any agent (Claude Code, Codex, OpenCode, Antigravity) or directly via shell.

## Inputs

- **Build Source**:
  - Default: the latest successful full-effort synthesis run on `master`.
  - Specific commit: `--sha <sha>` (e.g. `ef8da61`).
  - Specific CI run: `--run-id <id>`.
  - Local file: `--file <path/to/Amstrad.rbf>` (with optional `--reports-dir`).
- **Release Mode**:
  - `--draft`: (Default) Creates a draft release on GitHub for preview and staging.
  - `--publish`: Immediately publishes the release as public.
  - `--dry-run`: Validates artifacts, timing closure, and previews generated notes without touching GitHub.
- **Customization**:
  - `--tag <tag>`: Override default tag name `vYYYY.MM.DD` (or `vYYYY.MM.DD.<n>`).
  - `--title <title>`: Override default title (`Amstrad CPC / Plus Release YYYY-MM-DD (<shortsha>)`).
  - `--notes <file>`: Use custom Markdown release notes instead of auto-generated notes.
  - `--skip-timing-check`: Bypass setup/hold slack check (emergency only).

## Pre-flight Checklist

1. Confirm the GitHub CLI (`gh`) is authenticated with release/repo write scopes:
   ```bash
   gh auth status
   ```
2. Confirm the target commit has passed CI gates and produced a full-effort synthesis artifact:
   ```bash
   gh run list --branch master --workflow "Build core" --status success --limit 5
   ```
3. Ensure `roms/boot.rom` is present in the repository (it is automatically attached to the release).

## Execution

### 1. Dry Run (Preview & Verification)
Verify quality gates, artifact resolution, and preview release notes without modifying GitHub:
```bash
python3 scripts/release/promote.py --dry-run
```

To target a specific commit:
```bash
python3 scripts/release/promote.py --sha <commit-sha> --dry-run
```

### 2. Create Draft Release (Recommended Default)
Packages the assets, verifies timing closure, and creates a draft release on GitHub:
```bash
python3 scripts/release/promote.py
```
Inspect the output URL (e.g. `https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-...`) to preview the notes and asset attachments in the GitHub web interface.

### 3. Publish Public Release
When ready to publish directly or after reviewing:
```bash
python3 scripts/release/promote.py --publish
```
Or publish an existing draft from the GitHub web UI or with:
```bash
gh release edit <tag> --draft=false
```

## Release Assets Produced

Each release automatically bundles:
1. `Amstrad_YYYYMMDD.rbf`: The primary core bitstream, named for direct drop-in to `/media/fat/_Computer/` on the MiSTer SD card.
2. `Amstrad_YYYYMMDD_<shortsha>.rbf`: Duplicate alias preserving the exact commit SHA for archival identification.
3. `boot.rom`: The 160 KiB system ROM bundle (required in `/media/fat/Games/Amstrad/boot.rom`).
4. `Amstrad_YYYYMMDD_<shortsha>_reports.zip`: TimeQuest timing summary, fitter report, map report, and compile log.
5. `SHA256SUMS.txt`: Cryptographic checksum manifest for all release assets.
