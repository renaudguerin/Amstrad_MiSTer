# Non-classic independent review — 2026-09-03

Read-only cross-provider review by Muse of committed
`a98590a33345f2c529fcd6d30676235d4193bfd7`. Method: read-only `git show` at
the committed tip. No edits, no `make`/gates, no delegation, no remote
actions. Source claims checked against primary refs where cited; CI-green
alone is not clearance.

Excluded with no verdict: B3 P10 VRAM/frame-tap foundation (separate
reviewer); current FDC recovery (stash
`0fe18a4513a47e4f21e0f504f002673a853388c3`); classic CRTC/GA/B1/IA rows
(Accuracy Round 2 consequence + B1 hybrid blanking owned by the separate
classic reviewer — Live-blanking geometry belongs there); B8 full architecture
audit (`docs/backlog.md` B8, deferred by decision). Closed historical rows are
not work. `docs/b6-b10-review-2026-09-02.md` already holds the prior Opus
CLEAR for B6/B10; those verdicts are accepted without re-review — only
recording/follow-up reconciliation below.

## 1. B9 archive + first test-suite cleanup — CLEAR

Scope: `825ecef` (18 accuracy + 7 Plus records to `docs/accuracy/archive/`,
`docs/plus/archive/`, index READMEs, `docs/current-status.md:128-130` links)
+ `84e6969` (removed duplicated P8 `plus_model_select` table, duplicate
motherboard lint, unused `expect_xfail_*`, stale fixture-first comments).

- Archive indexes exist at tip (`docs/plus/archive/README.md:1-14` and the
  accuracy counterpart); `current-status.md` links the archives rather than
  narrating them.
- Removed P8 table (`sim/plus/plus_p8_test.cpp:324-353` deleted) was a leaf
  truth table mirroring `rtl/plus/plus_model_select.v:29-46`. The B6 fixture
  `sim/plus/b6_menu_mask_test.cpp:20-25` derives `0x38/0x04/0x14/0x24`
  (`+0x02` with crop) from the mask contract
  `rtl/plus/plus_menu_capability_mask.v:15-21`, independently re-derivable
  (classic `0x38`, GX4000 `0x04`, 6128+ `0x14`, 464+ `0x24`). `ram_128k` leaf
  is no longer leaf-pinned, but its production consumer (`Amstrad.sv:1353`
  `ram64k`, `mem_bank`) remains pinned by cleared HF-3 integration — intended
  de-duplication, not lost coverage.
- Removed lint block was a second identical `Amstrad_motherboard -Wall`
  recipe. Canonical `sim/plus/Makefile:622-630` `motherboard-lint` remains and
  `lint: motherboard-lint` still gates it. Not a coverage loss.
- `expect_xfail_*`: zero definitions or callers at tip; generic XFAIL/XPASS
  runner retained. Safe.

No source/acceptance blocker. The stashed u765 pre-edge discriminator
(`0fe18a4`) is FDC recovery, not checked-in suite failure. No hardware
residual for this row.

## 2. B6 conditional menu visibility — CLEAR (recordable); B10 stays CLEARED

Opus verdicts accepted without re-review. Reconciliation:

- B6: `rtl/plus/plus_menu_capability_mask.v:15-21`,
  `plus_model_select.v:29-46`, `Amstrad.sv:48-93` CONF_STR prefixes, `:673`
  `plus_model=status[34:33]`, `:1110-1120` mask wiring,
  `:726/:733/:1396` `status[32]` Dandanator-only. No collision with crop bit
  1; encoded slots (`S0/S1/F4/F8/OK/O2/OGH/O[5:4]`) prevent
  `ioctl_index`/VDNUM renumbering.
- Follow-ups, low and non-blocking: **B6-1** `Amstrad.sv:103` `R[32]`
  mislabeled, no capability prefix — rename or Dandanator-presence bit;
  **B6-2** `:673` mask follows raw `status[34:33]` so classic media vanish
  before apply-reset — one sentence in `docs/b6-architecture-decision.md`.
  Neither invalidates mask correctness.
- Hardware limits retained: `d<n>` hide-vs-grey and `S`/`F` vs `O` rendering
  unprovable in-repo (MiSTer_Main `menu.cpp` not vendored); HPS path
  `Amstrad.sv:216` read-verified only; actual OSD rendering is validation, not
  review closure.
- B10: clause-equivalence already CLEAR. Follow-ups only: **B10-1** (medium)
  `Amstrad.sv:386-392` wrapper-side invalid-chunk retention
  (`boot_bank`/`boot_a[22:14]` unwritten) unpinned — fixture proves only
  `addr_valid=0`; bounded fix via a wrapper-side vector on the
  `sim/plus/sdram_cartridge_test.cpp:780-793` precedent. **B10-2** (low) `:332`
  instance shadows module name, rename to `rom_route`. **B10-3** (low)
  `:323-324` `page`/`combo` ownership comment. Do not reopen B10.

## 3. OSD sync-filter toggle — CLEAR (wiring only)

Scope: `355855a` (single-bit `~status[35]`) → `74882c7` (two-bit
`status[36:35]` Full/Live/Off). Tip: `Amstrad.sv:78`, `:1305`
`.sync_filter(status[36:35])`; `rtl/Amstrad_motherboard.v:65`, `:679`,
`:731-756`.

Verified: default `status[36:35]=0` = Full = bit-for-bit prior behaviour per
`:712-730` comment; the `status` map shows no `35/36` collision; encoding
migration handled (B7 caught the old literal-off reading as Full; benches now
name raw mode explicitly). 7+13 lines of default-preserving wiring; no focused
review existed, none needed beyond this pass. Live-blanking geometry belongs
to the excluded B1/classic reviewer. Retained scaler/hardware residual: Off
garbles SHAKER/DSC4 (no stable lock); Full/Live comparison and per-title
retest remain validation (`docs/hardware-evidence-2026-09-02.md`).

## 4. P10j primitive/model contract — NOT CLEAR (low, doc-only)

Exact-tip Opus review at `bf1e785` (`1248d06` CLOCK1, `cd56d66`
collision-mode) found no production defect; three low notes keep it
technically NOT CLEAR, and all three remain undocumented at `a98590a`:

- `rtl/plus/plus_sprite_ram.v:15-16,199-201`: behavioral same-port returns old
  data; M10K uses `NEW_DATA_NO_NBE_READ` port A/B with `OLD_DATA` mixed-port.
  Header documents read-first but not the unreachability invariant (SNA drain
  holds CPU reset) nor the exact-synthesis boundary. Lint stub
  `altsyncram_lint_stub.v` proves elaboration only.
- `rtl/plus/asic_regs.v:197-199,580`: `host_addr=eff_addr` (`sna_wr ?
  sna_addr : A`) assumes the same CPU-reset exclusion of concurrent
  SNA-write/CPU-read; no comment at the boundary.

No RTL/test change needed; no production defect asserted. Bounded remediation
is two small source comments (3–5 lines at the `plus_sprite_ram` boundary
stating the collision-unreachable invariant, the `NEW_DATA` vs behavioral
divergence, and the exact-fit authority `33392854459`; one invariant line at
`asic_regs.v:197` `eff_addr` selection), plus source verification — then
clear. Row stays OPEN LOW.

## 5. B7 dark-silicon audit — CLEAR (primary claim) with retained limits

Scope: `d942435` + `1bcf65a`; method `docs/plus/b7-dark-silicon-audit.md`;
fixture `sim/plus/p10_boot_test_top.v`; runner
`sim/plus/b7_dark_silicon_audit.cpp`.

- Synthesis guard: `p10_boot_test_top.v:737-759` comment +
  `` `ifdef B7_DARK_SILICON_MUTATION `` + `// synthesis translate_off/on`
  enclosing all `$value$plusargs`/`force`/`release`; only the B7 Makefile rule
  passes `-DB7_DARK_SILICON_MUTATION`; fixture absent from `files.qip`.
  Cannot reach Quartus.
- Anti-faking: runner requires nonzero decoder ID before sampling;
  unknown/inactive mutation fails; negative control (`asic_ga.MODE`
  unconnected) stays `0xea03a76a5520e7eb`; every Plus name changed in Plus
  mode (Group A 9/9) and unchanged in classic (Group C 9/9); every classic
  name unchanged in Plus (Group B 4/4) but moved in classic controls — the
  null result is meaningful, not vacuous.

Retained limits (validation/model-inference, not review debt): type-0/1
engines share mutated signature `0xa3a62ef6a7438799` (not independently
proven); classic baselines identical `0x4ace163975bf3441` for both CRTC types
(no type-divergent coverage). CPU bus/cycle/IRQ signature uses the
TV80/T80pa-wrapper surrogate, not production VHDL T80 (Verilator cannot
compile it); interrupt-ack/exact bus timing remain synthesis/hardware
boundaries.

## 6. Plus hardware-defect triage + production seam — CLEAR (source/test) with retained validation

Scope at `ea0e0bd` (`120e0cb,da6fe25,0b88998,360a3d4,0ced09d,00173fb`);
record `docs/plus/hardware-defect-triage-2026-09-01.md`.

- B13/legacy gate: `rtl/plus/plus_legacy_cart_gate.v:1-38` gates
  `dandanator_active = ~plus_mode & ~nce & loaded`, preserves `loaded` for
  classic return; `Amstrad.sv:1391-1399` single instantiation; `files.qip:36`
  single manifest owner via `0ced09d`. Lifecycle vector failed-first then
  passed. `rom_map` correctly left uncleared (classic MMU ROM-enable forced
  inactive in Plus; clearing would discard classic expansion state). Navy
  Seals retest requires the Dandanator-prerequisite sequence — retained as
  hardware, not review debt.
- Sprite X clip `0b88998`: `c_xneg=&SPR_X[9:8]` (raw ≥768),
  `c_xmag=(~X)+1`, `c_xleft=(hp==0)&c_xneg&(mag<wid)`; `s09` failed-first
  (`X=-256` leaked at `hp=768`). Source: Arnold V1.5 §2.1 + [KT]
  raw/`R0>64` repeat; literal `-64..639` vs implemented `-63..639` left
  explicit pending hardware. Model-inference limit: magnification+clip
  interaction beyond x1 lightly pinned.
- Display origin `360a3d4`: `H_ORIGIN_DOTS` (leaf 0, production 16),
  `HP_SEAM=0-16`; `m13` cross-module (real leaf + compositor) pins X=0/-8
  visible, X=640/767 border-masked at R1=40 and exposed at R1=50; SSCR[7]
  paired no-mask/mask proves the stored bit changes palette before opaque
  sprite — a disconnected SSCR cannot pass. `H_ORIGIN=16` derivation from
  `asic_video` delay retained as model assumption pending a hardware trace
  for the CRTC3 leak.
- FDC diagnostic `da6fe25`: production-clock TV80 path to real
  `rtl/u765/u765.sv`, first divergence honestly XFAIL (`fdc-payload-poll`,
  XPASS on full 512-B match); same-edge checks exclude reduced-bench
  decode/wired-AND; no speculative u765 change. Omitted full-top contributors
  (MF2/mouse/PlayCity), TV80-vs-VHDL-T80, and System-CPR/real-T80 trace
  explicitly retained. Test-integrity sound.
- Screenshots: Burnin' Rubber right-edge now user-reported fixed on
  `84e6969`+Live (`docs/hardware-evidence-2026-09-02.md`) without proven
  causality — regression target only; CRTC3 leak, Sonic/Copter/Dick-Tracy,
  and corruption each have distinct discriminators with no RTL change —
  correct.

Bounded follow-ups (optional, non-blocking): one-line `H_ORIGIN` provenance
comment; mag×clip vector if a primary source is found. Retained validation:
Navy-Seals prerequisite retest, CRTC3-leak trace, System-CPR +
real-T80/full-top closure, exact-tip Quartus/hardware, TV80-vs-VHDL-T80 and
mag/model limits. A CLEAR verdict is not hardware closure.

Published ACCC v1.11 unchanged; the author message is dated clarification
only. FDC remains shared classic/Plus work owned by the active recovery
session, not this review.
