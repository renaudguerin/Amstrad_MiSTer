---
name: cpcse-update
description: Update the local CPCSyntaxError source and macOS release, verify the installed package, preserve old settings during replacement, and triage release accuracy changes against MiSTer evidence. Use when asked to check or install CPCSyntaxError releases.
---

# Update CPCSyntaxError

Use this for the tool installation and release-diff pass. For experiments with a
specific CPC program, use `cpcse-test` and
[the toolbelt guide](../../../docs/cpcsyntaxerror-testing.md).

## Establish the target

1. Inspect the current Git branch, attached worktrees and dirty files in this
   project and `../CPCSyntaxError`. Keep unfinished worktrees intact. Read the
   installed version and pinned source/build from the toolbelt guide; find
   actual package directories under `local/` in each relevant checkout.
2. Check the live [GitHub releases](https://github.com/WacKEDmaN/CPCSyntaxError/releases)
   and the release API/asset metadata for the newest tag and macOS universal
   package. Read every intervening release note. Record the tag, source commit,
   asset URL and any published digest. Do not infer the newest release from a
   previously downloaded file.

## Update and verify

3. Fetch tags in `../CPCSyntaxError` after confirming its local changes are
   safe. Check out the selected tag; record the exact commit. A source diff
   must compare that commit with the previously pinned tag, not a moving branch.
4. Download the macOS universal archive to a temporary path. Compare its
   SHA-256 with a published digest when provided; run `unzip -t`. Extract into
   `local/CPCSyntaxError-macos-universal-vX.Y.Z/`, which Git ignores. Check the
   app executable architecture with `file`, verify its signature with
   `codesign --verify --deep --strict`, and record the executable hash. Confirm
   the extracted version matches the checked-out source tag.
5. Use `xattr` recursively on the new package to remove
   `com.apple.quarantine` and `com.apple.provenance`; inspect the recursive
   attribute listing afterward. An exit status alone is insufficient: on an
   SSH-hosted macOS session, removal of `com.apple.provenance` returned success
   while the attribute remained on 59 entries. If it persists, record the
   count and do not claim it was cleared. Check whether quarantine remains and
   whether the headless executable runs; treat the cause of persistent
   provenance as unconfirmed.
6. Smoke-test the extracted binary with its bundled ROMs: one-frame headless
   `--shot` and a minimal CSL reset/wait in a fresh output directory. Keep
   command, exit status and output. `--help` is not a safe version probe for
   this build; it can open the GUI. The detailed SHAKER recipe is a separate
   acceptance step and must not be claimed from the smoke test.
7. If replacement/removal was requested, inspect old package directories for
   generated settings, layout files and crash reports; preserve user-created
   files in a clearly named ignored `local/` location before deleting only the
   exact superseded packages. Do not delete source trees or historical
   captures. Keep other worktrees' packages and settings separate.

## Transfer findings

8. Update the toolbelt guide and current-status handoff with installed version,
   source commit, executable hash, verified commands and evidence limits.
   Preserve historical version pins and hashes for old captures.
9. Compare the release notes with source diffs, then with our current RTL,
   [backlog](../../../docs/backlog.md), [roadmap](../../../docs/implementation-roadmap.md)
   and active worktrees. Classify each accuracy claim as an open MiSTer lead,
   already implemented, contradicted by original hardware, or emulator-only.
   Link a lead to its existing defect where possible. Source code is a
   hypothesis; original hardware and the French ACCC govern our behavior.
   Record overlap with V1 CRT response and the RTL audit before proposing a
   shared-module change. Re-run a candidate on the new binary before citing
   its result; old captures remain bound to their old executable.

For docs and skill edits, check `git diff --check` and validate the skill. If
RTL or code changes, follow this repository's focused failing-before proof,
review and selected simulation gate. Leave integration to the requested task
workflow.
