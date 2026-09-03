# Test-policy audit — 2026-09-03

Sole-writer audit (no delegation) of the repository test inventory against the
updated global testing policy. Committed as
`c04991ea9611210b1300aca729e78d297ee0617c` on `codex/testing-policy-audit`.
No pushes, CI dispatches, production RTL changes, or B8 architecture review
in this pass.

## 1. Exact base and policy source

- Base `08c1596` ("docs: finalize reviewed Plus
  READY handoff"), branch `codex/testing-policy-audit`; V1 fix committed as
  `c04991ea9611210b1300aca729e78d297ee0617c`. Tree was clean at Audit
  start (only untracked `.coord-inputs/`, which stays untracked).
- Installed policy: `.coord-inputs/global-testing-policy.md`
  (source-file SHA-256
  `884fe787c1c43002aa69d859ef83b4cb0e903021dfa498bb38f31abd6ecae6b8` for
  `/Users/renaudg/.codex/AGENTS.md`, read 2026-09-03; the local snapshot
  file itself is SHA-256
  `34d260127611e266763c9b2720aa95d29368f32b1bc8ba69703b7cabaf467ecd`;
  Claude global policy carries the same text). Operative rule: **a test earns its place
  only if it could fail for a reason the author did not already know**;
  rule-mirroring tests are documentation with a `make` target; prefer
  cross-module, degenerate-case, and not-yet-implemented-rule tests; every
  behaviour change still lands with a focused deterministic test.
- Local policy read: `AGENTS.md` (test-vector discipline §"Test vector
  discipline" already carries the same rule), `docs/backlog.md` B9
  (test-suite sweep; first slice done 2026-09-01), recent reviews
  `docs/accuracy/classic-review-2026-09-03.md`,
  `docs/plus/plus-review-2026-09-03.md`, `docs/review-debt.md`,
  `docs/session-handoff-2026-09-03.md`.
- Prepared Plus B3 candidate `bb77075` inspected read-only via
  `git show bb77075:...` (§7). Not in this checkout; untouched.

Judgement standard applied: useful failure modes, independence of expected
values from the implementation under test, real production coupling,
mutation/history evidence, regression value, duplication. A test is **not**
useless merely because it passes or examines a leaf. Independently sourced
ACCC/Arnold/`test.dsk` fixtures, failure-first vectors, accuracy rules,
expected hardware outcomes, golden hashes, and XFAIL/XPASS protection are
preserved untouched.

## 2. Inventory (suite families)

### Classic CRTC simulation (`sim/`, default gate)

| Family | Location | Size / count | Oracle |
|---|---|---|---|
| CRTC pin vectors | `sim/sim_main.cpp` | 192 registered, 0 XFAIL-flagged | ACCC (per-vector section+page cite) |
| Soak | same binary `--soak` | golden `0x2263c9fc44af4ee7` | change-detector, not oracle |
| crt_filter seam | `sim/crt_filter_blank_test.cpp` + `_top.v` | 9 cases | B1 fallback/watchdog contract, degenerate cadences |
| GA R2.JIT | `rtl/GA40010/r2jit_tb.cpp` | directed | ACCC §9.3.4 (F20: 4/3-px starts, fixed edges) |
| GA legacy bench | `rtl/GA40010/ga40010_tb.cpp` + `cpcram.bin` | upstream-inherited | real RAM snapshot; **not** run by default `test` target (only `all`/build) |
| FDC | `rtl/u765/u765_tb.cpp` + `test.dsk` | 6 cases | reset/mount/read seams + real EDSK image |

### Plus simulation (`sim/plus/`, 28 run-steps in default `test`)

Leaves (single production module): `asic_unlock` (13-byte Arnold sequence +
reset/lock lifecycle), `asic_video` (~2970 lines, CRTC3, ACCC-cited),
`plus_model_select` (4-row decode — §5 VIOLATION), `dandanator_loader_bounds`
(bounds+gating, 9 cases), `rom_loader_route` (directed boot.rom routes +
exhaustive legacy equivalence), `plus_legacy_cart_gate` (B13 lifecycle),
`plus_cartridge_memory`, `sdram_cartridge`, `plus_cpr_parser`, `plus_mmu`,
`asic_regs` (exhaustive page/mirror/mask/open-bus, asic-reference.md),
`plus_sprite_ram` (P10j storage/synthesis contract), `asic_pri` (exact-cycle
raster INT), `asic_sprites` (KT formulas + MAME corroboration), `asic_dma`
(Arnold V §2.6/§2.7), `plus_p8` (PPI quirks, SNA CPC+ chunks, FDC decode),
`b6_menu_mask` (decoder→mask contract through production modules),
`plus_cartridge_memory_min` (same source, `CLEAR_BYTES=1` degenerate).

Integration / differential: `p0_boot` (CPR→memory→MMU seams, malformed/
reset/arbitration), `asic_ga_timing_diff` (lockstep vs netlist-derived
ga40010 + directed Plus-only payload), `p1_pixel_phase` (Plus video vs
classic CRTC+ga40010 oracle slice), `p1_mobo_bench` (motherboard bus/INT
path), `p4_sprites_regs` (regs↔sprite-engine seam),
`p10_boot_test.cpp` (1027 lines here; real-T80 boot + 32-read VRAM proof),
`b7_dark_silicon_audit` (mutation matrix: 9 Plus-active changed,
classic-in-Plus unchanged, classic controls changed), `p10_input`,
`p10_dma_ppi` (slice concurrency), `p10_dma_mobo` (full-motherboard
concurrency).

### Build / CI gates

- `sim/Makefile`: `build`/`test`/`lint`/`soak`; `test` runs classic +
  crt_filter + plus + GA40010 + u765.
- `sim/plus/Makefile`: 27 binaries + `motherboard-lint` (runs first in
  `lint`; the B9-duplicate second recipe is already gone).
- `.github/workflows/build.yml`: `make -C sim clean test lint`, then pinned
  Quartus; `scripts/ci/test-*.sh` helper tests are CI-gated.
- `docs/ci-testing-policy.md` governs synthesis tiers (not re-audited).

### Sampled / deferred scope (transparent)

- `sim/sim_main.cpp`: registry (192/0-XFAIL verified by script), XFAIL/XPASS
  runner, and sampled vectors (reset/readback-table t01-family, R7/HCC
  fixture) inspected; not all 192 vectors line-read.
- `asic_video_test.cpp`: header discipline, timing harness, and citation
  pattern inspected; not all ~2970 lines line-read.
- `asic_sprites`/`asic_dma`/`plus_p8`/`p10_*`: headers, oracle cites, and
  test lists inspected; bodies sampled.
- B8 architecture, hardware/CI closure, and Quartus inference are out of
  scope by brief. No local greens claimed as hardware/CI closure (§8).

## 3. COMPLIANT examples (keep; rationale)

1. **F18 readback matrix** (`test_register_readback_table`): type-0/1
   readable-register table with width truncation (R12/R14 6-bit), R31
   type-1 `0xff`, and modulo-32 aliases — expectations from ACCC §21.2.2,
   cross-type in one fixture. Could fail on bus-decode, width-mask, or
   alias regressions the author did not predict. Keep.
2. **crt_filter nine-case seam** (`test_*expiry*`, `*missing_sync*`,
   `*stuck_high*`, `*masked_retrigger*`, `*production_mode_selector*`):
   degenerate cadences + production mux. History: exact-expiry and
   stuck-high watchdog repairs followed these cases. Failure-first
   character retained. Keep.
3. **`asic_ga_timing_diff` d01–d04 + r01–r03**: lockstep replica vs
   **netlist-derived** ga40010 (highest-trust oracle in repo) on
   randomized-but-seeded traffic, plus directed Plus-only payload from the
   published GA port description. Independent oracle + cross-module seam.
   Keep.
4. **u765 EDSK mount** (`test_mount_recognizes_edsk` over real `test.dsk`):
   independently sourced image, not author-generated. The 6 stashed
   pre-edge tests (`0fe18a4…`) stay unaccepted — residual, not cleanup.
   Keep.
5. **`rom_loader_route` directed cases**: 10-chunk `boot.rom` bundle map,
   index-7 forced bank-2 route, generic/auto routes, and non-ROM rejection
   are hardcoded from the bundle artifact, not from RTL. The exhaustive
   C++ `legacy_route` equivalence was verified by history to be a
   transcription of the **pre-extraction** `Amstrad.sv` inline decoder
   (commit `44eb1b7` diff, §6 evidence): a genuine refactor
   characterization with transcription-slip failure modes, honestly
   labelled. Keep both.
6. **Leaf↔integration pairs** (`asic_regs`↔`p4_sprites_regs`,
   `asic_dma`↔`p10_dma_ppi`↔`p10_dma_mobo`, `plus_sprite_ram`↔`a01`):
   distinct contracts per layer (decode path vs seam vs storage/synthesis
   vs full-motherboard wiring). B9 explicitly retains these. Keep.
7. **`plus_cartridge_memory_min`** (`CLEAR_BYTES=1` rebuild of the same
   source): degenerate-case coverage the policy explicitly prefers. Keep.

## 4. VIOLATION with bounded fix (implemented §6)

**V1 — `plus_model_select` leaf binary duplicates the B6 integration
stimulus through the same production module.**
`sim/plus/plus_model_select_test.cpp` asserts the 4-row capability table
row-for-row against `rtl/plus/plus_model_select.v:29-46` — the exact
"table mirroring the RTL case statement" pattern the B9 pass already
removed from `plus_p8_test.cpp` (reviewer-blessed, `plus-review-2026-09-03`
§1). The B6 fixture (`b6_menu_mask_test_top.v`) instantiates the **same
production decoder** and drives the **same 4 rows**; its mask contract
(`0x38/0x04/0x14/0x24`, `+0x02` with crop) distinguishes every model, so
any `plus_mode`/`has_fdc`/`has_tape` decode error fails B6 too. The leaf's
only unique coverage is `ram_128k`, which B6 exposes but never checks.
Fix: delete the leaf binary + Makefile test-binary/runner rules (retaining
the standalone `plus_model_select` clean-`-Wall` leaf lint recipe adjacent
to `plus_menu_capability_mask`), add per-model `ram_128k`
checks (machine facts: GX4000/464+ 64K → 0, 6128+ 128K → 1, Off → 0
fail-closed) to the B6 test. No production, docs-policy, hash, or XFAIL
change; gap-map Model-capabilities row target is now `b6_menu_mask`
(`docs/plus/asic-documentation-gap-map.md`) and stays owned via the B6
fixture.

No other concrete violation met the bar for bounded removal/replacement:
leaf tests that pin otherwise-unchecked outputs (`dandanator_loader_bounds`,
`plus_legacy_cart_gate`, `ram_128k` aspect above) earn their place, and
differential fixtures with honest caveats (§5) are not mirrors.

## 5. UNCERTAIN / residuals (no change; owners noted)

1. **U1 — `p1_video_test_top.v` `[production copy]` blocks.** Fixture
   copies two motherboard always-blocks verbatim ("must stay textually in
   sync"). The asserted contract (Plus pixel phase vs classic
   CRTC+ga40010 oracle slice) is differential against an independent
   oracle, so the test is not a mirror — but a motherboard edit that
   forgets the copy silently voids the premise. No bounded fix available
   (instantiating the full motherboard here duplicates `p1_mobo_bench`/
   P10). Residual: consider a textual-sync CI grep or a comment pointing
   at the owning bench. Owner: Plus stream.
2. **U2 — MAME as secondary oracle** (`asic_sprites_test.cpp`: Y-compare,
   render order corroborated against MAME). Primary is KT measured
   formulas; MAME is corroboration only, consistent with B5's ranking.
   No action; do not promote MAME to sole oracle anywhere.
3. **U3 — `rom_loader_route` non-ROM-index behaviour.** Old inline code
   ran the `else if (ioctl_index)` assignment branch for indices the new
   code gates with `rom_download`; test and RTL agree with each other,
   directed cases pin `rom_active=false`. Whether old≡new there is a
   don't-care or a preserved quirk was not decided. No action; flagged
   for the owning stream if that path is ever touched.
4. **U4 — GA40010 legacy bench** (`ga40010_tb.cpp`) is built but not run
   by the default gate (`test:` runs only `r2jit`). Upstream-inherited;
   leaving as-is (running it would add an unowned gate, deleting it
   would drop the only `cpcram.bin` consumer). No action.

## 6. Bounded fix committed (Plus group; `c04991ea`)

- `sim/plus/b6_menu_mask_test.cpp`: `ModelCase` gains `ram_128k`;
  `{0,0,1,0}` per machine facts + decoder doc header; checked per row for
  both crop states (top already exposed the tap — no `.v` change).
- `sim/plus/Makefile`: removed `MODEL_TOP/RTL/OBJ_DIR/SRC/BIN`, the
  `TEST_BINS` entry, the `./$(MODEL_TEST_BIN)` recipe line, and the test
  binary build rule. The standalone `plus_model_select` clean-`-Wall`
  leaf lint recipe is retained adjacent to `plus_menu_capability_mask`;
  only the test-binary and test-runner recipes are gone. Nothing else
  references the binary (checked Makefiles, CI, docs).
  Repair history: first Gemini review was NOT CLEAR solely on the dropped
  leaf lint (B1); repaired with the retained recipe. Follow-up Gemini 3.8
  Flash high review, run `20260903T080044Z-3321-ac9c`
  (`.coord-inputs/gemini-testing-policy-review-final.log`), verdict CLEAR,
  B1 closed. Rationale: the B6 functional build elaborates the decoder
  inside a fixture top with integration flags, so only the standalone
  `--lint-only -Wall` leaf recipe checks the decoder alone strictly;
  dropping it would silently lose the B1 lint contract the functional
  pass cannot replace.
- Deleted `sim/plus/plus_model_select_test.cpp` (69 lines; read before
  delete; sole writer).
- Retained contract: all four decode rows still driven through the
  production decoder; `plus_mode`/`has_fdc`/`has_tape` via mask bits,
  `ram_128k` via the new checks. Independent evidence: machine memory
  sizes + mask contract, not RTL text.
- Before/after: plus run-steps 28→27; per-row checks 4 mask→4 mask +
  `ram_128k`; leaf's 16 mirrored checks gone, 0 unique contracts lost.

## 7. B3 candidate `bb77075` — report-only

`git show bb77075 --stat`: `sim/plus/p10_boot_test.cpp` +1068 net (1027→
2095 lines), `p10_boot_test_top.v` +2 (`dbg_cpr_load_abort` tap),
`docs/plus/b3-capture-2026-09-03.md` + `b3-capture-review-2026-09-03.md`
(CLEAR by Gemini 3.8 Flash high, one non-blocking symlink-assertion note).
Sampled the C++ (constants, CPR builder, abort latching, CLI bounds):
fail-closed `--capture-cpr` (16-frame/16M-tick/32MiB-CPR/2GiB-output
budgets, `O_CREAT|O_EXCL|O_NOFOLLOW`), steady-state self-equality over two
frames with nonzero FNV-1a64, abort-before-open. Provisional read: the
bounds/CLI tests pin degenerate cases and the determinism check can fail
for unknown fixture reasons — consistent with the policy — but no verdict
is rendered here and nothing was reset or edited on that branch. The
`3db81d0` P10j comment-only notes ride the same branch (also READY,
pending authorized integration).

## 8. Gates and evidence

- Focused: `make -C sim/plus b6-menu-mask` exit 0
  ("PASS: B6 menu mask follows model and media capabilities").
- Focused strict leaf lint: `verilator --lint-only --language 1364-2001
  --top-module plus_model_select -Wall rtl/plus/plus_model_select.v`
  exit 0 (retained recipe adjacent to `plus_menu_capability_mask`; also
  verified inside the full `make -C sim lint` log).
- Required full gates on the final tree, coordinator reruns with actual
  exits: `make -C sim` exit 0
  (`.coord-inputs/parent-final-sim.log`: classic `Summary: 192 passed, 0
  xfailed, 0 xpassed, 0 failed`; all 27 remaining plus run-steps incl. B6
  and b7 audit; GA40010 r2jit; u765 `Summary: 6 passed, 0 failed`),
  `make -C sim lint` exit 0
  (`.coord-inputs/parent-final-lint.log`; strict decoder
  `--lint-only --top-module plus_model_select -Wall` present and clean).
  Soak hash untouched by construction (no RTL/counter/synthesis-manifest
  (`files.qip`) change; the `sim/plus/Makefile` build manifest did change
  to remove the redundant test binary while retaining leaf lint); soak
  not re-minted, XFAIL registry (0 flagged) untouched.
- Independent review is CLEAR: Gemini 3.8 Flash high follow-up, run
  `20260903T080044Z-3321-ac9c`
  (`.coord-inputs/gemini-testing-policy-review-final.log`), B1 repaired.
  Local greens are not hardware/CI closure; no integration, push,
  hardware, or B3/B8 closure claimed. Sampling limits in §2 and U1–U4
  stand; exhaustive policy compliance is not implied.
- Residuals for owner: U1–U4 above; stashed u765 pre-edge
  discriminator (`0fe18a4…`); B3/P10j integration (`bb77075`) awaiting
  authorization; private assets and unrelated work untouched.

## 9. Prioritized findings (summary)

1. **Done:** V1 leaf consolidation committed (`c04991ea`) — Plus group;
   Gemini follow-up CLEAR, B1 repaired; parent `make -C sim` / `lint`
   reruns exit 0.
2. **Next (Plus owner):** U1 production-copy sync guard (cheap grep or
   cross-reference; not this pass).
3. **Shared/FDC owner:** stashed u765 pre-edge acceptance per the
   next-session queue; FDC acceptance stays shared with classic AMSDOS.
4. **Process:** the suite is in good shape — oracles are documentary and
   independent (ACCC/Arnold/KT/EDSK/netlist), differentials pin seams,
   and the B9 precedent (dedup + archive + no-review-on-docs) is being
   followed. The main risk is fixture-copy drift (U1), not bloat.
