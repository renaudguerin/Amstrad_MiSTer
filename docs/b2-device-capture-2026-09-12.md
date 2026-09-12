# B2 real-device capture, 2026-09-12

B2 now boots the existing SHAKER 2.7 DSK through Main/MGL, types the French-ROM
launch command, selects module B (9), and retrieves fully decoded native PNGs.
This is a transport/navigation and repeatability result. It is not a verdict
that the CRTC measurements match hardware references, nor an exact VSYNC/pin
capture. Main reads its asynchronous scaler buffer.

## Identity and configuration

Device: `root@mister`, Linux `5.15.1-MiSTer`, ARMv7. Initial core was MENU.
The actual core directory is `/media/fat/_Computer/`.

| File | SHA-256 |
|---|---|
| `/media/fat/_Computer/Amstrad_20260911_5c16b17.rbf` | `8b3b5bed518165040f8e578c83f061891fa64d58ce3b52fb07d5765765506468` |
| `/media/fat/games/Amstrad/dsk/shaker27.dsk` | `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b` |
| `/media/fat/MiSTer` on disk | `ef0dbfde3744ccb160444932d8a7d31f298b16a95db506852c99ece018321596` |
| Temporary `/tmp/mbc-b2-20260912` | `0e99082bb8c9b8b2c2b6581c736dfc5d701a977a9c76fa548565cbc6642984db` |

The RBF matches the delivered local build. Main's FIFO, named PNG capture,
MGL DSK S0 mapping and MBC uinput worked on this device. MBC is the retained
pinned ARM binary; no compiler, daemon, firmware or system upgrade was installed.

**Visually observed:** Amstrad 128K / BASIC 1.1 boot; a readable disk catalogue;
SHAKER 2.7 module B menu; CRTC 1 heading and B (9) numeric interlace table.
French-ROM keyboard mapping was verified through visible typed text and launch.

**Saved configuration, not visual proof of active mode:** the pre-run
`Amstrad.CFG` is 16 bytes, hex `00004000000000000000000000000000`.
`Amstrad.sv` CONF_STR maps this to classic CPC 6128, CRTC 1, Full sync filter,
Color(GA), no scandoubler effects, Original aspect, Normal scale, 16 MHz pixel
clock and original CPU/FDC timings. Bit 22 enables Right Shift as Shift.
Main's pinned `user_io.cpp` loads/saves its status byte array directly; this
explains the interpretation, but does not independently read the live FPGA latch.
The native PNG was byte-identical with the OSD open and closed: **it omits OSD**.
Full/Raw selection must therefore not be labelled visually confirmed by these PNGs.

The saved ROM selections are `games/Amstrad/boot_fr.rom` (SHA-256
`1ba46e0f28388dd0d45b759f92468ffdccae8d40b973af736d7cc7ed0739eb4a`) and
`games/Amstrad/6128FR.ez0` (SHA-256
`36bed89152acff6397719132825b9b669c336cd33b1ce58cd23c51de3ea06611`).
A post-run hash of `/proc/2351/exe` also matches the recorded Main disk hash. `MiSTer.ini` and all existing `config/Amstrad*` files
were archived before loading. The INI records `vsync_adjust=1`, `vscale_mode=1`,
`direct_video=0`, `vga_scaler=0`, `forced_scandoubler=0`; alternate/active settings
and physical output are separate observations.

## Reusable launch and evidence

Use [shaker27-b9-fr.json](../scripts/hardware-loop/shaker27-b9-fr.json) with
[driver.py](../scripts/hardware-loop/driver.py). It pins the RBF and disk hashes,
waits for BASIC, types `RUN"SHAKE27B`, waits for disk load, presses physical 9,
and waits for the numeric table. See the [driver guide](mister-hardware-loop-driver.md)
for the exact Linux key sequence and temporary MBC override.

All original images, effective cases, command logs and manifests are retained
under ignored `docs/references/b2-device-20260912/` in the B2 worktree. Each
accepted load has three separate screenshot requests, complete decode before the
next request, and a fresh MGL/core load. The first exploratory load is additional
evidence; two interrupted attempts are excluded. Both identified host processes
were stopped and no MBC process remained before the clean sequence began.

The clean sequence completed with nine byte-identical 768 x 273 PNGs across
three independent loads. Shared PNG SHA-256:
`68cf044c423d607f74ec9c642de644f1d3cbf1f33388f24e9e50103ac22ef468`.

| Load | Manifest | Representative PNG |
|---|---|---|
| 1 | [manifest](references/b2-device-20260912/accepted-b9-crtc1-full-1/manifest.json) | [B9 CRTC1](references/b2-device-20260912/accepted-b9-crtc1-full-1/capture_shaker27_b9_crtc1_full_1789200672_2e98b6_c1.png) |
| 2 | [manifest](references/b2-device-20260912/accepted-b9-crtc1-full-2/manifest.json) | [B9 CRTC1](references/b2-device-20260912/accepted-b9-crtc1-full-2/capture_shaker27_b9_crtc1_full_1789200719_4e61f2_c1.png) |
| 3 | [manifest](references/b2-device-20260912/accepted-b9-crtc1-full-3/manifest.json) | [B9 CRTC1](references/b2-device-20260912/accepted-b9-crtc1-full-3/capture_shaker27_b9_crtc1_full_1789200767_cffb37_c1.png) |

The representative numeric screen reports `#2700/#2700/#2740/#2780/#2780`,
then `#43C0` for the first block, and `#1820/#1840/#1860/#1880/#1880`, then
`#25C0` for the `R7=#18 BEFORE R6` block. These are transcribed screen results,
not source-derived expected values or a D1/D6 pass declaration.

## Bounded follow-ons and restored state

Raw pixels was selected by changing only status bit 35 in the backed-up
16-byte CFG, before a fresh load. The resulting file was hashed back from the
device; the exact bytes were `00004000080000000000000000000000`. All three
[Raw pixels captures](references/b2-device-20260912/b9-crtc1-raw-pixels/manifest.json)
match the nine Full PNGs byte-for-byte. This is not evidence of a broken mode
latch: this numeric scene need not exercise B6’s shifted-pixel difference.
The applied mode remains unobserved; the pair is identified by saved selection.

The original CFG was restored and verified before loading module C and selecting
physical 4. A subsequent [module C menu capture](references/b2-device-20260912/c-menu-validation/capture_c-menu-validation_1789201019_8f10b1_c1.png)
visibly confirms option 4 is `CRTC 1 : IVM ON/OFF`. The [C4 captures](references/b2-device-20260912/c4-crtc1-full/manifest.json)
are three identical 768 x 546 images, SHA-256
`c31616fb22b9f9e6efd6ed0229222465d126f959c7c1c6043c9bd98cdd608056`.
They show a disrupted raster with small text interleaved with coloured stripes,
not a clean numeric table. This is captured output, not a D6 pass or a new RTL
cause. The C (4) follow-on must not replace the clean B (9) acceptance evidence.

A byte comparison of every pre-existing `config/Amstrad*` file after these
runs found no remaining differences. The original CFG SHA-256 is
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
No MBC process or task MGL remained. Original media/build files were not modified.
Task PNGs remain under `/media/fat/screenshots/`; originals and the pre-run tar
also remain in the ignored evidence directory.

Final device state: the known B (9) CRTC 1 numeric screen after a final fresh
load, again matching the baseline PNG hash. The temporary MBC executable was
removed after confirming no MBC process remained. A second complete config
comparison after that final load still found all original bytes unchanged.
See [final manifest](references/b2-device-20260912/final-b9-restored/manifest.json)
and [restoration check](references/b2-device-20260912/restoration-check.json).
Physical display output was not independently recorded; no answer to the
optional output question was received during this run.

## Driver delta and gates

Optional `expected_sha256` pins for RBF/media now stop a run before MGL upload,
load or input if a file at the selected path has been replaced. The actual hash
remains in the failure manifest. This is a pre-load file check, not an atomic
lock on device storage. Existing unpinned discovery cases remain supported.

The focused suite passes 26 tests, including a replaced-file rejection before
upload/load and a matching-hash complete capture. The two new tests were first
run against the unimplemented option and failed; the existing partial-image,
stale-name, command-quoting, timeout and interrupted-input checks remain green.
The pinned case dry-run is offline and `git diff --check` passes. No RTL,
simulation model or build manifest changed, so no full simulation or synthesis
was run for this host-only change.

Fresh Gemini 3.8 Flash high review returned **CLEAR**, exit 0 and complete
handoff in `20260912T081121Z-83276-1c3c`. It inspected the pinning delta,
French case, quoting, timeouts, capture freshness/partial files and key-release
boundaries, and independently ran all 26 tests plus dry-run. The parent
inspected the diff and retained the [review log](references/b2-device-20260912/review/output.log).
The reviewer made no device or hardware-correctness claim.

Gemini’s completed review covers the final implementation at `3b64673`;
subsequent changes only finalize this evidence record. An attempted Opus 5 high
call, `20260912T081701Z-89949-fa5d`, exited with expired OAuth before reviewing;
its [log](references/b2-device-20260912/opus-review-attempt/output.log) is retained
and is not a clearance. After authentication was refreshed, the user explicitly
asked not to duplicate completed reviews. No uncovered code delta remained, so
no duplicate Opus pass was started. The actual accepted reviewer is Gemini.

## Limits

- These PNGs are scaler-buffer captures, not physical HDMI/CRT photographs or
  exact instruction/SSM events. Static equality does not establish all interlace
  fields or detect every transient.
- Active OSD filter verification remains distinct from the saved CFG and the
  visibly identified CRTC/test. No visual filter-mode claim is inferred.
- D1/D6 acceptance still requires comparison with the appropriate hardware
  reference. Plus BASIC/title/sprite results in the [user report](hardware-evidence-2026-09-12.md)
  are not broadened by this Classic capture run.
