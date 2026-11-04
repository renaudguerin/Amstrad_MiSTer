# CPCSyntaxError v0.12.0 toolbelt pilot — 4 November 2026

## Position

The supplied matching macOS universal binary is usable directly for headless
CSL/SSM. Fresh captures replace older portal emulator images for this version.
The active [guide](../../cpcsyntaxerror-testing.md) and
[cpcse-test skill](../../../.agents/skills/cpcse-test/SKILL.md) describe the
workflow: establish a hardware match first, then inspect the relevant source
where MiSTer differs. AmSpirit remains the additional reference for cases
CPCSyntaxError fails or cannot resolve.

The original GUI remains available; these batches used a task-local package
copy to avoid sharing the app's settings with that GUI. No interactive command
API or GUI control was needed or exercised. AmSpirit remains paused on its
clean original DSC4 credit boot.

## Identity and exact inputs

- User package: main checkout `local/CPCSyntaxError-macos-universal-v0.12.0/`;
  copied unchanged into the same ignored task-local path.
- Source: user checked out v0.12.0 at `~/code/CPCSyntaxError`; parent verified
  clean commit `22de765ef44da46c55217e4dd7caa385d69a6045`.
- Universal executable SHA-256:
  `f4254653b11676055eedd89dcdae6c6aab381835344d69780f56fcbff218a577`.
- Bundled client SHA-256:
  `17632f30de36ae57616ee72d5936aa36dec8792403f71c2c19598f14afeec8bc`.
- Disk: new canonical shaker27.dsk, SHA-256
  `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
- `--no-chain --no-errata`, explicit `--roms` pointing into that package,
  and explicit model/CRTC for each run. The default A-U wait adjustment was
  therefore disabled, without editing canonical author scripts.
- Plus logs identify the packaged English Burning Rubber system cartridge.
  MiSTer comparisons use the separately pinned French system cartridge; do
  not silently describe these firmware environments as identical.

## Executed batches

| Batch | Scope and result | Evidence below task-local cpcse/ |
|---|---|---|
| BI / CRTC3 | Canonical B-3 boot and full I section, direct entry; eight captures, exit0 | bi3-v012/, bi3-v012.log |
| C-1 | Full canonical history;80 captures, exit0 | c1-v012/, c1-v012.log |
| AE/AR / CRTC3 | Canonical A-3 boot then E/R sections, direct entry; seven captures, exit0 | ae-ar3-v012/, ae-ar3-v012.log |

The AE/AR batch's measured wall time was **1.639 seconds**; its log reaches
21.36 seconds of emulated time. Invocation, exit and timing are in
`ae-ar3-v012-run.json`. The other batches were not separately wall-timed;
no timing is inferred from when the parent later polled their processes.

All 95 captures are execution coverage, not95 correctness passes. The full
C-1 pixel/photo audit and exact AE/AR transition/geometry adjudication are not complete.

## Hardware-backed leads already checked

### BI's short-pulse mode

Parent inspected all eight fresh BI images through `bi3-contact.png`, and
compared the characteristic wide/thin/thicker dark-band pattern with the
original-Plus photographs and AmSpirit. At BI/B (`01DF`, R3=1, OUTC), the
original Plus photograph shows white right-hand LOGON text. CPCSyntaxError
preserves that white mode; the MiSTer baseline changes mode and colors it.
This is a concrete observable match, not a claim of pixel-identical camera
geometry or a proven exact type3 transition dot.

The matching source's `GateArray::onHsyncStart()` rejects mode latching
when width<2; `onHsyncCutShort()` also distinguishes the short-pulse mode
rule. French §9.3.1 p.52 independently requires two microseconds minimum
for every CRTC type. The independently developed
[ASIC mode-latch repair](asic-mode-latch-2026-11-04.md) has a failure-first
GA→decoder vector for that contract; the source is corroboration, not its
sole oracle. Original BI photos are retained with URLs/hashes in
`asic/b3-r3jit-real-plus-ref/index.json`.

The rejected bus qualifier is a separate counter failure: it changes the
six short native bands into224pixel blocks. The authentic
[production-T80 replay](shaker-bi-r3-counter-consumption-2026-11-04.md)
locates its R3 admission after the count decision. Do not conflate that
rejected transport change with the new pixel-mode event.

### C E/P/S

Fresh full-history CPCSyntaxError E (`02C9`) prints **#0080** for R5=3
on the second adjustment line, matching the original-CPC photograph.
MiSTer on both new full-history captures still prints **#0060**. Other
deadlock values73/77/4A0/B20/223 match between these runs. This is a
useful focused source investigation; it does not validate CPCSyntaxError's
whole R5 implementation, whose re-entry route differs from author Q20.
The [source audit](emulator-source-author-question-audit-2026-11-03.md)
records that conflict. A subsequent bounded
[source comparison](shaker-e-0080-positive-r5-reentry-2026-11-04.md) identifies
positive-R5 re-entry as a useful discriminator: restarting C5 predicts the
eight physical lines behind 0060/0080. The actual aligned CPCSyntaxError
C5 trace is missing, so this remains a hypothesis.

P (`02CB`) is readable and prints C4=#26 as expected. S (`02D3`) reads
**#4E40** in the fresh full C-1 walk, as do the two new MiSTer runs.
The earlier native-scale transcription0240 was corrected from enlarged
crops; do not propagate it. Original/photo evidence for E's second page
`02CA` is still needed before treating that page's detailed state as a
hardware oracle.

### AE/AR reference inspection

Six original-Plus photographs were retrieved for AE008C and AR00AF/00B5/
00B6/00B7/00BC; no original-Plus00B4 image is listed in that reference set.
The parent viewed all seven fresh BMPs and all six photographs. AE has the
expected type3 readable marker09. AR retains the corresponding yellow fine
stripes, upper color bars and black lower region across the pattern families.
CRT shear and the exact transition columns were not scored as equal. These
screens are useful additional mode/monitor discriminators for the upcoming
MiSTer build; this inspection is not seven complete test passes.

Original URLs, hashes and attribution are preserved in
`ae-ar3-real-plus-ref/index.json`, alongside a derived contact sheet. Only
original-CPC photographs were fetched; older portal CPCSyntaxError captures
were not used as a current-version baseline.

### DT: an additional monitor/GA discriminator

A direct D-1/T experiment reached 02F8 and 02FA using the unchanged D binary,
new canonical D-1 boot and the older upstream event-paced T section. The
first broader attempt ended naturally at its time cap waiting for 0300
(line 49); 17 earlier captures are preserved as partial execution, not an
emulator-accuracy verdict or a valid full D walk. The new canonical fixed-wait
D script differs, so that wait must not be reported as an author defect.

A second bounded script ends at 02FA and completes with 13 captures, exit0
(`dt1-through-fa-v012/` and adjacent log). Both target BMP hashes match the
first attempt exactly. They show two black horizontal regions, as in the
original-CPC photos; MiSTer's recorded Full-mode 02F8 shows only one. This
is a useful hardware-matching source lead. It does not assign the defect to
the CRTC, GA or sync filter without observing their actual boundary signals.
The first 95 captures above and this 13-capture batch give 108 completed
captures; no overall pass rate is inferred.

## Limits and next work

Use hardware-matching cases to investigate specific first divergences.
Do not replace author-confirmed rules with a broad emulator implementation
merely because one numeric result passes. CPCSyntaxError's I/O scheduler
uses character-level advancement, and its monitor renderer is partly
trained on SHAKER. Fine bus edges and monitor physics still need their
own evidence. The number of current-version tests that fail original
hardware has not yet been measured.

Raw BMPs, derived PNGs/contact sheets, logs, identity and width-triage data
remain ignored under `local/overnight-20261103/cpcse/`. No emulator binary,
firmware, disk or screenshot is committed.
