# B25 portable instruction-acceptance diagnostic

Technical information sourced from the "Amstrad CPC CRTC Compendium" by
Longshot (CC BY-NC-ND).

## Predictions and measurement contract

Predictions recorded before implementation from French ACCC v1.11 §27.7.2,
pp289–290: the two rendered NOP chronograms retain the same acceptance boundary
when INT moves from T3 to T4. HALT supplies repeated NOP cycles and is the robust
synchronization control. The rendered ADD HL,DE chronograms instead accept the
current instruction in the on-time case and execute a further ADD in the late
case. The ADD Wait-row label is inconsistent with its T-cycle row; use the
T-cycle row and accompanying prose, retaining this ambiguity. CRTC1 units can
differ; matching CRTC0 is explicitly possible and is not an error.

RET NC with carry set and INC HL are additional phase-sensitive probes, not
instructions expressly diagrammed in §27.7.2. Both NOP padding parities are
collected; ADD uses all three whole-microsecond phases. Counts are observations,
not assertions that either the emulator or an absolute ISR instruction budget
is original hardware truth. A photograph records the complete parity group.
An electrical INT delay cannot be recovered uniquely from these counts.

The portable replacement for fixture IRQ masking is a real HALT interrupt,
followed by an identical arming ISR and an instruction sled until the next real
interrupt. R7=FF suppresses VSYNC realignment during the measurement; R0=63,
R2=46, and R3=8E retain 64-us lines and the reference HSYNC width. Software
resets the GA counter before HALT. The arming interrupt occurs after 52 HSYNCs,
and its acknowledgement clears bit 5 when the count is already zero; the
capture interrupt follows another 52 lines (3328 us), independently of the
initial CRTC counters. Before each batch the program allows enough time after
R7=FF for any pre-existing VSYNC to end. No CPU interrupt pin is masked, and no
snapshot establishes timing. Normal display timing is restored before results.

## Build and run

Build with `python3 scripts/diagnostics/classic_b25.py` (`sjasmplus` on PATH).
The outputs live under `output_files/classic-b25/`: `classic-b25.dsk`, the raw
program, assembler listing/symbols, and JSON containing the DSK SHA256 and RAM
layout. The standard single-sided 40-track DATA-format disk contains `B25.BIN`
with an AMSDOS binary header, load and entry address `&1000`; no firmware or
private media is bundled.

On an original CPC with a disk interface, insert the disk in drive A and enter
`RUN"B25"` from BASIC. On MiSTer select a Classic CPC configuration, identify
CRTC type 0 or 1 explicitly, select **Full** video mode, mount the DSK in A,
and issue the same command. Use normal CPC speed. Record the machine model,
physical CRTC marking or MiSTer type setting, MiSTer RBF identity, and DSK hash.
Do not infer the physical chip's type from this program. Other CRTC types and
Plus/GX4000 are outside this diagnostic's acceptance scope.

The program runs automatically. Video can lose sync briefly while R7=FF
suppresses VSYNC; wait for the restored numeric results screen. Photograph the
whole screen. Any keyboard key or joystick input, released then pressed, reruns
all 72 trials. Reset exits; this takeover program does not return to firmware.
It uses base 64K RAM and disables both ROM overlays.

Nine groups appear, each with eight columns: NOP, HALT, RET NC padding 0/1,
INC HL padding 0/1, and ADD HL,DE padding 0/1/2. Padding is that many 1-us NOPs
before the common sled, not an injected electrical INT delay. All numbers are
hexadecimal. `PC` displays the captured interrupt return address minus `&4000`;
`HL` is the register value saved before the capture ISR touches HL. INC and ADD
start from HL=0 with DE=1, so their HL values independently count the completed
one-byte instructions. RET NC runs with carry set. NOP and HALT leave HL=0;
HALT returns at `&4001`, so its displayed PC is 0001, not a count of HALT refresh
cycles. A model/hardware comparison must retain the complete phase group,
including trial variation and both controls. No number is labelled pass/fail.

## Memory and execution contract

- `&1000`: program load/entry; all code, strings and font end before `&3FFE`.
- `&3FFE..&3FFF`: two padding NOPs; `&4000..&4FFF`: one-byte instruction sled.
- `&8000`: status, 00 while running and A5 only after timing restoration/rendering.
- `&8100..&821F`: 72 records, each little-endian uint16 raw PC then uint16 HL,
  ordered by the nine groups above and then eight trials. Raw PC retains its
  absolute address; only the display subtracts `&4000`.
- Stack starts at `&A000`, screen at `&C000` in standard Mode 2 layout.

Each trial resets the GA counter, installs an IM1 arming handler, and executes
EI/HALT. Its real first interrupt discards the HALT return PC, installs the
capture handler, initializes HL/DE/carry and jumps through the common launch
sequence. The next real interrupt saves HL and the stacked PC, then returns to
the batch caller with interrupts disabled. Only the jump destination varies
between padding cases. RAM writes, opcode filling, table drawing and keyboard
polling occur outside the timed interval. R7 stays FF for the full batch, then
all normal CRTC timing registers are reprogrammed before drawing the results.

## Validation scope

`make -C sim classic-b25-test` assembles the actual disk artifact, independently
extracts B25.BIN using the existing SHAKER disk reader, verifies AMSDOS checksum,
length/load/entry and raw-body identity, then executes that extracted body on
production T80pa, GA40010 and CRTC0/1 with normal motherboard WAIT. The existing
harness's IRQ mask and snapshot-load inputs remain zero for every clock. A
small firmware-free bootstrap configures ordinary CRTC timing and varies CPU
launch phase through real instructions; it does not establish the measurement
phase. The fixture checks 144 actual bus acknowledgements per batch, capture
bounds, PC/HL instruction-count consistency, eight-trial stability and equality
across three launch phases per CRTC type. Absolute model counts are printed as
observations. RAM screen dumps are rendered into PGM images for visual QA.

This fixture does not model AMSDOS, floppy loading or keyboard scanning. A real
AMSDOS `RUN"B25"` boot remains a separate packaging acceptance check; the
completed AmSpirit runs are recorded below. Original hardware photographs remain necessary for B25's
phase question. The diagnostic changes no production RTL.

## Executed artifact evidence — 2026-09-29

The delivered DSK is 194816 bytes, SHA256
`90111197e72fdf424ff3b4fdb2311da7b08d5dbf19d77a6b580c31678a3b4db5`;
its program is 1822 bytes. AmSpirit Lite 1.15.1, core 2491682, cold-booted the
DSK through AMSDOS `RUN"B25"` on model 2 (6128), 128K, French ROMs, CRTC0
and CRTC1. Requested/effective settings matched; monitor Off, colour output.
Both runs reached status A5 and a readable full table, with eight identical
records per group. CRTC0 joystick-fire and CRTC1 space-key reruns each visibly
transitioned status 00→A5 and reproduced all 72 records. No RAM injection or
snapshot launch was used. The helper's checkpoint snapshots are evidence only.

The displayed PC deltas below are observations, **not expected hardware values**.
HL is zero for NOP/HALT/RET and equals PC delta for INC/ADD in every trial.

| Group | Production fixture CRTC0/1 | AmSpirit CRTC0 | AmSpirit CRTC1 |
|---|---:|---:|---:|
| NOP | 0CE2 | 0CE2 | 0CE2 |
| HALT | 0001 | 0001 | 0001 |
| RET NC pad0 | 0672 | 0672 | 0672 |
| RET NC pad1 | 0671 | 0671 | 0671 |
| INC HL pad0 | 0671 | 0672 | 0672 |
| INC HL pad1 | 0671 | 0671 | 0671 |
| ADD HL,DE pad0 | 044C | 044C | 044C |
| ADD HL,DE pad1 | 044B | 044B | 044C |
| ADD HL,DE pad2 | 044B | 044B | 044B |

INC pad0 differs by one accepted instruction in both emulator configurations;
ADD pad1 differs on AmSpirit CRTC1. These differences identify useful original
CPC comparisons. They do not establish which model is correct, identify raw
pin phase, or justify changing the production GA/CRTC/CPU. No original CPC or
MiSTer run was performed for this deliverable.

Reproducible AmSpirit cases and cold-boot manifests, screenshots, state dumps,
raw RAM records, rerun-control scripts/results and checkpoint snapshots are in
ignored `output_files/classic-b25/amspirit/`. Run either saved `crtc0.json` or
`crtc1.json` with `scripts/amspirit/amspirit.py run`, `--media-root .`, and a
fresh output directory. The case pins the DSK hash. Source inspection reports,
private page renders and fixture logs are under
`output_files/classic-b25/evidence/`; they are not redistribution artifacts.

## Independent review and acceptance boundaries

Opus 5.5 medium approved the final code in guarded review
`20260929T003303Z-59333-cc89` (exit 0, source review only). It checked the real
two-IRQ interval, carry/padding coverage, interrupt stack discipline, disk
header/directory layout and display bounds. No blocking findings. The supported
AMSDOS launch leaves the PPI in output mode before the first keyboard scan; an
arbitrary snapshot/register-state launch is not a supported delivery path.
The fixture is an on-demand slow diagnostic: its selection row follows the
diagnostic sources; existing classic IRQ benches retain production RTL change
coverage. It rebuilds the DSK deterministically in the default output directory.

An earlier review process (`20260929T002423Z-44266-1e2e`) disappeared without
output or a terminal status after a host interruption. It provided no review
evidence; the replacement started only after the bridge reaper confirmed no
live process. The valid review log and terminal state are preserved under
`output_files/classic-b25/evidence/`.

Final gate (2026-09-29):

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run --slow
```

Result: `select_tests: PASS 42 benches`, including
`classic-b25: PASS (CRTC0/1, three launch phases, 432 captures)`. The complete
command output and final selection line are preserved in
`output_files/classic-b25/evidence/selected-gate-llvm.log`. The first gate
attempt passed B25 but other benches could not compile with the host's system
C++ SDK (`cstddef` missing); passing CXX through recursive Make resolved that
host tooling issue without a code change. No code changed after the passing gate.
