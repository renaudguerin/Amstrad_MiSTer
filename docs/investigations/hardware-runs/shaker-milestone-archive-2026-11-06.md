# Current milestone SHAKER archive — 6 November 2026

The serial A–E × CRTC0/1/3 refresh is complete on the exact `fdf348c` milestone
RBF. The [release draft](https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-992c9d8a468de299083c)
remains unpublished. Its packaged RBF is byte-identical to the installed trial:
SHA-256 `17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`.
The current 2.7 disk and French system cartridge hashes are verified; Full is
requested and format-2 SSM records expose the actual mode per event.

## Checkpoint

| Cell | Native captures | Capture failures | State |
|---|---:|---:|---|
| CRTC0 A | 122 / 122 expected | 0 | Complete; hashes/dimensions and inventory checked |
| CRTC0 B | 220 / 221 expected | 1 | Complete; 019A produces a native-capture timeout |
| CRTC0 C | 22 / 22 expected | 0 | Complete on recovery attempt 002 |
| CRTC0 D | 110 / 111 expected | 1 | Complete; 02EA timeout, canonical D3 replaced by correct44 |
| CRTC0 E | 11 / 11 expected | 0 | Complete; inventory/contact-sheet inspection done |
| CRTC1 A | 127 / 127 expected | 0 | Complete; inventory/contact-sheet inspection done |
| CRTC1 B | 139 / 139 expected | 0 | Complete; inventory/contact-sheet triage done |
| CRTC1 C | 80 / 80 expected | 0 | Complete; inventory/contact-sheet triage done |
| CRTC1 D | 127 / 127 expected | 0 | Complete; canonical D3 excluded, prior correct44 reused |
| CRTC1 E | 7 / 7 expected | 0 | Complete; inventory/contact-sheet inspection done |
| CRTC3 A | 123 / 124 indexed | 0 | Complete; 03A7 is absent from this canonical script |
| CRTC3 B | 121 / 121 expected | 0 | Complete; inventory/contact sheets and three numeric pages inspected |
| CRTC3 C | 23 / 23 expected | 0 | Complete; inventory/contact-sheet triage done |
| CRTC0/3 manual D3 | 44 each | 0 | Complete; all88headers and whole-image contacts inspected |
| CRTC3 D | 150 / 150 expected | 0 | Complete on attempt 003; canonical D3 replaced by correct 44 |
| CRTC3 E | 5 / 5 expected | 0 | Complete; inventory/contact-sheet inspection done |

Canonical D3 names cannot be used as page identities on this asynchronous host
path: CRTC0 `0314` and `0318` both visibly show `R2=#27/R3=#06`, rather than
35/06 and 0B/06. Preserve the entire raw run but exclude canonical 0314–033F
from archive selection. Count-corrected manual overlays now supply all44verified parameter states
for CRTC0/1/3. This is the host's departure-marker
retrieval limitation, not an author-package defect or a new CRTC finding.

Fresh captures do not confer passing grades. CRTC0 A's 116 old stale entries
now have current evidence and become unknown; a further 65 non-D3 module-D
entries make the same transition after inventory and full contact-sheet review; its historical passing judgment
remains qualified as such. Four formerly stale CRTC1 D pages also become unknown after current acquisition
and inspection. The earlier provisional count included0305, whose full-resolution
physical/native comparison instead identifies a returned menu;0305 remains
stale on CRTC0/1/3 and is excluded pending a correctly bound DT/J8 frame. The source/tally retains page-level status separately
from archive acquisition. Five A7 images, 0081–0085, are identical and visibly
cropped compared with the physical reference; their content/acquisition boundary
needs further adjudication. Their distinct SSM records show advancing frames
and actual Full mode, so they are preserved as current unknown evidence rather
than substituted or labelled passing. AP/009C intentionally lacks a text header;
the current and original-CPC images both show the three dashed-row family.
Its missing header alone is not a capture failure or a whole-page pass.

All original runs and alternative pictures remain private. The archive selects one correctly bound image per code, retain failures/ambiguities in an
index, and exclude reference photographs, ROMs, disks and private machine state.
The existing 44 correctly bound exact-RBF D3 captures can be reused. The 1,390
page ledger is an inventory to reconcile, not a promise that every page produces
a native frame. Failed/no-frame captures will remain explicit; no substitute
image will be presented as a current machine capture.

## SSH recovery and current private state

A/B complete, then C attempt 001 fails authentication before device actions.
The task's Codex-forwarded SSH socket has disappeared; a clean three-attempt
retry confirms the failure. The GUI login agent, discovered from the current
user's launchd domain, has a live socket and authenticates normally. The batch
uses that socket only in its process environment, without key extraction or
global configuration changes. C attempt 002 succeeds and the serial run resumes.
The initial failed manifest, console, cell plan and recovery receipt are retained.

Live state and exact commands are under
`local/overnight-20261103/resumed-20261104/milestone-release-20261106/`:
`CELLS.json`, `BATCH.json`, per-cell manifests, `SSH-RECOVERY.json`, inspection
reports and release packaging. `STOP-AFTER-CELL` stops safely at a cell boundary.
Root remains the sole device operator. No archive or message has been sent to
Longshot, and no release has been published.

CRTC3 A/B capture hashes/dimensions and all440 actual Full0 observations pass;
CFG restoration is verified. Root viewed all244 images on14 contact sheets.
B6's complete six-check page passes; BR01F0/01F1 are confirmed current-program
failures after caller/table qualification.
The fresh images replace old stale evidence with current unknown judgments,
without asserting a full hardware comparison. Missing03A7 remains an explicit
archive inventory gap rather than a generated placeholder. After targeted S
and corrected D3 controls, the batch resumed; D preflight authentication failed
without device actions, then a clean task-local GUI-agent SSH check succeeded.
The retry pins that agent only in its process environment. D attempt 003 and E completed successfully. All 15 module/CRTC cells are now complete.

## Completed archive

The package selects **1,384 native PNGs from 1,390 indexed pages**. The six
unavailable entries are CRTC0 B/019A and D/02EA (capture timeouts), CRTC3
A/03A7 (not emitted by the canonical script), and DT/J8 0305 on each type
(returned-menu images excluded). All 132 D3 images use the correctly bound
manual states. There are no pending acquisition entries.

The ZIP contains native images, the full page index with separate grade/build/
evidence columns, coverage, README and SHA-256 inventory. It contains no ROM,
disk, snapshot or original-CPC photograph. Current acquisition does not upgrade
a historical grade or an unknown page into a current pass.

A [small paired official-core comparison](shaker-upstream-comparison-2026-11-06.md)
confirms host-driven fixed-wait captures are practical without upstream CSL/SSM.
That method is suitable for selected controls, with weaker timing/identity
assurance than instrumented acquisition; it is not a full upstream baseline.

The verified 12,488,190-byte `SHAKER_fdf348c_20261106.zip` is attached to the
unpublished draft. SHA-256:
`5154ced13340417c71f5af39f6c628b1ca7eb26e6bc03eb320a05bf4f696d080`.
Its 1,387 data-file hashes match the archive inventory.
