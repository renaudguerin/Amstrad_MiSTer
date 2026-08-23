# Reviewer's guide — `accc-review-and-fixes` + `accuracy/crtc-type-split`

Written 2026-08-23 for the whole-branch review pass required by the 2026-08-22 locked
decision in the session plan (`docs/plans/2026-08-22-accc-review-plan.md`): work on these
branches was authored by a single model (ox-alpha) with no cross-provider review available
on this harness, so **the entire diff is reviewed as one unit before any of it is treated
as settled or upstreamed**. Per that decision, no per-commit rows were added to
`docs/review-debt.md`; this guide is what a reviewer should read alongside the diffs.

You do not need to be fluent in Verilog to gate most of this: the strongest claims are
backed by mechanical checks you can run (commands below). The Verilog-specific reading list
at the end is for whoever does the line-level pass.

## Branch 1 — `accc-review-and-fixes` (base branch)

Scope: documentation reconciliation + verification tooling for the classic split. The
current checkout also carries later F6 Stage 1 and Plus-stream work; those are separate
behaviour/evidence streams and are not covered by the split-equivalence claim below.

| Commit | What | Why |
|---|---|---|
| P0–P4 commits | ACCC v1.10 faithfulness review deliverables, doc corrections, status-vs-code audit, review-debt repayment record (A1–A5 action items) | Recorded in `docs/accuracy/findings-review.md` and `docs/review-debt.md`; docs-only until the harness commit |
| `d5cab8f` | Merge of `accuracy/f9-t12-closure` (`aea80b5`: t12a/t12b vectors + docs, no RTL change) into base | The type-split branch had to inherit the full 87-vector suite and the exact core state the soak golden hash was minted against |
| `418aa68` | Soak-diff harness: `make -C sim soak` | Turns "bit-identical refactor" from a hope into a checkable claim; permanent tooling (F7/F10 will reuse it). Fixed seed; FNV-1a hash over all pins + hcc/line/row/c5/in_adj/type-0 latches sampled every CLKEN |

**Golden hash: `0x5b5004ff70148443`** (seed `0xaccc5eed20260822`). It is minted from the
unsplit core at `418aa68` and is the contract the split branch must reproduce. Re-minting is
only needed if the seed, sampled field set/order, or event schedule changes.
(2026-08-23 note: the F6 Stage 1 behaviour change on `accuracy/a3-f6-stage1` later re-minted
the golden value to `0x326ea81358e7d88f`; `5b5004ff70148443` remains the correct expectation
for the commits of these two branches.)
(Later the same day, review issue 4 remediation: the sampled field set gained the type-0
partial-VSYNC holdoff latch and the two type-1 status flops — no RTL change — re-minting
the current golden value to `0xf5f8ae01ffdf928d`; both earlier values remain the correct
expectations for their own commits.)
(F7 then intentionally changed type-1 behaviour under the unchanged fixed stimulus: random
R5 traffic can arm RFD. The current hash is `0xae27f2c3c758ed87`; directed `t13a`, not the
soak, proves the never-triggered path remains unarmed and unchanged.)
(A1 then removed the spurious type-1 VSYNC at the adjustment-ending final-row+1
comparison, re-minting the unchanged stimulus/projection to `0x6439f9805b20acaa`.)

## Branch 2 — `accuracy/crtc-type-split`

| Commit | What | Why |
|---|---|---|
| `27efc2d` | Split: wrapper + two per-type engines; files.qip same commit; prose sweep folded in | Implements approved option S2 from `docs/accuracy/crtc-per-type-separation.md`: F7 RFD is type-1-only and F10 interlace parity differs structurally per type; landing them into separate engine files stops the shared-state-machine tangle that produced review finding A1 |
| `4c28a89` | Plan checklists + resume point | Session bookkeeping |
| `63f4c01` | Rename wrapper `UM6845R` → `CRTC`, file `rtl/CRTC.v` | UM6845R is the *type-1* part number; naming the two-variant entry module after one variant misdescribed it (user-raised). Mechanical sweep; motherboard instance label was already `CRTC` |
| guide commit | This file + `docs/review-debt.md` branch-review section | Review logistics |

### The structural decision, and its rationale

The engines are **not** independently stateful. `rtl/CRTC.v` keeps singular state (register
file, hcc/line/row/c5/in_adj counters, VSYNC/vde machinery); each engine module holds every
*type-specific rule expression* plus only the flops provably private to its type:

- type-0-private: arbitration latch cluster (8 regs), partial-VSYNC holdoff latch;
- type-1-private: status bit 5 + R6-border condition.

Rationale: `CRTC_TYPE` is a live input, and four required vectors (t02j, t06d, t09f, t16l)
pin the round-trip contract that switching type mid-flight continues counting from the exact
state the previous type left behind. A physical CRTC has one counter bank regardless of which
variant sits in the socket; duplicated engine state would break those vectors or need a
fragile state-handoff bus. Consequence: engines compute next-state/predicate terms, the
wrapper clocks shared counters from muxed engine outputs, and every original expression moved
verbatim (wrapper ternaries became explicit `CRTC_TYPE ? e1_x : e0_x` muxes).

Register/bus decode stayed **shared** in the wrapper (explicitly allowed by the separation
doc): the decode is identical for both types, so duplicating it would double maintenance for
zero separation benefit.

### Evidence stack (run these)

```sh
make -C sim                                  # 98 passed / 0 xfail / 0 xpass / 0 failed
make -C sim lint                             # no errors (pre-existing warnings only)
make -C sim soak SOAK_EXPECT=0x6439f9805b20acaa # current sampled-field contract
```

The current checkout has 98 classic passes and uses `0x6439f9805b20acaa`, re-minted
for A1 after the F7 RFD, F6 Stage 1, and sampled-field re-mints. The historical
`0x5b5004ff70148443` value remains the correct expectation for the unsplit-core
comparison commit recorded above; the split-equivalence claim is bounded to the
sampled fields, stimulus, and phase documented in `sim/README.md`.

Plus, once per push: GitHub Actions "Build core" workflow green on
`accuracy/crtc-type-split` (Verilator gate first, then pinned Quartus 17.0.2 synthesis —
required because `files.qip` changed).

Stronger than any single run: during development, a lockstep differential harness
(pre-split reference core vs split core, identical stimulus, compared after **every** CLKEN
edge) ran ~45.5M samples across both types with zero divergence after the fix below. It is
now preserved and reproducible: `tools/split-differential/run.sh` (branch
`docs/split-differential-evidence`, `c68459b`), which extracts the reference from git
history automatically. An independently rerun capture against this branch's tip is at
`docs/accuracy/evidence/split-differential-run-2026-08-23.log`; see also the independent
review, `docs/accuracy/accc-review-and-fixes-independent-review.md`, which reproduced it.

The one real bug it caught (fixed before any commit): the relocated type-0 partial-VSYNC
holdoff latch initially applied its *set* path on R7 writes whose comparison was false
(`row != DI`), whereas the original only touches that latch inside the equal-comparison
branch. Exactly the class of subtle sequencing the split was expected to risk — and exactly
what the differential method exists to catch.

### For the line-level reviewer: read hardest, in order

1. **Wrapper mux seams** (`rtl/CRTC.v`, search `CRTC_TYPE ? e1_`): each must select the same
   leg the original inline ternary did.
2. **Type-0 latch cluster** (`crtc_type0_engine.v`, block under "Register writes are clocked
   at the 16 MHz bus rate"): moved verbatim including its outer clear
   (`~nRESET | SNA_LOAD | CRTC_TYPE`) — safe because the latches are held at 0 while type 1
   is selected, matching pre-split values at any switch instant.
3. **Holdoff latch** (`type0_vsync_wait_line_start`): three update sites replicated with
   original program order (count-tick clear → R7-write set/clear → unconditional
   type/SNA clear). See bug note above.
4. **`frame_adj_r`'s hcc==2 keep term**: consumed unconditionally under BOTH types.
   The exported term is the raw effective-R5 reduction — it carries no type gate of its
   own; correctness comes from the wrapper consuming it regardless of type, preserving the
   quirk that the flop updates even while type 1 runs. (Corrected per the independent
   review, which refuted this guide's original wording.)
5. **hcc==0 capture semantics**: `line_last_r`/`row_last_r`/`frame_adj_r` must keep updating
   under *both* types with type-muxed values — they are read by type-0 rules after a live
   switch, so gating them would change round-trip behaviour.

### Current bounded status

- A1 is closed by `t08m` plus the corrected `t08g` oracle; A2 remains open in
  `docs/review-debt.md` (§11.2.4 caveat pair). A3's t20 companion vector is complete: `t20i` covers live-entry
  R0=0 VMA reload and freeze.
- F11h residual: intra-character immediacy of R12/R13 on type 1 not modeled; p.242 re-read
  pending.
- F6 Stage 1 is accepted only as the documented full-character DE approximation; exact
  ACCC 0.5 µs pin timing remains F13 hardware-blocked. F7's R5-at-R0 route is implemented
  with t13a-t13d; F10 remains the next separately gated classic work.
- Plus P0 wiring and the P1 CRTC3 foundation are implemented in their separate stream;
  their current review-debt rows await independent confirmation and do not extend the
  classic split-differential claim.
- Hardware results always outrank simulation: none of the above evidence replaces a SHAKER
  session.

## Verification-ownership footnote

Per the AGENTS.md convention: Rule sections in `audit-findings.md` are ACCC-verified (P1)
and trusted; integration claims were re-checked against current sources during the split
prose sweep (F7 absence, F10 minimal-IVM, F11a–i anchors — stamped in place). Anything found
during later work becomes a recorded finding or plan addendum, never a silent fix.
