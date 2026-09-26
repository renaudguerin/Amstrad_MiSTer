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

**CRTC3 Plus acknowledge shaping and cross-line PRI (`codex/plus/crtc3-demo`),
2026-09-26 — CROSS-PROVIDER REVIEW UNAVAILABLE:** Gemini retries require OAuth
and time out; Opus5.5-medium run `20260925T235748Z-2596-a76a` exits with provider
signal9 after687.9 seconds without review output. User-authorized native
Astra-medium fallback does not discharge cross-provider debt. Look hardest at
raw versus ASIC-side IORQ routing (including FDC aliases), repeated empty-vector
scope within M1, reset/snapshot qualification of the added PRI line-entry event,
and the simultaneous raw-HSYNC-fall/line-entry seam. Preserve ordinary monitor
trailing-edge timing, nine-bit PRI no-wrap and pending/acknowledge behavior.
Evidence and acceptance limits are in the
[CRTC3 record](investigations/hardware-runs/crtc3-demo-2026-09-25.md).

**TV80 automatic I/O wait (`plus/ga-fast-write-latch`), 2026-09-22 — UNREVIEWED:** Opus made
`sim/plus/tv80/tv80_core.v` hold T2 for one wait state on I/O cycles (the existing, previously
dead `IOWait` parameter) and removed the P10 fixture's `production_wait` switch. Sim-only; no
RTL change. Measured IORQ windows now match the production T80pa exactly
([investigation](investigations/no-wait-ga-write-latch-2026-09-22.md)). Look hardest at:
whether the wait also applies where it should not (interrupt-acknowledge M1 is excluded
through `iorq`; block I/O instructions and the WAIT-stretch path through the wrapper's
`cen_pol`), and the resulting `b6-dynamic` type-1 failure (backlog B22).

**Plus cartridge stall released at SDRAM admission (`plus/sonic-cpu-cart-latency`), 2026-09-22 —
REVIEWED BY WORKHORSE TIER ONLY:** Opus wrote the RTL and the `d5-cart-timing` vector. Astra high
was requested twice but Codex hit its usage limit (runs `20260922T095830Z-36234-3fa4`,
`20260922T143147Z-83199-8563`; resets 2026-09-28), so Sol and Astra were unavailable. Muse Spark 1.3 xhigh (run `20260922T095955Z-38975-6693`) and Gemini 3.8 Flash
high (run `20260922T095958Z-39149-44d4`) reviewed `56771ff..d06972d` and found no blocking issue;
Muse's two comment-accuracy points (sdram `q` resync after configuration, watchdog after a grant)
were fixed in comments only. Not reviewed by either: the later P10 fixture edits (`production_wait`
input for the video-coherence test, since removed by `plus/ga-fast-write-latch`; the stall-run pin re-derived as 4 ticks). A Sol or Astra pass
should still look hardest at: any path that drops `cart_stall` while `cart_dout` is stale at the
T80pa latch (grant of a discarded/cancelled read, `sna_cart_mux` handover, `sdram.v` resync);
the budget (grant I, data at I+9, earliest latch I+11; `p0_boot_tests` measures data 7 clocks
after release); and the derivation of the `d5-cart-timing` 64/192-tick expectations. Evidence:
[cart-wait record](investigations/sonic/cart-wait-2026-09-22.md).

**ACCC section lookup tool (`scripts/accc/`), 2026-09-22 — REVIEWED (MiMo v2.6 Flash, run
`20260922T092939Z-5423-8625`), CHANGES REQUIRED, all 13 findings fixed by Opus; the fixes
are not re-reviewed:** Opus wrote `lookup.py` and `run_eval.py`; Gemini added round-2 eval data,
`--set` and the bilingual claim check. The review found silent text loss and page drift in the
parser, FR-only sections dropped by the merge, and several failure paths that crashed or called
the API unasked. Look hardest at: the widened `FOOTER` and `TABLE_HEADING` patterns (could one
swallow a body line?), `follows` accepting untitled headings, and the known residue (EN "9
GATE ARRAY" text sits at the end of 8.3; FR 3 and 3.1 and EN 11.2.2 are absent from their
edition; EN p21 and FR p280 footers are unmatched). Tooling only; no RTL or sim impact.

**ACCC page-anchor migration to the v1.11 re-issue, 2026-09-21 — REVIEWED (Gemini 3.8 Flash
high, run `20260921T175020Z-80016-18b2`; Astra low, run `20260921T175020Z-79905-dd1b`), both
CHANGES REQUIRED; round-2 re-review (runs `20260921T180739Z-90193-140e`, `20260921T180739Z-90182-2e61`)
confirmed the round-1 fixes and found more stale citations, all fixed; round-3 Astra review
(run `20260921T182232Z-98421`) found nine more small items, fixed; round 4 (both reviewers)
confirmed every round-3 fix and found seven residual occurrences, fixed; Gemini round 5 (run
`20260921T215820Z-24363-180c`) confirmed those and found three more, fixed:** Opus shifted
about 1,000 English page anchors (+1 from first-print p.29) and renumbered first-print English
§14.4-14.8 to §14.5-14.9 across live docs, RTL and sim comments; only digits and chapter-14
section numbers changed, and the selected benches pass. Look hardest at: unlabelled anchors that
were really French (French pagination did not change, so a shifted French anchor is now wrong;
ten were caught and reverted, IA-series citations are the riskiest); anchors inside multi-page
sections, where the rule relied on the anchor predating the re-issue rather than on the section
map; and the follow-up that converted the remaining v1.10-labelled page citations to v1.11
re-issue pages and corrected the French §13.7.2.2 citations in `sim_main.cpp` from p.127 to
p.128, each checked against the PDF text.

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
The formatter's classic byte mapping now has independently reviewed production-T80
capture/round-trip coverage (B18 fixture, integrated 2026-09-22; see
[b18-sna-save.md](b18-sna-save.md#classic-capture-and-round-trip-fixture-2026-09-22)).
That fixture evidence does not clear the unreviewed parent fixes named above.

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
