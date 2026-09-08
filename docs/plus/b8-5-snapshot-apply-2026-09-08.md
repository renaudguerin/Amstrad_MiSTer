# B8-5: Plus snapshot owner apply

The parser restores ASIC storage while the DMA, selected CRTC/GA and ordinary
Plus ROM controls still retain reset state. B8-5 repairs the transaction from
storage drain to owner apply and CPU resume. This work is not yet accepted.

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

## Failure-first evidence and pending acceptance

On base `f0ed9b6121afc5078185171546d5ea5f723c376c`, P8 now composes the real
DMA, video and GA alongside parser/register/MMU owners. The focused binary
exits 1 without its temporary XFAIL switch: live SARs and first fetch remain
zero despite nondefault stored bytes, GA mode/border and CRTC remain defaults,
and ordinary ROM disables are lost. The parent reran this failure. The initial
ROM-page check while high ROM is disabled is insufficient; replace it with a
separate enabled-ROM transaction before accepting the regression.

This initial fixture still drives the apply seam manually, has no shared
header collector/controller, and does not yet prove full counter restore,
CPU hold ordering, active-HSYNC isolation, real palette/video consumption or
repeated restore isolation. Those are required implementation gates. The
worker's final textual offset summary was inconsistent with the primary
format and is not evidence; the map above is authoritative for this task.

Final acceptance requires required-pass focused vectors, full simulation,
lint and soak `0x6e8258198d6e6137`, followed by fresh Opus 5/high review of the
frozen diff. No hardware is available.
