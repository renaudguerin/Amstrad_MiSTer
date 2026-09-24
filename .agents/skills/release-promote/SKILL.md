---
name: release-promote
description: Promote a verified FPGA synthesis build into a public GitHub Release with MiSTer RBF bitstream, timing verification, and changelog.
---

# release-promote

Promote a verified core synthesis build into a public GitHub Release. This workflow packages
the pre-compiled `.rbf` bitstream under standard MiSTer naming conventions (`Amstrad_YYYYMMDD.rbf`),
verifies compile effort and TimeQuest timing closure, generates structured release notes, and
creates a GitHub Release.

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
- **Customization & Inclusion**:
  - `--tag <tag>`: Override default tag name `vYYYY.MM.DD` (or `vYYYY.MM.DD.<n>`).
  - `--title <title>`: Override default title (`Amstrad CPC / Plus Release YYYY-MM-DD (<shortsha>)`).
  - `--notes <file>`: Use custom Markdown release notes instead of auto-generated notes.
  - `--skip-timing-check`: Bypass setup/hold slack check (emergency only).
  - `--include-reports`: Also attach synthesis reports zip.
  - `--include-boot-rom`: Also attach companion `boot.rom`.
  - `--include-sha-rbf`: Also attach commit-SHA-tagged RBF alias.
  - `--include-checksums`: Also attach `SHA256SUMS.txt` manifest.

## Pre-flight Checklist

1. Confirm the GitHub CLI (`gh`) is authenticated with release/repo write scopes:
   ```bash
   gh auth status
   ```
2. Confirm the target commit has passed CI gates and produced a full-effort synthesis artifact:
   ```bash
   gh run list --branch master --workflow "Build core" --status success --limit 5
   ```

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

By default, each release bundles:
1. `Amstrad_YYYYMMDD.rbf`: The primary core bitstream, named for direct drop-in to `/media/fat/_Computer/` on the MiSTer SD card.

*Note: Optional assets (`boot.rom`, reports zip, SHA-tagged RBF alias, and `SHA256SUMS.txt`) can be attached via their respective `--include-*` flags if ever needed.*
