# FDC diagnostics recovery (evidence only) — 2026-09-03

Base: `22ad766`. Scope: recover the preserved experiment's **early observe-only
first-divergence diagnostics** into the P10 harness. No controller RTL, CPU,
interrupt/halt/JP, or stop/resume-workaround changes. The experiment's later
synthetic stop/resume program is deliberately **not** recovered.

Preserved experiment (read-only, not applied):
- `.fdc-scratch/muse-experiment-2026-09-03.patch`
- `.fdc-scratch/muse-experiment-2026-09-03.log`
- Prior harness states in `.fdc-scratch/` (`p10_boot_test_stash.cpp`,
  `p10_top_stash.v`, `tb.diff`, `top.diff`, `u765_tb_stash.cpp`).

## What changed (observe-only, prints nothing asserted)

- `sim/plus/p10_boot_test_top.v:124-126,619-621,636-638`: three new
  `cpu_di_latch_edge`-sampled taps — `dbg_cpu_di_m_data` (`fdc.m_data`),
  `dbg_cpu_di_buff_wait` (`fdc.buff_wait`), `dbg_cpu_di_bytes_left`
  (`fdc.fdc.i_bytes_to_read`). Hierarchy verified against
  `rtl/u765/u765.sv`: `buff_wait` (line 168) and `m_data` (line 328) sit
  outside the named `: fdc` block (line 342), so `fdc.buff_wait` /
  `fdc.m_data` are correct; `i_bytes_to_read` (line 398) sits inside it, so
  `fdc.fdc.i_bytes_to_read` is correct. (The experiment itself first tried
  `fdc.fdc.buff_wait`, failed elaboration, and corrected to `fdc.buff_wait` —
  that lesson is applied from the start here.)
- `sim/plus/p10_boot_test.cpp:41-56` (struct fields + capture) and `:870-932`
  (print-only `DIAG polls / shift / result-slots` block, result slots raw and
  phase-unverified, no `st1_overrun` label) plus the XFAIL line
  fingerprint (`m_data/buff_wait/bytes_left`, print only).
- The Plus FDC XFAIL (`:990-993`) is byte-identical in strength: XPASS still
  fails the suite, and any non-zero first-mismatch shape still fails.

## Baseline reproduced (HEAD, before and after this change)

Command (run from `sim/plus`, the Makefile's CWD; running from the repo root
breaks the `../../rtl/u765/test.dsk` relative load — fixture evidence, not a
failure):

- `./obj_dir/p10_boot/p10_boot_tests` → exit 0 via parent direct gates
  (provenance: `.fdc-scratch/parent-sim-2026-09-03.log`,
  `.fdc-scratch/parent-lint-2026-09-03.log`). The old pipelined scratch
  echo (`... | tail ...; echo "EXIT:$?"` in
  `.fdc-scratch/muse-experiment-2026-09-03.log`) captured tail's status,
  not the harness, and is not exit evidence.
- `trace checkpoint: mount_sd_reads=1 total_sd_reads=3 last_lba=1 pending_rd=0
  pending_lba=1 fdc_writes=9 fdc_reads=1047 fdc_cpu_latches=1047 state=13
  msr=f0 seek=3e1 dirty=0 sector_pos=15b byte_count=27 results=6f/20/20`
- `XFAIL fdc-payload-poll: production-clock TV80 stored 0x0 instead of 0x21
  at payload byte 0; exact CPU latch saw selected u765 data/state/MSR=0/9/50
  after status latch=50/9` (+ `m_data/buff_wait/bytes_left=0/0/0` after this
  change).

## Recovered diagnostics output (post-change run, exit 0 per parent direct gate; see Gates)

- `DIAG polls: data_blocks=519 blocks_with_spin(>1 poll)=1
  max_polls_per_block=10 total_latches=1047`
  (519 = 512 payload + 7 result data latches; 1047 = 519×2 + 9 command-phase
  status polls. The initial 10-count block is the 9 command-phase MSR checks
  plus the first data byte's own check — not a repeated spin. Each of the
  subsequent 518 data reads sees exactly 1 preceding status check — i.e.
  **no poll loop ever spins**.)
- `DIAG shift: payload[0]=0x0 disk[0x200]=0x21 shifted_bytes_matching=124/511
  last_disk_byte=0x20`, first deviations shown all `got 0x0`.
- **One-byte-shift hypothesis does NOT fit**: only 124/511 bytes match the
  shifted expectation, which refutes a pure one-byte shift. That ratio does
  NOT imply all deviations are zeros or that matches occur only at zeros:
  sector 0x41 holds only 26 zero bytes in total, so 124 matches cannot arise
  from matching disk zeros. The first 8 printed deviations were `0x00`
  because the unrolled reads began before `u765` completed sector search
  (state 9, `COMMAND_RW_DATA_EXEC3`); `results_shifted_by_one=0`.
- `DIAG result-slots (raw, phase-unverified): 6f/20/20/20/20/20/61
  results_shifted_by_one=0` — the 7 consumed bytes are sector payload
  474..480 (`"o     a"` from `"e 30Ko     alors bon"`), read while the
  controller was still streaming sector data in state 13
  (`COMMAND_RW_DATA_EXEC6`), before `COMMAND_READ_RESULTS` (state 42). They
  are NOT valid ST0/ST1/ST2, and the computed `st1_overrun` label is removed:
  overrun is UNKNOWN from this trace. Raw bytes are retained; no status bit
  is asserted.

## Surrogate CPU limitation (fixture evidence, NOT an FDC hardware rule)

Verified in current sources:

- `sim/plus/tv80/tv80_core.v:69,118`: `jump_e` is wired from the microcode
  but `:369` advances PC only on `jump` (`if (jump) pc <= wz;`) — `jump_e`
  goes nowhere.
- `sim/plus/tv80/tv80_mcode.v:370-412`: DJNZ, JR e, and JR cc,e assert only
  `JumpE_r` → **JR never repeats**.
- `sim/plus/tv80/tv80_mcode.v:865-879`: JP cc,nn asserts `Jump_r`
  unconditionally → **JP cc never falls through**.

Consequence: fixture MSR-poll loops (`JR Z/NZ,poll` in
`sim/plus/p10_boot_test.cpp:779-824`) execute exactly once and fall through
regardless of RQM — proven by the `DIAG polls` histogram above. The
production VHDL T80 implements these branches, so real AMSDOS polling is
unaffected and **no controller change is justified by their absence here**.

## What remains UNMET (required, not claimed)

- **Classic AMSDOS full command/data regression: REQUIRED and UNMET.** This
  fixture issues the exact 9 AMSDOS READ DATA command bytes and consumes
  512+7 bytes through the production decode/bus, but it is a straight-line
  synthetic sequence, not an AMSDOS ROM boot: no firmware ROM is present,
  and the reduced TV80 cannot execute poll loops (above). Polling behaviour
  of actual AMSDOS therefore cannot be claimed from this harness.
- **Plus hardware boundary**: Plus-path decode/motor/media request pass;
  payload + result correctness through the Plus bus is open (same XFAIL
  shape; Plus twin of this fixture not added — out of scope).
- Evidence tiers, kept distinct: (1) standalone `rtl/u765` leaf suite — 6/6
  pass, direct controller, no CPU/bus (`make -C rtl/u765 test`, 6/6, covered
  by the parent sim gate); (2) P10 synthetic decode/command/media — passes (9 command
  bytes, SD LBA 1, motor); (3) full command/data regression — unmet.
- The six stashed pre-edge `u765_tb.cpp` tests remain **unaccepted**,
  preserved read-only in `stash@{0}` (`0fe18a4513a47e4f21e0f504f002673a853388c3`,
  NOT applied): `test_read_data_stages_payload_pre_edge`,
  `test_read_data_results_pre_edge`,
  `test_sense_interrupt_status_{no_pending,pending}_pre_edge`,
  `test_sense_drive_status_pre_edge`, `test_invalid_command_pre_edge`. No
  controller defect is justified by any of them.

## Smallest future acceptance boundary (identified, NOT implemented)

Flip the existing XPASS tripwire honestly: `first_payload_mismatch == 512`
(zero of 512 payload bytes diverge) **plus** the already-present clean-path
requires (status/data latch integrity, `stored == cpu_di`) extended to all
512 payload and 7 result bytes with the valid-phase ST1 overrun bit clear
(only once the result phase is actually reached — not computable from the
current phase-unverified slots) — at which point the
XFAIL block (`:990-1003`) is deleted in the same change. Anything weaker
(e.g. first-byte-only equality) would re-hide the current failure mode.

## Gates

- `make -C sim` → green (parent direct execution, exit 0;
  `.fdc-scratch/parent-sim-2026-09-03.log`; includes the `rtl/u765` 6/6 leaf
  suite and the full P10 harness with the new DIAG output).
- `make -C sim lint` → green (parent direct execution, exit 0;
  `.fdc-scratch/parent-lint-2026-09-03.log`).
- Old pipelined scratch echoes (`| tail ...; echo "EXIT:$?"`,
  `MAKE_EXIT:0`/`P10_EXIT:0` strings) are not exit evidence and are not
  cited as provenance.
- No new test mirrors code: the DIAG block prints only and asserts nothing;
  if it ever fails a gate, revert it alone — do not pursue a functional fix
  in this lane.

## Coordinator acceptance — 2026-09-03

The final [independent Gemini review](fdc-diagnostics-review-2026-09-03.md) is CLEAR. The coordinator reran `make -C sim` and `make -C sim lint` on the final diagnostic diff; both process exits were 0. Controller RTL is unchanged. Classic AMSDOS command/data validation remains required and unmet; the Plus payload XFAIL remains open.
