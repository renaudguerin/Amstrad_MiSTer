# Alcon 2020: garbage band above the playfield (2026-10-27)

**Verdict.** The band is the last character rows of the game's HUD CRTC frame, displayed
where the CRTC3 model should hold the R6 border. `rtl/plus/asic_video.v` re-derived vertical
display at every line start as `C4 < R6`. Alcon raises R6 after the `C4=R6` border, so the
model switched display back on. Fixed on `plus/alcon2020-top-lines` (`c48ad4b`) with a vector
that failed first. Simulation proves the fix. The fix is not yet hardware-tested: it needs an
RBF build and a MiSTer recapture.

## Media and settings

| Item | Value |
|---|---|
| Media | `/media/fat/games/Amstrad/cpr/ALCON2020.CPR`, 524 556 bytes, SHA-256 `8ada2eac44d25d272aba79da42a4f6de06e75e91675bb014138a39a6f40b87d9` (a GX4000 port, unrelated to the C4CPC `Alcon 2020.cpr` build `ab157962…`) |
| RBF | `Amstrad_20260929_cec641c.rbf`, SHA-256 `40f7d66b…4dd2c`. Only `34cb464` and `e4dc62a` (tests and build flags) touch `rtl`/`Amstrad.sv` since then, so it represents `master` for this defect |
| CFG | original `f8c6879a…cf3e` (6128+, CRTC bit 0, SSM markers on, Sync filter Full); Raw CRT run used `44e2dfc0…04e0` (bits 36:35 = 2). Original restored and verified, device left at MENU |
| Input | B17 `input_replay.py` with the temporary keyboard-joystick map (F18, LCtrl fire, F20), removed afterwards |
| AmSpirit | Lite 1.16.0, core 2491682, `cpc_model` 4 (6128+), `crtc_type` 3, same CPR hash |

Evidence (ignored) is under `docs/references/alcon2020-top-lines-2026-10-27/` in the main checkout:
MiSTer manifests and PNGs (`ms-001`..`ms-004`), zoom crops, the AmSpirit gameplay SNA
(`as-006`, `43808a88…`), a 250-frame gameplay checkpoint with RAM dump (`g250`),
breakpoint trace `bp2.json`, and the scripts `bptrace.py` and `decode.py`.

## Reproduction

MiSTer (native captures, 768×273 Full / 800×287 Raw CRT): every gameplay capture shows
the band. In Full, rows 0–14 hold garbage spanning x 129–640 (512 px = R1 32 characters),
then come 2 black lines, then the playfield (x 145–624, 30 characters). In Raw CRT the band
is 19 lines with 4 black lines before the playfield. Capture hashes: Full `e94dd6a9…`,
`8dc9ecc6…`, `1c569509…`, `92967bef…`; Raw CRT `a47c0db7…`, `82eb1570…`, `8d8cedee…`,
`897c70c2…`. The user's earlier OSD screenshot `20260928_155412-ALCON2020.png` shows the same.

AmSpirit, same moment: the 16 lines above the playfield are border black. The CRTC type
that matters is the Plus ASIC CRTC (3): the cartridge runs on 6128+/GX4000, and in Plus
mode the core uses `asic_video`, never the classic type 0/1 engines.

## What the game programs

AmSpirit instruction history plus PC breakpoints on the CRTC `OUT`s (`0x664`, `0x66B`,
`0x673`, `0x67B`, `0x697`, `0x69E`) give this per 19 968 µs picture. The writes ride
the 52-line interrupts. The picture is two CRTC frames with VSYNC suppressed in one of them:

| Beam y | Writes | Effect |
|---|---|---|
| ≈26 | R7=127, R6=29, R4=9, R5=n, R12/R13 = playfield scroll address | made at about C4=5 of HUD frame H (VSYNC row): H ends at C4=9, next frame displays fully, no VSYNC |
| ≈80 | R4=27 | inside playfield frame P: P has 28 rows |
| ≈236 | R12/R13 = &0040, R5=8−n | HUD start address and P's adjust (R5_H + R5_P = 8: smooth scroll) |
| ≈286 | R7=5, R6=2 | about 10 lines before H starts: H displays 2 rows (HUD), VSYNC at row 5 |

Frame H is 10 rows plus R5_H, P is 28 rows plus R5_P: 304 + 8 = 312 lines.

## Identification

Rendering AmSpirit's RAM through frame H's addresses (`MA=&0040 + 32·C4`) reproduces the
garbage line by line. Non-zero mode-0 pixel counts of MiSTer Full lines 0–7 are 121, 35,
12, 117, 112, 30, 107, 112: exactly H row 9 RA0–7. Lines 8–14 match row 10 RA0–6,
which is the R5 adjust lines with the pointer running on. Raw CRT starts at H row 8 RA2.
So DISPLAY ENABLE is high for H rows 8–9 and the adjust lines, after the `C4=R6=2` border.

The classic type-1 engine was checked first with a scratch vector (R6 raised at C4=2 and
C4=5 after the border): it keeps the border, and the scratch vector was discarded. The
defect is in the Plus path. `asic_video.v` evaluated `vde <= (charline_n < R6)` at every
line start, so R6=29 re-enabled display at the next line.

## Rule

ACCC v1.11 French edition:

- §18.1 p.189: once the border condition is met, "il faut attendre un nouveau frame
  (C4=C9=C0=0) pour rétablir l'affichage des caractères"; §18.2.1 p.189: "La condition
  commune au rétablissement de l'affichage du fond est C4=C9=C0=0".
- §18.2.4 p.190 (CRTC 3, 4): R6 is tested only at the start of a line; "Le BORDER est
  activé uniquement au moment où C0 passe à 0 lorsque C4=R6"; R6=0 "ne permet pas
  d'activer temporairement le BORDER comme sur le CRTC 1".

The border is therefore a latch: it is set by equality at a line start and cleared only at
the frame origin. Raising R6 does not cancel it, and lowering R6 below C4 without an equality
does not set it. `docs/plus/references/asic-reference.md` does not cover R6 timing; the ACCC
is the applicable source. No Logon System photograph covers this title, so the hardware
claim rests on the ACCC and on AmSpirit agreeing with it.

## Change and proof

`c48ad4b` makes `vde` a latch, clearing on `frame_restart` (equality test included) and
setting on `charline_n == R6`. After `SNA_LOAD` the first line seam keeps the previous `<`
derivation, because the latch state is not serialized.

- New `t03i R6 border latched until frame origin` (Alcon's shape plus the reverse case).
  Before the fix: `FAIL: t03i … raising R6 does not cancel the border: DE: expected 0, actual 1`.
- `t03e`'s last assertion ("display resumes once C4!=R6 at a line start") encoded the
  misreading and now asserts the §18.2.4 rule.
- Review (Codex Sol 6.1 medium, run `20261027T091602Z-3338-a804`): no blocking findings. One
  major: §18.2.1's restore condition is C4=C9=C0=0, which a C4 overflow wrap (§12.5 p.103,
  R4 lowered below C4) reaches without a frame end, and `frame_restart` missed it. The latch
  now clears on the shared `pointer_frame_origin` (equal to `frame_restart` under IVM and
  sync-interlace). New `t03j R6 border cleared by C4 overflow` failed first:
  `DE: expected 1, actual 0`.
- Gate: `python3 sim/select_tests.py --run` → `select_tests: PASS 3 benches:
  run/asic_video_tests, run/p1_video_tests, run/p1_mobo_bench_tests`.

## Open

- **Hardware recapture** after an RBF build: expect black rows above the playfield in
  both Full and Raw CRT. Rerun `mister_alcon.json` and `mister_alcon_raw.json` from the
  evidence directory, then the fire replay.
- **§18.3.4 granularity.** "Positionner R6 à 0 lorsque C4 et C9 valent 0, mais que C0>0
  n'aura aucune incidence avant le nouveau frame" conflicts with `t03e`'s middle
  assertion, which borders at the next scanline of the same character row. Either the
  line-start test is per character row, or the sentence assumes R9=0. This was left unchanged
  because the reading is ambiguous; it needs an author or hardware answer.
- The 2 (Full) or 4 (Raw CRT) black lines between the band and the playfield were not
  analysed separately. They should disappear into the border once the fix is on hardware.
