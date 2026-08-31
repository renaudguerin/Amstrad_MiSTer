# Plus P10 review-debt status — 2026-08-31

**Checkout inspected:** `plus/p10j-resource-timing` at
`d74cd2429f1a0fef0a3b00cf45e900d6d02c518c`

**Mode:** read-only history, report, RTL, fixture, and recorded-gate review before P10j.
No simulation, synthesis, or hardware test was rerun for this classification.

## Verdict

The inherited P10a-P10f/P10h **local RTL and deterministic-test review debt is
clear through hardware-round-two tip `d17a1bc773c12427ea980a102f5e4054ae7fa827`**.
The later round-three delta is separately review-clear through feature tip
`5275879ad96d87ede7fc484e890dd1f0804ff78e` and integration merge
`275a9a42a3569e460c83de10cfc9781fc00fbb9a`.

The broader row remains useful only as an external-evidence ledger. It must not
imply that simulation or review established physical sprite bandwidth, the
complete HPS-to-SNA producer topology, exact T80/top-level timing, or title-level
hardware behavior.

## Reviewed chain

| Stage | Range or tip | Recorded independent result |
|---|---|---|
| Initial P10a-P10f/P10h implementation | `3233837e4db12291d6e301af97bdc08e8ba46266..796ae04d0bae1e101c9501528d2bd90cc2704d35`, inspected at integration tip `d621230fcc8099727b6ed675c7f0ef95f5d54835` | Claude Opus 5 xhigh: not clear; defects and test gaps recorded in `p10-independent-review.md` |
| P10 remediation | base `d621230fcc8099727b6ed675c7f0ef95f5d54835`, feature tip `1d1795b4b04b2db9b31447d09536aa2d2de2b732`, merge `c985b6395b58b915e53ca0ca53a8fe413fa7a1e7` | Claude Opus 5 high: prior blockers mostly closed; one SNA tail-test integrity blocker and FIFO-headroom risk remained |
| Tail/FIFO and hardware-round-two remediation | from `f3b96b94140b2791f642a9d7844b97d415b4c95e` through status tip `3ce8268e4db3855f2d644fef9c158e0ef99790ba` | GPT-5.6 Sol high: clear after focused fixture reproduction |
| Inactive-DMA execute remediation | through `d17a1bc773c12427ea980a102f5e4054ae7fa827`, merged as `4d26cadde1bff93b8f63ca61bacc538da0588e26` | Claude Opus 5 high: clear after `d13` pinned channel identity and order; no unreviewed round-two RTL/test delta remained |
| Hardware round three | feature tip `5275879ad96d87ede7fc484e890dd1f0804ff78e`, merged as `275a9a42a3569e460c83de10cfc9781fc00fbb9a` | Native Sol plus guarded Claude: clear; CRTC3 R8=1 and DMA/PPI/PSG concurrency findings were remediated behind focused discriminators |

No P10-scoped implementation file changed between `275a9a4` and the inspected
base `d74cd242`: `Amstrad.sv`, `rtl/Amstrad_motherboard.v`, `rtl/i8255.v`,
`rtl/plus/`, `sim/plus/`, `files.qip`, and the build workflows have an empty
scoped diff across that interval. The intervening `4d3a7b9` commit adds the P10j
resource/timing milestone to the roadmap; the other intervening work is the
separately cleared ACCC bilingual stream.

## Remaining validation boundaries

- Physical sprite-pixel bandwidth, access-blanking duration, coordinate behavior,
  and title causality remain hardware questions. The deterministic sprite tests
  prove the implemented staging and collision model, not undocumented ASIC internals.
- The local SNA seam proves the parser, ASIC-register, and MMU contract under the
  checked two-byte producer-tail bound. It does not co-elaborate the complete
  `hps_io` to `Amstrad.sv` producer and apply topology.
- The production CPU is VHDL T80/T80pa. Local Plus fixtures use TV80 under a
  T80pa-shaped wrapper or a PHI-aligned substitute, so interrupt-acknowledge and
  exact bus timing remain synthesis/hardware boundaries.
- P10j still owes an exact full-effort fit with non-negative constrained setup and
  hold slack, zero TNS, fail-closed CI enforcement, entity/resource deltas, and a
  named RBF SHA-256. That evidence is not hardware confirmation.
- Arnold 5, Plotting, Pang, the CRTC3 demo, Switchblade, Burnin' Rubber, Copter 271,
  BASIC/System disk access, and reset/reload recovery remain named hardware retests.

## Proposed shared-row replacement

The Accuracy/shared-doc owner can replace the historical open row with:

> **Plus P10a-P10f/P10h local implementation review — CLEARED through
> `d17a1bc` (round two); round three separately CLEARED through feature tip
> `5275879`, integration `275a9a4`, 2026-08-30.** The initial and remediation
> reviews found real RTL and test-integrity defects; the final Sol and guarded
> Claude passes accepted the focused remediations, including SNA tail/headroom,
> all-16 sprite cadence, DMA inactive-slot ordering, CRTC3 R8=1 timing, and
> production DMA/PPI/PSG concurrency. No unreviewed local P10 RTL/test delta
> remains. Physical sprite bandwidth/access blanking, the full HPS-to-SNA and
> exact T80/top-level boundaries, exact full-effort timing/RBF evidence, and
> title-level hardware retests remain validation residuals, not independent-review
> debt. See `docs/plus/p10-review-debt-status-2026-08-31.md` and the round-two/
> round-three hardware records.

## Cross-check boundary

A bounded Gemini Analyze pass independently traced the same commit/review chain
at `d74cd242` and agreed that the remaining items are external validation rather
than unreviewed local RTL/test debt. The parent session verified the cited commit
ancestry, scoped post-`275a9a4` diff, and exact wording against the checked-in
review records before writing this note.
