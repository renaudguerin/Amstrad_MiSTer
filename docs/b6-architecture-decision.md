# B6 classic/Plus and video-boundary architecture decision

**Decision date:** 2026-09-01  
**Status:** architecture complete; menu-only separation is the first implementation slice

This note closes the design question in backlog B6 without claiming that the hardware-only
DSC4/SHAKER failures are understood. It follows the production signals from each timing engine
through the Gate Array/ASIC, `crt_filter`, the scaler, and the physical outputs, and separately
audits the mode selector, register-write enables, persistent state, and MiSTer menu mechanism.

## Decisions

1. **Keep one core and keep both timing engines synthesized.** Classic and Plus remain runtime
   selectable models of the same core. Clock or register-write gating cannot remove either
   implementation from the FPGA fit, so it offers no meaningful ALM or M10K saving. The current
   54% fit is not evidence for a resource-driven split.
2. **Do not gate either engine from raw `plus_mode`.** `plus_mode` changes combinationally from
   `status[34:33]`, while the menu instructs the user to apply `Reset & apply model` separately.
   Gating on the raw selection can therefore freeze one engine before reset, miss a boundary
   write, and later resume stale timing or cartridge/DMA state. Output selection already keeps
   the inactive engine off the visible and bus-facing paths.
3. **Use conditional menu visibility now.** A wider `status_menumask` can hide Plus-only,
   classic-only, disk-capable, and tape-capable options without changing any persisted status
   encoding. Hidden options retain their values; applying a machine remains an explicit reset.
   A mask cannot atomically rewrite several companion fields, so this is not described as a
   complete preset system.
4. **Keep scaler acquisition timing stable.** `HBLANK` becomes display enable in
   `video_mixer`; the scandoubler and ASCAL measure geometry from it. Sending live/raw HBLANK
   downstream necessarily changes acquisition width, exactly as the supplied hardware captures
   show. The scaler-bound tuple should therefore retain Full/filtered HSYNC, VSYNC, HBLANK, and
   VBLANK.
5. **Preserve raw phase before the filter.** Classic `ga40010` and Plus `asic_video` already
   use raw CRTC HSYNC to force RGB blanking. Any next B1 experiment should observe or explicitly
   select that pre-filter RGB/sync phase while leaving scaler geometry stable. The ownership
   split belongs at `crt_filter_output_select` / `Amstrad_motherboard`, not after ASCAL, where
   raw HBLANK no longer exists.
6. **Do not reopen the complete ASIC register page.** The B5 map shows that most ordinary ASIC
   behavior already has a production owner and deterministic fixture. Follow-up work is limited
   to its named G1-G7 gaps and evidence boundaries.

## Why the tempting gates are unsafe

Both `CRTC` and `asic_video` receive CRTC I/O cycles, and both classic and Plus Gate Array timing
remain live. That duplication looks wasteful, but runtime enables do not change synthesis
footprint. More importantly, model selection is not an atomic mode transition today. The design
must preserve these inactive-state contracts until a reset-delimited transition protocol and its
tests exist:

- classic and Plus CRTC/GA phase state;
- `plus_vidword` and sprite request/ack state;
- Plus MMU/RMR2, CPR atomic replacement, and the cartridge image;
- Dandanator ownership, `rom_map`, SNA parser drain and snapshot shadows;
- DMA reset and restart behavior.

The B7 mutation audit establishes output isolation for steady-state selections. It does not prove
arbitrary menu-transition edges safe. Before any future power-oriented gate, add a transition
matrix covering writes immediately before/after selection, snapshot load/drain, CPR completion,
DMA and sprite seams, then measure switching power. Do not promise a fit reduction from that work.

## Menu capability mask

The first implementation slice uses four capability bits in addition to the existing vertical-
crop bit:

| Mask bit | Capability | Visible items |
|---|---|---|
| 2 | Plus selected | Load CPR |
| 3 | classic selected | CRTC type, classic Model |
| 4 | classic, or selected Plus model has FDC | mounted DSK slots, FDC option |
| 5 | classic, or selected Plus model has tape | Load CDT, Tape sound |

`Machine preset` (`P2O[34:33]`) and `Reset & apply model` stay visible. Common display controls,
SNAC, Multiface, PlayCity, Dandanator, and ordinary ROM loaders stay unchanged until their
ownership or policy is explicit. The intended mask expression is:

```verilog
status_menumask = {10'b0,
                   (!plus_mode || plus_has_tape),
                   (!plus_mode || plus_has_fdc),
                   !plus_mode, plus_mode, en270p, 1'b0};
```

The associated `CONF_STR` entries use `d2` for Plus-only, `d3` for classic-only, `d4` for
FDC-capable, and `d5` for tape-capable visibility. Classic mode deliberately keeps disk and
tape controls visible, matching the existing `!plus_mode` capability gates; the Plus models
then narrow those controls to their physical capabilities. This changes reachability only; it
does not renumber status bits or silently coerce retained settings.

## B1 follow-up sequence

The current Live mode is a rejected hardware candidate, not a closure. Its HBLANK window moves
the acquisition boundary, so a narrower image is an architectural consequence rather than a
remaining off-by-one bug. The next useful steps are:

1. keep the scaler tuple Full while pinning raw phase movement and fixed display geometry in a
   production CRTC-to-GA/ASIC seam fixture;
2. make the existing raw-HSYNC RGB force blank an explicit diagnostic selection if the fixture
   proves the separation;
3. if hardware still differs, capture raw CRTC HSYNC, shaped GA/ASIC HSYNC, filtered HSYNC,
   HBLANK, DE, RGB, and physical sync together. Only hardware can decide whether SHAKER/DSC4
   depends on physical sync edges rather than RGB/blanking phase.

Direct-video and ASCAL paths both need coverage because both inherit display geometry from the
same pre-scaler HBLANK/DE contract. Do not route live HBLANK to either path as a speculative fix.

## Deliberately deferred

- A reset-delimited latched machine-mode protocol and power gating, pending transition tests and
  measured value.
- Multi-field presets: `status_menumask` controls visibility, not atomic status rewrites.
- B5 G1-G7, which each retain their own evidence or UI prerequisite.
- A raw physical-sync output mode, pending the hardware discriminator above.
