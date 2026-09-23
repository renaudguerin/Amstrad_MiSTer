# Phase P9/P10 Amstrad Plus Hardware Test Checklist

Targeted hardware verification plan for physical MiSTer testing of the Amstrad Plus & GX4000 core.

P0-P9 implementation is not a pass for this checklist. Check an item only against an exact,
full-effort RBF whose constrained internal domains have non-negative setup/hold slack and
zero TNS; record that the TimeQuest summary does not prove unconstrained external I/O paths.
The 2026-08-29 exploratory
sample used an unrecorded RBF/configuration and therefore supplies symptoms, not checked
passes: Panza grey/blue; RoboCop 2 garbled sprites; Arnold 5 keyboard inoperable; BASIC
cartridges `Drive A: read fail`; approximately half of sampled cartridges loaded. See
`hardware-checkpoint-findings.md` for the P10 repair order.

## Latest recorded results

The [2026-09-23 retest](../investigations/hardware-runs/plus-titles-8b18ac0-2026-09-23.md)
on integrated build **`8b18ac0`** (contains B20-7 terminal-PAUSE, cartridge
READY-rate fix and tape-bank relocation; device copy hash unchecked) reports:
**Sonic GX confirmed fixed** (title symptom only, progression depth unstated),
**Navy Seals / World of Sports sprite-line flicker appears fixed** (no assigned
RTL cause; closure pending a repeatable capture), and **Switchblade / Eerie
Forest still not loading** (failure shape unstated). Checklist boxes below stay
unchanged; the retest is recorded in the run table, not promoted to passes.

A subsequent paired check on the same `8b18ac0` RBF isolates Eerie Forest's
immediate failure to its malformed outer RIFF length. A copy correcting only
that header boots and progresses from the demon intro to the Logon System
scene; original media remains rejected on the old RBF. See the [container
finding](eerie-forest-container-2026-09-23.md). Switchblade's original CPR
produces an all-black native capture on the same RBF; the [ASIC unlock
finding](switchblade-unlock-2026-09-23.md) records the source repair and
production-T80 progress. The [exact `41a1f27` device retest](../investigations/hardware-runs/plus-cartridge-originals-41a1f27-2026-09-23.md)
now boots both unchanged CPRs. Switchblade reaches its title/high-score cycle;
Eerie Forest reaches the Logon System scene but then holds a striped forest
frame. Full gameplay/demo completion remains untested.

The [September 13 report](../investigations/hardware-runs/hardware-evidence-2026-09-13.md) confirms that
build **`a0778b6`** (fix "general: reset PSG R7 to 0x00 so bare-metal keyboard
scans work") **fixes all known keyboard / joystick issues with `arn5diag`, `Pang`,
and `Plotting`**. AY-3-8912 /RESET clears all registers to 0x00 (GI datasheet),
setting Port A to input; previous 0xFF reset wedged uninitialized R14 reads to 0x00,
causing active-low keys/fire buttons to read as permanently pressed. This closes the
held-fire symptom in Pang and Plotting, and cold-boot keyboard navigation in Arnold 5
(`arn5diag`). In addition, Copter 271's logo is fixed on device (`b5c3014`), and its
title flash is much improved (`05cb9fd`).

The earlier [September 12 report](../investigations/hardware-runs/hardware-evidence-2026-09-12.md) confirmed BASIC
boot fixed on **6128 Plus / `5c16b17`**. Left-edge sprite corruption is much
improved, possibly fixed. B6 Full versus Raw pixels showed no visible difference so far.
This closes the named BASIC boot symptom, not the multi-model System Cartridge checklist
or disk I/O. The exact output/media configuration and on-device RBF hash are unrecorded.

The earlier [September 9 report](../investigations/hardware-runs/hardware-evidence-2026-09-09.md) records the
`ce1d2da` retest and maps the SHAKER captures. Burnin' Rubber was reported OK,
Enforcer/Tintin seem good, and CRTC3's right-edge leak appears fixed. These
limited observations do not check the multi-feature title/subsystem boxes below.
Input, definitive sprite closure, DMA pitch and classic diagnostic acceptance
remain open. Disk access was blocked by boot failure at that earlier retest;
the September 12 boot confirmation permits subsequent disk testing.

## Prerequisite gate

- [ ] **Exact full-effort build accepted:** integration commit and RBF SHA-256 match;
  Quartus effort is `full`; constrained setup and hold slack are non-negative; constrained
  TNS is zero; external-path limitation is recorded. If this is unchecked, do not promote
  any result below to hardware-confirmed.

## Append-only test runs

Add one row per distinct environment. Do not overwrite an earlier run. The complete field
definitions are in `hardware-checkpoint-findings.md` §7; put per-title observations beneath
the corresponding run ID.

| Run ID/date | Commit | RBF SHA-256 | Effort; setup/hold/TNS | MiSTer version | Plus/classic model | CPR and mounted media | Reset/load order | Comparison source |
|---|---|---|---|---|---|---|---|---|
| 2026-09-09 Plus | `ce1d2da67c2598c0dd06208b9fc14c52ada01712` | `fef2c8553e85456a86bc6d28753cdbb43c386106b5cd719bbf808cea84dc6144` (local; device unverified) | full; +0.098/+0.238 ns; zero TNS | Unrecorded | 6128 Plus; Live blanking | Named titles in report; hashes/mounted media unrecorded | Partial; see report; no Dandanator | User observations / supplied captures |
| 2026-09-09 Classic | Same build | Same local hash | Same build | Unrecorded | 6128, Plus off; CRTC1, DSC4 also CRTC0; Full/Live/Off as recorded | SHAKER 2.7, DSC4, Amazing Demo; hashes unrecorded | DSC4 before SHAKER; reset reported; full sequence unrecorded | User observations / supplied captures; reference comparison pending |
| 2026-09-12 Plus | `5c16b17` (user-confirmed) | `8b3b5bed518165040f8e578c83f061891fa64d58ce3b52fb07d5765765506468` (delivered local artifact; device unverified) | full; +0.320/+0.247 ns; zero TNS | Unrecorded | 6128 Plus | BASIC, Pang, Plotting, Copter 271; exact media unrecorded | Unrecorded | User observations; see September 12 report |
| 2026-09-12 B6 comparison | `5c16b17` (user-confirmed) | Same delivered local hash | Same build | Unrecorded | Classic model/CRTC and output connection unrecorded; Full / Raw pixels | Amazing Demo, DSC4, SHAKER A (T); exact versions unrecorded | Unrecorded | User reports no visible difference so far |
| 2026-09-13 Plus input | `a0778b6` (user-confirmed) | Build artifact | full | Unrecorded | 6128 Plus | `arn5diag`, `Pang`, `Plotting` | Unrecorded | User hardware testing confirms all known keyboard/joystick issues fixed across all three titles |
| 2026-09-23 Plus titles | `8b18ac0` (user-reported) | `26185f485ac72a6be7e3ee17f9fc2e57a2e09c595ba8061d53a9aca90c960da2` (local; device unverified) | full; setup +0.215 ns minima per device record; hold/TNS unrecorded here | Unrecorded | Unrecorded | Sonic GX, Navy Seals, World of Sports, Switchblade, Eerie Forest; media hashes unrecorded | Unrecorded | User observations; Sonic fixed, Navy/World of Sports flicker appears fixed, Switchblade/Eerie Forest still not loading; see retest record |
| 2026-09-23 original Plus CPRs | `41a1f27` | `6c36309368659edbbfe1044e09a804639f6b7ec9c02b68526ff7d488e122b331` (artifact and device matched) | clean full; minimum setup +0.770 ns, hold +0.217 ns, constrained TNS zero | On-disk Main hash recorded in capture manifest; running version unverified | Explicit 6128+ / Full CFG; live OSD unverified | Original Switchblade `d958e2b1…` and Eerie Forest `72485083…`; full hashes in device record | Fresh RBF/CPR load per title; Eerie repeated after 45-second delay | Native serial captures and AmSpirit comparison; both boot, Eerie later striped frame stalls ([record](../investigations/hardware-runs/plus-cartridge-originals-41a1f27-2026-09-23.md)) |

---

## 1. OSD & Model Selection Setup

- [ ] **OSD Media Menu Layout**:
  - Open OSD and verify that primary media loading entries are grouped together:
    - `Mount A: (DSK)`
    - `Mount B: (DSK)`
    - `Load tape (CDT)`
    - `Load Plus cartridge (CPR)`
- [ ] **Model Selection**:
  - Under `Hardware` menu, test all `Plus model` options:
    - `Off`: Classic CPC (CRTC type selectable 0 or 1, standard Gate Array, 64KB/128KB).
    - `GX4000`: 64KB RAM, no FDC, no tape, fixed upper ROM page 1.
    - `6128+`: 128KB RAM, FDC present, unexpanded bare machine (`/EXP=1`) resolves ROM-select 0 to AMSDOS page 3.
    - `464+`: 64KB RAM, tape present, unexpanded bare machine (`/EXP=1`) resolves ROM-select 0 to AMSDOS page 3.
- [ ] **Cartridge Detach** (obsolete as written: since 2026-09-01 `R[32]` detaches the
  Dandanator only, and a CPR load replaces the Plus image atomically; see B6):
  - Verify `Reset & Detach Cartridge` unloads the current cartridge image and returns to basic unexpanded state.

---

## 2. Cartridge Boot & Auto-Reset (P0, P9)

- [x] **Auto-Reset on CPR Load** (MGL route, RBFs `cdcb3c3`/`4027f5e`, 2026-09-22; OSD
  selection itself not observed; see the
  [device record](../investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md)):
  - Select and load a `.cpr` file from OSD.
  - Verify that the core automatically asserts system reset during download, atomic commit occurs, and the CPU resets directly into cartridge page 0 (`&0000`) without manual OSD reset.
- [ ] **Case Tolerances (P9)**:
  - Test `.cpr` dumps with both `AMS!` (uppercase) and `Ams!` (mixed case) RIFF headers.
  - Test `.cpr` dumps with both `CB00`..`CB31` and `cb00`..`cb31` chunk IDs.

---

## 3. Real Hardware Titles Test Matrix

### A. Firmware / System Cartridge
The 6128 Plus BASIC boot symptom is confirmed fixed on September 12. The
box below retains the wider banner, model and cartridge checks.

- [ ] **Amstrad System Cartridge (v4)**:
  - [x] Boot on `6128+`: verify Firmware 4.0 banner, Locomotive BASIC 1.1, and AMSDOS
    (2026-09-22, `cdcb3c3`: v4 menu, BASIC 1.1, `CAT` answers `Drive A: disc missing`;
    no AMSDOS banner is printed, so `CAT` is the check).
  - [x] Boot on `464+`: verify Firmware 4.0 banner and Locomotive BASIC 1.1 (no AMSDOS)
    (same run: `CAT` answers `Press PLAY then any key:`).
  - Boot on `GX4000`: v4 banner, then BASIC `Ready` and a cursor, no menu. Logical ROM 7
    maps to page 1 on GX4000, so the far call to the page-3 menu lands in BASIC. Lines after
    `Ready` are state-dependent. See [gx4000-system-cartridge-2026-09-22.md](gx4000-system-cartridge-2026-09-22.md).

### B. Commercial Cartridges (GX4000 / Plus)
- [ ] **Burnin' Rubber**:
  - Boot page 0 auto-start.
  - ASIC 12-bit RGB palette display.
  - Hardware sprite cars (scaling, priority, coordinate positioning).
  - Audio playback (PSG music + digital engine sound effects).
- [ ] **RoboCop 2**:
  - Screen split: SSA/SPLT split-screen score bar at top/bottom.
  - Soft horizontal scroll: sub-character 16-dot pixel shifting.
  - PRI (Programmable Raster Interrupt) raster effects.
  - 3-Channel DMA sound playback during gameplay.
  - Hardware sprites for player character and enemies.
- [ ] **Pang**:
  - [x] Keyboard/joystick input: confirmed fixed on hardware (build `a0778b6`; PSG R7 reset to 0x00 fixes permanently held fire).
  - PRI scanline interrupt synchronization.
  - 16 hardware sprites with dual-palette bank switching.
  - High-color background and sprite layers.
- [ ] **Navy Seals**:
  - Smooth multi-directional soft scrolling.
  - Split screen status display.
  - Multiplexed hardware sprites.
- [ ] **Klax**:
  - Isometric tile rendering and sprite falling animations.
  - Palette animation and timing stability.
- [ ] **Switchblade / Dick Tracy / Plotting / Tin Tin on the Moon**:
  - [x] Plotting keyboard/joystick input: confirmed fixed on hardware (build `a0778b6`; PSG R7 reset to 0x00 fixes permanently held fire).
  - [x] Unchanged Switchblade CPR boots through intro, title graphics and high-score cycle on `41a1f27`; gameplay untested.
  - Cartridge loading, page banking, and gameplay stability.

### C. Demos & Diagnostics
- [ ] **`arn5diag.cpr` (Arnold 5 diagnostic)**:
  - [x] Cold-boot keyboard scan / menu navigation: confirmed fixed on hardware (build `a0778b6`; PSG R7 reset to 0x00 allows R14 matrix reads from cold boot).
  - Plus hardware diagnostic tests.
- [ ] **`crtc3_v2fix.cpr`**:
  - Diagnostic test for CRTC 3 timing, syncs, and status registers.
- [ ] **PhX Demo**:
  - ASIC multiple raster splits per frame.
  - 12-bit RGB color cycling.
  - Soft vertical and horizontal hardware scrolling.
  - Continuous 3-channel DMA sound streaming.
- [ ] **Batman Forever Demo**:
  - High-density color raster splits and oversized display windows.
- [ ] **SHAKER Diagnostic Suite**:
  - With `Plus model = Off`: verify CRTC 0 and CRTC 1 results match reference photos (`shaker.logonsystem.eu`).
  - With `Plus model = 6128+`: verify CRTC 3 behavior and identification.

---

## 4. Subsystem Verification Matrix

| Subsystem | Key Registers / Features | Test Method | Pass Criteria |
|---|---|---|---|
| **MMU / Banking** | RMR2 (`&7Fxx` with `&B8`/`&A4`/`&AC`/`&B4`), MRER (`D[2]`, `D[3]`), `&DF00` | System Cartridge / BASIC `|CPM` / Soft relocation | Low ROM relocates to `&0000`, `&4000`, or `&8000`; MRER D[2]/D[3]=1 disables ROM and exposes underlying RAM |
| **ASIC Unlock** | 16-byte unlock sequence to `&BC00` | Any Plus title / ASIC register writes | Registers accept writes only after valid unlock sequence |
| **ASIC Video** | SSA (`&6802/3`), SPLT (`&6801`), SSCR (`&6804`) | RoboCop 2, PhX demo, Navy Seals | Split screen line is clean with no jitter; soft scroll shifts pixels smoothly |
| **ASIC Sprites** | Coordinates (`&6000-&607F`), Pixels (`&4000-&5FFF`), Magnification (`&6004+8*n`) | Burnin' Rubber, Pang | 16 sprites rendered without missing scanlines; x1/x2/x4 scaling correct |
| **ASIC DMA Sound** | SAR (`&6C00`), CDR (`&6C02`), CPR (`&6C04`), DCSR (`&6800`) | RoboCop 2, PhX, Burnin' Rubber | DMA channels A, B, C stream audio without clicks/underruns; DMA INT fires |
| **PPI & ADC** | PPI 8255 quirks, ADC registers (`&6808-&680F`) | Keyboard matrix, analog paddle readings | Default paddle reads `3F 3F 3F 3F 3F 00 3F 00`; keyboard fully responsive |
| **Classic Non-Interference** | CPC 464/664/6128 modes, DSK, CDT, SNA | Standard games (e.g. Gryzor, Arkanoid) | 100% bit-identical classic behavior; tape and disk load cleanly |
