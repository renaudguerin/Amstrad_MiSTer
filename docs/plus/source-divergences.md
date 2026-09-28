# Plus ASIC: where the RTL departs from, or chooses between, written sources

This ledger lists every Plus ASIC rule where the RTL follows something other than a single
uncontested written source. That covers a source conflict, a derived rule, an emulator-derived
rule and a model assumption. Each row names the hardware mechanism the RTL implements, the
evidence for it, how confident we are, and the discriminator that would settle it.

Classic CRTC (types 0/1) divergences from the ACCC are recorded per finding in
[audit-findings.md](../classic/audit-findings.md), where the ACCC is the working oracle.

**Policy.** A row describes one hardware mechanism that explains every observation. It never
names a title-specific workaround. Titles and emulators are evidence, not rules. When a probe
result contradicts a row, the mechanism changes, the row is revised, and titles are rechecked.

Confidence, strongest first:

1. **hardware**: original Plus/GX4000 photographs confirm it.
2. **title+emulator**: a real title needs it and at least one emulator agrees; no hardware test.
3. **derived**: read from written sources with an inference the sources do not state.
4. **assumption**: a named model choice with no source.

The probe screen numbers refer to [the multi-test probe cartridge](../../scripts/diagnostics/README.md#multi-test-probe-cartridge-plus_hw_probespy).

| Rule (owner) | Written sources say | RTL does | Evidence | Confidence | Discriminator |
|---|---|---|---|---|---|
| Ordinary PRI request phase (`asic_ga_timing.v`) | Revised Arnold §2.4: monitor-HSYNC trailing edge, clamped at start+6 µs. Kevin Thacker (KT): ~10 µs, width-independent | 1 µs after raw HSYNC start, every width: the rising edge of the one-character-delayed comparator ([mechanism](#pri-delayed-comparator-candidate)) | Original Plus 2026-09-27: flat-plane markers at widths 3/6/11 end ~136/~135/~139 dots in, ~78 without padding. The width rule is absent; RTL predicts 139/139/139 and 75. AmSpirit agrees; Eerie Forest needs ≤ start+4 µs ([evidence](references/eerie-pri-trigger-counterfactual-2026-09-27.md#original-hardware-2026-09-27)) | **hardware** for width independence and the 1 µs CPU slot (probe 03 repeats it: ~140 dots). Sub-µs position is invisible to software | Settled. A raw INT trace would add sub-µs detail |
| PRI at HSYNC width 1 (same) | Not addressed | One request, 1 µs after HSYNC start (`HSYNC_d` is high for one character) | Original Plus 2026-09-27, probe 01: `IRQ/FRAME=01`, so width 1 **does** request (AmSpirit agrees). The RTL was wrong before the delayed comparator | **hardware** | Settled (`asic_pri_test` pr10) |
| PRI written to the current line during HSYNC (same) | Not addressed | Changing PRI to the current line while `HSYNC_d` (raw HSYNC one character late) is active requests at once. Same-value writes and writes after `HSYNC_d` ends do not | FF2 palette chain; CPCEC `cpcec.c:2106–2114`. Original Plus 2026-09-27, probe 04: marks for write C0 45–60, none 61–63, `IRQ/FRAME=16` (AmSpirit agrees). Writes during HSYNC do request, but the window runs one character past raw HSYNC end; the earlier raw-HSYNC window missed C0 60 | **hardware** | Settled (`asic_pri_test` pr11) |
| PRI line compare width (`asic_ga_timing.v`) | Revised Arnold: nine-bit `{0,PRI}` against `{VC5..0,RC2..0}`. CPCWiki ASIC page and Quasar: PRI=n also fires at n+256 | Nine-bit, no alias | Original Plus alias probe 2026-09-27: 32 IRQs in all four cases. Copter 271; AmSpirit; CPCEC | **hardware** | Settled |
| HSYNC crossing into the PRI line (same) | Revised Arnold §2.4: overlap can trigger at line entry | Line-entry request at the PRI line's C0 0 whenever `HSYNC_d` is still high there, including raw HSYNC ending exactly at the line start (the live-line term), plus the line's own request. With R2=63 the second request is at the next line's C0 0, where `line_d` still reads the PRI line | Original Plus 2026-09-27, probes 05–09: R2=49/57/58/62/63 with width 8 give `01/02/02/02/02` (AmSpirit agrees). Exact-end screens 19–20 (R2=56 width 8, R2=50 width 14): original Plus `02/02` (AmSpirit agrees). The CRTC3 demo's plasma (~38 s), sphere (~103 s) and Wolverine (~108 s) scenes use R2=50, width 14 and broke on MiSTer `d6618f4`, whose `line_d`-only comparator omitted that request | **hardware** | Settled (`asic_pri_test` pr12) |
| SPLT near the 312-line wrap (`asic_video.v`) | Eight-bit compare, so SPLT=55 also matches line 311 and is pathological (revised Arnold); KT says 56 | Eight-bit compare. A split captured on the terminal line survives the frame origin in VMA' while VMA reloads from R12/R13: frame line 0 shows R12/R13, line 1 onwards SSA | Original Plus 2026-09-27, probes 10–13: SPLT=54/56/57 split from line SPLT+1 as predicted. SPLT=55: frame line 0 still shows R12/R13, every later line comes from SSA, and the line-56 split restarts SSA again (AmSpirit identical). The line-311 capture is real; the frame origin wins for line 0 only | **hardware** (revised Arnold right, KT wrong) | Settled for the visible effect (`asic_video_test` t08k; probe sim matches AmSpirit 10–13). Original Plus 2026-09-27, cartridge V3: screen 21 rewrites SSA on line 311 at C0≈52 and frame line 1 shows the new value (`SSA ROW 05`, AmSpirit identical); control 22 agrees. The line-311 capture therefore happens after C0≈52, consistent with the [ARNOLD-REV §2.3] rule: C0=R0 when VCC=R4 and RCC=R9, C0=R1 otherwise. The RTL had implemented only the C0=R1 half; it now follows Arnold, equality included (`asic_video_test` t08l). Exact R0 timing and the ordinary R1 capture are source-based, not probed. Screen 23 (offset 7): the line-0 row capture replaces the held split (red to line 55), as the RTL and AmSpirit predict. V4 screen27 now shows captures at tested adjustment indexes0/1/8; screen28 gives an unresolved all-red terminal case with R5=16 (separate rows below) |
| SSCR vertical offset with R9>7 (`asic_video.v`) | §2.5 adds to the low three RA bits; the revised summary reads as a wider addition | Low-three-bit addition; the row capture is the level test `ra_eff >= R9` at C0=R1. With R9=11 and offset 5, raw rasters 0..11 display 5,6,7,0,1,2,3,4,13,14,15,8, so rasters 8, 9 and 10 capture and each character row advances three source rows | Original Plus 2026-09-27, probes 14–15: offset 0 shows rows in order; offset 5 shows rows 00, 03, 06, 09, 12, 15 with a second block 02, 05… lower right, identical to AmSpirit. The earlier equality capture repeated ROW 00; the lower-right block comes from the 2048-byte plane wrap of `{MA13..12, RA2..0, MA9..0}` | **hardware** (R9=7 unaffected: `>=` equals `==` there) | Settled (`asic_video_test` t08j; probe sim matches AmSpirit 14/15). R9<7 with an offset, where `>=` captures on several lines per row (R9=3, offset 2: raw rasters 1–3): original Plus 2026-09-27, cartridge V3 screens 24–25 show the bar staircase stepping on three lines of each row, as the RTL and AmSpirit predict |
| Sprite attribute write mirrors (`asic_regs.v`) | Revised Arnold: +3, +4, +5, +7 write magnification. KT: +4..+7 | +4..+7 write magnification; +3 is Y high only | Original Plus 2026-09-27, probe 16: +5/+6/+7 magnified, +3 not (AmSpirit agrees) | **hardware** (KT right) | Settled |
| Sprite X at the left edge (`asic_sprites.v`) | Arnold §2.1 prints X −64..+639 | X field is a 10-bit coordinate; at x4 the first visible dot is X=−63 | Original Plus 2026-09-27, probe 17: one-dot columns for X=−63 (x4) and X=−15 (x1) only | **hardware** | Settled |
| SSCR[7] extended border over sprites (`asic_video.v`) | §2.5: D7 extends the border; §2.1: border beats sprites | Sprites are hidden in the masked 16 dots | Eerie Forest green bar gone on MiSTer; five emulators agree. Original Plus 2026-09-27, probe 18: X=0 hidden, X=8 right half, X=16 whole | **hardware** | Settled |
| Sprite access blanking duration (`asic_sprites.v`) | "About one byte / 1 µs" | Two-dot tail (`blank_cnt=2`) | None | assumption | Needs sub-µs capture, not a photo probe |
| Classic request pending across a PRI 0→nonzero switch (`asic_ga_timing.v`) | Arnold §2.4 selects PRI instead of CPC interrupts; KT: CPC requests inactive while raster interrupts are active | A pending classic request is retained but masked, and delivered again when PRI returns to 0 | AmSpirit 1.15.1; Eerie Forest pending-classic investigation (`asic_pri_test.cpp` pr08) | title+emulator | No probe yet |
| Pending clear on PRI writes (not implemented) | Not addressed | Not imported. CPCEC clears a pending raster request on some PRI writes (Eerie Forest comment) | Eerie PRI writes never see a pending request in any tested timeline | decision | Revisit only with a title that needs it |

| Write-only GA `IN` CPU return and decoder byte (PA1) | KT reports79 on6128+ and78 on464+ for ED78; the general last-instruction-byte reading predicts78 | Decoder writes use last M1 opcode; otherwise-undriven GA CPU reads return the retained memory-read byte | 6128 Plus V4 screen26, IMG_3968: CPU78/40/50/58/60/68, repeat78 at7F54; palette follows opcode, controls066/F66 | **hardware** for tested instructions on this6128+ | PA1 fixed with production-T80 photo vectors; decoder path preserved. Other models/conflicting aliases remain unmeasured |
| Split captures inside adjustment (PA2) | Arnold restricts capture to the first adjustment character line, with ambiguous boundary wording | Split comparator remains active during adjustment | V4 screen27, IMG_3969: green297 and305–311, separated by red; phase dash visible | **hardware** for tested indexes0,1,8 and the index8 alias | PA2 implemented with fail-before `t08m`; R1 versus R0 capture and untested indexes remain unmeasured |
| Terminal split with R5>0 (PA3) | Frame-restart wording and next-opportunity wording permit different readings | R0 capture on R5=0 normal terminal or adjustment-ending line; last normal line with R5>0 uses R1 | V4 screen28, IMG_3970: all red; cyan dash not discernible | **hardware observation + emulator-derived mechanism** | AmSpirit matches late-arm all-red;24µs earlier enable produces green adjustment/red frame0–7. PA3 implemented with fail-before `t08n`/`t08o`; early/adjustment-terminal hardware controls unrun; interlace extra-line timing remains unprobed |
| Unmapped/write-only ASIC-page CPU reads (PA4) | Last byte of the reading instruction, including address operand | Page claims every read; explicit register ownership selects driven data or retained memory-read byte | V4 screen29, IMG_3971:50/68/7E/7E; RAM A5/5A and mapped0B/5A controls pass | **hardware** for the four instruction streams | PA4 fixed with production-T80 photo vectors and claimedFF controls; retained-memory-byte mechanism fits but is not uniquely proven |
| Compatible IRQ relative software slot (PA7) | Quasar relates Plus/CPC lateness to HSYNC; a common shift is invisible here | Same-Plus marker separation112 dots | V4 screen30, IMG_3972: approximately128 dots against local rulers, count02 | **hardware** for software-visible slot | [V5 request/acceptance controls](pa7-interrupt-phase-followup.md): Hardware G0/W192/L128/R128/I128; AmSpirit R144 differs at the first PRI reference marker. Resolve the CPU-visible reference phase before an IRQ-only fix. Not proof of an exact raw delay or absolute CPC-relative timing |

## Probe photographs

Original Plus, 2026-09-27, cartridge built at `749b9c4`: main checkout
`local/task-archives/plus-hw-probes-2026-09-27/real_hw/NN.jpg` (screen number; `IMG_*.HEIC`
originals beside them). Screens 01–02 are shot through a Retrotink 4K, because the OSSC
dropped sync at R3 widths 1–2; only their `IRQ/FRAME` counts are readable. Hardware matched
AmSpirit on every screen, and matched the RTL everywhere except 01, 04, 09, 11 and 15. For
the SPLT and SSCR screens the AmSpirit captures in `amspirit/` are pixel-exact stand-ins.

Screens 19–20 (cartridge V2, `local/task-archives/plus-hw-probes-2026-09-27/v2/`) were
photographed on the same original Plus later that day (`real_hw/19.jpg`, `20.jpg`): both read
`IRQ/FRAME=02`, with the line-entry and ordinary markers where MiSTer `1c85bee` puts them.

Screens 21–25 (cartridge V3, `local/task-archives/plus-hw-probes-v3-2026-09-27/`, photographs
in `real_hw/`) were photographed the same evening. All five match AmSpirit. Before the
terminal-line split fix the RTL differed on screen 21 only: its cyan dash confirms the SSA
write landed on line 311 after C0=R1.

V4 screens26–30: original **6128 Plus → Retrotink 4K → TV → phone**, supplied
2026-09-28 as `IMG_3968.HEIC` through `IMG_3972.HEIC`. Originals and viewable PNGs
are preserved in this task's ignored `output_files/plus-hw-probes/v4/hardware/`.
[Transcription, ruler reading and interpretation limits](asic-audit-probes-v4.md#original-6128-plus-photographs-2026-09-28)
were independently checked by the parent and an Astra medium reviewer. PA1/PA2/PA3/PA4 fixes and their remaining evidence limits are recorded in the
audit; PA7 remains open.

## PRI delayed-comparator candidate

Implemented in `asic_ga_timing.v` (2026-09-27); the heading keeps its anchor.

A single mechanism explains all three PRI mismatches (probes 01, 04, 09) and every settled
PRI observation. Request on the rising edge of

    HSYNC_d && PRI != 0 && (({0,PRI} == line_d && !adj_d) || ({0,PRI} == line && !adj))

where `HSYNC_d`, `line_d` and `adj_d` are the CRTC HSYNC, `{VC,RC}` line and vertical-adjust
flag delayed by one character (1 µs); `line`, `adj` and PRI are live. The line compare spans the
character in which the line changes: an `HSYNC_d` rise is judged against the old line, and a line
change while `HSYNC_d` is high is seen at once. This one edge detector replaced separate
ordinary, line-entry and value-change terms:

| Observation | Why it follows |
|---|---|
| Ordinary request 1 µs after HSYNC start, any width (hardware) | `HSYNC_d` rises one character late |
| Width 1 requests (probe 01) | `HSYNC_d` is active for one character |
| PRI writes request through C0 60 with HSYNC 49–59 (probe 04) | The `HSYNC_d` window is C0 50–60 |
| R2=57/58/62 give two requests (probes 06–08) | Line entry seen while `HSYNC_d` is active, then that line's own `HSYNC_d` rise |
| Raw HSYNC ending exactly at the line start requests at entry (screens 19–20 on an original Plus; CRTC3 demo) | `HSYNC_d` is high for one more character, and the live line already matches |
| R2=63 gives two (probe 09) | Line 7's HSYNC starts at its C0 63; `line_d` still reads 7 when `HSYNC_d` rises at line 8 C0 0 |
| Same-value writes never re-request after ACK (FF2) | The level is unchanged, so there is no edge |

The first version (`abf0b2d`) compared `line_d` only. It matched every photographed screen but
omitted the exact-end request, which broke three CRTC3 demo scenes; the live-line term restores
it without changing any photographed count. Line-entry requests land at C0 0, as in the earlier
three-term model.

## MiSTer validation (2026-09-27)

6128+/Full-sync CFG, each probe screen booted from its own start CPR. Captures: main checkout
`local/task-archives/plus-hw-probes-2026-09-27/mister/` (`run.py`, `measure.py`, one folder per
build).

- **`Amstrad_20260927_1c85bee.rbf`** (live-line term; hosted full build, setup slack +0.594 ns,
  TNS 0) is current. Probes 01, 04, 09, 11 and 15 match the original-Plus photographs, 06–08
  are byte-identical to master `fc1faaa` (line entry back at C0 0), and 19–20 read `02` as
  the original Plus and AmSpirit do. The CRTC3 demo plasma starts on a full row with no partial green line, the
  spheres are coherent and the Wolverine image is clean. Eerie Forest, FF2 attract and gameplay
  start, Copter 271 and the CRTC3 opening scenes look normal.
- **`Amstrad_20260927_d6618f4.rbf`** (`line_d`-only comparator) matched every photographed
  probe screen but broke the CRTC3 demo's plasma (partial green row above the effect, 155 px
  from x=613), sphere (rectangular fragments) and Wolverine (horizontal corruption) scenes,
  all R2=50, R3=14. Its short title captures missed those scenes; the user found them by hand.

## Recording a probe result

Add the photograph path (ignored media, main checkout `local/`), the machine model and the
observed outcome to the row. Then do one of three things:

- Promote the confidence.
- Open an RTL finding with a failing vector derived from the photograph.
- Revise the mechanism.

Update the owner's comment and [asic-reference.md](references/asic-reference.md) in the same
change.
