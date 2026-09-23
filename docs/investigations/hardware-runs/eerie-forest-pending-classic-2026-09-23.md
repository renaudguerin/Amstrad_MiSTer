# Eerie Forest: pending CPC interrupt crosses into PRI mode

Plus interrupt repair `cb60ff8cb5913700d130404b9e8b2d604df11227` on
`codex/plus/eerie-late-freeze`, starting from
integrated master `4453b760027f575d6c1c147492c53c1a24cd2de7`. This extends the
[context follow-up](eerie-forest-context-followup-2026-09-23.md). The original
CPR and production T80 remain unchanged. The exact full-effort RBF passes bounded device acceptance beyond the reported
freeze; full-demo completion and gameplay are not claimed.

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
ultimately resumes rendering into low code RAM. The exact-build device
acceptance below confirms progression beyond the reported freeze.

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
programmed-request retention behavior were checked in review and the
selected gate. The focused run after the candidate passes pr01–pr08. This
shared interrupt state and live mode transition have a realistic regression
path, so the vector belongs in the suite rather than being a one-shot check.

## Evidence and review

Private evidence is archived under
`local/task-archives/plus-compatibility-2026-09-23/b967/docs/screenshots/eerie-late-followup-2026-09-23/`:

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
its title trigger. The coordinator built the exact reviewed source for the device acceptance below.

## Exact-build device acceptance

[CI run 35864605669](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35864605669)
passes simulation, production-T80, synthesis(full) and required-gate for exact
`cb60ff8`. Artifact `Amstrad-build-260-1-full` records `build_mode=clean_full`.
Quartus timing closure passes: minimum setup +0.545 ns, hold +0.243 ns,
zero TNS across seven clocks. The downloaded provenance and timing summary
were inspected; no new source edits followed the gate or build.

Device RBF `/media/fat/_Computer/Amstrad_20260923_cb60ff8.rbf`, SHA-256:
`29f69fc4ab58c7072f922661da1b1bec9c1df80bc1e245d0ab73d5b22d2d7310`.
The driver verifies it and the unchanged media hashes before each run.

Eerie Forest: explicit 6128+ (CFG bits 34:33=2), Raw CRT (36:35=2), no input,
28-second boot delay and three serial captures six seconds apart. All three
848×287 PNGs were visually inspected. They show the coloured landscape,
different background positions and runner poses, and advancing top text
(`FROM A G`, `BY REFLECTIONS`, `FINALLY ON GX4000 & AMS`). This matches the
successful AmSpirit scene and passes the former frozen striped-screen point.
The old exact `41a1f27` run used the same case/settings and produced three
identical frozen frames (`a89fc976...`).

| Eerie capture | SHA-256 |
|---|---|
| 1 | `bcdcbec503a083fe5491f71a73b987b9b62b7d59ebb1e8af4165c6344787592b` |
| 2 | `d2e691669ab850eaaf8cc55812bf9dd67a2625c8c8585eb9d79e5e0e5f976710` |
| 3 | `3e88a729524aab840d5f7685012b59a9c6a0b4a44e1d387deb7913f8f0521dd2` |

Regression sampling on the same RBF, 6128+ and Full sync filter, no input:

- **Copter 271** (45-second boot delay, three captures four seconds apart):
  title logo is intact; helicopter animation and title text advance. No earlier
  top-row palette corruption is visible in the sampled frames. This is not a
  continuous flicker measurement or gameplay-scroll acceptance.
- **Switchblade** (28-second boot delay, three captures four seconds apart):
  title animation, high scores and returning title are visible. No boot or
  title/attract regression is observed; gameplay remains untested.

| Regression | Original CPR SHA-256 |
|---|---|
| Copter 271 | `4b75c62cbd660206ef30ff8cbf9c4b1281282a423b5eccc1ff8444589f2a9c1d` |
| Switchblade | `d958e2b1eeebaa227aa33c4f0f5627fc238a2791fd177f7ebffefdf81a07ff78` |

The manifests, nine native PNGs, cases, applied/saved CFGs and run logs are
private under `local/task-archives/plus-compatibility-2026-09-23/b967/docs/screenshots/eerie-pending-fix-2026-09-23/`. Each run's
`finally` restore was read back byte-for-byte; restored CFG SHA-256:
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
Native captures do not expose live OSD or original-hardware interrupt internals.
They establish progression past this reported freeze on the exact candidate,
not completion of the whole demo or all Plus timing rules.

## Integration-artifact confirmation, 2026-09-23

The exact master merge `57a90bc` was independently synthesized in
[CI run 35868033667](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35868033667).
Its clean-full RBF is delivered as `output_files/Amstrad_20260923_57a90bc.rbf`,
SHA-256 `11959f68712177fa2f73958ca98ac7e98b1b5a854f8cb47d1e0b4db6c9638d8b`.
Timing closure passes with setup/hold minima +0.501/+0.177 ns and zero TNS.
The RBF installed on MiSTer at `/media/fat/_Computer/Amstrad_20260923_57a90bc.rbf`
was verified against that hash on both hosts.

The same unchanged media and established no-input cases were rerun on this
**exact integration artifact**, with three native captures per title visually
inspected. Eerie Forest (6128+, Raw CRT, 28-second boot delay, six seconds
between frames) again advances beyond the old striped freeze: landscape,
runner poses and top text all change. Switchblade (6128+, Full, 28-second boot
delay, four seconds between frames) shows title animation, high scores and
return to title. Its first two PNG hashes match the prior `cb60ff8` captures.
The unchanged CPR hashes are `72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215`
(Eerie) and `d958e2b1eeebaa227aa33c4f0f5627fc238a2791fd177f7ebffefdf81a07ff78`
(Switchblade).

| Master RBF capture | SHA-256 |
|---|---|
| Eerie 1 | `cdc9930c4f4443d7b07abd2f0af6e90a66b6be99fe53782f4cc3abb5cd8c80f6` |
| Eerie 2 | `a60d8b51d2d3b108e38100311d38b51ff0c897d0ecec1d415b7280d657329287` |
| Eerie 3 | `9ba0a46eb9b807e09044747e6ec4b7b829c8f494b4a442cf88c15ba3b6c8e0a2` |
| Switchblade 1 | `56af42be683b237192a0c4dc6c4fae7139795e3024387a25abc660e3eae82bb3` |
| Switchblade 2 | `b8dfdc946c52a66fdef36827a413bda7ed5ce334b55168c29e91c1dbf2803ac8` |
| Switchblade 3 | `86cb834609147bc3a25b93520697765782c54ee327caaa8c574d28d7078fe7ba` |

Private cases, manifests, PNGs, capture log and saved CFG remain under
`local/task-archives/plus-compatibility-2026-09-23/b967/docs/screenshots/eerie-master-57a90bc-2026-09-23/` in the main checkout.
Manifest SHA-256 values are `c335f97e1faebb36a93810e41add6d5ec8c02c59a3bc71ede67cd162fc61d6be`
(Eerie) and `4dd14839be7feb15afbd912fe91e0c4ece5c1973a72ef4201bc66116245d3b27`
(Switchblade). The original CFG was restored and read back byte for byte,
SHA-256 `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.

This is bounded progression and title/attract acceptance, not full-demo,
gameplay, continuous flicker or pixel-perfect validation. The first Eerie
capture has a small white dotted sliver at the left edge of the bottom logo;
its cause was not investigated. The device was released after the tests.
