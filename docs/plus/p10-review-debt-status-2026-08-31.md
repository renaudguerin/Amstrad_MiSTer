# Plus P10 review-debt status — 2026-08-31

**Classification base:** `plus/p10j-resource-timing` at
`d74cd2429f1a0fef0a3b00cf45e900d6d02c518c`

**P10j accepted code tip:**
`c047a7dc7bf348be6bdd0637e1e6e2ee154412f1`

**Authoritative P10j fit:** local-VM full-effort run `33392854459`, job
`99490354763`, artifact `9758895514` (`Amstrad-local-build-6-1-full`).

The inherited-debt classification used read-only history, reports, RTL, fixtures,
and recorded gates at the base above. P10j then reran focused and full local gates
and exact Quartus 17.0.2 synthesis. No hardware test was rerun.

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

## P10j independent-review record and limitation

One Gemini 3.7 Flash guarded-wrapper lane, at its high reasoning setting, was
used throughout; no Claude, Luna, or duplicate reviewer repeated the work. It
reviewed the exact initial range
`d74cd2429f1a0fef0a3b00cf45e900d6d02c518c..36963dbefb3faa07e9bc0ca573e846fee9f4d477`
read-only, then continued incrementally through `037e5fe`, `5f47121`, and
`f16020e`. It returned PASS with no code finding at each point. This is recorded
as weaker-model review evidence, not as Opus/Sol-equivalent, fitter, or hardware
evidence.

The reviewer cleared the logical even/odd mapping, CPU and direct-SNA accesses,
one-clock video fetch, access blanking, mixed-port old-data behavior,
reset-preserved storage, `files.qip` integration, synthesis-path classification,
and fail-closed timing checks. Exact Quartus remained authoritative and exposed
four issues that source review and local vendor-stub lint did not prove:

1. `36963db` and `037e5fe` still retained a 16-Kbit soft-register mirror.
2. `5f47121` removed that mirror but inferred two copies per bank: four M10Ks.
3. `f16020e` connected `clock1` while its port-B registers selected `CLOCK0`.
4. `db60f8d` used an unsupported same-port `OLD_DATA` value for Cyclone V
   `BIDIR_DUAL_PORT` mode.

After the permission layer rejected export of the private-repository snapshot,
the existing Gemini lane could not review the final mechanical corrections
`db60f8d` and `c047a7d`; the task did not retry or substitute a second reviewer.
That limitation was later resolved at integrated SHA `bf1e785` by one guarded
Claude Opus 5 high review. The rebased correction commits are `1248d06` and
`cd56d66`; their in-scope blobs are identical to the accepted `c047a7d` feature
tip. Claude found no defect in either correction and independently cleared the
primitive legality, CLOCK1 grouping, even/odd and packed-pixel mapping, one-edge
latency, reset-preserved storage, `files.qip`, and exact-fit provenance.

The exact-tip verdict remains technically NOT CLEAR for three low contract and
coverage notes, not a known production defect:

1. `plus_sprite_ram.v`'s behavioral same-port read/write path returns old data,
   while the accepted M10K uses NEW_DATA. This collision is unreachable in the
   current integration because SNA drain holds the CPU in reset, but that
   invariant is not documented at the RAM boundary.
2. The constant-zero `altsyncram_lint_stub.v` proves port/parameter elaboration,
   not primitive data semantics; the behavioral collision assertions therefore
   remain distinct from exact-synthesis evidence.
3. `asic_regs.v` selects `host_addr=eff_addr`, so hypothetical concurrent SNA
   write and CPU read would redirect the CPU read. Current CPU-reset sequencing
   makes that case unreachable, but the local invariant is undocumented.

Quartus run `33392854459` remains the feature-tip primitive/fit evidence. Final
combined integration run `33396320914` separately proves the whole `bf1e785`
build and packaged RBF.

## P10j exact gate and fit evidence

Exact code tip `c047a7dc7bf348be6bdd0637e1e6e2ee154412f1` passed:

- focused `plus-sprite-ram`, production-branch lint, and P4 sprite/register gates;
- full `make -C sim` and `make -C sim lint`;
- `make -C sim soak SOAK_EXPECT=0x32d468e81eac63c9` with the expected hash; and
- `scripts/ci/check-quartus-timing.sh` against the downloaded exact STA summary.

The successful full-effort Quartus 17.0.2 fit reports:

| Evidence | Exact result |
|---|---|
| Whole core | 22,057 ALMs (53%), 26,101 registers, 701,596 block-memory bits, 102 RAM blocks |
| `asic_regs` | 1,019.7 ALMs needed; 1,144.7 ALMs in final placement; 1,224 combinational ALUTs; 976 dedicated registers; 16,384 bits; 2 M10Ks |
| `plus_sprite_ram` child | 10.2 ALMs needed; 12.8 in final placement; 20 combinational ALUTs; 3 dedicated registers; 16,384 bits; 2 M10Ks |
| Sprite RAM primitives | exactly one 2,048x4 true-dual-port M10K for each of `even_bank` and `odd_bank`; no soft pixel-register mirror and no duplicate bank copies |
| `asic_sprites` | 4,194.8 ALMs needed; 4,354.5 in final placement; 6,821 combinational ALUTs; 3,362 dedicated registers |
| Timing | minimum setup slack +0.323 ns; minimum hold slack +0.251 ns; setup and hold TNS 0.000 |
| RBF | `Amstrad_20260831_c047a7d.rbf`; SHA-256 `b0ccf327bcc5054466ef19828af50b21fbce9b3079a6d92655ba582968030945` |

Against exact base build `d74cd242` (run `33378020163`), whole-core use moved
from 37,255 to 22,057 ALMs (-15,198), from 42,370 to 26,101 registers
(-16,269), and from 100 to 102 RAM blocks while adding exactly 16,384 block
memory bits. The base `asic_regs` node used 16,185.7 ALMs needed, 17,146.6 in
final placement, 17,365 dedicated registers, and no memory. The accepted P10j
fit reduces those values to the `asic_regs` row above. Base `asic_sprites` was
4,214.1 ALMs needed and 4,251.8 in final placement; the accepted node is shown
above.

Rejected intermediate fits remain evidence, not accepted deliverables:

| Code tip / run | Disposition |
|---|---|
| `36963db` / `33383382343` | Timing and RBF green, but 16-Kbit soft-register mirror remained |
| `037e5fe` / `33386337051` | Reduced logic, but the same soft mirror remained |
| `5f47121` / `33388759822` | Soft mirror removed, but four M10Ks / 32,768 bits duplicated the two banks |
| `f16020e` / `33391580700` | Rejected at elaboration: port-B registers selected `CLOCK0` while `clock1` was connected |
| `db60f8d` / `33392411818` | CLOCK1 contract elaborated; rejected because same-port `OLD_DATA` was unsupported in this primitive mode |
| `c047a7d` / `33392854459` | Accepted exact fit: two M10Ks / 16,384 bits, timing closed, named RBF packaged |

## Reviewed chain

| Stage | Range or tip | Recorded independent result |
|---|---|---|
| Initial P10a-P10f/P10h implementation | `3233837e4db12291d6e301af97bdc08e8ba46266..796ae04d0bae1e101c9501528d2bd90cc2704d35`, inspected at integration tip `d621230fcc8099727b6ed675c7f0ef95f5d54835` | Claude Opus 5 xhigh: not clear; defects and test gaps recorded in `p10-independent-review.md` |
| P10 remediation | base `d621230fcc8099727b6ed675c7f0ef95f5d54835`, feature tip `1d1795b4b04b2db9b31447d09536aa2d2de2b732`, merge `c985b6395b58b915e53ca0ca53a8fe413fa7a1e7` | Claude Opus 5 high: prior blockers mostly closed; one SNA tail-test integrity blocker and FIFO-headroom risk remained |
| Tail/FIFO and hardware-round-two remediation | from `f3b96b94140b2791f642a9d7844b97d415b4c95e` through status tip `3ce8268e4db3855f2d644fef9c158e0ef99790ba` | GPT-5.6 Sol high: clear after focused fixture reproduction |
| Inactive-DMA execute remediation | through `d17a1bc773c12427ea980a102f5e4054ae7fa827`, merged as `4d26cadde1bff93b8f63ca61bacc538da0588e26` | Claude Opus 5 high: clear after `d13` pinned channel identity and order; no unreviewed round-two RTL/test delta remained |
| Hardware round three | feature tip `5275879ad96d87ede7fc484e890dd1f0804ff78e`, merged as `275a9a42a3569e460c83de10cfc9781fc00fbb9a` | Native Sol plus guarded Claude: clear; CRTC3 R8=1 and DMA/PPI/PSG concurrency findings were remediated behind focused discriminators |
| P10j block-RAM and timing closure | base `d74cd2429f1a0fef0a3b00cf45e900d6d02c518c`, accepted code tip `c047a7dc7bf348be6bdd0637e1e6e2ee154412f1`, integrated as `bf1e78538afda195476276ecce89f5fbc79df4a0` | Gemini 3.7 Flash high reviewed the architecture/CI chain through `f16020e`; Claude Opus 5 high then found no defect in the final CLOCK1 and collision-mode corrections, retaining only three low contract/coverage notes |

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
- P10j exact fit, resource shape, timing closure, fail-closed CI enforcement, and
  named RBF evidence are complete at `c047a7d`. This is synthesis evidence, not
  hardware confirmation. Claude's integrated exact-tip review found the final
  two primitive corrections clean; three low contract/coverage notes remain.
- Arnold 5, Plotting, Pang, the CRTC3 demo, Switchblade, Burnin' Rubber, Copter 271,
  BASIC/System disk access, and reset/reload recovery remain named hardware retests.

## Proposed shared-row replacement

The Accuracy/shared-doc owner can replace the historical open row with:

> **Plus P10a-P10f/P10h local implementation review — CLEARED through
> `d17a1bc` (round two); round three separately CLEARED through feature tip
> `5275879`, integration `275a9a4`, 2026-08-30; P10j block-RAM/timing evidence
> complete at code tip `c047a7d`, full-effort run `33392854459`.** The initial and remediation
> reviews found real RTL and test-integrity defects; the final Sol and guarded
> Claude passes accepted the focused remediations, including SNA tail/headroom,
> all-16 sprite cadence, DMA inactive-slot ordering, CRTC3 R8=1 timing, and
> production DMA/PPI/PSG concurrency. P10j moved sprite pixels from soft
> registers to exactly two M10Ks, closed setup/hold timing with zero TNS, and
> produced the named `c047a7d` RBF. One guarded Gemini lane reviewed the
> non-trivial P10j architecture/CI work through `f16020e`; a guarded Claude Opus
> 5 high exact-tip pass found no defect in the final two mechanical primitive
> corrections and retained three low contract/coverage notes. Physical sprite bandwidth/access blanking,
> the full HPS-to-SNA and exact T80/top-level boundaries, and title-level hardware
> retests remain validation residuals. See
> `docs/plus/p10-review-debt-status-2026-08-31.md` and the round-two/round-three
> hardware records.

## Cross-check boundary

A bounded Gemini Analyze pass independently traced the inherited commit/review
chain at `d74cd242` and agreed that its remaining items were external validation
rather than unreviewed local RTL/test debt. The sole P10j Gemini lane then
reviewed the non-trivial implementation incrementally as recorded above. The
parent session verified commit ancestry, scoped diffs, local gates, all rejected
intermediate fits, and the downloaded `c047a7d` fit/STA/RBF artifact before
closing this stream-local evidence note.
