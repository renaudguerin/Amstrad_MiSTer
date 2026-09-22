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

The B16 SNA path (header 4/5/6 selecting Plus) was not discriminated: without a
cartridge a Plus snapshot shows garbage whichever model runs.

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

## Restoration

Original CFG restored and hash-checked (`2e585b4c…d8e4`); the device is back at MENU.
Temporary SNAs, MGLs, replay files and `zz_*` screenshots are removed from the device
(screenshots archived in the evidence folder). The `cdcb3c3` RBF stays on the device.
