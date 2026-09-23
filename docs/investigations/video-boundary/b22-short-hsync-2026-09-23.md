# B22: type-1 short HSYNC and filter acquisition

## Finding and scope

On integration base `b5bf5965470b85365d26158d44424439fc116b1e`, the TV80
I/O-wait correction exposes an unsupported B6 fixture assumption: programming
R3l=5 does not guarantee shifted fetches before the compatibility filter has
reacquired. The CPU program, production RTL and filter history remain unchanged.
The dynamic fixture checks the complete raw five-microsecond pulses and every
existing transport oracle on all machines; shifted-fetch coverage remains
required on type 0 and Plus, and the type-0 Full/Raw visible discriminator remains
required. Type 1 must also produce shifted fetches if it records at least two
armed falls without hs4 cancellation; the second pulse ensures fetch time after
the first even at the measurement window boundary. Type-1 acquisition/monitor fidelity is not closed by this correction.

## Measured cause

The original assertion fails in all three type-1 modes, with zero tuple, byte,
mask, final-RGB and Full/Raw timing errors. A read-only Verilator internal probe
records these events (ticks count the existing dynamic-run loop, at 64 MHz):

| Tick | Event | Filter state |
| --- | --- | --- |
| 14,589 | Startup line estimate learned | `hSyncSize: 0 -> 474` CE4 ticks |
| 29,694 | Ordinary-stage marker | Estimate still 474 |
| 280,198 | R3l changes 14 -> 5 | C0=40, C3=0, HSYNC inactive; count=10, hs4=0 |
| 280,542 | Short-stage marker | Estimate still 474 |
| 280,829 | First probed short rising edge | count=49, hSyncReg=0 |
| 281,149 | Corresponding falling edge | count=69, hSyncReg=0, hs4=0 |
| 284,925 | Next rising edge | count=305, hSyncReg=0 |
| 285,245 | Corresponding falling edge | count=325, hSyncReg=0 |
| 490,718 | Missing-stage marker | No short-stage shifted fetches |
| 1,788,669 | Restore-stage estimate update | `hSyncSize: 474 -> 256` |

The short stage has 46 complete measured pulses, each 320 master ticks (20 CE4
ticks), with a 4096-master-tick (256 CE4) line period. Its filter-qualified
falling-edge count is zero, as is its `hs4` high-time. The early edge probe covers stage ages below 40,000; the summary covers ages
above 20,000, so together they cover the entire short stage. Edge-table count
values are **pre-edge**; `syncgen` increments the counter with a blocking
assignment before comparing it, so its classification uses the next count.
The proposed one-off
four-character pulse setting sticky `hs4` is therefore **disproved for this
fixture**. The R3 write is outside HSYNC, not an active-pulse R3.JIT event.

`crt_filter.syncgen` measures `hSyncSize` from its first two lines after VSYNC.
The CPU reprograms the CRTC during startup, so that initial estimate is not a
settled line period. Its resync path resets `hSyncCount` on the stored period
or the VSYNC alignment event; it arms `hSyncReg` only when a rising input HSYNC
coincides with that reset. Classification at a falling edge requires that arm.
Thus `hSyncCount` is a regenerated-phase counter, not unconditionally the
elapsed input-pulse width. In this trace no arm/classification occurs during the
short stage, and the later restore stage finally supplies a fresh estimate.
The fixture's 20,000-tick sampling delay does not establish filter acquisition.

The comparison traces explain why the kept coverage requirements are valid
for type 0 and Plus: both have learned 256 before the short stage. Type 0 learns
it at tick 26,877; Plus at tick 80,186. Both show pre-edge count=255 at the short
rising edge and count=19 with hSyncReg=1 at its falling edge, so the incremented
classification count is 20. Type 1 instead retains its startup estimate 474.
These are measured cross-module startup differences, not a rule that R3 writes
always acquire differently by CRTC type. The type-1 coverage condition becomes
active if a future CPU/CRTC timing change supplies armed short pulses without
hs4 cancellation. The origin and physical correctness of the distinct startup
acquisition remain outside this transport correction.

Reproduce the observation without changing instruction timing:

```sh
mkdir -p sim/plus/obj_dir/b6_dynamic
make -C sim/plus obj_dir/b6_dynamic/b6_dynamic_tests
sim/plus/obj_dir/b6_dynamic/b6_dynamic_tests --machine 1 --probe-sync
```

`--probe-sync` reports R3/hs4/line-estimate changes and the first 40,000 ticks of
input-filter edges in the ordinary and short stages. Summary `armed_falls` and
`hs4_ticks` are diagnostic counts, not hardware-derived expectations. The probe
reads Verilator's existing public-flat state and does not drive it.

## Source and monitor evidence

French ACCC v1.11 §14.1 p.132 defines a preprogrammed R3l as the raw CRTC HSYNC
duration in microseconds. This independently gives `5 * 64 = 320` master ticks
for the new complete-pulse assertion. It checks the raw CRTC HSYNC exposed as
HBLANK in `b6_raw_tuple`, not the Gate Array's shaped monitor pulse.

French §14.3–14.4 pp.134–135 (English pp.133–134) distinguish that raw duration
from the shorter GA C-HSYNC and physical monitor displacement. The French and
English pdf-inspector prose agree on this distinction. These pages do not
establish that the core's sticky `hs4` algorithm or its startup acquisition is
physically correct. No numeric expectation was taken from their flattened
timing table.

The history bit was already present when `crt_filter.v` entered local history
in `489f340` (2020-07-02, decapped GA integration); that commit gives no specific
hardware justification for it. The [B6 contract](b6-video-boundary.md#explicit-byte-phase-rule)
requires Full to continue sampling live SHIFT at accepted fetches and raw modes
to ignore it. Preserving that contract does not validate the filter as a CRT
model.

The [September 12 hardware report](../hardware-runs/hardware-evidence-2026-09-12.md)
records no visible Full/Raw improvement for the sampled titles and no direct
hs4/acquisition discriminator. The [CRT-session proposal](../hardware-runs/hardware-diagnosis-2026-09-10-second-pass.md#b6-boundary-brief-for-the-next-session)
leaves monitor phase response open. Neither supports changing RTL to make this
fixture produce SHIFT. MiSTer was not accessed: the Eerie Forest task owns it.

## Acceptance and residual

Fail-before: the instrumented, assertion-unchanged binary on `b5bf596` run as
`sim/plus/obj_dir/b6_dynamic/b6_dynamic_tests --machine 1` exits 1, failing all
three modes solely on the shifted-fetch coverage assertion. Its transport
error counts are zero; this is an assertion correction, not a claimed RTL fix.

After the final test-code edit, `python3 sim/select_tests.py --run --slow`
exits 0 with **`select_tests: PASS 1 benches: b6-dynamic`**. All nine
machine/mode cases pass, each with 46 complete short pulses and zero width
errors. Tuple, byte, mask, final-RGB and Full/Raw timing error counts remain
zero. The pulse sampler uses the pin state immediately before each master
edge, so a pulse already high when the measurement window opens is excluded.
The CPU program and its stage timestamps are unchanged.

Cross-provider review by Claude Opus 5.5 accepted the assertion correction as
an evidence-backed fixture-premise repair, not prohibited weakening. It requested
a self-checking type-1 acquisition condition, type-0/Plus comparison traces,
and clearer probe-window/count wording. All three were addressed. The final remediation review was **CLEAR**; its
wording correction (the two probe windows together cover the stage) is included.
The post-remediation gate log ends with the PASS line above. Review run IDs:
`20260923T080735Z-68133-4a7c` and `20260923T081252Z-71104-107c`. The reviewer
could not read the earlier external run log during the second pass, so it
reviewed the final source against the supplied three-item remediation brief;
it ran no tests and made no edits.
The remaining device gate is a matched original-CPC/CTM (or a justified monitor
oracle) versus MiSTer trace for the same startup and R3 sequence, recording
raw HSYNC/VSYNC, physical displacement and Full/Raw mode. A controlled steady
4/5/long-pulse sequence is additionally needed to judge sticky history itself.
No test retiming, synthesis request or hardware-correctness claim is part of
this change.
