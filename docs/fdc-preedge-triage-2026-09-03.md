# u765 pre-edge triage (leaf only, no CPU prerequisite) — 2026-09-03

Base: `08c1596` (`docs: finalize reviewed Plus READY handoff`).
Worktree: `/private/tmp/amstrad-fdc-preedge-20260903` (disposable).
Role: sole writer, no delegation. No commit/branch/integration/push.
Stash intact: `stash@{0}` (`wip-fdc-b3-capture-2026-09-01`) was listed before
and after; never applied, never dropped. The six tests were recovered from
`.coord-inputs/stashed-u765-tests.patch` (read-only input) via `git apply`
into the working tree only.

Owned writes: `rtl/u765/u765_tb.cpp` (production-shaped test added post-triage;
see `docs/fdc-preedge-review-2026-09-03.md`) and this report.
Scratch: `.coord-inputs/` (untracked). No production controller / wrapper /
CPU / Plus / build edits. P10/Amstrad sources were read for bus-contract
comparison only, never executed or edited.

Read: `CLAUDE.md`, `AGENTS.md`, `docs/ci-testing-policy.md`,
`.coord-inputs/global-testing-policy.md`,
`docs/fdc-recovery-2026-09-03.md`,
`.coord-inputs/stashed-u765-tests.patch`.

## Baseline reproduced (HEAD, clean)

CWD `rtl/u765` (the Makefile's CWD; `test.dsk` resolves there):

- `make test > .coord-inputs/baseline-u765-run.log 2>&1` → exit `0`
  (`.coord-inputs/baseline-u765-exit.txt`), direct capture, no pipe.
- `Summary: 6 passed, 0 failed`: `u765_reset_cancels_and_restarts_mount_request`,
  `u765_writes_ignore_a0`, `u765_reset_after_ack_fall_uses_history`,
  `u765_mount_recognizes_edsk`, `u765_read_data_survives_reset_during_active_request`,
  `u765_short_reset_waits_to_reload_trackinfo`.
- Full log: `.coord-inputs/baseline-u765-run.log`.

## Recovered runs (experimental, not retained in the tree)

Applied `.coord-inputs/stashed-u765-tests.patch` to `rtl/u765/u765_tb.cpp`
only (`git diff --stat`: 1 file, +251). Two runs, both direct capture:

1. Abort-on-first-failure (as stashed):
   `make test > .coord-inputs/recovered-u765-run.log 2>&1` → make exit `2`
   (`Error 1`). Baseline 6 PASS, then
   `FAIL u765_read_data_stages_payload_pre_edge — cycle 65609: READ DATA first
   payload byte staged before clock edge: expected 33, actual 0`.
   The remaining five pre-edge tests never ran.
2. Continue-on-failure (temporary runner tweak in the owned test file only:
   `run()` counts failures instead of rethrowing; summary `passed/failed`;
   return nonzero if any failed — preserved in
   `.coord-inputs/tested-u765-preedge.patch`):
   `make test > .coord-inputs/recovered-u765-run-all.log 2>&1` → make exit `2`
   (binary exit 1). Baseline 6 PASS, then all six pre-edge tests FAIL:
   `Summary: 6 passed, 6 failed`.

| case | result | detail |
|---|---|---|
| `u765_read_data_stages_payload_pre_edge` | FAIL | cycle 65609: first payload byte staged pre-edge: expected 33 (`0x21`), actual 0 |
| `u765_read_data_results_pre_edge` | FAIL | cycle 3313: result byte 0 staged pre-edge: expected 64 (`0x40`), actual 0 |
| `u765_sense_interrupt_status_no_pending_pre_edge` | FAIL | cycle 44: no-pending byte staged pre-edge: expected 128 (`0x80`), actual 0 |
| `u765_sense_interrupt_status_pending_pre_edge` | FAIL | cycle 3866: pending ST0 staged pre-edge: expected 32 (`0x20`), actual 0 (second byte never reached) |
| `u765_sense_drive_status_pre_edge` | FAIL | cycle 60: ST3 staged pre-edge: expected 32 (`0x20`), actual 0 |
| `u765_invalid_command_pre_edge` | FAIL | cycle 44: invalid ST0 staged pre-edge: expected 128 (`0x80`), actual 0 |

Every failure is the same shape: pre-edge sample sees `m_data == 0`
(power-on value; `m_data` has no reset assignment in `rtl/u765/u765.sv`).
Full logs: `.coord-inputs/recovered-u765-run.log`,
`.coord-inputs/recovered-u765-run-all.log` (+ `-exit.txt` / `-exit-all.txt`).

## Sampling: why pre-edge differs from the passing helper

- Baseline `read_data()` (`rtl/u765/u765_tb.cpp:105-114`): asserts `nRD=0`,
  runs `fdc_step()` (a `ce` edge), *then* samples `dout`. The controller has
  already observed the `rd` edge and loaded `m_data`.
- Stashed `read_data_pre_edge()` (`.coord-inputs/stashed-u765-tests.patch:9-19`):
  asserts `nRD=0`, runs `eval()` only (no clock), samples `dout` *before* any
  `fdc_step`, then steps and releases. `status()` (`u765_tb.cpp:89-93`) is also
  `eval()`-only, so the `wait_until(RQM&DIO)` gate is honest: the byte is
  awaited, then sampled pre-`ce`.
- So the six tests isolate staging; the six passing tests mask it. This is a
  helper-convention difference, not by itself a hardware rule (see below).

## Source-backed bus contract (our RTL + production sampling, not uPD765 lore)

Controller side (`rtl/u765/u765.sv`):

- `rd`/`wr` are combinational (`:324-325`); `dout` is combinational
  (`:330`: `assign dout = a0 ? m_data : m_status`).
- `m_data` is loaded only inside `ce`-gated `~old_rd & rd & a0` edges, and
  `old_rd`/`old_wr` advance only on `ce` (`:550-551`). Sites: EXEC6 payload
  (`:1224-1240`, `m_data <= buff_data_in` on the edge, `RQM<=0` with it);
  `COMMAND_READ_RESULTS` (`:1464-1502`, first result byte loaded on the first
  `rd` edge, `:1469-1474`); `SENSE_INTERRUPT_STATUS1/2` (`:866-885`);
  `SENSE_DRIVE_STATUS_RD` (`:897-909`); `INVALID1` (`:1512-1515`).
  Entering a data/result phase sets `RQM`/`DIO` *without* preloading `m_data`
  (e.g. EXEC6 `:1222-1223` sets `RQM<=1`; `READ_RESULTS` `:1466-1468` sets
  `RQM`/`DIO`; `IDLE` even serves `0xff` on a stray data read, `:855-857`).
- Consequence, verified by the runs above: whoever samples combinational
  `dout` before the next `ce` edge sees the *previous* `m_data`.

Sampling side (production, read-only evidence):

- Leaf wrapper (`rtl/u765/u765_test.sv:29-30,40`): `ce` pulsed once per eight
  `clk_sys` (comment cites `Amstrad.sv`); writes force `a0` (`a0 | ~nWR`).
  Reads pass `a0` through — same contract as production for selected reads.
- Production (`Amstrad.sv:123,131,841,881-888`): `ce_u765` is 8 MHz
  (`!div[2:0]`); `a0 = cpu_addr[0] | (u765_sel & io_wr)`; `nRD/nWR` gated by
  `u765_sel & io_rd/wr`; `fdc_dout = (u765_sel & io_rd) ? u765_dout : 8'hFF`.
  For a selected read the leaf and production views of `dout` coincide.
- P10 latch (`sim/plus/p10_boot_test_top.v:602-603,628-638`): the CPU-consumed
  byte is captured combinationally at `cpu_di_latch_edge` (mid-T3,
  `cen_n && cen_pol && tstate==011 && busak`) — `u765_dout`, `fdc.m_data`,
  `fdc.m_status`, `buff_wait`, `i_bytes_to_read`.

  **CORRECTION / WITHDRAWAL (2026-09-03)**: The earlier provisional claim below
  ("Pre-edge matches our production sampling; post-edge does not") was
  chronologically and mechanically incorrect. In production, an I/O read
  asserts `nRD` at the T1->T2 transition (`T80pa.vhd:164-168`), spanning T2
  (250 ns), a Z80-mandated wait state Tw (250 ns), and half of T3 (125 ns)
  before `cpu_di_latch_edge` fires at mid-T3 (total 625 ns post-assertion;
  `nRD` deasserts at end-of-T3 at 750 ns). At 8 MHz (`ce_u765` period 125 ns),
  `u765.sv` detects `~old_rd & rd & a0` on the very first `ce_u765` edge
  (within <=125 ns) and updates `m_data`. The data bus is thus valid and stable
  for ~500 ns (4 `ce` ticks) before the CPU latches `DI`. Production NEVER
  samples `dout` instantaneously (0 ns) pre-edge.

Cross-module risk assessment:
The earlier claim of cross-module stale `m_data` reads on real CPU paths is
WITHDRAWN. Real CPU reads hold `nRD` low across multiple `ce_u765` ticks and
sample late in T3, well after `m_data` is updated. Stale reads occurred in
the stashed experiments solely because `read_data_pre_edge()` sampled
combinationally at t=0 ns with zero clock ticks.

P10 consistency, with its confound stated: the open Plus XFAIL
(`sim/plus/p10_boot_test.cpp:945-1008`) showed stored `0x00` not because of
pre-edge controller staging, but because TV80 cannot execute poll loops
(surrogate TV80 `JR` never repeats, `tv80_mcode.v:370-412,865-879`;
`fdc-recovery.md:80-96`) and straight-line reads began prematurely while
`u765` was still searching for the sector (state 9, `COMMAND_RW_DATA_EXEC3`;
`fdc-recovery.md:62-70`).

## Which tests earn their place (global testing policy)

The six stashed pre-edge tests failed for an invalid fixture premise
(zero-tick combinational sampling before controller clocking), not an RTL
defect.
- The instantaneous pre-edge expectation is rejected based on manufacturer
  timing (`.coord-inputs/upd765a-p7-read-timing.png`).
- A production-shaped test that models the true Z80 `nRD` pulse width (held low
  across multiple `ce` pulses) and samples at pulse end (`read_data_production()`)
  properly tests edge stability and single-byte consumption.

## Source staging: manufacturer evidence refutes preload requirement

The pre-edge convention in the stashed helper does not match real hardware.
Status resolved via verified primary evidence:

- **Manufacturer timing verified**: The NEC µPD765A/µPD765B datasheet Read
  Timing diagram (`.coord-inputs/upd765a-p7-read-timing.png`) explicitly shows
  the data bus (`DB0-7`) is high-impedance prior to `nRD` falling edge, and
  only becomes valid after `t_RD` (access time from `RD`).
- **RQM/DIO contract verified**: `RQM=1 & DIO=1` advertises controller readiness
  for host transfer; it does NOT place data on the bus or require internal
  registers to be preloaded before the read strobe falls.
- **Controller verdict**: No `m_data` preload change is required in `u765.sv`.
  The current edge-triggered update on `~old_rd & rd & a0` is functionally
  correct for real bus timing.
- **Classic AMSDOS full command/data regression**: Remains **REQUIRED and UNMET**,
  independent of leaf unit tests (`fdc-recovery.md:98-119`).

## Accepted vs provisional (post-review state)

- Accepted test: `test_read_data_production_shaped_first_byte()` in
  `rtl/u765/u765_tb.cpp` (uses `read_data_production()`, which holds `nRD=0`
  across 3 `ce` ticks, verifies level stability, and samples byte 0 `0x5a` and
  byte 1 `0x7f` from synthetic EDSK). Passes 7/7 leaf tests and full
  simulation/lint suites (`full-sim.log`, `full-lint.log` exit 0).
- Rejected / Withdrawn: The 6 stashed pre-edge tests and the provisional triage
  claim of a controller "preload defect" are formally withdrawn.
- Durable docs: This triage report (updated) and the comprehensive review report
  `docs/fdc-preedge-review-2026-09-03.md`.

## Concrete next discriminator (resolved & forward roadmap)

1. Preload question: RESOLVED (rejected via NEC µPD765A datasheet timing and
   T80pa pulse interval analysis).
2. Production-shaped leaf test: PASS (7/7 leaf suite, full sim exit 0).
3. Classic AMSDOS full command/data regression: Remains an explicit, open gate
   requiring real firmware execution and loop-capable CPU emulation.
