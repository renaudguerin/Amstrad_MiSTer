---
name: mister-capture
description: MiSTer capture of a title, demo or SHAKER screen on the real device (root@mister) through the hardware-loop driver. Use to reproduce a reported display defect, compare RBFs, or collect native-screenshot evidence before title-driven RTL work.
---

# mister-capture

Loads an RBF plus DSK/CPR on the user's MiSTer, captures native PNGs, and turns them into
evidence. [The driver guide](../../../docs/investigations/hardware-runs/mister-hardware-loop-driver.md) owns the case JSON
format, driver flags and limits; for a SHAKER walk use its CSL runner section instead. This
skill caches the device facts and gotchas the guide does not.

A native capture is **reproduction evidence**, never a hardware verdict: it omits the OSD and
the analogue output, and the project ranks real hardware and Logon System photographs above it.

## 1. Brief from the user

The user has seen the screen; you have not. Ask in one message, before touching the device:

- **Which screen** shows the defect, and the machine settings they used (model, CRTC, sync filter).
- **Roughly how long** after load the screen appears.
- **What input** reaching it takes (keys, joystick, menu choices).

The time ballpark sets `settle_delay` and saves discovery runs. Input decides the shape of the
case: none (CPR auto-boot, self-running demo), a short typed sequence (`RUN"` plus a menu key),
or real play. Real play needs a full input script; agree its scope with the user before writing it.

**Done when** screen, settings, timing ballpark and input needs are known.

## 2. Preflight the device

One device operator at a time. Confirm reachability and what is running:

```sh
ssh -o BatchMode=yes -o ConnectTimeout=5 root@mister 'cat /tmp/CORENAME; echo'
```

`MENU` means idle. Any other core name may mean the user is playing: ask before loading.
If SSH fails, hand off to the user (device off or asleep).

Device layout:

| What | Where |
|---|---|
| RBFs | `/media/fat/_Computer/Amstrad_YYYYMMDD_<sha>.rbf` (ignore `._*` AppleDouble files) |
| Plus cartridges | `/media/fat/games/Amstrad/cpr/01_PlusGames/*.cpr` |
| Disks | `/media/fat/games/Amstrad/dsk/` |
| Core settings | `/media/fat/config/Amstrad.CFG` (16 bytes) |
| User's OSD screenshots | `/media/fat/screenshots/Amstrad/` (earlier reports often have one) |

Locate media with `find /media/fat/games -iname '*<title>*'`. The repository keeps the same
cartridges under `docs/plus/cartridges/`; compare SHA-256 to confirm the device copy matches.

**Done when** the device is idle, and the RBF and media paths and hashes are known.

## 3. Establish RBF identity

Captures are only useful against a named source. For the RBF you intend to load:

```sh
git log --oneline <rbf-sha>..master -- rtl Amstrad.sv files.qip
```

An empty list, or only changes irrelevant to the symptom, means the device RBF represents
`master` for this defect. Otherwise use a newer RBF (CI artifact or `output_files/`) and copy it
under a distinct filename, since the driver rejects ambiguous RBF prefixes.

**Done when** the manifest will pin an RBF whose relationship to `master` you can state.

## 4. Apply settings through the CFG

`driver.py` records `declared_settings` but applies none. The core reads `Amstrad.CFG` when it
loads, so settings are changed by editing status bits there, then restored.

Decode bits from `CONF_STR` in `Amstrad.sv`: `On` is one bit where `n` is `0-9` then `A=10 ... V=31`
(`oN` adds 32), and `O[hi:lo]` is a field. Bit `b` lives at byte `b // 8`, mask `1 << (b % 8)`.
Common fields: CRTC bit 2 (0 = type 1), Model `[5:4]`, **Plus model `[34:33]`
(0 Off, 1 GX4000, 2 6128+, 3 464+)**, Sync filter `[36:35]`.

**Loading a CPR leaves the machine model unchanged.** Set Plus model before a cartridge run.

Procedure, with `$S` a scratchpad directory:

```sh
ssh root@mister 'base64 < /media/fat/config/Amstrad.CFG' | base64 -d > $S/Amstrad.CFG.orig
shasum -a 256 $S/Amstrad.CFG.orig        # record this: restore must match it
python3 -c "d=bytearray(open('$S/Amstrad.CFG.orig','rb').read()); d[4]=(d[4]&~0x06)|0x04; open('$S/Amstrad.CFG.new','wb').write(d)"   # example: 6128+ (bits 34:33 = 2)
scp -O -q $S/Amstrad.CFG.new root@mister:/media/fat/config/Amstrad.CFG
ssh root@mister 'sha256sum /media/fat/config/Amstrad.CFG'   # must equal the new file's hash
```

Clear the other bits of a field when setting it. Restore with the same `scp -O` of `.orig` and a
hash check as soon as capturing is finished, including after a failed run. This is the user's
real configuration.

**Done when** the device CFG hash equals the intended file.

## 5. Write the case and capture

Start from an existing case in `scripts/hardware-loop/` and pin `expected_sha256` for RBF and media.

- **Input:** CPR boots without any. DSK needs an MBC `raw_seq`; MBC lives in `/tmp` on the
  device and disappears on reboot, so rebuild it per `docs/mister-mbc-cross-build.md` if absent.
- **Timing:** captures are serial and each round-trip takes seconds, so capture N lands well
  after `settle_delay + N * capture_delay`. Set `settle_delay` a little before the user's
  ballpark and take several captures to bracket it.
- Every run needs a new `--out-dir`; the driver refuses to overwrite a manifest.

```sh
python3 scripts/hardware-loop/driver.py <case.json> --target root@mister --ack-main-cmd --out-dir $S/run-001
```

Look at every PNG. **Done when** one capture shows the target screen; if none does, adjust
`settle_delay` from what the captures do show and rerun.

## 6. Characterise the defect

Compare the capture with the user's report or OSD screenshot. OSD screenshots are scaled, so
compare shapes and colours, not pixel positions; read capture dimensions from the manifest.

- Per-row colour sets over the affected region name the palette values in each band.
- Crop and upscale with `Image.NEAREST` to show the seam to the user.

If the defect does not reproduce, or its nature suggests timing (flicker, a moving seam), ask
the user whether a repeat load is worth it. A repeat goes to a second output directory. Identical
hashes mean a static frame. For animated screens, `ImageChops.difference` gives the rows that vary
between loads: a defect confined to varying rows points at a timing seam, not static content.

**Done when** you can state whether the defect reproduces on this RBF and on which rows or region.

## 7. Keep the evidence

Scratchpad files vanish with the session. Copy decisive captures, manifests, the case JSON and
zoom crops to `docs/references/<topic>-<YYYY-MM-DD>/` (gitignored, never committed). Record in
the relevant dated evidence document: RBF name and hash, media hash, CFG bits applied, capture
hashes, and what the captures do and do not establish. Confirm the CFG restore hash last.

The next step for a display defect is a simulation trace that finds the first wrong pixel's
plane and register state (see `docs/plus/hardware-defect-triage-2026-09-01.md` for the
discriminator each symptom family needs). Captures choose where to look; they do not choose the fix.
