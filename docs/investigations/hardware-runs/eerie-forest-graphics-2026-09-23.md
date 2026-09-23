# Eerie Forest left-edge spill and reveal-line leakage

Investigation base `a8d755b3d468c66d7b0ea65fa2f0b5f69205f658`. The user reports
left-edge graphical spill in three screenshots and unwanted horizontal lines
while the landscape is black. Settings: 6128+, believed Full sync, no input,
approximately 10–30 seconds after boot.

## Reproduction and reference

Both symptoms reproduce with explicit 6128+ and Full sync on the exact
`57a90bc` integration RBF, SHA-256
`11959f68712177fa2f73958ca98ac7e98b1b5a854f8cb47d1e0b4db6c9638d8b`.
Original CPR SHA-256:
`72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215`.
Later commits through the investigation base have no synthesized RTL changes.

The hardware-loop driver sampled nine native 768×273 captures after a
10-second boot delay and 2-second settle, with 3 seconds between serial
requests (transport adds wall time). The first two show three horizontal
landscape strips against the black reveal background; subsequent captures
show the widening landscape, left-edge green spill and dotted logo sliver.
All nine captures were visually inspected. These are scaler-buffer captures,
not an independent HDMI or original CPC hardware observation.

AmSpirit Lite 1.15.1 / core 2491682, model 4 / CRTC 3, boots the same unchanged
CPR. Sixteen samples at 100-frame intervals show the intended black backdrop,
logo entry, landscape reveal, and runner scene without these strips or
left-edge spill. Frame counters and wall times are not exact cross-system
alignment. The comparison identifies visible divergence, not a hardware rule.

Private evidence is in `docs/references/eerie-graphics-2026-09-23/`
(`docs/references` resolves to `docs/specs` in this checkout):

- User originals: `local/captures/broken/*Eerie_Forest*`.
- `full-baseline-02/manifest.json`: case, identities, all nine capture hashes.
- `full-case.json`, `capture-baseline.py`, saved/applied CFG files.
- `amspirit/`: original configuration, original SNA, sixteen screenshots,
  final scene SNA and state; `amspirit-reference.py` records the procedure.
- `mister-contact.png` and `amspirit-contact.png`: visual navigation sheets.

MiSTer CFG restored and read back byte-for-byte, SHA-256
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
AmSpirit's original model 2 / CRTC 1, rendering settings, snapshot and pause
state were restored; identity was checked afterward.

## Diagnostic boundary

The first diagnostic attributes each affected pixel to screen, border or
sprite and records SSCR, split and palette transactions with production
T80 timing. Widening SSCR's screen mask to clip sprites is not justified:
current sprite-over-mask behavior is deliberate and tested. The captures alone did not establish a behavioral repair; the production trace
below isolates the sprite-state defect.

## Sprite reveal-mask repair

The production-T80 replay uses the unchanged CPR, real 64 MHz clock ratios,
and Full sync. At frame 325, the landscape is hidden by opaque black sprites
2–13, magnified 4×4 and reused vertically. Sprite Y writes cross a scanline:
sprites 2–8 are repositioned after their X windows, while sprites 9–13 are
repositioned early in the following line, before their X windows.

At CRTC line 108, sprite 9 changes Y60→104, selecting source row 1 instead
of row 12. The old engine immediately compares against the new row but only
retags its staged banks at a line seam. It therefore exposes the landscape
until the next seam even though the fetch port has ample time to refill.
The same mechanism explains the right strips at lines 65 and 159.

[Revised Arnold §2.1, archived 2025-01-02, revision 115989](https://web.archive.org/web/20250102091704/https://www.cpcwiki.eu/index.php/Arnold_V_Specs_Revised)
was retrieved and read afresh. It states that attribute writes do not disable
a sprite and a changed coordinate resumes display at the new location when
reached. The local private HTML is `arnold-revised.html`. Original Arnold
§2.1 also permits position writes while the sprite is off; the focused test
uses a write before the horizontal window and does not assert zero-latency
mid-emission behavior.

The repair reuses row promotion/refill for live attribute retargeting. It
rejects stale same-edge requests/responses and retains the existing bounded
fetch-port model. Test `s13_y_rewrite_before_window` replaces the former
seam-only model assertion: sprite 9 at X432, line65, Y16→60 at hp16 must
render source row 1 after 1,664 uncontended master clocks. It fails before
the repair and all 17 sprite tests pass afterward. Exact command:

```text
make -C sim/plus run/asic_sprites_tests CXX=/opt/homebrew/opt/llvm/bin/clang++
```

Logs: `diagnostic/s13-before.log`, `diagnostic/s13-after.log`. This test pins
the interaction between live attributes, staged RAM and emission, with
expected row/pixel values derived from the coordinate rule.

The candidate's seven-second normal-boot replay completes successfully.
At frame 325, nonblack pixels on the three previously leaking rows change:

| CRTC line | Before | Candidate |
|---|---:|---:|
| 65 | 353 | 0 |
| 108 | 288 | 0 |
| 159 | 345 | 0 |

The small left fragments on those rows also disappear: earlier retargeting
supplies their data before the following seam without changing walker
priority. Diagnostic sources, commands, metadata format and paired frames
are retained in `diagnostic/README.md`, `run/` and `run-after/`. This is
simulation reproduction; candidate MiSTer acceptance remains pending.

## Separate left-edge screen residual

The dotted logo and later green landscape sliver are screen-plane pixels,
not the reveal-mask failure. Frame400 line228 changes SSCR AC→8C at C0=2,
serializer dot0, as the first-character mask ends. Old RA6 fetch data passes
through the 12-dot horizontal delay after RA becomes4. The recorded delay
matches its input history. Flushing that history or widening the mask has
no established source basis. The next discriminator compares the software
write phase and production address-to-pixel timing against AmSpirit.

## Review and selected gate

Fresh Opus 5.5 medium review `20260923T180629Z-62347-d90a` found no blockers
in the sprite diff. It checked live/seam arbitration, response rejection,
row-tag ABA, CPU write-through and service fairness. A same-row attribute
change can still leave speculative next-row preparation stale until the
next seam; this is pre-existing and outside the reproduced fix. Exact
mid-emission refill latency is not claimed. Wider per-sprite maintenance
enables make Quartus fit/timing verification important.

The final selected gate passed after using LLVM C++ for nested makefiles:

```text
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
select_tests: PASS 3 benches: run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests
```

The initial build attempt failed because system C++ could not find `cstddef`;
no behavioral test failed in that attempt. The retry log is
`diagnostic/selected-gate-retry.log`. No RTL or test changes followed the
passing gate or review. `git diff --check` passes.
