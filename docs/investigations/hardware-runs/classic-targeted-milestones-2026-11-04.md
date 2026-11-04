# Classic targeted milestones — 4 November 2026

`599ca80` has bounded native acceptance for the Type 0 Last Line correction and
Type 1 E result. The later `e574a46` ordinary-VSYNC publication correction keeps
E/P and the named DI/B6 controls, but changes the existing full-history S
failure. It remains a separate investigation build. No broad baseline was run.

## Exact builds

| Source | Full workflow / artifact | RBF SHA256 | Bytes | Min setup / hold |
|---|---|---|---:|---|
|599ca8098c655556457a6066b2e4fdeed2d8a9a7|41878239893 / Amstrad-build-356-1-full /13348697814|8c21cfeb24ed5cb33570e04a93add45aefb146f82e2dd169385b34059f02c87f|3121608|0.356 /0.244 ns|
|e574a46fccd392f4de7bd277fd6fe2b3558a3b5f|41881983617 / Amstrad-build-358-1-full /13350633612|f95bac8d60821cee62329d9314bd63811936a8102c892f47d484cfa74ae19df9|3134640|0.575 /0.243 ns|

Both clean full builds pass every required job, seven setup/hold clock groups,
zero TNS, and no compiler errors/critical warnings. Installed MiSTer RBF hashes
match the retained artifacts. The earlier c3e896d-only artifact met timing but
failed its historical negative fixture; that adapter was fixed in 28ba886.
It was retained without hardware acceptance. No synthesis was canceled.

## 599ca80 native checks

- Direct C0 E6: 16 commands, 4 captures, 0 capture failures, 9 SSM records. Case 5
  changes 0000→00C0 and case 14 changes 001B→0D97, matching printed expectations.
  Relative to accepted ee9, only those two numerical rows change; 0409 is
  pixel-identical. Cases 21/24 remain 0066/000E. Twelve of 28 unique program
  checks still differ, down from 14. This is a program self-report, not a
  real-CPC photograph failure rate; C0 E photographs are not cached.
- Direct C1 E/P: 15 commands, 3 captures, 0 failures, 6 SSM records. E 02C9 is 0080;
  all seven other numeric controls remain 0004/0B20/0073/0077/04A0/0B20/0223.
  E 02CA stays readable and P 02CB remains 26. The E/P photos corroborate 0080/26;
  the separate 02CA photograph remains unavailable.
- C1 DSC4: corrected 19-command run, 4 captures, 0 failures. Four credit colons and
  names are clear; static/main/later rotating/fire-band samples remain visible.
  This is bounded regression acceptance, not full-demo completion. Root first
  inherited `--ssm` from the SHAKER argument array: 63 unintended EDED captures
  delayed that run's checkpoints. It completed naturally and is retained without
  controlled timing acceptance. The accepted repeat omits `--ssm` and has 0 records.
- C1 canonical history through S: exact author-script prefix, 131 commands,
  64 captures, 0 capture failures, 121 SSM records. E retains 0080, but S 02D3 remains
  0640 WRONG versus 4E40. Fresh direct S on the SAME artifact remains 4E40
  (10 commands, 1 capture, 0 failures). The history path is a necessary known-defect
  discriminator, not a replacement baseline. Of 64 images, 52 are pixel-identical
  to the prior db4 walk. Parent inspected the 12 changed images: one E scalar
  correction and 11 row/raster-pattern views; their photo verdicts were not
  refreshed. No claim of universal absence of regressions follows.

All collected SHAKER sync observations request/apply Full 0. The demo has CFG
write evidence rather than an SSM applied-mode observation. Screen capture is
Main's asynchronous scaler grab; a stable numeric result differs in authority
from a dynamic row/raster pattern.

## e574a46 native checks and remaining bounds

E/P's three PNGs are pixel-identical to 599, including 0080 and 26. DI 02E3 keeps
the known DEC DE 58 versus 59 discrepancy; SET 40, CP indexed C4 (an allowed variant),
pending 00, R52=0018 and break 00 remain within their displayed expectations.
B6/019D remains fifth/fourth 1F/1F versus 1E/1F, with its other displayed R0
controls still GOOD. Uniform publication does not repair equality admission.

The same canonical C1 prefix completes 131 commands/64 captures/121 records without
capture errors, but S changes 0640→5060 WRONG, still versus 4E40. Fresh direct S on
this source gives 4E40 (10 commands, one capture, zero failures, one SSM record). This records the history-dependent failure across sources; it does not
isolate a publication-phase/C5 mechanism or justify dismissing a
computed result as screenshot phase. Keep 599 as the conservative comparison.

Of the 64 images, 50 are pixel-identical to the inspected 599 history. Parent
inspected all 14 changed images: 13 differ in raster-pattern rows and the last
is the S scalar change. The comparison ledger and five panels are retained in
`r7-njit-control/history-inspection/`. This does not refresh their photo verdicts.

A normal-boot own ordinary-NJIT guest reaches its expected AA00 display on
MiSTer. Its stable native PNG contains the split active picture and frame
border, but no VSYNC-black endpoint: that part lies outside the captured
viewport. Therefore it does NOT establish a sixth-pixel device measurement.
The independently bound 5/6 endpoint remains a French-source/AmSpirit/bench
result. The initial unsupported screenshot syntax was corrected after a
parser failure and dry-run; no timing or guest media was fitted.

The [ordinary publication report](crtc1-r7-onset-controls-2026-11-04.md) owns
rule/gate/review bounds; the [S history report](shaker-s-history-2026-11-04.md)
owns its unresolved cause. Native captures cannot replace physical CPC timing
or reveal all hidden state. Current MBC has no supported live status_set;
B18's classic RAM export works after an OSD Save snapshot action, but that
human action is not yet available through this automation. It must not become
an excuse to invent a replay's missing latch/phase.

## Evidence and current device availability

Private evidence lives under `local/overnight-20261103/resumed-20261104/`:
`build599-targeted-acceptance/` and `r7-njit-control/`. Complete build logs,
source/job metadata, media hashes, manifests and original PNGs are retained.
The installed current author disk remains 6b9ff106…eb61; no package was replaced
by a diagnostic. Original/modified A4 diagnostics are a separate pending unit.

After the completed e574 C1 history, the first A4 native-original attempt failed
SSH authentication before a new accepted capture. One clean SSH retry was
also denied; root requested the user's SSH-agent restoration asynchronously.
Last confirmed device state is e574/C1/S5060, not a completed A4 experiment.
Device authorization remains available; authentication is the blocker.

## Subsequent bounded DSC4 control

On e574a46, the existing CRTC1 DSC4 recipe completes19commands/fourcaptures/
zero failures/zeroSSM records. Parent viewed all four images: the credits'
colons and names remain clear, and static/main/later rotating/fire bands
remain visible. This preserves bounded regression acceptance, not complete
demo or monitor-edge acceptance. Evidence: r7-njit-control/device-c1-dsc4-e574/.

SSH was subsequently verified outside the sandbox and the hardware runs
resumed without user authentication changes. The earlier access failure
remains recorded; it is no longer a device availability blocker.

The matching bounded CRTC1 Amazing recipe also completes13commands/two
captures/zero failures/zeroSSM records on e574a46. Parent viewed both: the
animated logo and lower text/QR region remain visible. This is sampled
regression acceptance, not full-demo completion. Evidence:
r7-njit-control/device-c1-amazing-e574/.

## 9a91920 targeted CRTC0 acceptance

The subsequent [canonical-row VSYNC correction](crtc0-r4-switch-vsync-2026-11-04.md)
has full timing and exact-source native acceptance for E6 case25=051B.
Direct E6 completes16commands/fourcaptures/zero failures/nineSSM records;
11/28 unique printed checks remain wrong. The linked report records the
complete tally, build/hash and correction to the earlier0003/0037 reading.

The CRTC0 DSC4 recipe on the same installed RBF completes19commands/fourcaptures/
zero failures/zeroSSM records. Root viewed every PNG: four colons and names
are clear, with the static picture and sampled rotating/fire-band effects
visible. This preserves bounded regression acceptance, not full-demo or
physical monitor acceptance. Evidence: c0-row-vsync-build/device-c0-dsc4-9a91920/.
Current MiSTer state is this CRTC0 DSC4 run; no hardware job is active.
