# Sonic Plus hardware and interrupt investigation — 22 September 2026

This follows the [source/fixture audit](irq-audit-2026-09-22.md). **DMA PAUSE
candidate `190f4d3`, integrated and built at `a137d48`, failed hardware acceptance
and is rejected.** A matched repeat reaches gameplay on baseline `c59e03a`, but
the candidate remains on a severely corrupted title after early and later fire.
The coordinator restored baseline behavior in `b0e5bed`. The candidate passes simulation, review and
timing, and matches AmSpirit's measured relative interrupt cadence. Those results
do not override the device regression. AmSpirit is a comparison implementation,
not a replacement for real CPC Plus hardware authority.

## Controlled hardware reproduction

One operator owned MiSTer and AmSpirit. The cartridge was the original 64K Sonic
2025 CPR, SHA-256
`4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae`.
Both systems used CPC 6128 Plus (AmSpirit model 4, type 3). MiSTer used explicit
Plus model bits `[34:33]=2`; sync filter bits `[36:35]=0` (Full). Native screenshots
do not independently show the OSD setting. The original 16-byte CFG was saved
before changing the Plus field.

| RBF | SHA-256 | Evidence |
| --- | --- | --- |
| `Amstrad_20260914_88262b9.rbf` | `bad7d36995c5f480c9328aae1b7f0174881214998786b5610b18bcd11e7b1076` | Existing timing-clean baseline |
| `Amstrad_20260922_c59e03a.rbf` | `da28d0cd01c910ce68d7bfc7482730dbdb1ae235dc814894776f3f06308e3335` | Full Quartus 17.0.2 build 35681139829; coordinator verified setup +0.561 ns, hold +0.242 ns, TNS 0 |

The new artifact includes the model/expansion changes; its IRQ RTL is unchanged.
It is a fresh clean fit, not an IRQ-fix candidate.

The old build showed a clean “Condense Team Presents” intro and severe title
background displacement. A repeated 18-second boot bracket followed by the
existing B17 keyboard-joystick fire schedule reached the Act 1 intro and then
active gameplay. Each screenshot was inspected to identify the scene: elapsed
time alone was not accepted as a checkpoint.

The current and old builds produced **identical PNG bytes** at both post-fire
checkpoints:

| Actual scene | SHA-256 on both builds | Observation |
| --- | --- | --- |
| Act 1 intro | `62093d04614f58450e7b7defc6abcf504495fda386019d6c749660cd765cb1ac` | Mostly coherent scene with the moving title card |
| Gameplay | `0c94f96eb82982fb9fc431501352779420b6e46b9be0ef400f8604d0efe9380c` | Displaced horizontal band; Sonic and HUD visible |

The title remains corrupt on both. Title captures differ because they are
animated and not the same exact frame. Identical later captures demonstrate
repeatable reproduction and no visible change at those two checkpoints; they do
not prove timing equivalence throughout the game.

Evidence root (ignored, cartridge-derived material stays untracked):
`docs/screenshots/sonic-loop-2026-09-22/`. Driver cases and manifests are
`device-case.json`, `device-intro-title/`, `device-title-fire/`, and
`device-current/`; manual post-fire shots are `sonic-loop-after-fire-02.png`,
`sonic-loop-after-fire-later-02.png`, `sonic-current-after-fire.png`, and
`sonic-current-after-fire-later.png`.

## AmSpirit controls and snapshot boundary

AmSpirit Lite 1.15.1, core 2491682, renders coherent title and gameplay scenes with
the matching CPR. The machine was initially running BASIC Ready; its original
configuration, render configuration and full SNA were saved under `preflight/`.

AmSpirit and MiSTer did not reach scenes at the same elapsed wall time. In
`amspirit-baseline/`, the folder called `intro` actually contains the title, the
folder called `title` contains the Act 1 intro, and `after-fire` contains the
title after fire was pressed during attract mode. These names are acquisition
labels, not scene assertions. The later MiSTer fire sequence was independently
verified from its screenshots.

The title SNA contains 128 KiB RAM, a 2,296-byte `CPC+` chunk, and AmSpirit's
private `SPRT` chunk. A portable control removed only `SPRT`, retaining the
standard header, RAM and `CPC+`. The full snapshot continued rendering the title;
the portable control went black (CRTC R6 became zero) and remained black through
10, 52 and 101 frames. This prevents treating that snapshot as an equivalent
running-machine checkpoint. `roundtrip/manifest.json` and both image/state series
preserve the control.

A second diagnostic froze the CPU using `DI; JP BF01` in RAM bank 2. An initial
header-only PC/IFF patch was checked against a live full-snapshot freeze: RAM
writes redirected the paused PC 8353 to BF00, then execution reached BF01 with
IFF1/IFF2 zero. Ten frames later, the frozen machine's standard header, RAM and
CPC+ chunk were saved; removing SPRT and reloading this frozen snapshot retained
all RAM bytes and all CPC+ bytes (only the refresh register differed in the
header), but lost visible sprite content in AmSpirit. Background displacement
also exists when the title's dynamic split handlers are deliberately stopped.
These are diagnostic artifacts, not unmodified title acceptance evidence.

No frozen SNA was loaded on MiSTer and no IRQ-versus-video conclusion was drawn
from it. The failed portable controls are preserved under `freeze/`,
`roundtrip/` and `live-freeze/`.

A read-only loader check found that both 88262b9 and current source recognize and
restore CPC+ sprite pixels, attributes, palette, PRI/SPLT/SSA/SSCR/IVR, DMA state,
RMR2 and unlock state. They discard SPRT as an unknown chunk. `asic_video` seeds
VMA from R12/R13 and clears unsaved pipeline/history state; it does not promise
arbitrary mid-frame fidelity. Thus the old shorthand “SPRT is ignored” must not
be read as “CPC+ is ignored”, nor does the presence of the standard restore path
prove the AmSpirit control equivalent.

## Runtime witness

The title snapshot resolves the runtime IM2 table at B800 to
833F/8342/834A/8352 for DMA2/DMA1/DMA0/raster. At this checkpoint 833F contains
`JP A021`; the remaining entries contain `EI; RETI`. DMA2's self-modifying handler
chain starts A021, A048, A06F, A096, A0BD, A0E4, A10B, A132. Each entry patches the
JP target for the next interrupt and writes SPLT/SSA/SSCR. For example A021
writes SPLT=7 and SSA high=3 through a 16-bit store at 6801, SSA low=A3 at 6803,
and SSCR=80 at 6804. A048 sets the next split to 15. Those writes create a concrete
video deadline witness: correct vector selection is necessary but does not by
itself establish that the writes arrive before the split comparator uses them.

### Measured cadence and counterfactual

The unmodified production-T80 normal-boot trace stopped at the tenth interrupt
acknowledge, after 652,875,802 master ticks (about 10.2 simulated seconds, 436
wall seconds). Its fixed cap was 1.28 billion ticks / 20 simulated seconds. It
used the previously built D5 production adapter with observation taps; it did not
load a snapshot or modify the cartridge. The first title frame has a different
initial phase from the saved AmSpirit checkpoint, so absolute line numbers are
not treated as matched state.

The first seven DMA2 acknowledgements nevertheless show the relevant cadence:
CRTC lines 230, 239, 248, 257, 266, 275, 292. The first six are nine lines apart
for repeated `PAUSE 7; INT`; the following `PAUSE 15; INT` interval is 17 lines.
All DMA2 vectors were zero. Intervening legacy raster acknowledgements returned
six. The first handler wrote SPLT=7 at A02C on line 231. Raw logs and the RAM dump
are under `production-trace/`.

AmSpirit breakpoints at actual handler bodies captured the corresponding
sequence without a portable snapshot: A021/A048/A06F/A096/A0BD/A0E4/A10B entered
at beam Y=70/78/86/94/102/110/126. A132 and A159 entered at Y=174 and Y=214.
The first sequence therefore has eight-line and sixteen-line intervals,
respectively. These are API beam coordinates, not a claimed absolute CRTC-line
mapping. Sixteen entries covering a frame boundary are preserved in
`irq-reference/`. PC breakpoints at the JP stub alone did not stop reliably;
body breakpoints did. A missed stub breakpoint is not evidence that the CPU did
not execute that address.

In RTL, HSYNC decrements a terminal pause count from one to zero, but chooses
`active_ch` from the old count. This stalls instruction fetch for one more
HSYNC. The existing d03 expectation also pins that extra line. The original
Arnold V section 2.6 describes the prescaled interval as N × (PPR+1); its wording
about the instructions before and after PAUSE should not be used to conceal
inclusive-boundary ambiguity. The title's measured sequence is the concrete
boundary discriminator. Live PPR-write semantics are a separate existing finding
and are outside this repair.

The title list has 23 positive PAUSE instructions. A counterfactual in AmSpirit
loaded the full working snapshot and increased all 23 counts by one via live RAM
writes: address 835A (42 to 43), and 8378 through 83CC every four bytes. This
models the extra delay without changing interrupt vectors or video logic. After
50 frames the title showed severe displaced bands, and a saved RAM dump
confirmed all 23 patched bytes remained. The screenshot SHA-256 is
`258ecf8eaba56d35b49fb86ebbce4ae4f2fde46e4c3f871b611f960ff08ebc6b`;
`pause-counterfactual/manifest.json` records every edit. Similar appearance is
supporting evidence, not an exact frame/pixel equivalence claim.

Opus high independently identified the PAUSE terminal-edge issue before the
production trace completed. Its analysis is preserved in
`opus-causal-analysis.log`. PRI no-wrap, B19 retention and IRQ vector/clear logic
are not part of the proposed change.

## Candidate repair and verification

The bounded candidate activates an enabled DMA channel on the same HSYNC that
expires its final prescaled pause tick. Already-zero pause counters remain
eligible; the count decrement, prescaler reload and live-PPR policy are unchanged.
It changes no PRI, vector, acknowledgement or video behavior directly.

Opus authored the provisional RTL and focused vectors. Its managed environment
denied all make/Python execution, so its output was not accepted as tested.
The parent set aside the proposed RTL and the two changed historical
expectations, then ran the new d15 against original RTL. All fourteen existing
vectors passed; d15 failed with:

```
ch2 (PPR2=0, PAUSE 7) INT lines 9 18 27 36 45 54,
expected 8 16 24 32 40 48 56
```

This reproduces the production-T80 discrepancy with concurrently active
channels. The final test also pins a prescaled terminal edge, an unaffected
channel's PSG writes, and PAUSE zero/one followed by INT. The existing d03 and
d09 expectations are revised because they previously encoded the same extra
expiry line; no unrelated assertion is weakened. Direct CPC Plus hardware
measurement of PAUSE-one and nonzero-PPR boundaries remains outside the present
evidence; those expectations extend the prescaled rule consistently.

The corrected RTL passed `make -C sim/plus run/asic_dma_tests` (all 15 vectors).
The first selected gate passed three benches, but fresh Sol high review found a
real coverage gap: `plus_p8_test.cpp` still asserted the old extra line after
restored pause state, and the test index did not associate that bench with DMA
changes. Running the unchanged snapshot test against the corrected RTL failed
at the expected boundary: `pause leaked a fetch on HSYNC3`.

The two snapshot expectations now fetch on expiry HSYNC 3/4, respectively,
instead of 4/5. This retains the restored prescaler/count checks. The existing
P8 index row now includes `asic_dma.v`, so future DMA changes select this
integration test. All twelve focused snapshot cases and the rest of P8 then
passed. After those last code edits, `python3 sim/select_tests.py --run` ended:

```
select_tests: PASS 4 benches: run/asic_dma_tests, run/plus_p8_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests
```

Logs are `pause-fix/01-red.log`, `02-green.log`, `03-gate.log`,
`04-snapshot-old-expectation.log`, `05-snapshot-green.log` and
`06-final-gate.log` beneath the evidence root. Fresh Sol high review cleared
the RTL/test change after the P8 remediation.
A bounded Opus high review also cleared the parent-authored P8/index edits;
its comment that the new report must be staged is satisfied by including this
file in the same change. Its additional B18 dependency observation does not
require another DMA test: that slow bench runs six Classic capture/restore
cases, despite compiling the shared motherboard's Plus modules. Logs are
`sol-review.md` and `opus-remediation-review.log` in the same directory.

### Candidate production-T80 trace

The same normal-boot observation program and unmodified CPR were rerun against
clean `190f4d3` (RTL identical to `a137d48`), with the same ten-acknowledgement
stop and 20-second simulated cap. The run completed at 652,842,906 master ticks
in 405.47 wall seconds. The first 617 complete initialization log lines match
the original byte-for-byte.

| Observation | Original RTL | Candidate RTL |
| --- | --- | --- |
| DMA2 acknowledgement lines | 230,239,248,257,266,275,292 | 228,236,244,252,260,268,284 |
| Successive intervals | 9,9,9,9,9,17 | 8,8,8,8,8,16 |
| SPLT writes at A02C/A053/A07A/A0A1/A0C8/A0EF | 231,240,249,258,267,276 | 229,237,245,253,261,269 |

DMA2 vectors remain zero. The candidate's relative cadence matches AmSpirit's
measured intervals; absolute phase is still unmatched. This bounded trace does
not reach the final title handler or establish steady-state split deadlines.
`production-trace-corrected/` preserves source, build log, raw trace, comparison
script/output, RAM and hash provenance.

### Hardware rejection and matched repeat

Exact-SHA CI [35684468864](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35684468864)
passed all required jobs for `a137d480ddaba53864b1bd0f616f12398fb76ea9`.
Artifact `Amstrad-build-236-1-full` contains
`Amstrad_20260922_a137d48.rbf`, SHA-256
`c0293860c0c9a3d3a5c6a033cce6b7910e49a8abef0e40b3fe2cc5e514fe4984`.
The coordinator reported a full Quartus 17.0.2 fit with setup/hold minima
+0.514/+0.245 ns, zero TNS and 23,735 ALMs (57%). The device copy's hash was
verified before use.

The initial candidate run used the same CPR, Plus configuration, 18-second boot
bracket and B17 fire schedule as the baseline. All three inspected captures
showed the displaced yellow/purple title, including both post-fire checkpoints.
Evidence is under `device-candidate-a137d48/`.

A bounded sequential repeat tested `c59e03a` and `a137d48` with the same schedule:
capture title after the 18-second boot; fire; wait two seconds and capture; wait
four seconds and capture; wait six seconds and fire again; repeat the two/four
second captures. Capture and transfer overhead adds wall time, recorded by the
manifests. Both runs used the same temporary keyboard-joystick map and test CFG
SHA-256 `13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
Every title and post-fire image was visually inspected.

| Checkpoint | Baseline `c59e03a` | Candidate `a137d48` |
| --- | --- | --- |
| Initial capture | Corrupt title | Severely displaced yellow/purple title |
| Early fire, first capture | Act 1 intro | Corrupt title |
| Early fire, later capture | Gameplay with known displaced band | Corrupt title |
| Later fire, first capture | Gameplay with known displaced band | Corrupt title |
| Later fire, later capture | Mostly coherent gameplay, Sonic and enemy visible | Corrupt title |

The baseline's final gameplay PNG hash is
`56592b06f84d1db775d79d4b538189415ee7c63d748a001bee91e6e93ef92ebb`;
the candidate's corresponding title PNG hash is
`b6184b4c143695a7a3ce42a01409c0b9a72c17b53baa7ef07e7869288292fc91`.
All capture hashes, input logs, cases and cleanup assertions are in
`device-latefire-c59e03a/` and `device-latefire-a137d48/`. Their preserved
`run.py` records the shared acquisition procedure. The candidate's late border
pattern changes slightly; still images do not establish a completely frozen core.

**Decision:** reject and revert the candidate behavior. Commit `b0e5bed` restores
`asic_dma.v`, `asic_dma_test.cpp` and `plus_p8_test.cpp` exactly to `0e92c9c`,
retaining the useful P8 dependency in `sim/TESTS.md`. The coordinator reports
the four selected benches pass after the restore and fresh Opus review has no
findings. Integration synthesis of the restore passed as recorded below. No speculative follow-on
RTL change was made. Sonic remains unresolved on the baseline. The smallest
next discriminator is a matched full title-frame trace through DMA enable,
the final handler and the next frame's split writes, to locate the first phase
divergence beyond the present ten-acknowledgement window. This is a proposed
measurement, not a new cause claim or hardware acceptance.

## Device restoration

After the baseline and counterfactual phase, MiSTer returned to MENU, the owned
B17 temporary input map was removed, and the original CFG hash was verified:
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
The input replayer released its owned keys after each schedule. The new
c59e03a RBF remains installed under its distinct filename.

Each candidate and matched-repeat run independently verified the same restored
CFG hash, removal of its temporary input map and return to MENU in its
`acceptance-manifest.json`. Both early and later replays released all owned keys
and exited keyboard-joystick mode. The rejected a137d48 RBF remains installed
under its distinct filename for provenance; it is not the running core.

AmSpirit's original full SNA, configuration and rendering settings were
restored. The restored screenshot was inspected and shows BASIC Ready; execution
is running as originally found, with firmware ROM mapping restored. Our Z80
breakpoints were cleared. Evidence is under `restored/`. AmSpirit was untouched
during candidate hardware acceptance and the matched repeat.

## Restored integration artifact

The exact restore `b0e5bed` and final evidence were integrated at `0601050`.
Fresh Opus review confirmed the three behavior/test files match `0e92c9c`
byte-for-byte; the independent P8 test-selection dependency remains. The selected
four-bench gate passed before publication, with logs under `revert/` in the
ignored evidence root.

[CI 35686740255](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35686740255)
passed simulation, production-T80, synthesis policy, routing, full synthesis and
the required gate on exact SHA `06010503221468190d7c2b17f5ddec47e7024932`.
Artifact `Amstrad-build-237-1-full` uses Quartus 17.0.2 and contains the restored
baseline, not the rejected candidate. Setup/hold minima are +0.580/+0.191 ns,
TNS is zero, and utilization is 23,792 ALMs (57%), 28,351 registers, 102 RAM
blocks and 35 DSP blocks. Downloaded and delivered copies of
`output_files/Amstrad_20260922_0601050.rbf` both hash to
`66b8d72fa1b67a6535ba07a4d9118c9998f833a4bbe89a1339d02eb4d25fa7fd`.
Reports are in `output_files/reports-0601050/`. This exact restored artifact was
not loaded for another hardware run; the matched baseline/candidate evidence
above remains the device acceptance record. Sonic remains unresolved.
