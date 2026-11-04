# SHAKER C E/P: display and status recovery during stuck R5 adjustment — 2026-11-03

## Position

The classic type-1 engine now supplies one R6-origin event to reopen the
wrapper's vertical display gates and clear the sampled BORDER R6/status bit 5
condition at a Q20 R4 reset, even when adjustment remains active. This repairs
the missing display route behind blank SHAKER C E (`02C9`) and P (`02CB`) pages
identified in the [CRTC 1 sweep](shaker-crtc1-sweep-2026-11-03.md).

The focused pin suite passes with a failure-first counter/display/status
vector, as does the directly affected historical-engine discriminator. The
combined selection gate passes all 43 selected benches. Fresh Opus review cleared the final display/status change. Hosted full
synthesis of combined diagnostic commit `bedb1c5` passed; its new
RBF/photo comparison is running. A classic-only build at `8dde1e7` is
separately queued so the repair can be used independently of the ASIC
experiment. The classic worker used
neither MiSTer nor AmSpirit.

## Evidence and derivation

- French ACCC v1.11 §11.3.2 p.87 describes R5 becoming zero during adjustment:
  C5 keeps looping and adjustment stays active while C4 continues its R4
  comparison. The author's [Q20 answer](../../classic/accc-author-response-round2-2026-08-31.md#q20--n2--type-1-adjustment-after-r5-becomes-zero-1132)
  confirms the ordinary C4/R4 reset at the C9/R9 row end. The engine already
  implements that reset separately from a frame end.
- French §§18.1 and 18.2.1 p.189 give `C4=C9=C0=0` as the display restoration
  condition. §18.2.3 p.190 describes the type-1 R6 border and its priority.
  Together with Q20 these support display restoration at the R4 reset while
  R5 adjustment remains active. They do not establish display restoration at
  a seven-bit C4 carry wrap; that separate evidence gap is left unchanged.
- French §21.3.3 p.247 defines status bit 5 from the BORDER R6 condition,
  sampled at C0=R0. Its rendered chronograms distinguish the origin's false
  condition (`C4=C9=C0=0`) from the R6-row true condition
  (`C4=R6, C9=C0=0`). Origin-false has priority even when R6=0. Status is
  therefore not a literal copy of DE: R6=0 may keep display border while
  status clears at the origin. The special R6=0 border with C4>0 also does
  not independently set status.
- The Logon System original-CPC photographs of
  [E](https://shaker.logonsystem.eu/images/cpc/CPC/CE_CRTC1_A.webp) and
  [P](https://shaker.logonsystem.eu/images/cpc/CPC/CP_CRTC1_A.webp) both visibly
  contain result text. The saved authentic-code replay diagnosis establishes
  that these pages leave the model in stuck adjustment. Their visible text is
  hardware evidence for display recovery; the internal reset event is derived
  from that outcome and the French counter-origin rule.

The saved pdf-inspector report and primary French Markdown were read from the
main checkout's `docs/classic/extract/inspector-v1.11-fr/`. The report is
`native_partial`, with fallback pages 40–41, 182–183 and 280; none are needed
here. French pp.87, 190 and 247 were rendered and inspected locally, including
the status chronograms. Source PDFs and photographs remain untracked. The
photographs were inspected from the saved replay's `CE_02C9_A.png` and
`CP_02CB_A.png`.

## Change and proof

Originally the wrapper rearmed `vde` and `vde_r` only on `frame_new`.
The Q20 R4-reset origin deliberately does not assert `frame_new`, so once C4
crossed R6, both vertical gates remained closed indefinitely. The type-1
engine's sticky BORDER R6 condition and status latch likewise cleared only
on `frame_new`.

The engine now exports `r6_origin = frame_new_w |
crtc1_stuck_r5_row_reset_event`. The wrapper consumes the selected type-1
event to reopen display, and the engine uses it to clear its BORDER R6 and
sampled status latches. The event names the existing R4-reset route, excluding
a plain C4 127→0 carry wrap. No counter, parity, video-pointer, or
adjustment-release logic changed.

The wrapper's subsequent R6 comparison retains its display priority. The
engine retains origin-false priority for status, including R6=0. A new trailing
engine output required a test-only adapter in `sim/Makefile`: the historical
engine generated for `crtc-t80-old-test` gets that output tied to zero, leaving
its original frame-origin display path and historical rules intact.

Required vector `t08r_type1_r5_zero_row_origin_reopens_display` reuses the Q20
fixture. It reaches C4=C9=0, C0=1, C5=8 with adjustment still active, and checks
DE high/status `00`. A live unequal R6=2 write must preserve display; reaching
C4=2 must blank display and set sampled status `20`. The next Q20 reset must
restore DE and status `00`. With R4=10 and R9=2, origins are 33 scanlines
apart, so C5 becomes `(8+33) mod 32 = 9`. The final control writes R6=0 at
nCLKEN, observes border/status `20` after the line-end sample, then verifies
that the following Q20 origin retains border while status clears to `00`.
The repeated-origin check isolates recovery from the fixture's preceding C4
carry wrap. This pins shared-counter, R6-write and display/status interaction.

Exact focused command:

```sh
make -C sim crtc-test
```

| Failure-first state | Final result |
|---|---|
| Original RTL + initial DE vector | `Summary: 238 passed, 0 xfailed, 0 xpassed, 1 failed` |
| Display candidate + added status assertions | `Summary: 238 passed, 0 xfailed, 0 xpassed, 1 failed` |
| Corrected shared R6-origin event + final vector | `Summary: 239 passed, 0 xfailed, 0 xpassed, 0 failed` |

The original failure was expected DE high, actual 0, at character 27905,
tick 4/16. The review discriminator then failed at the same point with expected
status `00`, actual `20`. Exact logs and preserved failing VCDs are in ignored
`local/overnight-20261103/ep/`: `crtc-before.log`, `t08r-before.vcd`,
`crtc-review-before.log`, `t08r-review-before.vcd`, and `crtc-review-after.log`.

Direct interface compatibility was checked with:

```sh
make -C sim crtc-t80-old-test
```

Its final result was `PASS: Old pre-B8 engine correctly failed all 4 B8
side-effect checks`, with `1 passed, 4 failed, 1 xfailed, 0 xpassed (total 6)`
inside that intentional negative control. Output: `ep/crtc-old-review.log`.

The final deterministic soak command was:

```sh
make -C sim soak SOAK_EXPECT=0x7667d1a961e810c6
```

With unchanged seed `0xaccc5eed20260822`, stimulus and sampled projection, it
completed `2845088 characters, 2845088 CLKEN samples` and produced
`soak hash: 0x743ca9b795e30eaf`. The old candidate hash check mismatched
(make exit 2), reflecting the intended narrowing and status correction. The
accepted final hash replaces the provisional uncommitted candidate row in
[the soak history](../../../sim/soak-golden-history.md).
Output: `ep/soak-review.log`; the initial candidate's soak is preserved in
`ep/soak.log`.

The sole final combined selection command is:

```sh
python3 sim/select_tests.py --run
```

It passed against the final source changes, with exact final result:

```text
select_tests: PASS 43 benches: test, crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, b18-save-admission-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Output is preserved in `local/overnight-20261103/ep/select-final.log`.
`git diff --check` passed. No code changed after this gate.

## Hosted artifact and device handoff

User-authorized hosted full run
[41739354589](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41739354589)
succeeded at exact SHA `bedb1c5e20e1d720da4b75f5f735c987776e2835`.
Quartus 17.0.2 Build 602 Lite, clean full build, 0 errors and 0 critical
warnings. The timing checker reports setup minimum 0.137 ns and hold
minimum 0.241 ns across seven clocks, with zero TNS. Selected 43 benches,
production T80 and the required gate passed on that SHA. Existing ignored
fast-I/O/fitter and other warnings are retained in the artifact handoff;
no unchanged-warning comparison is claimed.

`Amstrad_20261103_bedb1c5.rbf` is 3,124,836 bytes, SHA-256
`02ed2ae66f6648d72c86cbaa9bbbe393d36b0d5656ab05e17cb32931b17321c7`.
The same digest was verified after copy to MiSTer's `_Computer` directory.
All reports and build metadata are saved in ignored
`local/overnight-20261103/build/`. The ASIC candidate in this combined
artifact is **diagnostic and unaccepted**; it does not affect the selected
classic path. This artifact must not be published as an accepted Plus fix.

The full C-1 history on the new disk completed in
`local/overnight-20261103/ep/c1-device-bedb1c5-r2/`: 165 commands,
80 captures, zero capture failures and152 SSM records. Its first attempt
stopped before execution because the parent passed `filename=hash` instead
of a bare media digest; the corrected run has its own directory and keeps
that failed preflight evidence.

Classic-only full build
[41742364942](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41742364942)
targets exact `8dde1e7e432cf344c44bc2937d9065050ecbd56f` on
`codex/classic-ep-20261103`. It is in progress; no second build of the
combined candidate was dispatched.

## Combined-artifact hardware observations

E (`02C9`) and its second page (`02CA`) are now readable, as is P (`02CB`).
E still prints `#0060` against the embedded/photo `#0080` expectation. P
prints C4=`#26` as expected. This establishes the missing-display repair
on this artifact and full recorded history, not every numerical result.

The later `02D3` capture now reads **`#0240` as expected**, whereas the prior
full C-1f capture on5fba168 reads `#0840 WRONG (EXP:#0240)`. This is a new
observed result, not yet a proven causal repair: R6 display recovery changes
capture duration and therefore subsequent keyboard/entry phase. The numeric
path polls PPI VSYNC, so it cannot be attributed directly to the status-bit
correction. Repeat the full history on the classic-only artifact before
closing its history/phase sensitivity.

Against the prior80capture C-1f walk, **66 PNGs are byte-identical**. The
remaining14 are E/P's three restored pages, the `02D3` text, and ten narrow
line bands (1–5 rows) at022D/022F/0230/0233/0234/0285/0287/0297/0298/0299.
Parent inspection found raster-pattern/colour-band phase differences in
those ten; no new numerical text differences there. A same-RBF phase
control was not run, so they are not silently counted as equal. The
original-CPC join has77reference pairs and three pages without a cached
reference; numeric pixel metrics are not verdicts. Comparisons and narrow
band contact sheet: `local/overnight-20261103/ep/c1-bedb-vs-old/` and
`c1-bedb-vs-cpc/`.

## Remaining boundaries

The authentic E/P/`02D3` measurements poll PPI VSYNC bit 0 at `F5xx`, not
CRTC status bit 5: E helpers `6F3F`/`7105` read at `6F69`/`7132`, P reads
at `6A21`/`6ABF`, and S's measurement reads at `5B0E`. The status reader
`A46D` belongs to startup classification; the retained replay seeds `BF03=1`.
Therefore the status correction has no direct route to E's `#0060`/`#0080`
or `02D3` count with type-1 classification fixed. Those numeric residuals
must not be claimed repaired by this display/status change.

- Hardware acceptance is a newly built RBF showing readable E/P pages against
  the linked photographs. This focused vector proves the display/status route,
  not the complete SHAKER result screen or numeric values.
- R5=0 write visibility on the adjustment decision edge, history-dependent
  `02D3`, and E's `CPU TO NEW FRAME` `#0060` versus photographed `#0080` remain
  separate unresolved questions. No new release rule is inferred.
- Display behaviour at a seven-bit C4 carry wrap remains an explicit source
  and hardware evidence gap; this patch only adds the Q20 R4-reset route.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by
Longshot (CC BY-NC-ND 4.0).
