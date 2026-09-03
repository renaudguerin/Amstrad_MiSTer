# Unattended session handoff — 2026-09-03 (docs-only)

Source writes are finished. Main source tip is unchanged at `a8286bd`; this
handoff touches documentation only and adds no new validated source. No
commits, branches, pushes, integration, simulation, or independent review
were run for it.

## Exact positions

- Main: `a8286bd` (docs-only B6/B10 wording/comments on accepted `d3aabbc`).
- Plus READY (NOT on main): `bb77075` on `plus/b3-capture-recovery`, based
  exactly on `a8286bd` — P10j comment commit `3db81d0` + B3 capture commit
  `bb77075`. Range-diff proves both reviewed patches unchanged since review
  (`d52df41`=`3db81d0`, `1e2fb3e`=`bb77075`); final parent `make -C sim` and
  `make -C sim lint` on exact `bb77075` both exit 0 (2026-09-03). Inspect via
  `git show bb77075 --stat` / `git show 3db81d0 --stat` from a clone
  containing the Plus branch. Plus-branch doc paths are deliberately not
  linked here because they do not exist on main.
- Stash `0fe18a4513a47e4f21e0f504f002673a853388c3` intact. B3 capture is
  recovered only on the Plus branch; the six stashed u765 pre-edge tests
  remain unaccepted and unapplied.

## What is reviewed, and what it is not

- **FDC:** accepted observe-only first-divergence diagnostics `d3aabbc`
  ([report](fdc-recovery-2026-09-03.md), independent review CLEAR in
  [diagnostics review](fdc-diagnostics-review-2026-09-03.md)). XFAIL
  byte-identical in strength; original `fdc-payload-poll` XFAIL stays.
  Required and unmet: classic AMSDOS full command/data regression. The
  reduced-TV80 surrogate never executes `JR`/`JP cc` conditionally, so the
  old polling fixture proves nothing about the controller — no controller
  RTL change is justified by it. The failed experiment is preserved only as
  untracked `.fdc-scratch/` scratch, not committed evidence.
- **P10j:** independently CLEARED on the Plus branch (`3db81d0`, Gemini 3.8
  Flash high, source-verified SNA-drain/CPU-reset chain, comment-only).
  Open on main until integration lands.
- **B3 capture:** independent review CLEAR on the Plus branch (`bb77075`)
  with one non-blocking test-tightening suggestion (assert the
  dangling-symlink target and link state on the filesystem; atomic refusal
  and all real CLI negative cases already pass). Bounded synthetic
  self-equality evidence only — no hardware, title, or screenshot oracle; no
  golden hash committed; reduced-TV80 opcode boundary recorded.
- **No unresolved independent-review findings** on the reviewed candidates is
  distinct from comments not yet integrated and from the many retained
  validation residuals (Navy-Seals prerequisite retest, CRTC3-leak trace,
  System-CPR + production-T80/full-top closure, exact-tip Quartus/hardware,
  TV80/model limits).
- Seven prior debt rows were cleared at `22ad766`; B6-1/B6-2/B10-2/B10-3
  handling and the B10-1 residual (proper wrapper coupled test, without
  source-scraping) are recorded in [review debt](review-debt.md) and the
  [B6/B10 follow-ups](review-followups-2026-09-03.md).

## Hardware and oracle tiers (unchanged, exact user qualifiers)

On `Amstrad_20260901_84e6969.rbf` with Live blanking: Amazing Demo named
screenshot **appears fixed**, Burnin' Rubber right-edge sprite **fixed**;
DSC4/SHAKER remain failures with **possibly changed form** and no new
screenshots; other defects TBD. Full record and limits:
[hardware-evidence-2026-09-02.md](hardware-evidence-2026-09-02.md). ACCC
v1.11 unchanged; the author message is dated clarification only. B8 excluded
by decision. No new screenshots, RBF, CI, or hardware closure this session.

## Next useful steps (technical prerequisites; B8 excluded; only item 1 needs separate authorization)

1. Separately authorized serial integration of the Plus READY branch
   (`bb77075`, NOT in main yet) into main (accuracy first only if a second
   stream exists; one integration writer at a time), then CI/gates and
   exact-artifact handoff for hardware tests; local gates on `bb77075`
   already passed.
2. Shared FDC recovery (already authorized; deferred pending a
   production-T80/faithful-CPU boundary) for the classic AMSDOS regression
   plus Plus disk coverage; B10-1 likewise waits on an appropriate wrapper
   test seam, not new approval.
3. Current DSC4/SHAKER captures (or equivalent timing traces) before any
   targeted accuracy fix (already authorized); accuracy RTL stays deferred
   meanwhile.
4. Bridge lifecycle visibility: recorded only as REQUESTED/PENDING Agents
   Roster setup task (pending client id only, never a repaired or running
   task without ID); the requested repair stays already-authorized work.
   No repair or completion is claimed.
