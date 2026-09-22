# Device acceptance of `cdcb3c3` (B20-1, B16, B18)

Unattended device session, 2026-09-22. RBF `Amstrad_20260922_cdcb3c3.rbf`, SHA-256
`f8bc3158826214d81ffeab09a311e78103f451add356247a70587592da6d63a1`, copied to
`/media/fat/_Computer/` and hash-checked on the device. Sonic CPR SHA-256
`4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae` (same file as the
[rearm record](../sonic/rearm-boundary-2026-09-22.md)). Evidence (gitignored, main checkout):
`docs/screenshots/device-acceptance-cdcb3c3-2026-09-22/`.

The only RTL change between the previous hardware-tested build `95e6f56` and `cdcb3c3` is
B20-1 (`5d12f56`, live PPR writes). Classic SHAKER was therefore not rerun.

Native captures omit the OSD and analogue output. They are reproduction evidence, not a
hardware-accuracy verdict.

## B20-1: Sonic no-input progression control

CFG: original with Plus model set to 6128+ (`[34:33]=2`), Sync Full; device hash
`13ef32c7…5747` before loading. Driver case `sonic-6128p-noinput.json`: 18 s boot, then three
captures requested 8 s apart.

| Capture | SHA-256 prefix | Screen |
| --- | --- | --- |
| c1 | `a270c8e1` | Corrupt title (same family as `c59e03a`) |
| c2 | `f9e6a65c` | Green Hill playfield, ring HUD `00`, lives x3 |
| c3 | `56b8dfc1` | Playfield, ring HUD `12`, lives x3 |

Result: `cdcb3c3` keeps the baseline `c59e03a` progression (title, then playfield
without input) and does not reproduce the `a137d48` title stall. The title corruption
remains. B20-1 makes no Sonic claim; this rules out a progression regression only.

## B16: CPR with Plus Off cannot be loaded, so B16's CPR path is unreachable

With the original CFG (Plus model Off, `2e585b4c…d8e4`) the driver's MGL load of the
Sonic CPR produced a classic `Amstrad 128K Microcomputer (f3) / BASIC 1.1 / Ready`
screen, identical across three captures (`253ce13d…`). B16 predicts a 6128+ boot.

Discriminator: two hand-written MGLs on the same RBF and CFG.

- `b16-cpr-then-sna.mgl`: CPR (index 8), then a Plus SNA (index 6, v3 header model 4,
  `sonic_loop_original.sna`, SHA-256 `13cc83ea…0403`). Result: the same BASIC screen, hash
  `253ce13d…`. **Neither file was applied**, including the SNA.
- `b16-sna-only.mgl`: the same SNA alone. Result: the screen changes (`35c496f5…`, vertical
  bars; expected garbage without a cartridge). MGL SNA loading works.

Cause, from `CONF_STR` and Main source: the CPR entry was `d2F8,CPR,...` and
`status_menumask` bit 2 is `plus_mode` (`rtl/plus/plus_menu_capability_mask.v`). Main's
`menu.cpp` (MiSTer-devel/Main_MiSTer, fetched 2026-09-22) renders a `d`-flagged item
disabled when its mask bit is clear; the user cannot select it, and an MGL naming it does
not complete, so later MGL items are dropped. With Plus Off the core never receives the
CPR, so B16's `select_cpr` never fires. The B16 simulation fixture drives the completion
event directly and could not see this.

This is a policy conflict: B6 gated CPR to Plus models, while B16 requires CPR to be
loadable from Off. B16 is the later, explicit requirement. Fix on branch
`general/device-acceptance-cdcb3c3`: the entry becomes `F8,CPR,Load Plus cartridge;`,
and the `sdram_cartridge_tests` source pin now requires the ungated entry. Hardware
acceptance of the fix needs an RBF built from that commit.

### Retest of the fix on `4027f5e`

Dispatched full-effort build: run
[35712168389](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35712168389),
all required jobs green; artifact `Amstrad-build-243-1-full`, RBF
`Amstrad_20260922_4027f5e.rbf`, SHA-256
`582ef0fa24de60ab8a5da76426e24bb8c3626e2a5f2e77cc1228040ff9b527f9`; worst slack
+0.241 ns, zero TNS. With the unmodified Plus-Off CFG, the same driver case
(`sonic-b16-plusoff-4027f5e.json`) now boots the cartridge: c1 `fbb866bf` corrupt title,
c2 `d2affffb` playfield with ring HUD `00`, c3 `142f6341` playfield with HUD `12`. The CFG
hash is unchanged afterwards (`2e585b4c…d8e4`), so the status echo did not persist a
setting. **B16 CPR-from-Off is accepted on hardware** for MGL loading; the OSD echo and a
manual OSD load were not observed (native screenshots omit the OSD).

### B16 SNA path: header type 4 selects 6128+

The same Sonic SNA (header type 4) was loaded alone, without its cartridge, in three
configurations:

| RBF | CFG Plus model | Screen SHA-256 prefix |
| --- | --- | --- |
| `cdcb3c3` | Off | `35c496f5` (vertical bars) |
| `cdcb3c3` | 6128+ (explicit) | `35c496f5` |
| `88262b9` (predates B16 `1b4d45e`; RBF `bad7d369…`) | Off | `253ce13d` (classic BASIC Ready) |

With Plus Off, `cdcb3c3` produces the explicit-6128+ result and the pre-B16 build does
not, so the header selected the Plus model before restore. Types 5 and 6 and the OSD
echo were not observed (native screenshots omit the OSD).

## B18: 664 and 464 saves round-trip on device

Procedure per model: write the Model field into the CFG, load the RBF through an rbf-only
MGL, type `big` with the B17 replay helper (B, I and G share positions on UK and FR
layouts), open the OSD and select `Save snapshot` (F12, nine Down presses, Enter:
Main keeps disabled items navigable, so the count includes the CPR entry), pull with
`sna_pull.py`, then reload the SNA through MGL index 6 **under the original 6128 CFG** and
compare native screenshots. Schedules `osd-save.json` and `type-big.json` are in the
evidence folder (`b18/`).

| Model | CFG `[5:4]` | Saved SNA | Header | `big` in RAM | Before/after screen |
| --- | --- | --- | --- | --- | --- |
| 664 | 1 (`d7d2de0e…`) | 65,792 B, `7ea8f04a…` | v3, type 1, 64 KB, RAM config 0 | `0xAC8A` | `c9d7eef6…` both |
| 464 | 2 (`81b55bec…`) | 65,792 B, `b878d0e6…` | v3, type 0, 64 KB, RAM config 0 | `0xACA4` | `983ec3fa…` both |

Both reloads are byte-identical to the pre-save screen while the CFG requested a 6128,
so the header selected the RAM map and the restored RAM carried the typed text.

Limits and media facts:

- The device `boot.rom` is 128 KB: 6128 and 664 slots only. With Model 464 and no
  464 ROM the core shows a blank grey screen. The 464 run loads
  `/media/fat/games/Amstrad/464FR.ez0` through F7 (`Load CPC464 ROM`) first, and the reload
  MGL loads it again before the SNA. The 664 slots of this `boot.rom` hold 6128 FR
  firmware (the banner reads `128K`, BASIC 1.1), so the 664 run tests the 64 KB map and
  header, not 664 firmware.
- No AmSpirit cross-load: the running AmSpirit is configured as a 6128 Plus and a
  classic SNA does not switch its model. The 2026-09-14 test already covers AmSpirit
  interchange for a 6128 save.
- `sna_pull.py --wait` waits for a generation different from the one it first reads.
  The core's generation restarts at 1 on every core load, so a first save after a
  reload is invisible to `--wait` when the slot already holds generation 1. A plain
  pull after the save works.

## Plus System Cartridge on three models

Checklist item 3A in [the Plus checklist](../../plus/hardware-test-checklist.md). Cartridge
`06_System/Plus_EN.cpr`, SHA-256 `3ce35dfc…81ae` (device and `local/test_media` copies
match). Each model is written into the CFG `[34:33]`, the driver loads the CPR, and 6 s
later two captures are taken 4 s apart. Then keypad 1 (CPC f1) and `cat` + Enter are
replayed. `CAT` separates AMSDOS (drive message) from tape firmware (`Press PLAY`).

| Model | Boot capture | After f1 | After `cat` |
| --- | --- | --- | --- |
| 6128+ | `103db419`: v4 banner, menu `f1 Amstrad BASIC / f2 Burnin' Rubber` | `7b7e2316`: BASIC 1.1 Ready | `6a26c986`: `Drive A: disc missing` (AMSDOS present) |
| 464+ | `103db419` (same menu) | `7b7e2316` | `b523682e`: `Press PLAY then any key:` (no AMSDOS) |
| GX4000 | `7a4e9eeb`: v4 banner, then `Ready`, `14592` and a cursor; no menu | not run | not run |

6128+ and 464+ match the checklist expectation for firmware, BASIC and disk/tape
presence. The checklist's "AMSDOS banner" wording is not what either machine prints;
the `CAT` response is the stronger check.

**GX4000 screen explained.** Master's `plus/gx4000-syscart-menu` integration
([record](../../plus/gx4000-system-cartridge-2026-09-22.md)) resolved this: firmware v4 makes
no model decision and always far-calls logical upper ROM 7 at `&C072`. On an unmodified GX4000,
logical ROM 7 maps to physical page 1 (BASIC) rather than page 3 (menu) per the revised Arnold V
specification (section 2.8), as already implemented in `rtl/plus/plus_mmu.v`. The far call enters
uninitialised BASIC, which prints `Ready`; lines after `Ready` (like `14592`) and `Ready` visibility
itself depend on power-on RAM. The previous checklist expectation of an insert-cartridge splash
had no source. Evidence: `syscart/` in the evidence folder.

## CDT on a CPC 464 overwrites the 464 OS ROM

B8-7 real-CDT playback was attempted on RBF `4027f5e` with `cdt/AmstradDiag.cdt`
(16,401 B, SHA-256 `203775c2…2ee7`), Model 464 (CFG `81b55bec…`) and
`464FR.ez0` loaded through F7. MGLs are in `tape/` in the evidence folder.

| MGL order | Model | Screen before any input |
| --- | --- | --- |
| F7 464 ROM only | 464 | `c90425c2`: BASIC 1.0 Ready |
| F7 464 ROM, then F4 CDT | 464 | `b1e2000c`: vertical bars (crashed) |
| F4 CDT, then F7 464 ROM | 464 | `c90425c2`: BASIC 1.0 Ready |
| F4 CDT only | 6128 (original CFG) | `253ce13d`: BASIC 1.1 Ready |

Cause, from the RTL: every tape SDRAM access uses bank `2'b10` (`rtl/sdram.v`, the
`tape_req` branch sets `active_bank <= 2'b10`), and the tape queue writes the image from
address 0 (`tape_write_queue`, `tape_play_addr` resets to 0). Bank 2 is also the CPC 464
model's memory bank (`rom_loader_route`: index 7 and boot-image chunks 8-9 go to bank 2;
464 OS at `a_hi = 0`). A CDT mounted on a 464 therefore overwrites the 464 OS ROM from
its first byte. The 6128 (bank 0) is unaffected. Upstream MiSTer-devel `rtl/sdram.v`
uses the same fixed tape bank, so the defect is inherited, not introduced by this fork.
It dates from upstream PR #41 (2026-05-09), which added the 464 model on bank 2.

Upstream reproduction on the device: `Amstrad_20260603.rbf`, same 464 CFG. `464FR.ez0`
alone gives `c90425c2` (BASIC 1.0 Ready); `464FR.ez0` then the CDT gives `b1e2000c`,
byte-identical to the fork's crash frame (MGLs `zz_u1.mgl`, `zz_u2.mgl`). PR #41's
description gives the purpose of the 464 mode as SNA compatibility ("many snapshots
were made on a 464 and ROMs need to match"); its file list does not include
`rtl/sdram.v` or any tape source. Before it, only the 6128 (bank 0) and 664 (bank 1)
models existed and bank 2 held tape data alone.

Address map behind the collision (`rtl/Amstrad_MMU.v`, 464 model, SDRAM bank 2):
lower ROM (OS) at page `0x000` = `0x000000-0x003FFF`, base 64 KB RAM at pages
`0x008-0x00B` = `0x020000-0x02FFFF`, upper ROMs at `{1, ROMbank}` (BASIC at
`0x400000`). The tape image is written from `0x000000`: any CDT overwrites the OS, and a
CDT over 128 KB also reaches base RAM.

Consequences: the CPC 464, the natural tape machine, cannot run with a CDT mounted.
Loading the ROM after the CDT boots but overwrites the start of the tape image, so
playback in that order is not a valid test. The `RUN"` attempt (Ctrl + keypad Enter)
only moved the cursor and did not start a load, so B8-7 playback stays open. A fix must
move the tape buffer out of every model's ROM/RAM range; check the 664 and Plus 464+
maps too.

### Fix candidate: tape image relocated to bank 3 at 0x100000

Integrated from `859fd24` on 2026-09-22; **not device-tested** — hardware acceptance is
still open. The tape image now lives in SDRAM bank 3 from `0x100000` upward, the
one region no map uses:

| Region | Owner |
| --- | --- |
| banks 0-2 (model banks) | classic 6128/664/464; every Plus model uses bank 0. Each holds OS ROM `0x000000-0x003FFF`, base RAM `0x020000-0x02FFFF`, upper ROMs `0x400000+` (`rtl/Amstrad_MMU.v`, `rtl/rom_loader_route.v`) |
| bank 3 `0x000000-0x07FFFF` | Dandanator (`dan_ena` override, `Amstrad.sv`) |
| bank 3 `0x080000-0x0FFFFF` | Plus cartridge (`plus_cartridge_memory` `CARTRIDGE_BASE`) |
| bank 3 `0x100000-0x7FFFFF` | **tape image, 7 MB maximum** (`0x700000` bytes) |

Maps audited for the fix: 664 (bank 1) and 6128 (bank 0, extended RAM to `0x1FFFFF`,
upper ROMs `0x400000-0x7FFFFF`) use the same in-bank layout; 464+/6128+/GX4000 run
from bank 0 with the Dandanator/cartridge on bank 3 as above; `vram_bank` follows
`mem_bank` and never reaches bank 3 (`valid_model` maps 3 to 0). Boot writes to bank 3
are the Dandanator download only.

`rtl/sdram.v` relocates the queue's logical address (`tape_addr + 0x100000`, bank
`2'b11`) for both tape reads and writes, and the B8-7 vram-cache invalidation is keyed
to the new bank and the offset address. The queue, `tape_play_addr` and the progress
bar stay 0-based; maximum tape length is 7 MB.

Proven fail-first by `test_tape_image_placement_outside_model_maps` in
`sim/plus/sdram_cartridge_test.cpp`: on the old fixed bank-2 mapping, image byte 0,
the first base-RAM byte and the top-of-window byte all landed in the 464 model bank
(9 assertions failed, including playback reads) and pass after the fix. Expected
addresses are derived from the map in the test's comment. Hardware acceptance of the
fix is open: no device run has exercised a CDT on a 464 with it.

## B8-7: real CDT playback on 464+ loses block 2

Setup avoiding the 464 bank collision: RBF `4027f5e`, 464+ (CFG `[34:33]=3`, hash
`600024f7…4e47`), MGL `zz_ptape.mgl` loading `Plus_EN.cpr` then `AmstradDiag.cdt`.
Replay: keypad 1 (f1, BASIC), then `RUN"` + Return typed on the UK layout
(`tape-uk.json`, built with `mk_typing.py` from `cpc_keys.translate_text`), then Space.

The CDT is well formed (`cdt_blocks.py`): a 3 s pause, then seven header/data pairs
of standard-speed 0x11 blocks (pilot 1162 T x4096, zero 581, one 1162), each header
followed by 10 ms and each data block by 2500 ms. Headers carry `DIAG.BIN` blocks
1-7; block 7 is the last.

Screen sequence, identical in two runs:

1. `Press PLAY then any key:` then `Loading DIAG.BIN block 1`: tape playback,
   header decoding and motor start work.
2. `Found DIAG.BIN block 3`, `Rewind tape`, then `Found … block 4`…`block 7`, each
   followed by `Rewind tape`. The firmware never receives block 2 (header or data), so
   it rejects every later block as out of order. The load never completes.

Block 2 is present in the file and blocks 3-7 decode, so the loss is not a format or
bit-timing problem.

### Classic 6128 control: the defect is Plus-specific

Same CDT on a classic 6128 (original CFG), with cpcec's UK `cpc6128.rom` (SHA-256
`31c3668c…9562`) loaded through the Main ROM slot so the UK layout can type `|TAPE`
(the device `boot.rom` carries French v3 firmware; AMSDOS still comes from it). Replay
`tape6128-uk.json`: `|TAPE`, Return, `RUN"`, Return, then Space.

| RBF | Result |
| --- | --- |
| Upstream `Amstrad_20260603.rbf` (`04080cb7…9db3`) | Full load; Amstrad Diagnostics v1.3a runs (`CPC 6128`, `FDC DETECTED`) |
| `4027f5e` | Full load; same Diagnostics screen |

Both builds load all seven blocks on a classic 6128. `rtl/tzxplayer.vhd` and its
instantiation are byte-identical to upstream. The block-2 loss is therefore specific to
the 464+ / v4 System Cartridge path: Plus tape input, motor output or firmware timing,
not `tzxplayer` itself. Timing note, not a finding: at the 90 s capture upstream had
already started Diagnostics while `4027f5e` showed `Loading DIAG.BIN block 6`; the two
runs were not phase-matched.

### Upstream context (Gemini 3.8 Flash research, run `20260922T150727Z-99263-8ed6`, spot-checked)

- Upstream README advertises `.CDT` support "in very basic form" and documents
  `|TAPE` then `RUN"` on the default 6128 (verified in the README).
- The CPC 464 model and F7 ROM slot arrived in upstream PR #41 (merged 2026-05-09) and
  the 160 KB `boot.rom` with 464 ROMs in PR #43 (2026-05-13), both verified with the
  GitHub API. The 464 bank collision therefore dates from May 2026 upstream. Gemini found
  no upstream issue or forum report of either defect. Upstream issue #26 (tape loading
  needs `|TAPE` when AMSDOS is present) and #22 (CDT speed) are unrelated.
- Gemini's proposed block-2 mechanism (firmware re-arming too slowly for the 2.5 s
  pause) is inference and is contradicted by the classic control above.

## Plus disk I/O on 6128+

RBF `4027f5e`, 6128+ (CFG `13ef32c7…5747`), MGL `zz_disk.mgl`: `Plus_EN.cpr` then
`Space_Gun_(UK)_(1992)_-CPC+-.dsk` (SHA-256 `7d0f799f…e675`) in drive A. After f1:

- `CAT` (UK layout) lists `0100.BIN 2K`, `DISC.BAS 1K`, `CATALOG.JEU 1K`, `174K free`.
- `RUN"DISC` loads the game. Captures 15 s apart: blank, black, then the Space Gun title
  (`be746e7a`, `2bf423a9`) with its logo palette fading in and "Please press fire to
  Continue".

AMSDOS directory read and a multi-file Plus disk load work on 6128+. Gameplay was not
exercised. Evidence: `disk/` in the evidence folder.

## Restoration

Original CFG restored and hash-checked (`2e585b4c…d8e4`); the device is back at MENU.
Temporary SNAs, MGLs, replay files and `zz_*` screenshots are removed from the device
(screenshots archived in the evidence folder). The `cdcb3c3` RBF stays on the device.
