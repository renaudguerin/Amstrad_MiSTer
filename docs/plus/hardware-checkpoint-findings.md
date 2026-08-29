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

- [ ] In Plus mode, drive both physical Port C nibbles from `opc_r` regardless of classic
  direction bits; preserve classic behavior when `plus_mode=0`.
- [ ] Add a focused physical-pin vector for control words `0x9B` and `0x92`.
- [ ] Add a production-path PPI -> PSG register 14 -> HID matrix test that selects at least
  two keyboard rows after reset and after SNA restore.
- [ ] Trace Arnold 5's PPI control writes and confirm the failing interval retains or writes
  an input-direction control word before attributing the hardware symptom to CF-1.
- [ ] Re-run Arnold 5 on 6128+ or 464+ and record which subtest first changes.

### CF-2 — Plus model capability outputs do not control FDC or tape hardware

`plus_model_select` derives `plus_has_fdc` and `plus_has_tape`, and the signals reach the
motherboard boundary, but neither is consumed. FDC select/motor logic in `Amstrad.sv` and
tape/PPI paths in `rtl/Amstrad_motherboard.v` remain unconditional. Consequently GX4000
and 464+ still expose the FDC, while model capability tests prove only decode values, not
machine behavior.

**Required fix and exit:**

- [ ] Gate FDC status/data and motor writes with `!plus_mode || plus_has_fdc`.
- [ ] Gate tape sense/motor paths with `!plus_mode || plus_has_tape` without changing classic
  model behavior.
- [ ] Add a model matrix: 6128+ positive FDC, 464+ positive tape, and negative GX4000/464+
  FDC plus GX4000/6128+ tape cases.
- [ ] Pin the intended open-bus/read result and motor behavior for an absent device.

### CF-3 — CPR/system reset does not reset the FDC or motor latch

CPR download/commit/apply contributes to the system `reset`, but the `u765` instance uses
only `status[0]`. The motor latch has no reset branch. An active command or motor state can
survive cartridge replacement and contaminate a later BASIC/System cartridge boot.

**Required fix and exit:**

- [ ] Define the FDC/motor reset contract for classic OSD reset, Plus CPR apply, and model
  changes; reset both controller state and motor on the selected system-reset event.
- [ ] Add `active FDC command -> CPR load/apply -> first FDC access` coverage.
- [ ] Drive the real AMSDOS aliases `&FADD` and `&FBDF` through a production-level bench;
  HF-1 currently has no such integration vector.

### CF-4 — P7's DMA-versus-PPI/PSG WAIT contract is absent

`docs/plus/architecture.md` requires CPU PPI accesses to wait during a DMA PSG operation and
requires the ASIC to preserve/restore the affected PPI/AY state. `asic_dma` exposes no WAIT
output, the T80 WAIT expression contains only cartridge memory stalls, and production wiring
merely multiplexes DMA PSG writes over CPU writes. The DMA leaf bench uses synthetic RAM and
does not exercise a concurrent CPU keyboard/PSG access.

**Required fix and exit:**

- [ ] Implement the documented CPU arbitration WAIT around DMA PSG operations.
- [ ] Preserve and restore PSG selected-register and relevant PPI direction/control state.
- [ ] Add a production motherboard test with DMA active while the CPU scans the keyboard,
  based on the Arnold 5 DMA/keyboard diagnostic.
- [ ] Verify the maximum stall and post-DMA state against a primary source or hardware trace;
  do not derive the expected delay from current RTL.

### CF-5 — CPC+ SNA parsing is broken at the production top level

This does not explain CPR boot failures, but it contradicts P8 completion and should not be
lost. The system holds `reset` high while `sna_download` is high and passes that reset into
`plus_sna_parser`; the parser therefore cannot consume the CPC+ chunk during the production
download window. Separately, a pending sprite low nibble and a consecutive new input byte
can write the same output registers on one edge, losing the pending nibble. The leaf test
holds reset low and inserts an idle cycle after every input byte, masking both cases.

**Required fix and exit:**

- [ ] Separate parser-download reset from the later machine apply reset, or otherwise define
  a sequencing contract in which the parser can consume the chunk.
- [ ] Add backpressure or a two-output queue so consecutive source bytes cannot overwrite a
  pending sprite nibble.
- [ ] Add a production `Amstrad.sv` CPC+ SNA integration test with consecutive bytes and
  verify PPI, PSG, ASIC registers, palette, sprite pixels, and model application.

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

- [ ] Use the production CPU trace to measure cartridge wait cycles and compare title progress
  before changing the memory path.
- [ ] If the trace confirms timing sensitivity, serve cartridge reads through the normal CPU
  SDRAM slot or a synthesis-affordable cache/window rather than a serial request round trip
  for every byte.
- [ ] Pin reset-vector and sustained-cartridge-execution timing with deterministic tests;
  retain fail-closed behavior during load/clear and classic-mode isolation.

### CG-3 — The sprite engine uses an undocumented staging/bandwidth model

`asic_sprites.v` invents a single registered fetch server and two staged row banks because
the real ASIC's internal sprite-RAM access is undocumented. CPU pixel access can poison or
invalidate staged data and force a row re-read. The reference instead establishes only that
the accessed sprite disappears for roughly the access duration; its stored image is not
corrupted. Current motherboard coverage pre-fills static sprite RAM and does not reproduce
RoboCop-like animation bursts.

**Required discriminator and likely fix:**

- [ ] Capture RoboCop writes to `&4000-&5FFF` and attributes at `&6000-&607F`, including
  timing relative to the failing display lines.
- [ ] Add delayed-ACK, sustained CPU pixel-write, all-16-overlapped, and changing-row/Y/mag
  production-cadence tests.
- [ ] Prefer an independent or true dual-port video-side sprite RAM design if the current
  invalidation model is the first divergence; emulate only the documented per-access hole.
- [ ] Do not silently encode the current stage recovery as hardware truth.

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

The proposed “classic/expansion ROM overwrites cartridge RAM/ROM” mechanism is not supported
by the current read mux. A `plus_cart_own` cycle has exclusive priority and suppresses the
normal SDRAM/main-ROM contribution. On a bare 464+/6128+, cartridge page 3 in the high window
is also the intended AMSDOS source.

The related integration concern is narrower and confirmed: classic peripherals are not fully
isolated because FDC/tape capability signals are unused, and FDC state survives CPR reset.
Investigate that device leakage before changing cartridge-window ownership.

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
   - Implement CF-5 and the top-level snapshot test.
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
