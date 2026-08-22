# Independent review: `accc-review-and-fixes`

Reviewed 2026-08-23 against the fork point specified in the review brief:

```text
codex/exploratory-gx4000-plus-plan..accc-review-and-fixes
a3b4ca096edc86b95764cb408fa5369b5a1dc9d5..2d4f880eefe1d992f31487bdfa8f4e3a0cd85271
```

The named tip includes the per-type split through merge commit `2d4f880` even though the
reviewer's guide describes the base and split branches separately. Branches
`accuracy/a3-f6-stage1` and `plus/p0-parser-wiring` were not reviewed.

No reviewed source, documentation, branch, or commit was changed during the review. This
report file was added only after the user explicitly requested a durable copy of the report.

## Overall verdict

**Blocking issues found.** The CRTC split itself appears behaviour-preserving, but the branch
should not be treated as settled while the GA40010 co-simulation dependency list is broken and
the canonical handoff documentation materially contradicts the branch tip.

## Verdict by area

| Area | Verdict | Summary |
|---|---|---|
| Documentation | **Blocking issues found** | Primary status/roadmap documents describe an obsolete pre-split state and retain the known-wrong F6 integration premise. |
| Soak harness | **Questions** | Deterministic and reproducible, but narrower than the stated "bit-identical" claim. The separate differential comparator substantially strengthens the split evidence. |
| Per-type split | **Sound** | All five high-risk relocations/mux seams match the unsplit `418aa68` RTL under static review, the requested gates, the golden hash, and a reproduced 45,498,863-sample lockstep comparison. |
| Wrapper rename | **Blocking issues found** | Production synthesis is green, but the renamed GA40010 co-simulation dependency list omits both new engines. |

## Issues

### 1. Blocking - GA40010 co-simulation no longer elaborates

`rtl/GA40010/Makefile:8` replaced `../UM6845R.v` with `../CRTC.v` but does not include
`../crtc_type0_engine.v` or `../crtc_type1_engine.v`, even though the wrapper instantiates
them at `rtl/CRTC.v:207` and `rtl/CRTC.v:236`.

Direct Verilator elaboration using the checked-in `HDL_FILES` list fails with:

```text
Cannot find file containing module: 'crtc_type0_engine'
Cannot find file containing module: 'crtc_type1_engine'
```

This is material to the documentation as well as the side target:
`docs/accuracy/f6-decision-gate.md:61-68` presents this CRTC+GA target as the cheapest Stage-2
measurement route.

Evidence that would resolve it: a clean elaboration and execution of the documented GA40010
CRTC/video target using only its checked-in dependency manifest.

### 2. Blocking - canonical handoff documents contradict the reviewed tip

The branch tip includes the split, rename, F9 vectors, 87-test gate, and current-tip CI
synthesis, but:

- `docs/current-status.md:4-6` says `accc-review-and-fixes` does not change RTL.
- `docs/current-status.md:136-138` reports 85 CRTC tests rather than 87.
- `docs/current-status.md:140-145` still describes F9 as living on an unmerged stream branch
  and repeats the superseded character-granularity F6 premise.
- `docs/current-status.md:199-203` says earlier debt repayment remains in progress.
- `docs/current-status.md:228-231` schedules F9 as future work.
- `docs/implementation-roadmap.md:27-29` and `:301-313` repeat the obsolete count and queue.

These are not harmless historical notes: `current-status.md` declares itself the fresh-session
handoff. The resulting instructions can direct the next session onto completed or rejected
work.

Evidence that would resolve it: a tip-grounded handoff audit in which branch contents, test
count, CI build, review state, and next-work order all agree with `2d4f880`.

### 3. Blocking documentation defect - known-wrong F6 integration premise remains authoritative

`docs/accuracy/audit-findings.md:225-231` still names the deleted `rtl/UM6845R.v` and states
that the GA consumes DE at 1 µs granularity, making a full-character approximation the
achievable result. That directly conflicts with the later code-aware conclusion in
`docs/accuracy/f6-decision-gate.md:53-73`, which calls the type-0 pin behaviour exact and
assigns seam width to downstream GA/glue measurement.

This is precisely the integration-assumption failure class that `AGENTS.md` says must not
survive a prose sweep.

Evidence that would resolve it: one internally consistent F6 boundary description backed by
the actual CRTC output path, GA sampling logic, and an operational CRTC+GA test target.

### 4. Non-blocking - the committed soak is useful but does not establish bit identity as stated

The pre-split and current hashes are genuinely reproducible, but coverage is narrower than
`sim/README.md:46-56` suggests:

- `sim/sim_main.cpp:183-205` reads `DO` and restores the idle bus before any CLKEN sample, so
  the randomized reads at `sim/sim_main.cpp:3653-3659` do not put their returned values into
  the hash.
- Sampling occurs only after CLKEN edges (`sim/sim_main.cpp:455-465`), so transient bus-phase
  divergences can escape.
- The field set at `sim/sim_main.cpp:233-267` omits the relocated partial-VSYNC holdoff latch
  and the type-1 status flops.
- `sim/README.md:53-56` describes excluding documented behaviour deltas from a hash window,
  but the implementation has no window/exclusion mechanism.
- The mixing operation is FNV-like whole-value mixing, not conventional byte-wise FNV-1a.

The golden match therefore proves equality for this fixed stimulus and sampled projection,
not general RTL bit identity. That distinction matters because the documented development bug
in the omitted holdoff path escaped the soak.

The supplementary comparator on `docs/split-differential-evidence` at `c68459b` closes much
of this evidence gap for the split itself: it samples the holdoff latch, type-1 status, shared
capture flops, sync/display state, and both video-pointer registers in addition to pins and
counters, and compares old/new snapshots after every CLKEN edge. It remains complementary
evidence rather than changing what the committed soak checks.

Evidence that would fully resolve the stronger general claim: a comparison covering every
relevant clock phase and all externally visible combinational states, or a claim explicitly
bounded to the checked stimulus, fields, and sample phase.

### 5. Non-blocking - rename/document sweep is incomplete

Fresh-session and current-path references still name the deleted wrapper:

- `CLAUDE.md:38-42` and `CLAUDE.md:68-72`, despite `CLAUDE.md` being authoritative.
- `sim/README.md:6-9`.
- `docs/accuracy/testbench-spec.md:24-27`.
- `docs/accuracy/f6-decision-gate.md:63-68`.

Historical references are legitimate; these instances describe current paths or commands.

Evidence that would resolve it: every current-path reference resolves to an existing source or
is explicitly labelled historical.

### 6. Non-blocking - smaller review-guide/document inaccuracies

- `docs/accuracy/type-split-review-guide.md:21` calls `aea80b5` an "F9 RTL fix". The commit is
  test/docs-only and explicitly records that no RTL changed.
- `docs/accuracy/type-split-review-guide.md:94-96` says `e0_hcc2_adj_keep` carries its own type
  gate. It does not: `rtl/crtc_type0_engine.v:246` is the raw effective-R5 reduction. The
  wrapper behaviour remains correct because it consumes this term under both types.
- `docs/accuracy/findings-review.md:93-97` refers to Q15/Q16, but
  `docs/accuracy/accc-author-questions.md` numbers only Q1-Q14; the two apparent intended
  questions are unnumbered at lines 67-73.

Evidence that would resolve these: commit descriptions, signal descriptions, and numbered
cross-references agree with the underlying history/code and resolve to existing entries.

## Five "read hardest" results

### 1. Wrapper mux seams - confirmed

The type-selected line/row/frame, reload/save, sync, display-skew, and write-fire legs preserve
the original ternaries. Representative muxes are at `rtl/CRTC.v:258-270`, `:351`, and
`:381-389`.

### 2. Type-0 latch cluster - confirmed

The eight registers, write/line-boundary priority, and outer
`~nRESET | SNA_LOAD | CRTC_TYPE` clear at `rtl/crtc_type0_engine.v:251-306` match the unsplit
core at `418aa68`.

### 3. Partial-VSYNC holdoff - confirmed

Count-tick clear, equal-R7-write set/clear, and final type/SNA clear remain in original program
order at `rtl/crtc_type0_engine.v:324-345`. A false R7 comparison leaves the latch untouched.
The reproduced differential comparator also samples this latch directly.

### 4. `frame_adj_r` hcc==2 behaviour - substantive claim confirmed; guide wording refuted

`rtl/CRTC.v:295` consumes the term under both CRTC types, matching the unsplit core. The term
itself is not type-gated as the guide says; `rtl/crtc_type0_engine.v:246` exports the raw
effective-R5 reduction.

### 5. hcc==0 capture semantics - confirmed

`rtl/CRTC.v:287-292` updates `line_last_r`, `row_last_r`, and `frame_adj_r` under both types
using the selected engine values. No accidental type-0-only gating was introduced. The
differential comparator samples all three flops.

## Test/assertion integrity

The only new ACCC-directed assertions in this diff are `t12a`/`t12b`. Their geometry and
expected results were independently derived from ACCC v1.10 p.82:

- an R9 write at exact C0==R0 produces C4=39, C9=8;
- the companion write in C0 in `[2,R0-1]` produces C4=38, C9=8.

Those expectations are stated directly by the source rather than read from the simulator. No
new directed assertion was found that encodes the current RTL instead of its cited ACCC rule.
The randomized soak has no ACCC-rule assertions and is correctly treated as differential
regression evidence, subject to the scope caveats above.

## Mechanical and source verification

- `make -C sim`: **87 passed, 0 xfailed, 0 xpassed, 0 failed**; all Plus suites also passed.
- `make -C sim lint`: exit 0, warnings only.
- Current-tip soak: seed `0xaccc5eed20260822`, 2,845,088 CLKEN samples, hash
  **`0x5b5004ff70148443`**.
- Exact unsplit commit `418aa68` was independently extracted and built in a temporary
  directory; it produced the same seed, sample count, and hash.
- Current tip `2d4f880` has successful simulation and Quartus synthesis in
  [GitHub Actions run 32603704090](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/32603704090).
- The local ACCC PDF has the planned SHA-256
  `1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560` and 295 pages.
  Text/visual spot-checks confirmed corrections B1, B4/B5, and B10.

### Supplementary differential evidence verified after the initial report

Provenance supplied additive-only branch `docs/split-differential-evidence`, commit `c68459b`;
the reviewed branch remained at `2d4f880`.

The reviewer inspected:

- `tools/split-differential/run.sh`;
- `tools/split-differential/diff_main.cpp`;
- `tools/split-differential/README.md`;
- `docs/accuracy/evidence/split-differential-run-2026-08-23.log`.

The tool extracts `418aa68:rtl/UM6845R.v`, renames the reference module, builds the reference
and current split models independently, drives them in lockstep, and `memcmp`s a `Sample`
snapshot after every CLKEN edge. The sampled state includes the output pins MA, RA, DE, HSYNC,
VSYNC, CURSOR, FIELD, and DO; shared counters and capture state; the eight arbitration latches
and partial-VSYNC holdoff; type-1 status; VSYNC/display state; and both video-pointer registers.

The reviewer independently ran the exact comparator from a temporary extraction of `c68459b`
against the unchanged `2d4f880` checkout. It reproduced:

```text
type 0 clean through 150000 events (22627500 samples)
type 1 clean through 150000 events (45498863 samples)
no divergence
```

This resolves the initial provenance question and is substantially stronger evidence for the
split than the committed soak alone. Its documented limitations remain honest: its schedule
mirrors but is not byte-identical to the soak, reads do not drive the bus, comparison occurs at
CLKEN edges, and unsampled combinational internals could theoretically differ.

## Could not verify

- Real-hardware equivalence or SHAKER behaviour: no hardware session was available. CI
  synthesis proves buildability, not hardware accuracy.
- The complete 295-page faithfulness review was not repeated. Per the brief, ACCC-verified
  Rule sections were trusted and selected corrected claims were independently spot-checked.
- ACCC p.242 says R12/R13 updates are "immediate" but does not, from prose alone, settle the
  known F11h intra-character residual.
- The differential comparator does not prove equality of unsampled combinational internals or
  behaviour between its CLKEN snapshots; it proves no divergence in its broad sampled state
  over the reproduced deterministic trajectory.

## Final disposition

The split RTL is accepted as sound within the reviewed evidence. The whole branch verdict
remains **blocking issues found** because the renamed GA40010 co-simulation route is broken and
the canonical handoff/F6 documentation is not reliable at the branch tip.
