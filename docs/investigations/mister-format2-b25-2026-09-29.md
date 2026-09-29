# MiSTer acceptance: delivered format-2 RBF and B25, 2026-09-29

## Artifact and execution boundary

This device session tests the delivered `Amstrad_20260929_cec641c.rbf`, SHA-256
`40f7d66bfcfda37ce9f5053ff3e3af833713bdb4065fe4bcde9c311a3cb4dd2c`.
The local file matches the [integration record](overnight-integration-2026-09-29.md)
and each runner pins the device file hash before loading. Task base `9ddbf74`
has no later changes to `rtl/`, `Amstrad.sv` or `files.qip` relative to `cec641c`.
This evidence does not extend the user's separate `4a44394` title acceptance.

B25 DSK SHA-256 is
`90111197e72fdf424ff3b4fdb2311da7b08d5dbf19d77a6b580c31678a3b4db5`;
SHAKER 2.6 DSK SHA-256 is
`f7082f8eab521d632c343a288f54038af6df090c59b372e0d2866269c2cc4d08`.
Both programs boot through AMSDOS using the existing CSL runner and French
keyboard translation. No snapshot launch or RAM injection is used.

Initial core name was `Amstrad`; that name does not identify its bitstream.
Initial CFG was `00004000040000000000000000000000` (Plus selected), SHA-256
`13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
The temporary Classic base clears Plus bits 34:33 and CPU-fast bit 6, preserving
unrelated settings, SHA-256
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
Each run selects CPC 6128 and its named CRTC/filter through CFG. The runner
restores that temporary base; session cleanup separately restores the original.

The kernel still reports `mem=511M memmap=513M$511M`, System RAM ending at
`0x1fefffff`. The SSM ring at `0x30000000` is outside that range and the documented
framework scaler buffer. Pinned MBC was rebuilt locally because `/tmp` held no
copy; its SHA-256 matches the documented
`0e99082bb8c9b8b2c2b6581c736dfc5d701a977a9c76fa548565cbc6642984db`.

## B25 numeric evidence

All values are hexadecimal observations, not original-CPC expectations. The
[diagnostic contract](b25-diagnostic.md) defines PC as the captured return address
minus `4000`; HL independently counts INC/ADD. Full mode is used once per CRTC.

CRTC0 and CRTC1 complete table (all eight trials, identical in both captures):

| Group / value | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| NOP PC | 0CE2 | 0CE2 | 0CE2 | 0CE2 | 0CE2 | 0CE2 | 0CE2 | 0CE2 |
| NOP HL | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 |
| HALT PC | 0001 | 0001 | 0001 | 0001 | 0001 | 0001 | 0001 | 0001 |
| HALT HL | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 |
| RET NC pad0 PC | 0672 | 0672 | 0672 | 0672 | 0672 | 0672 | 0672 | 0672 |
| RET NC pad0 HL | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 |
| RET NC pad1 PC | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 |
| RET NC pad1 HL | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 | 0000 |
| INC HL pad0 PC | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 |
| INC HL pad0 HL | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 |
| INC HL pad1 PC | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 |
| INC HL pad1 HL | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 | 0671 |
| ADD HL,DE pad0 PC | 044C | 044C | 044C | 044C | 044C | 044C | 044C | 044C |
| ADD HL,DE pad0 HL | 044C | 044C | 044C | 044C | 044C | 044C | 044C | 044C |
| ADD HL,DE pad1 PC | 044B | 044B | 044B | 044B | 044B | 044B | 044B | 044B |
| ADD HL,DE pad1 HL | 044B | 044B | 044B | 044B | 044B | 044B | 044B | 044B |
| ADD HL,DE pad2 PC | 044B | 044B | 044B | 044B | 044B | 044B | 044B | 044B |
| ADD HL,DE pad2 HL | 044B | 044B | 044B | 044B | 044B | 044B | 044B | 044B |

Both CRTC types produced byte-identical 768×273 PNGs, SHA-256
`ea0081dca5b0e279b2122b003985afa98f3f5caa3d4d7b451b577b16e8103677`.
These values match the production fixture. In particular INC pad0
is `0671`, whereas the recorded AmSpirit result is `0672`; this comparison
does not determine the original CPC result.

## Format-2 observations and bounded display comparison

A short derived CSL case boots SHAKER 2.6 module A, selects test 1 and stops
on its first screen, marker `0001`. This ordinary instruction screen provides
a repeatable three-mode observation/geometry check without repeating the B25
numeric suite three times. It is not the B22 short-HSYNC startup program and
cannot close B22's acquisition residual.

Each controlled reload observed a zero-written header before input (one startup
attempt), then consumed sequence 0 in slot 0 through the existing header/ring/
header coherence reader. All record headers report format 2, 64 entries,
written=1, dropped=0, reader_lost=0, reader_restarts=0 and margin=63 slots.
Raw command outputs and decoded records remain in each manifest.

| Mode | Requested/applied | Frame | Line | Hpos | Tick | Native PNG |
|---|---|---:|---:|---:|---:|---|
| full | 0/0 | 1598 | 254 | 177 | 2042921664 | 768×273 |
| raw-pixels | 1/1 | 1594 | 254 | 177 | 2037809826 | 768×273 |
| raw-crt | 2/2 | 1590 | 254 | 181 | 2032698046 | 800×287 |

All three marker observations have `line_estimate_ce4=256`, `hs4=false`,
`shift=false`, `no_hsync=false`, `hsync_mask=true`, `arm=false`, and
`training_syncs=3`. This is marker-time state, not a lock verdict or a history
of the interval before the marker. These steady states do not exercise pending
requested/applied transitions or prove the simulation's fetch-boundary sampling
cut independently on the device. Run-wide applied configuration fields remain
null as designed; the observed applied modes belong to their marker records.

Full and Raw-pixels PNGs are byte-identical, SHA-256
`cc1b5bc45076a162b7e62b818986e6790ab3086694d4aa57379fe40db004e634`.
Raw CRT SHA-256 is
`c37c99652a7fee4a8aaa99428e5d39d920219133b61bf43b8e9aa92e7b872468`.
Visual inspection shows the same type-1 A1 instruction screen with a larger
border/frame in Raw CRT. Native Main PNGs are asynchronous scaler-DDR reads;
their age and physical HDMI/analogue connector timing are not measured. There
is no physical CTM or original-CPC claim, no filter-lock claim, and no acquisition
policy change.

## Restoration, retained evidence and acceptance

All five runners exited successfully and verified restoration to the temporary
Classic base. Session cleanup copied the original CFG back, verified its original
`13ef32c7…` hash, loaded `/media/fat/menu.rbf`, read back `MENU`, and removed
`/tmp/mbc-task1`. The coordinator and task 3 received explicit device release.
The uniquely named delivered RBF and B25 DSK remain on the device; the existing
`4a44394` RBF is untouched. The initially running bitstream could not be identified
from `CORENAME`, so returning to MENU does not claim restoration of its prior
running program.

Local evidence is under `docs/references/mister-format2-b25-2026-09-29/`
(`docs/references` resolves to `docs/specs`). Its directory-local ignore file
keeps screenshots, CFG backups, manifests, case scripts and tools untracked.
`output_files/mister-format2-b25-2026-09-29.tar.gz` packages the five completed
runs, three executed CSL cases, original/temporary CFG, preflight and restoration
logs; downloaded compiler files and unused retry case are excluded. The pinned
MBC source/binary remain in the local evidence directory.

This is documentation-only: no RTL, host tool, harness or test changes.
Simulation selection and cross-provider code review are therefore skipped under
the repository's documentation exception. Verification consisted of artifact
hash pins, five completed runner manifests, controlled ring startup/coherence,
PNG inspection and byte comparison, and verified CFG/MENU restoration.
B25 rerun input, original hardware phase, pathological sync acquisition and
live mode transition acceptance remain outside this bounded session.
