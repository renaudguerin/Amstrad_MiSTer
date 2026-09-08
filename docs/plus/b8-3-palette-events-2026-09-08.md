# B8-3 palette write events

Date: 2026-09-08. Task branch `codex/plus/b8-field-palette`. This record covers
the palette repair only; FIELD (B8-2, accepted), SDRAM, snapshot and video work
are separate.

## Defect (B8 review §B8-3)

`asic_ga_timing` recognized each legacy INKR write, but the motherboard passed
only stored colour arrays to `asic_regs`, whose translation ran only on value
change. A repeated same-value legacy write is a write event with no value
change, so it was lost: select pen 0 → legacy black → ASIC-page white → repeat
same legacy black left white instead of black.

## Primary sources (not simulator-derived)

- Arnold V Issue 1.5 §2.2: the 32×12-bit palette has two ports. Primary
  `&6400-&643F` (even byte RED high/BLUE low, odd byte GREEN low). Secondary
  is the 5-bit interface: logic maps the written 5-bit colour to the entry at
  the palette-pointer address. First 17 entries only (pointer `00-0F` pens,
  `10-1F` border).
- [KT] Extra CPC Plus Hardware Information, Palette table: HW20 Black 0,0,0 →
  `000`; HW21 Bright Blue 0,0,15 → `00F`; HW0 White 6,6,6 → `666`;
  HW16 Blue 0,0,6 → `006`. Sprite colours unreachable via legacy.
- `docs/plus/references/asic-reference.md` §6 (same rules, condensed).

## Fix

- `rtl/plus/asic_ga_timing.v`: new `LEGACY_PAL_WR`, `LEGACY_PAL_ADDR[4:0]`,
  `LEGACY_PAL_DATA[4:0]`. `WR = ~reset & (inkr_en|border_en)` — the existing
  accepted strobes (sequencer window + `&7Fxx` decode). `ADDR` canonical
  0..15 pens / 16 border (`border_en ? 16 : {0,inksel[3:0]}`); `DATA = D[4:0]`.
  `inksel` cannot change same-cycle (`ink_en` vs INKR mutually exclusive), so
  ADDR is coherent. `BORDER_O`/`INKR_O` retained for the `asic_video`
  fallback and the `asic_regs` one-shot reset import (never runtime events).
- `rtl/Amstrad_motherboard.v`: `leg_pal_wr/addr/data` event wire from
  `asic_ga` to `asic_page`, `leg_border/leg_inkr` shadow arrays
  to `asic_page` for the reset import only. The same arrays still feed the
  `asic_video` fallback. No other production change.
- `rtl/plus/asic_regs.v`: ports `leg_pal_wr/addr/data` carry runtime writes; `leg_border/leg_inkr`
  serve reset/initial import only. Reset: `pal[*]=0`, `leg_inkr_q=all-1s`,
  `leg_border_q=5'b11111`, `import_pending=1` (prior zero state + sentinel,
  `initial` matches). First clock after release with `import_pending`: diff
  the CURRENT shadows against `q` into entries 0-16, latch `q`, clear pending;
  never sampled again, so idle shadows cannot overwrite a later page write.
  Source order per edge: page/sna writes, then import (wins, as the old
  translate did), then events (a coinciding event carries the newer colour
  and wins). All blocks `!reset`-gated, so reset dominates. Sentinel note:
  `q=all-1s` is HW31 exactly as before, so a retained HW31 shadow imports as
  0 rather than `0x66F`. This existing reset quirk is outside the repair. A GA-shadow snapshot restore must not drive event `WR`
  (B8-5: restored 12-bit palette survives).

## Regression (`sim/plus/b8_palette_*`, in the default gate)

New bench top (production motherboard, `cpu_din` muxed from `plus_asic_rd` as
in `Amstrad.sv`) + C++-driven CPU (I/O WR, MEM WR/RD; HOLD=127 covers the GA
sequencer window). All stimulus is bus cycles; shadows never driven as events.
Source-derived expectations above.

- pen: select 00 → `54` (HW20) → page `FF/0F` white → repeat same `54` → read
  `&6400` expects `00/00`.
- border: select alias `11` (nonzero low nibble) → `54` → page white at
  `&6420` → repeat same `54` → read expects `00/00`; pen1 stays grey `66/06`.
- positive: changed `55` (HW21) → `0F/00`. guard: sprite entry `&6422` `00/00`.

Failing-before events (exact HEAD `55151a0`, no RTL edit):
`make -C sim/plus obj_dir/b8_palette/b8_palette_tests` then
`./sim/plus/obj_dir/b8_palette/b8_palette_tests` → exit 1, 4 fails (pen
repeated even/odd `FF/0F`, border repeated even/odd `FF/0F`); first blacks,
whites, alias guard, blue control and sprite guard pass. Log + command under
ignored `docs/references/b8-palette-evidence/b8-palette-before.*`.
Failing-before reset (hardcoded `666/006` candidate, new test only):
same commands → exit 1, 3 fails (`asic-only reset pen0 even got 66 want 00`,
`pen0 odd got 06 want 00`, `border even got 06 want 0F`); all 17 event checks
still pass. Log + command under ignored
`docs/references/b8-palette-evidence/b8-asic-reset-before.*`.
After: exit 0, all 34 PASS lines (33 checks + summary), log
`b8-asic-reset-after.log` in the same directory.

New ASIC-only reset regression (same bench, `asic_reset_i` ORed into
`plus_asic_reset`; machine `reset` stays 0, bus idle so no event can fire):
legacy pen0 HW20 black + border-alias HW21 blue over the bus, page-white
pen0 to diverge, pulse `asic_reset_i` 16 clks, release, 64 clks settle, then
expect pen0 `00/00` (retained black), border `0F/00` (retained blue), pen1
`66/06`, sprite `00/00`; page-white pen0 again and expect `FF/0F` after 64
idle clks (no delayed replay).

## Validation (this checkout)

- `b8_palette`: exit 0, 34 PASS lines (repeated writes + reset import +
  post-import page stickiness).
- `asic_regs` leaf: 12 groups pass. `b8_field` (B8-2 preserved), `p1_mobo_bench`
  (m1–m13 incl. m12 ASIC border), `plus_p8`, `p4_sprites_regs`: all exit 0.
- lint: `motherboard-lint`, `asic_regs`, `asic_ga_timing`, `p4`, exact-Makefile
  `b8_palette` top: all pass. `plus_p8` elaborates via its build.
- Leaf `a04` emits events per pen/border (table coverage retained); leaf/`p4`/
  `p8` fixtures retain GA reset shadow values (`16`/`0`) for import. The
  `p4`/`p8` fixtures tie runtime events low. Leaf `a04` drives events explicitly.
- Parent final gates: `make -C sim`, `make -C sim lint`, and
  `make -C sim soak SOAK_EXPECT=0x6e8258198d6e6137` all exited 0. Soak
  matched exactly over 2,845,088 samples. Full simulation includes both
  B8 regressions and the production-shaped FDC boundary test. Raw logs:
  ignored `docs/references/b8-palette-evidence/b8-palette-final-{sim,lint,soak}.log`.

## Independent review

Gemini 3.8 Flash high reviewed the frozen Muse-authored diff against
`55151a0b1b75e894bd22eff1f6dd69663713d9be`: **CLEAR**, complete handoff and
exit 0 (`20260908T053759Z-946-d90c`). Review covered producer acceptance,
address/data coherence, reset dominance and import/event ordering,
motherboard wiring, and failure-first regression evidence. It did not
execute Z80 software or verify hardware. The test has 33 byte checks plus
one summary PASS line. Opus session capacity was exhausted; the coordinator
authorized this bounded cross-provider fallback.

## Scope and residuals

Changed: the three RTL files above, the new bench/test (`asic_reset_i` reset
regression) + Makefile target, `asic_regs_test.cpp` event API + shadow ties,
`p4`/`p8` tie-offs. B8-1/B8-2/B8-6 and unrelated code untouched; no snapshot
apply ports/controller, SDRAM, or worktree changes.
Residuals: scripted bus only (no executed T80/title test); filter-independent
palette (video timing untested); Copter causality unknown (legacy reaches
0–16, not sprite entries); no hardware retest; Quartus/RBF out of scope.
