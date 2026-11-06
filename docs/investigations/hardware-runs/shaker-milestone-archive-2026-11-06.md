# Current milestone SHAKER archive — 6 November 2026

A serial A–E × CRTC0/1/3 refresh is running on the exact `fdf348c` milestone
RBF. The [release draft](https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-c15fe46775a05244113b)
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
| CRTC0 D | 110 / 111 expected | 1 | Complete; 02EA timeout and 44 D3 identities excluded |
| CRTC0 E | 11 / 11 expected | 0 | Complete; inventory/contact-sheet inspection done |
| CRTC1 A | 127 / 127 expected | 0 | Complete; inventory/contact-sheet inspection done |
| CRTC1 B | 139 / 139 expected | 0 | Complete; inventory/visual triage in progress |
| CRTC1 C | 80 / 80 expected | 0 | Complete; inventory/visual triage in progress |
| CRTC1 D | 127 / 127 expected | 0 | Complete; canonical D3 excluded, prior correct44 reused |
| CRTC1 E | 7 / 7 expected | 0 | Complete; inventory/contact-sheet inspection done |
| Remaining cells | In progress | Pending | Serial batch, not a completed baseline |

Canonical D3 names cannot be used as page identities on this asynchronous host
path: CRTC0 `0314` and `0318` both visibly show `R2=#27/R3=#06`, rather than
35/06 and 0B/06. Preserve the entire raw run but exclude canonical 0314–033F
from archive selection. Count-corrected CRTC0/3 manual overlays are parser-
qualified and await serial device access. This is the host's departure-marker
retrieval limitation, not an author-package defect or a new CRTC finding.

Fresh captures do not confer passing grades. CRTC0 A's 116 old stale entries
now have current evidence and become unknown; a further 65 non-D3 module-D
entries make the same transition after inventory and full contact-sheet review; its historical passing judgment
remains qualified as such. Four formerly stale CRTC1 D pages also become unknown after current acquisition
and inspection. The earlier provisional count included0305, whose full-resolution
physical/native comparison instead identifies a returned menu;0305 remains
stale on both CRTC0/1 and is excluded pending a correctly bound DT/J8 frame. The source/tally retains page-level status separately
from archive acquisition. Five A7 images, 0081–0085, are identical and visibly
cropped compared with the physical reference; their content/acquisition boundary
needs further adjudication. Their distinct SSM records show advancing frames
and actual Full mode, so they are preserved as current unknown evidence rather
than substituted or labelled passing. AP/009C intentionally lacks a text header;
the current and original-CPC images both show the three dashed-row family.
Its missing header alone is not a capture failure or a whole-page pass.

All original runs and alternative pictures remain private. The archive will
select one correctly bound image per code, retain failures/ambiguities in an
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
