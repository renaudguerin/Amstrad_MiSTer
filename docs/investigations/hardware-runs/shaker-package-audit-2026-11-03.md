# SHAKER 2.7 Package Audit: `local/CSLSSM/` vs `CSL_27/` — 2026-11-03

## Executive Summary & Canonical Recommendation

A static, byte- and sector-level audit was conducted comparing the newly supplied SHAKER package in `local/CSLSSM/` against the prior upstream corpus (now backed up at `local/test_media/shaker/pre-CSLSSM-20261103/`), as well as private patched CSL scripts in `local/investigations/shaker-sweep-2026-11-03/csl/`.

### Key Conclusions

1. **C1 Marker Typo (`02F1`/`02F2` vs `03F1`/`03F2`): FIXED by authors.**
   In `local/CSLSSM/SHAKER-C-1.CSL`, lines 180 and 182 have been updated to `wait_ssm 0x03F1` and `wait_ssm 0x03F2` (previously `0x02F1` and `0x02F2`). This file is **100% byte-identical** to our private patch `SHAKER-C-1f.CSL` (SHA-256: `cb727ed4cdcfb50cdca1d44102e25bb56c8c307557e47a09f3b62755ce99f8bb`).
2. **B1 physical key Z vs W: French-layout workaround still required.**
   `local/CSLSSM/SHAKER-B-1.CSL` line 41 remains `key_output 'Z'` (byte-identical to original `CSL_27`). Because SHAKER scans the physical matrix directly while `--layout fr` translates keystrokes through firmware AZERTY positions, sending `'Z'` presses physical UK `'W'` and misses menu option B. **The private patch `SHAKER-B-1w.CSL` (`key_output 'W'`) must be retained** when executing under `--layout fr`.
3. **Disk Payloads (`shaker27.dsk`):**
   - Core modules `SHAKE27A.BIN`, `SHAKE27B.BIN`, `SHAKE27C.BIN`, and `SHAKE27D.BIN` are **100% byte-identical** in both payload and AMSDOS headers across both disk images.
   - Module `SHAKE27E.BIN` was updated by the author, growing from 18,937 to 20,240 bytes (incorporating new tests for CRTC 0, 1, 2, and Z80 IORQ timing). A leftover backup file `SHAKE27E.BAK` (20,191 bytes) is present on the new disk.
4. **Script Semantics in Modules B, C, D:**
   - `SHAKER-B-2.CSL`: Replaced unsupported `wait_ssm 0x01E8`..`0x01EF` (which hung CRTC 2) with fixed `wait` statements.
   - `SHAKER-C-2.CSL`: Fixed repeated typo `0x02C7` -> `0x02C8`; commented out deadlocked/unsupported markers `0x02D8` and `0x02D9`; removed CRTC 0-only test 'R' (`wait_ssm 0x03AD`).
   - `SHAKER-C-3.CSL` and `SHAKER-C-4.CSL`: Removed CRTC 0-only test 'R' (`0x03AD`).
   - `SHAKER-D-[0-4].CSL`: Author removed all `wait_ssm` statements in Module D and replaced them with fixed `wait <microseconds>` delays (CSL 1.4 style for headless CPCSE). `SHAKER-D-1.CSL` restored `shaker27.dsk` instead of `shaker26.dsk`.
5. **Impact on Existing Screenshots and Comparisons:**
   - **The CRTC1 A/B/C baseline remains applicable.** The exact script logic and disk binaries executed in the 2026-11-03 hardware sweep (`A-1` 127 captures, `B-1` 139 captures with `W`, `C-1` 80 captures with `03F1`) match the new package byte-for-byte. The 346 captures remain baseline evidence for their recorded RBF and history; the package update does not invalidate them.
6. **RTL Rerun Decision on `5fba168`:**
   - **No full fresh run is required on `5fba168`.** A, B, and C need no re-execution. Only Module D-1 (to test if the new fixed-wait script paces past the `0305` input stop) and Module E-1 (7 captures of the recompiled binary) represent new runtime targets when hardware runs resume.
7. **Canonical Recommendation:**
   - Adopt `local/CSLSSM/` as the canonical upstream baseline.
   - Retain `SHAKER-B-1w.CSL` (or apply the `'Z'` -> `'W'` translation in the runner under `--layout fr`).

The later E/P RTL repair is a separate change and needs its own C-1 history
retest. Workbook/script disagreements below require binary or execution
confirmation before reporting them to the authors as definite defects.

---

## 1. Cryptographic File Inventory (SHA-256)

### New Package (`local/CSLSSM/`)

| File | Size (Bytes) | SHA-256 Hash |
|---|---|---|
| `CSLSSM.txt` | 442 | `350a39885aae83d2c2ac55017445619c4c9e4729123c35c1ef397c9be3a5e7f2` |
| `SHAKER-A-0.CSL` | 5,047 | `4b16071410049226be42786a100428afc38b76baf18f0c50325119f26b576e7e` |
| `SHAKER-A-1.CSL` | 5,173 | `f18cd098158874d41aae82eff36f1ef0947051737f5385c30ec575c694383333` |
| `SHAKER-A-2.CSL` | 4,281 | `e48730ed059cbdaf7ed2e1d95513efc6587f7be3b39761b345ce535d531c8b38` |
| `SHAKER-A-3.CSL` | 4,958 | `82cc4d2a015f52dc80b045b270aa7bbc54abbe3281d8dddb954cd6d7d9211520` |
| `SHAKER-A-4.CSL` | 4,971 | `46f2504168ae8678aa67f479306cb35aa738b5a3f04d317f7efa5f12b6186770` |
| `SHAKER-B-0.CSL` | 6,820 | `18334a0c3ee675dcee7430b993cb3bce4e6dcffd1c9e28a78a984a465174fe50` |
| `SHAKER-B-1.CSL` | 4,968 | `2dc922d244cf9c58ebc4963444782362d3af8a277de8d8a2797c78a2f69bd421` |
| `SHAKER-B-2.CSL` | 5,492 | `13162a0d83543758f35868207c223f2afbc38fdb9985cb28b4ca04a03648350d` |
| `SHAKER-B-3.CSL` | 4,185 | `6563f8ea55d7220ec17a8434f0587ddf8e30f564f5d16be9295e21b3a0f3cf03` |
| `SHAKER-B-4.CSL` | 4,195 | `6859ea14d8d894bd0b03d8e232ff38493d8a804b8f4b84d1dbf3b4749a9e0e90` |
| `SHAKER-C-0.CSL` | 1,577 | `20f23ea1400aa329882ac6985a3913976456dac8b48cb44a21585d1f8881f9e3` |
| `SHAKER-C-1.CSL` | 3,549 | `cb727ed4cdcfb50cdca1d44102e25bb56c8c307557e47a09f3b62755ce99f8bb` |
| `SHAKER-C-2.CSL` | 2,671 | `41572054589a90280afc9ad673069e250c9f1a4aacdaf9c679157b3e56234660` |
| `SHAKER-C-3.CSL` | 1,602 | `c4624ff7720dd20b8698e9c4a07ca5a7cb2720675f1536c65e6532a222e7ef32` |
| `SHAKER-C-4.CSL` | 1,618 | `0852e4463a43b48ed8c203c5b0f6b02624cbc96df923d366f9a6cf437d9a24e6` |
| `SHAKER-D-0.CSL` | 3,913 | `efb44c432dd3227a75d9503d03429e9e40723cbc139143c9ad63d3a75cbf2aa5` |
| `SHAKER-D-1.CSL` | 4,571 | `4a572c6d2d5d07213d116b71b6a33c77afcac0dd2d1f8fcf17de546357f9a768` |
| `SHAKER-D-2.CSL` | 3,909 | `4e0fc3616faa42269fda50f102267606c4e7fa5f9e6a08c62868e7a83c6be3f9` |
| `SHAKER-D-3.CSL` | 5,437 | `fabb38d3cf3dfbbfec357230fc9e76e02c0507f946a2ee5c8319bbd0ab77378f` |
| `SHAKER-D-4.CSL` | 5,453 | `2c31fb49c0256770a221124193412e17e84e525a3cb1720ea1e0c88eff434ab0` |
| `SHAKER-E-0.CSL` | 877 | `3c66845091b1d74d9a8fc3081dd0c5155cb1168f548e644b2ad2ff91b0cef2cf` |
| `SHAKER-E-1.CSL` | 676 | `adb2074b4a996f587c59b7485e0e88793686eb7b78119f226b069d9eaaeafb32` |
| `SHAKER-E-2.CSL` | 645 | `b7f82739cba9708411022efc0efe449540fea45ef94628b5a90849ddc39d1481` |
| `SHAKER-E-3.CSL` | 564 | `96f09feca980fe10a5b74e27a01fe896ffdcf1c4e8166c39a7719a0bfa98292a` |
| `SHAKER-E-4.CSL` | 580 | `3bcd6a8ffafa1328bd55d7f6174ead5051ea0796404ec54a3c9c8ead533195e1` |
| `shaker27.dsk` | 194,816 | `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61` |

### Prior Upstream Package (`local/test_media/shaker/CSL_27/` and `shaker27.dsk`)

| File | Size (Bytes) | SHA-256 Hash | Match in New Package |
|---|---|---|---|
| `shaker27.dsk` | 194,816 | `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b` | Changed (`SHAKE27E.BIN` updated) |
| `MODULE A/SHAKER-A-0.CSL` | 5,047 | `4b16071410049226be42786a100428afc38b76baf18f0c50325119f26b576e7e` | **Identical** |
| `MODULE A/SHAKER-A-1.CSL` | 5,173 | `f18cd098158874d41aae82eff36f1ef0947051737f5385c30ec575c694383333` | **Identical** |
| `MODULE A/SHAKER-A-2.CSL` | 4,281 | `e48730ed059cbdaf7ed2e1d95513efc6587f7be3b39761b345ce535d531c8b38` | **Identical** |
| `MODULE A/SHAKER-A-3.CSL` | 4,958 | `82cc4d2a015f52dc80b045b270aa7bbc54abbe3281d8dddb954cd6d7d9211520` | **Identical** |
| `MODULE A/SHAKER-A-4.CSL` | 4,971 | `46f2504168ae8678aa67f479306cb35aa738b5a3f04d317f7efa5f12b6186770` | **Identical** |
| `MODULE B/SHAKER-B-0.CSL` | 6,820 | `18334a0c3ee675dcee7430b993cb3bce4e6dcffd1c9e28a78a984a465174fe50` | **Identical** |
| `MODULE B/SHAKER-B-1.CSL` | 4,968 | `2dc922d244cf9c58ebc4963444782362d3af8a277de8d8a2797c78a2f69bd421` | **Identical** (still has 'Z') |
| `MODULE B/SHAKER-B-2.CSL` | 5,461 | `35bb452ea16e1e97733466629c7c856756343917019e0ab2a74229a922fb9d8e` | Changed (test P waits) |
| `MODULE B/SHAKER-B-3.CSL` | 4,185 | `6563f8ea55d7220ec17a8434f0587ddf8e30f564f5d16be9295e21b3a0f3cf03` | **Identical** |
| `MODULE B/SHAKER-B-4.CSL` | 4,195 | `6859ea14d8d894bd0b03d8e232ff38493d8a804b8f4b84d1dbf3b4749a9e0e90` | **Identical** |
| `MODULE C/SHAKER-C-0.CSL` | 1,577 | `20f23ea1400aa329882ac6985a3913976456dac8b48cb44a21585d1f8881f9e3` | **Identical** |
| `MODULE C/SHAKER-C-1.CSL` | 3,549 | `4a500e7d4b14fc7bc2acaf52417c187913efd9e3ce02b00e0f4d31ae4af513da` | Changed (`03F1`/`03F2` fix) |
| `MODULE C/SHAKER-C-2.CSL` | 2,751 | `e8972ff1c2ea2038f13061032edefa27cd728aff1b2492458d22db5d7c4b2dcc` | Changed (`02C8`, deadlocks) |
| `MODULE C/SHAKER-C-3.CSL` | 1,684 | `65ac3d220a4c97aa1922b7d122a6e0642a428d20892665c47b381b12d54f83ab` | Changed (removed test R) |
| `MODULE C/SHAKER-C-4.CSL` | 1,700 | `320c012baadbaca35ac6b708de9c77ff9d8f4029563583cf3bb9715a4733c7c3` | Changed (removed test R) |
| `MODULE D/SHAKER-D-0.CSL` | 4,765 | `1174a499d068f7cc8c71efa7e22473b70047602a7902ec077f3cae62314c64e8` | Changed (wait_ssm -> wait) |
| `MODULE D/SHAKER-D-1.CSL` | 5,377 | `1ab3e5c66120c086a1d58e213e0161cf5d4664e97bea29b3fdf60e9628e9b526` | Changed (dsk load & wait) |
| `MODULE D/SHAKER-D-2.CSL` | 5,170 | `95bedf119df0f140270abf1a4be1e87a127798968d56ed5e7430385cd2b97aa4` | Changed (wait_ssm -> wait) |
| `MODULE D/SHAKER-D-3.CSL` | 6,571 | `60f27a474bfdb4abfe276b75dcc31f394ac1ab87bb97b5d6826f3597b7aecfee` | Changed (wait_ssm -> wait) |
| `MODULE D/SHAKER-D-4.CSL` | 6,585 | `1845b41752bc74b301839bd3a281805c140f205f1ebbf9bfab4293fd58be7f31` | Changed (wait_ssm -> wait) |
| `MODULE E/SHAKER-E-0.CSL` | 877 | `3c66845091b1d74d9a8fc3081dd0c5155cb1168f548e644b2ad2ff91b0cef2cf` | **Identical** |
| `MODULE E/SHAKER-E-1.CSL` | 676 | `adb2074b4a996f587c59b7485e0e88793686eb7b78119f226b069d9eaaeafb32` | **Identical** |
| `MODULE E/SHAKER-E-2.CSL` | 645 | `b7f82739cba9708411022efc0efe449540fea45ef94628b5a90849ddc39d1481` | **Identical** |
| `MODULE E/SHAKER-E-3.CSL` | 564 | `96f09feca980fe10a5b74e27a01fe896ffdcf1c4e8166c39a7719a0bfa98292a` | **Identical** |
| `MODULE E/SHAKER-E-4.CSL` | 580 | `3bcd6a8ffafa1328bd55d7f6174ead5051ea0796404ec54a3c9c8ead533195e1` | **Identical** |

### Private Patched CSLs (`local/investigations/shaker-sweep-2026-11-03/csl/`)

| File | Size (Bytes) | SHA-256 Hash | Relationship to New `local/CSLSSM/` |
|---|---|---|---|
| `SHAKER-B-1w.CSL` | 4,968 | `b5e2ba343ab57cdea5c44f3a9b47deb24fd026cd7ae53b233f6e1cf845e01dc2` | **Unmerged patch**: Differs from `CSLSSM/SHAKER-B-1.CSL` by line 41 (`'W'` vs `'Z'`). Still needed for French ROM. |
| `SHAKER-C-1f.CSL` | 3,549 | `cb727ed4cdcfb50cdca1d44102e25bb56c8c307557e47a09f3b62755ce99f8bb` | **Fully adopted**: **Byte-identical** to `CSLSSM/SHAKER-C-1.CSL`. |
| `SHAKER-D-1b.CSL` | 4,126 | `721e0c0a3a7aca52ae29713e1f4a26b419ca50c2f2cdbb58670de349bd7d7c9c` | Private test-cut (skips I, R, T). |
| `SHAKER-D-1probe.CSL`| 830 | `5e17285995583ec73663c4313ac7e293b891cd1ef5f7d3e5c0ebcbde3f31f527` | Private probe on test H. |
| `B1-probe.CSL` | 624 | `cf6fb2e9e72d40f89576a6e3d70c5341ad5ad47471e642167145a2795586d4b0` | Private probe for test 1 interlace. |
| `C-02D3.CSL` | 394 | `ea5be949b7af28619029a757e24af05974aff1e62dde25f9692e5fcf6de835d3` | Private probe for isolated test S (`02D3`). |

---

## 2. Disk Image & Sector/File Changes (`shaker27.dsk`)

Both disks are 194,816 bytes, formatted in Extended CPC DSK format (creator header: `WinAPE 2.0B03`, 40 tracks, 1 side, 9 sectors per track, 512 bytes per sector).

### AMSDOS Directory & File Payload Extraction

Using exact AMSDOS DPB geometry (blocks 0 and 1 allocated to Track 0 directory sectors &C1–&C4; 1 KB block allocation across logical sectors):

| File | Disk Image | Total Extracted Size | Payload Size | Total File SHA-256 | Payload SHA-256 |
|---|---|---|---|---|---|
| `SHAKE27A.BIN` | Old (`shaker27.dsk`) | 27,136 | 27,004 | `4da5513611cc47df...` | `ab8a7cb64bbf7149dd71a3958216fa84274636bb7b13c1e9f6a15527706c8a8d` |
| `SHAKE27A.BIN` | New (`CSLSSM/`) | 27,136 | 27,004 | `4da5513611cc47df...` | `ab8a7cb64bbf7149dd71a3958216fa84274636bb7b13c1e9f6a15527706c8a8d` |
| `SHAKE27B.BIN` | Old (`shaker27.dsk`) | 26,880 | 26,683 | `d9b9a2a33e8692f5...` | `c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88` |
| `SHAKE27B.BIN` | New (`CSLSSM/`) | 26,880 | 26,683 | `d9b9a2a33e8692f5...` | `c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88` |
| `SHAKE27C.BIN` | Old (`shaker27.dsk`) | 27,904 | 27,680 | `d98a1bc0999d0833...` | `3647e120e282412d5afc31bed0e4b6673beed850ba94ec7a4d5374ce04a4682f` |
| `SHAKE27C.BIN` | New (`CSLSSM/`) | 27,904 | 27,680 | `d98a1bc0999d0833...` | `3647e120e282412d5afc31bed0e4b6673beed850ba94ec7a4d5374ce04a4682f` |
| `SHAKE27D.BIN` | Old (`shaker27.dsk`) | 27,264 | 27,060 | `8deddfc20bd9424a...` | `65a90825dc674953b5dd9d4918bddf8079ea44877d89b08542b7fabb6b8d5ec5` |
| `SHAKE27D.BIN` | New (`CSLSSM/`) | 27,264 | 27,060 | `8deddfc20bd9424a...` | `65a90825dc674953b5dd9d4918bddf8079ea44877d89b08542b7fabb6b8d5ec5` |
| `SHAKE27E.BAK` | New (`CSLSSM/`) | 20,352 | 20,191 | `b882d35e085e1eb2...` | `87f13d4b272e8c3b31e721c6bb00c929476a65b4d284eb6d0190468e4fef1372` |
| `SHAKE27E.BIN` | Old (`shaker27.dsk`) | 19,072 | 18,937 | `4be91edb18fc23a7...` | `665f57dda6c59fa46a4c985a9da723b1665b87c8263571d238d8ad023c1e6c13` |
| `SHAKE27E.BIN` | New (`CSLSSM/`) | 20,480 | 20,240 | `facdd2593d411f0a...` | `1a74786431ffedee46e52df1a16f069f7a3ae36f37260c0dfeea645ffb34bca1` |

### Critical File Extraction Findings

1. **`SHAKE27A.BIN`, `SHAKE27B.BIN`, `SHAKE27C.BIN`, `SHAKE27D.BIN` are 100% UNCHANGED.**
   Their block allocation lists on the disk, 128-byte AMSDOS headers (load address `0x3D00` / `0x3900` / `0x3A00`, entry `0x4042`, file lengths, and checksums), and entire binary code payloads are identical bit-for-bit.
2. **`SHAKE27E.BIN` is the ONLY modified program file.**
   - In the old disk, `SHAKE27E.BIN` occupied blocks 2–9 and 18–28 (149 records = 18,937 bytes declared payload).
   - In the new disk, blocks 2–9 and 18–28 were deallocated.
   - A backup file `SHAKE27E.BAK` was written to blocks 29–36, 129, 139–148, 169 (159 records = 20,191 bytes declared payload).
   - The new `SHAKE27E.BIN` was written to blocks 149–167 and 170 (160 records = 20,240 bytes declared payload).
   - Analysis of `SHAKE27E.BIN`: The internal test table starting at CPC address `0x3A3C` remapped test mask flags for markers `0x0406`..`0x040C` from flag `0x03` to `0x01`. New test strings and Z80 routines were compiled into the binary, including `WAIT STATE ON IORQ AFTER Z80A INST`, `VSYNC FOR 'DEC BC (6 TS)+ INT'`, `VSYNC FOR 'LD A,(HL) (7 TS)+ INT'`, `ON C4=R4=38/C9=R9=7 R4=R9=0 AFTER HSYNC`, and `LAST LINE STATE FOR VMA' LOAD`.
3. **Physical Sector Diffs:**
   Across the 194,816-byte disk images, exactly 58,202 bytes differ across 1,057 contiguous ranges. These are strictly confined to the directory sector changes on Track 0 (freeing blocks 2–9, 18–28; adding `SHAKE27E.BAK`; updating `SHAKE27E.BIN`) and the corresponding sector data on Tracks 1, 2, 4–8, 28, and 30–37 where those blocks reside. No other files were touched.

---

## 3. Detailed CSL Script Semantic Diffs

A full unified diff of all 25 scripts between `CSL_27/` and `local/CSLSSM/` establishes:

### Module A (`SHAKER-A-[0-4].CSL`)
- **100% Byte-Identical across all 5 files.**

### Module B (`SHAKER-B-[0-4].CSL`)
- `SHAKER-B-0.CSL`, `SHAKER-B-3.CSL`, `SHAKER-B-4.CSL`: **100% Byte-Identical.**
- `SHAKER-B-1.CSL`: **100% Byte-Identical to prior upstream.**
  Line 41 remains:
  ```csl
  key_output 'Z'	; Passe sur R9 BASE=6
  ```
  **The runner-specific French-layout override is still needed.**
- `SHAKER-B-2.CSL`:
  In test 'P' ("FORCED STAB CRTC 0 R0=0", lines 290–306), the author replaced 8 `wait_ssm` statements (`0x01E8` through `0x01EF`) with 5 fixed `wait` statements:
  ```diff
  -; FORCED STAB CRTC 0 R0=0
  -wait_ssm 0x01E8
  -wait_ssm 0x01E9
  ...
  -wait_ssm 0x01EE
  -wait_ssm 0x01EF
  +wait 100000000	; 12828735+638976
  +key_output ' '
  +wait 100000000	; 5440000+638976
  ...
  +wait 1000000
  ```
  *Rationale:* In `SHAKER_SCREENSHOT_CODE.xlsx`, CRTC 2 is marked `x` (not emitted) for `01E9`, `01EB`, `01ED`, and `01EF`. In the old script, waiting for these non-existent markers caused a guaranteed hang on CRTC 2.

### Module C (`SHAKER-C-[0-4].CSL`)
- `SHAKER-C-0.CSL`: **100% Byte-Identical.**
- `SHAKER-C-1.CSL`: **Typo fixed by author.**
  ```diff
  -wait_ssm 0x02F1	; crt 1; parity01a
  +wait_ssm 0x03F1	; crt 1; parity01a
   key_output ' '
  -wait_ssm 0x02F2	; crt 1; parity01b
  +wait_ssm 0x03F2	; crt 1; parity01b
  ```
  This is byte-identical to `SHAKER-C-1f.CSL`.
- `SHAKER-C-2.CSL`:
  1. Line 82: `wait_ssm 0x02C7` repeated typo fixed to `wait_ssm 0x02C8`.
  2. Lines 126–127: `wait_ssm 0x02D8` and `wait_ssm 0x02D9` commented out with `;`. In `SHAKER_SCREENSHOT_CODE.xlsx`, CRTC 2 is marked `x` for both (02D8 is not emitted; 02D9 has note *"A voir (pas de photos reelles ??)"*).
  3. Lines 140–144: Test 'R' ("R9/R4 UPD LAST LIMIT", `wait_ssm 0x03AD`) deleted. `03AD` is CRTC 0-only.
- `SHAKER-C-3.CSL` and `SHAKER-C-4.CSL`:
  Lines 81–85: Test 'R' (`wait_ssm 0x03AD`) deleted (was mistakenly included in CRTC 3 and 4 scripts).

### Module D (`SHAKER-D-[0-4].CSL`)
- **Global conversion from `wait_ssm` to fixed `wait <microseconds>`:**
  Across all five scripts (D-0 through D-4), every single `wait_ssm` command was stripped and replaced with fixed `wait` statements (e.g. `wait 2000000`, `wait 10000000`, `wait 45000000`).
  The accompanying `CSLSSM.txt` describes headless CPCSE support and automatic SSM captures. That is compatibility context; it does not explicitly state why these particular D-script waits were changed.
- `SHAKER-D-1.CSL` specifically:
  Fixed lines 3–10 from loading `shaker26.dsk` / `RUN"SHAKE26D"` back to `shaker27.dsk` / `RUN"SHAKE27D"`.

### Module E (`SHAKER-E-[0-4].CSL`)
- **100% Byte-Identical across all 5 files.**

---

## 4. Assessment of Specific Audit Items

### B1 French Physical Z vs W
- **Status:** **Retain the French-layout override for this runner.**
- **Details:** `local/CSLSSM/SHAKER-B-1.CSL` line 41 retains `key_output 'Z'`. When running `csl_runner.py` with `--layout fr`, the translation table converts `'Z'` into the matrix position of French Z (which is UK W, row 3 bit 1). But SHAKER's menu code directly polls row 9 bit 7 (physical UK Z). Thus the menu selection fails and the run stalls at `wait_ssm 0x00E5`.
- **Action:** Retain `SHAKER-B-1w.CSL` (or maintain a layout-aware translation exception in `csl_runner.py`).

### C1 Marker Typo 02F1/03F1
- **Status:** **FIXED by authors.**
- **Details:** Lines 180 and 182 of `SHAKER-C-1.CSL` now wait for `0x03F1` and `0x03F2`. In `SHAKER_SCREENSHOT_CODE.xlsx`, `02F1`/`02F2` belonged to DT D1/D2 (Module D). The actual routine in `SHAKE27C.BIN` emitted `03F1`..`03F6` for CO subset I (Interlace VSYNC Nightmare).
- **Action:** Fully resolved upstream. Our private patch `SHAKER-C-1f.CSL` is retired in favor of the canonical script.

### Cross-CRTC Marker Audit & Spreadsheet Discrepancies
Cross-referencing all `wait_ssm` commands in all scripts against `SHAKER_SCREENSHOT_CODE.xlsx` revealed:

1. **Unlisted Codes (03E5–040C):**
   The reference workbook `SHAKER_SCREENSHOT_CODE.xlsx` abruptly ends at `0x03E4`. SHAKER 2.7 added 40 new test markers that are executed and expected by scripts but absent from the spreadsheet:
   - Module B: `03E6`, `03E8` (CRTC 1, 3, 4); `03EA`, `03EC`, `03EE`, `03F0` (CRTC 0, 2).
   - Module C: `03F1`–`03F6` (CRTC 1 CO subset I).
   - Module D: `03F7`, `03F8`, `03F9`.
   - Module E: `03FA`–`03FD` (all CRTCs); `0400`–`0402` (CRTC 1); `0403`–`040A` (CRTC 0); `040B`–`040C` (CRTC 2); `0406` (CRTC 3/4).
2. **Workbook/script discrepancies requiring confirmation:**
   - `SHAKER-A-2.CSL` (CRTC 2): Still contains `wait_ssm 0x000E`, `0x0018`, `0x001A`, and `0x0031`–`0x0036`, all flagged `x` for CRTC 2 in the workbook.
   - `SHAKER-A-3.CSL` / `SHAKER-A-4.CSL`: Still contain `wait_ssm 0x0007`, `0x0008` (SKEWDISP) and `0x00B4`, flagged `x` for CRTC 3/4.
   - `SHAKER-B-0.CSL`: Line 278 waits for `0x01D7`, flagged `x` for CRTC 0 (*"CRTC 1 : RFD R5 OTHER TESTS"*).
   - `SHAKER-C-0.CSL` & `SHAKER-C-1.CSL`: Contain `wait_ssm 0x02BF` (*"CRTC 2 : LAST LINE CONDITION"*), flagged `x` in the sheet, though annotated in C-1 comments as displayed across CRTCs in 2.7.

---

## 5. Impact on Existing Screenshots & Photo Comparisons

The existing hardware walk on 2026-11-03 (`docs/investigations/hardware-runs/shaker-crtc1-sweep-2026-11-03.md`) recorded 346 valid captures on `5fba168` RTL:

- **Module A-1 (127 captures):**
  - Script: `SHAKER-A-1.CSL` is byte-identical to `CSLSSM/SHAKER-A-1.CSL`.
  - Binary: `SHAKE27A.BIN` is byte-identical.
  - **Impact:** 100% identical. The 127 captures remain evidence for the recorded build/history.
- **Module B-1 (139 captures):**
  - Script: Walk ran with `SHAKER-B-1w.CSL`. The new upstream `CSLSSM/SHAKER-B-1.CSL` still has `'Z'`, so the `'W'` patch remains identical.
  - Binary: `SHAKE27B.BIN` is byte-identical.
  - **Impact:** 100% identical. The 139 captures remain evidence for the recorded build/history.
- **Module C-1 (80 captures):**
  - Script: Walk ran with `SHAKER-C-1f.CSL`. The new upstream `CSLSSM/SHAKER-C-1.CSL` is byte-identical to `SHAKER-C-1f.CSL`.
  - Binary: `SHAKE27C.BIN` is byte-identical.
  - **Impact:** 100% identical. The 80 captures remain evidence for the recorded build/history.
- **Module D-1:**
  - Old sweep stalled at `0305` under `wait_ssm`.
  - The new script replaces `wait_ssm` with fixed delays.
  - **Impact:** No previous full walk existed to invalidate.
- **Module E-1 (7 captures):**
  - Script: Byte-identical.
  - Binary: `SHAKE27E.BIN` was recompiled with new tests.
  - **Impact:** The 7 captures cover tests 1 and 2 (`03FA`..`03FD`, `0400`..`0402`), which remain present in the binary. Re-verifying E-1 is trivial (7 captures), but marker presence alone does not establish that the new test bodies or expected values are unchanged.

---

## 6. RTL Rerun Decision (`5fba168`)

**Decision: NO full fresh run is needed on `5fba168`.**

- Rationale:
  1. The audited base `6103619` has the same RTL as `5fba168`, the source compiled in `Amstrad_20261103_5fba168.rbf` used during the November 3 sweep.
  2. The software binaries for Modules A, B, and C are byte-identical between old and new packages.
  3. The script execution paths for A-1, B-1, and C-1 executed on November 3 are byte-identical to the new package (using the retained layout patch for B-1).
  4. Rerunning A-1, B-1, and C-1 would execute the exact same bits on the exact same core, so the package alone does not require invalidating or repeating that baseline.
  5. For the package update on this RBF, only focused targets are warranted: D-1 (to observe whether the fixed-wait script paces through Module D) and E-1 (to capture the 7 pages of the updated `SHAKE27E.BIN`).

---

## 7. Actionable Feedback for Authors (Logon System / Longshot / Brocky)

1. **French Firmware / Physical Keyboard Mismatch (`SHAKER-B-1.CSL` line 41):**
   - *Issue:* Line 41 sends `key_output 'Z'` to select menu item B ("Passe sur R9 BASE=6"). In our MiSTer CSL runner under `--layout fr`, typing character `'Z'` actuates matrix line 3 bit 1 (physical W), which does not activate the menu, causing automated runs under French layout translation to stall at `0x00E5`.
   - *Action:* Document that CSL runner scripts must assume physical UK matrix key positions, or provide an explicit note that French ROM environments require sending `'W'` to select menu item `'Z'`.
2. **Missing Codes in Reference Workbook (`SHAKER_SCREENSHOT_CODE.xlsx`):**
   - *Issue:* The spreadsheet truncates at `0x03E4`. SHAKER 2.7 introduces markers `0x03E5` through `0x040C` across modules B, C, D, and E.
   - *Action:* Release an updated `SHAKER_SCREENSHOT_CODE.xlsx` incorporating definitions for `0x03E5` through `0x040C`.
3. **Artifact on Distribution Disk:**
   - *Issue:* `shaker27.dsk` contains a stray backup file `SHAKE27E.BAK` (20,352 bytes, blocks 29–36, 129, 139–148, 169).
   - *Action:* Purge `SHAKE27E.BAK` from official distribution disk releases.
4. **Residual workbook/script discrepancies:**
   - *Issue:* Several scripts still contain `wait_ssm` for markers marked `x` in the author's own spreadsheet (notably `SHAKER-A-2.CSL` lines 43, 58, 61, 96–103; `SHAKER-B-0.CSL` line 278).
   - *Action:* Confirm these entries against the current binary and update the workbook or script as appropriate. A stale workbook marking is not proof that a current script wait is unsupported.

---

### Runner feedback after device execution

The canonical D-1 now loads the 27 disk and completes 127 captures on
`5fba168`. Its D3 bulk runs of 50 spaces outrun host-native PNG retrieval;
marker filenames then contain later states or the menu. A separately paced
MiSTer overlay is appropriate; this is not proof that the supplied script
is defective in CPCSE or AmSpirit. The reused old event-driven D3 section exposed an extra entry-space
keypress on module27: it advances every requested R2 value by one and
leaves the last capture at the menu. A corrected private overlay removes
that key before validation; monitor-response differences remain separate. See the device report before
using any D3 capture as a per-page verdict.

Suggested author-facing clarification (not sent): « Le nouveau C-1 reprend
bien les marqueurs 03F1/03F2 et remplace notre correction privée. Pour B-1,
notre traducteur clavier MiSTer en mode FR envoie W afin d'actionner la touche
physique Z attendue par le menu ; faut-il définir `key_output` en caractères
logiques ou en positions de touches ? Le D-1 charge maintenant SHAKE27D. Ses
séquences D3 de 50 espaces dépassent notre récupération de captures native,
donc nous gardons un déroulement cadencé propre à MiSTer. »

## 8. Evidence Limitations

- **Static vs. Dynamic Analysis:** All findings in this audit are established by static inspection: SHA-256 hashing, AMSDOS catalog parsing, sector-by-sector disk differential analysis, binary string/offset inspection, and CSL script parsing against `SHAKER_SCREENSHOT_CODE.xlsx`.
- **No Dynamic Device Claims:** No live MiSTer hardware captures, emulator runs, or FPGA bitstreams were executed during this audit. Statements regarding script execution flow (such as the D-1 fixed-delay pacing) describe script syntax and declared timing, not verified hardware loop completions.

## Installed position and rerun scope

The parent installed this package in the main checkout and task checkout under
`local/test_media/shaker/CSL_27/` plus `shaker27.dsk`. The supplied
`local/CSLSSM/` is unchanged. In the main checkout, the former canonical
`CSL_27/` and disk are preserved under
`local/test_media/shaker/pre-CSLSSM-20261103/`; use that directory to
reproduce the old-input side of this audit after installation.

The French-only override is
`local/test_media/shaker/CSL_27_FR/MODULE B/SHAKER-B-1.CSL`, byte-identical
to the prior W patch. Use it with `--layout fr --no-follow-loads`. The
author's canonical B-1 stays untouched. Canonical C-1 now replaces the
private marker patch. No media was copied to MiSTer during the device hold.

The rerun decision above applies to the package update on unchanged
`5fba168` RTL. The newly developed E/P display repair is a separate behavior
change: after synthesis, rerun the full C-1 history, including E/P and 02D3,
plus relevant RFD/R6 controls. That does not require discarding all historical
A/B captures. Module E needs fresh capture/expectation review on the changed
binary. The subsequent D-1/E-1 trials and D3 capture limitations are recorded
in the [device results](shaker-package-device-2026-11-03.md); execution
completion is distinct from photo correctness.

The host runner's `_sleep()` continues polling/draining SSM during fixed waits,
so the new D scripts can still request captures through marker observation.
They lose per-page completion waits; native screenshots remain asynchronous
and a completion timing claim requires a live run. Workbook `x` entries above
are audit leads, not confirmed script defects: the workbook predates 2.7 codes.
