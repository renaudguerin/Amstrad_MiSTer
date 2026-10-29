# V1: pulse-width response, admission and acquisition observations

## Position

The bounded continuation of [the V1 evidence unit](v1-sync-response-evidence-2026-10-29.md)
adds an offline width/admission sweep and an independent reading of the existing
acquisition trace. It changes no production or simulation source. The completed
`135868c` selected gate, soak and review remain applicable and were not rerun.

The result is **insufficient to select a monitor model**. Width sensitivity can
be represented by a causal hypothesis, and very short pulses can be rejected,
but this source stream cannot identify the response coefficient or threshold.
Width alone also cannot decide how a monitor responds to multiple ordinary-width
pulses. No filter, CRTC, GA, output interface or menu change follows.

## Source and assumptions

Input is the retained type-1 Raw-pixels B22 `sync-machine1-mode1.csv` from
`135868c`, sampled before each 64 MHz master edge. It executes the existing
synthetic program with production CRTC/GA/motherboard and TV80 substitute;
it is not a SHAKER or original-CPC waveform. Source fidelity and monitor
presentation remain separate questions.

French ACCC v1.11 **§15.1 pp.147–148**, read from the existing pdf-inspector
Markdown and rendered French p.148, describes position/width-dependent monitor
response and says the very short monitor pulse with R3l=2 is insufficient for
synchronisation. **§15.7 p.158** describes progressive displacement during
resynchronisation. These support testing width and pulse admission as mechanisms;
they do not give the numeric detector threshold, gain or time constant used below.
In particular, a five-tick pulse measured in this RTL trace is not a verified
original-machine pulse width.

The model grid is declared before all results and no cell is selected:

| Quantity | Declared exploratory value |
|---|---|
| Nominal scanner period | 4096 master ticks (64 µs) |
| Reference monitor-pulse width | 256 ticks (4 µs) |
| Phase measurement | rise + λ × (width − 256), relative to the nominal grid |
| Width coefficient λ | 0, 0.5, 1 |
| Admission threshold | ρ × 256, with ρ = 0, 1/32, 1/8, 1/2 |
| Phase correction | 1/8 of circular error, limited to ±16 ticks per admitted pulse |
| Frequency / vertical response | absent |

λ=0 is the width-insensitive control; nonzero λ values introduce pulse-width
sensitivity without claiming a physical detector law. Thresholds are 0, 8, 32
and 128 ticks, not measured hardware constants. Where width affects admission
or measurement, the decision acts at the **falling edge**; it cannot move past
pixels retroactively. Only λ=ρ=0 acts at the rising edge.

All current input pulses are complete and same-stage. The throwaway decoder
excludes incomplete/cross-stage pulses; that preprocessing has no effect on
this corpus but does **not** define a live stuck/missing-fall policy. There is
no general online monitor state machine here.

## Pulse extraction and held-out controls

Independent reconstruction from both GA edges matches the earlier pulse table:
707 complete monitor pulses, comprising 458 of width 256, 51 of width 197,
and 198 of width 5 ticks. No cross-stage or unfinished GA pulse is excluded.
Raw CRTC pulses are a different boundary; their widths are tabulated below.

One synthetic phase/width step is the exploratory input. The entire declared
12-cell grid is then held fixed for steady sync, ten missing pulses,
alternating widths, narrow extras and ordinary-width extras. No image is fitted.
These are held-out **synthetic controls**, not a held-out SHAKER acceptance run.

| Control | Observed consequence with fixed grid |
|---|---|
| Steady ordinary pulses | Zero phase offset in every cell |
| Ten missing ordinary pulses | Zero drift in every cell because frequency error is absent by construction |
| Alternating 256/197 widths | Maximum absolute phase 0 / 15.73 / 31.45 ticks for λ=0 / 0.5 / 1 |
| Twenty extra five-tick pulses at quarter-period offsets | With zero threshold, maximum absolute phase 119.14 ticks; each positive threshold rejects all twenty and preserves zero phase |
| Twenty extra 256-tick pulses at the same offsets | Every threshold admits them; maximum absolute phase 119.14 ticks |

The ±16 correction limit saturates for large errors, so equal 119.14 values
are properties of this model, not proof that a monitor treats narrow and wide
pulses equally. The extra-pulse controls have two pulses per nominal line;
the real B22 multiple stage has four. They share a spacing motif, not an
identical waveform or distortion.

At positive thresholds, all 198 five-tick B22 pulses are rejected and 509
other pulses are admitted. Any threshold strictly above 5 and at most 197 would
separate these two observed width populations. That large interval prevents
identifying the physical threshold from this trace. It also does not establish
that a real monitor should ignore every pulse within it.

The B22 short stage has unchanged rising-edge phase but width 197 instead of
256. After 51 pulses the model phase is 0 / −29.47 / −58.93 ticks for the
three λ values. All are compatible with the source timing alone; selecting
among them needs independent monitor-position evidence. The single stage-4
pulse adds +16 ticks even though its width is 256: pulse **position** remains
part of the update. The restore stage converges to +512 ticks for every cell,
reflecting a source-grid offset, not a measured monitor response.

A fall-time decision is chronologically possible for these complete pulses:
the minimum fall-to-next-rise interval is 379 master ticks. This establishes
neither FPGA processing cost nor a sufficient presentation buffer. A model
that uses completed width cannot assign its result to earlier pixels without
explicit latency/resampling. A stuck pulse, changing frequency, vertical
response and clipping still need contracts.

## Separate acquisition observation

`crt_filter` takes raw CRTC `hs_sel/vs_sel`, not GA-shaped monitor sync
(`rtl/Amstrad_motherboard.v`, `crt_filter` instance). Its frontend
`hsync_mask` suppresses frequent **CRTC** pulses; an offline monitor model
receives the **GA** pulse stream. These are distinct pulse-admission problems.

The independent observer counts full programmed stages, including their initial
20,000 ticks. These counts therefore differ from the existing dynamic bench's
later measurement windows without contradicting it.

| B22 stage | Raw CRTC rises / complete width | GA monitor rises / complete width | CE4-qualified filter-input rises |
|---|---|---|---:|
| Ordinary | 61 / 896 ticks | 61 / 256 ticks | 61 |
| Short | 51 / 320 ticks | 51 / 197 ticks | 51 |
| Multiple | 198 / 128 ticks | 198 / 5 ticks | 50 |
| Restore | 391 / 896 ticks | 391 / 256 ticks | 391 |

The final raw CRTC pulse is unfinished at trace end: 390 raw complete widths
are observed in Restore, while its earlier-ending GA pulse has 391 complete
widths. The observer retains that distinction.

In the multiple stage, the raw CRTC train has a 1024-tick period (16 µs), while
the mask admits approximately one pulse in four. Dropping raw CRTC pulses at
that frontend is not equivalent to applying a monitor-width threshold to the
198 five-tick GA pulses. A model that silently reuses the filter's masked
stream would lose the evidence it is meant to interpret.

The startup pair at ticks 10,517 and 14,613 is separated by exactly 4096 ticks,
but the training counter already contains pre-pair history. Its second-edge
updated value is 950 CE4 ticks, so the stored half-count becomes 475 rather
than the pair interval of 256 CE4 ticks. Later the estimate changes to 256 at
tick 1,788,694. The **187** accepted rises after tick 1,800,000 all still have
pre-edge `hSyncCount=76`, arm=0. A correct period value does not establish the
classification phase. This is a current implementation observation, not a
claim that a particular phase reset policy matches a CTM.

## Actual SHAKER trace-export prerequisites

The next missing input is a **native-byte, timed RGB plus GA-sync stream from
an authentic SHAKER execution**, with its prehistory and independent displayed
position evidence. Existing fixtures offer different parts of that contract:

| Available seam | Reusable | Missing before claiming a SHAKER RGB response input |
|---|---|---|
| `sim/plus/b6_dynamic_test.cpp` | Passive all-master-tick RGB/tuple export, production motherboard, SDRAM, pixel and output paths | Synthetic TV80 program; no authentic SHAKER loader/menu/history |
| `sim/diagnostics/shaker_b9_mid/b9_t80_top.sv` and retained Module-D replay | Production T80, authentic bounded caller, CRTC/GA clock/WAIT and sync generation | GA `.D(DO)` is CPU output, not the production VRAM-data mux; monitor/RGB ports are left open; flat RAM, absent ROM/keyboard and constrained caller environment |
| V3's ignored B18-derived diagnostic | Production T80/motherboard/MMU with physical-RAM video word and native RGB/GA taps | Snapshot continuation did not acquire a trustworthy title; authentic caller/setup and video-return timing remain unvalidated; no SDRAM arbitration/cache |
| Existing AmSpirit helper/API | Continuous media boot, SSM checkpoints, state/history and diagnostic screenshots | Published inspection/CPU-step interfaces do not establish an exported sub-instruction RGB/GA waveform; a `/api/ga` snapshot is not such a stream |

Exposing the B9/Module-D GA output ports alone would produce an invalid pixel
claim. Production uses `ga_din = e244_n ? vram_d : D`, with VRAM address,
CAS-byte sequencing and mode-dependent history in
`rtl/Amstrad_motherboard.v:929–973`; the diagnostic feeds `.D(DO)` directly
(`sim/diagnostics/shaker_b9_mid/b9_t80_top.sv:182`). Sync-only observations
from that diagnostic remain useful within its contract.

The smallest source probe is one AT transition and one AY width case, followed
by a held-out D4 ordinary-width multi-pulse case and a steady control. Retain
an ordinary frame before the transition and a bounded multi-frame effect
window; that window is an acquisition limit, not an asserted settling time.
Require actual SSM page/scene identity, raw CRTC HS/VS, both monitor-sync edges,
GA RGB/DAC pairs, native-dot enable, applied byte policy, accepted VRAM
address/word/half/byte, and CRTC-write timestamps in one epoch. Use all-master-
tick changes rather than instruction polling or four-tick edge quantisation.
Pin module/media, CPU, model and prior navigation. A cold caller can be an
explicit diagnostic, but does not replace the supplied full traversal or
prove hidden CRTC/filter history equivalent.

V3 owns its caller replay and A owns devices/shared AmSpirit. Reuse their
validated seam if it becomes available; do not create another writer or repeat
Amazing captures without a concrete state trigger. Current logo animation
provides no validated same-state trigger, so this continuation requested none.
If source execution is available without an independent monitor observation,
parameter non-identifiability still remains. Source validation can proceed
independently of presentation fitting.

## Acceptance and retained artifacts

The free DeepSeek worker wrote only ignored probe files. Parent inspection
found two acceptance-reporting defects: an audit count overwritten with only
the B22 count, and a control gate keyed by λ that discarded earlier ρ cells.
An intentional failure in the first grid cell was missed before correction
and detected afterward. Phase summaries now include the last post-event state.
The incorrect provisional outputs are retained separately, not relabelled.

The corrected stdlib probe reports **GATE PASS**, with all twelve cells
participating and 12,444 event decisions audited: 8,484 B22 and 3,960 synthetic.
Pulse extraction matches the independent earlier table. This is an offline
analysis check, not a new DUT gate or hardware acceptance. The separate
acquisition observer only reads the existing CSV. No production behaviour or
permanent test was added, so the earlier selected gate/soak was not repeated.
Under the user's current run override, this low-stakes offline unit does not
require a new cross-provider review or review-debt entry.

Current-checkout reproduction (stdlib Python, read-only source input):

```sh
python3 output_files/v1-width-admission-2026-10-29/v1_width_admission.py
python3 output_files/v1-acquisition-observation-2026-10-29/observe.py
```

The scripts assume those checkout-relative locations. In a fresh checkout, copy
the archived probe/observer directories back under `output_files/`, and expose
the retained `artifacts/` as `output_files/v1-sync-2026-10-29` and the earlier
`proof-and-analysis/` as `output_files/v1-work` (ignored links suffice). This
reuses the original inputs; no DUT build, simulation or device run is needed.

Private scripts, results, provisional versions, logs and source checks are
retained under
`local/investigations/sync-monitor-response-2026-10-29-f4da/continuation/`.
The worker run is `20261029T175223Z-63974-a28f`; no new consultation, device
operation, synthesis, integration, push, publication or paid-quota action.
