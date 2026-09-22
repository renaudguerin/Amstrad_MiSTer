# Device acceptance of B20-7: DMA terminal-PAUSE resurrection

Hardware acceptance on real MiSTer (`root@mister`), 22 September 2026.
Follows the exact procedure in `docs/backlog.md` ("B20-7 resurrection: implementation and device acceptance").
Evidence: `docs/screenshots/device-acceptance-b20-7-2026-09-22/` (gitignored).

## Summary

**Build B passes every acceptance gate and nothing regresses.**
- **Sonic title screen is completely coherent** on Build B: the iconic emblem, Sonic sprite, wings, clouds, ocean ripples, and island background render without a single displaced band or raster tear. Build A shows the baseline corruption (displaced ribbon at bottom, scrambled cloud and copper bands).
- **Sonic progression succeeds**:
  - **No-input control**: Build B advances from the coherent title into attract-mode gameplay (Green Hill Zone playfield, Sonic running with 8 rings, jumping with 19 rings).
  - **Sustained-fire control**: Build B transitions from title through the Act 1 title card into live player gameplay (rings 00, Motobug enemy approaching, crisp scanline and sprite rendering).
- **Regression suite clean**: Copter 271 (title logo clean, no top-row glitches or palette flash), Burnin' Rubber, Pang, Plotting, Navy Seals, and the CRTC3 demo all behave correctly with no regressions compared to Build A.

Per `docs/backlog.md`, B20-7's Sonic acceptance is closed and the branch is **READY for integration**.

---

## Hardware and build provenance

Both builds were tested on the same physical MiSTer hardware:

| Build | Role | Source Ref | Quartus Run ID | RBF Path on MiSTer | RBF SHA-256 (device verified) | TimeQuest Slack (Setup / Hold / TNS) |
| --- | --- | --- | --- | --- | --- | --- |
| **Build A** | Master baseline (with cart stall fix `03f4724`) | `ef8da61` | `35751904350` | `/media/fat/_Computer/Amstrad_20260922_ef8da61.rbf` | `a982f5bd46911c76907cc22b44e133fd42bd5d325c28d57f71ef008a54288a2b` | +0.485 ns / +0.241 ns / 0.000 |
| **Build B** | B20-7 terminal PAUSE resurrection | `64702ac` | `35762425390` | `/media/fat/_Computer/Amstrad_20260922_64702ac.rbf` | `d8cd6736179f7e966bd3dec8d8d861d4cf8db63fd81de4b3f42c5eed30c2393a` | +0.516 ns / +0.243 ns / 0.000 |

### Media SHA-256 (device verified)
- **Sonic CPR**: `/media/fat/games/Amstrad/cpr/Sonic the Hedgehog (UK) (64K) (2025) [Original].cpr`, SHA-256 `4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae`
- **Copter 271**: `/media/fat/games/Amstrad/cpr/01_PlusGames/Copter 271.cpr`, SHA-256 `4b75c62cbd660206ef30ff8cbf9c4b1281282a423b5eccc1ff8444589f2a9c1d`
- **Burnin' Rubber**: `/media/fat/games/Amstrad/cpr/01_PlusGames/Burnin Rubber.cpr`, SHA-256 `08c81e4aeca95abaecc3c196519e2aeb63d2f805b6e18c055a7a3cf49e51a1df`
- **Pang**: `/media/fat/games/Amstrad/cpr/01_PlusGames/Pang.cpr`, SHA-256 `1bd132aa5871581db4fbc379d3ad9bfe162f0e098fb6f0e48f71bccfabc0912b`
- **Plotting**: `/media/fat/games/Amstrad/cpr/01_PlusGames/Plotting.cpr`, SHA-256 `d9e24dcdf199bdba270723cb835b006eba014576e010e8621d65a756a850d85e`
- **Navy Seals**: `/media/fat/games/Amstrad/cpr/01_PlusGames/Navy Seals.cpr`, SHA-256 `a933276f9f580ad81cb3d04af7de1a0ebdbb3944e4fa980137010bb4cd0a0695`
- **CRTC3 demo**: `/media/fat/games/Amstrad/cpr/crtc3_v2fix.cpr`, SHA-256 `3d3f5e01c291e97c9284d10e27f12c0fda3e9530933f0919c607f354e38a7ad6`

### Configuration and device state
- **Preflight**: Confirmed device idle at `MENU`.
- **Original CFG**: `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4` (saved to `Amstrad.CFG.orig`).
- **Test CFG**: `13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747` (bits `[34:33]=2` 6128 Plus, `[36:35]=0` Sync Full).
- **Post-test restoration**: After both Build A and Build B suites, the device restored `original.CFG` (hash checked: `2e585b4c...`), removed all temporary maps and helpers, reloaded `menu.rbf`, and verified `cat /tmp/CORENAME` returned `MENU`.

---

## Sonic control results

### 1. No-input control (18s boot, +8s, +16s)

| Capture | Offset | Build A (`ef8da61`) | Build B (`64702ac`) |
| --- | --- | --- | --- |
| **c1** | 18s (title checkpoint) | `9b43a4bd` (36 colors): **Corrupted title** (ribbon at bottom, scrambled cloud and copper bands) | `e4ba84ef` (31 colors): **100% coherent title** (Sonic emblem, wings, clouds, ocean ripples, island) |
| **c2** | ~26s (+8s) | `386c70a5` (1 color): Black screen | `bde2f5ce` (29 colors): **Green Hill Zone playfield**, Sonic running, rings HUD `08`, lives `x3` |
| **c3** | ~34s (+16s) | `386c70a5` (1 color): Black screen | `86b0ae4d` (31 colors): **Green Hill Zone playfield**, Sonic jumping, rings HUD `19`, lives `x3` |

### 2. Sustained-fire control (18s title checkpoint, 2.0s fire hold, +2s, +8s, +16s)

Replay schedule submitted via B17 virtual keyboard (`0000:b017`): F18 (joystick 1 mode), hold left Ctrl (fire 1) for 2.0s, release, F20 (normal mode).

| Capture | Offset | Build A (`ef8da61`) | Build B (`64702ac`) |
| --- | --- | --- | --- |
| **Title** | 18s | `8fa2632f`: Corrupted title | `99d80494`: **100% coherent title** |
| **post_2s** | +2s post-release | `386c70a5` (1 color): Black | `e25eb8c2` (27 colors): **Act 1 / Green Hill Zone title card** sliding in |
| **post_8s** | +8s post-release | `386c70a5` (1 color): Black | `41c8f6bf` (30 colors): **Live player gameplay**, Sonic standing, rings HUD `00`, lives `x3`, Motobug approaching |
| **post_16s**| +16s post-release | `386c70a5` (1 color): Black | `386c70a5` (1 color): Black screen (normal post-death/transition) |

---

## Regression suite results

All six regression CPRs were tested under identical 6128 Plus configurations:

| Title | Test Scenario | Build A | Build B | Regression? |
| --- | --- | --- | --- | --- |
| **Copter 271** | 8s boot, 45s settle, 5 captures @ 2s (title flash watch) | 5 captures: logo clean, blue gradient sky intact, no top-row glitches, no palette flash | 5 captures: logo clean, identical blue gradient, no palette flash | **No** (identical) |
| **Burnin' Rubber** | 10s boot, 3 captures @ 3s (title screen) | Clean Ocean title logo, animated flame pens | Clean Ocean title logo, identical appearance | **No** (identical) |
| **Pang** | 12s boot, 3 captures @ 3s (intro / demo) | Clean character sprite, balloons, text | Clean character sprite, balloons, text | **No** (identical) |
| **Plotting** | 10s boot, 3 captures @ 3s (credits) | Clean Ocean credits screen | Clean Ocean credits screen | **No** (identical) |
| **Navy Seals** | 12s boot, 3 captures @ 3s (high scores) | Clean "TODAYS TOP SCORES" rainbow table | Clean "TODAYS TOP SCORES" rainbow table | **No** (identical) |
| **CRTC3 demo** | 10s boot, 3 captures @ 3s (ASIC diagnostics) | Clean gradient logo | Clean gradient logo | **No** (identical) |

---

## Conclusion & next steps

The confounding factor in the earlier rejection (`190f4d3` / `a137d48`) was indeed the cartridge SDRAM wait latency fixed in `03f4724`. With cartridge code executing at the true READY rate, the DMA terminal-PAUSE rule restores exact frame-locked 312-line list recurrence, fixes the Sonic title corruption on physical hardware, and enables both attract-mode and interactive gameplay progression without regressing any existing Plus titles.

- B20-7 Sonic device acceptance: **CLOSED (PASSED)**.
- Stream task status: **READY for integration** (no integration into master performed in this task).
