# Sonic preparation and timing diagnostics — 2026-09-22

## Integrated scope

| Scope | Accepted source | Integration | Evidence |
|---|---|---|---|
| Reference ingestion | `ba6705d` | `abe9fd3` | [35-item inventory](../../reference-ingestion/scrapes-2026-09-22.md), seven priority PDFs compared, diagrams checked; docs-only gates |
| B17 timed replay | `999e315` | `dadac2f` | [Device record](../hardware-runs/b17-input-replay-2026-09-22.md); five focused host tests, Opus review `20260922T024642Z-978-9f31` without blockers |
| TimeQuest path reporting | `96d783d` | `c59e03a` | Classifier/manifest tests pass; selector requires no simulation; Opus review `20260922T025122Z-19329-317d` without material findings |
| Sonic IRQ audit and T80 adapter | `2d68d74` | `b991487` | [Audit and gates](../sonic/irq-audit-2026-09-22.md); Opus architecture and fresh helper review |

Source branches were refreshed without code changes, merges were clean, and shared
status/roadmap evidence was reconciled. Existing review debt and the classic soak
`0xb1cb70da95c2e44f` are unchanged. No new Classic or Plus RTL behavior is included.
B17 implements replay only: Main's input grab invalidated the proposed passive
recorder. Sonic's PRI no-wrap policy remains backed by the Copter investigation;
weaker contradictory source prose did not reopen it. The IRQ audit found no justified
pre-trace repair; its capped production-T80 run covers startup, not gameplay.

## Exact-SHA build and delivered artifact

B17 run `35681070249` at `dadac2f` passed simulation but was superseded by the
next integration while production-T80 and synthesis were active. Its cancelled jobs
are not a completed acceptance gate. Despite a false path-classifier result for B17,
the workflow selects full synthesis for non-documentation default-branch pushes.

[Run 35681139829](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35681139829)
passed on `c59e03a033f71db4630292e53656ad2eacc7993b`: `simulation`,
`production-t80`, `synthesis-policy`, `route`, `synthesis (full)` and `required-gate`.
`synthesis-local` was correctly skipped. Monitoring used `gh run watch --exit-status`.

Artifact `Amstrad-build-234-1-full`, ID `10675555844`, Quartus 17.0.2,
`build_mode=clean_full`:

- RBF: `output_files/Amstrad_20260922_c59e03a.rbf`; downloaded/copied SHA-256 both
  `da28d0cd01c910ce68d7bfc7482730dbdb1ae235dc814894776f3f06308e3335`.
- Reports: `output_files/reports-c59e03a/`, including `Amstrad.sta.paths.rpt`.
- 23,804 ALMs (57%), 28,343 registers, 102 RAM blocks, 701,596 memory bits, 35 DSPs.
- Worst setup +0.561 ns; HDMI setup +0.636 ns; worst hold +0.242 ns; all TNS zero.
- The new report contains 20 setup and 10 hold paths with real endpoints and full
  clock/data delays, proving the hook ran in Quartus 17.0.2. Default analysis remains.

The previous `60a63e4` HDL-comments/host update failed HDMI setup by 0.006 ns and
retained no path detail. A bounded Opus diagnosis (`20260922T024619Z-283-c955`)
confirmed that the archived artifacts could not identify its failing endpoint. The
new report hook changes neither constraints nor seed; it is a diagnostic improvement,
not proof of the prior failure's cause. Commit-stamped build constants mean successive
builds are not byte-identical synthesis inputs. Do not relabel this artifact as the
later audit integration or claim it retrospectively validates the failed build.

## Hardware boundary

B17 used hardware-tested `88262b9`, proved Sonic fire navigation and same-device
key-release cleanup, then restored configuration/mapping and returned MiSTer to MENU.
Ignored evidence was copied to the integration checkout under
`docs/screenshots/b17-input-replay-2026-09-22/`; audit logs similarly live under
`docs/screenshots/sonic-irq-audit-2026-09-22/`. Private media and evidence are not committed.

The followup Sonic comparison owns the MiSTer and AmSpirit exclusively and starts
from qualified baseline media/model identity. Its later device results belong in a
separate dated report. This preparation record does not claim a Sonic fix, deterministic
gameplay replay, B16 hardware closure or full B18 model coverage.
