# Accuracy milestone release acceptance · 7 November 2026

The staged [v2026.11.07 draft](https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-ee9c07a27e573000d3bb) targets tested commit
`354388dd118a3982237b76ae15a63bd0d0fbfd29`. It contains the accumulated classic/GA and Plus fixes,
including CRTC3 compatible IRQ handling. V1 and optional RTL audit reductions
are excluded. Master is unchanged; no public release has been published.

## Exact artifact and gates

[CI42327881289](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42327881289)
is fully successful, including production-T80, selected simulation and full
Quartus 17.0.2 synthesis. The reviewed local selection passes 47 benches. The
checked-in production RTL and Quartus settings match hardware-qualified
`ae8bc6c`; `10e905c` changes only the test contract, and `354388d` adds handover
documents.

`Amstrad_20261107.rbf` is 3,124,560 bytes, SHA256
`78e063ef130bff543b5b81d6b541725cfc7e00a80be86ddc2a8623a22664073b`. The setup minimum is +0.671 ns and hold minimum +0.241 ns
across seven clocks each, with zero TNS. Fit: 24,623 ALMs (59%), 28,836 registers,
102 RAM blocks, 35 DSPs. No timing bypass or weaker constraint is used.
The RBF, full synthesis reports ZIP and SHA256SUMS are attached; all three
uploaded files were downloaded and checked byte-for-byte against staging.

## Final-bitstream native acceptance

All six CRTC3 checks succeed, producing 12 inspected native PNGs with verified
RBF/media hashes and matching original CFG restoration in every completed run.

| Check | Result |
|---|---|
| Original SHAKER BR01F0 |48/48 opcode/actual-result pairs match the real Plus |
| Original SHAKER BR01F1 |18/18 timing triples match, including 54 numeric fields |
| MRER R3=08/0C |128/128 P/T/D records match; repeat differences `00`, no `FFFF` |
| VS31/32/33 |All 9 IRQ counts match 0/0/64, three repetitions; D00; LP retained but ungraded |
| DSC4 |All four credit colons/names clear; first main image and later fire bands present. Credits and first main frame are RGB-identical to the accepted fdf control |
| Amazing |Logo/rupture bands and lower text/effects present in both sampled phases |
| Before the Storm |Landscape/scroller scene present and progressing in both sampled frames |

MRER/VS use exact source-font HEX decoding; BR panels round-trip all 2,000 cells
per image without unknown glyphs. Actual measured fields are compared with the
user's physical Plus transcription, separately from SHAKER's printed expected
fields. Native captures omit OSD/runtime-latch readback and analogue output;
CFG settings are requested/write evidence. The asynchronous demo samples do
not independently establish alternating-frame wobble, full-demo completion or
physical HDMI/CRT behaviour. The user's broader manual CRTC0/1/3 acceptance
remains separate from these exact-build CRTC3 checks.

One initial BTF attempt stopped before its 370 s wait because the runner's
normal bound is 300 s. The successful isolated retry uses `--max-wait 450`; earlier
successful results were preserved. This was a runner setup failure, not an
observed core defect.

## Residuals and retained work

The tally remains 377 passing / 6 broken / 1,004 unknown / 3 stale over 1,390 pages.
The six known broken pages are C0E6/0409, C0B6/019D, C1B6/019D, C1A4/0037,
C1AI/008D and C1S/02D3. It is not a fresh complete baseline. The historical
`fdf348c` 1,384-image corpus is not repackaged as a new current-build archive.

The [handover](../session-logs/release-handover-2026-11-07.md) preserves the V1,
RTL-audit and dirty S-observer worktrees, proof limits and next actions. Private
release receipts, commands, all 12 PNGs and manifests, raw comparisons and upload
verification are in local/release-preparation-20261107/ of the c1e6 checkout.
