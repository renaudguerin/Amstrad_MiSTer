# ASIC demo diagnosis — 3 November 2026

## Acceptance position

A source-derived ASIC register-acceptance defect is repaired locally: `OUT(C),r8/0`
previously changed the CRTC register in the third microsecond, while CRTC3 requires the
fourth. The repair samples only the ASIC CRTC's write ports on the CPU-owned rising phase.
OUTI/OUTD retain fifth-microsecond acceptance; OUT(n),A retains third-microsecond
acceptance; read ports and IN-performs-write remain functional. The production T80 and
actual motherboard regression fails before the repair and passes afterward.

This is a verified interface repair, **not yet hardware acceptance of DSC4 or Amazing**.
An authentic AmSpirit Plus boot renders DSC4's credits and main effect coherently, but
its instruction-stop SNA counter values cannot establish the FPGA caller's absolute
bus phase. No classic RFD behavior was transplanted into the ASIC.

No MiSTer operation, synthesis, commit or push was performed by this worker. AmSpirit
was used only after the parent transferred ownership; it is left paused at C80D, with
breakpoints cleared. Independent review and hardware retest remain parent-owned.

## Rule and production boundary

French ACCC v1.11 §4.4.3 p.26 explicitly distinguishes these write instructions:

| Instruction | CRTC3/4 register update |
|---|---|
| OUT(C),r8 / OUT(C),0 | Fourth microsecond |
| OUTI / OUTD | Fifth microsecond |
| OUT(n),A | Third microsecond |

The French pdf-inspector inspection report (`native_partial`) and primary Markdown
were read. The table was verified visually from the rendered French page, saved as
`local/overnight-20261103/asic/fr-p026.png`. Lookup output was navigation only; Jev
was unavailable. No PDF was modified. The section's prose specifically excludes
OUTI/OUTD from the OUT(C) delay; a blanket one-microsecond queue would be wrong.

`rtl/Amstrad_motherboard.v` previously wired `asic_video.ENABLE` directly to the whole
I/O level. The leaf register file accepts on any CLOCK edge, so that exposed the early
OUT(C) strobe. The production change is confined to the ASIC instantiation (normal WAIT operation
uses the qualified slot; the explicit no-WAIT option retains immediate writes):

```verilog
.ENABLE((io_rd | io_wr) & (A[9] | no_wait | (plus_phi_en_p & ~plus_cpu_n))),
```

A9=0 names the write ports, including IN's write-side-effect aliases. A9=1 keeps read
ports continuously responsive. The CPU-owned rising slot distinguishes the
instruction families and remains visible through READY-shaped A13-low aliases; no opcode decoder, delay queue, GA/RMR timing change, or new
production port is required. `asic_video` itself remains unchanged. Its raw HSYNC and
the existing GA/PRI/interrupt pipelines are not shifted by this repair.

## Fail-before and after proof through the actual motherboard

The permanent owner regression is `sim/plus/asic_cpu_bus_test.cpp` with
`asic_cpu_bus_test_top.sv`. It elaborates the actual motherboard, production T80pa
(GHDL-translated VHDL), ASIC CRTC and Plus GA clock/WAIT owners. The external memory
is synthetic RAM; pixels return zero. Existing snapshot inputs seed registers and
`sna_hold` holds the CPU until the chosen counter phase. No acceptance expression is
copied into the fixture.

The fixture reuses B18's existing motherboard adaptation. Inspection confirmed that
it only normalizes GHDL CPU pin spellings and supplies OUT0/R800 defaults; it copies
the motherboard's bus/acceptance body unchanged. Passive observations measure register
changes and counters. The test earns retention because the source-derived timing
contract crosses T80, GA clocking, motherboard decode, register storage and row reset.

The original-wiring command was run before the production edit:

```sh
make -C sim/plus asic-cpu-bus GHDL=/opt/homebrew/bin/ghdl
```

It failed OUT(C) timing and the R4 terminal-row seam; all six original controls passed.
The same command after the production edit passes all eleven retained cases. One microsecond
is 64 master clocks; the test measures from the first M1 fetch of the critical opcode,
using the same convention as the existing `crtc_t80` tests. Expected windows come from
the French table, not the simulator's observed intervals.

| Case | Source-derived expectation | Original | Repaired |
|---|---|---:|---:|
| OUT(C),C to R13 | 192..255 master ticks | 153, FAIL | 216, PASS |
| OUTI | 256..319 ticks | 265, PASS | 280, PASS |
| OUTD | 256..319 ticks | 265, PASS | 280, PASS |
| OUT(n),A | 128..191 ticks | 137, PASS | 152, PASS |
| IN on BD write port | Retained opcode 78 reaches R13 | PASS | PASS |
| IN on BE read port | CPU A receives stored R12=30 | PASS | PASS |
| A13-low OUT(C) alias | Fourth-microsecond acceptance and payload | 153 in exploratory slice, FAIL | 216, PASS |
| A13-low IN-write alias | Retained opcode 78 reaches R13 | PASS in original slice | PASS |
| Terminal-row R4 write | Old frame end survives; next C4=0 | C4=2, FAIL | C4=0, PASS |
| Adjacent safe R4 phase | Next C4=0 | PASS | PASS |
| Explicit no-WAIT mode | Static frame register payloads arrive | Immediate original behavior retained | PASS |

The seam case starts an actual OUT(C),0 at internal C0=60, with C4=R4=1 and C9=R9=7.
An early third-microsecond store hits C0=63 and changes R4 before the live frame-end
comparison, so C4 advances to2. Fourth-microsecond acceptance lets the old equality
reset C4 before R4 becomes0 (French §12.5 p.103). The adjacent control starts at C0=61.
These are deterministic fixture phases, **not a claimed calibration of AmSpirit's C802
instruction-stop counter to the FPGA's instruction-start phase**.

Logs are ignored under `local/overnight-20261103/asic/phase/`:

- `motherboard-before.log`: actual motherboard fail-before command and both failures.
- `motherboard-after.log`: first character-edge candidate passed the initial eight
  cases, but was rejected by the selected gate because it blocked A13-low aliases.
- `motherboard-alias-before.log`: actual T80 alias OUT/IN/read controls fail with that
  rejected character-edge candidate.
- `selection-character-gate-failed.log` (in the parent `asic/` directory): the selected
  gate exposing unchanged scripted P5 m9 failure. The corrected CPU-owned slot makes
  the unchanged scripted motherboard bench pass (`p1-mobo-cpu-slot.log`).
- `motherboard-after-final.log`: final focused pass after the last fixture edit;
  final line `asic-cpu-bus: PASS` (eleven retained cases).
- `*-gate0/1.log` and CSVs: preceding isolated production-module exploration.
  That exploratory wrapper duplicated the old motherboard seam and is not the
  permanent acceptance proof. `discarded-no-register-select-*.log` preserves its
  initial reset-entry/setup failure; the final fixture executes a real register select.

The exact within-microsecond sampling phase is a model selection supported by the
instruction windows and alias controls, not a new ACCC silicon claim. A bare character
edge was insufficient: IC116/READY masks A13-low IORQ then. READY-qualified PHI_EN_P
alone also selects an earlier slot and failed the fourth-microsecond window. The
CPU-owned rising slot excludes that earlier phase and satisfies the independent controls.
No scripted CPU pulse was stretched and no existing assertion changed. The script's
48-master-clock transfer is not a timing oracle; the new production-T80 test owns that
contract (observed OUT(C)/IN(C) raw bus holds88 clocks, OUTI/OUTD/OUT(n) holds40).

A newly explored A13-low **read** at0A00 returnsFF with both original ungated wiring and
the final qualifier (`alias-read-original.log`, `motherboard-cpu-slot-after.log`). It is
excluded from the required suite because the unconditional30 expectation has not been
established for that alias. Its diagnostic source remains `alias-read-diagnostic.cpp`.
Named follow-up **P5/ASIC-A13-READ**: establish the physical read response and sampling
contract before deciding whether a repair is needed. At0A00, PPI is deselected(A11=1),
GA is deselected(A14=0), and Plus FDC selects motor(A8=0), not its read-data responder.
The fixture's external I/O input is neutralFF, so it does not explain theFF observation
through an FDC/PPI/GA overlap. The full top-level can also decode Kempston mouse there
(A4=A10=0), so a full-machine read is not an isolated CRTC proof. IC116/READY read-phase
interaction is a hypothesis, not a diagnosed hardware defect. Normal BE readback and all
existing BE/BF scripted assertions remain required and pass.

The selected gate also exposed an explicit fast-mode compatibility requirement. The
existing cartridge boot fixture sets `no_wait=1`; the normal periodic sampler lost its
unaligned pulses. Passive observations of that unchanged program showed actual
R0/R1/R4/R9=0/40/0/0 and zero physical video requests after the raw write monitor had
reported programming complete (`p10-observe-before.log`). The actual-production-T80
fast static-frame control also failed before adding the exception
(`motherboard-fast-before.log`). With `no_wait` selecting the original immediate-write
path, that control reaches R1/R4/R9=40/38/7 and the unchanged complete P10 suite passes
(`p10-after-final.log`). No P10 wait, payload, threshold or assertion was changed.
This is a compatibility policy for the explicit speed hack, not a claim about normal
ASIC timing. The normal-mode instruction windows and seam assertions stay required.

The new target is slow-tier because it needs GHDL. It is also included in the existing
`production-t80-test` CI aggregate; no new workflow is introduced. The final selected
fast gate is recorded in `local/overnight-20261103/asic/selection-final.log`.

## Final local validation

After the last code/test edit:

- `make -C sim/plus asic-cpu-bus GHDL=/opt/homebrew/bin/ghdl` — all eleven retained
  cases pass, including independently supplied output bytes, R4=0 at the seam, alias
  controls and the no-WAIT payload case. Final line: `asic-cpu-bus: PASS`.
- `python3 sim/select_tests.py --run` — **PASS43 benches**. The complete exact final
  `select_tests:` line is retained in `local/overnight-20261103/asic/selection-final.log`.
  Existing P10 FDC-payload XFAIL remains unchanged; no new expected failure was added.
- `git diff --check` — clean.

The focused slow proof was run separately from the selected fast gate. No code edit
followed these passes. Independent cross-provider review and synthesized-artifact
acceptance remain outstanding.

## Authentic AmSpirit Plus comparison

AmSpirit Lite **1.17.1/core2491682**, existing localhost6128 instance, original disk
startup. Effective settings: `6128+`, CRTC3, RAM128K, French ROM, normal CPU;
`monitor=Off`, `monitor_cal=cm14`, colour screen. The migrated repository helper checked
command acknowledgements. The actual `RUN"-DSC4` launcher was autotyped after250 boot
frames. Times below are frame-paced offsets after completed autotype, not a cross-system
scene-identity assertion. Each stop has settled PNG, state/config/render/memmap/history
and streamed SNA under `local/overnight-20261103/asic/dsc4-plus-coldboot/`.

| Checkpoint | Emulator tick | Observation |
|---|---:|---|
| Autotype completed | 6,081,373 | Frame14957; Plus/CRTC3 retained |
| Credits ~23s | 29,104,469 | Frame16110; four complete credit labels and all four colons visible |
| Credits ~23.5s | 29,563,733 | Frame16133; repeated credit checkpoint |
| Main ~50s | 56,021,341 | Frame17458; coherent full otter/main effect, not a top strip |
| Main ~50.5s | 56,560,477 | Frame17485; repeated main-effect checkpoint |

Original `dsc4-alt.dsk` SHA-256:
`ec9e0d51b10a7455848bd9211793c585c46456759486057b12a491039923b8e4`.
Firmware source `/Applications/amspirit-lite-sdl.app/Contents/Resources/ROMs/OS_PLUS_FR.cpr`,
SHA-256 `9fc3fcb83f1cb0ff39534293a458fe0f04fad004138f50ce51aab25bc6404e1d`.
Snapshot headers identify model4/CRTC3, 128K plus CPC+/SPRT chunks. No SNA seeded this boot.
The capture script/log are adjacent `capture_dsc4_plus.py` / `capture-dsc4-plus.log`.

ROM provenance caveat: reinitialization identifies OS cartridge banks0-3 from that
firmware, but `/api/rommap` still lists previous Copter271 cartridge banks4-7. This is
recorded rather than silently called cleared. If consequential, the supported control
is documented cartridge ejection followed by explicit firmware CPR load/reset. No
unsupported process/runtime workaround was attempted.

## Original detector and counter checkpoints

A second authentic disk boot confirms DSC4's Plus/PPI detection, without consulting
CRTC status aliases. Original detector219E sets E=3, writes03 to PPI-C, sets control82,
then reads PPI-C at21AA. The observed C value is03, F=04 (NZ), E=3. RET NZ reaches1D2D;
LD A,E then leaves A=3 at1D2E. Main C000 later enters A=3 and stores03 into C014, selecting
its original type3/4 branch C7C1.

Accepted checkpoint snapshots/state are in `asic/dsc4-plus-checkpoints/`, with their
matching `checkpoint-dsc4-plus-retry.log`. The first attempt's near-return breakpoint
missed1D2D; its earlier log is preserved separately. The retry reused that directory,
so the accepted snapshots match the **retry** log, not the first partial log.
A newly armed C80B breakpoint also stopped a later caller (~19.98ms after the first
C802); do not combine those stops into one13µs sequence.

A subsequent same-caller sequence was captured entirely by single-stepping, saved as
`asic/dsc4-plus-write-chain/` and `step-dsc4-write-chain.log`:

| PC before next instruction | Tick | SNA C0/C4/C9 | Stored R4/R9 |
|---|---:|---|---|
| C802: OUT(C),0 | 92,224,211 | 63/2/7 | 2/7 |
| C804 | 92,224,215 | 3/0/0 | 0/7 |
| C807 | 92,224,218 | 6/0/0 | 0/7 |
| C808: select R9 | 92,224,219 | 7/0/0 | 0/7 |
| C80A | 92,224,223 | 11/0/0 | 0/7 |
| C80B: write13 to R9 | 92,224,224 | 12/0/0 | 0/7 |
| C80D | 92,224,228 | 16/0/0 | 0/13 |

The first captured C802 used C4=R4=1 and the same63→3 reset. The table proves coherent
emulator register/counter outcomes across the original instructions. It does **not**
expose exact within-instruction port acceptance or hidden VMA/VMA'. `/api/crtc` only
publishes registers/selection; counters here come from streamed SNA header fields.
The write uses BDxx (B bit5=1), so the special ROM-select overlap race of French §12.5
is not automatically implicated.

Amazing was not cold-booted in this bounded unit. Its preserved bank7 entry begins
DI; POP AF; LD(C186),A, and initialization branches on C186. An authentic Plus capture
of that value remains the next software-path discriminator; the older CRTC1 checkpoint
with C186=1 is not a type3 caller. z80dasm was unavailable during inspection; no install
or broad reconstruction was attempted.

## Separate credit/HSYNC residual

An earlier isolated serializer probe observed48 white dots for R1=R2=4 versus64 when
R2=5 or10, at both PIXEN cadences. Artifacts remain `asic/hsync_probe.cpp`,
`hsync-r2-*.log`, `hsync-build.log`; French §13.1 p104 and §15.1 p147 were rendered/read.
The64-dot interpretation assumes the leaf fixture's phase matches the source's displayed
character convention. That phase has not been independently calibrated. It remains a
pipeline discrepancy/candidate, not a globally proven physical HSYNC defect.

No pixel-blanking or exported-HSYNC change was made. Raw HSYNC has GA/PRI/interrupt and
Raw-CRT acquisition consumers, so a global shift would have wider consequences. The
existing pixel fixture adapts the VRAM-address latch branch for zero-latency memory and
leaves absolute boundary phase open; its green result is not a physical phase oracle.

The common missing colons on classic and ASIC remain separate from the demonstrated
write-acceptance defect until matched traces show otherwise. Shared Full-mode fetch
compensation can zero retained high-byte history when DE is low, but no corresponding
SHIFT/DE edge trace was captured. Do not call that mechanism proven.

## Next acceptance action

Parent: obtain a fresh cross-provider review of the bounded motherboard/test diff,
then synthesize under the normal integration policy and repeat the original user
checkpoints on that exact artifact: DSC4 credits23s, image43s, main50s; Amazing30s.
The worker must not operate MiSTer without a new explicit ownership grant.

If main effects remain broken, first capture the FPGA's actual type3 path and accepted
R4/R9 phases. Compare those against the source-derived acceptance contract and the
preserved original emulator caller before altering counters. Then separate native
PEN/RGB from final blanking/acquisition. Emulator coherence alone does not locate the
FPGA divergence or supersede original-hardware evidence.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
