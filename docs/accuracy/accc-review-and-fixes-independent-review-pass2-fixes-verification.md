Independent pass-3 review — accc-review-and-fixes

Base 0773ad4 (pass-2 tip) → HEAD d64e449. 14 commits. Worktree clean before and after.

Verdict: all 11 pass-2 findings resolved. All gates green. No regression found. Branch content may be treated as settled, subject to the two questions and the standing hardware gaps below.

---

PART A — Fix verification

All three gates re-run on HEAD, plus two extras pass-2 used.

┌─────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                    Gate                     │                                                              Result                                                              │
├─────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ make -C sim                                 │ exit 0 — classic Summary: 93 passed, 0 xfailed, 0 xpassed, 0 failed; Plus: 7 unlock, 27 asic_video (was 26), parser, MMU,        │
│                                             │ cartridge service, 6 P0 boot all pass                                                                                            │
├─────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ make -C sim lint                            │ exit 0 — 64 warnings, no errors                                                                                                  │
├─────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ make -C sim soak                            │ exit 0 — seed 0xaccc5eed20260822, 2,845,088 samples, hash matched                                                                │
│ SOAK_EXPECT=0xf5f8ae01ffdf928d              │                                                                                                                                  │
├─────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ git diff --check 2d4f880..HEAD              │ exit 0 (pass-2: exit 2, 201 errors)                                                                                              │
├─────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ tools/split-differential/run.sh             │ exit 0 — 22,627,500 / 45,498,863 samples, no divergence; reproduces the committed log exactly                                    │
└─────────────────────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘

Findings

→ 1. Critical, MMU watchdog returns open bus during a legitimate load. RESOLVED. rtl/plus/plus_mmu.v:195 adds a cart_busy branch that zeroes stall_count while the service is loading; rtl/plus/plus_cartridge_memory.v:89 drives busy = load_active; wired at Amstrad.sv:1052/:1122. rtl/plus/plus_cartridge_memory.v:202 changes cpu_consumed <= cpu_valid to <= 1'b0 at begin_event, so a held read is deferred and replayed against the new image rather than consumed. Integration proof is real: sim/plus/p0_boot_test_top.v:78 now instantiates plus_mmu, and sim/plus/p0_boot_test.cpp:427 holds a read across the production 512 KiB clear (CLEAR_BYTES is not overridden, so it is 20'd524288), asserts mmu_cart_stall stays high for 1400 cycles (> the 1024 STALL_TIMEOUT) and through every loader byte, then checks the committed byte, not FF.

→ 2. Critical, late acknowledgement satisfies a later read. RESOLVED. Two-sided fix at rtl/plus/plus_cartridge_memory.v:220 (same-cycle discard when request_kind==REQUEST_CPU && !cpu_valid) and :256 (arm discard_request when the logical cycle vanishes while the physical request is still in flight). I traced the race myself: if cpu_valid drops for one cycle and re-asserts for a new address before mem_ack, discard_request is already latched, so the first condition at :220 wins and cpu_ready never pulses for the stale address — the exact failure mode pass-2 named. Covered by two new tests at sim/plus/plus_cartridge_memory_test.cpp (abandonment + rearm, and load_begin with a pending physical read + replay), plus sim/plus/plus_mmu_test.cpp test_watchdog_waits_out_legitimate_load.

→ 3. High, GA40010 manifest incomplete. RESOLVED — verified independently, not taken on trust. rtl/GA40010/Makefile:8 adds casgen_sync.v. I elaborated from the checked-in HDL_FILES into a fresh --Mdir (/tmp/ga_elab_check, no stale obj_dir): exit 0, 13 modules, warnings only. The %Error-MODMISSING: casgen_sync pass-2 hit is gone. See question Q1 for what still does not build.

→ 4. High, P1 suppresses the live R2 HSYNC collision. RESOLVED — chronogram-verified. rtl/plus/asic_video.v:379 replaces the static W mod (R0+1)==0 relation with a live test: hsync_end_start_collision = (R3 != 0) && hsync_start, evaluated on the same edge as hsync_end_hit (:392). I read ACCC p.151 §15.3.5 visually (see pipeline) and the RTL reproduces row 6 of that chronogram exactly, including the detail the old code got wrong. New vector t04h at sim/plus/asic_video_test.cpp:433 encodes it; my hand-trace of the RTL matches the drawn C3 sequence and both HSYNC edges. Detail below.

→ 5. High, canonical handoff regressed. RESOLVED. docs/current-status.md:66 now says the split/rename were behaviour-preserving but F6 Stage 1 intentionally changed DE behaviour, and names 0xf5f8ae01ffdf928d; :69 reclassifies 4c78603 as the F8-era bisection milestone rather than the newest synthesized one; :101 states P0 is production-wired and P1 video uninstantiated (no more tied-off/live contradiction); docs/implementation-roadmap.md:312 schedules the P1 remainder, not the completed foundation. I verified the CI claim rather than accepting it: run 32645547100 is success on both simulation and synthesis at f6f09f5 — which is after every RTL commit in the delta, with only docs after it.

→ 6. High, git diff --check fails. RESOLVED. 68b8aef is provably whitespace-only (git show -w 68b8aef produces an empty diff). Gate now exit 0 over the full 2d4f880..HEAD range.

→ 7. Medium, r6_border_condition unsampled. RESOLVED and independently reproduced. tools/split-differential/diff_main.cpp:28,188,224 sample the latch on both frozen sides; README.md:48 narrows the overclaim to "unlisted combinational or temporary internal wires". I re-ran the harness myself: identical totals to the committed log, no divergence.

→ 8. Medium, wrapper rename sweep incomplete. RESOLVED. Every surviving UM6845R reference is now explicitly historical or correct: docs/plans/...:181 documents the rename itself, docs/implementation-roadmap.md:27 says "renamed from", and the differential tool's git show 418aa68:rtl/UM6845R.v is the frozen pre-split extraction, which is right. docs/plus/architecture.md no longer names the deleted file.

→ 9. Medium, t01e under-sourced. RESOLVED — honestly, which is the right outcome. The claim was not defended; it was reclassified. rtl/plus/asic_video.v:161 now says the post-write behaviour "is not given as a direct CRTC3 chronogram" and explicitly retracts §28.1.1 as support ("it describes C4/C9 identification overflow, not C0" — which is correct). The test name itself carries (unverified model assumption) at sim/plus/asic_video_test.cpp:893. The expectation was kept as a regression anchor, not dressed up as sourced.

→ 10. Low, F6 comments blur the approximation. RESOLVED. rtl/crtc_type0_engine.v:365 and sim/sim_main.cpp:3653 now state the source describes 0.5 µs while the bench pins DE low for the full 1 µs character, "not exact ACCC pin timing". I verified the underlying ACCC fact visually rather than trusting the paraphrase — p.186's §17.6.2 chronogram draws the C0=R0 cell half green, half orange, versus §17.6.1 above it where the cell is fully orange. The disclosure is accurate.

→ 11. Low, residual counts and formatting. RESOLVED. sim/README.md and docs/accuracy/testbench-spec.md:145 both say 93; docs/accuracy/type-split-review-guide.md:112 records A3's t20i as complete; docs/review-debt.md has exactly one table header (line 156). Remaining 87 hits are ACCC page 87 and a historical provenance row — both correct.

None of the eleven findings was factually wrong. Finding 4 in particular was a genuine RTL defect and the p.151 chronogram proves it.

Assertion integrity

The only deletions in sim/ are three copies of require(++guard < 4000000, ...) raised to 20000000. That is a hang watchdog, not a behavioural expectation, and the raise is required by the fix — the bench now waits out a real 524,288-write clear. No assertion was weakened. t04a setup changed (park R2=255, drain the reset pulse, then R2=6) but its assertion loop is byte-identical; see Q2.

---

PART B — Fresh-eyes spot-checks

I did these against git show 418aa68:rtl/UM6845R.v and the ACCC directly, before reading pass 2's reasoning on them.

→ B1. Wrapper mux seams — CONFIRMED. The 18 CRTC_TYPE ? sites in rtl/CRTC.v all reduce to the pre-split expressions. The two that could have hidden a bug both check out: c5_next moved from a conditional register update into the engines, and rtl/crtc_type1_engine.v:132 supplies the else c5_next = c5 hold that replaces the pre-split if(line_new) guard, so the wrapper's unconditional c5 <= c5_next is equivalent; rtl/crtc_type0_engine.v:196 hardwires 5'd0, matching the pre-split unconditional type-0 clear. in_adj_route (crtc_type0_engine.v:240) is character-for-character the pre-split line_new && !CRTC_TYPE && (...).

→ B2. Type-0 latch cluster and holdoff program order — CONFIRMED. The 8-register cluster at rtl/crtc_type0_engine.v:257-310 matches pre-split lines 300-355 line for line, including the ~nRESET | SNA_LOAD | CRTC_TYPE reset. The holdoff block at :328-350 has the load-bearing order right: tick-clear first, r7_write_hit second (so a write coinciding with a tick lands on !vsync_count_tick_t0 = 0, correctly not arming), and CRTC_TYPE || SNA_LOAD clear last so it dominates — which is what the comment claims and what the last-assignment-wins semantics require. The false-comparison branch genuinely leaves the latch untouched.

→ B3. hcc==0 capture and the hcc==2 keep term — CONFIRMED. rtl/CRTC.v:289-293 is identical to pre-split :384-388. rtl/CRTC.v:297 consumes e0_hcc2_adj_keep ungated by type, and rtl/crtc_type0_engine.v:250 defines it as |type0_effective_r5 — exactly the pre-split if(hcc == 2) frame_adj_r <= frame_adj_r & |type0_effective_r5. The type-split guide's item 4 describes this correctly; the quirk that the flop updates while type 1 runs is preserved.

→ B4. Parser fail-closed paths — CONFIRMED, adversarially. Bounds arithmetic is sound: riff_len_valid caps at 0x01FFFFF7 so riff_limit = full_riff_len[24:0] + 8 cannot overflow 25 bits and the truncation is lossless; chunk_extent_exceeds_riff uses 33-bit math against a full 32-bit length, no wrap. Sequence validation aborts on every wrong header byte, on is_cb_prefix && !cb_valid (a cbXX with a bad digit), on a block chunk over 16 KiB, on any byte arriving in STATE_DONE, and in the default: arm. has_block == 0 blocks commit, so an all-metadata image fails closed. load_error coinciding with the download falling is caught by the explicit && !load_error guard at :186. The one path I probed hardest: a byte arriving while load_valid is high is silently dropped without advancing expected_addr, so the next byte trips ioctl_addr != expected_addr and aborts — data loss under backpressure becomes an abort, never a corrupt image. Defence is layered: decoded_page maxes at 31 and chunk_bytes_read < 16384 gates load_valid, yet the service independently rejects load_page[5]/load_offset[14].

→ B5. Soak expansion scope — CONFIRMED. 750ea32 touches sim/sim_main.cpp only to append three mix() calls (type0_vsync_wait_line_start, r6_border_condition, status_bit5_r) after the existing fields, plus four doc files. No RTL, no seed, no stimulus, no schedule, no sample phase. The re-mint rationale is present in all four required locations: AGENTS.md:43, sim/README.md:107, docs/accuracy/type-split-review-guide.md:34, docs/plans/2026-08-22-accc-review-plan.md:260. The fix delta's own sim_main.cpp change is comment-only, confirmed empirically by the hash still matching.

---

The p.151 chronogram, read directly

Worth stating in full, because it is what converts finding 4 from an opinion into a fact.

Setup R2=11, R3=10. Baseline row: C3 runs 0..9 under C0=11..20, hits 10 = R3 at C0=21, HSYNC ends.

Collision row (OUT CRTC-R2, 21): at C0=21 the natural end (C3→10) and the rewritten start (C0=R2=21) land on the same edge. C3 does not reset. It continues 10,11,12,13,14,15, wraps 0,1,…,9, and reaches 10 again at C0=37. The Monitor Sync bar and the green "Hsync displayed" band run through C0=36; C0=37 is white.

The old static predicate is false here (W=10, R0+1=64, 10 mod 64 ≠ 0), so the pre-fix RTL would have ended the pulse at C0=21 — a 26-character error. The new RTL, hand-traced, holds HSYNC through 36 and clears it entering 37. t04h asserts precisely that.

Rows 2–5 of the same figure (R2 rewritten to 17/18/19/20, inside the pulse) show no restart, and row 7 (R2=22, after the end) shows two separate pulses. The RTL gets both right, because hsync_start is only consulted while !in_hsync except through the collision term. §15.3.1 p.148 states the rule flatly: "On the CRTC's 1, 2, 3 and 4, there is a bug if C0=R2 on C0=R2+R3."

---

EVIDENCE PIPELINE

Regenerated renders: yes. PDF verified first: docs/ACCC1.10-EN.pdf, SHA-256 1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560, 295 pages — matches the brief. Untracked, unmodified, never committed.

Rendered fresh at 200 dpi to a scratch directory outside the repo (…/scratchpad/accc_png/), never docs/accuracy/extract/pages/:

.venv/bin/python -c "
import pymupdf
d=pymupdf.open('docs/ACCC1.10-EN.pdf')
for n in [141,148,149,150,151,185,186,187,188,190,193,194,195]:
    d[n-1].get_pixmap(dpi=200).save('<scratch>/p%03d.png'%n)"

Page index mapping verified empirically (doc page N = PDF index N−1) rather than assumed. I relied on zero pre-existing artifacts under docs/accuracy/extract/.

Read visually from the fresh PNG (authoritative):
- p.151 §15.3.5 — the CRTC3/4 live-R2 chronogram. All seven rows. Sole basis for the finding-4 verdict. Marked visual-read: the text layer cannot represent this figure at all.
- p.186 §17.6.2 — the CRTC 0/2 border chronogram. The half-green/half-orange C0=R0 cell is visual-read only; the text layer conveys "0.5 µsec" but not that the blip occupies the second half of the character, nor the contrast with §17.6.1's full-orange cell directly above.
- p.148 §15.3.1/15.3.2 — rule text, cross-checked against the text layer, agreeing.

Read via text layer only (pdftotext -layout), no diagram load-bearing:
- p.186 prose — cross-checked against the visual read, agreeing.
- p.188 §18.1/§18.2.2 — Q16. Directly supported: "This rule is true whatever the value of C9", "Except on CRTC 3 and 4, R6 is considered immediately on the current C0", "The condition C4=R6 is considered immediately (regardless of the value of C0)". Q16's "not a first-scanline restriction" reading is confirmed. No diagram is needed for this claim.
- p.190 §18.3.2 — Q15. The contradictory sentence is verbatim as quoted: "This alternation only takes place when the condition R1 is fulfilled (BORDER R1 is false)." The preceding mechanism ("DISPLAY ENABLE changes to ON at the beginning of the CRTC character and returns to OFF 0.5 µsec later") is also verbatim. The repo's default reading is reasonable and correctly labelled a default reading, not established fact. Labelled text-layer-only: I did not visually read p.190, so if a figure there bears on the ambiguity I have not seen it. The repo does not treat Q15 as settled, so nothing rests on this.
- p.141 §14.5 — "A value of 0 in R3 will generate a HSYNC of 16 μsec, unless it is interrupted by modifying R3 during HSYNC." See Q3.

Rendered but not read: pp.149, 150, 185, 187, 193, 194, 195. Nothing in my conclusions depends on them.

Could not verify: nothing was blocked by inability to read images. Standing gaps unchanged from pass 2 — real .cpr boot on hardware, SHAKER/DE-pin capture for F13, the exact 0.5 µs type-0 event, CRTC3 live-R2 against real silicon (ACCC remains the working oracle, not final authority), and the Stage-2 temporary harness counts.

---

Issues

→ Q1 (question, non-blocking). The GA40010 co-sim target still cannot build to completion, for two reasons unrelated to the manifest. Running the Makefile's exact command (verilator --trace --top-module ga40010_test -cc $HDL_FILES) exits 1: %Error: Exiting due to 4 warning(s) — four PINMISSING warnings (ga40010.MODE, CRTC.nCLKEN, and two more) are fatal without -Wno-fatal, which the Makefile does not pass. Separately, rtl/GA40010/Makefile:5 hardcodes /usr/bin/verilator, which does not exist on this machine. Both predate this delta and neither is what finding 3 was about — the missing module is genuinely fixed, and this directory is not a make -C sim gate. Flagging so nobody later reads "manifest fixed" as "the target builds". Resolved by adding -Wno-fatal (or wiring the missing pins) and honouring a VERILATOR_BIN override.

→ Q2 (question, non-blocking). t04a's setup was changed in the same commit as the RTL behaviour it tests. sim/plus/asic_video_test.cpp:355 now parks R2=255, drains via run_until_hsync_idle(), then writes R2=6. I checked whether this masks anything: the assertion loop is unchanged, and run_until_hsync_idle throws after 256 characters, so a wrongly-triggered infinite HSYNC would fail rather than pass. The transient is real — at reset R2=R3=0, and writing R3=0x65 mid-pulse can now legitimately collide. I judge this legitimate isolation, not a green-washed test, and the comment cites §15.3.1. Raising it because "test edited alongside the RTL it covers" is exactly the pattern the repo's discipline exists to catch, and a second opinion is cheap.

→ Q3 (question, non-blocking). The R3l != 0 guard on the collision is an inference, not a cited rule. rtl/plus/asic_video.v:380 disables the collision when R3l=0, and the comment justifies it as "retains the bounded 16-character rule from §14.5". §14.5 p.141 says an R3=0 HSYNC is 16 µsec and is bounded — consistent with, but not a statement about, the end/start collision. §15.3.2's infinite-HSYNC example uses R3=1, not R3=0. Given finding 9 was resolved by honestly labelling exactly this kind of gap, the same treatment would fit here. Resolved by an explicit "unverified model assumption" note, or a direct rule/hardware observation.

→ Q4 (question, non-blocking). The load-time fail-closed policy trades open-bus data for an unbounded CPU stall. Pass-2's finding 1 grounding included Amstrad.sv:672 not listing cpr_download in machine reset; that is still the case (only rom_download, dan_download, sna_download etc. reset). The consequence is now different: instead of returning FF, a cartridge read during a load holds the Z80 via WAIT until publication. load_active is bounded in practice — the parser pulses load_commit or load_abort when cpr_download falls, and both clear it — so a user-aborted transfer recovers. But if the HPS wedges with ioctl_download stuck high, the machine now hangs where it previously limped. This is a deliberate, defensible fail-closed choice and the architecture doc records it; I am flagging the changed failure mode, not disputing the choice.

No blocking issues.

---

Closing disposition

The fixes stand. Every pass-2 finding is closed at the level pass 2 asked for, and the two that mattered most were closed with real evidence rather than argument: finding 1 by an integration bench that now instantiates the MMU and runs a production-sized 512 KiB clear, and finding 4 by RTL that reproduces the p.151 chronogram I read myself. Finding 9 was closed the honest way — by retracting an unsupported citation rather than defending it — which is a good sign about the fix session's posture.

The branch content may be treated as settled within its declared boundaries. Three things are not closed and should not be read as closed: F6 Stage 1 is a 2× full-character approximation of a documented half-character blip (disclosed, F13 hardware-blocked); t01e and the R3l=0 guard are model assumptions, not sourced rules; and no .cpr has booted on real hardware. Those are recorded gaps, not review debt from this delta.

Review-debt disposition: the four rows d64e449 marks "awaiting reviewer confirmation" — accc-review-and-fixes, plus/p0-parser-wiring, docs/split-differential-evidence, plus/p1-crtc3-foundation — may be cleared on this pass. accuracy/a3-f6-stage1 and accuracy/crtc-type-split were already clear and remain so. A1/A2 remain open and were not in scope here.

Also found: asic_video.v is correctly absent from files.qip (verified), so the P1 collision fix cannot affect the current RBF; plus_mmu.v, plus_cartridge_memory.v and plus_cpr_parser.v are all present, and the top-level cart_busy wiring is covered by the green synthesis job in run 32645547100. Per CLAUDE.md, d64e449 is docs-only on top of that, so no further CI run is owed.
