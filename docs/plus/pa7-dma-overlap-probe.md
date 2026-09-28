# PA7 DMA / compatible-interrupt overlap probe

Status: cartridge built; production-T80, counterfactual, MiSTer and AmSpirit runs completed.
Independent implementation and final-delta reviews passed; repository gate passed.
Base: `7f12f1c653085270f598be869eed20ee7f603349`. No production RTL change.
Original hardware has not run this probe.

## Question and experimental boundary

V5 establishes the software-visible compatible interrupt timing, not where
inside the ASIC the request becomes pending. This companion asks whether an
acknowledge of an already-pending DMA interrupt suppresses the compatible
request near its boundary. A later request-creation mechanism and a shifted
internal HSYNC can give identical results here; they are not separately
identified merely by counting interrupts.

The actual wiring matters: `asic_regs` chooses vector and DMA auto-clear from
`int_pending = ~plus_ga_int_n` (delivered raster). `asic_ga_timing` clears its
raw pending latch while `intack | irq_reset | irqack_rst` is active. Its
raw-qualified `irqack_rst` can hold until M1 releases. DCSR bit 7 records the
first ASIC acknowledge's raster provenance. The board shapes IORQ using
physical READY and A13; an A13=0 interrupted instruction may corrupt vectors.
The diagnostic must therefore record vector and DCSR separately and retain
all interruptible instructions at A13=1.

## Preconstruction reachability evidence

A passive copy of the existing V5 production-T80 fixture added ASIC ACK,
`int_ack_active`, M1 and compatible-delivery taps. The unchanged V5 screen34
CPR ran for 15 frames. Example reference ACK: tick 26231802 through 26231842,
from C52 dot 12 to C53 dot 6. The compatible ACK also ended at dot 6 with all
clear terms released. These are model timing observations, not original-ASIC
pin measurements. Build/script/trace: ignored
`output_files/pa7-dma-overlap/reachability/`.

Raw HSYNC falls at dot 0. Current raw creation follows by about 1 master tick;
delivery follows at the next character edge. An ACK ending at dot 6 is about
24 ticks after the raw edge and about 40 ticks before later creation. A
whole-character translation of a pending-DMA acceptance can therefore put
ACK release strictly inside that interval. There is a reachable observation
window; final software must prove it actually lands there. EI inhibition,
WAIT alignment, M1 release and any held clear term are acceptance criteria,
not assumed from the requested delay number.

## Predeclared conditional predictions

Let E be early creation, L be the later creation/delivery edge, and [a,b]
the complete effective clear interval, including a held `irqack_rst`.
D denotes a DMA0 vector 04 and R a raster vector 06. DMA overlap cases start with
PRI=0 and a DMA0 INT|STOP request pending under DI; other channels are off.
No-DMA controls omit that request; DMA-only uses PRI255 to mask compatible IRQ.
The counter boundary is far from VSYNC. ISR recording reads DCSR before
clearing DMA and does not clear DMA on a raster acknowledgement.

| Timing class | Early raw pending + delayed delivery | Later creation, same ACK-clear policy |
|---|---|---|
| Clear interval finishes before E | D then R | D then R |
| a<E<b<L, with clear fully inactive before L | D only (early event suppressed) | D then R |
| E<a<L<b | D only | D only if late event is suppressed during ACK |
| First ACK after both sources are delivered | R then D | R then D |
| No DMA, EI before boundary | R only | R only |
| No DMA, EI after boundary | R only | R only |
| DMA only, compatible masked by PRI255 | D only | D only |

The interval must be measured in the final trace. A start in (E,L) alone does
not establish a discriminator. A silicon policy that preserves compatible
creation during a DMA ACK can also produce D,R: that result would reject the
current cancellation behavior without uniquely proving later creation.

With automatic clear (IVR bit 0=0), DMA0's post-ACK flag is predicted clear;
with manual clear (bit 0=1), DCSR bit 6 remains set until the handler clears it.
Expected status pairs are D/00 then R/80 for early automatic cases, D/40 then
R/80 for manual cases, and R/C0 then D/00 (automatic) or D/40 (manual) for
late cases. The first arming raster may leave pre-window bit 7 set; pre-window
bit 6, not bit 7, establishes DMA pending. These status predictions are model
policies, including the known unresolved auto-mode DCSR readback question.

## Cartridge and validation contract

The companion preserves V5 and reuses its existing
sjasmplus, font, boot and screen conventions. It pre-pends DMA0 using INT|STOP
many lines before the boundary so execution cadence cannot confound the
cancellation sweep. An early PRI arming interrupt establishes the phase, then PRI0
precedes MRER reset. Runtime delay/acceptance and all ISR return tails live at A13=1.

Sweep several whole microseconds around the boundary; compare NOP and
LD A,(HL) acceptance, automatic/manual clear, and HSYNC widths 8/12.
Include no-DMA early/late and DMA-only controls. Record source order, count,
DCSR per interrupt, pre/post DMA status and repeat disagreements. Bound the
observation well below the next 52-line interrupt and flag excess interrupts.
Screen results must be observations, without a hardware PASS/FAIL verdict.

Before release, verify actual CPU/ASIC ACK pulses, no doubled ACK, DMA cadence,
PRI/MRER ordering, pending flag before EI, all clear-release margins, bounded
observation and stable repeat results in production T80. The secondary
instruction family is useful only if its acceptance phase actually differs.
A shared internal HSYNC shift remains equivalent to later creation unless
an additional independently anchored consumer measurement separates them.

Vector safety follows the current IC116/READY/A13 equation in
`Amstrad_motherboard.v` and `asic-reference.md` §7. All interruptible code
and the I register must retain A13=1.

## Independent design review, before construction

Opus5.5 medium reviewed the sources and proposed experiment in guarded run
`20260928T142827Z-67555-c8eb`: mechanism isolation supported; validation
conditions retained below. The review could not read the old private traces
from another checkout. The parent independently collected the passive trace
above in this checkout. Full report: ignored
`output_files/pa7-dma-overlap/review/design-opus.txt`.

READY fixes ACK release phase. Changing the post-EI instruction changes ACK
width, not the sub-character release phase. Measured widths on unchanged V5 screens 30/33 are 56 ticks for
NOP and 72 for LD A,(HL), agreeing with the independent source arithmetic.
Both release at dot 6 (E+23 in the straddling slot). RET NC
was measured at 40 ticks but is unnecessary for the primary discriminator.
All clear terms release with M1/IORQ in the observed A13=1 case.

The review requires a no-DMA acceptance anchor. Immediately after EI and its
following instruction X, a single `LD (HL),E` writes a RAM marker from 0 to 1.
Each handler records that marker. Define s* separately per page as the first
padding value where the no-DMA handler runs before that store (marker 0).
This anchors the lost-slot location to CPU-visible compatible delivery;
the absolute position of a loss band alone is insufficient.

| Hypothesis | s*−4 | s*−3 | s*−2 | s*−1 |
|---|---|---|---|---|
| H0 current early raw / delayed delivery | D,R | D | D | R,D |
| H0rv early raw also visible to vector selection | D,R | D | R,D | R,D |
| H2 later creation, suppressed during ACK | D,R | D,R | D | R,D |
| H4 shifted internal HSYNC feeding counter | same as H2 | same | same | same |
| H1 compatible request survives DMA ACK | D,R | D,R | D,R | R,D |

These predictions are declared before companion simulation. A creation edge
on either side of the fixed ACK-release threshold can change the class;
software does not reconstruct a continuous electrical delay. H1 also includes
later creation that is retained during ACK. H2 and H4 remain observationally
equivalent in this pre-pended-DMA experiment. DMA-only is a cadence/STOP
control, not an independent measurement of a shared HSYNC shift.

Disposable counterfactuals validate the distinction: `late` delays the
shared internal HSYNC path by one character and removes the compatible output delay;
`retain` preserves compatible pending/creation during a DMA-classified ACK.
They change only generated fixture copies. Neither is a proposed RTL repair
or an assertion about real silicon. The software anchor must remain coherent
in all models and each claimed class must produce visibly different records.

The implementation has four pages (W8/NOP automatic, W8/LD
automatic, W8/NOP manual, W12/NOP automatic), 17 rows each. Three controls
(DMA early, DMA late, DMA only), six no-DMA anchor paddings, eight DMA sweep
paddings. Eight repetitions record first observations and disagreements.
The ISR reads DCSR before clearing and uses its vector stub, not DCSR bit 7,
to decide whether it should clear DMA. This avoids making the known uncertain
auto-mode DCSR readback policy control the experiment.

The far-early control acknowledges DMA about 25 lines before the boundary
(padding −1600µs, counter below 32). The earliest sweep rows still acknowledge
near count 51. Under the current raw-qualified counter ACK wiring both give
D,R. If hardware instead clears counter bit5 on every DMA ACK, the near-early
rows may postpone compatible IRQ outside the observation window while the
far-early control remains D,R. That is a counter-policy finding, not proof
of the delayed-pending mechanism. The fixed observation ends only a few
lines after the target52 event and before the next one even in this control.


## Original 6128 Plus run

Build with `python3 scripts/diagnostics/pa7_dma_overlap.py --start 0` (sjasmplus on PATH).
The default output is `output_files/pa7-dma-overlap/cartridge/pa7-dma-overlap-start00.cpr`.
The validated release candidate is in `output_files/pa7-dma-overlap/production/`:

- Start-00 CPR SHA-256: `0f499bc6c50f935b0f9a455ec953db44a97a6a2db5da57aa215914c80c7c7106`.
- Four pages in one 16 KB cartridge; 5,283 program bytes. The start-01/02/03
  variants change only the initial page and are convenient for automated captures.
- Delay base 3294 NOP slots; width 12 adds four slots. The calibrated position
  places the anchor inside the displayed sweep; it does not alter the predictions.

1. Load start-00 on the original 6128 Plus and wait about four seconds for `DONE`.
2. Photograph the whole page, including its title, controls, all rows and footer.
3. Press and release any key or joystick fire, wait for `DONE`, and photograph
   the next page. Collect all four: W8/NOP auto, W8/LD auto, W8/NOP manual,
   W12/NOP auto. Another press wraps to page1 and reruns it.
4. Keep unexpected values, nonzero DIFF/FL, or a stuck RUNNING screen in the report.
   Report the cartridge hash and machine. No software PASS label interprets hardware.

Every number is hexadecimal, except signed row labels (microseconds). `N` is
IRQ count. `V1 S1 M`, `V2 S2 M`, `V3 S3 M` give ordered vector, DCSR status and
marker at handler entry. V04 is DMA0; V06 is raster. Marker0 means before the
post-EI marker store, marker1 after it; unused entries are FF/FF/F. PRE/POST
are DCSR before EI and after the bounded observation. DIFF counts repetitions
that differ from the first full observation; REP must finish at08. FL bit 0
means more than three interrupts, bit1 means an unexpected source. The fourth
interrupt disables further interrupts to bound a storm. An unexpected vector
outside the four even ASIC sources is caught through a 3333-filled IM2 table;
this is not a guarantee for arbitrary bus-corrupted odd vectors.

Interpret the DMA sweep relative to each page's own no-DMA marker transition
s*, not to an assumed absolute offset. First check the far-early, late and
DMA-only controls. A failed control or missing anchor prevents classification
by the mechanism table. DCSR readback remains an independent observation:
cleanup is selected by the vector handler, never by bit 7.

## Model validation, 28 September 2026

Commands (all paths relative to this checkout):

```sh
python3 scripts/diagnostics/pa7_dma_overlap_sim.py --pages 0 1 2 3 --out output_files/pa7-dma-overlap/production
python3 scripts/diagnostics/pa7_dma_overlap_sim.py --hypothesis late --pages 0 --out output_files/pa7-dma-overlap/late
python3 scripts/diagnostics/pa7_dma_overlap_sim.py --hypothesis retain --pages 0 --out output_files/pa7-dma-overlap/retain
```

The production fixture uses GHDL's production T80 netlist and physical READY/WAIT
clocking. Each page finishes after 158 frames including boot/render settling.
All 17 rows repeat eight times without tuple disagreements or anomaly flags.
No-DMA anchors are 0, −1, 0, 0 respectively. Production observes H0; generated
`late` observes H2/H4; generated `retain` observes H1, exactly as predeclared.
The latter two use the identical start-00 CPR hash and preserve the same s*=0.
Their differing counts are visible on screen, not just in internal taps.

Passive full-run traces establish 336 CPU/ASIC ACK pulses per production page,
with one ASIC pulse per CPU ACK and A13=1 at every interrupted PC and ACK
address. Every enabled trial executes exactly one DMA0 INT|STOP before EI;
no-DMA trials execute none. All measured ACK clear terms and M1 release at
the ACK end. Counterfactual traces have 344 ACK pulses (`late`) and 352
(`retain`), again one ASIC pulse per CPU ACK. Their registered vectors settle
by ACK+1 master tick and remain unchanged for the rest of the pulse, including
when compatible delivery matures during a retained DMA ACK. The final vector
matches the recorded handler source in every observation. At s*−3, the NOP interval is E−33 through E+23 master ticks;
LD A,(HL) is E−49 through E+23. Late creation is E+63, leaving 40 ticks of
clear-free margin. Width 12 translates the same relation by four characters.
The measured widths are 56/72 ticks. The target event is around line 59, away
from VSYNC at 240 and the next compatible event near 111. The finite timing
buffer ends before either can contaminate the observation.

One-shot checks and full traces are retained under ignored
`output_files/pa7-dma-overlap/validation/` and `production/`, plus `late/` and
`retain/`. `check_results.py` checks controls, anchor monotonicity, source/status
predictions, preconditions, eight repeats and unused slots. `check_trace.py`
checks pulse correspondence, A13, one-shot DMA cadence and clear margins.
These validate this experimental artifact; they do not assert silicon rules
or add a permanent simulator bench. Existing V5 source and cartridge remain
unchanged; the final fixture's 15-frame V5 screen30 run reproduced the original
passive trace fixture's image byte for byte. The final companion's production-T80
key-navigation run completed page0→1 at frame315; both its PPM and complete
RAM record JSON match independently starting page1 byte for byte.

## MiSTer and AmSpirit observations, 28 September 2026

MiSTer ran all four start variants on
`/media/fat/_Computer/Amstrad_20260928_1346f39.rbf`, SHA-256
`680940ac1d03d6195bccfe33fc15178a7a83213308655070bb88ff943b61d523`.
There are no Quartus-source changes between that RBF's source and this probe's
base `7f12f1c`. Each run pins RBF and CPR hashes in its hardware-loop manifest.
Saved configuration is 6128+ / Full, SHA-256
`13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`;
it was observed before and after, without modification.

Three native captures per page were identical. Each visible 640×200 screen
region is pixel-identical to the corresponding production simulation image
(after accounting for the capture's 40-row vertical origin). All four pages
show DONE, REP08, DIFF00 and FL00. This verifies the probe on the FPGA build;
it is not an original-ASIC result. Cases, manifests and twelve captures are
in ignored `output_files/pa7-dma-overlap/mister/`.

AmSpirit Lite 1.15.1/core 2491682 ran all four pages as 6128+ / CRTC3 / 128 KB,
monitor Off. Saved SNA RAM confirms DONE, eight stable repeats, no anomaly
flags, and the same no-DMA anchors 0/−1/0/0. Its DMA-only control receives no
interrupt while DMA0 remains pending (PRE/POST=C0). All DMA sweep rows receive
raster then DMA; even the far-early control does so. In automatic mode the
first raster handler reads 40 and the following DMA handler 00; POST readsF0.
Manual mode reads 40 for the DMA handler and POST=B0. These control failures
mean AmSpirit cannot classify the pending/delivery mechanism with this run.
They are emulator observations, not evidence to retune the probe or change RTL.
A single start-00 cartridge also completed the page0→1→2→3→0 fire-button
sequence; every navigated screenshot matches the independently started page.
Cases, identity/settings, screenshots, SNA snapshots and extracted records are
in ignored `output_files/pa7-dma-overlap/amspirit/`.


## Review and acceptance

Opus5.5 medium independently reviewed the complete implementation in guarded
run `20260928T151242Z-77964-9260`, with no blocking correctness defect. Its
hardware robustness suggestion was applied: restore PRI2 under DI after the
observation window, so the DMA-only control cannot leave a later PRI255
request to contaminate arming. The timing path inside the window is unchanged. The one-shot trace comparison
records eight post-window PRI255 assertions before this change and zero after it.
Progress-column, metadata, catchall-comment and counterfactual-output fixes
were applied together. The focused fresh final-delta review
`20260928T152245Z-80472-8d58` reported **no findings**, no edits and no tests.
Both reports are in `output_files/pa7-dma-overlap/review/`. All delivered
captures and results use the final CPR hash above; earlier runs are retained
separately under `pre-review/`.

Final repository gate, after the last code edit:

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
```

Exit 0; final line: `select_tests: no simulation needed`. Diagnostic scripts
are outside the standard bench triggers. The dedicated production-T80 runs,
conditional-model checks and physical FPGA captures above provide the relevant
verification. No production RTL changed and no soak hash was re-minted.
Original Plus results remain pending. The task stops at locally committed
READY: no integration, push or release promotion.
