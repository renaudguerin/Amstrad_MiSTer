# GX4000 / CPC Plus Support — Architecture & Phasing

Companion to `references/asic-reference.md` (facts live there; design reasoning lives here).
Target: add 464+/6128+/GX4000 emulation to this core. Everything below is grounded in the
current RTL (`Amstrad.sv`, `rtl/Amstrad_motherboard.v`, `rtl/UM6845R.v`, `rtl/GA40010/`).

## 1. What the ASIC replaces, and what that means here

The AMS40489 ASIC integrates: Gate Array + CRTC (type 3) + PAL/MMU + PPI + new features
(32×12-bit palette, 16 sprites, PRI, SPLT/SSCR, 3-channel DMA sound, cartridge paging, ADC).

The current core wires three separate modules in `Amstrad_motherboard.v`:

- `UM6845R` (behavioral CRTC, types 0/1) → `MA/RA/DE/HSYNC/VSYNC` →
- `ga40010` (netlist-derived Gate Array: pixel pipeline, 27-colour palette, R52 interrupts,
  Z80 clocking/WAIT, RAS/CAS) → 2-bit-plus-OE R/G/B outputs →
- `Amstrad_MMU` (ROM/RAM banking).

Three structural facts drive the whole design:

1. **`ga40010` is a netlist. It cannot be meaningfully extended** (no pen-index tap without
   fragile netlist surgery), and its colour output is already DAC-encoded 27-colour RGB —
   the Plus's 4096-colour palette cannot pass through it.
2. **The Plus video features are coupled to CRTC internals.** SPLT replaces the CRTC's
   line-start "stored MA"; SSCR offsets the RA output without touching the counters; sprites
   compare against the CRTC's VCC/RCC/HCC directly; PRI compares `{VC,RC}`. A sprite/palette
   overlay bolted onto the *output* of the existing CRTC+GA cannot implement SPLT/SSCR
   correctly.
3. **A locked ASIC behaves like a CPC** — Plus machines run classic software through the
   ASIC's own GA/CRTC emulation. So a Plus-mode core must reproduce classic behavior too,
   through whatever path implements Plus mode.

## 2. Chosen architecture: parallel behavioral video path, muxed at the motherboard

**Build one new module, `rtl/plus/asic_video.v` (behavioral: CRTC-3 + pixel pipeline +
palette + sprites + PRI + SPLT/SSCR), plus small siblings (`asic_dma.v`, `asic_regs.v`),
selected by a `plus_mode` input. Classic mode keeps UM6845R + ga40010 untouched.**

Rejected alternatives, for the record:

- *Extend UM6845R with a type-3 mode and overlay sprites after ga40010*: fails on fact 1/2
  above (no pen index, no palette width, SPLT/SSCR unreachable). Type 3 semantics
  (mod-8 register reads, R12/R13 readable, both sync widths programmable, VSYNC gated on
  C4=R7 ∧ C9=0 ∧ C0=0, +1µs HSYNC/display alignment) also diverge enough that stuffing a
  third personality into the 2-type behavioral model would degrade its readability for
  little reuse gain.
- *Reimplement the classic GA behaviorally and share one path for both modes*: throws away
  the gate-accurate netlist for the mode 99% of users run. Not acceptable.

Consequences to embrace explicitly:

- Plus mode's classic-compatibility (locked-ASIC games) rides on the new behavioral code,
  not on ga40010. It will initially be less accurate than classic mode. That matches real
  history (the ASIC's GA emulation itself has documented deltas: colours ~½µs late, PPI
  quirks, interrupt +1µs).
- The CRTC-accuracy work stream (docs/accuracy/) hardens `UM6845R.v` independently; the
  ASIC's CRTC-3 is a *new* implementation informed by ACCC v1.10's type-3 notes, reusing
  the Verilator testbench harness (same pin contract) with a type-3 rule set. The v1.10
  changes clarify CRTC0/2 behavior and do not change this type-3 architecture; see the
  [edition comparison](../accuracy/accc-1.10-differences.md).

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

### Module/bus sketch

```
Amstrad_motherboard.v
  ├─ plus_mode == 0: UM6845R + ga40010 (exactly as today)
  └─ plus_mode == 1:
       asic_video   — CRTC3 counters, MA/RA gen (SPLT/SSCR aware), DE, syncs,
       │              pen pipeline (mode 0/1/2), 32×12 palette RAM, sprite engine,
       │              PRI + 52-line interrupt, Z80 clock enables + WAIT (reuse the
       │              ga40010 timing contract: CCLK_EN_*, PHI_EN_*, READY, RAS/CAS
       │              or a simplified equivalent — see §5 Risk 1)
       ├─ asic_regs  — unlock FSM, RMR2, ASIC page decode (&4000-&7FFF): sprite RAM
       │              (4K×4 BRAM), attribute regs, palette RAM port, PRI/SPLT/SSA/
       │              SSCR/IVR, DCSR, ADC stubs; open-bus read rule
       ├─ asic_dma   — 3 channels: fetch during HSYNC slots, AY-list decode,
       │              PSG write mux + 8255/PSG arbitration WAITs, DCSR interrupts
       └─ plus_mmu   — cartridge paging (&DFxx pages 0-31, RMR2 low-ROM position/page),
                      ASIC-page priority at &4000-&7FFF, no write-through
```

Video output: widen the motherboard/top RGB path to 4 bits/channel (Plus native). Classic
mode maps its 2-bit+OE levels onto the same 4-bit bus (the 27-colour DAC levels are
0/50/100% → 0/8/15 approximately; do the conversion where `color_mix.sv`/`Amstrad.sv`
consume it today, and delete no classic code).

## 3. Loading, boot, and model selection

- **OSD**: add a separate `Plus model` option rather than extending the existing classic
  `Model` field: `Off / GX4000 / 6128+ / 464+`, selecting `plus_mode` + RAM size + FDC +
  tape presence per the model table in the reference §12. Plus mode ignores the CRTC type
  toggle (ASIC = type 3).
- **CPR loading**: new ioctl index for `.cpr`. RIFF parsing is trivial and sequential —
  do it in the `Amstrad.sv` ioctl stream handler (skip `RIFF`+`Ams!` header, read `cbNN`
  chunk headers, route ≤16KB payloads to SDRAM cartridge slots NN, zero-fill short chunks).
  512KB cartridge region in SDRAM (the current map has room; snapshot/tape/disk paths are
  independent).
- **Boot**: no `boot.rom` in Plus mode. Reset vectors the Z80 to `&0000` with RMR2=0
  (cart page 0 low). The high window starts on page 1 for GX4000; on 464+ and 6128+ the
  external `/EXP` state dynamically selects page 1 or page 3 (reference §11). P0 must
  define how that input is supplied and sampled before it implements reset mapping. The
  system-cartridge firmware does the rest. Ship nothing: users provide a CPR (GX4000 games
  are self-contained; 6128+ needs a system cartridge image).
- **ACID**: not emulated, per universal emulator practice (reference §11) — CPR pages load
  and run unconditionally.

### Cartridge SDRAM contract (P-1)

Cartridge bytes occupy SDRAM bank 3, byte addresses `0x080000..0x0fffff`. The cartridge
memory service converts a page and offset to `{4'b0001, page[4:0], offset[13:0]}` and passes
that 23-bit address plus bank 3 to `sdram`; the controller does not reinterpret or bound the
mapping. This keeps all region policy in the service and the physical SDRAM client generic.

The synchronous cartridge port is a held request/acknowledge interface. `cart_req`, write
direction, bank, address and write data remain stable until the rising edge carrying
`cart_ack`. Read data is valid with that acknowledge. A requester may keep `cart_req` high
and change the other fields after the acknowledged edge for the next transfer; the
controller acknowledges each transfer exactly once.

At each SDRAM slot the order is: a new main CPU/chipset edge, an overdue forced refresh,
cartridge, tape, video, then ordinary refresh when no client is selected. After 32
cartridge grants without a refresh, one no-client refresh slot becomes due. A simultaneous
main edge still wins, but does not clear the pending refresh. Initialization admits no
client. Until P0 connects the cartridge service, `Amstrad.sv` ties the new port inactive,
so the classic client behavior and SDRAM map are unchanged.

## 4. Phasing (each phase = usable milestone, separately testable)

Two integration gates precede P0. **P-2** adds a separate `Plus model` OSD field (`Off`,
`GX4000`, `6128+`, `464+`) and derives model capabilities without changing the existing CPC
`Model` field. **P-1** fixes and tests cartridge SDRAM ownership: one 512KB region, one
page/offset address conversion, and explicit arbitration between ioctl loader writes and CPU
reads. CPR parsing must not begin until that contract is accepted. See
`../implementation-roadmap.md` for their deterministic exits.

| Phase | Content | Exit test |
|---|---|---|
| **P0 cartridge boots** | `plus_mmu` + CPR loader connected to the already-tested cartridge memory service + boot/reset state; video still via classic CRTC+GA as a temporary stopgap; unlock FSM present but ASIC page not yet backed | Parser vectors cover chunk order, short-page zero fill, bounds, and malformed input; GX4000 firmware/game reaches its first screen |
| **P1 CRTC-3 timing foundation** | `asic_video` CRTC3 counters, MA/RA, DE, both sync widths, VSYNC C0=C9=0 gate, +1µs alignment, basic locked-ASIC pixel path, and the CPU/WAIT timing contract needed by later raster consumers; exact register readback quirks may remain pending | Cycle assertions for counter rollover, sync/DE/MA/RA timing, classic-palette pixel output, and stable CPU/SDRAM handshake; locked-ASIC title reaches the same boot point as P0 |
| **P2 ASIC page + palette** | `asic_regs` page decoder, unlock-gated RMR2, sprite/attribute backing RAM, palette RAM both ports, legacy PENR/INKR translation, and 4-bit RGB path; implement `8'hFF`-neutral wired-AND participation, explicit open-bus responses, per-register read/write masks, and no write-through to underlying RAM | Exhaustive page decode/read/write/mirror/mask/open-bus tests; static Plus palettes display correctly (Burnin' Rubber title) |
| **P3 interrupts** | PRI driven from P1's CRTC3 counters, IVR + vector supply, DCSR bit 7, MRER bit-4 clear, +32-line bit-5 rule; A13 bug remains deliberately not emulated unless evidence changes the decision | Exact-cycle PRI/vector/DCSR assertions; raster-split games stable (Pang, RoboCop 2) |
| **P4 sprites** | sprite compare/compositor driven from P1's counters, priority, magnification, and access-blanking side effect | Per-pixel overlap/priority/magnification/blanking assertions; Switchblade, Copter 271, and Klax sprite smoke tests |
| **P5 CRTC-3 bus semantics** | modulo-8 reads, R12/R13 readable, status groups, unmapped/open-bus reads, and IN-performs-write traps on CRTC/GA ports; timing remains owned by P1 | CRTC3 detection and bus-trap tests; SHAKER on its CRTC3 setting |
| **P6 split & scroll** | SPLT/SSA capture at HCC=R1 using P1's stored-MA model; SSCR H-delay/V-offset/border-mask | Exact capture/offset assertions; Plus demos and games using hardware scroll (Fluff intro screens etc.) |
| **P7 DMA sound** | `asic_dma`, SDRAM fetch slots, PSG arbitration waits, PAUSE/REPEAT/LOOP semantics including the undocumented &3xxx note | DMA instruction/DCSR/WAIT tests; DMA-music titles/demos |
| **P8 polish** | Plus-PPI quirks (port B input, port C latches — mod to `i8255` under `plus_mode`), ADC paddle stubs (wire defaults `3F 3F 3F 3F 3F 00 3F 00`), GX4000 `&DF=7->page 1` quirk, greyscale weights | Model-by-model compatibility sweep plus classic-mode regression |

P1 deliberately precedes PRI and sprites: those units consume CRTC counters and edge timing,
so implementing them against the classic stopgap would create a second timing contract to
remove later. P5 separates exact CRTC3 readback/I/O quirks from that foundation because they
do not feed PRI or sprite positioning. P6 still follows the stored-MA model established by
P1.

## 5. Risks / open design questions (decide before P0/P1 coding)

1. **Z80 clocking & WAIT generation in Plus mode.** Classic mode gets CCLK/PHI enables,
   READY and RAS/CAS from the ga40010 netlist. asic_video must reproduce that timing
   contract (T80pa + sdram.v depend on it). Options: (a) replicate the ga40010
   syncgen/casgen timing behaviorally — the module boundaary already exposes exactly the
   needed signals; (b) keep ga40010 running in parallel *only* as a clock generator in Plus
   mode (feed it constant DISPEN/sync inputs, ignore its video). (b) is a cheap P0 stopgap;
   (a) is the honest end state. Measure drift: the ASIC's documented +1µs interrupt/colour
   deltas suggest Amstrad's own re-timing differed too.
2. **SDRAM bandwidth for sprites and DMA.** Sprite pixel RAM is 4KB — keep it in BRAM
   (dual-port: CPU via ASIC page, video side free-running). Palette likewise (64B). The
   tied-off cartridge client now has an explicit held-request slot and forced-refresh guard.
   DMA will be the next client; verify its ≤3 words per HSYNC against the accepted
   cartridge/main/tape/video arbitration instead of assuming an unused slot.
3. **GX4000 clock (39.90257 vs 40 MHz)**: ignore (0.25%; MiSTer video pipeline normalizes;
   note in docs).
4. **A13 vector bug**: default = not emulated; revisit only if a title provably depends on
   it (none known — software uses the DCSR re-dispatch workaround which works either way).
5. **6128+ FDC**: reuse the existing u765 implementation, but add explicit Plus-model
   selection when P0 enables behavior. The current top-level port decode is controlled by
   the drive-disable option, not by the classic model field, so do not assume model gating.
6. **Where does `plus_mode` snapshot support land?** SNA v3 has no Plus state; punt —
   document "no snapshots in Plus mode" initially.
7. **Reference gaps** (reference §15): PRI fire offset 6 vs ~10µs and sprite +3 write
   mirror need hardware measurement eventually — implement [ARNOLD-REV] values, tag with
   `// ⚠ ASIC-REF §15` comments so they're greppable when contradicted.

## 6. Testing strategy

- Extend the Verilator harness (docs/accuracy/testbench-spec.md) with an `asic_video` bench:
  the same script format drives ASIC-page writes; assertions on pen/RGB output per cycle,
  sprite compositing, PRI timing. The unlock FSM and CPR paging are pure-logic — unit-test
  them exhaustively in simulation before any hardware build.
- Acceptance titles per phase are listed in the phase table. Plus-specific test software:
  Longshot/community CRTC3 tests, `GX4000 diagnostics` cartridge images, and the usual
  suspects (Pang, RoboCop 2, Switchblade, Burnin' Rubber ships inside the system cart).
- MAME `amstrad_m.cpp` / `amstrad_v.cpp` is the behavioral cross-reference when the
  written sources conflict — it runs the full commercial library.

## 7. Deliverable-sized work packages (for implementation agents)

Each phase above decomposes into prompts the way `docs/accuracy/audit-findings.md` does.
Write later-phase prompts at phase start, after the preceding interface has been measured.
The pre-P0 and P0 packages are:

- P-2.1 **complete** — a non-overlapping `Plus model` field decodes once into `plus_mode`
  and explicit model capabilities; all selector values are unit-tested
- P-2.2 **complete and behaviorally inert** — capabilities reach the motherboard boundary
  without selecting Plus hardware or changing classic model paths
- P-1.1 **complete** — the cartridge SDRAM region, page/offset function, held request/ack
  contract, arbitration, refresh fairness, and loader/CPU ownership are documented and
  tested; Dandanator uploads are bounded below the reserved region
- P-1.2 **complete as a tied-off foundation** — the atomic cartridge memory service and real
  service-to-SDRAM integration are tested; production top-level hookup waits for P0
- P0.1 `plus_mmu.v` + motherboard memory mux and cartridge page/reset mapping
- P0.2 CPR ioctl parser in `Amstrad.sv`, using only P-1's memory-service interface, with RIFF
  and chunk validation plus short-page zero fill
- P0.3 **leaf complete, integration pending** — the unlock FSM is exhaustively tested but
  does not yet gate an ASIC register page
- P0.4 Plus bench scaffolding and the P0 boot integration test

For every package, add each new synthesizable source to `files.qip` in the same commit that
first instantiates it, and add it to the simulation file list. The package is incomplete if
Verilator passes but Quartus cannot discover the module. Keep decoder/open-bus/write-mask
tests with P2 and CRTC counter/timing tests with P1; do not defer those contracts to the
title-level smoke tests.
