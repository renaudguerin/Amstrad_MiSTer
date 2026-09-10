# Lockstep differential comparator (split-vs-reference)

Supplementary evidence for the per-type CRTC split. The committed contract for
behaviour-preservation is the soak golden hash (`make -C sim soak`, see
`sim/README.md`); this harness is the deeper check behind the "~45.5M-sample
lockstep run, no divergence" claim in `docs/accuracy/type-split-review-guide.md`.

## What it does

Drives **two Verilated models in lockstep**, both extracted at run time from git
history: the reviewed split integration tip (`2d4f880`, `rtl/CRTC.v` + engines)
and its pre-split reference (`418aa68:rtl/UM6845R.v`, module renamed to avoid a
clash). Pinning both sides keeps this a reproducible provenance check after later
intentional CRTC behaviour changes. It uses an identical stimulus schedule (seed
`0xaccc5eed20260822`, 150000 events per CRTC
type: random register writes at arbitrary phases incl. CLKEN/nCLKEN-aligned,
held writes, reselects, snapshot loads, live type round-trips, resets; idle
ticks between events). After **every CLKEN edge** it compares a full state
snapshot of both models via `memcmp`: all pins (MA, RA, DE, HSYNC, VSYNC,
CURSOR, FIELD, DO) plus hcc, line, row, c5, in_adj, line_last_r, row_last_r,
frame_adj_r, field, crtc1_adj_from_row0, both engines' private latches
(8 arbitration regs + holdoff latch + the type-1 `r6_border_condition` latch),
status bit 5, VSYNC_r/vde/vde_r/vsync_allow/hde/hsc and both video-pointer
registers. First mismatch aborts with a state dump and a 40-edge history.

## Provenance

- Original development run: 2026-08-22 ~23:10 local, split tree at commit
  `27efc2d` (pre-rename) vs reference `418aa68`; result "no divergence",
  45,498,863 CLKEN comparisons. Harness was session-temporary; this directory
  is its durable form.
- Fresh pinned rerun against base `accc-review-and-fixes` @ `2d4f880`
  (renamed wrapper): captured verbatim in
  `docs/accuracy/evidence/split-differential-run-2026-08-23.log` — identical
  sample counts (the stimulus is deterministic), same result.
- Input hashes for the rerun are recorded inside that log file.

## Honest caveats

1. This harness's event schedule **mirrors but is not byte-identical to** the
   committed soak in `sim/sim_main.cpp` (e.g. read operations do not drive the
   bus here). Same seed and event budget; different trajectory — the two runs
   are complementary, not duplicates.
2. Both compared revisions exist in git history (`418aa68` and `2d4f880`);
   nothing checked in depends on them at build time — `run.sh` extracts them on
   the fly. This reproduces the reviewed split-equivalence evidence; it does not
   compare later intentional behaviour changes with the unsplit core.
3. Pass criteria are equality of the sampled fields only. The listed sequential
   state is sampled on both sides, but unlisted combinational or temporary
   internal wires could theoretically differ without detection if they do not
   affect the sampled projection.

## Run

```sh
tools/split-differential/run.sh                    # defaults: 418aa68 vs 2d4f880
tools/split-differential/run.sh <ref> <split>      # override either frozen side
```

Exit 0 = no divergence over both type phases (~45.5M comparisons, ~15 s).
