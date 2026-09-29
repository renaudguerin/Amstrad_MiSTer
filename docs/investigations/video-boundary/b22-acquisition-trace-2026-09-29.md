# B1/B22: applied mode and acquisition trace

## Scope and result

This slice adds passive simulation and SSM marker observations on base
`5a234183ac4295099a6081693fc43f3fb1807ffc`. It does not change the CPU program,
filter history or acquisition policy. The B6 fixture uses production motherboard
divider topology (`production_clocking=1`), real CRTC/GA, SDRAM controller,
colour converter and mixer, with its existing TV80 CPU substitute. It does
not establish production T80 instruction-by-instruction startup parity or
original-monitor response.

The new trace distinguishes two acquisition problems: startup training includes
history before the first ordinary HSYNC pair; later learning the right line
period still does not establish phase alignment. Neither is diagnosed reliably
by a screenshot or by a line-estimate value alone.

## Current-base measurements

All times below are pre-edge loop ticks at 64 MHz, starting **after fixture
initialization**. An effect of edge `n` appears in row `n+1`. The old
[September 23 report](b22-short-hsync-2026-09-23.md) is evidence from its stated
base, not an exact timestamp oracle for this base: its estimate was 474;
this reproduction learns **475 CE4 ticks**. The program and delays are unchanged
by this slice. Startup may have begun before trace tick zero; the trace does
not infer the earlier counter origin from unobserved history.

| Type-1 event | Tick | Direct observation |
| --- | ---: | --- |
| Trace starts | 0 | Raw VS=1, training counter=36, training syncs=0, estimate=0 |
| Raw VS falls | 967 | No accepted horizontal edge has been recorded in this trace |
| First accepted HS rise | 10,517 | Raw VS=0, pre-edge training counter=693, syncs=0 |
| Second accepted HS rise | 14,613 | Exactly 4,096 master ticks later; pre-edge counter=949, syncs=1 |
| New estimate visible | 14,614 | Incremented counter=950; `950 >> 1 = 475`, syncs=2 |
| Short stage starts | 280,550 | Estimate still 475; applied mode observed directly |
| Restore learns ordinary period | 1,788,694 | Estimate becomes 256 after a real VS-qualified training cycle |

The accepted HS pair is already separated by the ordinary 256 CE4 ticks.
The 475 estimate is **not** evidence of two settled 475-tick lines: the training
counter was not reset at that first rise because VS was already low. It includes
startup history. The first accepted rise increments `syncs` from zero to one,
and the second increments it to two and stores half the elapsed counter.
This explanation follows the observed training inputs and the current
`syncgen` ordering; it is not a proposed hardware rule.

Across the complete ordinary and short stages, raw CRTC and GA HS rising edges
are 4,096 master ticks apart, while filtered HS rises are **7,600 ticks apart**
(`475 * 16`). In the short stage there are 51 raw rises and 51 filter-input
accepted rises; their 50 consecutive periods are all 4,096. There are no armed
falls, no `hs4` high samples, and no missing-HSYNC fallback. Thus this stage's
failure to classify short pulses is regenerated-phase alignment, not dropped
input rises or sticky four-character history. The older transport summary
counts only its delayed measurement window (46 complete short pulses); this
full-stage diagnostic includes the initial 20,000 ticks too.

After the estimate becomes 256, each accepted rise after tick 1,800,000 still
sees **pre-edge `hSyncCount=76`**, through the end of the captured restore stage.
The period is corrected, but that rise is not at the phase-reset boundary and
does not arm classification. The complete restore stage has one armed fall,
earlier in its history. This directly demonstrates why `line_estimate=256`
cannot be relabelled “acquired.” The later filtered period is 4,096 ticks; the
transition also contains one 2,048-tick interval. Physical lock/displacement
cannot be inferred from either count.

Type 0 first observes estimate 256 at tick 26,902; Plus at tick 80,187.
Each machine's Full and Raw-pixels runs have identical acquisition timing.
All nine runs observe requested mode equal to the motherboard's applied mode,
and retain the existing pulse-width, tuple, byte, mask and final-RGB checks.

## Reproduce and inspect

```sh
mkdir -p output_files/b22-sync-acquisition
make -C sim/plus obj_dir/b6_dynamic/b6_dynamic_tests
sim/plus/obj_dir/b6_dynamic/b6_dynamic_tests --trace-sync output_files/b22-sync-acquisition/sync
```

Use `--machine 1` for the bounded type-1 reproduction. On the investigated
macOS host, compilation additionally required
`CPLUS_INCLUDE_PATH=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1`;
this is a host include-path workaround, not a repository build change.

The CSV contains requested and actual applied mode, raw CRTC HS/VS, GA/raw,
filtered and selected tuples, mixer DE, line estimate, arm, absence/mask/history
flags and training counters. Tuple bits are HS/VS/HBLANK/VBLANK at bits 3..0.
Event bits are initial=1, stage entry=2, observed state change=4, CE4-qualified
filter-input edge=8 and observed phase counter reset/wrap=16. Counter increments
alone do not generate records. Every row is a consistent pre-edge snapshot;
these are simulator observations, not pin measurements.

For example, list the exact CE4-qualified startup training inputs without
interpolating missing counter rows:

```python
import csv
with open("output_files/b22-sync-acquisition/sync-machine1-mode0.csv") as f:
    for r in csv.DictReader(f):
        if int(r["tick"]) < 32000 and int(r["event_bits"]) & 8 and r["filter_hs"] == "1":
            print(*(r[k] for k in ("tick", "crtc_vs", "training_syncs",
                                  "hSyncCount2x", "hSyncCount", "line_estimate")))
```

Raw artifacts are under `output_files/b22-sync-acquisition/`: the nine
`sync-machineN-modeM.csv` files, `trace.log`, initial diagnostic `gate.log`,
`artifact-check.json` and the applied-mode negative-control log. These
pre-SSM-extension traces preserve the measured source base. All 54 stage
markers and all nine PASS summary lines matched between traced and untraced
runs. The temporary applied-mode control forced a disagreement and exited 2
at tick zero, before transport oracles could silently use the requested mode;
the control is not retained in production or fixture source.

## Observed boundaries

The current production wiring, checked directly in
`rtl/Amstrad_motherboard.v`, `Amstrad.sv` and the fixture, is:

| Trace boundary | Meaning |
| --- | --- |
| Raw CRTC HS/VS | `hs_sel/vs_sel`, classic CRTC or Plus CRTC; directly feeds `crt_filter` |
| Raw tuple | GA-shaped monitor HS/VS, raw CRTC HS as HBLANK, GA VBLANK |
| Full tuple | All four regenerated filter outputs |
| Selected tuple | Full tuple in Full/Raw pixels; raw tuple in Raw CRT |
| Mixer DE | Production video-mixer output, after colour/output pipeline |
| Applied mode | Motherboard `sync_filter_applied`, independently of the requested mode |

`hSyncCount` is regenerated phase, not elapsed raw pulse width. Its blocking
increment occurs before classification inside `syncgen`; a pre-edge trace
shows the prior count. `hSyncReg` arms only on a qualifying phase reset and
clears on a classified fall, so a zero value alone cannot diagnose loss of
acquisition. `SHIFT` is `shift ^ hs4`; the latter is sticky history. Mode changes
clear byte history but do not clear this filter history.

Full and Raw pixels share acquisition timing. Comparing their pixels can
exercise byte compensation or the vertical pixel mask, but cannot independently
validate the monitor model. The bounded B6 machine/mode corpus does not expand
SHAKER numeric/readback testing into a three-mode matrix.

## Device observability and remaining experiment

The run-wide capture manifest deliberately leaves `sync_filter_applied` null;
CFG/request metadata does not read back the motherboard register. Native
screenshots omit the OSD and consume an asynchronous scaler buffer. Neither
can establish this internal state on a device.

Other existing routes were checked before choosing the passive fixture:
`obs_applied_config` terminates on internal wires in `Amstrad.sv` and only
encodes Raw CRT versus safe modes; it cannot distinguish Full from Raw pixels.
The HPS `status_in/status_set` route is used for snapshot model echo, `info_req`
for snapshot-save notifications, and the connected ioctl path is download-only.
UART outputs are tied to zero. The saved SNA header has CRTC/GA observations,
but no filter mode or acquisition state. A mode-only diagnostic could reuse
HPS status/menu readback with a new read-only field and arbitration; that would
still require RTL, host interpretation and a new RBF, and would not provide
edge history. There is no demonstrated host-only command that retrieves the
needed state from the current RBF.

The SSM ring currently timestamps motherboard-selected `hs/vs` at native
`ce_16`. Those are neither raw CRTC sync nor final mixer/scaler acquisition.
`rtl/ssm_marker.v` leaves 5 bits in event word A and 16 in word B reserved, but
format 1 transported no mode or filter state. Format 2 reuses those bits and
captures state at the same final marker-byte fetch completion as the timestamp,
then carries it through marker recognition and DDR queuing. Sampling live state
at DDR write time would attach the wrong acquisition state to the marker.
Single marker snapshots still miss history between markers; the present B6
program writes RAM stage labels, not SSM markers. A new RBF is required before
any device record can establish these fields.

The remaining physical test requires a matched startup/R3 sequence on an
original CPC and CTM (or an independently justified monitor oracle): capture
raw CRTC HS/VS, GA-shaped monitor sync, and displayed horizontal displacement.
For MiSTer, use a diagnostic RBF with observed applied mode and filter history,
record output configuration, and measure actual analog/Direct Video connector
sync as well as pre-filter signals. Raw CRT through HDMI/ASCAL is not a CTM
oracle; `sync_fix`, composite sync and framework output routing can alter what
reaches the connector. A separate steady 4/5/long-pulse sequence is needed to
judge sticky `hs4` history. No acquisition fix follows from the present trace
alone, and the fixture must not be retimed to conceal it.

No MiSTer access or original-hardware observation was performed in this
slice. Integration and synthesis belong to the coordinator; device acceptance
requires the resulting RBF.

## SSM format 2 contract and use

Record size, ring capacity, DDR base, marker recognition and publication order
are unchanged. The new 21-bit observation occupies word A bits 63..59 and word B
bits 63..48. Timestamp/code/raster/sequence fields keep their existing positions.

| Observation bits | JSON field | Meaning at final marker-byte fetch completion |
| --- | --- | --- |
| 1..0 | `requested_mode` | Raw request: 0 Full, 1 Raw pixels, 2 Raw CRT, 3 reserved |
| 3..2 | `applied_mode` | Actual motherboard register; reserved request normalizes to Full |
| 12..4 | `line_estimate_ce4` | Stored filter line estimate, in 4 MHz enables |
| 13 | `hs4` | Sticky filter history |
| 14 | `shift` | Effective SHIFT output (`shift ^ hs4`) |
| 15 | `no_hsync` | Filter's absence/fallback state |
| 16 | `hsync_mask` | Input pulse mask state |
| 17 | `arm` | Fall-classification arm, **not** lock |
| 19..18 | `training_syncs` | Filter training state |
| 20 | — | Reserved zero |

The new host reader accepts formats 1 and 2. Format-1 records return
`sync_observation: null` even if reserved bits are nonzero; unsupported versions
remain errors. With a format-2 RBF and the existing `csl_runner.py --ssm`
workflow, per-marker records in `manifest.json` now contain
`ssm_records[].sync_observation`. This is a timestamped observation, not a new
run-wide configuration claim: `effective_settings.applied_b6_config` remains
requested-only and its applied fields remain null. Inspect the relevant marker
record and header format, not a later CFG file. The same markers still trigger
asynchronous screenshots, whose age remains unknown.

Use one Full SHAKER numeric run as before. For a deliberate output-boundary
comparison, repeat only the needed case with `--sync-filter raw-pixels` or
`raw-crt`, enable `--ssm`, and compare each marker's requested/applied modes and
state. A mismatch is observed pending state, not proof that the request failed;
application waits for filtered VBLANK and CPU phase, or reset. If no marker
arrives, there is no observation; neither the requested mode nor an old ring
record substitutes for it. Preserve the runner's startup/coherence checks.

The filter output is combinational observation only. Three registers move from
named-block to module declaration scope so ordinary output ports can expose
them; their update logic is unchanged. The motherboard packs the two modes,
and `Amstrad.sv` wires the bundle to the marker detector. There is no feedback
from the observer into CPU, CRTC, GA, filter or mode control.

## Verification and remaining debt

The focused new marker-time test failed on the previous behavior with an
observation of zero instead of the hand-packed `0xBEDA7`; after implementation
all 27 marker tests pass. It changes live observation across fetch, recognition
and stalled DDR publication to distinguish the sampling boundaries. The Python
format-2 test failed against the old reader (unsupported version); all 51 reader
and runner tests in that focused file pass after implementation, including
format-1 unknown-state handling. A disconnected motherboard diagnostic output
also fails the new dynamic wiring assertion; the real connection is restored.
These controls are retained as focused cross-module checks where applicable;
the temporary wiring and applied-mode mutants are not committed.

The existing executing-CPU SSM fixture uses TV80, not production T80pa. Its
marker tests and the passive capture cut do not close B4's original motherboard
raw-fetch-provider validation debt. Physical DDR allocation/coherence and
marker-time mode observations still require device acceptance with the new RBF.

A bounded production-T80 substitution was also attempted in the existing SSM
top: GHDL generated the production netlist, and an ignored copy of
`ssm_marker_top.v` changed only the CPU instance's named-port casing to that
netlist's VHDL-derived interface. The unchanged tests passed **26/27**. Basic
marker execution and publication passed; the interrupt-between-pairs case
stopped at its TV80-specific terminal-address assertion (`0x39` observed,
`0x38` expected), before reaching its event-count cancellation assertion. No
assertion was weakened and this experiment is not reported as a green gate or
a core regression. The temporary top and `production-t80.log` are preserved
under `output_files/b22-sync-acquisition/ssm/`. Even a green result there would
use the isolated harness's copied fetch expression and RAM mux, not the full
motherboard `cpu_data_bus`/WAIT integration; B4 provider debt remains open.


Fresh cross-provider review by Claude Opus 5.5 (medium) accepted the final
change **CLEAR** after two corrections: the passive assignment now follows all
its declarations for conservative Quartus compatibility, and the existing
live-transition fixture checks distinct requested/applied fields against its
independent transition oracle. Swapping those fields in a temporary RTL control
produced `transition=3152079/4` failures, exactly matching pending-transition
wait ticks, while tuple/byte/mask errors stayed zero; the correct wiring is
restored. Review runs: `20260929T002201Z-42089-067d` and
`20260929T002654Z-47353-f75d`; logs are retained in the artifact directory.

The post-extension nine-machine/mode CSV files were byte-identical to all nine
pre-extension files. The canonical CRTC soak also retained
`0xe99ab434a5e1cdb3` (`make -C sim soak SOAK_EXPECT=0xe99ab434a5e1cdb3`).
The selected fast gate and both relevant B6 slow benches are recorded below;
unrelated motherboard slow benches are not required by this passive port change.

Final selected gate (with the host include-path environment above):

```text
python3 sim/select_tests.py --run
select_tests: PASS 5 benches: crt-filter-blank-test, ssm-marker-test, run/p1_mobo_bench_tests, run/p10_boot_tests, run/p10_dma_mobo_tests
```

Log: `output_files/b22-sync-acquisition/ssm/final-gate.log`. The Python reader
command was `python3 -m unittest discover -s scripts/hardware-loop -p test_ssm_ring.py`
(51 tests, OK), recorded in `ssm/pass-python.log`.


Final focused slow verification: `make -C sim/plus -j2 b6-video-boundary b6-dynamic`
exited 0, with six boundary cases and nine dynamic cases passing after the final
code edit (`ssm/final-b6.log`). The requested/applied transition check passes on
both classic type 0 and Plus; pending waits remain nonzero. No test expectation
was weakened, the CPU program was not retimed, and no hardware-acquisition fix
is claimed.
