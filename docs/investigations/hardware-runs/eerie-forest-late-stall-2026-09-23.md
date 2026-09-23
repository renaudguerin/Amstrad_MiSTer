# Eerie Forest late static frame: interrupt handshake discriminator

Investigation base: `b5bf5965470b85365d26158d44424439fc116b1e`.
The original, unchanged Eerie Forest CPR (SHA-256
`72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215`)
now boots after the CPR container compatibility repair. This record concerns a
separate freeze after the intro and landscape scene. No RTL repair is claimed.

## Device and emulator observation

The exact `41a1f27` full-effort RBF (SHA-256
`6c36309368659edbbfe1044e09a804639f6b7ec9c02b68526ff7d488e122b331`)
on MiSTer with 6128+ and Full sync reaches the landscape, then a monochrome
striped backdrop with a coloured character. Captures c6–c8 and a delayed repeat
are byte-identical. See the [original-CPR device record](plus-cartridge-originals-41a1f27-2026-09-23.md).
There is no symptom-relevant RTL change from that RBF to the base commit.

A fresh-core rerun with only sync filter bits 36:35 changed to Raw CRT (2),
retaining Plus model 6128+ (bits 34:33 = 2), reproduced the same static frame.
Three serial captures after a 28-second boot delay were identical, SHA-256
`a89fc9765f624c70d5e7d65e0c8813179779912b337373e502b0002ce7350e47`.
The original CFG was restored and checked as
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
The case, manifest and PNGs are ignored local evidence under
`docs/screenshots/eerie-late-2026-09-23/`. This excludes a Full-only display
acquisition effect, but a still capture cannot observe device CPU signals.

AmSpirit Lite 1.15.1 runs the same CPR through relative frames 1400 and 2000
with an active coloured landscape. Its frame-1400 state has PC `0x07A4`,
IFF1=IFF2=1, IM1 and RAM byte `0x0E04=0xFF`. These emulator checkpoints are
comparative evidence, not cycle-aligned MiSTer measurements.

The core's SNA save cannot supply a frozen Plus CPU snapshot: `save_admit` in
`Amstrad.sv` explicitly excludes `plus_mode`. An OSD save attempt left the DDR3
slot at generation zero. The temporary replay helper was removed from MiSTer;
the CFG restore hash above was rechecked.

## Production-path reproduction

Ignored scratch fixtures under `sim/plus/obj_dir/eerie_long/` and
`sim/plus/obj_dir/eerie_mode/` run the original CPR through generated production
T80pa, production clock/READY, ASIC/MMU and a behavioural SDRAM model. Their
Full-sync 6128+ run reaches a permanent CPU-level stall: at simulated second
24, about 676,731 opcode fetches and 2,150 interrupt acknowledgements still
occur; in seconds 26–28 the CPU remains in HALT, motherboard `INT_n` remains
low, and there are no further acknowledgements or RGB/VRAM frame changes.
Raw and selected VSYNC continue at 50/s. This is a model reproduction of the
static scene, not direct observation of the hardware CPU. The mode diagnostic
found zero active-display mismatches between the `MODE_SYNC_EN`-sampled GA mode
shadow and `asic_video.mode_q` through second 24, so the proposed mode-latch
gap is not implicated in this transition.

The focused trace halts at master tick `1541912250`, held next PC `0x06D3`,
SP `0x00DC`, IFF1=0, while actual `INT_n` is low. IFF1 last fell at tick
`1541398762` on an accepted raster-vector `0x06` interrupt at PC `0xC711`.
T80 clearing IFF1 on a maskable acknowledge is expected; the finding is that
this path reaches the later HALT without executing another EI. The HALT opcode
at `0x06D2` is also present in the AmSpirit RAM snapshot. Its branch at
`0x06B7` reads `0x0E04`: `0xFF` takes an explicit DI/restore/EI path that skips
HALT; other values wait while equal to `0x01`, then execute HALT. The simulated
physical base-RAM byte at the stall is **`0x0B`**, neither `0xFF` nor `0x01`.
An earlier scratch line reading `h.memory[0x0E04]` was invalid because the
SDRAM model uses physical address `0x20E04` for base RAM; do not reuse it.

The SDRAM write log gives the first narrow discriminator. At the fixture's
zero-tick start, `h.cycles=12714759`, so subtract that offset from write ticks
to compare them with the master-tick trace. The last normal write to physical
`0x20E04` before the accepted interrupt sets `0xFF` at master tick
`1539688326`. A unique final write sets **`0x0B` at tick `1541399254`**, only
492 master clocks after IFF1 falls and 512,996 clocks before HALT. A repeated
prior sequence had written `0x01, 0x10, …, 0xFF` many times; `0x0B` occurs only
once in this interval. The RAM `0x0038` interrupt routine begins `EXX; LD
(HL),C`, so that write is a strong candidate for its first store, but the
current trace does **not** record the writer's PC or alternate HL/C at that
edge. Treat attribution as a hypothesis until a transaction-level probe pins
it. The routine deliberately pops several context words and advances SP by
10 relative to its RST38 entry; that movement alone is not stack corruption.

Trace log SHA-256:
`8f61c7d372680fddc1e9a1f34439c2cce1dde996a854e7a76d03a52a6b892dae`.
It is preserved in ignored evidence as
`docs/screenshots/eerie-late-2026-09-23/d5-handshake-write.log`. The trace's
post-HALT bus-latch rings are **not valid pre-HALT data**: they were overwritten
during the two-million-clock wait before printing and mainly contain HALT
refresh cycles. Its physical RAM value and SDRAM write history are valid.

## Review and next discriminator

A read-only Opus 5.5 medium second opinion used the source tree but could not
read the `/tmp` trace, screenshot folder or private CPR in its CLI sandbox.
Its stack-overflow and DMA suggestions remain unproven; source inspection and
the trace weakened the simple stack reading. Bridge run
`20260923T073013Z-58880-9d4e` exited cleanly. No provider failure report is
needed.

At the final `0x0E04 <- 0x0B` write, capture the CPU write address, data, PC,
physical `ram_a`, ASIC-page ownership, alternate HL/C, SP, and the interrupt
handler's stack record/return target. Freeze the ring **at HALT entry** rather
than after a waiting interval. This should distinguish an intended context
message from an incorrectly timed or mapped acknowledge. Compare AmSpirit at
a corresponding handshake transition where possible. Do not infer an ASIC
timing repair from vector `0x06` alone: the separate B20-4 source/clock probe
finds normal-width PRI timing agrees with revised Arnold, while short-width
and seam cases remain independent questions.

If the first wrong transaction and a source-grounded Plus rule identify a
repair, write a focused deterministic vector that fails before the RTL change,
then run the selected gate once after the final edit and seek fresh
cross-provider review. The unchanged CPR, exact full-effort RBF and MiSTer
retest remain the title-level acceptance gate. Until then the late Eerie
freeze is open.
