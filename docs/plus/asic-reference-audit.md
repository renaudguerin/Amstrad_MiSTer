# Plus ASIC Reference Traceability Audit — Clause Table & Candidate Findings

Audited: [`references/asic-reference.md`](references/asic-reference.md) (the implementation
reference digest) against `rtl/plus/*.v`, the Plus paths of `rtl/Amstrad_motherboard.v`,
`rtl/i8255.v` and `Amstrad.sv`, and the `sim/plus` benches. Base: `master` at `80c81a1`
(2026-09-27), which includes the terminal-line SSA capture fix `edd6d80` / `t08l`.

Trigger: original-Plus probe V3 screen 21 exposed a clause that the reference had quoted
correctly for weeks ("SSA captured when HCC == R1 — **or when** HCC == R0 if VCC==R4 and
RCC==R9") but the RTL implemented only in its main half. The existing split vectors
(t08a-d, t08k) were derived from observed effects, so none reached the exception. This audit
enumerates every normative clause of the reference, splits each rule from its exceptions,
and records where each one is implemented and which test, if any, drives that exact
condition.

Authority: hardware (original Plus photographs and connected probes) outranks every written
source, and the reference is itself a digest of [ARNOLD], [ARNOLD-REV], [KT], [QUASAR] and
[MAME]. A row marked *contradicted* therefore means "RTL and reference disagree", not "RTL is
wrong": several rows below are cases where hardware has already overruled the text and the
reference is what needs editing. Every RTL change taken from this table needs a fail-before
vector (AGENTS.md) and, where the rule is source-only, a hardware probe first.

Method: five read-only section audits (§1-4, §5-6, §7, §8+§13, §9-12) enumerated clauses and
proposed RTL/test locations; every row not marked `impl+tested`, and the RTL line of every
candidate finding, was then checked by hand against the RTL at `80c81a1`. Test names are
bench functions in `sim/plus/*.cpp`; the selecting `make` target is in `sim/TESTS.md`.

## How to read this document

Status values:

| Status | Meaning |
|---|---|
| `tested` | RTL implements the clause and a named test drives that exact condition |
| `untested` | RTL implements it; no test reaches the condition (a main-rule test does not count for its exception) |
| `not-impl` | No RTL implements it |
| `contradicted` | RTL does something different from the clause |
| `conflict` | The sources disagree (recorded by the reference, or found here) and the RTL has picked one reading |
| `scope` | Deliberately not modelled: the reference says ignore/do not emulate, or it is software advice, a physical property, or outside the core |

Candidate findings (`PA*`) collect every `not-impl`/`contradicted` row that could need an
RTL change, plus test gaps that meet the keep rule. Reference corrections (`RC*`) collect
rows where the RTL is right (usually hardware-confirmed) and the reference text is stale or
wrong; they are documentation-only. Clause IDs are `§section.index`.

---

## Candidate findings

Ordered by expected value (impact × confidence ÷ cost of evidence). The descriptions
identify gaps at the audited base; the clause table and execution status record
subsequent coverage and evidence.

### PA1. `IN` on the Gate Array port writes the last opcode byte on every model; [KT] measured &79 on a 6128+

- **Clause** (§4 l.166-173, §14.6; [KT] "Ports"): an `IN` on a write-only Plus port performs
  the write with the open-bus byte. [KT] measures `IN A,(C)` (`ED 78`) as always writing
  `&79` on a 6128+ and `&78` on a 464+ ("Into the Eagles Nest": green border on 6128+,
  purple on 464+). The same text gives R6 = `&79`/`&78` for an `IN` on `&BDxx`.
- **Current**: `Amstrad_motherboard.v:290-297` latches `io_bus_byte` on every M1 opcode
  fetch; `plus_io_data` (`:288`) feeds it to the Plus I/O decoders for reads. For `ED 78` the
  last M1 byte is `&78` on every model. At the audited base, the only bench `&79` was synthetic:
  `t80pa_bench_cpu.v` step 48 seeds it directly. V4 screen 26 now executes real
  `IN` instructions through the production CPU; its hardware result remains pending.
- **Read side**: the CPU readback is separate from the decoder write value. The inactive
  SDRAM port exports `FF` (`rtl/sdram.v:88`), rather than its retained internal byte, to
  `Amstrad.sv`'s ordinary CPU bus. V4 screen 26 traces real production-T80 instructions:
  all seven CPU reads return `FF`, while the palette follows the opcode. This corrects
  the original audit's stale-byte inference (§4.09); hardware remains pending.
- **Why this matters**: the 464+ matches, the 6128+ does not. The reference's own "last byte
  of the instruction" wording predicts `&78` too, so the reference is internally inconsistent
  for the 6128+ and the RTL followed the general wording. The model-specific `&79` is a
  measured [KT] claim with no mechanism given (a bus pull-up difference, or a different last
  byte on the bus).
- **Impact**: the "Into the Eagles Nest" bottom border on 6128+; any detection code that
  reads the value back.
- **Confidence**: medium. The RTL/[KT] disagreement is certain; the value is one measurement
  from one source.
- **Next step**: probe on the original Plus: `IN A,(C)` to `&7Fxx` after selecting the border,
  plus `IN r,(C)` with other `r` (`ED 40..78`) to see whether the value tracks the opcode (the
  RTL model) or is a constant per model. Then a fail-before vector in `p1_mobo_bench` with a
  real instruction stream, not a seeded bus byte.

### PA2. Screen split during vertical adjustment is blocked on every adjustment line

- **Clause** (§8 l.409-410, [ARNOLD-REV]): "A split can occur during the **first** char-line of
  vertical adjust, but not later ones."
- **Current**: `asic_video.v:556`, `split_latch_event = CLKEN && !in_adj && split_match && …`
  suppresses every in-adjustment match. `source-divergences.md` already lists this as open
  ("its boundary wording is inconsistent"); the reference does not say the RTL deviates.
- **Reading to test first**: since `t08l`, a terminal-line match (C4=R4, C9=R9) captures at
  C0=R0, which with R5>0 is the edge that starts the first adjustment line, so SSA already
  drives that line. [ARNOLD-REV]'s sentence may describe exactly this, not a SPLT value
  matching `{R4[4:0], adj_index}` inside adjustment. That is inference; nothing probed
  distinguishes them.
- **Tests**: no vector combines R5≠0 with a split.
- **Confidence**: low for any specific fix. **Next step**: a probe screen with R5≥2 and
  SPLT = `{R4,0}`, `{R4,1}` and the terminal line, on the same cartridge as PA3.

### PA3. Terminal-line split with R5>0: SSA covers the adjustment lines but not frame line 1

- **Clause** (§8 l.393-407): SSA "is used from the next scan line onward … until the next split
  or frame restart"; it is never applied at VCC=0/RCC=0 and "takes effect at the next
  opportunity, e.g. VCC=0/RCC=1" (probe 11, R5=0).
- **Current**: with R5=0 the C0=R0 capture coincides with the frame origin and
  `!split_latch_event` (`asic_video.v:600`) keeps SSA in VMA', so frame line 1 shows SSA
  (`t08k`, probe 11). With R5>0 the capture happens at adjustment entry instead:
  `split_held` is cleared by the `hcc_last` edge (`:589`), the adjustment lines redisplay SSA
  from VMA', and the later frame origin reloads VMA' from R12/R13 (`:599-601`), so frame
  line 1 does **not** show SSA.
- **Status**: `untested`, and the reference is ambiguous. "Until frame restart" supports the
  RTL; "next opportunity, VCC=0/RCC=1" supports carrying the split across the frame origin.
  Only the R5=0 case is hardware-evidenced.
- **Next step**: add R5>0 to the PA2 probe. Qualify §8 l.402 as R5=0 until then.

### PA4. Unmapped and write-only ASIC-page reads return &FF, not the instruction's last byte

- **Clause** (§4 l.165-169, §14.5): unmapped ASIC-page areas and the write-only `&6800-&6807`
  read "the last byte of the instruction performing the read" (`LD A,(&5000)` → `&50`);
  needed by some detection code.
- **Current**: unused/write-only addresses leave `plus_asic_rd` inactive. ASIC-page
  selection also suppresses SDRAM OE in `Amstrad.sv`, so `rtl/sdram.v:88` exports `FF`
  into the ordinary CPU bus. V4 screen 29 confirms four real production-T80 reads return
  `FF`, with independent mapping/RAM controls. The ASIC module's neutral contribution
  alone was not enough to establish this full-path result. For `LD A,(nn)`, the last
  instruction byte is the non-M1 operand high byte; the motherboard's M1-only latch
  would not implement that source reading. Hardware evidence must precede any fix.
- **Tests**: `a06_open_bus` pins the `&FF` neutral contribution, which is the assumption,
  not the clause.
- **Confidence**: medium (two sources, [ARNOLD-REV §2.16] and [KT]). No title is known to
  depend on it. **Next step**: title-driven, or a one-screen probe reading `&5000`/`&6800`
  with `LD A,(nn)` and `LD A,(HL)` (after `LD A,(HL)` the value would be the `7E` opcode).

### PA5. Hardware-matched SSCR/split interactions with no vector

Both rows pin a cross-interaction on shared pointer state and are photographed on an original
Plus, so they meet the keep rule. Expect them to pass on current RTL: they are regression
armour for hardware-confirmed behaviour, not findings.

- **PA5a — SSCR row capture with R9<7** (§8 l.431-437): `row_latch_done` is the level test
  `ra_eff >= R9` (`asic_video.v:543`). With R9=3 and offset 2 it captures on raw rasters 1-3
  (probe V3 screen 25, AmSpirit identical). `t08e` runs R9=3 but asserts only RA, not MA.
  Add the MA assertions. The `t08j` header comment ("R9 < 7 with an offset … is unprobed",
  `asic_video_test.cpp` ~l.2983) is stale; fix it in the same commit.
- **PA5b — V-scroll offset cancels a held terminal-line split** (§8 l.443-444, "when V-scroll
  == 7 it takes precedence over the split-wrap bug case"): not a special case in the RTL. On
  frame line 0, `ra_eff = offset >= R9` fires the row capture at C0=R1, which overwrites the
  VMA' split held from line 311 (`:584-587`). Probe V3 screen 23 (offset 7, R9=7) matches:
  red to line 55. `t08k` never sets SSCR. Add one `t08k` sub-case with SSCR=`&70` (frame
  line 1 = R12/R13+R1, not SSA). The general rule is "any offset whose line-0 `ra_eff`
  reaches R9"; restate §8 l.444 in those terms (RC3).

### PA6. Implemented rules with no vector, worth one each

Each is a boundary or cross-module rule that a routine edit in a shared block could break:

- **PA6a — lock keeps the ASIC page mapped** (§1 l.54-59): `asic_page_on` changes only in the
  unlocked RMR2 arm (`plus_mmu.v:236-240`), so re-locking with the page on leaves it on and
  a locked `101xxxxx` cannot clear it. `test_rmr2_locking_positions_pages` covers locked MRER
  aliasing and unlocked RMR2, but never re-locks with the page on. Pins the unlock/MMU
  interaction (`asic_unlock` feeds `plus_mmu`).
- **PA6b — ASIC-page writes do not reach RAM underneath** (§2 l.94-97): `Amstrad.sv:826-827`
  drops SDRAM `oe`/`we` while `plus_aspage_sel`. No motherboard bench writes `&4000-&7FFF`
  with the page on, turns it off, and reads RAM back. Cross-module (top-level SDRAM gating vs
  `plus_mmu`).
- **PA6c — REPEAT 0 is a NOP** (§9 l.490-492): `asic_dma.v:438` (and the channel 1/2
  copies) guards `REPEAT 0` alone. This is the reference's chosen reading of a live source
  conflict, so it should be pinned before anyone "fixes" it toward the French tutorial.
  `d04` pins N+1 and `d15` pins PAUSE 0, but no vector uses REPEAT 0.
- **PA6d — 464+ tape gating** (§12 table): `Amstrad_motherboard.v:1163,1196-1197` gate
  `tape_in`/`tape_motor`/`tape_out` on `has_tape`. `test_p10c_fdc_motor_tape_gating` has only
  FDC/motor vectors.
- **PA6e — interrupt priority and the bit-5 clear** (§7 l.322-324, l.346): the vector
  source chain raster > DMA2 > DMA1 > DMA0 (`asic_regs.v:678-685`) and the PRI fire clearing
  GA counter bit 5 (`asic_ga_timing.v:532-536,543`) are both implemented, but no vector
  raises two sources at once or re-enables the CPC interrupt after a PRI fire. Both pin
  interactions between `asic_ga_timing` and `asic_regs`. The DMA1 source code (`010`) is also
  never asserted; fold it into the same vector.

Not proposed (fail the keep rule, structural one-liners): SPLT n+256 alias with C4≥32, select
31 reading R15, DCSR enable re-write no-op, second REPEAT overwriting the loop context.

### PA7. GA-compatible interrupt timing: "~1µs later than a real CPC" is not modelled

- **Clause** (§7 l.327-328, [QUASAR] only): even with PRI=0 the Plus interrupt arrives ~1µs
  later than on a CPC.
- **Current**: the PRI=0 path is the GA40010-derived counter in `asic_ga_timing.v`, pinned in
  lockstep against the classic GA by `asic_ga_timing_diff_tests`, so it asserts on the same
  edge as a CPC. That bench makes a Plus-only delay an explicit design decision, not an
  accident.
- **Status**: `not-impl`. **Confidence**: low (single source, no stated reference point).
  [KT]'s "colour changes ½µs later" and "some CRTC changes 2µs later" (§12 l.645) are the
  same kind of claim and are also unmodelled. **Next step**: use the
  [V5 hardware results](pa7-interrupt-phase-followup.md#original-6128-plus-results-2026-09-28)
  and the isolated shared-T80 late-sampling finding before selecting an IRQ timing fix.

---

## Reference corrections (documentation only; applied 2026-09-28)

The RTL is right, or at least hardware-matched, in every row here. RC1-RC8 have been applied
to the reference digest; none needs a gate or review.

- **RC1 — §9 PPR paragraph corrected** (l.493-498): [ARNOLD-REV] and the captured French DMA
  tutorial p.4 say changes take effect immediately, including mid-pause. Since B20-1
  ([live-ppr-2026-09-22.md](live-ppr-2026-09-22.md)), a CPU PPR write ends the current
  prescaler interval (`asic_dma.v:167-175`, `:274-286`; one-shot `pprN_wr` from
  `asic_regs.v:470-481`), covered by existing `plus_p8` b20_02/b20_03.
- **RC2 — §13 Status 1 wording corrected** (l.669): ACCC v1.11 FR §21.3.4 p.248 gives
  C0=R2+R3, the character after HSYNC. `asic_video.v:953-955` uses raw R3 low bits, so
  R3=0 compares at C0=R2; it does not expand to a 16-character width. The row also records
  bit 2's R0≥R1 gate.
- **RC3 — §8 SSCR rule qualified** (l.432-444): the R9=11 probe shows three source rows can
  advance per character row, so the old "correct for R9 ≥ 7" wording was too broad. The
  reference now states the confirmed R9=7 full-screen case and the effective-RA row-capture
  condition; it does not make scroll offset 7 a unique trigger.
- **RC4 — SPLT 55/56 conflict marker removed** (l.387-388, §15; also
  `asic-documentation-gap-map.md`): probes 10-13 settle the terminal-line value at 55. The
  reference and gap map now record that result and the unprobed R9>7 double-match case (raw
  rasters r and r+8, because only RC2..RC0 is compared).
- **RC5 — §5 sprite X formula qualified** (l.202-204): [KT]'s `char = (X & &FFF8)>>3` means
  8 X units per CRTC character, while X is in mode-2 pixels (16 per character) and the same
  paragraph says sprites repeat when R0>64 (1024/16). The reference now records this as a
  [KT] inconsistency; RTL (`asic_sprites.v:241`, `hp` at 16 per character,
  `s10_r0_gt_64_repeat`) uses the 16-per-character grid.
- **RC6 — §11 `&DFxx` value 0 clarified** (l.576-583): the former "value 0 or 7 → page 3"
  wording conflicted with the /EXP rule. The reference now separates value 7 from value 0;
  `plus_mmu.v:202` applies /EXP to every 0 write, not only at reset
  (`test_reset_defaults_and_exp_sampling`).
- **RC7 — §13 wording corrected**: A9/A8 are the port-address decode bits; Type 4 may differ
  from type 3 at Status 2 bit 3 (ACCC v1.11 FR §21.3.4 p.248); R16/R17 can be seeded by a
  snapshot even though this core has no light-pen strobe. The §13 source anchor is now v1.11.
- **RC8 — §8 PA3 qualifier added**: the VCC=0/RCC=0 exclusion and next-opportunity example
  are explicitly limited to the probed R5=0 case. R5>0 remains open as PA3.

---

## Clause table

`R` = RTL location, `T` = tests. Lines refer to `80c81a1`.

### §1 Lock / unlock

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §1.01 | Sequence written to CRTC select port, any `&BCxx` (l.20) | tested | `plus_mmu.v:166-167` | `test_io_read_traps_unlock_and_rmr2`, unlock suite |
| §1.02 | Sync: non-zero then zero resets the pointer (l.30) | tested | `asic_unlock.v:107` | `test_resynchronization`, `test_every_nonzero_sync_lead_unlocks` |
| §1.03 | 13 fixed bytes `FF 77 … 8A` (l.32) | tested | `asic_unlock.v` `sequence_byte` | `test_partial_prefixes_do_not_unlock`, `test_wrong_bytes_fail_closed` |
| §1.04 | Mismatch mid-sequence cannot unlock; a mismatching zero after non-zero re-syncs | tested | `asic_unlock.v:124-129` | `test_wrong_bytes_fail_closed`, `test_malformed_input_preserves_unlocked` |
| §1.05 | *RTL extension*: extra zeros before `FF` keep waiting (Switchblade, AmSpirit) | tested | `asic_unlock.v:116-121` | now recorded in §1 |
| §1.06 | STATE `&CD` unlocks; any other value locks (l.33, l.50-53) | tested | `asic_unlock.v:134` | `test_state_byte_and_trailing_ee` |
| §1.07 | Unlocked already after STATE; `EE` superfluous (l.34-38) | tested | `asic_unlock.v:131-137` | `test_state_byte_and_trailing_ee` |
| §1.08 | Lock semantics [ARNOLD] vs [WIKI] (l.48-53) | conflict | `asic_unlock.v:134` | RTL follows [WIKI-Unlock] as recommended |
| §1.09 | Locked after power-on and hard reset (l.45) | tested | `asic_unlock.v` reset | `test_reset_and_no_strobe_stability` |
| §1.10 | Lock gates only RMR2 (l.54) | tested | `plus_mmu.v:236-249`, `asic_ga_timing.v:287` | `test_rmr2_locking_positions_pages` |
| §1.11 | Locked `101xxxxx` acts as MRER, bit 5 ignored (l.55-56) | tested | `plus_mmu.v:242-243`, `asic_ga_timing.v:287` | `test_rmr2_locking_positions_pages` |
| §1.12 | Sprites, DMA, PRI keep running when locked (l.56-58) | untested | no lock input to `asic_regs`/`asic_dma`/`asic_sprites` | structural: those blocks never see `unlocked` |
| §1.13 | ASIC page mapped at lock **stays** mapped; cannot unmap until re-unlock (l.58-59) | tested | `plus_mmu.v:236-240` | PA6a: `test_rmr2_locking_positions_pages` relocks while mapped, rejects locked unmap, then unlocks and unmaps |

### §2 RMR2 and mapping

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §2.01 | `&7Fxx` data-byte register select table (l.68-74) | tested | `asic_ga_timing.v:285-307`, `plus_mmu.v:236-243` | `test_rmr2_locking_positions_pages`, `asic_ga_timing_diff_tests` |
| §2.02 | D4D3 00/01/10 place low ROM at &0000/&4000/&8000, page off (l.80-82) | tested | `plus_mmu.v:238-240` | `test_rmr2_locking_positions_pages` |
| §2.03 | D4D3 = 11: low ROM at &0000 **and** ASIC page on (l.83) | tested | `plus_mmu.v:238,240` | same (`&B8`) |
| §2.04 | D2-D0 cartridge page 0-7 as low ROM (l.84) | tested | `plus_mmu.v:239,277` | same (`&A4`) |
| §2.05 | ASIC page beats RAM, ROM, expansion at &4000-&7FFF regardless of order (l.91-93) | tested | `Amstrad.sv:826-827,1250-1252` | `p4_sprites_regs`, `asic_regs` benches read the page; expansion side is moot (§11.07) |
| §2.06 | Write-through still works for relocated low ROM (l.94-95) | untested | `Amstrad.sv:827` (`we` not gated by cart windows) | no bench writes under a relocated low ROM |
| §2.07 | **No** write-through from the ASIC page to RAM (l.95-97) | tested | `Amstrad.sv:827` `~plus_aspage_sel` | PA6b: `d5-asic-audit` CPU writes under sprite/palette/unmapped page addresses, then unmaps and reads original RAM sentinels; gates extracted from production top level |
| §2.08 | External RAM expansion also written when the page is on (hardware bug) (l.98-102) | scope | — | §14 item 3; the core has no external RAM expansion in Plus mode |
| §2.09 | MRER keeps CPC meaning; bit 4 clears the interrupt counter (l.103-104) | tested | `asic_ga_timing.v:290,307` | `pr04_mrer_clears_pri`, lockstep diff |
| §2.10 | MRER ROM-disable bits disable low ROM **wherever** mapped (l.104-105) | tested | `plus_mmu.v:191` (`~lromen` in `low_hit`) | `test_rom_enable_gating` |

### §3-§4 ASIC page map and read-back

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §3.01 | Sprite pixel RAM `&4000-&4FFF`, 256 B per sprite (l.116-118) | tested | `asic_regs.v:357`, `plus_sprite_ram.v` | `a01_sprite_ram`, `host_zero_and_latency` |
| §3.02 | `&5000-&5FFF`, `&6080-&63FF`, `&6440-&67FF`, `&6810-&6BFF`, `&6C10-&7FFF` unused (l.119-144) | tested | `asic_regs.v:376-379` decodes | `a06_open_bus` (value per PA4) |
| §3.03 | X/Y little-endian at `+0/+2` (l.120-121) | tested | `asic_regs.v:486-492` | `a02_sprite_regs` |
| §3.04 | Magnification at `+4`, POR 0 (l.122) | tested | `asic_regs.v:493`, reset `:426` | `a02_sprite_regs`, `a07_reset_contract` |
| §3.05 | Palette `&6400-&643F` (l.125-127) | tested | `asic_regs.v:377,497-503` | `a03_palette` |
| §3.06 | PRI, SPLT, SSCR POR 0; IVR bit 0 = 1 at reset (l.129-134) | tested | `asic_regs.v:407-412` | `a07_reset_contract` |
| §3.07 | SSA `&6802` = high, `&6803` = low (l.131) | tested | `asic_regs.v:509-510` | `a05_raster_dma_regs`, `t08a` |
| §3.08 | SSA POR undefined (l.131) | tested | `asic_regs.v:411-412` (0) | defined-zero refinement; harmless |
| §3.09 | `&6806-&6807` writes have no effect (l.134) | tested | `asic_regs.v:513` | `a05_raster_dma_regs` |
| §3.10 | ADC `&6808-&680F` read-only (l.135) | tested | `asic_regs.v:631-641`; writes fall to `default` | `a11_adc_paddles` |
| §3.11 | DMA SAR/PPR layout, `&6C03`/`&6C07`/`&6C0B` unused (l.137-141) | tested | `asic_regs.v:518-531` | `a05_raster_dma_regs` |
| §4.01 | Pixel writes masked to low nibble; reads `written & &0F` (l.148-149) | tested | `asic_regs.v:603` | `a01_sprite_ram` |
| §4.02 | X high reads `&FF` if `written & 3 == 3`, else `written & 3` (l.151-152) | tested | `asic_regs.v:590-592` | `a02_sprite_regs` |
| §4.03 | Y high reads `&FF` if `written & 1`, else 0 (l.152-154) | tested | `asic_regs.v:593-595` | `a02_sprite_regs` |
| §4.04 | Writes to `+4..+7` set magnification (l.155-160) | tested | `asic_regs.v:493` | `a02_sprite_regs` |
| §4.05 | Write to `+3` sets magnification ([ARNOLD-REV]) or Y-high ([KT]) (l.157-160) | conflict | `asic_regs.v:489-492` stores Y-high | reference asks for hardware verification; unprobed |
| §4.06 | Reads of `+4..+7` mirror `+0..+3`; magnification write-only (l.160-164) | tested | `asic_regs.v:610-615` | `a02_sprite_regs` |
| §4.07 | `&6800-&6807` write-only, read as unmapped (l.165) | tested | `asic_regs.v:643-644` | `a06_open_bus` |
| §4.08 | Unmapped/write-only page reads = last instruction byte (l.166-169) | tested | `Amstrad_motherboard.v` retained memory-read byte; `asic_regs.read_driven` | PA4 production-T80 `audit_page_readback`: photo29 50/68/7E/7E, fail-before FF; claimed FF and RAM/mapped controls pass |
| §4.09 | Same rule for the value an `IN` on a write-only Plus port returns (l.169-170) | tested | `Amstrad_motherboard.v` `plus_ga_open_read` | PA1 production-T80 `audit_ga_readback`: photo26 bytes78/40/50/58/60/68/78; palette and claimed-responder controls. Conflicting aliases conservatively unchanged |
| §4.10 | `IN` on `&7Fxx` performs a GA write with the bus value (l.170-172) | tested | `Amstrad_motherboard.v:288`, `asic_ga_timing.v` I/O decode | `test_io_read_traps_unlock_and_rmr2`, t80pa m9 |
| §4.11 | That value is `&79` on 6128+, `&78` on 464+ (l.172) | contradicted | `Amstrad_motherboard.v:290-297` (`&78` on both for `ED 78`) | PA1: 6128 Plus photo26 writes opcode palette, including ED78→066; hardware supports RTL write path over KT79 claim |
| §4.12 | DCSR readable at any `&6C00-&6C0F`, writable only at `&6C0F`; SAR/PPR unreadable (l.174-175) | tested | `asic_regs.v:386-391,624-627` | `a05_raster_dma_regs`, `a08_dcsr_raster_status` |

### §5 Hardware sprites

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §5.01 | 16 sprites, 16×16, 4 bpp (l.183) | tested | `asic_sprites.v:173-184` | `s02`, `s17_all_sprites_live_animation_cadence` |
| §5.02 | Pixel 0 transparent; 1-15 index sprite palette; no entry 0 (l.183-184) | tested | `asic_sprites.v:660,684-702` | `s07_priority_and_transparency`, `s08_palette_mapping_and_order` |
| §5.03 | 1 byte/pixel, row-major from top-left (l.185-186) | tested | `asic_sprites.v:653-655` | `s02_basic_placement_x1` |
| §5.04 | Unmagnified pixel = mode-2 pixel × 1 line, independent of screen mode (l.187-189) | tested | `asic_sprites.v` (no mode input, `PIXEN` dot grid) | `s02`, `b6_plus_layers` |
| §5.05 | X 10-bit: 0..767 direct, 768..1023 = −256..−1; **not** ordinary signed (l.189-191) | tested | `asic_sprites.v:174-176` (`c_xneg = &X[9:8]`) | `s09_x_signed_clip_and_offscreen` |
| §5.06 | Y 9-bit signed −256..+255 (l.191) | tested | `asic_sprites.v:185` | `s03_y_compare_formula_and_masking`, `p4_frame_entry` |
| §5.07 | (0,0) = top-left of the display area, not the border (l.192) | tested | `asic_sprites.v` `H_ORIGIN_DOTS` | `t06a_sprite_over_screen_ink`, `p4_live_prediction` |
| §5.08 | Visible span at ×4: X −63..+639, Y −63..+199; [ARNOLD] prints −64 (l.193-198) | conflict | `asic_sprites.v:242-243` | RTL uses derived −63; `s09`, `s06_quad_magnification_corners`; awaiting hardware |
| §5.09 | Positive X accepted through 767 for non-standard widths (l.199-201) | tested | `asic_sprites.v:175,241` | `s09` |
| §5.10 | Y compare line `(C4 << 3) \| (C9 & 7)` (l.202) | tested | `asic_sprites.v:155` | `s03` |
| §5.11 | [KT] X formula implies 8 X units/CRTC character, while its repeat rule implies 16 mode-2 pixels/character (l.202-204) | conflict | `asic_sprites.v:241` (16 dots/char) | RC5 documents the internal [KT] inconsistency; RTL follows the 16-pixel grid |
| §5.12 | **If** R0 > 64, sprites can repeat horizontally (l.204) | tested | `asic_sprites.v:279` (10-bit `hp` wrap) | `s10_r0_gt_64_repeat` |
| §5.13 | Y compare **not** gated by R6 (l.205) | tested | `asic_sprites.v:185-187` (no R6 input) | `s03` |
| §5.14 | Mag bits 3-2 = X, 1-0 = Y; `00` off, `01` ×1, `10` ×2, `11` ×4 (l.206-207) | tested | `asic_sprites.v:177-184` | `s01_disabled_codes_off`, `s04`, `s05` |
| §5.15 | **Either** X or Y mag 0 turns the sprite off (l.207-208) | tested | `asic_sprites.v:186` | `s01_disabled_codes_off` |
| §5.16 | Magnification cleared at reset (l.208) | tested | `asic_regs.v:426` | `a07_reset_contract` |
| §5.17 | Priority border > sprite 0 > … > sprite 15 > screen (l.211-213) | tested | `asic_sprites.v:672-679`, `asic_video.v:1253` | `s07`, `t06a`, `t06b_border_over_sprite` |
| §5.18 | Pixel-data access blanks **that sprite only**, ~1 byte per 1µs, image intact (l.214-217) | tested | `asic_regs.v:742-743`, `asic_sprites.v:301-305` | `s11_access_blanking_scope_and_integrity`, `a08_sprite_access_indicator` |
| §5.19 | X/Y/mag writes do **not** blank (l.217-218) | tested | `asic_regs.v:742` (pixel range only) | `a08_sprite_access_indicator` |
| §5.20 | Changing X/Y mid-display cuts and continues at the new position (l.218-219) | tested | `asic_sprites.v:244,261,336-342` | `s12_x_rewrite_cut_and_continue`, `s13`, `p4_live_prediction` |
| §5.21 | No per-line sprite limit (l.220-221) | tested | `asic_sprites.v:651-679` | `s14_overlap_bandwidth_within_capacity`, `s17` |
| §5.22 | SSCR offsets do not move sprites; D7 extension still hides them (l.222-223) | tested | `asic_sprites.v` (no SSCR input), `asic_video.v:1232,1253` | `t08g_sscr_border_mask_and_sprites` |

### §6 Palette

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §6.01 | 32 × 12-bit: pens 0-15, border, sprite 1-15 (l.231-232) | tested | `asic_regs.v:377,497-503` | `a03_palette` |
| §6.02 | Even byte R/B, odd byte G; low byte first (l.232-239) | tested | `asic_regs.v:500-503` | `a03_palette`, `t05i_asic_palette_drives_rgb` |
| §6.03 | High byte reads `& &0F`; low byte reads exactly (l.240-241) | tested | `asic_regs.v:620-621` | `a03_palette` |
| §6.04 | Dual-ported, no access restriction (l.242) | tested | `asic_regs.v` `pal_r` video port | `t05j_registered_palette_alternating_pens` |
| §6.05 | 16-bit write shows the intermediate colour (l.242-244) | tested | byte-wise write, `asic_regs.v:500-503` | emergent from the 8-bit bus; no dedicated test needed |
| §6.06 | Legacy PENR/INKR still reach entries 0-16 through the fixed table (l.245-249) | tested | `asic_regs.v:307-345,567-569` | `a04_legacy_translation`, `t05a_legacy_colour_rom_sweep`, `b8_palette` |
| §6.07 | Legacy writes readable in ASIC RAM (l.249) | tested | `asic_regs.v:568` | `b8_palette`, `a04` |
| §6.08 | Sprite colours **not** reachable via the legacy port (l.250) | tested | `asic_regs.v:567` (`addr <= 16`) | `a04_legacy_translation` |
| §6.09 | Border undefined at power-on (l.251) | tested | `asic_regs.v:31-40,433-436,546-554` (zero, then GA-shadow import) | deterministic refinement of "undefined"; named model assumption |
| §6.10 | Mono luma G:R:B = 9:3:1 (l.252) | tested | `amstrad_video_color.sv:88-89` (177:59:20) | `video-color-test` |

### §7 PRI, interrupts and IVR

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §7.01 | PRI at `&6800`, write-only, POR 0 (l.258) | tested | `asic_regs.v:407,507` | `a05_raster_dma_regs`, `a06_open_bus`; POR not asserted by `a07` |
| §7.02 | PRI = 0: the 52-line GA interrupt operates (l.262) | tested | `asic_ga_timing.v:517-559,694` | `pr01_baseline`, `asic_ga_timing_diff_tests` |
| §7.03 | PRI ≠ 0: interrupt raised **instead** at that line; 52-line delivery suppressed (l.262-264) | tested | `asic_ga_timing.v:649-653,694` | `pr02_pri_line`, `pr08_pending_classic_mode_switch` |
| §7.04 | …while the 52-line counter **keeps running** (l.264-265) | tested | `asic_ga_timing.v:535-559` (no `pri` term) | `pr08_pending_classic_mode_switch` |
| §7.05 | Compare `{0,PRI} == {VC5..VC0, RC2..RC0}`; VC5 must be 0, so lines ≥ 256 never match (l.266-273) | tested | `asic_ga_timing.v:649-651` | `pr02_pri_line`, `pr09_live_pri_write`; no-wrap hardware-confirmed (alias probe) |
| §7.06 | n+256 alias ([QUASAR], CPCWiki p.4) (l.272-274) | conflict | nine-bit compare | hardware sides with no-wrap |
| §7.07 | PRI = 0 selects the 52-line mechanism, not line 0/256 (l.284-285) | tested | `asic_ga_timing.v:649` (`pri != 0`) | `pr01_baseline` |
| §7.08 | Trigger point: [ARNOLD-REV] 6µs clamp vs [KT] +10µs (l.286-288) | conflict | superseded by §7.09 | resolved by original-Plus photographs (§15) |
| §7.09 | Delayed comparator: rising edge of `HSYNC_d && PRI≠0 && (line_d match && !adj_d \|\| line match && !adj)` (l.289-295) | tested | `asic_ga_timing.v:635-653` | `pr10_ordinary_phase`, `pr11_pri_write_window`, `pr12_line_entry` |
| §7.10 | Ordinary request 1µs after raw HSYNC start at **every** width, width 1 included (l.297-300) | tested | `asic_ga_timing.v:640-653` | `pr10_ordinary_phase` |
| §7.11 | Writing PRI to the current line requests while `HSYNC_d` is high (l.301-304) | tested | `asic_ga_timing.v:649-653` (live `pri`) | `pr09_live_pri_write`, `pr11_pri_write_window` |
| §7.12 | Same-value writes and a held level after ACK **never** re-request (l.305) | tested | `asic_ga_timing.v:652-653` (edge detect) | `pr09_live_pri_write` |
| §7.13 | Other PRI writes do **not** clear a pending request (l.305-306) | tested | `asic_ga_timing.v:694,750-758` | `pr09_live_pri_write` |
| §7.14 | `HSYNC_d` high at the PRI line's C0 0 adds a line-entry request, incl. R2+width = 64 (l.307-313) | tested | `asic_ga_timing.v:649-651` (`line` term) | `pr12_line_entry`, `p1_video` `pri_cross_line` |
| §7.15 | With R2 = R0, `line_d` still holds the PRI line at the next line's C0 0: two requests (l.313-316) | tested | `asic_ga_timing.v:649-651` (`line_d` term) | `pr12_line_entry` |
| §7.16 | PRI = 0 52-line interrupt on the trailing edge of the **CRTC** HSYNC (l.317-319) | tested | `asic_ga_timing.v:517-527` | `asic_ga_timing_diff_tests` (lockstep with GA40010) |
| §7.17 | PRI does **not** trigger during vertical adjust (l.320) | tested | `asic_ga_timing.v:650-651` (`!adj`, `!adj_d`) | `pr03_adjustment_gate` |
| §7.18 | …but a request due from the last normal line can still land in the first adjustment line (RTL comment, `asic_ga_timing.v:617-619`) | untested | `asic_ga_timing.v:650` (`line_d` term) | RTL refinement not stated in the reference |
| §7.19 | PRI "misbehaves for some values when R9 < 7" (l.320-321) | conflict | compare on raw `RC2..RC0` | unspecified behaviour; nothing to implement until probed |
| §7.20 | A PRI fire clears bit 5 of the GA counter, so a re-enabled CPC interrupt waits ≥ 32 lines (l.322-324) | tested | `asic_ga_timing.v:532-536,543` | PA6e: `pr13_fire_clears_counter_bit5` observes 40→8 before ACK, then 44 HSYNC falls; KT’s universal ≥32 inference remains ambiguous |
| §7.21 | Raster interrupt cleared by INT acknowledge **or** MRER bit 4 (l.325-326) | tested | `asic_ga_timing.v:290,675-676` | `pr01_baseline`, `pr04_mrer_clears_pri`, `test_b20_two_distinct_acks` |
| §7.22 | GA-compatible interrupt ~1µs later than a CPC (l.327-328) | not-impl | `asic_ga_timing.v` lockstep with GA40010 | PA7: V4 hardware gap approximately128 dots versus RTL112. V5 screens31–35 distinguish pending acceptance, width and instruction phase; V5 hardware G0/W192/L128/R128/I128; AmSpirit R144 disagrees in the first PRI reference marker. Isolated T80 sampling is one CPU clock late; independently confirmed Zilog reading; shared-CPU scope authorized, with classic-CPC impact evidence required before PA7 repair. [Follow-up](pa7-interrupt-phase-followup.md) |
| §7.23 | IVR at `&6805`; bit 0 = 1 at reset, bits 7-1 undefined (l.330) | tested | `asic_regs.v:410,512` | `a07_reset_contract` (bits 7-1 defined zero) |
| §7.24 | The ASIC always supplies a vector byte on acknowledge (l.334-335) | tested | `asic_regs.v:693-694`, `Amstrad.sv:1250` | `p1_mobo_bench`, `test_b20_two_distinct_acks` |
| §7.25 | Locked/plain Plus bus byte is `&00` (some `&56`) (l.335-336) | conflict | `asic_regs.v:692-693` (`(IVR & F8) \| src`) | with reset IVR a pending raster gives `&06`, only an empty ack `&00`; IM 1 ignores it |
| §7.26 | Vector = `(IVR & &F8) \| source`, D0 = 0 (l.337) | tested | `asic_regs.v:692-693` | `p1_mobo_bench`, `test_b20_two_distinct_acks` |
| §7.27 | Source codes: DMA2 000, DMA1 010, DMA0 100, raster 110 (l.339-344) | tested | `asic_regs.v:678-685` | `test_pa6e_simultaneous_priority` (110, 000, 010, 100) |
| §7.28 | Priority raster > DMA2 > DMA1 > DMA0 (l.346) | tested | `asic_regs.v:678-685` (if/else chain) | PA6e: `test_pa6e_simultaneous_priority`, live PRI plus all three DMA INT flags; vectors 06/00/02/04 and retained flags 70/60/40/00 |
| §7.29 | IVR bit 0 = 0: DMA flags auto-cleared by acknowledge (l.347-348) | tested | `asic_regs.v:396-399,460-462` | `test_b20_double_pulse_dma` |
| §7.30 | IVR bit 0 = 1: DMA flags held until DCSR write-1 (l.348-350) | tested | `asic_regs.v:460-464` | `a05_raster_dma_regs`, `test_b20_double_pulse_dma` |
| §7.31 | Bits 4-7 freeze in automatic mode ([QUASAR], unresolved) (l.351-352) | conflict | not implemented | the reference says not a rule |
| §7.32 | Acknowledge while executing at A13 = 0 gives the DMA0 vector (4) instead of raster (6) (l.354-358) | tested | `Amstrad_motherboard.v:258-269`, `asic_regs.v:686-687` | `b20_bus_diag` `run_cell` (write-low) |
| §7.33 | With auto-clear, DMA vectors are affected too (l.358-359, 362-364) | tested | `asic_regs.v:396,686-687` | `test_b20_double_pulse_dma` |
| §7.34 | Interrupted instruction address matters, not handler address; low-A13 HALT does not split (l.364-366) | tested | `Amstrad_motherboard.v:258-269` (READY-shaped) | `b20_bus_diag` `run_cell` (halt-low control) |
| §7.35 | `ASIC_IORQ_n = CPU_IORQ_n \| (~READY & ~A13) \| reset`; raw IORQ kept for expansion devices (l.368-371) | tested | `Amstrad_motherboard.v:258-269` | `b20_bus_diag` `run_cell` (raw vs shaped rise counts) |
| §7.36 | Empty repeated pulse in the same M1 returns offset 4; isolated empty acks unestablished (l.371-373) | tested | `asic_regs.v:686-687` (`ack_seen && intack_m1`) | `test_b20_double_pulse_dma`; isolated case keeps `&00` |
| §7.37 | The ASIC does not decode RETI (l.374) | tested | no opcode snooping in `asic_regs`/`asic_ga_timing` | structural |
| §7.38 | No IEI/IEO daisy chain; expansion interrupts need IM 1 (l.374-375) | scope | — | no expansion interrupt source in Plus mode |

### §8 Split and soft scroll

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §8.01 | SPLT = line **after** which the split occurs; 0 = off (l.385) | tested | `asic_video.v:554` | `t08a_split_screen_capture_and_advance`, `t08b_split_screen_disabled_when_zero` |
| §8.02 | Compare `{SPLT} == {VC4..VC0, RC2..RC0}`, 8-bit, so n and n+256 match; with R9>7 a value can match at raw rasters r and r+8 (l.385-387) | untested (alias) | `asic_video.v:554` | n+256 alias lacks a kept vector; R9>7 double match is unprobed; RC4 |
| §8.03 | Pathological terminal-line value is 55 on a 312-line frame; [KT] reports 56 (l.387-389) | tested | `asic_video.v:554-605` | original Plus probes confirm 55; `t08k_split_on_terminal_line`; RC4 |
| §8.04 | SSA in R12/R13 format incl. bank bits; MA of the first line after the split (l.390-392) | tested | `asic_video.v:585,604` | `t08a`, `t08h_overscan_carry_14bit` |
| §8.05 | SSA captured at HCC == R1 (l.395) | tested | `asic_video.v:557` | `t08a`, `t08l_split_ssa_sampling_point` |
| §8.06 | HCC == R0 on the R5=0 normal terminal or non-interlaced adjustment-ending line; ordinary capture before adjustment | tested | `asic_video.v` `split_at_r0` | `t08l` preserves V3 hardware timing; PA3 `t08n` pins late/early normal-terminal enable; `t08o` pins late adjustment-terminal carry. Original-Plus controls pending; exact edge is an emulator-supported model |
| §8.07 | Used from the next scan line; replaces the line-start MA until the next split or frame restart (l.399-401) | tested | `asic_video.v:584-605` | `t08a`, `t08c_split_screen_multiple_splits` |
| §8.08 | Terminal split does not retarget frame0; R5=0 carries to frame1, earlier adjustment capture does not | tested | `asic_video.v` frame-origin reload and `split_held` | `t08k`, PA3 `t08n`/`t08o`; normal-terminal early arm gives green adjustment/red frame0–7, last-adjustment capture carries to frame1. Additional hardware controls pending |
| §8.09 | Multiple splits per frame (l.408) | tested | `asic_video.v:584-605` | `t08c` |
| §8.10 | Split captures occur at tested adjustment indexes0/1/8, contradicting Arnold’s first-only restriction | tested | `asic_video.v` comparator active in adjustment | PA2 `t08m` fails before fix; photo27 and AmSpirit green297/305–311. Other indexes and exact intra-line boundary unmeasured |
| §8.11 | DRAM refresh hazard with non-16k split addresses / R4=R9=0 rupture (l.411-415) | scope | — | §14 item 2 |
| §8.12 | SSCR D3-D0: horizontal delay 0-15 mode-2 pixels (l.423) | tested | `asic_video.v:1196-1216` | `t08f_sscr_horizontal_pixel_delay` |
| §8.13 | SSCR D6-D4 added to RA low 3 bits (l.424, 432) | tested | `asic_video.v:529` | `t08e_sscr_vertical_scanline_offset` |
| §8.14 | SSCR D7 extends border over the first 2 bytes of each line (l.425) | tested | `asic_video.v:1218-1232` | `t08g` |
| §8.15 | Affects both halves of a split; sprites not scrolled, D7 hides them (l.427-430) | tested | `asic_video.v:529,1232,1253` | `t08g` |
| §8.16 | Effect immediate, several times per line (l.431) | tested | `asic_video.v:529,1216,1232` (live `SSCR`) | `t08f`, `t08g` |
| §8.17 | CRTC counters unaffected; odd behaviour **when** R9 < 7 (l.432-433) | tested | `asic_video.v:543` level test | PA5a: `t08e_sscr_vertical_scanline_offset` pins offset-2 MA staircase from V3 screen 25 |
| §8.18 | Clean full-screen scroll is confirmed for R9=7; do not generalize to R9>7 (l.433-435) | tested | `asic_video.v:543` | `t08i` covers R9=7; R9>7 behavior is in §8.19; RC3 |
| §8.19 | Row capture is the level test RA ≥ R9 at HCC == R1; R9=11 offset 5 advances three rows (l.436-442) | tested | `asic_video.v:543-546` | `t08i_sscr_vertical_wrap_advances_ma`, `t08j_sscr_vertical_offset_r9_above_7` |
| §8.20 | Clean full-screen row capture occurs when effective RA reaches R9 at HCC == R1 (l.433-435) | tested | `asic_video.v:543` | `t08i`; RC3 |
| §8.21 | At frame line 0, row capture can replace a held terminal-line split when effective RA reaches R9 (l.443-444) | tested | `asic_video.v:584-587` (row capture overwrites held split) | PA5b: `t08k_split_on_terminal_line`, R9=7 and SSCR=70; frame line 1 uses R12/R13+R1 instead of SSA (V3 screen 23) |

### §9 DMA sound

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §9.01 | Fetch from central RAM only, base 64K, MMR/ROM ignored (l.452-455) | untested | `asic_dma.v:337`, `Amstrad_motherboard.v:916-917` (video-slot address) | flat RAM model; no bench banks MMR mid-list |
| §9.02 | Address bit 0 ignored; word-aligned little-endian (l.454-455) | tested | `asic_dma.v:337` | `d02_load_instruction` |
| §9.03 | Max one instruction per channel per HSYNC; rupture doubles the rate (l.456-458) | tested | `asic_dma.v:160-165,270-300` (`hsync_rising`) | `d12_active_channel_fetch_timing`; rupture emergent |
| §9.04 | `&0RDD` LOAD writes PSG register R (l.464) | tested | `asic_dma.v:412-423` | `d02`, `d08`, `d11` |
| §9.05 | `&1NNN` PAUSE N; PAUSE 0 = NOP (l.465) | tested | `asic_dma.v:432-435` | `d03_pause_and_prescaler`, `d15_repeated_pause_int_cadence` |
| §9.06 | `&2NNN` REPEAT: nonzero N executes the body N+1 times (l.466) | tested | `asic_dma.v:438-441,447-451` | `d04_repeat_and_loop` |
| §9.07 | REPEAT 0: NOP (CPCWiki, RTL) vs REPEAT 1 (French tutorial, Quasar) (l.490-492) | conflict | `asic_dma.v:438` | PA6c: `d04_repeat_and_loop` pins NOP while a prior nonzero loop context is live; source conflict retained |
| §9.08 | `&3xxx` acts as PAUSE then REPEAT (l.467) | tested | `asic_dma.v:432-441` | `d09_undocumented_pause_repeat` |
| §9.09 | `&4000` NOP, `&4010` INT, `&4020` STOP (SAR left at next) (l.468-471) | tested | `asic_dma.v:442-445` | `d05`, `d06`, `d07_compound_int_stop` |
| §9.10 | LOOP: if counter ≠ 0 decrement and jump; **if** 0 no effect (l.469) | tested | `asic_dma.v:447-451` | `d04` |
| §9.11 | Op field is bits 14:12, bit 15 ignored; LOAD **only** when all three are 0 (l.473-475) | tested | `asic_dma.v:410-454` | `d16_combined_context` |
| §9.12 | Control uses bits 5/4/0 only; STOP suppresses LOOP (l.477-478) | tested | `asic_dma.v:447` (`!instr[5]`) | `d16` |
| §9.13 | INT suppressing LOOP is **not** a rule (l.478-479) | tested | `asic_dma.v:447` (bit 4 not in guard) | `d16` |
| §9.14 | REPEAT\|STOP installs count and address, then stops (l.480-481) | tested | `asic_dma.v:438-445` | `d16` |
| §9.15 | REPEAT\|LOOP unresolved, left unimplemented (l.481-486) | conflict | `asic_dma.v:425-430` (explicit NOP) | deliberate; no hardware claim |
| §9.16 | One loop context per channel; a second REPEAT overwrites it (l.487-488) | untested | `asic_dma.v` single `loop_cnt`/`loop_addr` | structural; fails the keep rule |
| §9.17 | ASIC never writes RAM; REPEAT not re-fetched per iteration (l.488-489) | tested | `asic_dma.v:439-440` (`loop_addr = SAR` after REPEAT) | `d04` |
| §9.18 | PAUSE total `N × (PPR+1)` lines (l.493-494) | tested | `asic_dma.v:274-286` | `d03` |
| §9.19 | PPR change takes effect immediately, even mid-pause (l.495-498) | tested | `asic_dma.v:167-175,274-286`, `asic_regs.v:470-481` | CPU PPR write ends the current interval; `plus_p8` b20_02/b20_03; RC1 |
| §9.20 | SAR rewrite does **not** interrupt a pause (l.498-499) | tested | `asic_dma.v:259-261` (pause state untouched) | `d10_byte_sar_writes` |
| §9.21 | Disabling a channel **suspends** its pause; resumes on re-enable (l.499-500) | tested | `asic_dma.v:274-278` (`dcsr_ena` gates the count) | `d15`, `plus_p8` b20 |
| §9.22 | SAR rewritable while running (list jump) (l.501) | tested | `asic_dma.v:259-261` | `d10` |
| §9.23 | Re-writing a set enable bit is a no-op (l.501-502) | untested | level-only `dcsr_ena` | structural; fails the keep rule |
| §9.24 | Reset stops all channels (l.502) | tested | `asic_regs.v:415`, `asic_dma.v:180-204` | `d01_reset_and_defaults` |
| §9.25 | DCSR bit layout, W1C interrupt bits, bit 3 unused (l.506-515) | tested | `asic_regs.v:442-466,624-627` | `a05`, `a08_dcsr_raster_status` |
| §9.26 | Enables cleared by STOP or reset; readable while running (l.517-518) | tested | `asic_regs.v:464-466` | `d06_stop_instruction` |
| §9.27 | Flags cleared by CPU write 1, **or** auto-cleared by INT ack **when** IVR bit 0 = 0 (l.518-520) | tested | `asic_regs.v:396-399,460-464` | §7.29, §7.30 |
| §9.28 | Bit 7 = last acknowledge was raster, not live pending (l.508, 521-523) | tested | `asic_ga_timing.v:655-720`, `asic_regs.v:626` | `pr05_dcsr_level`, `pr06_dma_ack_then_raster`, `a08_dcsr_raster_status` |
| §9.29 | Per HSYNC: 1 dead cycle, fetch per active channel, execute in channel order (l.529-531) | tested | `asic_dma.v:160-165,327` | `d12`, `d13_active_channel_execute_timing` |
| §9.30 | LOAD ≥ 8 cycles, +1 on a CPU 8255 access, +2 on a CPU PSG write (l.531-533) | tested | `asic_dma.v:412-423` (`load_extra` `:416-417`) | `d11_load_timing_and_ay_restore`, `d14_all_channel_collision_extensions` |
| §9.31 | CPU 8255 access held off up to 8µs; PPI/AY state restored after LOAD (l.534-537) | tested | `asic_dma.v:516-521`, `Amstrad_motherboard.v:1154,1202` | `p10_dma_ppi`, `p10_dma_mobo` |
| §9.32 | DMA RAM fetches never stall the Z80 (assumed) (l.537-541) | untested | video-slot fetch; no READY path | ⚠ MISSING in the reference; assumption holds by structure |
| §9.33 | Narrowing HSYNC with DMA running can damage the ASIC (l.542-544) | scope | — | §14 item 7 |

### §10 Analogue inputs

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §10.01 | 8 × 6-bit, read-only, writes ignored (l.550-552) | tested | `asic_regs.v:631-641` | `a11_adc_paddles` |
| §10.02 | Default `3F 3F 3F 3F 3F 00 3F 00` (l.553-555) | tested | `asic_regs.v:633-640` | `a11_adc_paddles` |
| §10.03 | Updated ~200×/s from 4 wired channels (l.550-552) | not-impl | constants only | no paddle source wired; title-driven |
| §10.04 | Analogue fire buttons map onto joystick 0 fire; no ghosting (l.556-558) | not-impl | — | as §10.03 |

### §11 Cartridge and boot

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §11.01 | 32 × 16 KB pages, 5 page lines (l.568-570) | tested | `plus_mmu.v:198`, `plus_cpr_parser.v:69-90` | `plus_cpr_parser`, `plus_mmu`, `plus_cartridge_memory` |
| §11.02 | `&DFxx` 128-255: low 5 bits = physical page (l.572-573) | tested | `plus_mmu.v:198` | `test_rom_select_rules` |
| §11.03 | 0-127 (≠ disc code): page 1 (l.574) | tested | `plus_mmu.v:202` | `test_rom_select_rules` |
| §11.04 | 7: page 3 on 464+/6128+ (l.575-576) | tested | `plus_mmu.v:200` | `test_rom_select_rules` |
| §11.05 | 0: /EXP decides page 1 or 3 on 464+/6128+; GX4000 remains page 1 (l.575-583) | conflict | `plus_mmu.v:201` (/EXP for every 0) | the reference now resolves its duplicated 0 rule; source distinction remains; `test_reset_defaults_and_exp_sampling`; RC6 |
| §11.06 | **On GX4000**, 7 (and 0) still page 1 (l.576-577) | tested | `plus_mmu.v:199` | `test_gx4000_overrides` |
| §11.07 | Expansion ROMs via ROMDIS still override matching pages (l.578) | not-impl | `Amstrad.sv:1250-1252` (cart wins) | deliberate: no expansion ROM reaches Plus mode (`architecture.md` B13) |
| §11.08 | Low bank: pages 0-7 only (l.579-580) | tested | `plus_mmu.v:277` | `test_rmr2_locking_positions_pages` |
| §11.09 | Reset: RMR2 = 0, page 0 at &0000, ASIC page off (l.581) | tested | `plus_mmu.v:208-214` | `test_reset_defaults_and_exp_sampling` |
| §11.10 | 464+/6128+ reset: /EXP low ⇒ page 1, else 3 (l.582-583) | tested | `plus_mmu.v:201`; `Amstrad.sv:1289` ties `/EXP` low | same; production boots BASIC (`d5-basic-boot-input-2026-09-11.md`) |
| §11.11 | Boot starts at &0000 in cartridge page 0 (l.583-585) | tested | `plus_mmu.v:277` | `p0_boot`, `p10_boot` |
| §11.12 | CPR: RIFF, `Ams!` form, `cbNN` decimal chunk ids (l.593-597) | tested | `plus_cpr_parser.v:221-307` | `plus_cpr_parser` suite |
| §11.13 | Short chunk zero-padded to 16 KB (l.597-598) | tested | `plus_cartridge_memory.v:268-276` (clear sweep) | `test_short_and_oversized_blocks` |
| §11.14 | Data beyond 16 KB ignored (l.598-599) | contradicted | `plus_cpr_parser.v:384-391` (abort) | deliberate fail-closed policy (`architecture.md` CPR); tested; not a finding |
| §11.15 | ACID not emulated; CPR never fails for lack of it (l.615-619) | tested | absent | every CPR bench |

### §12 Model differences

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §12.01 | RAM 64/128/64 KB (table) | tested | `plus_model_select.v:29-47` | `b6_menu_mask` |
| §12.02 | FDC on 6128+ only (table) | tested | `plus_fdc_decode.v:23-36` | `plus_p8` p10_03 |
| §12.03 | Tape on 464+ only (table) | tested | `Amstrad_motherboard.v:1163,1196-1197` | PA6d: `d5-asic-audit`, production model decoder and motherboard/PPI input, output and motor across all three Plus models |
| §12.04 | GX4000 `&DFxx` = 7 → page 1; boot page 1 at &C000 (table) | tested | `plus_mmu.v:199` | `test_gx4000_overrides` |
| §12.05 | GX4000 keyboard absent, Pause = P, pads on lines 9/6 (table) | scope | `hid.sv`/top level, not audited here | integration assumption, unverified |
| §12.06 | GX4000 printer and expansion absent (table) | scope | not modelled | — |
| §12.07 | GX4000 master clock 39.90257 MHz, 0.25% slow (table) | scope | not modelled | waived in `architecture.md` |
| §12.08 | PPI: port B always input, port C always output (l.641-642) | tested | `i8255.v:57-59,82-83,104-105` | `plus_p8` p8_01 |
| §12.09 | Control-register rewrite does **not** clear output latches (l.642-643) | tested | `i8255.v:111-113` | `plus_p8` p8_01 |
| §12.10 | Port A input mode presents `&FF` to the PSG bus (l.643-644) | tested | `i8255.v:56` | `plus_p8` p8_01 |
| §12.11 | Printer BUSY sampled ~500×/s (l.644-645) | not-impl | — | no title need known |
| §12.12 | Colour changes ~½µs later than a CPC (l.645-646) | not-impl | — | PA7 source-timing context; V4 same-Plus IRQ screen does not measure a cross-machine colour delay |
| §12.13 | Joystick ports lack the third fire line (l.647) | scope | top-level input mapping | unverified here |

### §13 CRTC type 3

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §13.01 | Port decode by A9/A8: select (W), write (W), both read ports return the selected register; no status port (l.657-660) | tested | `asic_video.v:1005-1015` | `t07g_readonly_and_neutral_cycles`; RC7 |
| §13.02 | `IN` on `&BCxx`/`&BDxx` also performs the write with the open-bus byte (l.661-662) | tested | `Amstrad_motherboard.v:288-297` | t80pa m9 steps 42-52, `test_io_read_traps_unlock_and_rmr2`; value per PA1 |
| §13.03 | Reads repeat modulo 8 over selects 0-31: R16, R17, S1, S2, R12, R13, R14, R15 (l.663-674) | tested | `asic_video.v:1009-1020` | `t07a_mod8_read_map_and_storage` |
| §13.04 | Status 1 bit-by-bit, including bit 2's R0≥R1 gate and bit 4 at C0=R2+R3[3:0] (l.669) | tested | `asic_video.v:950-966` | `t07b`, `t07c`, `t07d`; R3=0 compares at C0=R2; ACCC v1.11 FR §21.3.4 p.248; RC2 |
| §13.05 | Status 2 bit-by-bit (l.670) | tested | `asic_video.v:968-977` | `t07e`, `t07f` |
| §13.06 | R12 all 8 bits readable, VMA uses bits 5:0 (l.671) | tested | `asic_video.v` R12 store | `t07a` |
| §13.07 | R14 bits 7:6 forced zero (l.673) | tested | `asic_video.v:306` (6-bit store) | `t07a` |
| §13.08 | R14/R15 readable (ACCC v1.11 supersedes [KT]) (l.676-678) | tested | `asic_video.v:1009-1020` | `t07a`; RC7 |
| §13.09 | R16/R17 readable; zero without a light-pen value, but snapshot load can seed them (l.678-679) | tested | `asic_video.v:1011-1012` | `t07a`; RC7 |
| §13.10 | R3 HSYNC width 0 ⇒ 16 (l.681) | tested | `asic_video.v` `hsc` wrap | `t04b_r3_zero_means_sixteen` |
| §13.11 | R3 VSYNC width 0 ⇒ 16 (l.681) | tested | `asic_video.v` `vsc` wrap | `t04f_vsync_width` |
| §13.12 | No R31 dummy register (l.682) | untested | `asic_video.v:308` (write `default`); read select 31 → R15 | structural; fails the keep rule |
| §13.13 | Type 4 read map is similar to type 3, but Status 2 bit 3 may differ (l.682-683) | conflict | — | older equivalence claim is overbroad; ACCC v1.11 FR §21.3.4 p.248; RC7 |

### §14 Hardware-flaw checklist

| ID | Clause (ref line) | Status | RTL | Tests / note |
|---|---|---|---|---|
| §14.1 | A13=0 IM2 vector corruption: emulate or ignore (l.690-691) | tested | emulated | §7.32-§7.36 |
| §14.2 | RAM refresh loss with rupture + raster interrupts: arguably skip (l.692-694) | scope | — | not modelled |
| §14.3 | External RAM expansion write-through (l.695) | scope | — | §2.08 |
| §14.4 | PPI quirks required (l.696-697) | tested | `i8255.v` | §12.08-§12.10 |
| §14.5 | Open-bus reads for write-only I/O and unmapped page (l.698-699) | tested | Motherboard retained-byte fallback | PA1/PA4 production-T80 photo vectors; mapped registers and overlapping responders retain priority |
| §14.6 | `IN` on GA/CRTC ports performs writes (l.700-701) | tested | | §4.10, §13.02; value PA1 |
| §14.7 | ASIC-damage contention: do not emulate (l.702) | scope | — | — |

---

## Summary

222 clauses: 181 `tested`, 9 `untested`, 13 `conflict`, 11 `scope`, 2 `contradicted`,
6 `not-impl`.

| Status | Rows | Disposition |
|---|---|---|
| `contradicted` | §4.11 | Hardware supports opcode78 on this6128Plus, contradicting KT79; preserve decoder writes |
| `tested` | §4.08, §4.09, §8.10, §14.5 | PA1/PA2/PA4 fixed with hardware-derived fail-before vectors |
| `contradicted` | §11.14 | deliberate, tested fail-closed CPR policy; no action |
| `not-impl` | §7.22, §12.12 | PA7 (single-source timing offsets) |
| `not-impl` | §10.03-§10.04, §12.11 | no paddle/printer source; title-driven only |
| `not-impl` | §11.07 | deliberate Plus-mode expansion isolation (B13) |
| `tested` | §1.13, §2.07, §7.20, §7.28, §8.17, §8.21, §12.03 | PA5/PA6 regression vectors pass; PA6c also pins the chosen REPEAT 0 reading in conflict row §9.07 |
| `tested` | §8.06, §8.08 (R5>0) | PA3 late/early normal-terminal and adjustment-terminal vectors; original-hardware early/last-adjustment controls pending |
| `untested` | others | structural or single-line; recorded, no vector proposed |

Terminal sampling now distinguishes the last normal line from the frame-ending
adjustment line (PA3). PA2 follows measured adjustment captures over Arnold's
first-only claim. PA1's model-specific KT79 claim remains contradicted by this
6128Plus; other models and conflicting I/O aliases remain unmeasured.

## Execution status (2026-09-28)

RC1-RC8 and the §1.05 reference refinement are committed. PA5a/b and PA6a-e
pass their focused checks and the phase-2 selection gate (41 benches). Independent
Opus medium cross-provider review accepted the tests, including the same-geometry
SSCR control and tape-input independence check. Production RTL remains unchanged.

The first PA6e priority attempt restored header raster pending together with
DCSR `70`. The snapshot policy attributes that pair to DMA, so this did not
create simultaneous sources and was a test-setup defect, not an RTL finding.
The repaired setup uses live PRI and DMA requests and checks both are pending
before acknowledge; expected vectors `06,00,02,04` and remaining DMA flags
`70,60,40,00` are unchanged. The corrected focused priority run passes.

PA6e's counter vector pins KT's bit-5-clear operation: count 40 becomes 8 before
acknowledge, then CPC delivery resumes after 44 HSYNC falls (52−8). KT's broader
“not closer than 32 lines” inference is not general: clearing bit 5 leaves 31
unchanged. This source ambiguity is separate from the implemented operation.

Phase-2 and final phase-3 validation (Homebrew LLVM replaces the host’s incomplete default C++ setup):

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
make -C sim CXX=/opt/homebrew/opt/llvm/bin/clang++ soak SOAK_EXPECT=0xe99ab434a5e1cdb3
```

```text
select_tests: PASS 41 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
soak hash: 0xe99ab434a5e1cdb3
soak hash matches expected
```

The slow `b20-ack-diag` and `d5-asic-audit` targets passed separately with
`CXX=/opt/homebrew/opt/llvm/bin/clang++`. D5 covers all three Plus models and
both tape-input levels. Its initial build-only error called a private helper;
using the fixture’s public SDRAM map fixed access without changing expectations.

Phase 3 is complete; original-6128-Plus photographs have now been received. Independent Opus medium review
accepted the designs before assembly and the implementation after its final
PA1 sentinel/mapping and PA7 ruler improvements. V4 screens 26–30 cover
PA1/PA2/PA3/PA4/PA7; [pre-build predictions and simulation evidence](asic-audit-probes-v4.md)
remain distinct. All five production-T80 runs pass, with controls, exact write
coordinates and complete-frame traces. All 25 earlier screens retain their
normalized event traces; image differences are confined to version/count glyphs.
Navigation through the new screens and back to the title passes.

PA2 directly displays adjustment, with PA3's terminal-split strip providing the
same-geometry visibility control. A negative late-capture result cannot separate
first-eight-only eligibility from absence of the RC3 alias. PA7 measures the
relative IRQ acceptance slot on one Plus; a common Plus-versus-CPC HSYNC shift
and the separate colour-delay claim remain unresolved. These simulations alone
do not establish hardware behavior; the subsequent photographs are recorded below.

The final selection command above passed 41 benches after the last code edit;
the final soak reproduced `0xe99ab434a5e1cdb3` over 2,845,088 samples. The golden
history is unchanged. Hardware instructions and both title-first/start-at-26
cartridges are linked from the [V4 record](asic-audit-probes-v4.md).

### Hardware evidence received

[Five photographs](asic-audit-probes-v4.md#original-6128-plus-photographs-2026-09-28)
from a 6128 Plus through Retrotink4K → TV → phone show:

- PA1 CPU reads track the opcodes rather than RTL FF. Palette writes match RTL;
  this 6128 Plus returns/writes78 for ED78, contradicting KT's79 claim.
- PA2 shows captures at tested adjustment indexes0,1,8, including the index8
  alias. It does not establish every index or the exact R1/R0 capture point.
- PA4 returns50/68/7E/7E, with all RAM/mapped controls passing.
- PA7's approximately128-dot relative marker gap differs from RTL112; count02
  passes. This establishes a software-slot discrepancy, not an exact raw delay.
- PA3 is all red and lacks a discernible cyan phase dash. AmSpirit reproduces
  that result; earlier enable with unchanged disable timing makes adjustment
  green and frame0–7 red. The working diagnosis is a missed capture opportunity,
  not restart persistence. The early-arm original-hardware control remains unrun.

### Phase-4 implementation (2026-09-28)

PA4 and PA1 now return the hardware-observed bytes; tests execute production
T80 from cartridge and SDRAM, preserving palette side effects, mapped ASIC
reads (including legitimateFF), and PPI/CRTC/FDC priority. The old audit claim
that `plus_asic_rd` did not select unused page reads was wrong: it already
claimed the entire page, while `asic_regs` supplied neutralFF. The repair
uses explicit register read ownership and a retained memory-read byte.

PA2 permits adjustment captures. PA3 distinguishes last-normal-line R1 capture
from frame-ending R0 capture, including the last adjustment line. No frame
restart policy or snapshot attribution was changed. Each finding has a
failing-before vector. Opus medium reviews cover the bus and video changes;
the final adjustment-terminal extension uses Muse Spark implementation.

PA7 remains open: AmSpirit measures exactly128dots, matching the phone photo,
but raw request timing is not uniquely determined by this acceptance slot.
The [oracle record](asic-audit-probes-v4.md#phase-4-oracle-follow-up-2026-09-28)
names the missing original-Plus early-arm and adjustment-terminal controls.
Interlace terminal timing and live R5 rewrites remain unprobed.

## Suggested execution order

| Order | Item | Size | Prerequisite |
|---|---|---|---|
| 1 | RC1-RC8 reference corrections (applied 2026-09-28) | XS, docs | none |
| 2 | PA5a, PA5b, PA6a-e vectors (complete, passing regression armour) | S | none |
| 3 | Probe screens: PA1 (`IN` values), PA2/PA3 (split × R5), PA4 (page open bus), PA7 (PRI=0 phase) | M | V4 complete; 6128 Plus photographs recorded |
| 4 | PA1/PA2/PA3/PA4 implemented; PA7 request-phase investigation remains | S each | recorded hardware/oracle evidence and outstanding controls |

## Phase-4 validation

Final code selection, run once after the last code edit (exit0):

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
```

Final line:

```text
select_tests: PASS 41 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Soak (exit0):

```sh
make -C sim CXX=/opt/homebrew/opt/llvm/bin/clang++ soak SOAK_EXPECT=0xe99ab434a5e1cdb3
```

`soak hash: 0xe99ab434a5e1cdb3`; `soak hash matches expected`.
This soak hashes the classic CRTC projection; the Plus fixes intentionally
change behavior but do not change that projection, so no golden was re-minted.

Focused production-T80 `d5-asic-audit` passes PA1/PA4 plus all six PA6b/d
model/tape controls. PA1 executed from SDRAM, PA4 from cartridge. Leaf/motherboard
controls pass, including classic isolation and drivenFF. PA1’s Opus review
suggested requiring a non-FF FDC response; that assertion was added and the
focused D5 run passed again.

Production-T80 diagnostic run:

```sh
python3 scripts/diagnostics/plus_hw_probes_sim.py --out output_files/plus-hw-probes/v4/phase4-sim --screens 21 22 23 24 25 26 27 28 29 30 --frames 40
```

All ten screens completed40 frames. Screens21–25 are pixel-identical to the
pre-fix V4 preservation renders. Screen26 reports78/40/50/58/60/68/78 with the
photographed palette words, screen29 reports50/68/7E/7E and passing controls.
Screen27 captures adjustment0/1/8; screen28 late arm no longer captures on the
last normal line. Screen30 remains pixel-identical: PA7 is still unresolved.

Independent Opus medium reviews accepted PA4, PA1, PA2/PA3, and the additional
Muse Spark PA3 adjustment-terminal implementation. Remaining evidence limits
are recorded in the V4 oracle section, not treated as hardware passes.

### PA7 diagnostic follow-up (2026-09-28)

[V5 design and evidence](pa7-interrupt-phase-followup.md) adds five controls
with predeclared predictions and independent Opus design review. Passive
production-T80 tracing separates the counter event, INT, CPU interrupt-cycle
entry and ACK. AmSpirit gives G=0, width12 gap192, and L/R/I mean gaps
128/144/128. The RET NC result distinguishes the simple uniform-shift reading
under the stated phase assumptions; it is not an exact ASIC delay measurement.
Original6128Plus photographs IMG_3973–3978 now give G0/W192/L128/R128/I128.
RET NC pass-A PRI reference differs by32 dots from both RTL and AmSpirit;
the subsequent ASIC-free rule check identifies shared T80 INT sampling one clock late.
A disposable C16/K64 timing model matches all hardware markers; production repair
awaits classic-CPC impact evidence; shared-CPU scope is authorized. No production RTL
changed; §7.22 remains not-implemented. Summary counts remain unchanged:
222 clauses,181 tested,9 untested,13 conflict,11 scope,2 contradicted,6 not-implemented.

V5 final gate passed41 benches; soak reproduced `0xe99ab434a5e1cdb3`.
The [exact commands and final selection line](pa7-interrupt-phase-followup.md#review-and-validation)
are recorded with the follow-up evidence. Both design and final code received
independent Opus medium review.
