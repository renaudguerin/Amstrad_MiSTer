# ASIC R4 decision lifetime — 4 November 2026

## Position

The original-C000 production-motherboard/T80 replay below does **not** lose its
first C802 frame wrap. Its first twelve invocations also never make C851's R4←2
a matching C4 write. The measured earlier counter/write candidate is generated
R0←59 at old C0=R0=63. The next useful reference is that first-loop R0/R9
chronology, not a held-R4 repair or a broad I/O delay.

There is a useful counter-lifetime discriminator, but no source-backed ASIC latch
repair is established yet. The current RTL evaluates live C4/R4 at the line seam.
CPCSyntaxError v0.12 can retain an equality set by an earlier R4 write. The French
source describes live end-of-line effects and the ROM-select boundary exception,
without explicitly defining whether a matching write survives a later unequal write.
Do not transplant the software latch or delay general CRTC admission from this evidence.

The original saved AmSpirit caller proves a coherent C802 counter outcome. It does
not prove the first FPGA divergence is at C802, or calibrate that instruction stop
to the rejected synthetic seam's CPU-fetch phase. Actual SHAKER BI pulse-width
comparisons independently rejected the broad admission qualifier; that rejection
remains binding for this investigation.

The initial source pass used no execution. A separately authorized private
production-motherboard/T80 replay is documented below. There were no production
or retained-test edits, general gates, RBF builds, device/API access, emulator
launches, commits, pushes or delegation. New private artifacts are confined to
`asic/r4-counter-discriminator/` and `asic/r4-natural-proof/`.

## Original evidence located

Accepted pre-initializer state: `asic/dsc4-plus-checkpoints/main-entry-c000/`.
The state before C802 and the fully single-stepped sequence are preserved in
`asic/dsc4-plus-write-chain/`, with `step-dsc4-write-chain.log`. These are authentic
boot/stepping artifacts, not execution from a later snapshot with the initializer
restarted. The older independent `r4-before-c802`/`r4-after-c802` checkpoints show
the analogous C4=R4=1 outcome, but belong to another captured invocation.

| Original same-caller stop | Tick | C0/C4/C9 | R4/R9 | BC |
|---|---:|---|---|---|
| C802, before OUT(C),0 | 92224211 | 63/2/7 | 2/7 | BD04 |
| C804, next instruction | 92224215 | 3/0/0 | 0/7 | BD04 |
| C80B, before R9 data write | 92224224 | 12/0/0 | 0/7 | BD04 |
| C80D, next instruction | 92224228 | 16/0/0 | 0/13 | BD04 |

The v3 snapshots identify Plus/type3/128K, not a guessed counter layout. C802 is
ED71, OUT(C),0; the saved state explicitly selects R4. At C808 the caller selects
R9; C80B is ED59, OUT(C),E, with E=0D. Register/counter stops are known; hidden VMA,
match-latch state and the within-instruction storage edge are not serialized.

The saved 2000-instruction history before C802 contains an earlier explicit R4←2:
C84D selects register4 via OUT(C),D with D=4; C850 increments B; C851 writes E=2
via OUT(C),E at BD09. The CPU state and bytes establish the transfer, but history
contains no C4/C9 at C851. It therefore cannot prove a matching-write latch was
armed. A static summary is retained as
`asic/r4-counter-discriminator/saved-history-r4-writes.json`, with the source
history path/hash. No runtime state was invented from that summary.

The rejected synthetic R4 seam fetched OUT(C),0 at internal C0=60 and stored it at
C0=63. The authentic AmSpirit stop is PC=C802/C0=63 before a four-us instruction.
Those are different observation anchors. Neither old counter-window tests nor the
saved command-start snapshot establish their absolute FPGA equivalence. In
particular, a title screenshot improving after an admission delay is not proof
that this synthetic seam reproduces the title's first counter failure.

## French authority checked

`lookup.py` was used for navigation. The existing French v1.11 pdf-inspector report
and primary Markdown were reused; classification is `native_partial`, with fallback
pages outside this material. §12.5 p.103 was rendered with `pdftoppm` and read
visually (`asic/r4-counter-discriminator/fr-103.png`). §10.3.4.1 p.78 was read in
the primary Markdown; no table/chronogram was used without its rendered form.

French §12.5 p.103, types3/4:

- Writing R4 equal to C4 makes C4 return to zero at the C9 row end and reloads the
  start address. R4 modification is considered immediately at line end.
- Writing R4 below C4 produces C4 overflow. This differs from C9/R9's >= behavior.
- For type3 only, R4←0 on C0=0 can precede the C4 reset if the I/O also selects ROM;
  that can prevent the reset. B bit5=0 activates this overlap; B bit5=1 does not.
  Type4 is excluded. PPI/printer/FDC overlap is not stated to change that speed.

French §10.3.4.1 p.78 says an R9 write at/below C9 resets next C9, then resets C4
if C4=R4 or increments it otherwise. This also states a comparison, without an
explicit type3 persistent last-frame-line latch. The explicit irreversible
last-line discussion in §12.4.1 belongs to type2 and cannot supply that missing
rule for type3.

C802 uses B=BD (bit5=1). The documented ROM-select overlap exception is therefore
not directly present. It remains a useful separate boundary control, not a reason
to add an R4/opcode admission exception for DSC4.

## Actual RTL and software hypothesis

In current `asic_video.v`, `last_charline = (charline == R4_v_total)` is live.
For the saved ordinary R8=0/R5=0 state, `body_frame_end = c9_done & last_charline`.
C4/C9 commit together on CLKEN with old C0=R0. If R4 becomes0 before this edge with
C4=2/C9=R9=7, last_charline is false and next C4/C9 is3/0. If the write shares the
edge, nonblocking assignments leave the old R4 visible to that decision and it
wraps to0/0. There is no retained old equality or distinct post-C0-wrap C4 stage.
Those are model facts, not a new physical phase rule.

The inspected CPCSyntaxError working source is clean at
`22de765ef44da46c55217e4dd7caa385d69a6045`; `v0.12.0^{commit}` resolves to that same
commit (the annotated tag object's own SHA differs). No executable was run.
`crtc_type_3.h:114–135`:

- `verticalTotalMatches()` accepts `verticalMatchForced` before the live equality.
- An R4 write equal to current C4 sets that flag.
- A later unequal R4 write normally does not clear it; the type3 ROM-select/C0=0
  exception can clear it.
- `crtc.cpp:1252` consumes the flag at row end; `newFrame()` also clears it.

This gives a concrete decision-lifetime hypothesis. It is not automatically the
DSC4 cause: C851's C4 is missing, and a matching write that survives an unequal one
is a stronger claim than the French text explicitly makes. Its instruction-level,
character-rounded I/O scheduler cannot settle the actual ASIC bus/decision edge.

## Proposed bounded discriminator, not run

Keep raw I/O admission and the R3/HSYNC logic unchanged. Use type3, R0=63, R9=7,
R5=R8=0, current C4=2/C9=7, and ordinary BD writes (no ROM-select overlap).
Land writes safely mid-line before testing a terminal edge:

| Case | R4 history in the terminal raster | Live comparison model | Sticky matching-write hypothesis |
|---|---|---|---|
| Equal-and-held | Start R4=3; write2 at C0=20 | Next C4=0 | Next C4=0 |
| Lower-and-held | Start R4=3; write1 at C0=40 | Next C4=3 | Next C4=3 |
| Match-then-lower | Start R4=3; write2 at20, then1 at40 | Next C4=3 | Next C4=0 |
| No matching write | Seed R4=2; write1 at40 | Next C4=3 | Next C4=3 in CPCSE, since seeding does not arm its write flag |

All predict next C9=0. The first two have direct §12.5 expectations. The third
isolates the disputed lifetime without any terminal bus-phase ambiguity; it should
remain a diagnostic/XFAIL question until hardware or an author clarification fixes
its expected outcome. The fourth catches an unjustified general "old equality was
once true" latch. An unconditional per-line type0/2 latch is not justified here.

This matrix is deliberately a **paper discriminator**, not a claim that the current
RTL fails a new source-established vector. The current live model predicts the
French-safe controls correctly. A generic held equality fix cannot be recommended
before the disputed row has an independent outcome. Retain original BI pulse-width
acceptance if any later counter-only hypothesis is implemented.

## Exact missing evidence and next action

First obtain the earliest natural divergence from the original pre-initializer
C000 state, preserving the original RAM/mapping and caller history. Do not restart
C000 from C802's already initialized/self-modified RAM. Around C851 and C802 record:
PC, accepted R4 bus data/port, stored R4, C0/C4/C9 immediately before/after acceptance
and the row-reset decision. At C851 we specifically need whether C4 was2. At C802 we
need whether R4 changed before the C4 reset decision, after it, or on the same edge.
Do not infer any unrecorded latch from the existing v3 SNA.

If those traces favor retained equality, the focused author/hardware question is:

> On type3, with R5=0 and C4=2/C9=R9=7, does an ordinary BD-port R4←2 at C0=20
> still force next C4=0 if R4←1 is then written at C0=40? Or does the later
> R4<C4 value cancel that decision and give next C4=3? Separately, when is the
> ordinary non-ROM-overlap terminal R4 comparison committed relative to C0=0?

This distinguishes a counter-decision lifetime from general admission and from the
source's explicit ROM-select race. Until it is answered or an actual FPGA trace
shows a different first divergence, the correct result is a named evidence gap,
not a production latch or a port-wide delay.

## Authentic C000 forward replay

All new executable artifacts are ignored under
`local/overnight-20261103/asic/r4-natural-proof/`; `commands.txt`, `build.sh`,
`provenance.json`, `private-source-hashes.json`, `original-header.json` and
`chunk-control.json` preserve the setup. Original checkpoints and prior replay
artifacts were left unchanged.

The input is the accepted original pre-initializer `main-entry-c000.sna`, SHA256
`89e054ccf1afe33c425ac068a2835e48aa109f7dadf37bf47dedea1e693e2c82`.
Its standard header, all128KiB RAM and the2296-byte CPC+ chunk were copied
unchanged to `entry-with-cpcplus.sna`; only AmSpirit's trailing private SPRT chunk
was omitted. The comparison C802 snapshot was never an initializer input.

The fixture reuses the BI/B18 actual motherboard, generated production T80 and
production standard-header/apply owners. It adds the production CPC+ parser and
passive taps. The copied motherboard adapter only translates generated T80 port
spelling and supplies OUT0/R800_mode constants. It changes no behavioral RTL.
Copied HEAD is `d50fee32dcb4447356034000c424817796eefa25`; the relevant
motherboard/ASIC video/ASIC GA files have no diff from accepted mode3113323.
Per-file hashes pin the copied source independently of later shared edits.

Snapshot CPU mapping is RAM banks0/1/2/7, with both ROMs disabled. Bank7's C000
bytes match the saved CPU-view dump. Its initial calls C26A and CC80 are present
in saved RAM; CC80 begins the original PPI-VSYNC synchronizer. Execution guards
would stop on any missing ROM read or FDC transfer. Neither was requested during
the completed run. No firmware, disk controller or new emulator was required.

The CPC+ state is locked/RMR2=0, PRI/SPLT/SSA/SSCR=0, zero sprite attributes and
DCSR=80 with no enabled DMA channel. It was consumed through the production
parser, including palette, IVR and internal DMA shadows. No hidden VMA or CPU/GA
phase was inferred from the omitted private chunk. The ordinary SNA loader seeds
VMA/VMA' from R12/R13 as documented in production; the title then runs its own
original initialization and synchronization.

Unlike classic, Plus actually consumes the v3 counters. This was checked at the
apply edge and first fetch, not assumed from the header version:

```text
RESTORE PC=c000 expected=c000 C0=2 C4=24 C9=1 expected=2/24/1 R4=38 R9=7
FIRST_FETCH pc=c000 byte=f3 c0=2 c4=24 c9=1
```

The following original fetches are C001, C004, C26A, CC80, CC82. There was no
trampoline, overwritten RAM, PC redirect, chosen C0, raster reseed or initializer
restart. The standard snapshot still omits absolute CPU/GA phase and VMA history;
the original synchronizer, rather than a reference-selected raster, establishes
the subsequent title timing.

### First natural C802 and subsequent counter drift

The preserved `run-first2/` establishes the first two invocations. The final
`run/` adds passive R0/register taps and follows twelve natural invocations:

```text
DONE firstpc=c000 pc=bf00 fetches=844136 writes=14666 C802=12 C851=11 clocks=110163895 dependency=0
```

Times here are64MHz master clocks after release. The first original C802 is
observed on the motherboard's M1/INSN_START interface at96104327. Nine clocks
earlier, the real terminal CCLK edge has already made the correct decision:

| Event | Tick | C0/C4/C9 before | R4/R9 | Result |
|---|---:|---|---|---|
| Terminal seam, PC still C801 | 96104318 | 63/1/7 | 1/7 | `frame_wrap=1`; next0/0/0, VMA reload4096. |
| Observed first C802 M1 | 96104327 | 0/0/0 | 1/7 | CPU is naturally entering OUT(C),0. |
| Accepted R4←0, portBD04 | 96104455 | 2/0/0 | 1/7 | Stored R4 becomes0,137 clocks after the successful wrap. |
| C804 M1 | 96104567 | 3/0/0 | 0/7 | Counter result agrees with the original first reference. |
| Accepted R9←13, portBD04 | 96105287 | 15/0/0 | 0/7 | Stored R9 becomes13. |
| First C851 R4←2, portBD09 | 97285767 | 28/0/2 | 0/7 | Stored R4 becomes2; C4 is0, so this is not a matching write. |
| Second C802 M1 | 97382279 | 0/0/2 | 2/7 | Already differs vertically from the reference's later recurring0/0 result. |

Do not compare M1 at C0=0 directly to an AmSpirit instruction-boundary stop at
C0=63 and infer a lost frame. The terminal edge and the actual write acceptance
are both recorded here, and demonstrate that first C802 did not cancel a pending
wrap. No synthetic fetch at C0=60 was used.

Across all eleven C851 acceptances, C0=28, C4=0 and C9 progresses2,4,6,8,9, then
stays9. Matching C4=payload2 occurs **zero** times. C802's C9 progresses0,2,4,6,7,
then stays7, always with C4=0. The CPU loop cadence remains1277952 master clocks
(19968µs). Thus this natural model develops a vertical-counter discrepancy before
the later C802 reference, but supplies no matching-write history that could
justify the CPCSE held-R4 flag as its repair.

### Reference ordinal is recoverable

`checkpoint_dsc4_plus_retry.py` saves the original C000 checkpoint, steps DI and
the type store, then **replaces a nonempty breakpoint set** with B07:0802. There
is no empty-clear between C000 and that first R4=1 stop. The matching retry log
records C000 at45734678 and C802 at47236307, a1501629µs elapsed initializer.
The private first C802 is1501630.109375µs after release, approximately one
microsecond later at its different observation anchor. Both first C804 stops
have C0/C4/C9=3/0/0.

The retry then steps C802, replaces its breakpoint with C80B and catches a later
caller at47256288. This is19981µs after first C802, consistent with one19968µs
loop plus its13µs C802→C80B sequence. Its HL low byte is1. That later C80B has
C4/C9=0/0, whereas the private second C80B has0/2. This bounds a real observed
counter difference by the second loop, without pretending to have its exact
first divergent edge.

`step_dsc4_write_chain.py` does **not** reboot or reload C000. It imports only
the definition prefix of the earlier script and arms C802 on the already-running
machine. The retry's final empty `bp()` had resumed execution, consistent with
the separately measured breakpoint-clear behavior. Its coherent single-stepped
C802 chain at92224211 is44987904µs after the first stop: exactly2253×19968µs.
HL low0→205 and E1→206 agree with2253 loop advances modulo256. This is a later
same-boot lineage, not a first/second-invocation reference; under the measured
cadence it is the2254th C802. The exact same-caller C802→C80D sequence remains
valid, but cannot be overlaid on the first initializer traversal.

### Earlier measured boundary and precise missing reference

The first generated R0 traffic provides a more useful earlier discriminator:

| PC | Tick | Current C0 | Old R0 | Stored new R0 | C4/C9 |
|---|---:|---:|---:|---:|---|
| 0E00 | 96157511 | 63 | 63 | 63 | 0/12 |
| 0E2B | 96161607 | 63 | 63 | 59 | 0/13 |
| 0E54 | 96165447 | 123 | 59 | 0 | 0/13 |
| 0E56 | 96165703 | 127 | 0 | 57 | 0/13 |

At0E2B the actual accepted R0←59 removes the live `hcc_last` comparison while
C0 is63, before its CCLK decision edge. The model then advances past the new
R0; C0 is123 by the next R0 write. These are measured production counter/write
facts, **not yet a claim that hardware must retain the old comparison**.
The original reference C802 RAM was read only for comparison, resolving CPU
addresses through its saved mode2/banks4..7 map; its generated0E2B bytes are
OUT(C),D. No comparison-image bytes were fed into initialization.

The current missing reference is the first generated R0/R9 sequence after the
successful first C802: stop immediately before/after0E2B and its next line
decision, recording current C0/C4/C9 and old/new R0/R9. That can distinguish an
intended R0 overflow from a prematurely cancelled old terminal decision. Check
the French type3 rule before writing a fail-first expectation. Preserve raw I/O
admission and BI's already accepted pulse widths; this evidence authorizes
neither a broad port delay nor a held-R4 production flag.

`run/{io,seams,c802-fine}.csv` preserve the emitted logs. Their final logging
helper added two post-state values, R0 and selected register, without adding
those two header names; `analyze.py` names the columns from that helper's exact
emitted order and writes separate `*-normalized.csv` files. No numeric values
are changed. `analysis.json` and `analysis.log` preserve the derived summaries.
All private processes exited. No live emulator or device was touched.

## R0 source follow-up: boundary known, lowering outcome still open

This follow-up was source-only: no further simulation, tests, emulator/API calls
or device operation. Navigation used `lookup.py`; the existing French
pdf-inspector primary Markdown and native-partial inspection report were reused.
French pp.123–126 were rendered and read visually as
`asic/r4-natural-proof/fr-r0-{123,124,125,126}.png`. The diagram, rather than its
flattened text extraction, determines the timings below.

The suggested classic sections do not define an ASIC retained-match rule:
§13.2.2 p.107 concerns type0 VSYNC qualification when C0 never reaches2;
§13.7.1.2 p.126 concerns type1 RFD after enlargement of a terminal frame line.
Neither can be transposed to type3. French §13.5 p.123 instead says the ASIC
handles all R0 values without the classic counter-freeze constraints, identifies
the separate C9/R9 lowering behavior, and reminds the reader of the one-us I/O
offset. It does not explicitly state what C0 does when new R0 is below current C0.

French §13.6.3 p.125 directly establishes an **instruction deadline** for
enlarging R0 from#3F to#7F. Using the document's C0vs instruction-start anchor:

| Transfer | Starts in time for the current line | Too late for the current line |
|---|---|---|
| OUT(C),r8 | #3B or#3C: chart continues #3F,#40 | #3D or#3E: chart gives #3F,#00 |
| OUTI | #3A or#3B: chart continues #3F,#40 | #3C or#3D: chart gives #3F,#00 |

The late value is not lost as register storage; it is too late to prevent that
old line's wrap. The chart's lower panel similarly distinguishes when R0=0
becomes positive. Its sentence that C0 never exceeds R0 is expressly qualified
by the timings shown. It does **not** provide a lowering-to-below-C0 chronogram
or establish a generic `C0>=R0` ASIC comparator.

C0vs is the source's PPI-VSYNC-derived timing reference (§§7.1–7.2 pp.38–39),
not an arbitrary first-M1 timestamp. The natural trace's raw C0=63 at acceptance
cannot be placed in one of these instruction rows merely by subtracting a
nominal instruction duration. That would repeat the unverified absolute-phase
assumption from the rejected bus qualifier.

### Software hypothesis versus current RTL

In matching CPCSE v0.12 source, `crtc.cpp::writeRegister` recomputes
`horizontalTotalMatch` on every R0 write. Type3's
`horizontalCounterAtTotal()` returns `horizontal >= R0`; other default chips
use equality. Its character tick later consumes `horizontalTotalMatch` to wrap
C0 and advance vertical state. Thus new R0 below current C0 forces a next-wrap
condition in this software. It is not simply an old equality retained forever:
an R0 write explicitly recomputes the condition.

Current `asic_video.v` instead makes `hcc_last=(hcc==R0_h_total)` combinational
and samples it on CLKEN. This equality/overflow behavior is already marked an
unverified P1 assumption in that source. The live equality explains the measured
0E2B path, but the CPCSE >= alternative remains a hypothesis. Neither source's
comment supplies missing silicon evidence.

### Paper discriminator, not a required failing assertion

Select R0 through BC00, then use ordinary BD00 data writes, with R5=R8=0 and a
nonterminal vertical row to isolate horizontal behavior. Keep raw I/O admission
and R3/HSYNC unchanged. Use the original/source-defined synchronizer for any
instruction deadline, and score raw counters rather than monitor position.

| Case | Transfer/history | Independently established outcome or open question |
|---|---|---|
| Safe held control | R0=63; write59 well before its terminal region, then hold. | Normal sequence reaches59 and wraps0; vertical state advances once (§13.1 p.104). |
| Source early enlargement | R0=63; OUT(C),D with D=127 begins on C0vs#3C. | Current line continues through#40 (§13.6.3 p.125). |
| Source late enlargement | Same transfer begins on C0vs#3D. | Old line still wraps#3F→0; next line uses new R0 (§13.6.3 p.125). |
| Natural lower-at-old-terminal | Store59 during raw old C0=R0=63, recording actual acceptance and next CCLK. | Live equality predicts64; retained old decision or CPCSE catch-up predicts0. No definitive French outcome for this lowered-value case. |
| Distinguish catch-up from old-match retention | Store20 safely during old C0=40/R0=63. | No old equality exists to retain: a next0 favors catch-up; next41 favors equality/overflow. Again a hardware/author question, not a source-proven expected pass. |

The next precise question is therefore: on type3, does an ordinary R0 write below
current C0 force a line end at the next character, and is the result different
when old C0 already equals old R0? Calibrate that through the proven early/late
enlargement controls before using the natural0E2B trace as an ASIC comparator
verdict. A first-loop reference stop immediately before/after0E2B remains useful
even without absolute instruction timing: it can directly show the next C0 and
C9 change. No new fail-first fixture is justified yet for the disputed lower
case, and no production comparator or bus-admission change is proposed.

## Private catch-up experiment against the fresh natural reference

The parent subsequently captured the missing natural reference using
**B04:0E2B**, not B00. `0E2B` in the private trace is the logical CPU PC of
generated code. C816 writes7FC2, selecting mode2/banks4..7; C830 jumps to0E00.
T80's instruction-start tap is M1/T2 with Prefix00, excluding operand fetches,
continued prefixes and IRQ acknowledge. Both original reference CPU maps and
bank4 bytes corroborate the address. A breakpoint on B00:0E2B cannot catch it.

Fresh original-C000, unpatched AmSpirit evidence is preserved in
`asic/r0-first-loop-amspirit-ordered/`. Its instruction-boundary stops are:

| Stop | Emulated µs since load | C0/C4/C9 | R0 |
|---|---:|---|---:|
| Before first0E2B | 1502522 | 60/0/13 | 63 |
| After its original OUT(C),D | 1502526 | 0/0/0 | 59 |
| Before first0E54 | 1502582 | 56/0/0 | 59 |
| After its original OUT(C),0 | 1502586 | 0/0/1 | 0 |

This supplies an independent **software-reference oracle** for the actual lower
case. It does not turn French §13.6.3 into a lowered-R0 silicon golden.

These E54 stops are60µs after E2B, in the same first loop. The older
`r0-first-loop-amspirit-b04/` instead stopped at1522550/1522554µs, one extra
19968µs loop later. Its2/4 variant comparison is preserved separately as
`score-skipped-nextloop.{py,json,log}`. The primary scorer now uses the ordered
first-loop reference and selects the first E54/E56 samples. No simulation was
rerun to make that correction.

### Baseline failure and one-line private variant

All new work is ignored under `asic/r0-catchup-proof/`; no production or retained
test file changed. `score-baseline.py` scores the frozen natural trace against
the fresh reference and exits1:

```text
FAIL frozen baseline first-E54 vertical [0, 13] reference [0, 0]
```

The baseline did not sample the immediate next CCLK after0E2B. Its post-write
state and known equality expression imply next64/0/13, but that is explicitly a
model-derived value. The direct frozen failure is C9=13 at
the first E54 acceptance, where the ordered reference remains C9=0 before that
instruction. Only that unchanged vertical state is scored across the different
within-instruction observation phases. No baseline simulation was rerun.

The private source copy changes exactly one behavioral line:

```diff
-wire        hcc_last = (hcc == R0_h_total);
+wire        hcc_last = (hcc >= R0_h_total);
```

This is a catch-up hypothesis: while stable C0 never exceeds R0 it is identical
to equality; when a write lowers R0 below current C0 it permits the next line
edge. It is broader than CPCSE's on-write recomputation, so equivalence with that
software is not claimed. Raw I/O admission, CPU/GA clocks, R3 storage and pulse
logic remain unchanged. `hypothesis.patch`, `experiment.json`, source hashes and
the frozen baseline provenance pin the change. The same original header, RAM,
CPC+ input and initial clock are used, with no CPU/PC/RAM/raster patch.

The completed natural run has the same CPU instruction/write counts and elapsed
clock count as baseline:

```text
DONE firstpc=c000 pc=bf00 fetches=844136 writes=14666 C802=12 C851=11 clocks=110163895 dependency=0
```

`score.py` reads expected values directly from the parent's saved checkpoint
metadata, hashes the corresponding v3 snapshots, and compares the variant's
original instruction-boundary observations:

```text
PASS before-e2b ref [60, 0, 13, 63] observed [60, 0, 13, 63]
PASS after-e2b ref [0, 0, 0, 59] observed [0, 0, 0, 59]
PASS before-e54 ref [56, 0, 0, 59] observed [56, 0, 0, 59]
PASS after-e54 ref [0, 0, 1, 0] observed [0, 0, 1, 0]
NATURAL_CHECKPOINTS 4 / 4
```

The added passive tap now records the exact next CCLK edges. At96161662, C0=63,
R0=59, C4/C9=0/13 becomes0/0/0. At96165502, C0=59, R0=0, C4/C9=0/0 becomes
0/0/1. Thus both measured same-first-loop counter discrepancies disappear under
the one-line variant, without shifting the bus. The correctly ordered reference
supports all four comparisons. The old skipped-next-loop score is capture-order
evidence, not the primary acceptance interpretation.

All twelve C802 observations now have C4/C9=0/0; the former0→2→4→6→7 raster
drift disappears. This is **not complete phase agreement**: the first C804 still
has C0=3, but subsequent C804 observations have C0=4 instead of the reference3.
Later C802 M1 is C0=1. Absolute instruction/counter phase and admission therefore
remain open even though this vertical discrepancy is removed.

### Bounded controls and preserved failure

The existing private BI actual-motherboard/T80 adapter and unchanged external
pulse oracle were rebuilt against the private current source/variant. No old
baseline was rerun. All eight original BI snapshots pass:

```text
BI_FINAL 8 / 8
```

This covers2376 critical pulses at the independently established14/1/2/3-character
widths and7488 normal14-character pulses, with zero width failures. It preserves
the hardware-backed pulse-admission result which rejected the earlier broad bus
qualifier. It is a pulse-width check, not a full BI image comparison.

Three private leaf controls pass: safely held R0=59; enlargement to127 well
before the old terminal; and enlargement to127 after the old wrap has already
committed. These are broad source-safe sides of §13.6.3, **not** a calibrated
OUT/OUTI C0vs deadline test. They do not resolve the adjacent#3C/#3D boundary.

The original leaf `t01e_r0_shrink_overflow` body and assertions are unchanged:

```text
FAIL existing t01e unverified eight-bit overflow assumption: overflow climb toward 255: HCC: expected 22, actual 0
PRIVATE_CONTROLS passed=3 failed=1
```

This known unverified model expectation conflicts with the catch-up hypothesis.
Its failure is retained in `controls.log`, rather than weakened or described as
a green suite. It requires independent resolution if a production fix is later
chosen. No general selection gate was run for this private experiment.

### Independent A4 constraint and next hypothesis boundary

During this experiment the parent reported an independent actual-hardware A4
deadline mismatch on3113323: Plus reference/CPCSE HCC#3D is KO while MiSTer says
OK, and OUTI on C0vs#3C differs01 versus00. Artifacts are
`asic/a4-3-device-3113323/` and `cpcse/a4-3-real-plus-ref/`. These were supplied
by the parent, not captured or reinterpreted here. A4 constrains register timing;
it is not a hardware oracle for lowering R0 below current C0.

The catch-up variant does not fix that independent admission deadline. Moreover,
deferred storage at the old line-end edge could produce the same first E2B
result while retaining equality. The
present result therefore demonstrates a sufficient mechanism for the first
discrepancy, not a complete repair or uniquely the right physical mechanism.

A bounded private CCLK_EN_N acceptance hypothesis is worth investigating against
the actual A4 early/late pair, natural E2B/E54 and BI1µs control together. If R3 is
stored on the same edge as its terminal comparison, nonblocking storage alone
leaves that comparison reading old R3; fresh-data forwarding would need its own
source-supported control. No such second hypothesis was implemented here.

There is also an existing hard integration constraint: the production IC116
equation exposes A13-low IORQ only while READY is high. Prior actual-motherboard
phase evidence places those visible OUT windows at[8,24) and[68,88), outside the
CCLK level/CCLK_EN_N edge. Simply ANDing ENABLE with CCLK_EN_N previously lost
alias OUT and IN-write transfers. A proposed clocked acceptance boundary must
therefore prove those contracts as well, without an A13/opcode bypass or an
unsupported extra window. Pending capture/commit state may be necessary, but is
not a source-derived repair established by this report.

Exact commands and final lines are preserved in `commands.txt`, `baseline-score.log`,
`score.log`, `bi-batch.log`, `controls.log` and `run/run.log`. All private processes
exited. There was no production/retained-test edit, general gate, API/device
operation, RBF build, commit, push or delegation. Preserve both the >= result
and the t01e failure for independent review before any production decision.

## Ordered-reference correction and character-edge hypothesis

The parent then captured a correctly ordered **same-first-loop** sequence at
`asic/r0-first-loop-amspirit-ordered/`: E2B before/after1502522/1502526µs,
E54 before/after1502582/1502586µs. Counter/register values are the same as the
four-row table above, but E54 is now60µs after E2B rather than20028µs later.
The parent separately proved that a nonempty breakpoint PUT resumes execution;
an unconditional resume POST can therefore run past an already-hit nearby
target. Evidence is `classic/nonempty-breakpoint-pause-control.json`. This is
why the older B04 E54 stops belong to the next loop.

The `>=` source and executable remain frozen. A read-only rescore against the
ordered reference, saved under `asic/character-write-proof/score.{json,log}`,
now gives **4/4** first-loop checkpoint matches for that variant. The older
2/4 result remains a valid comparison with a different E54 ordinal, not the
same-first-loop acceptance test. This does not remove A4's independent deadline
failure, the later horizontal offset or the unverified t01e conflict.

### Second private hypothesis: equality plus character-edge writes

The separate ignored `asic/character-write-proof/` starts from the original
3113323-equivalent production copies, retaining `hcc_last == R0_h_total`.
Its only motherboard behavioral change is the ASIC instance:

```verilog
.ENABLE((io_rd | io_wr) & (A[9] | no_wait | plus_cclk_en_n))
```

A9 selects the CRTC port's read/write operation independently of the CPU
opcode, so read ports remain live and IN on a write port still performs its
normal write side effect. `no_wait` explicitly retains fast-mode admission.
No opcode/A13 exception or pending-write infrastructure was introduced.

The private ASIC copy also forwards DI's R3 low/high nibbles to horizontal and
vertical terminal-width comparisons only when that R3 transfer is accepted on
the current CLKEN edge. This avoids the comparison seeing stale nonblocking
register storage at that same decision. No counter/GA clock was moved, and R0
equality was not changed. The complete small diff is `hypothesis.patch`.

An initial provisional copy also gated the dormant classic instance through an
overbroad textual replacement. It is preserved under
`pre-classic-owner-correction/`; that change was removed and the final natural
and BI runs below were rebuilt/rerun with only the ASIC instance changed. No
production classic source was touched.

### Fatal result in the authentic caller: register selection is lost

The final run preserves the same original C000 input, counter restore, CPU/RAM,
CPC+ parser and clock. It completes twelve invocations with the same844136
fetches,14666 logged data-write pulses and110163895 master clocks, with no ROM
or FDC dependency. Those counts do not establish correct transfers.

The ordered checkpoint comparison is **1/4**, not a counter repair:

```text
character-edge PASS before-e2b ref [60, 0, 13, 63] actual [60, 0, 13, 63]
character-edge FAIL after-e2b ref [0, 0, 0, 59] actual [0, 0, 0, 63]
character-edge FAIL before-e54 ref [56, 0, 0, 59] actual [56, 0, 0, 63]
character-edge FAIL after-e54 ref [0, 0, 1, 0] actual [60, 0, 0, 63]
```

The apparently correct first E2B counter wrap hides the wrong register write.
At96161662 the actual accepted data pulse has logical PC=0E2B, selected
register9, portBD09, data59 and post-R0 still63. The preceding original code is:

```text
0E23  AF       XOR A
0E24  D3 FF    OUT (#FF),A     ; address00FF selects R0, A13=0
0E26  16 3B    LD D,59
0E28  BE BE BE
0E2B  ED 51    OUT (C),D      ; should write R0; BC=BD09
```

The character qualifier misses that short READY-shaped A13-low select at00FF.
R9 remains selected, so E2B writes59 to R9 rather than R0. E54/E56 similarly
hit the wrong selected register. This is the previously predicted alias loss
inside the **authentic DSC4 caller**, not just an artificial port test. Later
C802 C9 becomes4 and stays there. No counter-lifetime conclusion can be drawn
from those wrong-register side effects.

### What survives, what is not proven

With incoming-R3 terminal forwarding, the unchanged private BI oracle still
reports `BI_FINAL 8 / 8`:2376 critical and7488 normal pulses retain the correct
widths. That is useful evidence for the R3 consumer interaction, but cannot
validate admission for other traffic.

The original production-T80 motherboard bus fixture, used diagnostically only,
preserves ordinary OUT/OUTI/OUTD payloads77, OUT(n) payloadBD, IN-write opcode78,
normal BE read30 and fast setup R1/R4/R9=40/38/7. Both A13-low alias OUT and
IN-write fail, leaving R13 at10hex. Its final result is
`DIAGNOSTIC_BUS 7 / 9`; the old storage-window assertions are not newly promoted
silicon oracles. No assertion was weakened.

No original A4 caller was executed in this private unit. Its actual#3C/#3D C0vs
enlargement deadline therefore remains
**untested here**; the old fixture's raw M1-coordinate windows cannot substitute
for that anchor. The independent original-Plus A4 photograph still requires a
faithful admission repair, but this simple character-edge AND is already
disqualified by lost aliases and the natural wrong-register transfer. No new
emulator/API session was opened.

The parent subsequently supplied a genuine pre-A4 menu snapshot at
`cpcse/a3-menu-v012/a3-menu-before-a4.sna`, SHA256
`eac8609c7458f75178a244ee32c6756b0dafe26b14a0ee0b05621ca98a57afef`.
Header inspection confirms v3/model4/type3/128KiB, PC4195; provenance is a fresh
CPCSEv0.12 Plus run with the English cartridge. It is ready for a separate
baseline measurement, not consumed here. The verified key4 PS/2 press/release
values are625/025hex. This B18 adapter currently ties ps2_key to0 and has no A4
result scorer, so obtaining a meaningful baseline would require an additional
bounded fixture unit after this already rejected hypothesis. Details are saved
in `character-write-proof/a4-input-ready.json`.

Recommended next boundary is source-supported transaction capture versus commit:
preserve the physical READY-shaped address-select/data traffic, then establish
when the ASIC consumes it using actual A4 and BI together. Do not paper over
the gap with an A13 bypass, a register-specific delay or a generic pending queue
without evidence. The present stage proposes no production change. The `>=`
experiment is a separate frozen result, not combined with this qualifier.

Final source/input hashes, exact commands, `score.log`, `bus-batch.log`,
`bi-batch.log`, `handoff.json` and `run/run.log` are preserved in the new scratch
directory. All processes have exited. No production/retained-test edit,
selection gate, API/device operation, RBF build, commit, push or delegation
occurred.

## Authentic A4 baseline attempt: blocked at the first opcode fetch

The separate baseline-only unit is ignored under `asic/a4-authentic-proof/`.
It uses the genuine menu SNA identified above, unchanged header/RAM/CPC+ bytes,
and the original equality/raw-admission production copies. No `>=`, character
write qualifier or fresh-R3 hypothesis is present. The English cartridge
provenance of the CPCSE snapshot is retained; no firmware image was silently
substituted.

The actual motherboard keyboard input is exposed at the private top. Planned
key4 input is625hex at20ms, then025hex release at100ms, through the real HID
matrix. **Neither event was reached.** Both runs stop within the first millisecond
because direct SNA resumption corrupts the first opcode. This is not an A4
measurement or a keyboard failure.

The first run and the subsequent passive first-bus trace both verify:

```text
RESTORE PC=4195 C=38/12/3 expected=38/12/3
FIRST_FETCH pc=4195 byte=ca C=38/12/3
MISSING_ROM PC=3
DONE entered=0 fetches=967 topresults=0 outiresults=0 r0writes=0 marker=0 PC=3 clocks=62448 dependency=1
```

The later ROM request is a consequence of the wrong path, not evidence that the
original A4 caller needs firmware. The passive trace identifies the first error
much earlier:

| Master tick after release | Observation |
|---:|---|
| 15 | Logical4195 reads RAM opcodeCA, but CPU input isC0. Raw IORQ is asserted during the M1 read. |
| 23 | T80 instruction-start/T2 observes PC4195; READY shaping temporarily gives CPU inputCA. |
| 67 | Still the first fetch/T2: CPU input is againC0 although RAM remainsCA. |
| 87 | The actual T80 IR isC0, notCA. |
| 135..287 | The unintended RET NZ reads stack bytes3C andFC fromC000/C001. |
| 295/311 | Address/next instruction-start isFC3C. |

Saved RAM at4195 is `CA C4 41`: JP Z,41C4. Saved F=30hex has Z clear, so the
correct original next PC is4198. Instead, the initial simultaneous I/O read
allows PPI-B to answer at4195 (A11=0, A9:A8=1). Its input isF4 for these jumpers
and inactive VSYNC. The wired-AND bus gives CA AND F4=C0, RET NZ; NZ is true and
it pops the screen data at the saved empty-stack position. The first opcode's
value at instruction-start was therefore insufficient to prove correct CPU
resumption: the IR load occurs later in that fetch.

`run/first-bus.csv` directly records logical address, physical address, RAM byte,
actual CPU bus, T80 DI/IR/TState/MCycle, M1/RD/IORQ and CPU enables. `blocker.json`
selects the decisive samples. The first run remains unchanged in
`run-direct-first-attempt/`; the second adds taps only and reproduces the same
bad path. Any RAM dump from the stopped wrong path is not an authentic A4 result.

The private wrapper uses the same production `plus_sna_apply` countdown,
registered owner reset, T80 DIRSet and CEN gating as B18/production. Its existing
pause ORs into sna_hold during loading, then releases without a PC change or
extra raster seed. Counter/PC restoration itself is correct. The failure is the
known first-fetch I/O overlap boundary; no trampoline, RAM edit, PC redirect,
special first-cycle mask or general SNA repair was introduced to hide it.

### Actual A4 code and result locations are now identified

An original extracted `SHAKE27A.asm`/binary already existed under the main
checkout's
`local/investigations/sync-monitor-response-2026-10-29-f4da/continuation/shaker-callers/`.
The private copy and hash are retained. The binary's org3D00 mapping matches
the menu RAM signature at4190; exact opcode bytes also match at every critical
address below.

- The real menu checks keyboard row7/bit0 at4105 and dispatches key4 to71AA.
- The top seven HCC tests run from71CB. Original OUT(C),C at71DB changes R0 to7F;
 71EE restores3F. PPI reads71F8/71FA are subtracted at71FC and stored through IX
 at71FD into74EE..74F4.
- The display loop72B6 maps those seven results to labels39..3F. Its lookup at
 7583 converts raw0 to OK and raw1 to KO. Thus index4 is the actual HCC3D
 comparison, without a synthetic M1-to-C0vs phase assumption.
- Original OUTI at7190 is measured with PPI reads719C/719E; its difference is
 stored at71A9 by71A1. The first such call supplies the displayed C0vs3C result;
 the later call overwrites that temporary for the3B result, so a future trace
 must preserve each store rather than read only final RAM.
- The original A487 synchronizer establishes the title's own VSYNC reference;
 no counter position should be imposed from a screenshot or synthetic fixture.

The next precise prerequisite is a genuine pre-A4 menu stop whose first fetch
can resume without this I/O overlap, or a separately owned production SNA
first-bus repair. A different original checkpoint must preserve original control
flow and register state; changing PC or injecting an initializer is not a
replacement. Once that input is available, the key/result taps above can score
the independent hardware HCC3C=OK/HCC3D=KO and first OUTI3C=01 results cheaply.
This unit establishes **no new A4 baseline mismatch**.

Exact commands, input/source hashes, both run logs, the passive bus CSV and
`blocker.json` are preserved in the task directory. Both processes exited.
There were no production/retained-test changes, general gates, device/API calls,
RBF builds, commits, pushes or delegation.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).

## Private pending-write capture/commit discriminator — 4 November 2026

The authentic A4 menu blocker above is resolved by the separately reviewed T80
reset repair; see [the first-fetch report](t80-first-fetch-iorq-2026-11-04.md).
With that repair, the original menu resumes at 4195→4198 and accepts real HID
key 4. Its frozen raw-admission ASIC baseline computes top results
`[0,0,0,0,0,1,1]`: HCC 3C=OK and HCC 3D=OK. The original Plus photograph and the
parent's fresh CPCSE comparison require 3C=OK and 3D=KO. The baseline therefore
has an authentic, independently measured admission/consumption mismatch. The
preceding blocked-run evidence remains pre-repair evidence, not a current
prerequisite or an A4 result.

The new unit is entirely private under
`local/overnight-20261103/asic/pending-write-proof/`. No production source,
retained test, gate, device/API, RBF, commit, push or delegation was involved.
The parent was notified before this report append. All private processes exited.

### Hypothesis and source boundary

The private `asic_video.v` captures each leading edge of the **original**
`ENABLE & ~nCS & ~R_nW` decode, storing RS, DI and the selected target. It commits
one pending transaction at the next existing CLKEN (`plus_cclk_en_n`). A capture
exactly on CLKEN commits directly if no earlier transaction is pending. If an
old transaction commits alongside a new capture, the new one remains pending;
selection forwarding covers a select commit coinciding with a data capture.
A second capture while pending without a simultaneous commit stops simulation.
There is one pending transaction, no FIFO, opcode decode, A13 bypass or fast-mode
exception. Read ports remain continuously connected to their original decode.

Reset clears pending state. SNA_LOAD discards pending work and seeds the edge
tracker from the current decoded level, preventing a held pre-restore level from
being replayed. The R8 interlace parity side effect consumes the same committed
transaction as its register storage. No clock, raw-HSYNC/blank, IRQ, sprite or
PRI event was moved.

R0 still uses the original equality comparator; **no `>=` catch-up change is
combined with this experiment**. Its character decision sees old stored R0 on
the commit edge. The horizontal and vertical R3 terminal comparisons instead
see the R3 payload committing on that edge. Horizontal forwarding is exercised
by BI's live-width outcome; vertical forwarding is the same private live-R3
hypothesis, not separately verified here.

French ACCC v1.11 §4.4.3 p.26's rendered table distinguishes fourth-us OUT(C)
from fifth-us OUTI/OUTD and third-us OUT(n),A. Its text explicitly warns that
within-us acceptance differs by instruction. French §13.6.3 p.125's rendered
CRTC3/4 chronogram makes an OUT beginning at C0vs3C just in time to extend old
R0=3F, while the next 3D case is too late. Both existing primary-page renders
were visually rechecked (`asic/fr-access-026.png` and
`asic/r4-natural-proof/fr-r0-125.png`). The stronger top-boundary oracle here is
the original Plus A4 photograph, independently reproduced by CPCSE. This does
**not** establish that the literal pending register or its storage edge exists
in silicon. It demonstrates a coherent separation of transaction capture from
counter consumption; the source-grid/T1 calibration caveat above remains open.

### Authentic A4: the previously failing deadline now matches

The same unchanged v3 menu SNA and actual motherboard/production T80 run through
real key 4, without CPU/RAM/PC/raster changes. The snapshot is still the fresh
Plus English-firmware CPCSE input, SHA256
`eac8609c7458f75178a244ee32c6756b0dafe26b14a0ee0b05621ca98a57afef`.
All private executables use the reset-fixed frozen T80 netlist SHA256
`435eb2ac16780e0f60c62f76e098239658f2c5f643c7746541ee7c2b5dfeb565`.

The original 71FD result stores now produce `[0,0,0,0,1,1,1]`, and the unchanged
original display lookup means:

```text
PASS HCC 3C expected OK actual OK
PASS HCC 3D expected KO actual KO
```

`a4-run/transactions.csv` records the mechanism at the actual original 71DB OUT:

| A4 label | Capture tick/C0 | Commit tick | Old R0 / pre-C0 | Post-C0/C9 | Stored R0 |
|---|---|---:|---|---|---:|
| 3C | 68363783 /62 | 68363838 | 63 /62 | 63 /1 | 127 |
| 3D | 83715655 /63 | 83715710 | 63 /63 | 0 /2 | 127 |

Thus 3C stores 127 before the old terminal decision, while 3D retains the old 63
wrap on its commit edge. Both take 55 master clocks from capture (GA S=0x1F)
to commit (S=0x03); that measured phase is evidence, not an asserted silicon
phase. All 185 captured transactions have 185 commits and no overwrite.

The five-emulated-second cap ends in ordinary delay-loop code at A515:

```text
DONE entered=1 fetches=2853177 topresults=7 outiresults=0 r0writes=51 marker=0 PC=a515 clocks=320000000 dependency=0
```

Exit 2 means the original OUTI measurements and 0037 marker were not yet reached.
No OUTI A4 numeric or full-page acceptance is claimed, and no capture extension
was added after the bounded cap. `score-a4.{py,json,log}` preserves the original
result stores and paired capture/commit rows.

### Natural DSC4: ordered first-loop reference and later counters

Only after A4 passed, the unchanged genuine C000 v3 snapshot was resumed with
original full CPC+ state. Header PC=C000 and C0/C4/C9=2/24/1 restore exactly;
the first original byte is F3. No initializer, bank, RAM, PC or clock patch was
introduced. `natural-run/` preserves the bounded actual-caller trace.

The four independently captured, correctly ordered AmSpirit first-loop
instruction boundaries all agree:

```text
PASS before-e2b ref [60,0,13,63] actual [60,0,13,63]
PASS after-e2b  ref [0,0,0,59]   actual [0,0,0,59]
PASS before-e54 ref [56,0,0,59] actual [56,0,0,59]
PASS after-e54  ref [0,0,1,0]   actual [0,0,1,0]
NATURAL_CHECKPOINTS 4 / 4
```

These are ordered same-FIRST-loop **software-reference** comparisons, not a new
hardware golden for lowering R0. `score-natural.py` hashes their saved snapshots
and retains the original reference ordinal. The skipped-next-loop E54 capture
is not used.

At character edge 96161662, E2B has old R0=63 and pre-C0/C4/C9=63/0/13; the old
terminal decision produces0/0/0 while the pending payload 59 commits. At 96165502,
E54 has old R0=59 and pre-C0/C4/C9=59/0/0; the old terminal decision produces0/0/1
while payload 0 commits. No held-R4 flag or lower-than-C0 catch-up comparator is
needed for these two observed outcomes.

All twelve observed C802 instruction starts have C0/C4/C9=0/0/0; all twelve
C804 starts have 3/0/0, and C80B has 13/0/0. The former vertical drift disappears.
Unlike the private `>=` experiment, later C804 does not acquire a +1-character
C0 discrepancy. This agrees with the bounded original C804=3/0/0 and later
C80B C4/C9=0/0 reference evidence described above; it is not a claim of every
instruction or pixel matching a complete demo run. CPU/write counts and the
110163895-master-clock bounded duration remain unchanged, with no dependency.

### BI and bus controls remain intact

The existing private BI adapter, unchanged original snapshots and unchanged
hardware-backed scorer pass **8/8**. Critical raw-HSYNC widths remain
14/1/2/3 characters in both groups; each page scores 297 critical pulses and 936
normal 14-character pulses with zero failures. In 01DF, edge 755657 has
C0=14, HSC=0, old stored R3=14 and committed payload 1. It produces HSC=1, stored R3=1
and HSYNC low on that edge, retaining the one-character pulse. This is a
counter-level live-R3 control, not merely a stored-value check.

The unchanged old private CPU-bus diagnostic passes **9/9**: OUT(C), OUTI,
OUTD, OUT(n), IN-on-write, normal BE readback, A13-low write-port OUT and IN aliases, and
no-WAIT fast register delivery. Payloads remain 77hex for OUT/OUTI/OUTD/aliasOUT,
BDhex for OUT(n), 78hex for both IN-write paths, and normal BE read returns 30hex.
Fast R1/R4/R9 remain40/38/7. Measured fetch-to-storage intervals are 208 master
clocks for OUT/IN-write including aliases, 272 for OUTI/OUTD, and 144 for OUT(n).
The old fixture's absolute storage-window assertions are preserved diagnostic
results, **not independent silicon phase oracles**. Its rejected synthetic
counter-equivalence seams were not rerun or reinstated as acceptance criteria.
No existing assertion was weakened. Runtime overwrite assertions remain active
in all private executables; none fired.

### Exact reproduction and next action

`pending-write-proof/commands.txt` records all build/run/score commands. The
focused final lines are `TOP_RAW [0,0,0,0,1,1,1]`, `NATURAL_CHECKPOINTS 4 / 4`,
`BI_FINAL 8 / 8` and `DIAGNOSTIC_BUS 9 / 9`. Logs, `hypothesis.patch`, complete
copied source, `source-hashes.json` and `handoff.json` preserve the reviewable
private unit. No baseline rerun or general selection gate was performed.

The capture/consume mechanism needs independent review of its
reset/SNA/selection ordering before any production proposal. It is the first
bounded candidate here that satisfies authentic A4's deadline, both ordered
natural R0 transitions, BI's live one-character pulse and actual alias/IN/fast
controls together. The physical storage edge, A4 OUTI numeric, broader register
side effects and full demo pixels remain unverified; these passes do not
justify declaring the ASIC bus architecture or all R0-lowering cases correct.

## Narrower terminal-decision discriminator

The broad capture/consume candidate now reaches the original A4's first OUTI3C
result store: baseline00 versus candidate01, against hardware01. Both execute
the same first caller72F2→715A and stop at its first71A1→71A9 store, before
later calls overwrite the result. Preserved evidence is under
`asic/pending-write-proof/outi-extension/`; baseline6.41130 and candidate6.09157
emulated seconds are normal exits. The earlier five-second cap above remains
an incomplete run, not a failed measurement.

A smaller alternative preserves all live register writes and remembers whether
the current character was entered with C0=R0. That remembered terminal opportunity
cannot be canceled by a later unequal R0 write. Reset seeds the reset equality;
snapshot load seeds equality from the incoming counters/register. This is a
counter-lifetime hypothesis, not evidence of a silicon latch or physical storage
edge. The original `>=` experiment, register queue and motherboard admission
qualifier are separate hypotheses and must not be combined silently.

The private `asic/r0-terminal-proof/` candidate passes the four early/late
OUT/OUTI counter consequences, the four ordered first-loop AmSpirit stops and
all twelve later C802=0/0/0 and C804=3/0/0 observations. No later R4 failure
appears within that authentic bounded chain. Production acceptance, retained
test and review are recorded separately when complete.

### An unsupported CPU observation anchor

The synthetic program's extra assertion that the first post-return LD BC at
PC800C must be observed at C0/C4/C9=0/30/0 was not source-backed. T80's
INSN_START represents an architectural state before completion of a possibly
WAIT-extended fetch; French §7.2's C0vs description does not bind that signal
to a particular master edge. The private precursor preserves that red assertion
and its logs rather than concealing it.

An independent AmSpirit run of the unchanged generated OUT3C and OUT3D programs
also observes63/29/7 at PC800C. This rejects the proposed zero-valued binding as
an acceptance oracle. It does not replace it with a new hardware golden.
AmSpirit observes critical opcode starts59/30/0 and60/30/0, versus T80's60/30/0
and61/30/0; that one-character observer difference remains explicit. After the
OUTs, AmSpirit observes63/30/0 and0/30/1 with R0=127, respectively. No NOP count,
CPU state, RAM byte or reference counter was adjusted to fit those outcomes.
Evidence: `asic/c0vs-reference-amspirit-v2/` and
`asic/c0vs-retained-proof/README.md`.

The first AmSpirit probe used B02:800C incorrectly: the banked-address API carries
an offset above3FFF into a later bank. It timed out after the actual program
halted. The successful probe used CPU addresses resolved through the current
mapping; the failed attempt is preserved and supplies no emulator-accuracy claim.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
