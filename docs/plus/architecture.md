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
  ASIC's CRTC-3 is a *new* implementation informed by the Compendium's type-3 notes, reusing
  the Verilator testbench harness (same pin contract) with a type-3 rule set.

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

- **OSD**: extend the existing `Model` option (`P2O[5:4]`) or add a new one:
  `GX4000 / 6128+ / 464+` selecting `plus_mode` + RAM size + FDC + tape presence per the
  model table in the reference §12. Plus mode ignores the CRTC type toggle (ASIC = type 3).
- **CPR loading**: new ioctl index for `.cpr`. RIFF parsing is trivial and sequential —
  do it in the `Amstrad.sv` ioctl stream handler (skip `RIFF`+`Ams!` header, read `cbNN`
  chunk headers, route ≤16KB payloads to SDRAM cartridge slots NN, zero-fill short chunks).
  512KB cartridge region in SDRAM (the current map has room; snapshot/tape/disk paths are
  independent).
- **Boot**: no `boot.rom` in Plus mode. Reset vectors the Z80 to `&0000` with RMR2=0
  (cart page 0 low), `&DFxx`=0 (page 1 high on GX4000; page 3 if FDC model — reference
  §11). The system-cartridge firmware does the rest. Ship nothing: users provide a CPR
  (GX4000 games are self-contained; 6128+ needs a system cartridge image).
- **ACID**: not emulated, per universal emulator practice (reference §11) — CPR pages load
  and run unconditionally.

## 4. Phasing (each phase = usable milestone, separately testable)

| Phase | Content | Exit test |
|---|---|---|
| **P0 cartridge boots** | plus_mmu + CPR loader + OSD model + boot flow; video still via classic CRTC+GA as a stopgap; unlock FSM present but ASIC page not yet backed | GX4000 firmware/game reaches its first screen (most carts show *something* before touching sprites) |
| **P1 ASIC page + palette** | asic_regs (page RAM, unlock-gated RMR2), palette RAM both ports (page writes + legacy PENR/INKR translation table), 4-bit RGB path end-to-end | Titles with static Plus palettes show correct colours (Burnin' Rubber title) |
| **P2 interrupts** | PRI, IVR + vector supply, DCSR bit 7, MRER bit-4 clear, +32-line bit-5 rule; decide A13 bug = **not** emulated (document) | Raster-split games stable (Pang, RoboCop 2 use PRI heavily) |
| **P3 sprites** | sprite RAM, attributes, compare-and-composite engine, priority, magnification, access-blanking side effect | Sprite-based games (Switchblade, Copter 271, Klax) |
| **P4 CRTC-3 semantics** | replace stopgap CRTC in plus path with asic_video's own CRTC3: mod-8 reads, R12/R13 readable, both sync widths, VSYNC C0=C9=0 gate, +1µs alignment, IN-performs-write trap, open-bus reads | CRTC3 detection tests; SHAKER on CRTC3 setting |
| **P5 split & scroll** | SPLT/SSA capture at HCC=R1 (stored-MA model), SSCR H-delay/V-offset/border-mask | Plus demos & games using hardware scroll (Fluff intro screens etc.) |
| **P6 DMA sound** | asic_dma, PSG arbitration waits, PAUSE/REPEAT/LOOP semantics incl. undocumented &3xxx note | DMA-music titles/demos; DCSR polling tests |
| **P7 polish** | Plus-PPI quirks (port B input, port C latches — mod to `i8255` under plus_mode), ADC paddle stubs (wire defaults `3F 3F 3F 3F 3F 00 3F 00`), GX4000 `&DF=7→page 1` quirk, greyscale weights | compatibility sweep |

Ordering rationale: P0-P2 make *many* GX4000 games playable-ish before any sprite exists
(several titles degrade gracefully); sprites (P3) before CRTC-3 exactness (P4) because
visible progress beats invisible correctness for motivation, and nothing in P3 depends on
P4. P5 needs P4's stored-MA model, hence after.

## 5. Risks / open design questions (decide before P0/P1 coding)

1. **Z80 clocking & WAIT generation in Plus mode.** Classic mode gets CCLK/PHI enables,
   READY and RAS/CAS from the ga40010 netlist. asic_video must reproduce that timing
   contract (T80pa + sdram.v depend on it). Options: (a) replicate the ga40010
   syncgen/casgen timing behaviorally — the module boundaary already exposes exactly the
   needed signals; (b) keep ga40010 running in parallel *only* as a clock generator in Plus
   mode (feed it constant DISPEN/sync inputs, ignore its video). (b) is a cheap P0 stopgap;
   (a) is the honest end state. Measure drift: the ASIC's documented +1µs interrupt/colour
   deltas suggest Amstrad's own re-timing differed too.
2. **SDRAM bandwidth for sprites.** Sprite pixel RAM is 4KB — keep it in BRAM (dual-port:
   CPU via ASIC page, video side free-running). No SDRAM impact. Palette likewise (64B).
   The only new SDRAM client is DMA fetch (≤3 words per HSYNC, fits the video slot pattern)
   — verify against `sdram.v` slot allocation in P6.
3. **GX4000 clock (39.90257 vs 40 MHz)**: ignore (0.25%; MiSTer video pipeline normalizes;
   note in docs).
4. **A13 vector bug**: default = not emulated; revisit only if a title provably depends on
   it (none known — software uses the DCSR re-dispatch workaround which works either way).
5. **6128+ FDC**: reuse existing u765 path untouched (it's model-gated already for 664/6128).
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
Write them at phase start, not all upfront (the P0 experience will recalibrate P1+ scoping).
P0's package list, ready to be turned into prompts:

- P0.1 `plus_mmu.v` + motherboard mux + `plus_mode` plumbing from OSD (no ASIC page yet)
- P0.2 CPR ioctl parser in `Amstrad.sv` + SDRAM cart region + boot/reset state
- P0.3 unlock FSM in `asic_regs.v` (state machine only, gates nothing yet, +testbench)
- P0.4 Verilator bench scaffolding for the plus modules (clone of CRTC harness)
