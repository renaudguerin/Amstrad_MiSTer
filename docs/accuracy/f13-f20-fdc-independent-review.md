# F13, F20, and shared FDC independent review — 2026-08-30

## Scope and reviewer

Claude Opus 5 xhigh reviewed `5a8f801..a468993` read-only through the guarded
cross-provider bridge. The review covered F13's half-character type-0 DE phase,
F20's integrated CRTC/GA R2.JIT timing, and the shared CPC/u765 transaction
changes. The reviewer did not run gates; parent verification owns every result
below.

## Pass 1 verdict: NOT CLEAR

Two blockers were both in F20:

1. The initial implementation replayed the actual JIT write phase at the
   trailing edge and therefore preserved the physical raw-HSYNC width. ACCC
   v1.11 §9.3.4.1 pp.53-54 and §9.3.4.3 p.57 say R2.JIT removes the left side
   of blanking without moving display reactivation: type-0/type-1 raw widths
   shrink by four/three Mode-2 pixels.
2. The integrated fixture required unchanged width, so it rejected the correct
   behavior and locked in the first error.

The review independently accepted F13's phase mechanism and its load-bearing
`t31a` vector, and accepted the CPC A0 write-alias rule. It also identified
non-blocking gaps: no type-0 R2.JIT vector, a latent same-value-write trigger,
the classic fixture's non-production clock ratio, an unclaimed u765 sector-size
syntax change, delayed post-reset host buffer traffic, the production/bench A0
expression mismatch, mid-pulse live-type behavior, and stale roadmap
hash/count prose.

## Remediation

- F20 now retains only each type's ordinary trailing-edge phase. Integrated
  production-ratio controls require type 0 at +4/-4 pixels and type 1 at +3/-3
  pixels, plus a same-value normal-path intent control. Production bus phasing
  lands that same-value write after HSYNC has risen, so it documents the
  invariant but does not make the defensive inequality guard load-bearing.
- The classic character-level suite and soak remain regression layers, not
  proof of the 64 MHz-to-16 MHz phase relationship; the integrated GA fixture
  owns that timing claim.
- The u765 reset path now quarantines delayed ACK and `sd_buff_wr` traffic until
  ACK has been observed low, then issues a fresh request. The focused test uses
  production one-in-eight CE, requires no request during the stale transfer,
  proves a seeded sector-buffer byte was not overwritten, and requires recovery
  after it retires.
- The production A0 expression is qualified with the selected FDC write, which
  matches the standalone wrapper's effective seam. Decoder comments were
  corrected.
- `SECTOR_SIZE` uses portable function-result assignment on every call. Its
  non-default-N transaction behavior remains explicitly unclaimed pending a
  READ DATA/DTL vector.
- Current hash/count/status documentation was reconciled. The corrected golden
  soak is `0x32d468e81eac63c9`.

## Parent gates after remediation

- `make -C sim`: 176 classic passes; every Plus bench; three integrated GA
  R2.JIT controls; three u765 transaction tests.
- `make -C sim lint`: pass with existing non-fatal warnings.
- `make -C sim soak SOAK_EXPECT=0x32d468e81eac63c9`: exact match.
- `git diff --check`: clean.

## Evidence still outside code review

DSC4 and SHAKER `(TAB)` remain the F20 hardware gates. SHAKER Module A `(O)`
and, if possible, a DE-pin capture remain the F13 hardware gates. The Demo's
`disc missing` report still needs an exact RBF SHA, disk hash, FDC setting, and
first motor/status/data trace. OUTI, an R2 update during an already-active
pulse, non-default-N/DTL reads, and a live type change during a phase-shifted
pulse remain separately named residuals. ACK-low quarantine narrows but cannot
tag a stale response which first arrives only after ACK was already seen low.

## Pass 2 verdict: NOT CLEAR on test integrity, RTL accepted

The focused Claude re-review confirmed both F20 blockers fixed and accepted the
quarantine RTL, A0 seam, function syntax, hashes, and documentation. It found
one remaining blocker in the reset regression: the delayed-ACK loop held `ce=0`
and never read the injected stale byte back, so neither retry suppression nor
the buffer-write gate was load-bearing.

The final test now:

- seeds sector-buffer byte 7 through the active pre-reset host transfer;
- runs the stale ACK/write window at production one-in-eight CE;
- requires no outward SD request during quarantine;
- reads byte 7 back and requires the stale `0xa5` burst not to replace `0x3c`;
- keeps mount state at the retry stage until quarantine ends, then requires a
  fresh request.

Two parent bite tests prove the assertions are load-bearing: removing the
buffer-write quarantine fails with byte 7 changed to `0xa5`; removing mount
retry retention times out waiting for the fresh request. Both mutations were
reverted before the final green gates.

## Pass 3 verdict: CLEAR

Claude Opus 5 high reviewed only `4d4e7ce..e9eeac0` and found no material
blocker. It independently confirmed that the stale window advances production
one-in-eight CE, the direct seeded sector-RAM readback makes the buffer-write
gate load-bearing, and retaining `image_scan_state` at the retry stage until
ACK-low makes fresh mount recovery load-bearing. It also confirmed that the RTL
and documents retain the transaction-tag limitation and do not claim either
u765 fix as The Demo's proven cause. F13/F20 hardware checks and a title-level
FDC trace remain external evidence gates rather than review debt.
