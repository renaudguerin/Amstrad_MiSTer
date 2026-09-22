# Device acceptance of `cdcb3c3` (B20-1, B16)

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

## Restoration

Original CFG restored and hash-checked (`2e585b4c…d8e4`). The temporary SNA
`/media/fat/games/Amstrad/zz_b16_tmp_sonic.sna`, the `/tmp` MGLs and the `zz_b16_*`
screenshots are removed at session end; the `cdcb3c3` RBF stays on the device.
