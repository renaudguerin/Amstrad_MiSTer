# SHAKER B9 type-1 page B: discriminator brief — 2026-09-22

A scratch replay of the actual SHAKER binary on the production T80/GA/CRTC
logic reproduces the C0=3F boundary interaction and its extra 64 µs VSYNC
interval. This isolates a concrete mechanism, but does not establish the
correct hardware ordering at the collision. No RTL change or hardware
experiment was made. The [September 23 result-buffer follow-up](#result-buffer-follow-up-2026-09-23)
now connects the replay to all five displayed values in the first update-delay block.
The MID FRAME reference glyph remains disputed; its recorded residual is
retained pending clearer evidence. With no pre-fix page-B capture, neither
observation establishes a regression.

## Established residuals

From the [repaired-build hardware record](shaker-repaired-95e6f56-2026-09-22.md)
(build `95e6f56`, device disk SHA-256
`65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`):

| Page-B row | Real CPC reference | Repaired capture | Status |
|---|---|---|---|
| C9=0, C0=3F, each of three update-delay blocks | 2740 | 2780 | Established +64 µs residual (all three blocks) |
| Other update-delay rows (C0=3D/3E/00/01) | 2740, 2740, 2780, 2760 | Same | Match |
| Four MID FRAME SIZE (R6 pairs 50/50, 7F/50, 50/7F, 7F/7F) | 4F40 | 4E40 | Residual retained; reference glyph E/F ambiguous, looks closer 4F40 — do not correct or claim closed |
| Four EVEN+ODD totals | 9C60 | 9C60 | Match |

No source-derived repair is established. No proven regression.

## Binary provenance and instruction map

`SHAKE27B.BIN` extracted from `test_media/shaker/shaker27.dsk` (disk SHA-256
above): binary SHA-256
`c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88`.
AMSDOS load `&3900`, entry `&4268`. Static map only — no binary, dump or
disassembly is copied into this repository:

- R8 update-delay write: `&91E7` `OUT (C),C` (R8=3), reached through a
  calibrated NOP sled with entry points `&9FC0/&9FBF/&9FBE/&9FBD/&9FBC`
  for the C0=3D/3E/3F/00/01 points (adjacent labels 3E/3F one sled byte apart).
- Measurement loop `&91F7–&91FE`, formatted with constant `0x0470`.
- MID FRAME SIZE routine `&9266..&932B`: two VSYNC-count loops at `&92CD`
  and `&92FE`, calibrated offsets `0x0470` / `0x5270`.
- Setup written by that routine: R4=`4D`, R7=0, R9=7, R8=3; the four cases
  patch the two R6 OUT immediates with decimal 50 (`32` hex) and `7F`.

**Limits.** Static disassembly alone does not establish write acceptance.
The replay below observes that phase under its explicit entry/setup contract;
it does not prove equivalence to the complete menu-launched hardware run.

## Rejected: global stage-B removal

A retrieval pass proposed removing the stage-B C9.0 poke globally. Rejected:

- French ACCC v1.11 §19.5.3 p.211 diagram panels 18(R)/22(V)/26(Z1) (even
  frame, R9 odd, C9 initially 1) show C9=1 at C0=5 and C9=0 at C0=6 — stage B
  does change C9 in documented panels. Bench t21 matches
  ([sim/sim_main.cpp](../../../sim/sim_main.cpp) ~5995–6004).
- French §19.5.3 p.210 places the third/fourth microsecond relative to the
  `OUT` on R8, not to an arbitrary unit write.
- French §19.8.2 p.226 line counting does not resolve stage-vs-boundary
  priority when both coincide.
- [rtl/CRTC.v](../../../rtl/CRTC.v) 362–369: `line_new` wins the poke on the
  shared edge; the source marks that coincidence explicitly unpinned. t21
  keeps both stages mid-line and never exercises it.

Also rejected: a never-reset C9 assertion and a claimed 157-line frame-origin
oracle derived from the calibrated VSYNC interval. No reinterpretation of the
MID glyph is accepted; the recorded residual stands.

## Executed production CPU/bus diagnostic

The existing `sim/crtc_t80_top.sv` fixture had 256-byte RAM and no PPI VSYNC
input. A scratch copy widened RAM to 64 KiB, routed PPI reads to raw VSYNC,
and added observation ports. Production T80pa, GA divider/WAIT, CRTC wrapper
and rule engines were unchanged. GHDL 6.0.0 generated the T80pa netlist;
Verilator 5.052 built and ran three points of the authentic driver at `9139`.
The full header-stripped payload was loaded at `3900`, with `BF03=1`.
Per-point setup, self-modifying sled, synchronization and delay helpers all
executed from that payload. These helpers are inside the module, not missing
firmware. No baseline suite or assertion oracle was involved.

| Label | R8 bus-write start C0 | Stage A result | Stage B result | Last raw VSYNC interval |
|---|---|---|---|---|
| 3D | 61 | C0=62 | C0=63 | 643072 ticks / 157 lines |
| 3E | 62 | C0=63 | C0=0 boundary, C9=2 | 643072 ticks / 157 lines |
| 3F | 63 | C0=0 boundary, C9=1 | C0=1, C9=0 | 647168 ticks / 158 lines |

Each write starts 152 master ticks after opcode fetch at `91E7`; the toggle
arms one tick later. Thus stage A meets the line boundary specifically in the
3F case. Stage B then resets C9.0, unlike the 3E case where stage B itself
meets the boundary. The extra interval is 4096 ticks = 64 µs, matching the
observed discrepancy's magnitude. The final VSYNC rise precedes each point's
return fetch at `9211` by the same 7471 ticks.

**Evidence limits.** The driver starts at `9139`, bypassing menu-level setup;
initial SNA registers and simplified PPI input are fixture assumptions. Its
synchronization does not prove identical hidden parity/history to a complete
hardware launch. Logged states resolve the collision for this fixture. The
157/157/158 intervals are raw VSYNC observations, not recovered output strings:
the payload's formatted result buffer was not inspected. Post-evaluation logs
also do not directly sample the pre-edge poke pulse; stage transitions, C9
changes and unchanged RTL establish that mechanism together.

Private evidence is retained under ignored
`docs/screenshots/shaker-b9-page-b-2026-09-22/replay/`: scratch top/driver/build
script, payload, observations and `b9_run.log` (SHA-256
`d93adf5bf4061301cf639af398e11a8ab58a59f04b360591e2f6cdf122d4d705`).
**Use raw ticks:** the diagnostic's printed µs/line conversions are wrong by
four; correct scales are 64 ticks/µs and 4096 ticks/line. The observations'
claim that both 3E stages precede the boundary is also incorrect; the table
above records the reviewed result. Original artifacts are retained unchanged.
Build and run commands were:

```sh
bash .roster-scratch/b9-trace/b9_build.sh
(cd .roster-scratch/b9-trace && timeout 570 ../run.dgQed6/b9_obj/b9_t80_tests ../b9-retrieval/SHAKE27B.BIN 3)
```

The script records task-local paths; preserve the named scratch layout or
adapt those paths from the retained copies before reproducing it. Next,
observe the result buffer at `9211` to tie the raw interval to SHAKER's own
calculation, then resolve collision precedence against targeted hardware or
source clarification. Do not globally remove stage-B pokes: p.211 rules that
out. No fail-first repair vector is justified yet. Hardware remains under
coordinator ownership; this task released its unused grant without access.

## ACCC adjudication: the collision is not source-settled

Second pass, 2026-09-22 (branch `accuracy/shaker-b9-page-b`, base `56771ff`). Question:
does the RTL violate a documented French ACCC v1.11 rule at the C0=3F collision?
**No. The ACCC is silent on this ordering, so no fail-first vector and no RTL change.**

**What the source pins:**

- §19.5.3 FR p.210: the parity updates happen "sur la 3ème et la 4ème µseconde de
  l'instruction OUT(C),C". The 3rd-µs rule is `ParitéC9=C9.0`, then
  `ParitéC9=ParitéC9 xor (C4.0 and not(R9.0))`. The text does not say which line's C9
  applies when that µs starts a new line.
- §19.5.3 FR pp.211–212: all 16 chronograms put the OUT mid-line (the 3rd-µs column at C0=5,
  "on" at C0=6). They pin the offsets between the write and its two stages, not the
  absolute C0. Bench t21 relies on the same limit.
- §19.8.2 FR p.226: type-1 IVM counting runs "Lorsque C0 passe à 0". Normal counting
  resumes "Dès que R8 repasse à 0". Neither sentence says which mode applies when the
  toggle and C0→0 share an edge.
- §19.8.1 FR p.221 does contain an explicit rule, "À partir de la ligne C9 qui suit celle
  où R8 passe à 3", but only for type 0. Type 1 has no equivalent sentence, and the
  type-0 rule cannot be carried over.

**The candidate ordering that matches the hardware.** The derivation uses R9=7 and the
pre-write state from the replay (C9=0, ParityFrame odd). Stage labels follow the RTL:
stage A is one character after the bus write, stage B two after.

| Ordering at the shared stage-A / C0→0 edge | C9 on next line | After stage B | Lines, C4=0 (C9 ≤ 6) | 3F prediction |
|---|---|---|---|---|
| Current RTL (`rtl/CRTC.v` counter block: `line_new` wins, `line_next` uses the registered `ivm`): count in the old mode (C9+1), and the stage-A value comes from the pre-edge C9.0 | 1 | 0 (poke := ParityC9=0) | 0,1→0,2,4,6 = 5 | 2780 (observed MiSTer) |
| (1) New IVM mode governs the coincident count (C9+1+R9.0 = 2) | 2 | 2 | 0,2,4,6 = 4 | 2740 (reference) |
| (2) Old-mode count, with the stage-A value from the post-edge C9 | 1 | 1 | 0,1,3,5,7 = 5 | 2780 |

Ordering (1) leaves the other four rows unchanged. In the 3D/3E/00/01 cases, stage A lands
mid-line, or only stage B meets the boundary (3E). Under (1), 3F finishes in the same
state as 3E. With R9 odd the IVM step keeps C9's parity, so ordering (1) does not depend
on whether stage A samples C9 before or after the edge. That holds for either
ParityFrame value.

(1) is therefore the **candidate** reading. It fits rank-1 evidence and all five
update-delay rows. It is still **not adopted**, for two reasons. First, the link between
the "C0=#3F" label and a stage-A edge at C0=0 comes from the replay's fixture alignment:
menu bypassed, and SNA and PPI assumptions. Second, the result rests on one test family.
Changing it would move `line_next` and the `ivm`-gated row-end tests at every type-1 stage-A
edge that falls on C0=R0. That covers entering and leaving writes, and R9 even as well as odd.
The photograph covers only entering writes with R9 odd.

**The discriminator a hardware run needs.** Neither AmSpirit nor the device was used. The
device belongs to the coordinator.

1. Tie the label to the edge: run the production-T80 replay's result-buffer observation
   at `9211` (next step above) so that SHAKER's own computed value for 3F is `2780`.
   Without that, the 157/158 raw interval is only a proxy.
2. Test the rule independently of SHAKER on type 1. Use R0=63, R9=7, R8=0 and an R4/R7
   pair that gives a stable frame. Place one `OUT R8,3` with its bus write on C0=R0 of a
   C9=0 line. Measure the next raw VSYNC interval, using SHAKER's calibrated delay and
   the phase of the 3E/3F sleds. Ordering (1) predicts no extra line; the current RTL
   predicts +64 µs. A second case checks whether the rule generalizes beyond an odd R9:
   R9=6 (even), with the bus write on C0=R0 of the C9=5 line. Under ordering (1),
   `C9+not(R9.0)`=6 matches R9 excluding parity, so the edge becomes a C4 row end with
   the `ParitéC9 xor (not R9.0)` toggle. Under old-mode counting it steps to C9=6 with no
   row end, a one-line difference.
3. Alternatively, ask the author. The question is filed as item 6 of the
   [author feedback note](../../classic/accc-author-feedback.md).

A repair can follow when (1) or (2) confirms ordering (1). Its fail-first vector is the
3F row as derived above: bus write at C0=R0 on C9=0 with R9=7, next line C9=2, and C4=0
completes after four lines. It would cite §19.5.3 FR p.210 and §19.8.2 FR p.226, plus the
confirming evidence.

**MID FRAME SIZE:** no ACCC rule reaches it either. The glyph in the reference photograph
is still ambiguous (`4E40` or `4F40`; checked again, not resolved). The routine
`&9266..&932B` writes R8=3 "ON LINE 0" at a C0 that is not yet observed. This session
ran no replay to test whether the same collision applies there. Its discriminator is the
same result-buffer observation, applied to the two VSYNC-count loops at `&92CD`/`&92FE`.

## Result-buffer follow-up, 2026-09-23

**Closed evidence gap:** on unchanged production RTL at `b9edac2`, the authentic
SHAKER driver computes the same five result strings as the retained `95e6f56`
MiSTer capture. The `3F` extra line is now tied to SHAKER's own `2780` result,
not only to a raw VSYNC proxy. This is a replay result, not new device acceptance
or proof of the correct collision ordering.

The scratch top adds only a combinational RAM observation port and exposes the
existing T80 register-state output. The driver reads the completed buffer on the
first opcode fetch at `9211` (the routine's RET); observation advances no clocks
and writes no simulated memory. Production T80, GA and CRTC sources are unchanged.
The payload hash and entry/setup contract are the same as the replay above.

Static instruction inspection locates the measurement loop at `91F7–91FE`.
The routine multiplies its loop count by 16 at `9200–9203`, adds `0470` at
`9204–9207`, and formats the word into `9235–9238` through the call at `920B`.
It then loads the message address `9212` into HL before returning at `9211`.
The observed message contains the C9 character at `921D`, the two C0 label
characters at `9224–9225`, and the four result characters at `9235–9238`.
Thus the label and value below come from executed SHAKER memory, not a
host-side label assignment or conversion of the raw interval.

| Buffer C9 / C0 label | Result characters | Last raw VSYNC interval, master ticks | Raw interval, µs | Raw rise to RET fetch, ticks |
|---|---|---:|---:|---:|
| 0 / 3D | `2740` | 643072 | 10048 | 7471 |
| 0 / 3E | `2740` | 643072 | 10048 | 7471 |
| 0 / 3F | `2780` | 647168 | 10112 | 7471 |
| 1 / 00 | `2780` | 647168 | 10112 | 7471 |
| 1 / 01 | `2760` | 645056 | 10079 | 7535 |

The first four rows' calibrated result words equal the raw interval in µs.
For `01`, the calibrated value is 10080 µs (`2760`), one µs longer than the
last raw interval. Its return fetch also follows the last rise one µs later.
This is an observed measurement distinction, not a new hardware defect or
an adjudication of the sampling cause. Do not equate every displayed value
with an exact raw edge interval. The diagnostic now consistently uses
64 master ticks/µs and 4096 ticks per 64-µs line.

All five points completed at tick 101963130; the driver exits nonzero if
fewer than five complete. Commands, from the task checkout root:

```sh
bash .roster-scratch/b9-result/b9_build.sh
.roster-scratch/b9-result/b9_obj/b9_t80_tests .roster-scratch/b9-result/SHAKE27B.BIN 5
```

Private payload, disassembly, observation sources and `b9_run.log` remain in
`.roster-scratch/b9-result/` in task checkout
`/Users/renaudg/.codex/worktrees/66d3/Amstrad_MiSTer`; preserve them before
checkout cleanup. They are untracked and are not part of this documentation
commit. Log SHA-256:
`5e71cb0eed7eea8679ef8b9ff375f6e2e0600ae6fd948ee285538000c2c71fa3`.
Gemini scratch implementation run: `20260923T053841Z-14890-58b7`.
The coordinating agent inspected the observation diff, formatter instructions
and completed log; no simulation gate is claimed for this diagnostic run.

**Remaining discriminator.** Step 1 of the hardware plan above is complete for
the first five-point block. Menu-level setup equivalence, the later two delay
blocks and MID FRAME SIZE remain unobserved in this replay. French and English
§19.5.3 pp.210–212 and §19.8.2 p.226 were re-read through pdf-inspector;
the parity chronograms on pp.211–212 were also inspected as renders. The
collision remains source-unpinned. Before RTL changes, resolve the independent
odd/even-R9 hardware discriminator in step 2 or the author question in step 3;
AmSpirit agreement alone would remain comparative evidence. No MiSTer or
AmSpirit access, failing repair vector, RTL edit, synthesis or title closure
is claimed by this follow-up.

## Sync path: raw PPI vs filtered display

- SHAKER's numeric rows poll PPI Port B bit 0 = raw selected CRTC VSYNC
  ([rtl/Amstrad_motherboard.v](../../../rtl/Amstrad_motherboard.v) `ppi_ipb`
  at 1151), which bypasses `crt_filter` in every mode; the display filter
  cannot directly change a polled value.
- OSD "Sync filter" has three modes — Full, Raw pixels, Raw CRT
  ([Amstrad.sv](../../../Amstrad.sv) `status[36:35]`) — selecting the timing
  tuple for the display/scaler/screenshot stream, not the PPI input.
- Full and Raw pixels both use the entire filtered acquisition tuple.
  Raw pixels changes pixel/byte policy; it does not expose raw output sync.
  Raw CRT selects GA-shaped monitor HSYNC/VSYNC and raw CRTC HSYNC as
  horizontal blank, bypassing filter outputs. Raw CRT is therefore not a
  direct raw-CRTC-sync output in every signal.
- Requested vs applied is distinct: the request is normalised and commits
  during filtered VBLANK and CPU-owned phase, or immediately on reset ([rtl/Amstrad_motherboard.v](../../../rtl/Amstrad_motherboard.v)
  request/commit); native PNGs cannot verify the live OSD mode. Capture
  provenance and stale images still matter for screenshot reliability.
- No mode comparison was performed. The earlier run requested Full through
  its configuration; neither native PNGs nor that saved request independently
  observe the live applied register. Native and scaled screenshots use the
  same scaler buffer according to the companion capture investigation;
  software resizing is not an independent physical HDMI observation.

## Evidence

Source retrieval/adjudication runs: `Gemini20260922T063753Z-96527-513b`,
`Opus20260922T063947Z-2200-2b4b`, `MiMo20260922T064717Z-26888-12f7`; bounded
Astra adjudication rejected source overclaims. Muse replay
`20260922T065604Z-42773-d104` completed; a fresh bounded Astra inspection
accepted its localized timing evidence with the qualifications above. Rule citations use French
[ACCC v1.11](../../specs/ACCC1.11-FR.pdf) (user-owned, untracked). Scratch
inputs are not linked. `git diff --check` clean; no simulation gate for this
documentation-only change.
