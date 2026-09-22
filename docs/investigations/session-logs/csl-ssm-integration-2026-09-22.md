# CSL 1.5 / SSM 1.2 integration — 2026-09-22

## Source and review

Source `40fb7e3` was refreshed onto `1d7d8da` as `c68b997`; `git range-diff`
confirmed both source patches unchanged. The original task checkout was no longer
present, so the preserved source tip was refreshed in a managed replacement worktree.
Integration `60a63e4c9179af9dcdd269c401efeac9cdc01bfa` merged cleanly. Integration
edits were limited to roadmap scope: validation gaps belong in the execution queue
only when they gate a named milestone or selected task. The generic FDC validation
paragraph was removed; its residuals and evidence remain in current status and the
linked investigation records.

The host runner supports CSL 1.5 `wait_ssm 0xHHLL` with bounded, one-shot code
matching, preserves unrelated queued events, and clears them on hard reset. Legacy
`wait_ssm0000` and SHAKER 2.6 scripts remain supported. SHAKER 2.7 scripts are future
inputs. SSM 1.2 requires consecutive `ED LL ED HH` opcode fetches; existing detector
logic already conforms, so the RTL change only updates comments. The supplied PDFs
remain ignored under `local/test_media/shaker/`; provenance is in
[the specification index](../../specs/README.md).

Source gate results were verified in the task execution transcript:

- `python3 sim/select_tests.py --run`: `select_tests: PASS 1 benches: ssm-marker-test`
  (26/26 cases).
- `python3 -m unittest discover -s scripts/hardware-loop`: 177 tests run, OK,
  12 skipped.
- Gemini review `20260922T010257Z-79129-bbcb` found a stale version assertion and
  requested reset-boundary coverage. Both were repaired; remediation review
  `20260922T010939Z-81150-3cf5` returned CLEAR.

No code changed during integration, so exact-SHA CI supplies the merged gate without
a duplicate local run. Existing review debt remains open. The classic soak hash
remains `0xb1cb70da95c2e44f`.

## CI and artifact

[Build run 35677844676](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35677844676)
targets the integration SHA above. The path classifier requires full synthesis because
`rtl/ssm_marker.v` changed, despite its changes being comments only. `gh run watch --exit-status` followed the run to completion. `simulation` (26/26
SSM cases and lint), `production-t80`, `synthesis-policy` and `route` passed.
`synthesis (full)` compiled successfully but failed the timing gate; `required-gate`
failed accordingly. The alternate `synthesis-local` leg was correctly skipped.

Artifact `Amstrad-build-232-1-full` (ID `10673779759`) contains reports only:
packaging was skipped after the timing failure. Reports are retained in the integration
checkout at `output_files/reports-60a63e4-failed/` (ignored).

- Quartus 17.0.2, `build_mode=clean_full`, seed 1.
- 23,801 ALMs (57%), 28,347 registers, 102 RAM blocks, 701,596 memory bits, 35 DSPs.
- HDMI PLL output-clock setup slack and TNS: **-0.006 ns**. All other reported TNS
  values are zero; worst hold slack is +0.178 ns.
- No new RBF was packaged or delivered. The last timing-clean artifact remains
  `output_files/Amstrad_20260922_37ccfc3.rbf`, SHA-256
  `a3c9fb8eba80a64df2928bc55d2cca7d68a9a302121b6678dad07ded04a3780c`.
  It is an ancestor build, not validation of `60a63e4` timing.

The standards patch changes no RTL behavior, but `sys/build_id.tcl` embeds each
commit SHA in the synthesized OSD configuration string. Therefore these builds do
not have identical synthesis inputs. The available report identifies the failing
clock domain but does not include detailed failing data-path endpoints; it does not
establish the cause. Do not weaken timing constraints or repeatedly change seeds to
hide this result. A targeted timing follow-up needs the failing path report and a
reviewed repair, then a successful exact-SHA build. Integration is committed and
pushed; full stream-finish acceptance remains blocked on this build gate.

## Remaining acceptance

A bounded live `wait_ssm` check remains before device acceptance. It can use an existing
Phase 1 SSM-capable RBF: the standards refresh changes no hardware logic. This integration
performs no device or AmSpirit mutation. B16/B18 hardware residuals are unchanged; see
[their integration record](b16-b18-integration-2026-09-22.md).
