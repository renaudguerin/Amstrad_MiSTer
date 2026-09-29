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
