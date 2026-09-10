# F16/F17/F18 independent review — Codex (roster-codex exec), 2026-08-27

Scope: the branch diff on `accuracy/classic-f16-f17-closure` (F16 fixtures+RTL, F17 fixtures+RTL, F18 readback validation+pinning, docs). Reviewer: Codex (`gpt-5.6-sol`, high reasoning effort) via `~/.claude/bin/roster-codex exec` (workspace-write sandbox), fresh session, with the rendered ACCC v1.10 pages (pp. 219–224, p. 88, p. 245, p. 293) named as primary sources and the gate/soak state stated in the brief.

Initial Verdict: **NOT CLEAR** on one blocking finding (B-1). Remediated in-branch; remediation verified with its own bite test.

1. **BLOCKING B-1 — Repeated RFD on C9=R9 with pre-existing `rfd_vma_flag == 1` reloaded VMA on the trigger rollover.**
   `rfd_vma_active` evaluated `rfd_vma_flag | rfd_vma_arm`, suppressing only the *new* arm on `C9==R9` (`rfd_vma_arm = 0`). If an earlier RFD had armed `rfd_vma_flag = 1`, a subsequent (repeated) RFD triggered on `line == crtc1_line_max` still evaluated `rfd_vma_active = 1` on that same rollover edge before the sequential clear took effect at `posedge CLOCK`, triggering an unwanted R12/R13 reload.
   **Remediation**: Added `rfd_vma_disarm_hit = rfd_arm & (line == crtc1_line_max)` and defined `rfd_vma_active = (rfd_vma_flag & ~rfd_vma_disarm_hit) | rfd_vma_arm`. Updated fixture `t13n` to explicitly set up an active source flag prior to triggering a repeated RFD on `C9=R9` and verify that MA does not reload on that rollover.
   **Bite**: Removing `~rfd_vma_disarm_hit` fails `t13n`.

Positive findings recorded for the session:
- **F16 (Type-0 post-IVM exit frozen comparator)**: `rtl/crtc_type0_engine.v` correctly latches `exit_frozen_vma` upon leaving IVM and keeps comparing it against plain R9 in `type0_limit_value` and `type0_seam_value` until match or re-entry, matching ACCC v1.10 §19.8.1 and the pp. 223–224 tables. Fixtures `t22l`–`t22s` and `t30a`/`t30b` verify the full exit matrix and mid-line recovery recipe.
- **F17 (Type-1 RFD C9=R9 source disable)**: Disarms `rfd_vma_flag` while arming `rfd_parity_flag`, per ACCC v1.10 §11.6.1 p. 88 Case 2. Effective `crtc1_rollover_r5` is also properly wired into `crtc1_adj_entry_from_row0`.
- **F18 (Type-1 register readback matrix)**: Readback multiplexer in `rtl/CRTC.v` conforms strictly to ACCC v1.10 §21.2.2 and §28.1.9, pinned across all 32 addresses in `t01`.
- **Stream isolation**: Zero modifications to `rtl/plus/`, `sim/plus/`, or `docs/plus/`.

Post-remediation state:
- Gate: 172 required passes / 0 failed.
- Lint: Clean across both suites.
- Soak: Verified golden hash `0x48146d2b681268ab`.

Final Verdict: **CLEAR**

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot (CC BY-NC-ND).
