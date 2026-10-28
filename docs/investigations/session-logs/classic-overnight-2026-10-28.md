# Classic CRTC unattended evidence, 2026-10-28

## Position and scope

This run starts from `5ade887` on the MacBook Air M5, in the task-owned
`codex/accuracy/crtc1-module-d-replay` checkout. The user authorizes integration
and push. Existing worktrees, stashes and private inputs are preserved. Sol
6.1 high performed the bounded module-D replay and snapshot comparison;
parent owns device operations and acceptance.

No RTL, production code, committed test or soak golden changed. The evidence
narrows two investigations without inventing a hardware rule: D1's `0305`
stop has a script/input workaround; `03AE`/`03B4` depend on prior state in the
model and need the same history on original hardware before an RTL repair.
`019D` and `008D` retain their independent evidence requirements.

## D1: sufficient input overcomes the `0305` stop

The unmodified supplied `CSL_27` D1 script actually loads `shaker26.dsk` /
`SHAKE26D`. On exact release `321f96a`, it again times out at line 81,
`wait_ssm 0305`: 30 captures, zero capture failures. Its saved caller is an
interactive sweep, not a VSYNC-wait wedge. A2FD is delay-return padding;
the stack returns to 710B, which repeats the sweep through 7093 and polls
input through 9FE0/7107.

The loop starts at length 40, outer count 3A, and decrements until count 09:
49 accepted advances. The supplied script sends 36+11=47 spaces after
`0302`. Both old and release snapshots retain length 16, count 10, equivalent
to only 42 advances. The release snapshot `D1-release-1.sna` has PC=A2F3,
SP=405D, C0/C4/C9=51/1/1, SHA-256
`e855e5092a13eaeb19d596247c5b6464e4bf1155457c50aa5e39786378120e75`.
The first pull timed out; retry succeeded at generation 1. Repeat-save
navigation did not produce a later generation, so post-save input controls
were inconclusive and are not used to establish the cause.

A scratch copy preserves boot, media and earlier tests, then changes the
final sweep to 36+13 spaces with `key_delay 250000 250000 400000`, stopping
at `0305`. It completes on the same release: `status=success commands=69
captures=31 capture_failures=0 ssm_records=41`, returning to the module menu.
Count and pacing changed together; this does not establish a minimum reliable
count or isolate key repeat/debounce. Markers emitted inside the key burst
were drained afterwards: captures `0303`–`0305` show the same menu. This is
bounded progress proof, not numeric-page correctness or a whole-module pass.
The source script is unchanged. See the [D1 report](../hardware-runs/d1-27-stall-2026-10-27.md#sufficient-input-device-control-2026-10-28).

## Module D: cold-entry controls and retained state

Authentic SHAKE26D and SHAKE27D entries A=4966 / C=4994 execute their real
setup, screen draw, VSYNC lock and measured body with production T80/GA/CRTC
timing. From reset, both reach row 1 at `3190 + 40 = 31B8`. Three SHAKE26D
iterations retain `31B8..31DF` across all eight row-1 rasters. This is bounded
replay: immediate RAM, direct PPI VSYNC, BC00 RET stub, incomplete loader/font
and marker state. It is not a full motherboard verdict.

Cold-boot direct E reaches `03AE`/`03AF`/`03B4`, visibly OK, on both old
`cec641c` and released `321f96a`: each walk has 14 commands, three captures,
zero failures and five SSM records. Corresponding PNGs are identical:

| Page | SHA-256, old and release |
|---|---|
| `03AE` / `03AF` | `c91dea6b9b3b161b2cba377a1bea0d410552847eb3417ee52bf52beeaa8ef71b` |
| `03B4` | `fcb90f46fa95fd8f3c16f1f4a2833ddb9c6b31fa4bd45c3d22a1451cb28c8d79` |

The historical October 15/16 full-walk captures were pulled and visually
rechecked: they really say WRONG. The saved original CFG and applied
classic CRTC1/Full/SSM CFG hashes match those runs exactly (`f8c6879a…67cf3e`
and `022f2aae30beb5237f42036bda82ed68370afff135558f210740dba865742abc`).
A build change alone does not explain the old result on the short path.

An old-build `9`-then-E control displays OK but repeats the text across
character rows and hides the explanatory header: 34 commands, 15 captures,
zero failures, 27 SSM records. Snapshot `E-after9-old.sna` has SHA-256
`dd1ecab063f3702a2bfdbd5bd0c4c4e5259e3afea643aaaab0dd87a73bc22d45`.
RAM comparison with direct-E snapshot `E-03AE-release.sna`
(`c5cd9f998a56af521bef540f22927178714b1a5534f9f8af24e14ad40362f972`)
finds intact font/header data and one OK string. All 528 differing VRAM bytes
are intentional A/C explanatory text. Saved CRTC/GA/RAM/PPI configuration
matches. SNA omits RFD flags, parity and video-address latches; loading a
snapshot clears candidate RFD state and cannot preserve this discriminator.

### Bounded RFD/parity mechanism probe

A scratch experiment executes R5=1 then R5=0 at an R0=63 rollover, then
resets only the CPU before running authentic E. This is a synthetic
precondition, not an authentic menu-9 replay. Two iterations each produce:

| Precondition | Row 1 MA | Following rows |
|---|---|---|
| Reset, no RFD | `31B8` (OK) | Advance normally |
| CPU-executed RFD | `3028` (WRONG) | Advance normally |
| Same RFD, one frame later | `31B8` (OK) | Repeat `31B8` through row 10 |

No RTL or internal signal was injected. This shows how retained RFD/parity
phase can produce the observed layout classes; it does not establish which
is faithful after the same history on original CRTC1. R3 was corrected to
the module/snapshot's 0E for these final probes; earlier 8E logs are explicitly
distinguished and unchanged. Do not turn observed addresses into required-pass
vectors or clear hidden state on register initialization without an oracle.

The French ACCC v1.11 was read directly through pdf-inspector for
§§11.6–11.6.3 pp.89–92, including a render of p.91. §11.6.1 p.90 describes
parity-dependent VMA' saves; §11.6.2 p.91 describes subsequent-frame effects
and a minimum within-frame lifetime. In context this does not justify a
blanket frame-start clear. A bounded check of French §19.5.3 pp.209–210 (including rendered p.209 table)
finds that a frame origin aligns ParityC9 with newly toggled ParityFrame;
it does not force absolute even parity. Current engine/wrapper assignments
match that boundary. A paper E sequence retains the initial phase after
its short frame and even-R9 row. No source/model contradiction was established;
RFD lifetime after the same history still needs original-hardware evidence.
The source check is retained in `module-d/parity-source-check.md`.

The original full 2.6 menu-history walk through `03B4` completes on old
`cec641c`: 191 commands, 124 captures, zero capture failures, 178 SSM records.
Its `03AE`/`03B4` PNGs reproduce the October WRONG captures pixel-for-pixel
(`49eb3655…bf3936` and `8fff451c…33eb0`); `03AF` remains OK. Commands are
unchanged, line endings normalized, and execution stops after that wait.
The original full-script SHA-256 is
`32e968d0488779e08664ada2435fcefb1bec718a3ffa548df9006158bc0cb936`,
identical across the two Macs.

Snapshot `E-full-history-old.sna` SHA-256
`44d2faa87e6a7b48cf3544391cecd38a2066677701195b4ab44252373f7bc916`
has the same saved CRTC/GA/RAM/PPI configuration, E body, sync/delay helpers,
firmware wrapper, font and entire VRAM as the repeated-OK C snapshot after
menu 9. This is a concrete retained-state discriminator, not RAM corruption.
[Author question 23](../../classic/accc-author-questions.md) records the missing
same-history original-hardware/lifetime evidence and has not been sent.
The first equivalent release walk stopped before E, after 24 captures,
when MiSTer SSH authentication failed. Its cleanup also failed to restore
CFG. A bounded access retry succeeded; the original CFG was then manually
restored and MENU verified before starting one fresh full-walk attempt.
The failed manifest/logs remain in `full26-e-release-a1/`. A snapshot cannot
resume this experiment because it omits/clears candidate hidden history.
The one fresh release attempt is the final live control.

## Artifacts and verification

Private evidence is under `docs/references/classic-overnight-2026-10-28/`
(the repository alias resolves to `docs/specs/`). It includes decisive PNGs,
manifests, SNAs, scratch CSL controls and the module-D source/trace bundle.
Reproduction commands and limitations are in `.roster-scratch/module-d/README.md`
and `snapshot-analysis.md`, copied to the private evidence's `module-d/`.
The remaining full run is retained on the Mac mini under
`/tmp/crtc-overnight-20261028/full26-e-old-a1/`; preserve decisive outputs
before that host's temporary storage is cleaned. Media and PDFs stay untracked.

RBF identities:

- Release `321f96a`: `6bc437243db9ef5f9db2349f420520a46364dee4b7954ccc95e96f55ba36400d`.
- Old `cec641c`: `40f7d66bfcfda37ce9f5053ff3e3af833713bdb4065fe4bcde9c311a3cb4dd2c`.
- MBC helper: `f2b3a4f3cbd98c7a13f8770eb4c893bc352eae9d204f92f664f5eaad6c697330`.

The release RBF was copied unchanged to this task's
`output_files/Amstrad_20261027_321f96a.rbf` and its full hash verified.
Exact-source [CI run 40521275117](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/40521275117)
was rechecked: simulation, production-T80, synthesis-policy, route, full
synthesis and required-gate pass; local synthesis is intentionally skipped.
This is ancestor artifact provenance, not a new build or new-tip CI evidence.
Documentation-only changes require no simulation or independent code review.
The synthesis classifier returns false, and the push workflow excludes docs.

One Opus medium review of the untracked diagnostic was attempted through
`ask-claude`; it failed before producing a review (OAuth 401, expired token).
The supervisor exited cleanly and reaper reported no live bridge workers.
The diagnostic remains author analysis, not independently reviewed code or
hardware proof; a future RTL change needs a fresh cross-provider review.
The preserved incident is `.roster-scratch/bridge-incidents/20261028T060434Z-opus-review-auth.md`
(intended destination: `~/code/agents-roster/inbox/bridge-incidents/`).
Worker/coordinator wasted token/cost is unknown; runtime was 6.2 seconds.
No credentials or account settings were changed.

## Continuation and boundaries

The local heartbeat successfully resumed this chat at 05:30 after a deliberate
quota pause (94% short-window usage before reset, 0% after; weekly 48%). This
verifies that case only, not recovery after sleep, restart or quota exhaustion.
Two banked resets remain untouched. The one-shot wakeup is removed on completion.

Next useful work is same-history original-CPC evidence or source clarification
for RFD/parity lifetime, the physical write-visibility rule for `019D` (including
B8 assumptions), and an original-hardware discriminator or author answer for
`008D`'s width-counter clock. Snapshot re-arming is separate tooling work.
No speculative latch or counter change is justified by these results.

Final teardown must verify original CFG SHA-256
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`
and `CORENAME=MENU` after the remaining capture completes.
