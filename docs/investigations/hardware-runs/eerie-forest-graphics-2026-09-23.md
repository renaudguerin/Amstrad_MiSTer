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
simulation reproduction; candidate MiSTer acceptance is recorded below.

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

## Left-edge timing discriminator: unresolved

The additional production-T80 probe records 1,017 post-startup programmed
interrupt events. Every event has raw-HSYNC-to-raster-fire = 384 master
clocks (6 microseconds) and INT assertion one master clock later. With
R2=49 and R3l=11 this agrees with revised Arnold's shaped trailing-edge
clamp. DMA is not pending. A representative PRI7 chain, relative to raw
HSYNC, is ACK +480, opcode0038 consumed +791, opcode0039 consumed +855,
first SSCR write onset +912 and register change +913. The reveal loop's
later SSCR writes occur at C0=2 and recur every 4,096 master clocks.

AmSpirit normal-boot instruction breakpoints and a live STATUS1 calibration
were collected. They do not expose the actual CRTC counter/subphase at bus
sampling. Their instruction-completion boundaries cannot be subtracted
from the production opcode-consumption/write-onset events as though they
were identical. In particular, AmSpirit's exported standard SNA CRTC
counter bytes are zero, and a synthetic snapshot with the private SPRT
chunk removed did not preserve usable timing; those observations are
excluded. No CPU or PRI fault is established by this comparison.

The left-edge screen spill therefore remains open. Preserve the
source-consistent PRI timing and the documented 16-dot mask. The next
useful evidence is an original-Plus trace of INT, interrupt acknowledge and
SSCR `/WR` against raw CRTC HSYNC for this CPR, or an AmSpirit bus-event
trace exposing actual C0/subphase and input/write sampling. Private records:
`diagnostic/PHASE.md`, `phase.log`, `amspirit-left/README.md` and its scripts
and datasets. AmSpirit's original full state/configuration was restored and
breakpoints cleared after asynchronous snapshot application completed.

## Exact candidate build

[CI run 35900649453](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35900649453)
passes simulation, production-T80, full synthesis and required-gate for
source `566e0c7d220d226f06eda6cc0d4817ac04f95703`. Artifact
`Amstrad-build-264-1-full` records `build_mode=clean_full` and Quartus17.0.2.
Timing: setup minimum +0.580 ns, hold +0.243 ns across seven clocks, zero TNS.
Resources: 23,795 ALMs (57%), 28,380 registers, 102 RAM blocks and 35 DSP
blocks. The compile takes 18m36s, including 13m42s fitting; the review's
fit/timing concern did not materialize on this build.

Candidate RBF `output_files/Amstrad_20260923_566e0c7.rbf`, SHA-256
`d9784cc40a737b5359e248f73cca0e3436aeddb862ad219239e95106400fd919`,
was copied to the same filename in MiSTer's `_Computer` directory and its
hash checked there. Reports are under `output_files/candidate-566e0c7/`.

## Candidate device acceptance

The exact `566e0c7` RBF was tested with explicit 6128+, Full sync, Original
CPU timing and unchanged media. Nine Eerie native captures (768×273) show
clean black/revealing backgrounds without the three horizontal image
strips, followed by advancing landscape and runner animation. All nine
were inspected. The dotted logo and green left-edge screen sliver remain
visible; this is partial graphical-defect closure, not a pixel-perfect or
full-demo verdict.

Per user instruction, Sonic GX was the only sprite regression title.
Three native captures show the coherent title, then two advancing Green
Hill attract-gameplay scenes. No sprite regression is visible in those
samples. Media SHA-256:
`4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae`.
No player-input gameplay or continuous flicker measurement is claimed.

Private case files, applied/saved CFG and manifests with every PNG hash
are under `hardware-566e0c7/` in the evidence directory. Final CFG restore
was read back byte-for-byte and matches
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.

Eerie manifest SHA-256: `97f46c3bf664d85edc9a17fe664355cd192b2e25570de51f544a4302df3b49d3`.

Sonic manifest SHA-256: `637177475de3fcac0f7719105df46a6ea5b59314659d10654bdbe631eaadf431`.
