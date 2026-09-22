# Sonic rearm boundary and no-input controls

Diagnostic work from published `8e449ac23298d185e7c03b9cd5df3205a1ece70e`.
Production RTL and cartridge bytes are unchanged. The earlier
[phase discriminator](phase-discriminator-2026-09-22.md) remains the source for
336-line baseline, 313-line terminal-PAUSE candidate and 312-line AmSpirit
recurrence. The candidate's partial timing improvement and failed hardware
acceptance are separate observations.

## Hardware progression controls

Four normal cartridge boots compare `c59e03a` against experimental `a137d48`,
with no input and with a two-second fire hold. Parent inspection of every native
PNG found:

| RBF | No-input control | Sustained-fire control |
| --- | --- | --- |
| `c59e03a` | Corrupt title at first checkpoint; later playfield, ring HUD and lives | Corrupt title checkpoint; later Green Hill Zone / Act 1 card and playfield |
| `a137d48` | Magenta/corrupt title remains at both later checkpoints | Corrupt title remains at all three later checkpoints |

Baseline therefore reaches a playfield presentation without fire. Older fire
runs reaching gameplay do not by themselves prove that fire was accepted.
The candidate's progression difference is reproduced without an input window;
a short missed fire pulse is insufficient to explain this comparison. These
captures do not distinguish attract mode from user-controlled gameplay, establish
a cause, or score a pixel-level rendering improvement.

Each title checkpoint was requested by the existing 18-second boot recipe.
No-input captures were requested 8 and 16 seconds after the driver's return.
Fire controls used F18, left Ctrl held for two seconds, and F20; subsequent
captures were requested 2, 8 and 16 seconds after replay completion. The saved
replay logs establish submitted transitions and owned releases, not cartridge
acceptance. Inspecting the title image delayed replay start by 15.86 seconds on
baseline and 19.96 seconds on candidate after the title driver completed.
Consequently, the title scene is identified at the captured checkpoint, **not
verified at the exact input moment**. These are bounded procedural comparisons,
not frame- or instruction-phase-matched acquisitions. The no-input pair is the
stronger progression discriminator.

Both runs use the original CPR SHA-256
`4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae`.
RBF SHA-256 values, checked on the device:

- `c59e03a`: `da28d0cd01c910ce68d7bfc7482730dbdb1ae235dc814894776f3f06308e3335`
- `a137d48`: `c0293860c0c9a3d3a5c6a033cce6b7910e49a8abef0e40b3fe2cc5e514fe4984`

The written CFG explicitly requests 6128 Plus (`[34:33]=2`) and Sync Full
(`[36:35]=0`); remote hashes verify the file applied before loading. Native
screenshots omit OSD, so active model/Sync readback was not independently
verified. Raw CRTC sync, filtered display and capture timing are not equivalent
measurements. The runner uses an exact copy of transport driver `210eca4`, with
non-multiplexed SSH/SCP, and records that copy's hash.

After every run, MENU and the original CFG were restored. Final independent
preflight confirmed MENU, original CFG SHA-256
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`,
no B17 temporary map and no task-owned temporary replay files. Both replays
completed their releases/F20. MiSTer ownership was released to the coordinator.

## AmSpirit acquisition limits

Four fresh normal boots on Lite 1.15.1/core 2491682 missed their requested
FBF9/F922/F927 breakpoints within bounded 90-second windows. No new rearm timing
measurement follows. Frames advanced, and one endpoint screenshot showed a
playfield, so these misses do not demonstrate an emulated CPU stall.
The probe's continuation checks raced the asynchronous resume transition;
its `continued:false` results are invalid and must not be cited as failures of
Sonic continuation. The earlier successful 44-handler run remains the accepted
recurrence reference; these failed acquisitions neither replace nor invalidate it.

Small breakpoint mechanism controls on BASIC worked for paused/running arming
and a reset without media replacement. Their success does not settle the
cartridge-load case. No imported SNA or wrong-base RAM dump is used as a timing
oracle. The worker restored the saved configuration/render before its original
full SNA, cleared breakpoints and preserved the paused state. Parent inspection
confirmed the restored BASIC Ready image; its hash matched the original. AmSpirit
was released. The user's later relaxation removes any need to repeat original
state restoration in future AmSpirit work.

## Production-T80 rearm boundary

A fresh normal-boot diagnostic substitutes only the exact experimental
`a137d48:rtl/plus/asic_dma.v` into scratch generated inputs. Production source
remains unchanged. The production-T80 D5 route retains production clocking and
WAIT, behavioral SDRAM and the unused classic-GA stub. Master ticks are 64 MHz;
a programmed scanline is 4096 ticks. This is model integration evidence, not a
physical CPC Plus timing measurement.

The trace reaches the final DMA2 INT, captures one complete rearm and two further
scanlines, and terminates with all required markers and `STOPWINDOW_END`.
The following ticks identify the consumed update or registered event explicitly:

| Event | Master tick |
| --- | ---: |
| DMA2 INT, SAR2 `83D0` | 653629038 |
| Acknowledge begins / ends | 653629090 / 653629146 |
| FBF9 instruction fetch starts | 653629682 |
| F922 instruction fetch starts | 653632818 |
| HSYNC selects enabled DMA2, pause zero, SAR2 `83D0` | 653632963 |
| ST_DEAD samples `83D0` into RAM address | **653633006** |
| STOP `4020` fetched; SAR2 advances to `83D2` | 653633070 |
| SAR low-byte write first consumed: `83D2` → `835A` | **653633091** |
| STOP pulse / DMA2 enable clears | 653633134 / 653633135 |
| SAR high-byte write first consumed: high byte remains `83` | 653633155 |
| F927 instruction fetch starts | 653633394 |
| DCSR write reenables DMA2 | 653633667 |
| Next ST_DEAD samples `835A` | 653637102 |
| PAUSE 42 fetched / pause counter becomes 42 | 653637166 / 653637230 |

The decisive address-sampling edge is 85 ticks (1.328125 µs) **before** the low
SAR update. Because the old and new high bytes are both `83`, the low-byte
consuming edge is the first point at which the complete desired pointer exists;
waiting for the high-byte store overstates this particular deadline miss. STOP
was already fetched before either CPU store could redirect it. Its enable clear
is subsequently followed by a successful DCSR reenable. This is a missed
scheduling opportunity, not evidence of a lost reenable write or a STOP/write
collision requiring a priority repair.

FBF9-to-F922 fetch starts are 3136 ticks apart, or 49 µs. In that half-open
interval, `plus_mem_wait` and `cart_stall` have the same 25 pulses of 17 ticks:
425 ticks (6.640625 µs) of overlapping assertion. Do not add them. No
`dma_ppi_wait` transition occurs anywhere in the captured window, and sampled
fetch states report it zero. Cartridge wait activity is therefore present in
the CPU rearm path; DMA/PPI wait is not observed here. Stall-high duration is
**not** the net execution delay after CPU enable-phase alignment. It cannot
be subtracted from 49 µs to construct an unwaited CPU timing result.

The earlier single AmSpirit witness has beam X 144 at FBF9 and 800 at F922.
Its beam units and instruction sampling phase have not been independently
calibrated. An inferred 41 µs reference, and hence an alleged 8 µs CPU excess,
are not established measurements. A matched CPU/memory timing control or an
explicit scratch counterfactual is the next useful discriminator. Moving the
whole DMA schedule shifts both its INT and address-selection opportunities;
that alone is not evidence of an enlarged rearm interval. This trace does not
justify another DMA behavior change, nor prove that initial phase is irrelevant
to the separate split/rendering problem. Coordinate any future DMA proposal with
the B20 owner; live-PPR semantics remain separately owned.

### Trace validity

Independent Astra review of the Muse-authored taps found no blocking width or
Harness-cycle defect. It checked that an ordinary Harness tick contains one
rising clock edge and SP is `cpu_reg[63:48]`. Important interpretation limits:

- CPU `FETCH` events identify read starts. Their `op=` data is sampled before
  read completion and is not executed-opcode evidence.
- Generic `WRITE_BEGIN/END` uses the fixture's registered write logger, one
  master edge after the underlying bus level. The table anchors exact margins
  to direct SAR requests and observed SAR/enable updates instead.
- ST_DEAD address sampling is identified by state `1 → 4`, active mask `4`
  and asserted RAM request. An address-value transition alone is insufficient
  when consecutive selections use the same address.
- One rearm plus 8192 following ticks is not a two-rearm repeatability check.
  A tick-cap exit would also return zero; the required event markers, not merely
  process success, establish completion here.

No test expectation or production behavior changed. No simulation suite or
synthesis is required for the committed documentation-only diff. The scratch
build and bounded diagnostic execution are separate evidence; neither substitutes
for hardware acceptance. The earlier no-empty-ack/collision observations remain
limited to their captured windows and do not establish the physical board's
acknowledge equivalence.

## Evidence and reproduction

Ignored evidence is under `docs/screenshots/sonic-rearm-2026-09-22/` in this
checkout. `device/control.py` and the four per-run folders retain cases, CFGs,
input schedules, input logs, native PNGs and acquisition/restoration manifests.
`reference/driver-210eca4.py` pins the transport used. `amspirit/` retains the
failed acquisitions, scripts, mechanism controls, report and restoration record.
The private evidence was also preserved under the same relative path in the main
checkout. `trace/` contains source taps, build command/log, binary, raw trace,
provenance and independent-review notes. Rebuild with
`bash docs/screenshots/sonic-rearm-2026-09-22/trace/build.sh` from this checkout
after the existing T80/D5 preparation; run the resulting binary
with the original CPR and cap `1280000000`. Absolute paths in the scratch build
script identify the original assigned checkout and need adjusting after a move.
No copyrighted media, SNA, generated binary or private capture is committed.
