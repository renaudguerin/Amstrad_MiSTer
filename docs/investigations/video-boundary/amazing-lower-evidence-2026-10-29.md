# Amazing2021 lower-screen corruption: V3 evidence

2026-10-29; task `codex/accuracy/amazing-lower-corruption`, integration base
`040f57df2d0a97b98c7103db1f0a035a108a01d0`. This is an evidence handoff, not a
repair. The lower-screen defect reproduces on the current production RTL in
both independent Full-mode boots. No CRTC/GA cause is established and no
production code changed. The upper effect remains V1's separate investigation.

## Matched reproduction

Sole device/shared-AmSpirit operator A collected the captures; V3 performed only
offline inspection. Both Full boots used classic CPC6128, Plus off, CRTC1 and
`RUN"AMAZING`, after applying the test CFG and loading the core afresh.

- Disk `/media/fat/games/Amstrad/dsk/amazing2021_rev2.dsk`, SHA-256
  `4a631dabab9d8b9a916620a0e0b78f634daeae7736aee3c06b6c0e9979f1e8c9`.
- RBF `Amstrad_20261027_925b5bb.rbf`, SHA-256
  `9ba5e34be1bd6d0eba4d9dcf6a46b686648b6abacef1b093903252d72aed77eb`.
  `git log 925b5bb..040f57d -- rtl Amstrad.sv files.qip` has no production
  changes; this is a reproduction against current production source.
- Full is established by applied CFG, not a runtime mode-latch observation.
  The native screenshots are 768x273 scaler-buffer captures; they do not
  observe analog output or raw CRTC/GA signals.
- Applied Full CFG SHA-256:
  `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
  Original/restored CFG SHA-256:
  `f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`.
  A restored the original CFG and verified MENU after each Full batch.
  The manifests retain file identities, on-disk MiSTer.ini and command logs;
  the INI snapshot does not prove every active framework setting.

All twelve PNGs were inspected. The WELCOME header remains legible in every
sample. The lower region changes between coherent content and dense repeated
fragments matching the historical reported corruption:

| Boot / run ID | c1 | c2 | c3 | c4 | c5 | c6 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 / `1793279331_901e2a` | paragraph readable | corrupt | corrupt | QR readable | corrupt | corrupt |
| 2 / `1793279526_0b1f5e` | paragraph readable | corrupt | corrupt | corrupt | credits readable | credits readable |

These are sampled states, not a measured flicker frequency or corruption duty
cycle. Actual screenshot-request offsets after the input command completed:
boot1 `30.319,31.985,45.281,46.743,60.300,61.581` seconds; boot2
`30.297,31.594,45.322,46.715,60.320,61.600`. The input command includes about
one second of post-sequence waiting. Final Return and scaler sampling were not
independently timestamped, so do not label these exact execution times.

Changing upper geometry and lower content demonstrate progress between samples;
they exclude a wholly unchanged captured frame. They do not distinguish wrong
RAM contents, wrong fetch addresses, GA decoding or presentation. Two earlier
bad-command batches (`amazing-full`) stopped at BASIC because of French-keyboard
mapping errors and are excluded. A owns the separate host-helper repair.

## AmSpirit comparator and source narrowing

A's uninterrupted same-media run on Lite1.16/core2491682 used effective
6128/CRTC1 settings, confirmed by `config.json` and `state.json`. All six
30/45/60-second and adjacent captures have coherent lower paragraph, QR or
credits content. Horizontal coloured bands are present and are not classified
as V3 corruption. Elapsed time only approximately aligns these runs; paragraph,
QR and credits provide the useful scene correspondence. AmSpirit is a diagnostic
comparator, not original-CPC proof.

The checkpoint SNA has SHA-256
`67fe958c4aca7192d06f72dbfef484c905c16f558143d6d9376b2a80e0c5eda9`, version2,
128K standard RAM and an emulator-specific SPRT chunk. Byte A4 is unspecified
in version2; its zero does not contradict the effective CRTC1 configuration.
CPU PC is `92E6`, RAM configuration `C2`. Private RAM disassembly and history
identify a useful lower-effect entry: `C049` programs R4=0; the following setup
programs R9=8 and jumps to `C234`; that entry writes R9=0, sets SP=`4600` and
IX=`3F40`, then enters a generated/table-driven chain. Do not substitute the
alternate `C21F` routine for this selected entry or assume a cold-entry replay
preserves the preceding top effect and frame phase.

Current production source separates the relevant boundaries:

- `Amstrad_MMU.v` maps CPU writes through RAMmap/RAMpage; the display port in
  `Amstrad.sv` fixes physical base64K at `0x20000` within the selected machine
  bank. A write into extended RAM alone is not evidence of a mapping defect.
- `Amstrad_motherboard.v` composes the word address from
  `{MA[13:12],RA[2:0],MA[9:0]}`. The corresponding byte offset is
  `((MA&0x3000)<<2)|((RA&7)<<11)|((MA&0x3ff)<<1)`.
- Full's SHIFT compensation acts on bytes before the GA. Thus a GA RGB tap in
  Full is already affected by presentation policy; label it accordingly.
  A mode comparison alone cannot assign causality.

French ACCC v1.11 §12.3 p96 and §17.4.2 p184 describe type1 line-to-line
rupture and row-zero address reload. §20.3.2 p242 explicitly reloads VMA whenever
C0 returns to zero while C4=0, independently of C9; §20.5 p244 describes the
14-bit pointer carry and fixed C9 address bits. These pages were extracted with
pdf-inspector; p242/p244 diagrams were rendered and inspected. They guide the
next expected-address derivation, but no observed title transaction has yet
been shown to violate them.

## Excluded replay and next decisive unit

An ignored adaptation of the existing B18 snapshot fixture built with production
T80pa/GA/CRTC/MMU and ideal CPU/video RAM. It adds passive register, MA/RA,
fetch-byte and native-GA-colour observations. It omits SDRAM arbitration/cache,
firmware/FDC boot and mixer/scaler. It does not restore hidden classic CRTC/GA
phase. Continuations of 4,000,000 and 40,000,000 master ticks did not establish
trustworthy title reacquisition; the latter raw VS edges at sample indices
`1154160,5319792,8850544` (one sample/four master clocks) are not a stable
50Hz replay. The rendered diagnostic is not the title screen and is excluded
from causal conclusions. This is not fail-first proof of a production bug.

A bounded FPGA OSD snapshot attempt timed out without a new generation. No
MiSTer RAM/CPU state at a corrupt frame was obtained; no further blind OSD
attempt was made. Consequently there is no matched first-wrong-fetch trace.

The recommended next unit is an authentic caller/table replay that includes
the lower entry's preceding timing setup, or a matched device trace/save with
known trigger observation. Retain CPU physical writes, R4/R9/R12/R13 acceptance
phase, MA/RA and actual fetched bytes across one readable-to-corrupt transition;
compare native byte policy and Full separately. Derive expected addresses from
the French rule before touching RTL. A standalone snapshot at `92E6` is not a
sufficient replay contract. Do not make V3 depend on V1's redesign or assume a
shared cause with DSC4.

## Artifacts and acceptance

Private evidence is ignored under
`local/investigations/amazing-lower-2026-10-29-v3/`; A's complete capture bundles
are also retained under `local/investigations/classic-video-v2-2026-10-29-b157/`.
The V3 README records build/run commands, trace format and limitations. Preserve
media/SNA/disassembly/captures locally. No copyrighted payload is committed.

This documentation-only unit needs no simulation selection, soak remint or code
review. The diagnostic build and exploratory runs do not count as behavior gates.
No fix, synthesis, hardware repair acceptance, integration or push is claimed.
At the stop checkpoint shared Codex usage was 89% of the five-hour window and 47%
of weekly; finish this evidence unit and start no new investigation. No reset,
paid overage or wakeup was requested or used. V3 is READY as an evidence handoff;
the defect and first-wrong-fetch discriminator remain open.
