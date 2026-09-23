# Eerie Forest: interrupt-context follow-up

This continues the [late-stall investigation](eerie-forest-late-stall-2026-09-23.md)
on Plus task `codex/plus/eerie-late-freeze`, based on
`7878f05e8ae0c5ae503fccd3451c341f3096166f`. The original CPR is unchanged
(SHA-256 `72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215`).
The evidence checkpoint is rebased onto coordinator master `d4f68eb`; its
intervening changes do not alter production RTL. No RTL repair or title closure
is established by the comparisons below.

## Fresh device comparison

A fresh load of the exact full-effort `41a1f27` RBF reproduced the frozen,
striped landscape on 6128+ with Raw CRT. All three native 848×287 captures
were visually inspected and have SHA-256
`a89fc9765f624c70d5e7d65e0c8813179779912b337373e502b0002ce7350e47`, identical
to the earlier Raw CRT capture. The unchanged RBF hash is
`6c36309368659edbbfe1044e09a804639f6b7ec9c02b68526ff7d488e122b331`.
There are no production RTL differences between that build and the task base.
The saved CFG was restored in a `finally` block and read back byte-for-byte;
SHA-256 `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.

AmSpirit Lite 1.15.1 / core 2491682, model 4 / CRTC 3, ran the same CPR after
a fresh load. At frame 8393, relative to load origin 6938, the settled image
shows a coloured landscape and character. CPU PC is `07A4`, SP `0066`,
IFF1=IFF2=1. Image SHA-256:
`12cc37725af6632c85ab4b9461312149fea48939dd72d8f1fbaa88b16aaffce1`.
This is a progression comparison, not cycle alignment or original-hardware
validation. MiSTer captures still do not expose the device CPU.

## Observed AmSpirit dispatch contract

Instruction stepping and CPU-view RAM reads establish the following for the
running original demo; these are observations of this executable, not general
ASIC rules:

- RAM `0038` contains `EXX; LD (HL),C; POP IX; POP HL; LD (6802),HL;
  POP HL; LD (6800),HL; POP DE; POP BC; RET`.
- Handler entries alternate stores to `0E04`, palette addresses such as
  `6400`/`6402`, and ASIC `6804`. Not every acknowledge should write the
  handshake byte. The stack holds deliberately constructed dispatch records.
- A sampled normal slot-`30` entry at PC `0039` has SP=`00D0`, HL=`0E04`,
  C=`30`. Executing that instruction changes the handshake from `28` to `30`.
  The POP/RET sequence reaches `02FF` with SP=`00DC`; `POP HL; EX DE,HL;
  EXX; EI; JP (IX)` restores the foreground and enables interrupts. Records
  can select other targets; this one is not a universal ISR exit.
- At `06B7`, a sampled `0E04=FF` takes `06BE`, loads the saved flags,
  executes DI / PUSH HL / POP AF / EI, and jumps over `06D2` to `06D5`.
  Thus a permanently halted IFF1=0 state is not explained merely by finding
  a HALT opcode in the demo.

The debugger breakpoint at `0038` did not stop the emulator in the sampled run;
`0039` did. Entry samples therefore observe the registers **after EXX**.
The saved step JSON identifies each PC, both register banks, stack bytes and
handshake value. Early exploratory samples that did not stop at `0039` are
not handler-entry measurements; use the explicitly named entry and slot-30
records.

## Earlier gap identified by the read-only consultation

Opus 5.5 medium read the staged prior write log in bridge run
`20260923T081702Z-74369-3ddc` (clean exit). Its CPR parsing was denied, so the
later directly sampled AmSpirit instruction evidence above supplies that gap.
The existing log shows the last `FF` write at master tick `1539688326`, followed
by `0B` at `1541399254`, a gap of 1,710,928 clocks. Normal consecutive `FF`
writes are 1,277,952 clocks apart in the final complete frames. The final `0B`
therefore lands near the usual slot-`30` phase after one whole missing frame
of handshake stores. This is comparative schedule evidence, not proof that
all interrupts in the gap were absent or that the final write is intended.

The old PC ring also reaches `5000` with IFF1=0, enters `0038`, later jumps
from `0506` to `0E04`, then enters `0038` again. Those entries must not be
counted as maskable acknowledgements without the bus evidence: a fetched
`FF` is a software `RST 38`. The old early-fetch `din` samples are insufficient
to prove the opcode actually consumed. The overwritten post-HALT latch ring
remains invalid for reconstructing earlier execution.

## Bounded trace: final writer and corrupted return code

The follow-up repeats the final physical `20E04 <- 0B` write at tick
`1541399254` and HALT entry at `1541912250`. The writer is now attributed:
CPU address `0E04`, output `0B`, held PC `003A` (the `0039` store),
SP=`00D0`, active HL=`0E04` and BC=`000B` after the handler's EXX. The
ASIC page is enabled but the address is outside it; cartridge ownership is
inactive. The final interrupt pushed `C711` at physical `200D0/200D1`.

The physical base-RAM dump at HALT reveals that the handler exit code differs
from the successful AmSpirit executable, not merely its register context:

| Address | AmSpirit byte/instruction | Frozen simulation byte/instruction |
|---|---|---|
| `02FF` | `E1`: POP HL | `2C`: INC L |
| `0302` | `FB`: EI | `F6`: OR immediate, consuming the following DD prefix |
| `0502` | `D9`: EXX | `F6` |
| `0503` | `FB`: EI | `F6` |
| `0504` | `18`: JR | `F6` |
| `0506` | `DD`: prefix for JP(IX) | `BD`: CP L; the following E9 is now JP(HL) |

The unchanged CPR independently contains these original sequences in chunk
`cb00`: the handler template is at offset `15DE`, the `02FF` sequence at
`18A5`, and the `0501` sequence at `1AA7` (consistent template base `15A6`).
The comparison therefore does not rely only on AmSpirit. The `0038` handler
bytes themselves still match. Its slot-30 RET record at
`00DA/00DB` still contains `02FF`, but that target no longer pops a word or
executes EI. This accounts for the one-word stack drift and missing EI. The
changed `0506` prefix also accounts for the jump to HL=`0E04`; it is not
proof that POP IX read back the wrong RST return address. The second Opus
consultation (`20260923T082755Z-80054-c0c7`, clean exit) preceded this frozen
RAM evidence, so its proposed RST push/read mismatch is superseded as an
explanation of that jump. The corrected wider trace below confirms execution from ASIC space, but
places the low-RAM overwrite earlier.

Probe limitation discovered during this run: production T80 takes **DInst**
at the beginning of T3 (`rtl/T80/T80.vhd`, M1/TState=2), whereas T80pa takes
**DI_Reg** at mid-T3. The inherited data-latch probe observes refresh addresses
on M1 and must not be labelled consumed opcode evidence. Its memory data and
physical write records remain useful. The wider follow-up separately captures
the opcode edge, sampled memory owner and post-edge IR, alongside non-M1 data
cycles. No production logic was changed to correct this diagnostic.
The bounded log SHA-256 is
`5002806d9d8f0fb9086f2faf5e8ac7f01ec39ad431bdca56904cfa3e98b4678c`;
the RAM dump is
`ccfe53f59231ca5a578d53d95555c689ec84dddfa89b571eadc4fc78e2cd8b91`.
Only written locations are used for the code comparison: this diagnostic
serializes absent sparse-model entries as zero, so its raw RAM dump is not
a faithful initialization image or a resumable machine snapshot.

AmSpirit escape-path control: after a fresh load, breakpoints at `4000`,
`5000` and `0E04` were armed at relative frame 1004. None fired through
relative frame 1707; the paused end PC is `3B11`, IFF1=IFF2=1. This bounded
comparison argues against an intended scheduler route, without proving
cycle alignment or excluding a later legitimate use.

## Correctly sampled trace: earlier corrupting execution

The completed wider trace samples DInst at its actual consumption edge and
records the post-edge IR, memory ownership, both general register banks, SP,
IFF, PRI, raster line and IRQ inputs. It reaches the same HALT tick and RAM
hash as the bounded run. The first overwrite of `02FF` is **08, not 2C**:

| Master tick | Observed event |
|---|---|
| `1538817410` / `1538817474` | `LD (06E3),IX` saves return target `0506`; foreground BC=`023B`, HL=`0061` |
| `1538818882` / `1538818946` | Context save stores BC=`023B` at `06D6/06D7` |
| `1539842778` / `1539842842` | Restored execution consumes intact `DD E9` at `0506`, then reaches renderer `3B3F` with BC=`023B`, HL=`0061`, SP=`0066`, IFF1=1 |
| `1539843094` | Physical renderer write `2023B <- 08`; subsequent writes advance through odd low addresses |
| `1539893338` | Consumed `02` at `3D5B`: LD (BC),A with BC=`02FF` |
| `1539893394` / `1539893398` | CPU write `02FF <- 08`, then physical SDRAM write `202FF <- 08` |
| `1539935578` | First observed ASIC-space opcode fetch at `6804`, returning FF and causing RST 38 |
| `1541355734` | Physical write `20302 <- F6` during later execution in ASIC sprite space |
| `1541377302` | Handler store finally overwrites `202FF <- 2C` |

Thus the `02FF` corruption is an actual CPU-directed write with matching
physical address and data, not evidence of an MMU Cxxx-to-0xxx alias. The
renderer resumes with a low destination before the first observed ASIC fetch.
The first ASIC entry is a RET through stack `0076/0077=6804`, following an
intact `JP (IX)` at `0506` to `0044`; execution at `4000` is not its origin.
These findings explain the later damaged exits but do not yet identify why
the earlier context saved `0506` or whether a scheduling/timing error caused it.

The final bounded AmSpirit comparison stopped at `06E2` on 64 restores,
frames 10100–10226 of the running instance. Saved BC is also `023B`, and
sampled foreground DE=`12D9`, HL=`0061`, SP=`0066`, IFF1=IFF2=1. However,
return targets are `079E` (29 samples), `07A4` (28), or `07A1` (7), never
`0506`. This rules out treating BC=`023B` alone as corruption. It is a
successful-context comparison, not an aligned replay of the failing frame.
AmSpirit was left paused with breakpoints cleared.

The focused next discriminator is the **context capture before tick
1538817410**: reconstruct how IX became `0506`, relate the interrupted
foreground and saved return to the intended `079E/07A1/07A4` loop, and compare
that transition with AmSpirit. Start with the existing
`context-save-excerpt.log` and full trace; do not repeat the full simulation
merely to rediscover downstream corruption. If additional capture is needed,
target that transition and record IX/IY explicitly. No narrow RTL fix or
source-derived failing regression has been established. The subsequent [pending-CPC investigation](eerie-forest-pending-classic-2026-09-23.md)
resolves this discriminator at the first startup exit, before the late trace window.

Wider trace SHA-256:
`6451cd8132b30d75eecde3a4503d32993c50fb09e7bfd17a763e82156604d749`.
Bus records use decimal master ticks and hexadecimal register/address values.
Deduplicate repeated ring-dump lines when counting events. `addr` is the
consumed opcode address; logged PC is post-fetch. `page` means ASIC-page
enabled, not cartridge bank. The inherited `di` field is not the opcode;
use `bus` and `ir` on `kind=OP`. `kind=DATA` is a non-M1 sampling event and
can also accompany writes; inspect bus controls/transaction context before
calling one a read. Physical write events supply independent pin-model evidence.

## Write-path boundary check

A read-only source comparison checked production `Amstrad.sv`, motherboard,
`Amstrad_MMU.v` and `sdram.v` against `p10_boot_test_top.v`, its physical
SDRAM pin model and the D5 adapter. Both normal Plus paths use the same MMU
and direct CPU address/data with the same ASIC-page suppression. In the
base mapping, logical Cxxx writes map to physical 2Cxxx, not 20xxx. The
fixture does not supply a Cxxx-to-0xxx conversion. Main-port write data is
sampled live at SDRAM STATE_CONT in both paths; actual address and payload
still need the transaction evidence.

The fixture omits outer reset boot writes and MF2/Dandanator routing. Those
are explicit coverage limits, not evidence of the cause during this ordinary
late Plus run. No simulations or implementation changes were made for this
read-only boundary retrieval.

## Private evidence and reproduction

Evidence is ignored under `docs/screenshots/eerie-late-followup-2026-09-23/`
in the task checkout. It includes `device-raw/manifest.json`, all three device
PNGs, `device-run.log`, original/restored CFG, `amspirit-landscape.png`,
`amspirit-state/`, `amspirit-slot30-steps.json`, `amspirit-slot30-ram.bin`,
`amspirit-06b7-steps.json`, `amspirit-routines.asm`, the handler-entry samples,
and the consultation output. Original old evidence remains in the
`eerie-forest-progress` checkout; the copied prior write log still hashes to
`8f61c7d372680fddc1e9a1f34439c2cce1dde996a854e7a76d03a52a6b892dae`.

The local diagnostic uses generated production T80, production clock/READY,
ASIC/MMU and behavioural SDRAM through the D5/P10 adapter. Its generated probes
and executable remain in ignored `sim/plus/obj_dir/eerie_mode/`. This follow-up
uses the fixture default `SYNC_FILTER=2` (Raw CRT), not an assumed Full setting.

The corrected probe and executable remain under ignored
`sim/plus/obj_dir/eerie_context/`. Probe sources, build descriptions and the
AmSpirit sampling helper are also copied to private evidence
`diagnostic-sources/`, so a simulation clean does not erase the next-session
starting point. `context-bus.log`, `context-ram.bin`,
`context-save-excerpt.log` and `amspirit-restore-targets.json` hold the latest
results. Both simulations and both consultations completed; no background
run remains part of this checkpoint. These are local diagnostics, not a
selected-test gate or physical-device CPU trace. This documentation-only
change does not require simulation or independent code review.
