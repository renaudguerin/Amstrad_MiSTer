# Independent review debt

This file tracks work that was merged without the independent cross-provider review the
project normally requires. It exists because that review was unavailable at the time, not
because the work was judged low risk. Clear entries from this list only after a real
independent review has run; do not clear them because a later change touched the same file.

Cleared rows, the original 2026-08-22 per-commit review record (action items A1-A5), the
branch-level review register and the pass-3 triage are archived verbatim in
[archive/review-debt-cleared.md](archive/review-debt-cleared.md). Move a row there, unchanged,
once a real independent review clears it.

## Rule for future unreviewed work

New commits merged without independent review get a row under "Open debt" below in the same
commit that introduces them. Name what a reviewer should look at hardest. A change that is both
high risk and cheaply deferrable should wait rather than grow this list.

## What this does not excuse

- The Verilator suite and the GitHub Actions synthesis job still gate every commit. A red
  gate is a blocker regardless of review status.
- Hardware-test results are still the authority over simulation results.
- A finding must still have a deterministic regression test before its behaviour is changed.

## Open debt

Newest first. A row stays here until a real independent review clears it; source-review
clearance never closes a hardware gate, which is tracked in `backlog.md` and
`implementation-roadmap.md`.

**Per-change simulation selection and one-run gate policy, 2026-09-15 — REVIEWED BY SPARK
(Muse Spark 1.3 xhigh, run `20260915T120117Z-43757-1bbc`), declined findings open:** Opus
wrote the parallel wrappers in `sim/Makefile` and `sim/plus/Makefile`, the bench index
`sim/TESTS.md`, the selector `sim/select_tests.py`, the CI simulation step that runs it, the
6-boot default for `b6_video_boundary_test.cpp` (`--strict` keeps all 18), and the gate rules
in `CLAUDE.md`, `AGENTS.md`, `docs/ci-testing-policy.md` and `stream-finish`. Look hardest at:
benches a change can break but the selector skips (Covers globs are hand-written; the safety
net only catches RTL no row covers); the CI base choice for pushes, pull requests, new
branches and force pushes; `make -n` dependency parsing across recursive makes; sub-makes never
sharing a directory under `-j`; and whether dropping classic type 1 and HSYNC width 14 from the
default B6 run hides a real output-chain difference. Spark found no Makefile, parser or CI-base
bugs and confirmed no B6 assertion lives only in the dropped cases. Accepted: same-area Covers
for the output chain, GA lockstep diff, SDRAM boot, PPI/PSG/DMA and motherboard rows; strict-B6
guidance. Declined by design, because selection follows what a bench asserts rather than what it
compiles: classic CRTC/GA changes do not select Plus integration benches (p1, p10, B6-B8); an
index or selector edit selects no benches, and a `sim/TESTS.md`-only push skips CI (`**/*.md`
ignore) until the next code push runs `--check`; pull requests select from the PR head while
testing the auto-merge tree (integration goes through local merges and pushes, not PRs).

**Plus PRI 9-bit line compare, 2026-09-13 — UNREVIEWED:** `rtl/plus/asic_ga_timing.v`
`pri_line_match` now requires bit 8 of `{VC5..VC0, RC2..RC0}` to be 0, following the
Arnold-revision §2.4 formula `0 PRI7..PRI0 == VC5..VC0 RC2..RC0`. The previous
don't-care fired PRI=&37 again on line 311, which loaded Copter 271's title sky palette
55 lines early (MiSTer captures versus AmSpirit, both under ignored
`local/test_media/defects/copter271/`). `sim/plus/asic_pri_test.cpp` pr02 was
rewritten from the old n/n+256 expectation and failed on the old RTL before the fix;
pr03 now picks a line with bit 8 clear. Look hardest at: whether any other consumer of
`crtc_line` relied on the alias (SPLT is a separate 8-bit compare in `asic_video.v` and
is unchanged); the rejected CPCWiki "PRI=10 also fires at 266" claim, which only
hardware can finally settle; and the device recapture, which remains pending an RBF.

**B18 slice 4c, top-level save wiring, 2026-09-14 — REVIEWED BY GEMINI ONLY, NO SIMULATION OF
THE TOP LEVEL:** the parent (Opus) wrote the `Amstrad.sv` wiring, `rtl/sna_cart_mux.v` and
stream case 7; Gemini wrote `scripts/hardware-loop/sna_pull.py`. Gemini (gemini-3.8-flash-high)
reviewed the whole diff and found nothing blocking; its one low finding, the `--wait` usage
order, is fixed in the design doc. Gemini reviewed its own pull script, so that part is not
cross-provider, and Astra had no quota for this slice. `Amstrad.sv` is checked only by Quartus
synthesis. Look hardest at the admission and abort terms (`save_admit`, `save_abort`) against
every download and overlay path, and at `sna_cart_mux`'s two-edge drain against `sdram.v`
arbitration if its `clkref`/`q` alignment ever changes.

**B18 slice 4, freeze controller and save stream, 2026-09-13 — REVIEWED, FIXES UNREVIEWED:**
Astra high reviewed `rtl/sna_save_capture.v`, `rtl/sna_save_stream.v`, `rtl/sna_ddr_mux.v` and
their tests (Gemini-authored, parent-fixed) and returned CHANGES REQUIRED. The parent fixed all
three findings without a second review:
- mux ownership kept through reset while a write is outstanding;
- generation consumed when the final write is issued;
- 64K page decode in the test.

Each fix has a case that its revert fails. Look hardest at the interaction between the stream's
`S_QUIESCE` path and the mux release condition when the stream and mux resets differ in 4c
wiring. Also check that Case 6E really lands the reset on the acceptance clock.

**B18 slice 3, observation ports, 2026-09-13 — PARTLY REVIEWED:** the port and shadow RTL was
written by Gemini and reviewed by the Opus parent, so it is cross-provider. The parent's own
fixes were not independently reviewed: the `u765` one-clock `pcn` copy, the `sna_hw_header.v`
integer scope, and the `t36a_b18_vsw_elapsed_counter` vector. Look hardest at whether
`vsw_elapsed` in `rtl/CRTC.v` truly shadows every `vsc` assignment, including the R7-write load
outside the `CLKEN` branch; and at whether t36a's mid-line samples can miss a one-line offset.
The formatter's byte mapping is unverified until the slice 4 capture fixture exists.

**B4 phase 1 original fetch-provider review/evidence, 2026-09-12 — OPEN:**
The revised detector, ring reader, recorder and top-level SSM connections are covered
by the "B4 revised SSM path" source review (CLEAR, now in the archive). That does not discharge the original
`rtl/Amstrad_motherboard.v` raw-fetch-provider boundary or production T80pa evidence.
The existing tests exercise T80pa-shaped TV80 fetches and synthetic held fetches;
the production T80pa netlist still needs a suitable mixed-language gate.

DDR allocation, verified 2026-09-13 on the device: the kernel boots with
`mem=511M memmap=513M$511M`, and `/proc/iomem` lists System RAM only at
`00000000-1fefffff`, so Linux never owns the ring's 1,040 bytes at `0x30000000`. The
framework scaler buffer is `RAMBASE 0x20000000`, `RAMSIZE 0x00800000`
(`sys/sys_top.v`), ending well below the ring. Main's own `/dev/mem` mappings were not
enumerated; the ring sits in the window MiSTer reserves for core DDR use. Address units
are 64-bit words in the source interface. New logic on `clk_sys` and the DDR port still need synthesis/timing and
real-media validation. Source passivity and green simulation do not close those gates.

**B4 phase 0, CSL runner, 2026-09-12 — UNREVIEWED:** `scripts/hardware-loop/csl_runner.py`,
`scripts/hardware-loop/cpc_keys.py` and their 51 offline tests were written and
gated by the parent alone; no cross-provider review was available. Host-only
Python, no RTL. Look hardest at: the power-on fold, where configuration and
media commands on both sides of a `reset` are bound to one core load and a
later `crtc_select` is judged as a live change instead (a mis-scoped window
would silently run a SHAKER module under the wrong CRTC); the CFG read/modify/
restore path, which writes to the user's real `config/Amstrad.CFG` and must
leave it byte-identical; and the derived `CPC_KEY_TO_LINUX` table, of which only
15 entries are confirmed against the B2 device capture, the rest being read back
from `rtl/hid.sv` through the standard PS/2 set-2 encoding. The corpus test
covers exactly the characters the 25 bundled SHAKER scripts use, so an
unconfirmed entry outside that set would not be caught.
