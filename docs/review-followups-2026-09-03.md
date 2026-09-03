# B6/B10 follow-up closure — 2026-09-03

Independent review: **CLEAR** by Gemini 3.8 Flash (high effort),
recorded in `.fdc-scratch/main-followups-gemini-review.md`.
Base: `d3aabbc` (`test: preserve shared FDC first-divergence diagnostics`)
on `accc-review-and-fixes`. Scope is uncommitted B6/B10 follow-ups only;
no executable RTL logic change.

## Closed findings

- **B6-1 — OSD label matches wiring.** The `R[32]` CONF_STR entry now reads
  `Reset & Detach Dandanator`. `status[32]` feeds `reset_base` and the SNA
  parser reset, and drives only the `.dandanator_detach(status[32])` port on
  the `CPC_Dandanator` instance. Plus cartridge detach stays tied off via
  `.detach(1'b0)` on `plus_cartridge_memory`. Key binding and status-bit
  mapping unchanged.
- **B6-2 — Mask timing lifecycle documented.** [docs/b6-architecture-decision.md](b6-architecture-decision.md)
  records that `status_menumask` is combinational on the raw
  `plus_model = status[34:33]` selection (through the `plus_model_select`
  `always @(*)` block and the `plus_menu_capability_mask` continuous assign
  to the `.status_menumask(status_menumask)` port on the `hps_io` instance),
  so visibility changes at selection time, while the classic `model` register
  only latches `menu_model` via the `else if(reset) model <= menu_model`
  assignment, applied through `Reset & apply model` (R0). No claim that the
  Plus model latches on reset; no claim about HPS/OSD hide-vs-grey rendering.
- **B10-3 — Single-owner commentary accurate.** The `page`/`combo` note
  states the only sequential writers are the `page <=` / `combo <=`
  assignments in the download `always` block, while `rom_loader_route` takes
  `.page(page)` as an `input [8:0]` and never drives either signal.

## Gates

- Parent `make -C sim`: exit 0 (`.fdc-scratch/main-followups-parent-sim.log`,
  791 lines, exact match with worker run).
- Parent lint: exit 0 (`.fdc-scratch/parent-lint-2026-09-03.log`).
- This cleanup changes only [docs/b6-architecture-decision.md](b6-architecture-decision.md)
  (leading-whitespace removal and numeric line references replaced with the
  stable signal/assignment anchors above; rule text unchanged) plus this new
  report, so no further simulation or review was run.

## Residuals (deferred, not blockers)

- **B10-1:** wrapper invalid-chunk retention contract unpinned by a
  top-level harness; deferred per test discipline.
- **B10-2:** `rom_loader_route rom_loader_route` instance-name shadowing;
  deferred.
- **OSD/hardware boundary:** `status_menumask` rendering (hide vs. dim/grey,
  row-type handling) lives in the upstream MiSTer `menu.cpp` binary and needs
  real-hardware OSD verification.
