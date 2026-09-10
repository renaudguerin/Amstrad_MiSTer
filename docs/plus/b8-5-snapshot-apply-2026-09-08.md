# B8-5: Plus snapshot owner apply

The parser restored ASIC storage while the DMA, selected CRTC/GA and ordinary
Plus ROM controls still retained reset state. B8-5 repairs the transaction from
storage drain to owner apply and CPU resume. The full agreed scope is now
implemented and the twelve focused restore cases pass. Full simulation, lint,
the unchanged classic soak and fresh independent review pass. The branch is
READY for integration, subject to the format and hardware limits below.

## Source and restore contract

The primary format is [Kevin Thacker's SNA specification](https://cpctech.cpcwiki.de/docs/snapshot.html),
checked in as `docs/references/Snapshot (.SNA) file format.md`.

| Serialized state | Format location | Required owner behavior |
|---|---|---|
| GA pen select, 17 hardware colours, RMR | Header 2E, 2F–3F, 40 | Restore selected GA shadows; initialize ASIC palette entries 0–16 only for a plain SNA |
| RAM configuration, ROM select | Header 41, 55 | Restore RAM mapping and ordinary Plus low/high ROM gates and upper ROM selection |
| CRTC select and R0–R17 | Header 42, 43–54 | Restore selected `asic_video`, using settled payload on the apply edge |
| HCC, character row, raster, VTA count | v3 header A9, AB, AC, AD | Restore mapped counters; CRTC3 uses its raster counter during VTA |
| HS/VS width counts and active flags | v3 header AE, AF, B0–B1 | Follow published flags: bit 0 VS, bit 1 HS, bit 7 VTA |
| GA delayed sync, line count, pending interrupt | v3 header B2, B3, B4 | Restore mapped phase/count/pending state and verify first post-apply transitions |
| Sprite pixels/attributes, 32 full palettes, ASIC control and DCSR | CPC+ 000–8DF | Retain the parser's restored storage; CPC+ palette apply must not impersonate legacy I/O |
| SAR/PPR for all three DMA channels | CPC+ 8D0–8DB | Reload live current addresses, not only stored bytes |
| DMA loop count/address, pause count/prescaler | CPC+ 8E0–8F4 | Three seven-byte records: LE16 loop count (12 meaningful bits), LE16 loop address, LE16 pause count (12 bits), prescaler8 |
| RMR2, lock state, unlock-sequence state | CPC+ 8F5, 8F6, 8F7 | Restore mapping/lock and next expected sequence state |

The CPC+ analogue input bytes 8C8–8CF represent external inputs; the existing
analogue input owner is outside this selected DMA/video/GA/MMU apply repair.
Keep that omission distinct from unspecified internal state.

The DMA pause field is a remaining prescaled tick count. The prescaler field
is its remaining phase/downcounter. The format's old uncertainty notes are
resolved by the [Arnold V PAUSE description](https://cpcrulez.fr/coding_cpcplus_arnold_v15.htm)
and [Arnold runtime](https://github.com/rofl0r/arnold/blob/e5dce08964f94add100f7db992a6d0e49fe74f01/src/cpc/asic.c#L1655):
PAUSE M delays M*(PPR+1)*64 us. The old Arnold loader skips the 21 internal
bytes and saves zeroes, so it is runtime-semantic evidence, not proof of a
working serialized restore.

B2 is a remaining delayed-HSYNC count: 2 at VSYNC entry, then 1, then 0 with
the reset action. [Caprice32 runtime](https://github.com/ColinPitrat/caprice32/blob/6c12c4c92360065cdc229ac9ada7551f941436b8/src/crtc.cpp#L618)
and its [direct B2 loader](https://github.com/ColinPitrat/caprice32/blob/6c12c4c92360065cdc229ac9ada7551f941436b8/src/slotshandler.cpp#L409)
resolve the format prose against its explicit 0-inactive/1-or-2-active values.
The old Arnold getter/setter invert this count without a zero special case;
do not copy that representation into the format interface.

8F7 names the next expected unlock byte: 0 waits for nonzero, 1 for zero,
2–E for FF,77,B3,51,A8,D4,62,39,9C,46,2B,15,8A; F waits for CD and 10 for
EE. Lock state 8F6 is independent. Old Arnold also swaps the header's HS/VS
flag constants and omits the terminal sequence state; follow the format and
original sequence specification rather than those inconsistencies.

## Apply boundary

Share the production drain/apply controller with the P8 fixture. The CPU
must remain unable to execute until parser/storage writes drain and the
owners consume one apply transaction. Release reset before T80 `DIRSet`,
but gate both T80pa clock enables through apply/settle: actual T80 reset
has priority over `DIRSet`, while `DIRSet` loads independently of CEN.

Use settled snapshot inputs, not same-edge updated GA/video outputs, for
mode and sync-history initialization. Restoring active HSYNC must not create
a false DMA line edge on the next clock. Discard unspecified pending memory,
LOAD/FSM and serializer transactions rather than allowing pre-restore work
to escape after resume.

B8-2/B8-3 owns FIELD and accepted legacy palette writes. Snapshot apply has
separate provenance: retain the full loaded 12-bit palette for CPC+, or
translate the header's hardware colours for a plain SNA. It emits no ordinary
legacy palette write event.

## Format limits

SNA v3 does serialize raster counters and flags; omitting them is not a format
limit. It does not serialize VMA/row-address latches, frame parity, C9 parity,
interlace pending events, or sub-character pipeline phase. Split history and
mid-frame R12/R13 writes make the missing address state non-unique. Arnold's
[header loader](https://github.com/rofl0r/arnold/blob/e5dce08964f94add100f7db992a6d0e49fe74f01/src/cpc/snapshot.c#L370)
sets the serialized counters after reset without reconstructing that history.

The agreed approximation is a deterministic address seed from settled
R12/R13, explicit reset defaults for unrepresented parity/private history,
and direct restoration of all mapped serialized counters/flags. It does not
promise exact first-frame pixels, monitor shaping, arbitrary SNA raster
fidelity or hardware closure.

## Where the restore lives

| Concern | Owner |
|---|---|
| Header decode (10, 2E, 2F–3F, 40, 42, 43–54, 55, A9–B4) | `rtl/plus/plus_sna_header.v` |
| Drain → apply → CPU-resume sequencing | `rtl/plus/plus_sna_apply.v` |
| CPC+ chunk unpacking into ASIC storage | `rtl/plus/plus_sna_parser.v` |
| Selected CRTC-3 register file and v3 counters | `rtl/plus/asic_video.v` |
| Legacy GA register file, sync/interrupt phase | `rtl/plus/asic_ga_timing.v` |
| Palette provenance (CPC+ 12-bit vs plain 5-bit) | `rtl/plus/asic_regs.v` |
| DMA live SAR/loop/pause/prescaler and HSYNC history | `rtl/plus/asic_dma.v` |
| RAM/ROM mapping, RMR2, lock, unlock sequence state | `rtl/plus/plus_mmu.v` |
| B4 interrupt-source attribution | `rtl/Amstrad_motherboard.v` |

The header decode moved out of `Amstrad.sv` on purpose. `Amstrad.sv` is neither
linted nor simulated, so an offset typo there would only ever surface in
hardware; `plus_sna_header` is linted and the P8 fixture drives it from a real
byte stream at real file offsets. `Amstrad.sv` keeps the Z80, PPI, PSG,
RAM-configuration and memory-size bytes, which are outside this slice.

## Decisions a reviewer should check hardest

**B2 → hcnt phase.** `asic_ga_timing` walks hcnt 00 → 01 → 06 on CRTC HSYNC
falling edges and raises the shaped monitor VSYNC (and the interrupt-counter
re-sync) at 06, so B2 maps to hcnt 00 / 01 / 06-or-parked-1E for 2 / 1 / 0.
`hcnt_next` is a registered successor and is seeded alongside `hcnt`.

**B4 attribution.** The field is one aggregate pending flag, but a Plus has two
INT sources. A pending flag the restored DCSR already explains is credited to
the DMA path, so the interrupt vector's source field stays right; only an
otherwise-unexplained flag is held by the Gate Array. The aggregate level on
INT_n is preserved either way. A simultaneous GA-and-DMA pending pair is not
representable and restores as DMA-only. Subsequent GA interrupts follow the
restored counter and ordinary runtime rules; there is no guarantee of recovering
the omitted pending GA interrupt or of doing so within one frame.

**DCSR bit 7.** Restored `dcsr_stat` carries the last-ack-was-raster
provenance during idle before the first acknowledge. The GA's own `last_raster`
level starts clear on `SNA_LOAD`. On the first acknowledge cycle, `dcsr_stat`
in `asic_regs` is retired to hand ownership of DCSR bit 7 to runtime GA
provenance (`intack_raster`). If the acknowledged interrupt was a restored
pending raster interrupt (`intack` with `!INT_N`), `last_raster` sets to 1 in
`asic_ga_timing` exactly as a normal fire would have set it. If the acknowledge
was for a DMA interrupt (or empty), `last_raster` clears to 0 on cycle
completion (`!intack && intack_d && ack_empty`). `cnt5` is seeded from the
restored counter for the same reason: without it, the counter's top bit would
appear to fall on the next clock and fabricate an interrupt.

**Approximation, not fidelity.** Mapped counters are restored exactly. The
video pointer, frame/C9 parity, interlace pending events, the pixel serializer
phase and the monitor sync-shaping phase are NOT serialized by the format;
they take the deterministic seeds described under "Format limits". First-frame
pixels are therefore approximate for an arbitrary mid-frame snapshot. R10/R11
stay read-only type-3 status groups with no storage, so their serialized bytes
are dropped rather than inventing writable cursor registers; R16/R17 do have
storage and are seeded.

## Independent-review P2 remediation (first ACK provenance)

Independent Astra review identified a first-ACK provenance failure on snapshot-restored
interrupts:
1. When restoring a pending GA raster interrupt (B4=1, B3=0, DCSR=0), `INT_N` was
   asserted directly on `SNA_LOAD`, bypassing `classic_fire` and `raster_fire`. The
   first GA acknowledge cycle retired the interrupt but never set `last_raster`,
   leaving DCSR bit 7 at 0 after a raster-sourced acknowledge.
2. Conversely, snapshot-loaded `dcsr_stat` in `asic_regs` was permanently ORed into
   DCSR readback and never retired at runtime. A restore attributing B4 to DMA with
   DCSR bit 7 set (e.g. DCSR=0xC1) correctly serviced the DMA interrupt, but DCSR
   bit 7 remained sticky 1 even after DMA acknowledge.
3. The P8 test fixture had tied `.intack_raster(1'b0)`, `.intack(1'b0)`, and
   `.int_pending(1'b0)` on `aregs` and `asic_ga`, preventing existing tests from
   exercising CPU acknowledge cycles.

**Remediation & verification:**
- `sim/plus/plus_p8_test_top.v` mirrors production motherboard acknowledge wiring
  (`cpu_ack = ~ga_m1_n & ~ga_iorq_n`, `ga_last_raster`, `~ga_int_n_out`, vector byte/valid).
- `test_b8_ga_interrupt_restore` in `sim/plus/plus_p8_test.cpp` verifies vector presentation,
  aggregate INT_n retirement, and DCSR bit 7 readback across both payloads. Restored DCSR
  bit 7 is verified during idle before acknowledge, and ownership transitions to runtime
  source tracking on first ACK.
- A red run was captured before production RTL changes (`docs/references/b8-5-recovery/b8-5-p8-ack-red.log`),
  failing on both counts.
- In `rtl/plus/asic_ga_timing.v`, `last_raster` is set to 1 on the first acknowledge
  of a restored pending raster interrupt scoped via a dedicated lifecycle latch
  (`sna_raster_pending`), preventing uninitialized simulator-startup `INT_N` levels from
  erroneously asserting raster provenance during initial empty acknowledges (`asic_pri` `pr01`).
- In `rtl/plus/asic_regs.v`, snapshot-loaded `dcsr_stat` is retired on the first acknowledge
  cycle (`intack && !intack_d`), allowing runtime `intack_raster` to report interrupt provenance.
- In `rtl/plus/plus_mmu.v`, documented unused/reserved bits of `sna_ga_config` are annotated
  with local Verilator `UNUSEDSIGNAL` pragmas to maintain clean module lint.
- All 12 B8-5 focused snapshot-apply tests pass (`docs/references/b8-5-recovery/b8-5-p8-b8-green.log`)
  and existing `asic_pri` tests pass cleanly (`docs/references/b8-5-recovery/b8-5-pri-green.log`).

## Acceptance evidence and limits

The temporary `--xfail` omission vector is removed. All twelve focused cases
(`--b8`) are required-pass checks on owner-visible state: CRTC register
readback, counters, sync levels, interrupt vector and retirement, DCSR readback,
and consumed palette RGB. They compile and pass, including repeated
CPC+/plain/CPC+ restores and first CPU activity after apply.

Slice B has a failure-first chronology gap: its worker changed production RTL
before obtaining an executable red run because its build commands were denied.
The parent subsequently compiled the new fixture against the prior video/GA/
palette owner bodies, retaining current port declarations only. All four slice
B cases and repeated-restore palette isolation failed. This demonstrates that
the assertions discriminate the prior omissions; it does not retroactively
satisfy failure-first ordering. The P2 acknowledgement remediation above has
separate, genuine pre-fix red evidence.

The first independent review found a missing closing `end` in `Amstrad.sv`
and the first-acknowledge provenance failure. Both are repaired. An explicit
production-top syntax check fails with the missing `end` restored and passes
with the repair. That check uses missing-module black boxes and waives existing
`mf2_store_addr` procedural-assignment diagnostics; it is not full Quartus
elaboration or synthesis evidence.

Final parent-run acceptance on 2026-09-08:

- `make -C sim`: exit 0, including all twelve B8-5 cases and the existing PRI,
  P10 concurrency, B7 audit, field and palette tests. Existing unrelated XFAILs
  remain; none was added or weakened for B8-5.
- `make -C sim lint`: exit 0.
- `make -C sim soak SOAK_EXPECT=0x6e8258198d6e6137`: exit 0, matching the
  recorded hash over 2,845,088 sampled characters.
- Fresh native Astra medium review of the foreign-authored complete branch:
  CLEAR, including the snapshot-origin acknowledgement latch and both original
  findings. The final code/manifests/tests diff against `98d5e07` has SHA256
  `6ef753f10807a4ea7b04525da5ab4c22192022ff41faec3ff3e6c4471c34f191`;
  the separately reviewed new header decoder has SHA256
  `64c720a8eb9c114135dc5c73e1b7c21eb9f4ec2e9d90dc4cccde483c45bb4bc5`.
  Documentation is excluded from the diff hash.

Hardware has not run; there is no hardware closure or promise of arbitrary
mid-frame pixel fidelity. Local red/green logs and reproduction scripts are
preserved in the ignored `docs/references/b8-5-recovery/` directory.
