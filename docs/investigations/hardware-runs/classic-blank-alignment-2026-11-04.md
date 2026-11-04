# Classic blank alignment — 2026-11-04

Status: reviewed implementation with bounded hardware acceptance of DSC4's missing credit colons on CRTC0/1. The reported CRTC1 bottom row is a reference-crop coverage difference; its viewport policy remains separate.

The production T80/motherboard replay showed correct colon VRAM bytes reaching the serializer but disappearing under blanking. The previous fast GA vector wrote R2 at a later bit-banged phase and checked only relative timing. This change aligns blanking with displayed pixels and corrects the qualified JIT onset without moving the ordinary raw trailing edge or MODE_SYNC.

## Source and acceptance boundary

French ACCC v1.11 §14.7.1 pp143–144 anchors the byte being displayed at C0=R2 to character R2−1. Rendered §9.3.4 chronograms pp56–57 specify these zero-based Pixel-M2 indices for R3=2:

| CRTC | Normal blank onset | JIT onset | Reactivation | Mode switch |
|---|---:|---:|---:|---:|
| Type0 | 4 | 8 (half of7 allowed) | 36 | 37 |
| Type1 | 5 | 8 (half of7 allowed) | 37 | 37 |

Thus visible normal blanking lasts32M2 for either type; JIT lasts28M2 on type0 and29M2 on type1. The Type0 diagram retains one old-mode M2 after reactivation. That mixed-mode behavior remains a named pre-existing failure; this change does not claim complete GA compliance.

The new two-register blank path is an abstract pixel-alignment model. It is not claimed to reproduce Gerald's transistor-level signal path. Source authority applies to displayed output, not to an invented physical propagation delay.

## Implementation

`rtl/GA40010/video.sv` samples FORCE_BLANK through two cen_16 registers. The synchronous RGB decoder consumes the old second register, giving three effective sample edges for assertion and release. The stages start blanked. Serializer, palette, mode latch and MODE_SYNC are unchanged. This delay applies to GLOBAL FORCE_BLANK, including vertical assertion and release: an input transition reaches RGB after the third sampling edge, about8–12 master clocks depending on phase. The stages are not reset by SNA load. Active vertical-edge acceptance and snapshot/cold-fill behavior still require review.

`rtl/CRTC.v` adds an eight-master-clock countdown only after a qualified R2.JIT comparator write. Ordinary phase constants, the JIT trailing phase and HSYNC_RESTART logic retain their timing. One lifecycle behavior also changes: SNA load now cancels an ordinary pending Type1 start, alongside cancelling the delayed JIT event; previously that ordinary pending state was cleared only on a switch to Type0. Reset, SNA load, a live type replacement, or expiration of the comparator context cancel the delayed event; SNA/type cancellation has highest assignment priority, including the intended publication edge. No separate internal/public HSYNC pulse exists.

## Retained proof

The existing `ga40010-test` owns the cross-module byte-to-RGB contract. It uses six distinct physical bytes B6,6D,E3,9C,5B,A5, with RAM_DIN supplied from MA/RA at the E0 even-byte and03 odd-byte video captures. Fixed Mode1/Mode2 controls must independently match the complete192-master-clock decoded pattern and measure the one-M2 Mode2 advance. A complete configured frame retires the old row pitch before capture.

The R2 strobe lands at preedge CRTC phase8/S1F. This matches the frozen real production-T80/READY trace: address BD03 appears at tick824, IORQ/WR assert at840 and release at928; the level lasts88 master clocks, including52 READY-low samples. The fast fixture reproduces acceptance phase and held level, but does not execute a CPU. Its predecessor waited for C0 and then performed a full T1, accepting at phase21. Separate real-T80 evidence remains the transport authority.

The primary retained assertions compare every master-clock interior of all six displayed bytes against the source blank window and the independently decoded unblanked control. They do not infer expected visibility from HSYNC or mirror the new registers. Existing +3/+4-M2 relative assertions remain unchanged. R3=1 controls extrapolate the one-character R3-width step from the drawn R3=2 chronograms; those exact short-pulse windows are not directly drawn on pp56–57. They protect this shorter-pulse interaction; Type1 Mode1→2 checks the actual distinct new-mode RGB slot at index37. Type0 `type0_mixed_mode_old_pixel` is XFAIL with a specific pen0 signature and fails on XPASS or a changed failure signature. Lifecycle controls cancel both types' pending JIT at initial and final countdown age under reset, snapshot and live type changes.

All eight source byte windows failed before RTL edits: normal19 wrong master samples and JIT26 per case. They pass after the change with zero mismatches. The final held-bus fixture also reproduces these failures when compiled against the frozen prior CRTC/video sources. Evidence is under ignored `local/overnight-20261103/classic/blank-production/` (`fail-before.log`, `fail-before-final-fixture.log`, `retained-final-focused.log`).

## Measured behavior and remaining failure

| Visible blank width, master clocks | Type0 normal/JIT | Type1 normal/JIT |
|---|---:|---:|
| R3=2 | 128 /112 | 128 /116 |
| R3=1 | 64 /48 | 64 /52 |

The frozen actual-T80 private combined proof compares eighteen matched cases before/after: raw ordinary edges and every trailing edge, mode/data pipeline, GA-shaped HS/VS, IRQ/counts, raw-fall R52/V26 event tick, restart and CPU/READY traces are unchanged; only RGB and the qualified JIT leading interval differ. Normal raw R3=2 width remains128 clocks. JIT raw widths become111(type0)/115(type1); sampled RGB widths are112/116. Raw width and visible blanking must not be conflated.

Type0 mode1→2 still produces new-mode pen0/RGB21 at index36; the fixed old-mode control requires pen1/RGB1 across that four-clock slot. The fresh mode decode reaches RGB at index35, partly hidden by blanking. Both baseline and candidate fail the source's old-mode extra-M2 requirement. Type1's distinct new-mode RGB at index37 remains correct. MODE_SYNC is deliberately not retimed here.

Focused validation: `make -C sim ga40010-test crtc-test` passes (CRTC239 passed); the final selection covers subsequent fixture-only changes. `make -C sim classic-irq-phase-test` passes all8 real-T80 NOP/HALT/RET-NC cases: Type0 HS−N=−7/IRQ−N=−6 and Type1 −3/−2, retaining the expected interrupt acceptance. The soak seed/stimulus/projection are unchanged; intended JIT raw-output/lifecycle behavior moves the fingerprint from `0x743ca9b795e30eaf` to `0xdc253576aa4029a0` (2,845,088 samples). The single final selection passed all44 benches (exact result below).

Existing CRTC vertical vectors protect raw vertical sequencing, the real-T80 IRQ bench protects the raw trailing-edge consumer, and GA restart controls protect monitor/mode/R52/V26 separation. None is a source-derived assertion of vertical RGB blank assertion/release latency; the selected integration tests are regression coverage, not evidence for that timing rule. No mechanical vertical-edge test was added without an independently justified expected value.

Further acceptance needs independent review of the abstract GA pipeline, active vertical edges and startup/SNA fill, and hardware colon visibility. Other mode transitions, Plus output and unseen JIT phases are not established by this proof. The countdown is anchored to the measured production acceptance phase8/S1F. A sufficiently late write (phase55 or later) can cross CLKEN and expire the comparator context before publication; the old immediate path would have emitted it. A later genuine production-T80 OUTI probe observes phase56/S01, so this limit is not exclusive to fast/no_wait operation. Its fixed R2 remained ahead of both writes: it establishes transport phase, not an OUTI JIT source observer or a failed source result. No general JIT timing claim for those phases is made. The live-type cancellation test enforces a MiSTer context-replacement choice, not an ACCC-specified physical CRTC switch behavior. No production MODE_SYNC correction is included.

## Final selection

Command: `python3 sim/select_tests.py --run` (one run after the final code edit). Exact final line:

```text
select_tests: PASS 44 benches: test, crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, b18-save-admission-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_mode_latch_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Log: ignored `local/overnight-20261103/classic/blank-production/selection.log`. The existing FDC payload polling XFAIL remains; there were no newly failing benches. No code was edited after this selection.

## Independent review and acceptance limits

Opus5.5/high review20261104T040025Z-1839-e8fa read the rendered French source
and final diff, finding no RTL defect. Its evidence blocker was valid: the
initial soak/IRQ logs preceded the last cancellation edits. Both checks were
then run once on the unchanged final code: `make -C sim soak
SOAK_EXPECT=dc253576aa4029a0` matches that hash, and `make -C sim
classic-irq-phase-test` reports8 passed/0 failed. The final selection was not
rerun. Parent independently checked all four RTL/fixture hashes against the
pre-review handoff. Review and refreshed logs are preserved in
`classic/blank-review/` and `classic/blank-production/final-{soak,irq-phase}.log`.

Raw-mode metadata is a known integration limit. Motherboard `pixel_vblank`
samples raw GA vertical blank once, while the new classic RGB path consumes
three native-dot edges. Raw CRT also takes its horizontal blank directly from
raw HSYNC. Thus Raw pixels/Raw CRT cannot be treated as pixel-aligned oracles for
this change: their masks/acquisition can precede the new RGB blank by roughly
two native dots. Full does not use the raw pixel mask. The follow-up is tracked
in backlog B1; hardware milestone captures must record Full explicitly. No
raw-mode metadata correction is included in this bounded repair.

Parent accepts the reviewed code and refreshed evidence for synthesis and
targeted hardware checks. This does not accept vertical RGB timing, physical GA
pipeline identity, the Type0 mixed-mode residual, or unseen write phases, including the OUTI boundary.

## Exact build and hardware milestone

Source `db4be806455eda22adee83ba4073cc9a5944dce8` passed
[hosted full build41771359810](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41771359810),
including selected simulation/lint, production-T80 and required gate. Quartus
17.0.2.602 reports setup0.280ns/hold0.242ns, zero TNS across seven clocks,
zero errors, critical warnings or implicit-net warnings. Artifact
`Amstrad-build-345-1-full/Amstrad_20261104_db4be80.rbf` has SHA256
`84661e28263acc630539f0f5b86decf9b117f4419fc79cce80e59b812530e4f6`;
the copy in MiSTer's `_Computer` directory matches. Source9de75d3's CPU reset
repair is included; no R0 terminal-latch proposal is in this RBF.

The unchanged DSC4 disk SHA256 is
`ec9e0d51b10a7455848bd9211793c585c46456759486057b12a491039923b8e4`.
Each CRTC run starts from a fresh core load and requests Full via CFG. This
demo run has no runtime SSM readback of the applied filter latch. Each finishes
19commands/four captures/zero failures. Evidence is under ignored
`local/overnight-20261103/milestone-next/dsc4-crtc{0,1}-db4be80/`, with
`capture-hashes.json`, input CSL and build reports beside it.

- CRTC0 and CRTC1 credits show all four colons with readable handwritten names.
  Their native768×273 credit PNGs are identical, SHA256
  `d6f6bfe064b66ad65fdcb897403e2d2c9080b84599e78ffe40204ff7cba04fb2`.
  The previous native CRTC1 capture visibly clipped the label endings.
- CRTC1's otter image, including the corrupt final line, is byte-identical to
  the earlier `bedb1c5` Full capture: SHA256
  `d811cce0582fcaa25c098194898e07d36dcace76ac8c77c71d793c9e4e88f373`.
  This rules out a repair or a new regression of that captured residual.
  CRTC0's corresponding captured bottom row is black.
- Both later captures show the rotating main image with the fire bands.
  The first capture named `main` is still the preceding static image; capture
  names and approximate waits must not replace scene inspection. The CRTC1
  captures occur24.8/43.6/50.4/56.2 wall seconds after the key command returns,
  including capture/transport overhead. No full animation or all-frame
  acceptance is claimed.

This accepts the missing-colon repair on the tested device and settings. A
subsequent [uncropped comparison](dsc4-bottom-row-crop-2026-11-04.md) matches
all768 pixels of the CRTC1 bottom row to AmSpirit outside its default viewport.
Its framing policy belongs to B1. The Type0 mixed-mode extra pixel, raw metadata,
OUTI coincidence and vertical RGB-edge questions remain separate open work.

## Final-tree evidence after independent review

The evidence-only review requested these two checks after the final RTL edit; both were run once against the frozen final RTL, without repeating the selected gate:

```text
make -C sim soak SOAK_EXPECT=dc253576aa4029a0
soak: seed 0xaccc5eed20260822, 2845088 characters, 2845088 CLKEN samples
soak hash: 0xdc253576aa4029a0
soak hash matches expected

make -C sim classic-irq-phase-test
classic_irq_phase: 8 passed, 0 failed
```

Logs: ignored `classic/blank-production/final-soak.log` and `final-irq-phase.log` beneath `local/overnight-20261103/`. IRQ measurements remain Type0 HS−N=−7/IRQ−N=−6 and Type1 −3/−2. RTL and C++/Verilog fixture SHA256 values still match the frozen pre-review `HANDOFF.json`; only this report and the soak-history explanation changed after review. The existing PASS44 selection is unchanged and was not rerun.
