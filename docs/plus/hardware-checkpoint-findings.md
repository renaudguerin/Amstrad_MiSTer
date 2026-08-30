# Plus cartridge compatibility findings and closure plan

**Checkpoint date:** 2026-08-29
**Status:** OPEN — P0-P9 are implemented at module/simulation level, but full-system and
hardware compatibility are not closed
**Next stream topic:** `plus/p10-compatibility-closure`

This document is the durable handoff from the first broad post-P9 cartridge test. It
supersedes the implementation todos formerly recorded here for HF-1 through HF-3: those
changes landed in `ee50c33`, with the later banking/SNA corrections in `421aec6` and
`7c46b8d`. The new observations show that passing leaf benches and review is not equivalent
to a timing-clean, production-top-level, real-title acceptance result.

## 1. Hardware observations

The tested RBF hash, fitter effort, OSD Plus model, mounted DSK/CDT, and MiSTer version were
not recorded with these observations. Preserve them as symptoms, not as proof against one
specific commit:

| Cartridge | Result | Immediate interpretation |
|---|---|---|
| Panza Kick Boxing | Grey active area with blue border | The video output and at least part of the boot path are alive, but no first-divergence trace exists. Timing, cartridge-fetch pacing, interrupts, or an ASIC-video assumption may stop normal setup. |
| RoboCop 2 | Starts normally; sprites are garbled | Cartridge parsing and basic execution work. The dynamic sprite-RAM path is a stronger suspect than palette, priority, or CPR format. |
| Arnold 5 diagnostic | Loads; keyboard is inoperable | CF-1 is a confirmed PPI defect and a plausible match only if Arnold leaves Port C configured as input or rewrites `0x9B`/`0x92` while relying on Plus always-output behavior. Standard firmware can later write an output-direction word and mask the defect. Confirm the diagnostic's control writes under 6128+ or 464+; a GX4000 has no full keyboard. |
| BASIC/System cartridges | Copyright banner, delay, then `Drive A: read fail` | HF-1's address decode exists, but the production FDC path is untested, model capabilities are inert, FDC state survives CPR reset, and the media/model configuration was not recorded. |
| Other cartridges | Approximately half load; the rest remain black or grey/blue | This title-dependent split is compatible with a timing-invalid RBF, per-fetch cartridge WAITs, or uncovered production integration. It does not by itself select one ASIC rule. |

The sample Panza and RoboCop images use ordinary `cb00`-style cartridge chunks that the
current parser accepts. No title-specific parser-format defect was found.

## 2. Evidence baseline and acceptance vocabulary

The current integration checkout at the checkpoint was `0fe0ab3`; its latest code tip was
`7c46b8d`. The checkpoint investigation re-ran `make -C sim` at `0fe0ab3`: all 175 required
classic vectors and every Plus suite passed. That is useful regression evidence, but the
benches do not execute a real title through the production top level.

The only current local RBF is the smoke-fit artifact for `7c46b8d`:

- file: `output_files/Amstrad-build-154-1-smoke/Amstrad_20260829_7c46b8d.rbf`;
- SHA-256: `895bd0491e9ff249295bab27b091870ddf5728961b97b07ee1129f1316faad86`;
- utilisation: 34,556 / 41,910 ALMs (82%);
- main setup slack: **-0.796 ns**, TNS **-10.576 ns**;
- HDMI setup slack: **-0.063 ns**.

Per `docs/ci-testing-policy.md`, a smoke RBF is not retained hardware-build evidence. If it
was the tested file, its negative setup slack is a plausible source of title-dependent
failure, but not proof of any specific symptom. The first P10 gate is therefore an exact-tip
full-effort compile. Prefer `gh workflow run local-build.yml --ref <branch> -f effort=full`
when the Quartus VM runner is online; otherwise dispatch hosted `build.yml` with
`effort=full`. Require non-negative setup/hold slack and zero TNS in the constrained internal
domains, while retaining the existing warning that unconstrained external I/O paths are not
proven by those summaries. The last full-effort milestone `27cb993` had +0.635 ns setup
slack, so the smoke failure may be FAST-FIT-specific; only an exact-tip full build can decide.
Do not interpret further cartridge results until the tested RBF is identified and passes
this gate.

Use these four states in status documents and test reports:

1. **Implemented** — RTL and integration code exist.
2. **Simulation-verified** — focused deterministic vectors and the required suite pass.
3. **Full-fit/timing-clean** — an exact full-effort Quartus build meets timing.
4. **Hardware-confirmed** — a named RBF/configuration passes the relevant physical matrix.

P0-P9 are implemented and simulation-verified. They are not collectively
hardware-confirmed.

## 3. Confirmed implementation defects

These findings follow directly from production RTL and do not require another reference
search before writing a failing vector.

### CF-1 — Plus PPI Port C readback was fixed, but its physical pins remain direction-gated

`rtl/i8255.v` still drives `opc[7:4]` and `opc[3:0]` through classic 8255 direction bits.
`plus_mode` changes Port C read/write semantics but does not change those output assigns.
The Plus reference says Port C is always output and a control-register rewrite preserves its
output latches. While reset or SNA control value `0x9B` remains active, the current physical
`opc` value is `0xFF`; `rtl/Amstrad_motherboard.v` feeds `portC[3:0]` directly to HID as the
keyboard row. Row 15 produces an apparently dead keyboard. Standard firmware commonly writes
an output-direction control word later, so CF-1 explains Arnold 5 only if the diagnostic
retains or re-enters an input-direction configuration.

The P8 leaf vector calls Port C output-only but checks latch readback only. It never leaves a
classic input-direction word active while inspecting the physical `ppi_opc` pins.

**Required fix and exit:**

- [x] In Plus mode, drive both physical Port C nibbles from `opc_r` regardless of classic
  direction bits; preserve classic behavior when `plus_mode=0` (implemented in `rtl/i8255.v`).
- [x] Add a focused physical-pin vector for control words `0x9B` and `0x92` (verified in `sim/plus/plus_p8_test.cpp`).
- [x] Add a production-path PPI -> PSG register 14 -> HID matrix test that selects at least
  two keyboard rows after reset and after SNA restore.
- [ ] Trace Arnold 5's PPI control writes and confirm the failing interval retains or writes
  an input-direction control word before attributing the hardware symptom to CF-1.
- [ ] Re-run Arnold 5 on 6128+ or 464+ and record which subtest first changes.

### CF-2 — Plus model capability outputs do not control FDC or tape hardware

At the checkpoint, `plus_model_select` derived `plus_has_fdc` and `plus_has_tape`, but the
devices did not consume them. P10 added the gates; the remaining problem is production
coverage rather than absence of the capability wiring.

**Required fix and exit:**

- [x] Gate FDC status/data and motor writes with `!plus_mode || plus_has_fdc` (implemented in `Amstrad.sv`).
- [x] Gate tape sense/motor paths with `!plus_mode || plus_has_tape` without changing classic
  model behavior (implemented in `rtl/Amstrad_motherboard.v` and `Amstrad.sv`).
- [ ] Add a complete production model matrix: the shared decoder test now covers 6128+
  positive and GX4000/464+ negative FDC/motor cases, but tape pins and real controller reads
  are still absent.
- [ ] Pin the intended open-bus/read result and motor behavior for an absent device. The
  decoder output is pinned; the production data mux and tape open-bus choice are not.

### CF-3 — CPR/system reset does not reset the FDC or motor latch

At the checkpoint, the `u765` instance used only `status[0]` and the motor latch had no reset
branch, allowing cartridge replacement to inherit controller state. P10 connected system
reset to both, but no real-controller transaction test proves that contract yet.

**Required fix and exit:**

- [x] Define the FDC/motor reset contract for classic OSD reset, Plus CPR apply, and model
  changes; reset both controller state and motor on the selected system-reset event (connected `u765.reset(reset)` and `motor` reset in `Amstrad.sv`).
- [x] Add `active FDC request -> reset -> delayed stale ACK/buffer traffic -> fresh request`
  coverage with the real u765 model and a deterministic mounted disk. Integration `074c182`
  also retains a mount retry across the quarantine. A full CPU READ DATA boot remains open.
- [x] Drive CPC selected-write aliases through the production `Amstrad.sv` decode and bounded
  u765 bench. A full CPU/AMSDOS transaction at `&FADD`/`&FBDF` remains a hardware/top-level gate.

### CF-4 — P7's DMA-versus-PPI/PSG WAIT contract is absent

At the checkpoint, CPU PPI accesses did not wait during a DMA PSG operation. P10 added the
base WAIT/ownership and AY-selected-register restoration paths, but the DMA leaf bench still
uses synthetic RAM and cannot exercise a concurrent CPU keyboard/PSG access.

**Required fix and exit:**

- [x] Implement the documented CPU arbitration WAIT around DMA PSG operations (`dma_ppi_wait` in `rtl/Amstrad_motherboard.v`).
- [x] Preserve and restore PSG selected-register and relevant PPI direction/control state (8-cycle LOAD with AY register restoration in `rtl/plus/asic_dma.v`).
- [ ] Add a production motherboard test with DMA active while the CPU scans the keyboard,
  based on the Arnold 5 DMA/keyboard diagnostic. `asic_dma_test.cpp` cannot observe the CPU,
  PPI gating, or motherboard WAIT path.
- [ ] Verify the maximum stall and post-DMA state against a primary source or hardware trace;
  the base 8-cycle envelope is implemented, but the documented simultaneous-CPU-access
  `+1/+2` contention extension is not.

### CF-5 — CPC+ SNA parsing is broken at the production top level

This does not explain CPR boot failures. P10 added the queue/backpressure and parser-download
reset split, then independent review found that RMR2/unlock shadows were cleared before the
delayed apply pulse. A later production-path scan found three more defects: `asic_regs` stayed
in reset while CPC+ payload writes arrived, the registered strobe consumed live rather than
captured byte data (and could repeat across a stall), and snapshot application could begin
before the FIFO and final registered strobe drained. The integrated remediation fixes those
paths. A production-shaped parser/`asic_regs`/MMU seam passes locally, but the full
`Amstrad.sv` lifecycle still cannot be elaborated by the local Verilator harness.

**Required fix and exit:**

- [x] Separate parser-download reset from the later machine apply reset. The remediation
  branch uses a dedicated `sna_parser_reset` for parser/shadow ownership and a one-clock
  `plus_asic_reset` pulse at SNA start, so `asic_regs` accepts restored payload while the CPU
  and ordinary motherboard remain reset. Unrelated reset sources still clear both paths.
- [x] Add backpressure or a two-output queue so consecutive source bytes cannot overwrite a
  pending sprite nibble (8-entry FIFO write queue with `ioctl_wait` backpressure in
  `rtl/plus/plus_sna_parser.v`). The top captures each accepted data byte with a one-cycle
  strobe; the parser accepts the final registered tail after `sna_download` falls, exposes
  `busy`, and delayed application waits for the FIFO/write pipeline to drain.
  A new snapshot edge also aborts any residual prior FIFO/write tail in the
  same clock that `plus_asic_reset` clears the ASIC register image.
- [x] Add a local production-shaped seam test covering consecutive sprite expansion,
  asserted/released backpressure, the falling-download tail byte, FIFO drain, ASIC sprite/
  palette/control/DMA register restore, retained RMR2/unlock application through `plus_mmu`,
  a rapid restart before the previous FIFO drains, and an ordinary later reset. This
  elaborates the production parser, `asic_regs`, and MMU.
- [ ] Add a production `Amstrad.sv` CPC+ SNA integration test with consecutive bytes and
  verify PPI, PSG, ASIC registers, palette, sprite pixels, RMR2/unlock retention, and model
  application. The local seam does not elaborate the top-level ioctl decoder, its exact
  reset expression, PPI/PSG restore, or model application; Quartus and hardware remain the
  integration gate for those boundaries.

### CF-6 — Locked MRER and unlocked RMR2 shared an unsafe classic/Plus ownership seam

The hardware report's AmstradDiag fragment during Burnin' Rubber is supported by a direct
source mechanism. Both the Plus MMU and the concurrently active classic Gate Array path saw
`101xxxxx`. After ASIC unlock, the Plus MMU treated it as RMR2 while the legacy path still
treated it as MRER, altering classic ROM enables behind the relocated cartridge window.
Burnin' Rubber contains the matching RMR2 relocation/copy/jump sequence, so this is a strong
causal explanation rather than generic ROM-leak speculation.

**Required fix and exit:**

- [x] Export one authoritative ASIC lock state and make locked `101xxxxx` the MRER alias while
  unlocked `101xxxxx` belongs exclusively to RMR2.
- [x] Suppress the classic onboard-ROM decoder in Plus mode; unclaimed Plus addresses expose
  base RAM while cartridge windows remain owned by `plus_mmu`.
- [x] Pin locked MRER disable/re-enable and unlocked RMR2 page/position in the MMU suite;
  `r04` separately proves unlocked RMR2 cannot alter legacy GA mode or ROM enables. The
  motherboard suite remains the elaborated integration regression.
- [ ] Add a production real-CPU relocation fixture that copies from a cartridge page to RAM,
  relocates the low window, and proves the jump target. The attempted title harness is not a
  trustworthy oracle yet.
- [ ] Trace and decide the residual Dandanator seam: the legacy `ga40010` ROM-enable output
  still reaches `CPC_Dandanator.nRomEn` in Plus mode even though the motherboard's ordinary
  mode, interrupt, and onboard-ROM consumers use the Plus ASIC path. Do not change this until
  the intended Plus/Dandanator compatibility rule is established and pinned.
- [ ] Re-test Burnin' Rubber and record the exact RBF and expansion-ROM inventory.

### CF-7 — HF-1's unified FDC equations regressed the classic partial decode

HF-1 correctly added 6128+ firmware aliases, but applying its Plus A9/A4 qualifications to
classic mode changed the upstream A10/A8/A7 decoder. That is a source-confirmed classic
regression and a plausible cause of classic titles reaching the wrong FDC port, including
the reported `The Demo` result if its first access uses the classic aliases.

**Required fix and exit:**

- [x] Use one shared production/test module with separate classic and Plus truth tables.
- [x] Restore classic A10/A8/A7 partial decode and retain 6128+ `&FADD`/`&FBDF`, PlayCity,
  Kempston-mouse, model-capability, and menu-disable cases.
- [ ] Trace `The Demo` to its first motor/status/data access and complete one real-u765 DSK
  transaction before attributing the hardware symptom to the decoder alone.

### CF-8 — SSCR vertical wrap changed RA without advancing the displayed row base

The prior P6 path offset the exposed low three RA bits but continued updating VMA' from the
raw raster comparison. With ordinary R9=7 and non-zero vertical scroll this repeats the old
row base on the wrapped RA=0 scanline, producing one discontinuity per character row. That
matches the choppy scroll reported in Navy Seals and World of Sports.

**Required fix and exit:**

- [x] Use effective RA for the row-end VMA latch comparison while leaving the CRTC counters,
  PRI, and split-line comparisons on raw raster state.
- [x] Pin offset-1 raw raster 0..7, the wrapped RA=0 line, and the next row's stable MA in
  `t08i`.
- [ ] Resolve the reference conflict for R9>7 before broadening the model: the SSCR register
  description says to add the scroll value to the low three RA bits, while its later summary
  approximates the result as a five-bit addition. The current RTL follows low-three-bit
  modulo-8 scrolling and is only claimed for the ordinary R9=7 case pinned by `t08i`; use a
  hardware discriminator rather than selecting the other reading from simulation output.
- [ ] Re-test Navy Seals and World of Sports. Treat the Navy Seals bullet trails as a separate
  sprite/background first-divergence problem unless the same build removes them.

### CF-9 — Frame-origin consumers disagreed, and ACCC v1.11 is internally ambiguous

The video-pointer reload path uses `C4=0`, `C9=0`, and `C0=0`, while the status-2
16-frame timer's `frame_origin` originally used only `C4=0` and `C0=0`. With ordinary
R9=7 that advances the timer eight times per frame, despite the comment claiming the two
signals identify the same edge.

ACCC v1.11 section 20.3.4 p.243 contains both readings on the same rendered page: its
opening rule explicitly says the pointers initialize when C4, C9, and C0 change to zero,
but its closing summary says they load when C4=0 and C0=0, omitting C9. The remediation
aligns `frame_origin` with the opening sentence and with the existing pointer-reload model;
that is a documented model choice, not yet hardware confirmation.

**Required fix and exit:**

- [x] Make pointer reload and `frame_origin` use one named frame-origin condition and pin
  that status bit 3 advances once, not eight times, for an ordinary R9=7 frame.
- [ ] Resolve the p.243 internal conflict with the ACCC author or a real CRTC3/4 status-2
  capture before claiming the C9 condition as silicon-established behavior.

## 4. Production coverage gaps and likely compatibility causes

These are real design/test gaps, but they need a title trace or hardware discriminator before
the associated RTL behavior is changed.

### CG-1 — “P0 boot integration” is not a CPU or top-level boot test

`sim/plus/p0_boot_test_top.v` instantiates parser/service/MMU pieces, forces Plus mode, and
disables I/O. It does not instantiate `Amstrad.sv`, T80, Gate Array, FDC, or video. The
motherboard bench replaces T80 with a scripted fake CPU. Existing coverage proves atomic
publication and byte readback, not reset-vector execution, AMSDOS selection, or a real
title's I/O sequence.

**Required harness:**

- [ ] Add an actual T80 production-top-level harness covering CPR parse, commit, automatic
  reset, first opcode fetch, cartridge page selection, and bounded CPU progress.
- [ ] Observe PC, M1/MREQ/RD/WAIT, cartridge page/ownership, MRER/RMR2, ASIC unlock, CRTC
  writes, interrupt request/acknowledge, and FDC selection.
- [ ] Run one tiny deterministic fixture first, then a bounded trace from a real BASIC/System
  cartridge and Panza. A title screen is not the assertion; the first divergence is.

### CG-2 — Every cartridge byte currently incurs an SDRAM WAIT round trip

The architecture explicitly records that cartridge fetches run slower than real ROM. A WAIT
is functionally legal to the Z80, but stretching every opcode and operand fetch changes the
relationship between instruction execution, raster interrupts, DMA, and video timing. Titles
that copy code to RAM early may therefore behave differently from titles that execute in the
cartridge windows.

**Required discriminator and likely fix:**

- [x] Use the production CPU trace to measure a sustained cartridge window before changing
  the memory path: 4,096 64-MHz ticks produce 38 M1 fetch phases, 73 physical reads, 803
  WAIT-low ticks, and an 11-tick maximum stall in the deterministic P10 harness.
- [ ] Compare equivalent ordinary-RAM or title progress. The current harness hardwires the
  no-wait RAM side and cannot provide that like-for-like result.
- [ ] If the trace confirms timing sensitivity, serve cartridge reads through the normal CPU
  SDRAM slot or a synthesis-affordable cache/window rather than a serial request round trip
  for every byte.
- [x] Pin reset-vector, upper-page data, and sustained-cartridge-execution timing with a
  deterministic test; retain fail-closed behavior during load/clear and classic isolation.

### CG-3 — The sprite engine uses an undocumented staging/bandwidth model

`asic_sprites.v` invents a single registered fetch server and two staged row banks because
the real ASIC's internal sprite-RAM access is undocumented. CPU pixel access can poison or
invalidate staged data and force a row re-read. The reference instead establishes only that
the accessed sprite disappears for roughly the access duration; its stored image is not
corrupted. Current motherboard coverage pre-fills static sprite RAM and does not reproduce
RoboCop-like animation bursts.

**Required discriminator and likely fix:**

- [x] Add a sustained 16-pixel burst (`s15`) and a one-cycle access colliding with a delayed
  pre-write ACK (`s16`) in `sim/plus/asic_sprites_test.cpp`.
- [x] Add the remaining production-cadence discriminator: `s17` overlaps all 16 sprites in
  one early window under concurrent CPU animation writes, changing row/Y/magnification, and
  the real leaf memory-grant cadence; a unique emitted marker proves every staged row ready.
- [x] Implement coherent CPU pixel write-through into matching staged buffers in `rtl/plus/asic_sprites.v`
  and eliminate destructive bank invalidation cache flushes; emulate only the documented per-access blanking hole.
- [ ] Confirm the access-blanking duration and staging model on hardware; until then keep
  them documented as implementation assumptions, not ASIC truth.

Two source conflicts require hardware discrimination:

- writing sprite slot offset `+3`: Y-high only versus a magnification mirror;
- horizontal coordinate formula: the source's `X>>3` comparison conflicts with its stated
  mode-2 visible range and “repeat after 64 characters” behavior. Do not apply an 8/16-dot
  scale change from one sentence alone.

### CG-4 — Panza requires a first-divergence trace, not a guessed CRTC patch

Grey active area plus blue border shows that sync/video output and part of palette setup are
alive. Remaining high-impact named assumptions include lowered-R0 behavior, the R3-low-zero
end/start collision, PRI timing, the two-microsecond mode latch, INKR power-up, and exact
pixel phase. More documentation may settle some, but title traces and focused hardware
discriminators are more useful than broad source collection.

**Required discriminator:**

- [ ] On a timing-clean RBF, trace Panza from reset until the first divergence in PC/page,
  ASIC unlock, MRER/RMR2, interrupt count/ack, or CRTC/ASIC register programming.
- [ ] Turn that divergence into one focused vector with an expectation from a primary source,
  MAME/WinAPE trace, or real Plus capture—not from current RTL.
- [ ] Keep each resulting CRTC3, PRI, MMU, or pixel-phase correction in its own commit.

## 5. Assessment of classic ROM interference

The original broad overwrite theory remains too broad: a `plus_cart_own` read has exclusive
data-mux priority, and high-window cartridge page 3 is intentionally Plus AMSDOS. However,
the post-review investigation found a narrower real ownership leak at the MRER/RMR2 seam.
Unlocked RMR2 changed the Plus cartridge window and the classic GA ROM-enable state on the
same write, letting the classic onboard-ROM path answer after relocation. CF-6 records the
source mechanism and provisional fix.

Expansion-ROM persistence and OSD visibility are a separate state/UI problem. They can make
the configuration surprising, but no source trace currently shows an expansion slot
overwriting a cartridge-owned read. Record the active ROM inventory on every retest rather
than collapsing these two mechanisms into one claim.

## 6. P10 ordered execution plan

Keep these as separately reviewable commits or sub-milestones. Do not combine confirmed
PPI/FDC repairs with speculative sprite/video changes.

1. **P10a — reproducible timing-clean baseline and real-CPU harness**
   - Dispatch `local-build.yml` with `effort=full` when the Quartus VM is online, otherwise
     hosted `build.yml`; require non-negative constrained-domain setup/hold slack and zero
     TNS, then record the external-path limitation and artifact identity.
   - Add the real T80/top-level CPR boot harness and baseline traces.
2. **P10b — Plus PPI Port C physical-output repair**
   - Implement CF-1 and its keyboard production-path regression.
3. **P10c — model capabilities and FDC reset/alias integration**
   - Implement CF-2/CF-3; reproduce BASIC with a known-good DSK under 6128+.
4. **P10d — cartridge execution timing**
   - Use CG-2's trace to decide normal-slot versus cached cartridge reads.
5. **P10e — DMA/PPI/PSG arbitration**
   - Implement CF-4 with the Arnold concurrency vector.
6. **P10f — RoboCop sprite dynamic-write closure**
   - Capture first, then replace the staging behavior only where the trace proves it wrong.
7. **P10g — Panza CRTC3/interrupt/video first divergence**
   - Close one sourced behavior per commit.
8. **P10h — production CPC+ SNA repair**
   - Keep the implemented CF-5 production repair and parser/register/MMU seam green; add the
     remaining full-top snapshot test or equivalent Quartus/hardware discriminator.
9. **P10i — timing-clean hardware matrix and status reconciliation**
   - Re-run the checklist, record every result, and promote only passing items to
     hardware-confirmed.

Every RTL/simulation change runs `make -C sim` and `make -C sim lint`. Clock, WAIT, memory,
RGB, and top-level arbitration changes also require a full-effort exact-tip Quartus build.
A non-trivial diff receives fresh independent review; unresolved ASIC behavior remains named
as a model assumption instead of being converted into a passing self-derived test.

## 7. Hardware report fields for every retest

Record these alongside each cartridge result:

- integration commit and RBF filename/SHA-256;
- full or smoke fitter effort, worst setup/hold slack, and resource utilisation;
- MiSTer framework/core version;
- Plus model and classic Model/CRTC settings;
- CPR filename and file hash;
- DSK/CDT filename and whether it was mounted before or after CPR load;
- cold boot, OSD reset, or hot cartridge replacement;
- first visible failure and, where possible, first trace divergence;
- comparison source: real Plus, MAME/WinAPE, documented diagnostic expectation, or merely the
  previous core.
