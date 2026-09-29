# PA3 original-Plus follow-up cartridge

This bounded B24 batch tests terminal split capture with and without vertical
adjustment. It rebuilds the three temporary V4 variants from their recorded
source deltas, alongside the late-arm baseline and an R5=0 control. It changes
no production RTL. Emulator and production-CPU simulation results are diagnostic
evidence; original-Plus acceptance remains outstanding.

## Build and run

With `sjasmplus` on PATH:

```sh
python3 scripts/diagnostics/pa3_followup.py
```

Load `output_files/pa3-followup/cartridge/pa3-followup-start00.cpr` on the
original Plus and cold boot. It begins at case 1. Tap any key or joystick fire
to advance; case 5 wraps to case 1. Let each screen settle, then photograph the
whole screen including its case label, `HANDLER: RAN`, cyan border dash and
red/green measurement strip. Record the machine model and display/capture setup.
If `HANDLER: WAITING` persists, record that failure; an all-red strip alone is
not proof that the timed code ran.

All five cases are in the same CPR. `--start 0` through `--start 4` select a
cold-boot entry for repeatable emulator or simulation work; they do not change
the case order. Adjacent `.json`, `.lst` and `.sym` files record the build identity
and assembled program. The standard `plus_hw_probes.py` cartridge retains its
existing screens and behavior.

## Cases and predeclared outcomes

All cases use SSA0200, R4=36, R9=7 and SSCR0. Cases 1–4 use R5=16:
normal lines0–295, adjustment296–311, then frame0. Case 5 uses R5=0,
so line295 is the actual frame end. Frame0–7 means the first eight scanlines
after the origin, not the upper eight rows of an arbitrarily cropped photograph.

| Case | Enable target | Predicted adjustment | Predicted next frame |
|---|---|---|---|
| 1 Normal terminal late | 295/C48 | All red | 0–7 red |
| 2 Normal terminal early24 | 295/C24 | All 16 lines green | 0–7 red |
| 3 Last adjustment late | 311/C48 | Red | 0 red; 1–7 green |
| 4 Last adjustment early24 | 311/C24 | Red | 0 red; 1–7 green |
| 5 R5=0 terminal late control | 295/C48 | None | 0 red; 1–7 green |

The first four predictions come from the recorded
[V4 AmSpirit discriminators](asic-audit-probes-v4.md#pa3-amspirit-discriminator).
The R5=0 case controls the existing terminal-capture path. Hardware disagreement
is a new observation to investigate; do not change the prediction to match it.
Green frame0, missed handler evidence or another pattern should be recorded
explicitly. Interlace, live R5 writes, aliases and a broader ASIC audit are
outside this batch.

## Recovery provenance

A bounded search on 2026-09-29 found no original V4 CPR/ASM/PNG assets in the
main checkout, `local/task-archives`, or retained worktrees. The old managed
checkout `4a47/Amstrad_MiSTer` is absent. The archived session
`rollout-2026-09-28T00-51-20-01a0e547-c1aa-7572-83c5-8a2ef9533ba2.jsonl`
preserves artifact listings and these historical identities:

| Temporary V4 case | Historical CPR SHA256 |
|---|---|
| Normal terminal early24 | `c9c9b989353e363c00331d283f6668b15673b1f58c7701e32f1bd3e85d4c5e87` |
| Last adjustment early0 | `9719e353927e8860cf97eae953eda633f30cd526b5ab778f425c10ec28a3166e` |
| Last adjustment early24 | `2da347049eeaa6222551912648b65160021c5730b0e62ae58eb047c122491f91` |

The latter two were named `plus-hw-probes-start28.cpr` under
`amspirit-phase4/last-adjustment-early0/` and `...early24/`. These hashes identify
lost historical builds; the follow-up is a new reproducible package, not a
claim to have recovered identical bytes. Its timing deltas are enable minus24
NOPs with compensating padding before disable, and plus1024 NOPs for the
last-adjustment line.

## Execution evidence (2026-09-29)

Final default CPR SHA256:
`060a7f252d38679892b6b16cfaaf905d5d8881be6ab4930f97eaaadfc00d7543`.
The program occupies 12426 bytes before bank padding. The five `--start` builds
have distinct metadata identities because their initial case differs.

```sh
python3 scripts/diagnostics/pa3_followup_sim.py
```

This builds the existing production-T80 fixture and executes each case for 40
frames. The final evidence run reused that unchanged fixture with:

```sh
python3 scripts/diagnostics/pa3_followup_sim.py --no-build --out output_files/pa3-followup/rtl-final
```

Result: `pa3_followup_sim: PASS 5 production-T80 timing/visible-border cases`.
The traces confirm the table's enable coordinates, disable296/C10 for cases 1–2,
and disable frame0/C10 for cases 3–5, all at dot0. The second cyan border pulse
is C41–48 on the disable line, after active display ends at C40 and before
HSYNC starts at C49. The first experimental pulse is retained but is not the
visibility control. Full-width production-model images agree with the predicted
colour strips. These are observed implementation timings, not original ASIC
edge measurements.

The visible-marker check rejected the first build's pulse at C16–36, which was
hidden inside active display; it passes after the post-disable padding change.
Pixel inspection also caught a long row11 label crossing the reserved red
baseline span; the final short label ends before that span. These failures and
their raw outputs are retained under the ignored evidence root.

All five final CPRs were cold-booted and settled 150 frames in AmSpirit
Lite1.15.1/core2491682, model4 (6128 Plus), CRTC3, 128 KiB, monitor Off, colour,
with CRT effects disabled. `HANDLER: RAN` appears on each screen. All measurement
bands match the predeclared predictions across the full 640-pixel active span
x15–654:

| Case | AmSpirit screenshot rows, inclusive | Visible cyan pulse rows |
|---|---|---|
| 1 | Red240–271 and272–287 | 240–241 |
| 2 | Green240–271; red272–287 | 240–241 |
| 3–4 | Red240–273; green274–287 | 272–273 |
| 5 | Red254–255; green256–269 | 254–255 |

The pulse is visible at x659–767 in these cropped screenshots. Case 5 has a
296-line frame, so its image origin differs: do not reuse the R5=16 image row
mapping. Text repeats/wraps elsewhere because of the deliberate MA geometry;
the full case label below the measurement band is the readable reference.

Raw CPRs/listings, pre-run predictions, simulation traces/images, AmSpirit
case JSON, screenshots, state/SNA checkpoints and manifests are under
`output_files/pa3-followup/` (ignored). Final evidence is `rtl-final/` and
`amspirit-release/`; earlier directories retain the rejected display layouts.
AmSpirit case JSON can be rerun with:

```sh
python3 scripts/amspirit/amspirit.py run output_files/pa3-followup/normal-early24-case.json --out-dir output_files/pa3-followup/recheck-early24
```

Those case files record the absolute local CPR path and exact hash; update the
path if the evidence package is relocated. Screenshot coordinates describe this
emulator's crop and scaling, not a required original-monitor position.

Standard-cartridge preservation compared all 36 generated includes with the
base revision and rebuilt screen28 before/after: exact CPR SHA256
`18fbc9f299ffd5421d6af7639a5cb6d8d5e157606e76342cb80a3b61d85c77ec`.
The final repository gate, `python3 sim/select_tests.py --run`, exited 0 with
`select_tests: no simulation needed`. The focused production-T80 execution
above supplies the timing validation for this diagnostic-only change.

A single default CPR was also driven through 1→2→3→4→5→1 using three-frame
joystick-fire taps and 150-frame settling intervals. Every settled screenshot
is pixel-identical to its separate cold boot, including the final wrap to case 1.
The recipe, images and comparison record are in `amspirit-navigation/`.

On an original monitor, cropping or timing differences may shorten or hide the
cyan dash. Record that observation separately; `HANDLER: RAN` proves completion
at least once, while the dash only locates the post-disable phase. Neither
indicator alone measures the ASIC capture edge.

Cross-provider review: Opus5.5 medium approved the cartridge code/timing in
run `20260929T001545Z-37932-0310`; its stale-capture and missing-hash findings
were closed by final-evidence review `20260929T002630Z-46853-0c26`, which also
approved the timing-check script. Logs are retained under `reviews/` in the
ignored evidence root. Parent hashing independently confirmed all five CPR
identities; the reviewer confirmed matching metadata and byte equality.

The timing checker inspects one settled CRTC-origin frame. For cases3–5 its
frame0 disable/marker belongs to the preceding loop iteration, before that
frame's terminal enable. This verifies steady-state timing, not first-iteration
enable-to-disable pairing. Cold-boot and navigation captures separately prove
the settled display/handler outcomes. The first-iteration edge and exact original
ASIC sampling mechanism remain outside this acceptance claim.
