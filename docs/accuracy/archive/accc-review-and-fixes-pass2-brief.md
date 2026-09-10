# Independent review brief — `accc-review-and-fixes`, pass 2

You are the independent pass-2 reviewer for the Amstrad MiSTer fork. Work from the
authoritative checkout and review the exact delta:

```sh
git diff 2d4f880..accc-review-and-fixes
```

READ-ONLY: do not edit, create, delete, stage, commit, amend, rebase, merge, push, or switch
branches. Do not fix findings. You may run read-only inspection commands and local verification
gates that create only ignored build artifacts. Leave the named branch and all reviewed files
unchanged; return the review as your response for the coordinator to preserve separately.

## Read first

1. `AGENTS.md`
2. `CLAUDE.md`
3. `docs/plans/2026-08-22-accc-review-plan.md`
4. `docs/review-debt.md`
5. `docs/accuracy/archive/accc-review-and-fixes-independent-review.md`
6. `docs/accuracy/type-split-review-guide.md`
7. `docs/accuracy/audit-findings.md`
8. `docs/accuracy/f6-decision-gate.md`
9. `docs/current-status.md`
10. `docs/implementation-roadmap.md`
11. `docs/plus/architecture.md`

## Scope

The delta `2d4f880..accc-review-and-fixes` contains the preserved pass-1 report/guide
correction (`6cfd4dd`) and the six post-review closure commits on
`accc-review-and-fixes`: `90f0cda` repairs the GA40010 co-simulation manifest, `72d7cf4`
tip-grounds the handoff/roadmap, `4140ebb` corrects the stale F6 integration premise,
`d66ec23` bounds the soak claim and corrects its mixing/re-mint documentation, `c7558ae`
finishes the rename and question-number sweep, and `6554d8f` records the remediation state.
It then contains these merged streams: `accuracy/a3-f6-stage1` (A3 companion vector plus
the classic F6 Stage 1 type-0 border-byte RTL/vector change); `plus/p0-parser-wiring` (A5
fail-closed CPR-parser decisions plus P0 cartridge service, MMU, top-level wiring, boot
integration, and tests); `docs/split-differential-evidence` (the preserved split-vs-unsplit
lockstep comparator and pinned run log, plus the post-merge correction that freezes both
historical sides so later intentional RTL changes do not invalidate the provenance check);
`accuracy/f6stage2-soak-expand` (soak sampled-field expansion and golden-hash re-mint, F6
Stage 2 seam measurement, and Stage-2b disambiguation/F13 findings); and
`plus/p1-crtc3-foundation` (the new CRTC3 register/counter/timing/video-pointer/sync RTL and
directed tests, including the C0=R1=R0 VMA save/reload priority fix). The tip also contains
this docs-only dispatch brief, added after the reviewed integration gates solely to preserve
the review instructions.

Review the whole delta, not only the named priorities. Treat pass-1 findings as unresolved
until you independently confirm their fixes; do not inherit the earlier verdict merely
because remediation commits exist.

## Authority rules

Use the project authority ranking exactly:

1. Real hardware and Logon System reference photographs are final authority.
2. ACCC v1.10 is the working oracle, not final authority.
3. Repository digests are derived aids.

When simulation and hardware disagree, hardware wins and the vector is wrong. Within
`docs/accuracy/audit-findings.md`, already faithfulness-reviewed **Rule** sections may be
trusted for this pass. Integration assumptions are unverified by default: check claims about
GA sampling, motherboard wiring, manifests, clocks, current paths, and test boundaries against
the actual sources. Clearly separate locally verified simulation, CI synthesis/RBF evidence,
and unavailable real-hardware/SHAKER evidence. The user-owned untracked
`docs/ACCC1.10-EN.pdf` must never be added or modified.

## Priorities

1. Recheck every pass-1 issue and its claimed remediation, especially the GA40010 dependency
   manifest, canonical handoff/roadmap truth, F6 boundary wording, soak claim limits, current
   wrapper paths, and guide/question numbering. State explicitly whether each issue is closed,
   remains open, or regressed.
2. Review classic `accuracy/a3-f6-stage1` as new behavior: independently derive the t20i and
   t10a-t10e expectations from their cited ACCC rules; verify the R1>R0 comparator, the
   substituted type-0 border-start term, its placement before SKEW-DISPTMG, type-1 isolation,
   and the first behavior-driven hash re-mint.
3. Review Plus A5/P0 as new load-bearing logic: classic-mode bit identity, parser abort and
   backpressure behavior, cartridge publication atomicity, MMU ownership/watchdog behavior,
   CPU mux/WAIT integration, top-level wiring, and `files.qip` inclusion in the same commit as
   production instantiation. Look specifically for fail-open publication or stale ownership.
4. Verify the soak sampled-field expansion commit changes **only** the sampled field set: no
   seed, stimulus schedule, event budget, sample phase, or production RTL change. Confirm the
   added fields close pass-1 issue 4 and that the re-mint rationale/current hash is recorded
   consistently in `docs/plans/2026-08-22-accc-review-plan.md`, `sim/README.md`,
   `docs/accuracy/type-split-review-guide.md`, and `AGENTS.md`.
5. Verify F6 Stage 2 and Stage 2b are measurement-and-documentation only, with no production
   RTL behavior change. Audit the measurement harness/log and the boundary inference against
   the real GA path. Confirm promoted finding F13 is evidentiary and explicitly
   hardware-blocked, not presented as an implemented fix.
6. Review Plus P1 like any other new ACCC-derived RTL: attribution headers, point-of-use ACCC
   citations, directed assertions derived from the source rather than the implementation,
   counter/live-write edge cases, and fail-closed discipline. Verify the `files.qip`
   same-commit rule is respected: `asic_video.v` is deliberately not yet instantiated, so its
   manifest addition must remain deferred until the commit that first wires it into production.
   Read hardest the C0=R1=R0 VMA save/reload priority, R9 completion versus equality capture,
   R5 adjustment entry/end and live retargeting, C4=0/C0=0 reload, HSYNC start/width including
   the static infinite-HSYNC relation, VSYNC re-fire behavior, and all documented P1 scope cuts.
7. Review `tools/split-differential` and its evidence log for provenance and honest claim
   boundaries. Confirm the default run extracts `418aa68` as the unsplit reference and
   `2d4f880` as the reviewed split tip, that overrides are explicit, and that `no divergence`
   is not misrepresented as proof about later intentional behavior changes or unsampled phases.
8. Audit test/assertion integrity throughout: expected values must follow cited rules rather
   than simulator output, no assertion may have been weakened to go green, known divergences
   must retain correct XFAIL/XPASS semantics, and review-debt rows must name the genuinely hard
   seams without claiming mechanical gates substitute for review.

The soak hash changed twice by design: first for the F6 Stage 1 production behavior change,
then for the sampled-field expansion with no RTL change; the rationale for each re-mint is
recorded in `docs/plans/2026-08-22-accc-review-plan.md`.

## Required gates

Run and report the exact commands and outcomes:

```sh
git diff --check 2d4f880..accc-review-and-fixes
make -C sim
make -C sim lint
make -C sim soak SOAK_EXPECT=0xf5f8ae01ffdf928d
tools/split-differential/run.sh
```

Also inspect the exact-tip GitHub Actions result and distinguish the simulation and Quartus
synthesis jobs. Synthesis proves buildability, not hardware accuracy. If a local gate fails,
do not repair it; capture the command, first useful failure, and likely owning change.

## Deliverable format

Return one self-contained Markdown review with:

1. **Reviewed range and tip** — exact SHAs and whether the worktree was clean.
2. **Overall verdict** — `approved`, `blocking issues found`, or `questions`, with a concise
   rationale.
3. **Verdict by area** — at minimum: pass-1 remediation, classic A3/F6 Stage 1, Plus A5/P0,
   soak expansion/re-mint, F6 Stage 2/2b/F13, Plus P1, differential evidence, documentation,
   and test/assertion integrity.
4. **Findings** — ordered by severity. For each, give severity, precise `file:line` grounding,
   impact, reasoning, and the evidence or change that would resolve it. Say explicitly when
   there are no findings in an area.
5. **Priority-check results** — a direct confirmed/refuted/unverified answer for every numbered
   priority above.
6. **Mechanical verification** — command outputs summarized with counts/hash, plus the exact CI
   run and separate simulation/synthesis conclusions.
7. **Could not verify** — especially real `.cpr` boot, classic side-by-side hardware regression,
   SHAKER/DE-pin evidence for F13, or any ACCC claim not independently checked.
8. **Review-debt disposition** — which rows can clear, which remain, and why.

Use precise, actionable findings rather than implementation suggestions. Do not expand scope
into F7 RFD or any further classic/Plus implementation.
