# Eerie Forest: pending CPC interrupt crosses into PRI mode

Candidate Plus interrupt repair on `codex/plus/eerie-late-freeze`, starting from
integrated master `4453b760027f575d6c1c147492c53c1a24cd2de7`. This extends the
[context follow-up](eerie-forest-context-followup-2026-09-23.md). The original
CPR and production T80 remain unchanged. Hardware acceptance is outstanding.

## Earliest context divergence

The previous trace starts too late to explain IX=`0506`. At tick `1538810082`,
the CPU correctly acknowledges an interrupt in an already-established
`DD E9` self-loop, pushes `0506`, and the handler correctly pops it into IX.
The `0628` save at `1538817410/1538817474` faithfully stores that value.
An earlier restore at `1538171162` already uses `0506`.

A new production-clock diagnostic records IX/IY and a rolling bus history,
dumps the first acknowledge whose return PC is `0506`, and stops at the first
`0628` save of that value. It reaches the event after only six simulated
seconds, rather than the visible freeze near 24 seconds:

| Master tick (decimal) | Consumed bus / CPU state |
|---|---|
| `385712602` | Earliest retained record: PC `C218`, PRI=0, IFF1=0, GA interrupt already low, DMA request=0 |
| `385781850`–`385782106` | `LD IX,C22C` at `C225`; the following jump reaches setup tail `04E3` |
| `385783682` | CPU writes `6800 <- 01` on line `113` hex (275); pending interrupt remains low |
| `385784794` | `EI` at `0503`, IX=`C22C`, IY=`0C31`, BC=`023B`, SP=`0066`; interrupt was already pending |
| `385784858` | `JR +0` at `0504` |
| `385785058` | Acknowledge before `JP (IX)`: return PC=`0506`, old IX still `C22C` |
| `386104770/386104834` | First context-save bytes `06E3/06E4 <- 06/05`, IX=`0506` |

This is not an interrupt splitting the DD prefix from E9, nor an incorrect
stack read. T80 accepts the already-pending interrupt after the instruction
following EI. The handler replaces IX with the interrupted PC, losing the
intended `C22C` continuation. The later trace shows how the resulting context
ultimately resumes rendering into low code RAM; the candidate still needs
end-to-end title validation.

A fresh original-CPR AmSpirit run stops at the **first** `0503` exit, at relative
frame 301. It has the same IX=`C22C`, IY=`0C31`, SP=`0066` and IFF1=0. Stepping
executes `0503 -> 0504 -> 0506 -> C22C -> 079A -> 079E`; `079A` installs
IY=`07A1`, entering the intended idle loop. There is no intervening acknowledge.
This is a matching executable transition, not cycle alignment. The debugger
breakpoint must be installed after asynchronous hard reset has completed;
a timed-out breakpoint run does not count as this observation.

## Pending-request discriminator and source boundary

Arnold V issue 1.5 §2.4 selects programmable interrupts *instead* of the normal
mechanism when PRI is nonzero; §2.7 identifies acknowledge and MRER bit 4 as
raster-clear mechanisms. Kevin Thacker's
[Extra CPC Plus Hardware Information](../../specs/plus/Extra%20CPC%20Plus%20Hardware%20Information.md),
interrupt section, states that CPC interrupts do not occur while ASIC raster
interrupts are active. These support source selection, but do not explicitly
specify retention of an already-pending request across a PRI round trip.

The following bounded CPU program isolates that retention in AmSpirit Lite
1.15.1 / core 2491682, using the already unlocked ASIC mapping:

1. DI, set SP=`B000`, write PRI=0, and loop for six frames without acknowledge.
2. Write PRI=1, EI, NOP. At the following instruction, IFF1=IFF2=1 and SP=`B000`:
   the old CPC request is not delivered.
3. DI, write PRI=0, EI, NOP. IFF1=IFF2=0 at the following boundary; the next
   debugger step reaches `0039` and SP=`AFFE`, through a private `0038` NOP
   marker followed by a jump to `8100`. The old request is immediately delivered.

Both switches finish within the same observed display line, with no intervening
raster acknowledge. Thus this emulator masks rather than clears the CPC
request. This is an emulator observation, not original-hardware proof of
undocumented retention. The original demo was reloaded and left paused after
the synthetic program; the CPR file was never modified.

The retained test `pr08_pending_classic_mode_switch` exercises the same
pending-event/selector boundary in `asic_pri_test.cpp`: generate a real 52-line
request, switch to a nonmatching nonzero PRI, switch back without a clear,
and acknowledge the recovered request. It fails on the old RTL:

```text
make -C sim/plus run/asic_pri_tests
FAIL: pr08: nonzero PRI must mask an already-pending CPC interrupt
```

The candidate separates CPC-compatible and programmable pending state, masking
only the CPC request while PRI is nonzero. Existing acknowledge/MRER clear and
programmed-request retention behavior remain in scope for review and the
selected gate. The focused run after the candidate passes pr01–pr08. This
shared interrupt state and live mode transition have a realistic regression
path, so the vector belongs in the suite rather than being a one-shot check.

## Evidence and review

Private evidence remains under
`docs/screenshots/eerie-late-followup-2026-09-23/`:

- `origin-bus.log`, SHA-256
  `75d6025be2641be0eb13648c2d2bc77176332133e69e65b87cfc25dc475463be`;
  `diagnostic-sources/eerie_origin/` preserves the generated probe/build recipe.
- `faithful-save-excerpt.log`, `amspirit-first-tail.json` and its RAM image;
  `amspirit-tail-steps.json` contains 16 additional successful return samples.
- `amspirit-pri-pending.json` and `amspirit-pri-pending-post.json`, with the
  generator in `diagnostic-sources/eerie-pri-pending.py`. Use Lua `cpc.setZ80`
  for direct PC changes: the API execution helper has additional execution
  behavior and its exploratory result is not the accepted discriminator.
- `pr08-before.log` and `pr08-after.log` record the focused failure/pass.

Read-only Opus 5.5 medium consultation `20260923T124539Z-95159-ef6b` exited
cleanly (170.4 s), predicting no EI/prefix defect and recommending the
first-acknowledge capture. Its ranking of possible causes preceded the new
capture and is not evidence that the PRI comparator is wrong. No PRI compare,
phase, no-wrap, CPU or Classic CRTC change is proposed here.

Fresh independent Opus 5.5 medium review `20260923T130003Z-3056-2ab6`
found no blocking defect. The suspected snapshot same-edge race does not
occur: `asic_regs` receives PRI through the CPC+ `sna_wr` drain before
`plus_sna_apply` emits its delayed apply pulse. Plain snapshots reset the
ASIC first. The parent verified those source paths; no waveform claim is made.

Two modelling limits are explicit. ACK continues to clear both pending
latches, as the previous aggregate latch did; specifically clearing a hidden
CPC request on a DMA-only acknowledge is not covered by the no-ack AmSpirit
round trip. A programmed fire clears the documented counter bit 5 but does
not by itself clear the retained CPC request. The discriminator has no fire
between its switches, so it does not adjudicate that collision either. These
are recorded limits, not newly asserted hardware rules. The reviewer requested
checking the added PRI compare on the INT timing path in the exact Quartus build.

Final source gate after the last code edit:

```text
python3 sim/select_tests.py --run
select_tests: PASS 7 benches: run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_pri_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests
```

`git diff --check` also passes. The relevant slow diagnostics are listed by the
selector but not repeated: the new focused vector covers the changed live
selection boundary, and the production-T80 origin trace already identifies
its title trigger. The candidate is ready for the coordinator's exact full-effort
pre-merge build. Title acceptance and Quartus timing are still pending.
