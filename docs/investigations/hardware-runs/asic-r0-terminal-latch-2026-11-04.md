# ASIC R0 terminal lifetime — 4 November 2026

## Current acceptance boundary

The retained actual-motherboard/T80/PPI test failed against the unchanged
production ASIC: **3 passed, 3 failed**. The parent inspected that result and
approved the exact minimal entered-terminal latch with live status preserved.
It is now applied. The expanded owner regression passes **7/7**, and default
classic B18 passes. All 75 ASIC leaf cases now pass with their body assertions
preserved. Fresh independent review accepts the bounded implementation; the
single final selected gate passed 44 benches. It is ready for a hardware
milestone, not hardware acceptance.

## Rule and independent evidence

French ACCC v1.11 §13.6.3 p.125's rendered type 3/4 chronogram requires an OUT(C)
starting at C0vs=3C to extend old R0=3F to 7F, while 3D is too late. OUTI 3B extends
the line; OUTI 3C misses it. The independent original Plus SHAKER A4 photograph
and fresh CPCSE observations corroborate HCC 3C=OK/HCC 3D=KO and OUTI 3C=01. The
MiSTer RBF3113323 returns 3D=OK and OUTI 3C=00. Thus the protected behavior is the
old terminal wrap decision, not a guessed physical storage timestamp.

The private pending-register queue could produce those outcomes but also delayed
other registers' comparator consumption. The narrower private alternative retains
one old terminal opportunity while leaving all bus/register writes live. Its
complete bounded ledger is under
`local/overnight-20261103/asic/r0-terminal-proof/`:

- All four synthetic early/late counter checks pass with the same source PPI
  synchronizer and no CPU/PC/RAM/phase patch.
- Genuine A4 top results become 0,0,0,0,1,1,1; the first original OUTI 3C store
  becomes 01. Both reproduce the original Plus reference. The first 72F2 caller
  and its 71A1 result are preserved before later 3B overwrites the temporary.
- Original DSC4 C000 execution agrees with all four correctly ordered first-loop
  E2B/E54 AmSpirit checkpoints, and twelve C802/C804/C80B observations remain
  0/0/0,3/0/0,13/0/0. This is software-reference evidence, not a new hardware
  golden for arbitrary R0 lowering.
- No R4 early-write defect appears in that natural chain: C802 writes 0 at
  C0=2/C4=C9=0; C851 writes 2 at C0=28/C4=C9=0. Zero of eleven C851 writes have
  current C4=the new 2. No held R4 state was introduced.

## Retained test and oracle split

`sim/asic_r0_terminal_test.cpp` reuses `b18_snapshot_top.sv`, its actual production
T80, motherboard, MMU, PPI and SNA owners. A fixture-only PLUS_MODE parameter
selects Plus at elaboration, default 0 preserving existing classic B18 behavior.
All new counter observations are passive. The target is registered in the
existing production-T80 list/aggregate; no workflow or production test seam exists.

Each case generates an ordinary uncompressed Plus v3 snapshot: PC=8000, IFFs=0,
ROMs off, standard R0=63/R4=38/R5=0/R7=30/R9=7, PPI-B input and counters at
origin. RAM contains only the small synthetic program, its payload and the
attributed 100-byte calibration/wait routine from French §7.2 pp.39–41. The CPU
synchronizes through real PPI bit 0, then ordinary timed instructions select the
nominal test positions. No title/disk/emulator payload or assembler is required
at runtime. No counter chooses when the CPU is released.

| Case | Required terminal C0 | Authority |
|---|---:|---|
| OUT(C) 3C early, payload 127 | 64 | French rendered §13.6.3 p.125 / Plus A4 deadline |
| OUT(C) 3D late, payload 127 | 0 | Same |
| OUTI 3B early, payload 127 | 64 | Same |
| OUTI 3C late, payload 127 | 0 | Same / original Plus OUTI 3C result |
| Early enlargement then live 63 while C0=63 | 0 | Parent-selected model choice; unverified against delayed register admission |
| Payload 59 on old terminal 63 | 0 | Ordered original DSC4 AmSpirit reference; explicitly software-only |

The last two controls additionally require that the program actually writes
while C0=63, with old R0=127 and 63 respectively. Every case requires the specified
payload to reach R0 and the real CPU to HALT. The expected counter result is not
computed from the new latch equation or a source-equivalent wrapper.

The unsupported post-RET zero expectation is deliberately absent. Both the T80
observer and the parent's independent AmSpirit stop see 63/29/7 there. There is
also an unresolved observation distinction at the critical OUT: AmSpirit's
before-instruction PC stops see 59/60 for 3C/3D, whereas T80's representable T2
INSN_START sees 60/61. No NOP count was changed to force agreement. Those C0
observations are diagnostics; the unchanged PPI-calibrated program and the
independently required early/late counter consequences are the deadline oracle.
The first VSYNC line C4/C9=30/0 is required when the critical instruction starts.

## Exact fail-before proof

Production ASIC SHA256 before the proposed edit:
`0eccba3bb6de718bf295b771c12ca18720fd2fca1bdfff6c167f0d69ba4a6657`.
The new retained test and fixture execute against that unchanged module.

```sh
export CPATH="$(xcrun --show-sdk-path)/usr/include/c++/v1"
make -C sim asic-r0-terminal-test
```

Preserved log: `/tmp/asic-r0-terminal-before.log` (also reported directly to the
parent before authorizing any production edit). Final results:

```text
PASS OUT(C) 3C early: terminal=64 expected=64
FAIL OUT(C) 3D late: terminal=64 expected=0
PASS OUTI 3B early: terminal=64 expected=64
FAIL OUTI 3C late: terminal=64 expected=0
PASS live equality at C0=63: terminal=0 expected=0
FAIL lower59 on old terminal (software reference): terminal=64 expected=0
asic-r0-terminal: 3 passed, 3 failed
make: *** [asic-r0-terminal-test] Error 1
```

All failures deliver their program's specified payload; they fail on the counter
consequence. No existing assertion or fixture pulse was weakened.

## Production boundary

Retain an entered-character comparison bit, reset true for reset HCC=R0=0,
seeded from incoming HCC==R0 on SNA apply, and updated on the existing CLKEN
from entering HCC (`hcc_next`) against the old stored R0. The terminal event is
live equality OR this remembered opportunity. This implements the observed
late-write deadline without a general queue, greater-than comparator, opcode,
A13, R4 or turbo exception. Register storage remains original/live.

The bit is a model for the observed lifetime, not physical ASIC latch proof.
Outside a preexisting terminal opportunity, the old equality/overflow model for
R0 below current C0 remains an explicitly unverified P1 assumption covered by
existing leaf tests. A standard SNA does not serialize arbitrary in-flight
comparison history; incoming-equality seeding is a documented model choice.

Status must not silently become a retained event. French §21.3.4.1 p.248's
rendered S1 table specifies bit 0 on C0=R0; §28.1.10 p.293 explicitly adds zero
otherwise. French §21.3.4.2 p.249's rendered S2 table likewise uses literal
C0=R0 for bits 0/1/2/7. The implemented `hcc_equal` remains a live comparator for
those five existing status uses; only counter/event consumers use `hcc_last`
with retained opportunity. This preserves the previous status behavior, with
no unsupported transient-status golden.

The parent-approved proposal and fail-before log are preserved under
`asic/r0-terminal-proof/retained-production/`. No broader register queue or
zero exception was added. The final selected gate and cross-provider review are
complete; the hardware milestone remains pending.

## Focused verification and scoped fixture corrections

The new retained target passes **7/7**, including the exact zero-exit transport
below. Default B18 passes its cold first fetch, six classic restorations and six
host publication races. The complete ASIC leaf suite passes **75/75**. The
original red logs and every targeted old/candidate trace remain under
`asic/r0-terminal-proof/retained-production/`; no ASIC HSYNC, R3, FIELD, interlace,
VMA, IRQ or clock rule was changed to make those tests pass.

### Zero exit: the direct-write assumption was unsupported

The original leaf t01b expected C0 to increment immediately on the next CLKEN
after changing an already matched R0=0. Its citation to the general §13.1
inclusive period did not establish that write-lifetime assertion. An exact
original/candidate production-T80 transport was compared with the parent's
independent AmSpirit run of the identical generated Plus v3 SNA, SHA256
`56a193a9681473198a4f718b8f422593b40bdf8699d2739344f8a78eccc9f8e2`.

The original program at 8000 selects R0, loads A=10, executes OUT(C),A at 800B,
then HALTs at 800D. All start at zero and observe before OUT C0/C4/C9=0/1/5,
R0=0. Original RTL observes after OUT 1/2/0, R0=10; both candidate and AmSpirit
observe 0/2/1, R0=10. At raw-write tick 1031 both immediately store 10; at character
edge 1086 only the candidate completes the entered zero line. This is an exact
**software-reference** CPU-boundary comparison with French §13.6.3 p.125's
zero-exit chart as context, not a hardware C4/C9 golden.

That original zero-exit program is the seventh retained owner case. Input,
reference hashes and raw traces remain in `asic/r0-zero-exit/` and
`asic/r0-zero-reference-amspirit/`. The original t01b red log is preserved.
The leaf t01b now explicitly checks the entered zero completion before the
original 1..63 period. t01c's alias period, t01d's C0=5 widening precondition and
t01e's C0=20 overflow precondition each finish that one startup character before
their unchanged objective assertions. No generic bus delay, counter offset or
resynchronization was added.

### Public FIELD establishes interlace context

The old fixture helpers guessed starting parity from reset-zero rollover counts.
Targeted old/candidate traces used the existing public FIELD output and showed
the following setup states; their C0/C4/C9 and pointers otherwise agreed.

| Setup | Original FIELD | Candidate FIELD | Required context / correction |
|---|---:|---:|---|
| IVM adjustment helper | 1 | 0 | Align after mode enable to true origin; assert even FIELD=1 |
| Shared interlace sync helper | 1 | 0 | Observe even FIELD at true origin, at most two origins; sync-only C9=0 is not parity |
| Local R4=0 added-line case | 1 | 0 | Require a real FIELD transition into even FIELD=1 |
| Local R9=0 added-line exit | 0 | 1 | Require a real transition into odd FIELD=0 |
| Local R5/R9=0 added-line exit | 1 | 0 | Require odd FIELD=0, as the intended 3+4-line sequence demands |

R4=0 makes C4=C9=0 identify both a real origin and an added line. The three
short local cases therefore wait for an observed FIELD transition (at most two
fields), require HCC=C4=C9=0 and ADJ=0 there, then advance the documented one
character to retain their existing HCC=1 sampling phase. All MA103/101, ADJ,
R7=0 outgoing-parity, MID-VSYNC, pulse-width/delay and added-line assertions stay
intact. Public FIELD checks additionally pin the intended contexts. The old R5
subcase happened to pass in the wrong starting parity; the new odd 3+even 4-line
context specifically samples the intended even added-line exit.

### HSYNC width is anchored to the actual pulse

The width-zero leaf cases had assumed a particular pulse phase at frame origin.
Public HCC/HSYNC traces showed unchanged pulse widths and gaps with a different
startup-history phase:

| Case | Original frame-origin HS | Candidate frame-origin HS | Unchanged measured pattern |
|---|---:|---:|---|
| t04b | 1 | 0 | 16 on /8 off, every rise at HCC=6 |
| t04i | 0 | 1 | 16 on /16 off, every rise/fall at HCC=0 |

Both fixtures park R2 and flush the programming pulse, then anchor to the first
genuine rise at their specified HCC. t04b checks the complete 16-on/8-off pattern
and next start. t04i checks the full initial pulse, full gap, next pulse and end.
Its bounded width-zero collision behavior remains explicitly an **unverified
model assumption**; no rule claim or collision expectation was changed.
These are targeted event anchors, not blanket frame waits or removed failures.

### Commands and frozen results

```sh
export CPATH="$(xcrun --show-sdk-path)/usr/include/c++/v1"
make -C sim asic-r0-terminal-test b18-snapshot-test
make -C sim/plus run/asic_video_tests
```

The expanded owner was rerun after its final C++ edit:
`asic-r0-terminal: 7 passed, 0 failed`. Default B18 was green after its final
fixture edit and was not redundantly rerun for leaf-only corrections.
`All 75 asic_video foundation tests passed` is preserved in
`retained-production/leaf-after-local-fields.log`. Every original failure and
public-signal trace remains in the same evidence directory, summarized by
`fixture-findings.json`.

Code is frozen for the parent's fresh cross-provider review. The one final
selection command, run after the last code edit, was:

```sh
python3 sim/select_tests.py --run
```

It exited 0. The exact final line is:

```text
select_tests: PASS 44 benches: test, crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, b18-save-admission-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_mode_latch_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

The complete log is `retained-production/final-selection.log`; its SHA256 is
`29a77a9b1864d2819b79b394403542548ace7a0e5d1b4479b461f5867e5d9274`. `final-code-freeze.json`
pins all seven changed code/test/index files, and the post-gate check reports
no changes. The new production-CPU bench is slow and was explicitly run as the
focused owner; the normal selection ran the affected fast benches. No soak
remint is needed for this Plus-only change. No commit, push, device/API or RBF
build was performed by this unit; independent review and hardware acceptance
remain owned by the parent.

## Independent review and parent acceptance

Opus5.5/high review20261104T044157Z-11709-2c7f returned **ACCEPT**, finding no
demonstrable RTL or test defect. It read the rendered French p.125, current
diff, investigation and final gate log without rerunning tests. Parent checked
all seven frozen code/test/index hashes, and independently reconstructed the
seventh case's SNA: it exactly matches the reference input SHA256
`56a193a9681473198a4f718b8f422593b40bdf8699d2739344f8a78eccc9f8e2`.

The review corrected one authority claim. Raising-R0 chronograms fit both this
retained-terminal model and a delayed-register-admission model. Creating a new
live equality by lowering127→63 while C0=63 is therefore a **parent-selected
model control**, not a consequence established by §13.1. The case stays, with
its label corrected in source and the table. The smaller model preserves live
admission and agrees with the bounded genuine DSC4 observations; this does not
prove which mechanism silicon uses.

One overlap remains unverified: terminal-line split capture still compares live
C0 with R0, while a late R0 rewrite can now end that line through the retained
opportunity. The combined SPLT/late-R0 case has no independent oracle here. No
split-capture logic was changed on speculation. HWRAP and setup comments now
describe their actual contracts.

After review, parent changed only comments and documentation. A saved-source
comparison confirms identical code after removing comments and empty lines;
the final gate was not redundantly rerun. Review, before-cleanup sources and
`comment-cleanup-check.json` remain in ignored `asic/r0-terminal-review/`.
Hardware acceptance must cover original A4 deadlines/OUTI, BI pulse widths,
AE/AR controls and the affected Plus demos on the exact new RBF.

## Full build and bounded device results

Exact source `7ac11a23551411b6f4b8e5435ac580022656ca6f` passes
[full build41775641712](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41775641712),
including simulation/lint, production-T80 and required gate. Quartus17.0.2.602
reports setup0.279ns/hold0.204ns, seven clocks, zero TNS, errors, critical or
implicit-net warnings. Artifact `Amstrad-build-347-1-full` contains
`Amstrad_20261104_7ac11a2.rbf`,3137680bytes, SHA256
`4dc1e7628248a30092719ff026c63997bae6d95810c49bc7ad981c10b7a501ec`.
The MiSTer copy matches. Build reports and metadata remain in ignored
`local/overnight-20261103/r0-build/`.

Fresh normal-boot device runs request Full with the same FR6128+ cartridge
SHA256 `ab241580c9b6a9fa9aeae94ca6847ea70dca386d305e38fc2ac03fce603360cf`.
The installed SHAKER disk SHA256 is
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.

| Target | Observation |
|---|---|
| A4,0037 | 3C=OK/3D=KO; OUTI3C=01; the additional C9=0..7 OUTI results are all01. Corrects the previous3D=OK/OUTI00. |
| BI | 25commands/eight captures/no failures/17SSM. All eight PNGs byte-identical to accepted3113323: widths224/16/32/48 repeated, including white1us LOGON. |
| AE/AR | All seven native PNGs byte-identical to3113323, preserving marker09, stripes and controls. |
| DSC4 Plus | Full static otter and later rotating image/fire bands replace the earlier top-band/black result. Credit labels remain clipped. |
| Amazing Plus | Logo and lower text/effect region remain present in both sampled main/later frames; prior matched script lost most of the logo and then went black. This is bounded visibility acceptance, not every animation frame. |

Evidence is under `milestone-next/`: `a4-3-device-7ac11a2/`,
`bi3-device-7ac11a2/`, `ae-ar3-device-7ac11a2/`,
`dsc4-plus-7ac11a2/`, `amazing-plus-7ac11a2/` and their comparison JSONs.
Parent inspected all title captures and the BI contact sheet. The saved3113323
references already have original-Plus photo comparison; exact PNG identity
preserves those bounded observations.

### A4 is not fully compliant

**Correction, 4 November:** the earlier last-four-row 1/1 interpretation below
was an E/F glyph misreading. A loss-free native result observer gives 0/0,
and the new original page is pixel-identical to 7ac11a2. First/third 0/1 versus
the older photo’s 1/1 remain open, with revision/history binding unproved.
[Native result qualification](asic-a4-native-results-2026-11-04.md) supersedes
the following residual analysis; no ASIC timing change follows from that
withdrawn last-four-row claim.

The lower eight printed PPI words still differ. Masking away other PPI bits,
the original Plus photo has bit0 pairs1/1 in the first four rows and0/0 in the
last four. New7ac11a2 shows1/1 throughout; the prior3113323 last four showed0/1.
Those lower-row differences must not be hidden by the successful top deadline.
The reference photo lacks the current additional C9 OUTI row, so its complete
preceding history is not assumed byte-identical.

A fresh AmSpirit1.17.1 run of the current disk and identical FR cartridge also
has0/0 in the last four rows; its first and third rows differ from the photo.
AmSpirit remains a software reference. Native high PPI bits also differ between
machines; this comparison explicitly concerns VSYNC bit0. Motherboard PB0 is
raw selected CRTC VSYNC (`vs_sel`), so the filter cannot change the sampled
PB0 value. The native image alone does not establish whether its printed hex
faithfully represents the result RAM. The existing saved-menu private R0-only and broad-queue
proofs both give the same low0/0 lower rows and therefore have not reproduced
the fresh-device failure. No new register-admission rule follows from that
initialization/entry-state gap.

Fresh reference evidence is in `asic/a4-amspirit-1171-v2/`; the first attempt
stopped before media load because `crtc=keep` is valid only for SNA, not CPR.
The corrected run uses the normal CPR load/reset contract. No data, PC or
counter patch was used.

The bounded full-caller comparison is now complete. The terminal-only model,
broad pending-register queue and one delayed-key control all reach the original
caller at73A9/IX7508, execute all16 actual measurement stores and format all
eight words. All produce `F4F5,F5F5,F4F5,F5F5,F4F4,F4F4,F4F4,F4F4`; neither
the queue nor moving the real key4 press from20ms to1second reproduces the
native last-four1/1 result. There was no delay search. Two initial wrappers
exit2 because their tally includes16 initialization writes as well as16
actual measurement stores; a source-PC-qualified scorer proves completion.
That raw diagnostic failure is preserved. The corrected delayed-key logging
run exits0.

The next discriminator is native result RAM74F8..7507 at this page, separating
execution from formatting/display. Existing B18 cannot supply it:
`Amstrad.sv` explicitly requires `!plus_mode` for save admission. No direct
CPU-RAM read path was found, and no model switch, state patch or new save
tooling was introduced for this unit. Frozen sources, traces, result scoring,
hashes and process completion are in `asic/a4-bottom-comparison/`.
These results accept the specific R0 deadline repair and restored sampled
main effects, not full SHAKER or ASIC compliance. They do not justify the
broader queue mechanism.

A separate disk-only observation probe was rejected before device use. Static
inspection found30 bytes of apparent padding at4045..4062 for a post-measurement
SSM emitter, preserving file length, header and allocation. Paired production-CPU
replay then found genuine nested stack writes there: CALL7523 pushes its return
7526 into4062/4061, and CALL604D next writes4060/405F. The emitter placement is therefore unsafe;
an earlier five-register-save stack bound was incomplete. Logical cave accesses
from bank4 backup are a separate alias and do not explain away those stack
writes. No replacement cave or load-length workaround was tried. Frozen
failed placement, exact disk diff and passive access traces are in
`asic/a4-ssm-probe/`; `asic/a4-ssm-feasibility/` is provisional static analysis,
not accepted media. No native result-RAM claim follows from it.

Both bounded jobs finish with identical16 measurement rows, eight formatter
rows and instruction-start signature through the first73A9 boundary. At that
boundary ten emitter-tail bytes4059..4062 have been overwritten in both runs;
the instrumented run subsequently leaves its intended path. Both wrappers and
the strict scorer remain red. Physical bank attribution uses source/MMU mapping
and complete at-hook RAM, not a physical-address CSV field. A separate passive
wrapper sampled marker `last_code` one publication clock too early; that stale
value is not a transport result or a production SSM defect. No marker burst or
native observation is accepted. The disk is named `.REJECTED.dsk`, and all
jobs have exited.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
