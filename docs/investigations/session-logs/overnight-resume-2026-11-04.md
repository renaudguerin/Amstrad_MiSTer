# Active unattended handover — 4 November 2026, 22:00 UTC

Continue known SHAKER and DSC4/Amazing work until approximately 80% weekly
Codex usage. Do not stop merely because one unit finishes. Last live OAuth
reading at 21:46: 59% weekly used, reset 5 November 19:34 UTC; three banked
resets untouched. No credits, overage or reset redemption. Keep `ask-claude`
paused until its five-hour window resets and fresh telemetry verifies that.
Use Gemini 3.8 for routine work, prepared Gemini 4 for hard independent review,
and Sol high for RBF changes. Known defects precede a broad fresh baseline.

Checkout: `/Users/renaudg/.codex/worktrees/c1e6/Amstrad_MiSTer`.
Ordinary branch: `codex/overnight-validation-20261103`, HEAD `51ae4dd`.
Branch pushes and full milestone builds are authorized. No master integration,
PR, release or author message. Parent owns all shared MiSTer/AmSpirit access;
device use remains authorized with no hold. SSH and Git metadata writes need
the approved `require_escalated` path; no user SSH-agent action is needed.

## Committed CRTC0 fix: build, then native acceptance

Sol high `/root/r3_output_model_scope` changed only `rtl/crtc_type0_engine.v`,
`sim/sim_main.cpp` and `sim/soak-golden-history.md`. They are frozen after the
final gate. Parent verified these SHA256 values at 21:53:

- Type0: `148837f54e888869d1cb0c29fb219edac05c8481d25a5095a16dda1d9460f05d`
- sim: `6c8b3d89c14054e4fb1ecef800a5c6b7b8b7765b6513041b90b7c3a7d70859e1`
- history: `7cac32e4d557c1f1e384ad3e1e963921ae9a353bddd1798fe177a3a41d2601b0`

CRTC 240 PASS. Intentional soak `f9b5f86e16277c55`, unchanged seed/projection,
2,845,088 samples. Final `python3 sim/select_tests.py --run`: PASS 45 benches,
once after the last source edit. Trust that gate; no source edit followed it.
Private evidence root: `local/overnight-20261103/resumed-20261104/`.
Read `c0-forced-zero-adjust-fix/r4-terminal-fix/HANDOFF.md`, `new-delta.diff`,
`full-candidate.diff`, source hashes and red/gate logs before acceptance.

French §12.2 p95 distinguishes a forced current additional line from the
unmodified short-R0 one-extra-line case. §11.2.2 p83 / §13.2.4 p108 retain the
R9 comparator while pending C4=R4; an unequal R4 selects R5. Initial forced
zero-entry vectors failed before RTL. Old universal one-line expectations
were corrected against primary source, with red logs preserved. Opus accepted
that initial candidate, but its review does not cover the terminal delta.
The rejected generic downward-R9 completion at natural wrap is not a rule:
7→6 completes on line32, not line25. Keep the correct L32 and original
positive-R5 t16z controls.

The real terminal-R4 red uses natural C4=R4=8, R0=63, R8=0 and either
R9=C9=31/R5=0 or R9=C9=3/R5=4. Writing R4=10 at C0=1 already selects R5;
origin must occur at the first boundary. The prior candidate held row8.
The final delta consumes that terminal result through canonical decisions,
retains its selected target using existing R5 override storage, and excludes
accepting-edge R9 ownership on short R0. No wrapper, new flop or global
case28 rematch. Public window/origin/VSYNC/follow-through controls pass;
a storage-echo assertion was removed rather than changed to mirror storage.

Fresh Gemini 4 Argon high review `20261104T215343Z-88790-cc7b` ACCEPTS
with no findings; exec4182 exited cleanly and logs are preserved under
`r4-terminal-fix/terminal-cross-provider-review/`. Parent verified source
hashes; no later edit or gate rerun. Committed/pushed `51ae4dd30b2a16c9d2cb59e1e1fa8d511aced520`.
Full milestone workflow41921307910 is running on that exact SHA. Push workflow
41921280149 is simulation-only. No other synthesis is active. Wait for the full
artifact, verify timing/hash, then native C0 E6 and bounded DSC4 regression.

## S diagnostic: native pair complete, source proof next

Isolated branch `codex/s-passive-native-20261104` at
`ae9ccdc02d6ac941eddc9b4aab771b15218ffa20` only enables the reviewed observer.
Workflow41910599920 succeeds; artifact13363847112, build363.
RBF `Amstrad_20261104_ae9ccdc.rbf`, 3,139,340 bytes, SHA256
`ed83e351fc650633c7e6021cf3f7521b38550b3265a5a8132c7b8c5a7a250d54`.
Full Quartus17.0.2.602, Standard Fit/High Performance Effort, macro ON,
seven setup/hold groups, zero TNS, minima0.548/0.178ns, no errors/critical
warnings. Installed file hash matches. Normal production QSF remains OFF.
Default-off observer50b2402 has a passing 717-M1/50k-clock production-T80
checkpoint-instruction replay, 45-bench gate, unchanged soak and independent
Gemini4 acceptance. Replay trampolines are not original hidden-state evidence.

Both native paths COMPLETE and drained: direct10commands/1PNG/0fail/16SSM;
history131commands/64PNGs/0fail/136SSM. Each has two identical1040-byte reads,
all retained sequence checks, seven ordered cuts, error0/drop0. Direct glyph
and timing give4E40/N1188; history gives0440/N4, exactly retaining production9a.
Direct acquisition has six failed retries, history zero; both R8 pulses8µs.
All64 history images and eleven changed pairs were inspected:53 pixel-identical
to production9a, eleven raster-pattern changes; E/P/S identical. No new photo
verdict. Evidence: `s-passive-fpga-trace/native-{direct,history}-ae9ccdc/`,
`native-pair-qualified-values.json`, `native-history-page-comparison.json`.

Direct adjustment is off/C5=0 through R5. History already has adjustment on/
C5=31 at A4CF, stays on through A067/A06B with C5=0, then reaches the R5
fetch5BFB with C5=24. At R7 fetch5B0A, both have C0=13/C9=C5=0/IVM1/parities0;
direct has C4=4/adjustment off, history C4=2/adjustment on/added line active.
This is an observed inherited-state difference, not permission to clear C5.

Sol `/root/r3_output_model_scope` now owns only private
`c1-s-live-state-analysis/` source analysis and an isolated failing proof.
Parent read French §11.8 p93: R9=7/R5=8 in IVM has four C9 per C4, hence two
C4 increments during eight adjustment lines. Current Type1 overrides IVM
stepping and row tests inside adjustment despite calling the interaction
unpinned. Worker may prepare a private deterministic red, but must not edit
tracked RTL/sim/history yet. Keep C5 lifetime/restart and E0080/P26 unchanged.
The original round2 `message.txt` is absent; its tracked technical summary is
not literal correspondence. Do not invent the missing wording.

AmSpirit matched original direct/history both give4E40/N1188 with all ten
register arrays equal; v2 SNA counters are unavailable, not zero. CPCSE4E40
versus physical photo4E30 revision binding remains unresolved. Device is idle
at diagnostic history S0440; no device/API job. AmSpirit is paused at C0 E6
040A with temporary scripts/breakpoints cleared. A prior same9a repeat failed
after28captures on SSH/SCP timeouts: no S scalar, no core verdict, no active
job; later SSH succeeds with no orphan MBC client.

## Other completed evidence and remaining work

Gemini3 case2/4 retrieval COMPLETE; exec78595 drained. Run
`20261104T214048Z-85364-e02a`, logs preserved in
`e6-case2-4-retrieval/provider-log/`. Parent verified media/routine/label
hashes and accepted recipes as navigation only. Its report confuses184,320-byte
sector payload with194,816-byte DSK container; parent qualification records
that and OUT-versus-preceding-INC PC conventions. No native bus phase or
causal RTL conclusion. Native2=0037 versus00F7,4=0038 versus0032.

Functional9a build41900154684/artifact13359402650 has RBF SHA256
`eadc739cb8809236df65866ad36f99b9b6ba1dd04c22e71d507318e53d0c5576`.
C0 E6 case25 fixed0003→051B;19 changed0003→002B versus00A3. Eleven of28
printed checks fail:2,4,6,8,9,10,16,17,19,27,28. Cases5/14/21/24 retain
00C0/0D97/0066/000E. Case27 is0055 versus0065. Use immutable-label glyph
decoding, not tiny-font guesses. AmSpirit passes28/28, CPCSE9/28; no E physical
photos. Bounded C0 DSC4 views are clear. C1 E/P0080/26 remain; DI58vs59,
B6 1F/1Fvs1E/1F and R7.JIT8/4vs9/5 remain open.

R3/mixed-mode controls are qualified but no universal model is selected.
Latest JIT2→0 requires source40 pen5, actual Type0pen0/Type1pen1; successor
A6 pens13/2 and all six fixed controls pass. 0.25µs is four Pixel-M2, not one.
Historical S words:599=0640,e574=5060 (earlier9600 reading withdrawn),9a=0440;
all direct4E40. A4 last four PB0 pairs already0/0; earlier1/1 was an E/F glyph
error. ccd49d4 fixes the runner's poll-completion versus screenshot deadline;
223 tests/12 skipped and selected gate passed. ASIC275af8a credits/BI/sampled
effects have bounded acceptance. Full demos, monitor and remaining SHAKER
questions are recorded in dated reports; no broad baseline has been launched.

Keep current-status and tally focused on the current tip. Parent owns dirty
docs; stage code separately. Pure docs need links/diff checks, no simulation
or provider review. Check the reaper before replacing a bridge; preserve logs
and own each run through exit. Commentary at least once per60s during work.
No goal tool was created.
