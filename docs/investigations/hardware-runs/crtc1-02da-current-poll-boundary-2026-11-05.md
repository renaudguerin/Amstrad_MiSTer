# CRTC1 O/02DA: current failure and first-poll qualification

The current experimental90f3b18 RBF still prints0050 instead of0060 in all
four R7=0 rows of original SHAKER C page02DA. The other eight rows retain
3C20/1E20. AmSpirit1.17.1/core2491682 running the same original normal-boot O
sequence gives the expected twelve values. The recent IVM adjustment repair
therefore did not close this residual. No new timing rule or RTL edit follows
from the scalar alone.

## Original caller and bounded native run

This is direct normal-boot O, traversing D4/D5/D6/D8/D9 before DA. It is not
direct PC entry, and equivalence to the full canonical C-module history is
unproved. Native Full-filter CRTC1 run completed19commands/6captures/zero
failures/10SSM records. Root inspected all six PNGs. The disk SHA256 is
6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61;
RBF SHA256 is a7c3b0ad177744a3c2e2120365c3febdbd8226f0f80782603ccb1e3e8008a102.
Private receipts are under
`local/overnight-20261103/resumed-20261104/ga-native-build/device-c1-direct-o-90f3b18/`.
The original O dispatch, literal table and poll formula are source-qualified
in the adjacent private`c1-02da-source/REPORT.md`; keyT selects a different test.

## AmSpirit first R7=0 row

A second normal boot used the same original sequence, with execution
breakpoints only. The first DA acquisition has IY57FC; the third has IY5800,
identifying the first original R7=0 row. ROM language is FR, model6128,
CRTC1,128KiB. The settled configuration after script reset reports
freq_screen1. No guest RAM, PC or hidden-counter writes were used. Original
5575..558D instructions and57FC..5813 expectation table were byte-guarded
against SHAKE27C body SHA256
3647e120e282412d5afc31bed0e4b6673beed850ba94ec7a4d5374ce04a4682f.

| Stop | Emulator tick (µs) | Observation |
|---|---:|---|
| Third acquired row,554F |42156766|IY5800; R8=0,R9=7,R4=38,R5=0,R7=30|
| R7 OUT before,5575 |42156818|R8=3,R9=7,R4=77,R5=0,R7=30|
| R7 OUT completed,5577 |42156822|R7=0|
| First IN completed,5581 |42156845|A=1E,PB0=0,HL=0004|
| Second IN completed,5581 |42156861|A=1F,PB0=1,HL=0005|
| Poll exit before doubling,5588 |42156871|HL=0006|

The second5581 stop is a distinct occurrence,16µs after the first; breakpoint
replacement did not silently reuse the prior stop. Actual HL4→5→6 proves
two polls without a modular-wrap ambiguity in this reference row. The caller
then prints6×16=0060. These are instruction-completion observations, not
measurements of the physical PPI sampling aperture or hidden CRTC phase.

Private evidence:`amspirit-c1-02da-poll-probe/run-001/` retains all write
before/after stops, operations, immutable source checks, original boot/tail
scripts and final cleanup. `amspirit-c1-direct-o-current/run-001/` retains the
complete twelve-row reference screenshot. Its initial post-run state dump
failed because the destination subdirectory was absent; the original script
had completed. Root created the directory and recovered the same paused state
without restarting or changing guest execution. The error and recovery remain
recorded. Script and breakpoint sets are cleared, emulator paused.

## Matching CPCSyntaxError and a concrete source omission

A fresh isolated CPCSyntaxErrorv0.12.0 original-O batch also completes all six
pages and gives all twelve expected02DA values. Binary/source identity is the
same pinned pair as the [B6 experiment](crtc1-b6-cpcse-ordering-2026-11-05.md);
this run uses packaged UK firmware, `--no-chain --no-errata`. Root viewed all
six pages and full DA. Private`cpcse-02da-current/` preserves script identity,
logs, original BMPs, PNG copies and hashes. Its first R7=0 trace is frame1931,
C0/C4/C9=2/0/2 at PC5577; this is its character-level dispatch, not measured
native hidden phase.

Matching source`src/core/crtc.cpp:759–817` applies active MID-VSYNC to a live
matching R7 write before the midpoint, and uses live frame parity for the
write rather than frame-origin parity. Inspection of our current wrapper
finds a concrete bypass: the live equal-R7 path sets logical VSYNC immediately,
independent of `vsync_mid`; the existing midpoint arm is populated only by the
line-seam path. French ACCC§19.7.1–2p219, re-read in the position-aware layer
and rendered page, says active MID-VSYNC triggers at C0=R0/2, with even parity
including R7=0. This gives an independently documented before-midpoint rule;
it does not select a new numeric delay or an after-midpoint policy.

The source-derived natural-state vector fails before implementation: after
four natural frame origins, evenPF/R8=3/C4=0/C9=2/C0=8, a matching R7 write
raises logical VSYNC before the midpoint. The oddPF sibling and existing
ordinary partial-width/held-active/rearm controls pass. The initial odd
fixture accidentally requested unreachable C9=2; that attempt is retained,
and only its setup guard was corrected to C9=3. The even failing assertion
and phase did not change.

## Experimental source correction and proof

An engine-owned one-bit accepted-comparison ticket carries the already-consumed
write to the existing midpoint/start/count path. Register storage is unchanged;
no numerical delay is added. Explicit interlace/MID qualification preserves
ordinary/odd writes, while a write coinciding with the actual midpoint starts
there. Reset, SNA apply and live Type1 exit clear the context-local ticket.
The wrapper only connects engine contributions to shared sequencing. Type0,
GA, CPU and ordinary pin transport are unchanged.

The candidate passes15 focused cases, including full16-line logical/public
width, actual-midpoint coincidence, three context replacements and existing
origin/overlap/active-R8 controls. Parent caught an unreachable tail assertion
using an8-bit helper for the16384-clock width; the old red/source is preserved,
and the tail was corrected to a full unsigned comparison before candidate
qualification. No passing width is inferred from the earlier failing run.
The original wrapper-owned prototype and its receipt are also retained; the
final engine-owned port passes the identical15-case test source.

The retained suite adds six live-write/context cases to the existing owner,
reusing existing D1/F3 registrations. `make -C sim crtc-test` gives246 passes;
`make -C sim soak` gives e30456ff0f009775 with unchanged seed, stimulus and
sampled projection. Final `python3 sim/select_tests.py --run` passes46 benches
after the last code edit. Root verified all four source hashes against the
worker's final manifest and accepted its receipt without rerunning the gate.
Final wrapper/engine SHA256 are
07f5f2268179a897139501b0b6db3c42931becd2340b9f7a064220e4e2a3e944 and
b7940b789f19deda98250c60e9cfcdb78e592a59d650415d52cfd09cb945a0c9.
Private proof/patch/gate receipts are under`c1-02da-current-replay/` in
`mid-red/`, `mid-candidate/` and `production-adoption/`.

Pending R8/R0 modifications inherit the ordinary MID line lifetime. Unlike
the ordinary seam arm, the live write has already consumed allow; changing
MID selection before onset can drop the pending pulse. This wider case is
unqualified. No new
physical claim is made about them, after-midpoint or short-line cases. A later
unequal R7 write does not revoke an accepted ticket, matching the existing
seam-arm policy. Strong Claude Opus5.5 review accepts this C1 source/RBF change within its
stated bounds; its [debt entry is archived](../../archive/review-debt-cleared.md).
It does not clear the separate GA G1 regression found in the same review.
Full synthesis and native0050→0060 verification now pass on5261414 (details below). The separate
non-IVM B6 admission and optical9/5 questions are not explained by this rule.

## Current-T80 discriminator

Observe the same first R7=0 row using current production T80/CRTC sequencing:
R8/R9/R4/R7 writes, C0/C4/C9/C5 and adjustment, both parities, logical and
published VSYNC, and the actual first PPI sample. Preserve authentic entry and
source identity. The existing source audit already finds incoming parity at
R7=0 origin and parity-aware IVM row counting implemented; do not duplicate
those rules or move a VSYNC edge merely to make0060 appear. A native0050
print alone still does not prove the first poll count or an exact16µs early edge.


The current-T80 replay uses the genuine normal-menu v2 architectural snapshot;
its omitted hidden counters are naturally normalized by the existing loader,
not restored measurements. The first attempt's PS2 builder incorrectly ORed
Space with the old O payload. Root found the exact bad6D versus required29
payload before accepting any target data. It stopped with exit130; root
confirmed no live process, and source/logs/partial traces are preserved in
`attempt0-invalid-space/`. Only its O→D4 prefix qualifies. The corrected
attempt preserves the same menu and original instructions, with fatal exact
O/Space make/break guards and identical firstD4 tick485807303. It completes with the corrected Space at1,341,102,392masterclocks, reproducing
native0050 as rawHL5/one poll.
No late PC/counter fit or hidden-state checkpoint is claimed from that repair.


The qualified current-T80 baseline observes the actual R7=0 write at tick
1,341,100,167, C0/C4/C9=2/0/2, C5=0/adjustment off, PF/PC9=0/0,
effectiveIVM1 and old allow1 with no active pulse. Logical VSYNC rises one
master tick later, public VSYNC six ticks later, both before the midpoint.
The first PPI interval returns PB0=1, and the sole poll exits with HL5. This
qualifies the documented before-midpoint class in the original caller, with
architectural snapshot limitations retained. The identical candidate replay now completes at1,341,103,416 clocks and gives0060/rawHL6/two polls. All2,912 CRTC writes, five markers and ten key events are byte-identical; the collected prefix through R7 acceptance also matches12,322 state events,1,698 checkpoints and3,473 dense clocks. This is a comparison of recorded observables, not an unrecorded whole-machine hash.

The first recorded difference is intended: at1,341,100,168 baseline logical VSYNC is1 while candidate remains0. Candidate logical VSYNC starts at1,341,102,015, actual C0=31/evenPF midpoint, with public VSYNC five clocks later. First IN completes at1,341,101,751 with PB0=0, second at1,341,102,775 with PB0=1; rawHL6 follows at1,341,103,415. Acquisition-relative OUT/IN/IN/exit timings56/79/95/105µs match the original AmSpirit checkpoints. The CPU branch diverges after the changed PPI read. Only the first R7=0 row is replayed; all-row native acceptance remains pending. Private `original-o-candidate/HANDOFF.md` and `COMPARISON.json` retain the exact source tuple, all first-column differences and immutable receipts.

## Targeted native acceptance on5261414

Clean full Quartus17.0.2.602 build41980916339 passes all workflow jobs, seven setup/hold groups with minima0.588/0.242ns and zeroTNS. RBF`Amstrad_20261105_5261414.rbf` SHA256`4282cdb6f2f6f306bba9b6ce0d7fe541455e125b99183dc8e390d6ed4d936d7b` is verified identically after installation. Production diagnostics are off. The host-only Makefile repair in8a62a3a changes no synthesis input.

The original normal-boot O sequence on CRTC1/Full/FR completes19 commands, six PNGs, zero failures and ten SSM records. Root inspected all six images and full02DA: all twelve rows match3C20/1E20/0060, including0060 in all four formerly0050 R7=0 rows. This closes the observed original-O02DA residual on this build; it does not claim canonical whole-module history or hidden native first-poll timing. The source proof and current-T80 replay supply the bounded causal explanation independently.

Exact device settings, media/RBF identities, captures and marker records are under`local/overnight-20261103/resumed-20261104/g1-native-build/device-c1-original-o/`. Other known B6/optical and author-question boundaries remain separate.
