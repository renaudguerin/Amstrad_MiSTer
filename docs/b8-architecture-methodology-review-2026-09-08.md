# B8 architecture and methodology review

Reviewed 2026-09-08 against `65364eee0b66897d6001f9e7f841f197df8830e4`.
Scope: production paths, fixture fidelity, test/process complexity, and a practical
hardware-loop plan. Astra parent review with bounded native supporting audits;
this is not a cross-provider certification or an exhaustive ACCC rule re-audit.
No production RTL changed and no hardware result is claimed.

## Decision

There are concrete defects worth addressing before building an ambitious harness.
The strongest accuracy finding is a disagreement between the CPU's write phases,
the CRTC register file, and the rule engines. Several implemented special-write
paths cannot activate from normal production CPU writes, although direct-edge
unit tests activate them. This is directly relevant to the RFD prerequisite in
Longshot's DSC4 explanation. It does not yet prove that repairing it fixes DSC4.

The broader pattern is **lost events and incomplete ownership across module
boundaries**: a register's new value substitutes for the write event, restored
storage substitutes for restored engine state, and selected RGB substitutes for
the complete display interface. More tests of isolated values do not resolve
these contracts. Neither a wholesale core split nor replacing the Gate Array is
justified by the evidence found here.

The September 2 hardware report remains the baseline: the named Burnin' Rubber
right-edge sprite symptom is fixed; the named Amazing Demo symptom appears fixed;
DSC4/SHAKER still fail; other symptoms have no newer verdict. The old conversation
is motivation, not current hardware evidence.

## Production findings

| ID | Finding | Evidence | First repair scope |
|---|---|---|---|
| B8-1 | Classic RFD and other old-value side effects miss production write phases | source proof plus real-GA/CRTC phase experiment | accuracy: define bus-event/character-decision contract |
| B8-2 | Plus FIELD output belongs to the inactive classic CRTC | production wiring plus mutation experiment | Plus: selected field ownership through scaler boundary |
| B8-3 | Same-value legacy palette writes cannot restore an ASIC-edited colour | real GA/register-page reproduction and Arnold V §2.2 | Plus: carry accepted write event and destination |
| B8-4 | Video word cache stays stale after a CPU write to an unchanged fetch address | physical SDRAM model, actual WRITE and refresh control | general/shared: video request/coherence contract |
| B8-5 | Plus SNA restores storage without restoring all selected engines | source trace; lost DMA SAR event reproduced | Plus: atomic snapshot apply across owners |
| B8-6 | Plus RGB bypasses the register that delays its sync/blanking | real color-mix/gamma boundary; added-register positive control | Plus: preserve pixel/metadata latency through top-level conversion |
| B8-7 | Tape writes are re-admitted after late ACK; a next payload can corrupt them | physical SDRAM model plus synchronous producer and timing controls | general/shared: accepted-request lifetime and latched payload |

### B8-1 — CRTC old-value rules are disconnected from CPU write timing

The production chain is:

1. `rtl/T80/T80pa.vhd:108,155-167` advances the CPU and asserts an I/O write
   using `CEN_p`. `rtl/T80/T80.vhd:373,441,829-836` gates normal data-output
   updates through the same positive CPU enable.
2. `rtl/GA40010/ga40010.sv:136-149` generates `PHI_EN_P` at sequencer states
   `00/0f/ff/f0`, CRTC `CLKEN` at `03`, and `nCLKEN` at `e0`. They are disjoint.
3. `Amstrad.sv:125-132` generates one `ce_16` pulse per four system clocks.
   A CPU output changes after its launch edge; the first CRTC register-file
   capture is the following system edge, with `ce_16=0`.
4. `rtl/CRTC.v:163-184` stores writes on every system clock. By the next CRTC
   enable, the stored register already equals DI.

Consequently these conditions cannot recognize the normal write:

| Side effect | Source condition that loses its old value |
|---|---|
| Type-1 R5 RFD | `crtc_type1_engine.v:239`: `CLKEN && old_R5==0 && DI!=0` |
| Type-1 R0 widening/RFD | `crtc_type1_engine.v:320`: `CLKEN && DI>old_R0` |
| Type-0 R0=1 widening, IA-6 | `crtc_type0_engine.v:307`: `CLKEN && old_R0==1 && DI>1` |
| Type-1 R6 display reopening | `crtc_type1_engine.v:562`, consumed at `CRTC.v:762` on `nCLKEN`: `row==old_R6 && DI!=row` |

WAIT can suppress CPU advancement but cannot move its write-launch enable to a
different GA state. A post-edge combinational change of an enable cannot retrigger
a block sensitive only to the system-clock rising edge. Fast timing does not
change these enable decodes. Snapshot/debug writes are separate from this claim.

Ordinary R5 adjustment and horizontal widening still happen. They do not replace
RFD: the private VMA/parity flags are armed only by the broken routes and their
pending derivative (`crtc_type1_engine.v:500-528`). The R6 fallback can copy
`vde_r`, but cannot reopen a row whose old R6 comparison cleared both vertical
display states. This finding does **not** mean all CRTC writes are broken:
R2.JIT already recognizes the first write at system-clock cadence and retains
pending state (`CRTC.v:475-482`).

**Experiment.** Unchanged production GA and CRTC, production divider, sequential
bus launches matching the VHDL CPU's positive-enable phase; snapshot/reset only
establish the initial fixture. A type-1 R5 0→1 write was launched from each
permitted CPU phase on C0=R0. A direct-CLKEN injection is the positive control.

| Launch state | System clocks to CRTC edge | RFD VMA/parity flags | MA after rollover |
|---|---:|---|---|
| `0f` | 56 | 0 / 0 | `123c` |
| `ff` | 40 | 0 / 0 | `123c` |
| `f0` | 24 | 0 / 0 | `123c` |
| `00` | 8 | 0 / 0 | `123c` |
| Artificial direct CRTC edge | 0 | 1 / 1 | `1234` |

The address difference rules out an equivalent ordinary reload in this fixture.
Parent rebuilt and reran it; a second source inspection challenged the phasing
and found no escape route. This is a scripted bus, not execution of production
T80 or a new silicon timing measurement. The other three side effects are
source-derived consequences of the same mismatch, not separately reproduced here.

**Repair direction.** First carry the source-derived RFD discriminator through a
production-phase bus fixture. Re-read the French rules for when a write is
accepted and when its side effects are consumed; retain the relevant old/new
value and phase until that decision. Do not blindly delay every register write
to CLKEN: that would jeopardize existing sub-character behavior. Cover R5, both
R0 routes and R6 in the contract review, and retain their distinct rule semantics.
Then execute a real CPU instruction sequence before title-level acceptance.

### B8-2 — Plus video exports classic field metadata

`rtl/Amstrad_motherboard.v:242` connects classic `CRTC.FIELD` directly to the
motherboard output. `Amstrad.sv:1351` sends it to `VGA_F1`, even in Plus mode.
It affects the local scandoubler decision (`Amstrad.sv:1535-1551`) and ASCAL's
interlace/field-buffer handling (`sys/sys_top.v:756`,
`sys/ascal.vhd:1217-1258`). Plus `asic_video` has its own frame parity and
interlace lifecycle, which are not the source of this metadata.

With Plus selected and clocks/Plus state held fixed, changing classic field
0→1 changed exported field 1→0. Changing Plus parity 0→1 did not change that
output. This confirms ownership leakage; it does not establish the correct
field polarity or the CRTC3 demo's exact failure mechanism.

B7 never observed or mutated FIELD: its P10 wrapper disconnects `.field()`
(`sim/plus/p10_boot_test_top.v:510`). P1 exposes `vsync_field`, but its C++ tests
never assert it. B7's existing RGB/bus isolation result remains valid within
that scope; the blanket isolation conclusion does not.

**Repair direction.** Export selected-machine field metadata with a deliberate
polarity and timing contract. Test R8 transitions and interlace against selected
VSYNC, plus a negative control perturbing the inactive engine. Check the
scandoubler/scaler-facing boundary, not only motherboard RGB.

### B8-3 — Palette shadow changes cannot represent repeated writes

`asic_ga_timing` recognizes each legacy INKR write, but the motherboard passes
only stored colour arrays to `asic_regs`. Its translation at
`rtl/plus/asic_regs.v:426-446` runs only when a value changes. Arnold V §2.2
describes the legacy port as another writer of the same palette, not a one-time
conversion of a separate shadow.

Reproduction with real GA, register-page and sprite-RAM modules:
select pen 0 → legacy black (`54`) → ASIC palette white (`0fff`) → legacy
black (`54`) again. GA ink remains 20, so the last event is lost and palette
remains `fff` instead of `000`. A changed-value blue write produces `00f`,
confirming that the path works when the shadow differs.

The `a04` leaf test drives colour shadows, so it cannot express this missing
event. **Repair direction:** carry the accepted write strobe, pen/border index
and payload to the palette owner; test both pen and border through actual
I/O and memory-page accesses. Copter causality is unknown, especially until the
wrong pixel's plane is identified: legacy writes affect entries 0–16, not sprite
palette entries 17–31.

### B8-4 — Repeated-address video reads lack coherence

`rtl/sdram.v:170-175` requests a video read only when address bits `[15:1]`
change. A CPU write does not invalidate the remembered address; `vram_bank`
is not part of the key either. The result updates only after a video request.

The physical SDRAM fixture fetched `3412` at `20100`, then accepted an actual
CPU-port WRITE of low byte `56`. With the same video address held for another
64 clocks, RAM contained `56` but video remained `3412`, not `3456`. Moving
the video address away and back refreshed it correctly. This address alias
exists in production: CPU `0100` maps to `20100`, and video word address `0080`
maps to that same location in bank zero.

Normal incrementing raster addresses hide this defect. Repeated-address or
degenerate raster/DMA cases are plausible triggers; no SHAKER entry or title
has been reproduced with it yet. Bank-only changes are another missing key,
but ordinary steady-state Plus operation uses bank zero, so do not call that
variant a general Plus banking failure.

**Repair direction.** Specify when video requests a fresh word independently
of whether its address changed, and how writes invalidate retained data.
Preserve cartridge/CPU/refresh scheduling. Pin a real CPU-write/video-read
alias with a subsequent production-motherboard pixel observation before
making a broad arbitration change.

### B8-5 — Plus snapshot restore stops at register storage

The production reset split allows the CPC+ parser to fill ASIC registers while
the machine remains reset (`Amstrad.sv:729-748`). However, DMA receives ordinary
machine reset (`Amstrad_motherboard.v:562`), so the SAR-write pulses emitted
during download/drain lose to its reset branch (`asic_dma.v:145-179`). There
is no later replay/apply event for DMA's current address.

Reproduction restored SAR=`2210` and enabled channel 0: register readback was
correct, but after reset release the DMA current address was `0000`. Repeating
the same SAR write after reset correctly loaded `2210`.

The wider restore path has related omissions: snapshot CRTC/GA inputs feed the
classic instances, while selected `asic_video`/`asic_ga_timing` lack restore
ports. `plus_mmu` applies RMR2/unlock but resets ordinary ROM enables and does
not restore the snapshot's ROM selection. The P8 fixture instantiates the
parser/register/MMU seam, not DMA or selected video/GA; its SAR/DCSR checks
therefore certify stored bytes rather than a resumed machine.

**Repair direction.** Define one snapshot apply transaction across the selected
owners, after storage drain and before CPU release. Check first DMA fetch,
selected CRTC/mode and CPU memory mapping from deliberately non-default state.
Until then use DSK/CPR for the first hardware-loop slice. This finding concerns
SNA restores, not cold CPR boot or unexplained cartridge crashes.

### B8-6 — Plus pixels bypass a timing register

`Amstrad.sv:1483-1503` routes RGB and all four sync/blanking signals through
`color_mix`. That module registers them together on `ce_pix`
(`rtl/color_mix.sv:101-121`). Classic uses those registered RGB outputs.
Plus instead expands/converts its palette combinationally (`Amstrad.sv:1509-1530`)
and selects those undelayed pixels at the mixer (`Amstrad.sv:1546-1548`), while
retaining the registered sync/blanking. Pixels therefore advance by one `ce_pix`
sample relative to their metadata.

The real `color_mix` + `gamma_corr` diagnostic sent three active red samples
followed by black blanking. The first red sample remained marked blank and the
last active sample became black. A positive control adding one `ce_pix` register
to Plus RGB removed both mismatches; the classic path retained its red active
sample. This is a pipeline-latency contract, not an inferred CRTC timing rule.

There is no compensating Plus delay on the normal scandoubler-off path:
`HDMI_FREEZE=0` selects freezer passthrough; gamma and mixer stages retain RGB
and timing together and have no later Plus-mode selector. Full vendor-mixer
simulation was attempted but blocked by existing Verilator incompatibilities
in `sys/hq2x.sv`; only the color-mix/gamma boundary was simulated here. Adaptive/
HQ2x manifestation, hardware displacement and title causality remain unproved.

**Repair direction.** Give Plus conversion the same `ce_pix` latency as its
timing. Keep a changing-colour/blanking test at the top conversion boundary and
cover the actual pixel-enable choices. Do not compensate by shifting sprite
coordinates or CRTC display windows.

### B8-7 — Tape ACK permits duplicate writes with a later payload

The tape producer holds `tape_wr` until it synchronously samples `tape_wr_ack`
(`Amstrad.sv:770-774`). SDRAM asserts ACK at READY, `q=7`, so at the following
IDLE arbitration edge it still sees the old asserted request while the producer
clears it. With no higher-priority client, that completed write is admitted again
(`rtl/sdram.v:164-168,271-277`). The duplicate captures the old address but takes
live `tape_din` three clocks later (`sdram.v:255-258`). Tape writes do not
contribute backpressure to `ioctl_wait` (`Amstrad.sv:302`).

The physical SDRAM diagnostic models those synchronous producer updates without
forcing DUT internals. An isolated byte `35` at `120` generated two identical
WRITEs. Accepting the next byte `a6` at `121` one edge after ACK consumption
produced `WRITE(120,35)`, `WRITE(120,a6)`, `WRITE(121,a6)`; the first byte was
corrupted. Delaying the next byte preserved both, although writes still duplicated.

A conditional variant ends download before duplicate admission: the production
address mux (`Amstrad.sv:717`) switches to the parked playback address zero,
and the duplicate overwrites the header. Ending download after completion
preserves it. These are allowed local-interface phases, not measured HPS
cadence or evidence of a corrupt real CDT file. Duplicate admission itself
does not require a fast next byte; corruption does.

**Repair direction.** Pin synchronous request/ACK lifetime, latch accepted address
and data together, and drain outstanding writes before changing address ownership.
Reuse the physical-DQ memory fixture and preserve contention/refresh behavior.
The cartridge client already acknowledges early enough for a synchronous
producer; do not apply a global timing shift without checking each client.

## Test and process review

The principal problem is fidelity and interpretation, not the number of tests.
Keep independent source-derived rules, adversarial bus interactions, real EDSK
checks, GA differential tests, small-memory edge cases and strict lint. The
following changes reduce work while making the evidence more useful:

1. **Own the complete production seam once.** P1's copied fetch block changes
   the address-capture phase for zero-latency RAM. Its pending wording correction
   assigns absolute timing to P1 motherboard, but that fixture ties `vram_din=0`
   and discards the address. P10's physical SDRAM address checks stop short of
   asserting returned pixels. Add the missing address→return→pixel observation
   to a production fixture; keep the existing P1 differential as a relative
   pixel-phase test. Do not build another copied motherboard.
2. **Retire source-string assertions as behavior evidence.** Approximately 80
   lines in `sim/plus/sdram_cartridge_test.cpp:759-840` check literal instance
   names, mux expressions and menu text. Formatting can fail them and matching
   dead text can satisfy them. Replace their useful ownership claims with
   executable seams, then remove the redundant strings; do not write a richer
   source parser. Keep the actual SDRAM arbitration tests.
3. **Share real peripheral composition.** The stub-file explanation that SV
   children cannot join the `.v` motherboard is stale: B7/DMA targets already
   use mixed-extension parsing and real GA/YM/HID. Reuse that proven composition
   for integration fixtures; retain deliberate scripted CPU drivers for narrow
   bus tests and label them accurately.
4. **Stop adapting software to a broken CPU substitute.** The current TV80
   surrogate lacks working conditional jumps and important interrupt behavior.
   Hand-unrolled transfers and WZ-priming workarounds make tests run but reduce
   their value as firmware evidence. The preserved CPU candidate is unfinished.
   Evaluate a bounded route to running the production VHDL T80, or establish
   equivalence of a complete substitute for the needed instructions. Neither
   approach needs a new CPU emulator designed for these tests.
5. **Finish the existing small consolidation.** Pending `c12c264` retains RAM
   capability checks while removing one duplicate model-selector binary and
   keeping strict decoder lint. Reconcile its stale ownership wording from
   item 1 before integration. It is a sensible cleanup, not the accuracy priority.
6. **Make default-gate cost visible before pruning expensive tests.** Plus
   defines 28 binaries; B7 runs 35 processes, yielding roughly 62 Plus test
   launches before the classic/filter/GA/FDC suites. Mutation controls earn
   their place, but a broad audit need not grow without measured value. Separate
   compilation time from test time before changing cadence. The full suite took
   197.64 seconds from a clean archive and 56.59 seconds warm on this host.
   Compilation dominates the difference; the warm run includes mutation/audit
   work as well as test execution. No gate was removed during this review.
7. **Reduce duplicated status, preserve evidence.** At this SHA current-status
   is 1,533 lines and review-debt 728; old handoffs compete with current decisions.
   Keep one concise live queue and link dated evidence rather than copying its
   verdict into several living documents. A cleared review is scoped evidence,
   not a reason to reject a new reproduction. Documentation-only edits do not
   need repeated model-review ceremonies.

The standalone classic harness uses 16 system ticks per character while production
uses 64. F20 has a separate real-GA production-ratio fixture, but no executed T80
instruction. The controlled R3 restart test imports a fixed 14-tick delay into
the 16-tick harness. A temporary rerun of its three cases at 64/32-tick character/
half-character timing passed: no new R3 defect was established. The missing
acceptance layer is the legal CPU launch phase, not merely increasing the clock
constant or deleting the existing tests.

## Architecture choices and next work

Keep one runtime-selectable core and the per-type CRTC engines. Runtime clock
gating does not recover fitted ALMs and is not a remedy for incomplete ownership.
Complete the selected-machine interfaces and distinguish accepted writes, stored
values, and apply/reset events explicitly. The B7 synthesis report still supports
its dated conclusion that no second sprite-RAM-scale inference problem was found;
this pass did not rerun Quartus or establish current physical timing.

`crt_filter` also feeds `SHIFT` back into VRAM byte assembly, before RGB. Thus
there is no filter-independent pre-filter RGB tap in the current design, even
though raw/selected sync taps exist. The B3 review recorded this correctly; B6's
follow-up must consume it. Keep stable scaler acquisition as a design aim, but
do not infer that changing blanking alone isolates the emulated picture.

B11 must be scoped by the complete memory path. CPU writes bypass the CRTC;
SDRAM service/cache and GA byte sampling own write-to-pixel visibility. The
categorical claims that SHAKER's VRAM timing is impossible because of character
granularity, or that all required work is CRTC-side, are unsupported. Preserve
the accurate GA and investigate these interfaces before planning its replacement.

Recommended order, with one writer per shared file and separate behavior streams:

1. **Accuracy:** B8-1 production-phase regression and French-rule event contract,
   then a focused repair and actual-CPU discriminator. This has the strongest
   connection to the reported absence of DSC4 progress.
2. **Plus:** B8-6 pixel/timing latency, B8-2 field ownership and B8-3 palette event
   as separate bounded fixes; verify their complete downstream paths before
   title attribution.
3. **General/shared:** B8-4 repeated-address memory coherence, with a real
   motherboard video-return fixture. It may proceed independently of CRTC RTL,
   but coordinate SDRAM/fixture ownership with cartridge and FDC work. B8-7 tape
   request lifetime uses the same SDRAM file and should follow serially.
4. **Plus:** B8-5 complete snapshot apply before relying on SNA for automation.
5. **B2/B4:** follow the [hardware-loop plan](mister-hardware-loop-plan.md), starting
   with existing Main capture and input tools, then a declared CSL subset.
   Exact SSM event-to-image capture is a later design, not a prerequisite for
   useful unattended screenshots.

Fable is optional after these repairs or for a disputed architectural choice.
A useful second-opinion packet is this report, the exact diff, the failing
production-phase cases and their source rules—not the full historical transcript.
No publication, merge, RBF, or hardware closure is implied by this review.

## Verification and retained local evidence

Parent rebuilt and reran the palette, SDRAM, snapshot/DMA, field-ownership,
classic RFD, pixel/timing and tape-handshake diagnostics. Their fixtures deliberately
demonstrate current defects; they are not accepted regression tests and have not been added to the default
suite. Source and result logs are retained locally under ignored
`docs/references/b8-audit-2026-09-08/`; run its `reproduce.sh` to rebuild against
the current checkout. A future repair must convert the relevant case into a
failure-first, source-derived regression with proper controls.

The initial baseline-suite attempt encountered generated dependencies pointing
to removed Verilator 5.050 headers after the local upgrade to 5.052. A clean
archive of exact `65364ee` was used for the subsequent baseline measurement,
preserving existing generated output. `make -C sim` completed successfully with
the existing FDC XFAIL retained: 197.64 seconds clean, 56.59 seconds warm. Logs
are retained alongside the diagnostics as `baseline-clean.log` and
`baseline-warm.log`. No production or test source changed; no new lint, soak,
Quartus, RBF or hardware result is claimed.
