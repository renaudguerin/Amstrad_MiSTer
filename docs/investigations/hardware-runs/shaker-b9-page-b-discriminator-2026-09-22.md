# SHAKER B9 type-1 page B: discriminator brief — 2026-09-22

A scratch replay of the actual SHAKER binary on the production T80/GA/CRTC
logic reproduces the C0=3F boundary interaction and its extra 64 µs VSYNC
interval. This isolates a concrete mechanism, but does not establish the
correct hardware ordering at the collision. No RTL change or hardware
experiment was made.
The MID FRAME reference glyph remains disputed; its recorded residual is
retained pending clearer evidence. With no pre-fix page-B capture, neither
observation establishes a regression.

## Established residuals

From the [repaired-build hardware record](shaker-repaired-95e6f56-2026-09-22.md)
(build `95e6f56`, device disk SHA-256
`65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`):

| Page-B row | Real CPC reference | Repaired capture | Status |
|---|---|---|---|
| C9=0, C0=3F, each of three update-delay blocks | 2740 | 2780 | Established +64 µs residual (all three blocks) |
| Other update-delay rows (C0=3D/3E/00/01) | 2740, 2740, 2780, 2760 | Same | Match |
| Four MID FRAME SIZE (R6 pairs 50/50, 7F/50, 50/7F, 7F/7F) | 4F40 | 4E40 | Residual retained; reference glyph E/F ambiguous, looks closer 4F40 — do not correct or claim closed |
| Four EVEN+ODD totals | 9C60 | 9C60 | Match |

No source-derived repair is established. No proven regression.

## Binary provenance and instruction map

`SHAKE27B.BIN` extracted from `test_media/shaker/shaker27.dsk` (disk SHA-256
above): binary SHA-256
`c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88`.
AMSDOS load `&3900`, entry `&4268`. Static map only — no binary, dump or
disassembly is copied into this repository:

- R8 update-delay write: `&91E7` `OUT (C),C` (R8=3), reached through a
  calibrated NOP sled with entry points `&9FC0/&9FBF/&9FBE/&9FBD/&9FBC`
  for the C0=3D/3E/3F/00/01 points (adjacent labels 3E/3F one sled byte apart).
- Measurement loop `&91F7–&91FE`, formatted with constant `0x0470`.
- MID FRAME SIZE routine `&9266..&932B`: two VSYNC-count loops at `&92CD`
  and `&92FE`, calibrated offsets `0x0470` / `0x5270`.
- Setup written by that routine: R4=`4D`, R7=0, R9=7, R8=3; the four cases
  patch the two R6 OUT immediates with decimal 50 (`32` hex) and `7F`.

**Limits.** Static disassembly alone does not establish write acceptance.
The replay below observes that phase under its explicit entry/setup contract;
it does not prove equivalence to the complete menu-launched hardware run.

## Rejected: global stage-B removal

A retrieval pass proposed removing the stage-B C9.0 poke globally. Rejected:

- French ACCC v1.11 §19.5.3 p.211 diagram panels 18(R)/22(V)/26(Z1) (even
  frame, R9 odd, C9 initially 1) show C9=1 at C0=5 and C9=0 at C0=6 — stage B
  does change C9 in documented panels. Bench t21 matches
  ([sim/sim_main.cpp](../../../sim/sim_main.cpp) ~5995–6004).
- French §19.5.3 p.210 places the third/fourth microsecond relative to the
  `OUT` on R8, not to an arbitrary unit write.
- French §19.8.2 p.226 line counting does not resolve stage-vs-boundary
  priority when both coincide.
- [rtl/CRTC.v](../../../rtl/CRTC.v) 362–369: `line_new` wins the poke on the
  shared edge; the source marks that coincidence explicitly unpinned. t21
  keeps both stages mid-line and never exercises it.

Also rejected: a never-reset C9 assertion and a claimed 157-line frame-origin
oracle derived from the calibrated VSYNC interval. No reinterpretation of the
MID glyph is accepted; the recorded residual stands.

## Executed production CPU/bus diagnostic

The existing `sim/crtc_t80_top.sv` fixture had 256-byte RAM and no PPI VSYNC
input. A scratch copy widened RAM to 64 KiB, routed PPI reads to raw VSYNC,
and added observation ports. Production T80pa, GA divider/WAIT, CRTC wrapper
and rule engines were unchanged. GHDL 6.0.0 generated the T80pa netlist;
Verilator 5.052 built and ran three points of the authentic driver at `9139`.
The full header-stripped payload was loaded at `3900`, with `BF03=1`.
Per-point setup, self-modifying sled, synchronization and delay helpers all
executed from that payload. These helpers are inside the module, not missing
firmware. No baseline suite or assertion oracle was involved.

| Label | R8 bus-write start C0 | Stage A result | Stage B result | Last raw VSYNC interval |
|---|---|---|---|---|
| 3D | 61 | C0=62 | C0=63 | 643072 ticks / 157 lines |
| 3E | 62 | C0=63 | C0=0 boundary, C9=2 | 643072 ticks / 157 lines |
| 3F | 63 | C0=0 boundary, C9=1 | C0=1, C9=0 | 647168 ticks / 158 lines |

Each write starts 152 master ticks after opcode fetch at `91E7`; the toggle
arms one tick later. Thus stage A meets the line boundary specifically in the
3F case. Stage B then resets C9.0, unlike the 3E case where stage B itself
meets the boundary. The extra interval is 4096 ticks = 64 µs, matching the
observed discrepancy's magnitude. The final VSYNC rise precedes each point's
return fetch at `9211` by the same 7471 ticks.

**Evidence limits.** The driver starts at `9139`, bypassing menu-level setup;
initial SNA registers and simplified PPI input are fixture assumptions. Its
synchronization does not prove identical hidden parity/history to a complete
hardware launch. Logged states resolve the collision for this fixture. The
157/157/158 intervals are raw VSYNC observations, not recovered output strings:
the payload's formatted result buffer was not inspected. Post-evaluation logs
also do not directly sample the pre-edge poke pulse; stage transitions, C9
changes and unchanged RTL establish that mechanism together.

Private evidence is retained under ignored
`docs/screenshots/shaker-b9-page-b-2026-09-22/replay/`: scratch top/driver/build
script, payload, observations and `b9_run.log` (SHA-256
`d93adf5bf4061301cf639af398e11a8ab58a59f04b360591e2f6cdf122d4d705`).
**Use raw ticks:** the diagnostic's printed µs/line conversions are wrong by
four; correct scales are 64 ticks/µs and 4096 ticks/line. The observations'
claim that both 3E stages precede the boundary is also incorrect; the table
above records the reviewed result. Original artifacts are retained unchanged.
Build and run commands were:

```sh
bash .roster-scratch/b9-trace/b9_build.sh
(cd .roster-scratch/b9-trace && timeout 570 ../run.dgQed6/b9_obj/b9_t80_tests ../b9-retrieval/SHAKE27B.BIN 3)
```

The script records task-local paths; preserve the named scratch layout or
adapt those paths from the retained copies before reproducing it. Next,
observe the result buffer at `9211` to tie the raw interval to SHAKER's own
calculation, then resolve collision precedence against targeted hardware or
source clarification. Do not globally remove stage-B pokes: p.211 rules that
out. No fail-first repair vector is justified yet. Hardware remains under
coordinator ownership; this task released its unused grant without access.

## Sync path: raw PPI vs filtered display

- SHAKER's numeric rows poll PPI Port B bit 0 = raw selected CRTC VSYNC
  ([rtl/Amstrad_motherboard.v](../../../rtl/Amstrad_motherboard.v) `ppi_ipb`
  at 1151), which bypasses `crt_filter` in every mode; the display filter
  cannot directly change a polled value.
- OSD "Sync filter" has three modes — Full, Raw pixels, Raw CRT
  ([Amstrad.sv](../../../Amstrad.sv) `status[36:35]`) — selecting the timing
  tuple for the display/scaler/screenshot stream, not the PPI input.
- Full and Raw pixels both use the entire filtered acquisition tuple.
  Raw pixels changes pixel/byte policy; it does not expose raw output sync.
  Raw CRT selects GA-shaped monitor HSYNC/VSYNC and raw CRTC HSYNC as
  horizontal blank, bypassing filter outputs. Raw CRT is therefore not a
  direct raw-CRTC-sync output in every signal.
- Requested vs applied is distinct: the request is normalised and commits
  during filtered VBLANK and CPU-owned phase, or immediately on reset ([rtl/Amstrad_motherboard.v](../../../rtl/Amstrad_motherboard.v)
  request/commit); native PNGs cannot verify the live OSD mode. Capture
  provenance and stale images still matter for screenshot reliability.
- No mode comparison was performed. The earlier run requested Full through
  its configuration; neither native PNGs nor that saved request independently
  observe the live applied register. Native and scaled screenshots use the
  same scaler buffer according to the companion capture investigation;
  software resizing is not an independent physical HDMI observation.

## Evidence

Source retrieval/adjudication runs: `Gemini20260922T063753Z-96527-513b`,
`Opus20260922T063947Z-2200-2b4b`, `MiMo20260922T064717Z-26888-12f7`; bounded
Astra adjudication rejected source overclaims. Muse replay
`20260922T065604Z-42773-d104` completed; a fresh bounded Astra inspection
accepted its localized timing evidence with the qualifications above. Rule citations use French
[ACCC v1.11](../../specs/ACCC1.11-FR.pdf) (user-owned, untracked). Scratch
inputs are not linked. `git diff --check` clean; no simulation gate for this
documentation-only change.
