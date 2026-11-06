# SHAKER D3: private manual-script count correction

All 44 CRTC1 D3 captures now have the expected R2/R3 parameters on trial
`fdf348c`. A single extra Space in our private manual overlay caused the
remaining offset. The author’s canonical scripts and the original guest binary
are unchanged. This closes capture alignment. Digital-content adjudication and
the separate monitor-response project remain open.

## Defect and independent count proof

The original classic R2 target table occupies SHAKE27D addresses 5AC4–5AEF:
four groups of 11 values, with R3=6/5/4/3. The guest advances R2 through
35..3F, 00..27, then enters the next handler. Its source and the photographs
bind `0328` to R2=26/R3=5 and `0329` to R2=27/R3=5.

Our private `SHAKER-D-1-D3-manual-100ms.CSL` sends two Spaces at line 96,
from R2=26. The first enters 27; the second leaves that handler and enters
R2=35/R3=4. Therefore `0329` already shows the following group. The older
report’s “first 22 headers” statement was wrong: only the first 21 match.
Held-key sampling across a handler remains possible in the source, but it
is not needed to explain this exact offset.

Changing only that sequence from two Spaces to one makes the input counts
agree with all 44 target-table bytes. The unchanged overlay has 23 wrong states
from `0329` onward; the corrected copy has none. Symmetric 100 ms timing,
all other key strings, waits and screenshot ordering are retained. A proposed
5 ms pulse experiment was abandoned before sending any pulse because inspection
exposed this simpler count defect.

## Native verification

One unchanged-prefix run completes 187 commands/22 captures and reproduces the
wrong final state. One corrected targeted D3 run completes 187 commands/44
captures with zero capture failures. The parent reads every header against the
immutable target table and views all 44 complete images. `0329` is now 27/05,
`032A` is 35/04, and `033F` remains 27/03 rather than the menu. No SSM exit
capture, CPU-state patch, image tolerance or timing change is used.

Inputs are the original SHAKER 2.7 disk SHA256
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`
and extracted SHAKE27D payload SHA256
`65a90825dc674953b5dd9d4918bddf8079ea44877d89b08542b7fabb6b8d5ec5`.
Native RBF SHA256 is
`17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`.
The repaired MBC hash and Full-filter request are recorded in the manifests.
Firmware is the ordinary French classic boot, CRTC1; no Plus cartridge is used
for the D3 walk.

The corrected overlay is installed separately at
`local/test_media/shaker/CSL_MiSTer/MODULE D/SHAKER-D-1-D3-manual-count-fixed-100ms.CSL`.
Use it with the existing CSL runner **without `--ssm`**, Full filtering and
the pinned disk. Older private inputs remain available. Native capture is
before page advance; the canonical synchronous SSM script serves a different
capture path and needs no corresponding edit.

## Tally and remaining display scope

The 44 pages move from stale to unknown in the maintained ledger. They now
have fresh correctly bound captures; the complete digital-content/blanking
comparison has not been adjudicated. Parameters alone do not establish all
surviving digits, raw blanking or content outside the captured viewport.

The CRT deflection/shear is a separate display response. Full and Raw pixels
use regularized acquisition; native framebuffer pictures do not photograph a
physical CRT’s beam trajectory. **Missing curvature alone is not a CRTC/GA
failure and must not block functional page acceptance.** A digital behavioural
monitor model could reproduce the response; it does not require a component-
level circuit simulation. Raw CRT may deliver the waveform to an actual CRT,
subject to the output/framework contract, but its native PNG is still a buffer
capture. See [V1/B1](../../backlog.md#b1-crt_filter-owns-the-sync-and-blanking-geometry-and-discards-the-crtcs)
and the [response candidate](../video-boundary/v1-shaker-caller-response-2026-10-29.md).

Use these current inputs for source-content comparison and V1 response work;
keep those verdicts separate instead of repeating an input-timing sweep or
forcing CRT curves into CRTC rules.

A subsequent Terra-high worksheet covers all 44 correctly bound physical/native
pairs. The comparable ruler/cutout families agree broadly and it identifies no
concrete missing-content discrepancy. Compressed/overlapping transition rows
and some screen-edge boundaries prevent a complete glyph comparison; these
are evidence limits, not failing tests. Root checks the report and the 0339
interior-hole pair at original resolution. The private per-page worksheet is
under `digital-content-adjudication/`; it supplies no new whole-page grades.

A subsequent Gemini 3.8 high diagnosis inspects ten representative physical/
current pairs and identifies no concrete missing-content discrepancy. Root
spot-checks five pairs and confirms the broad surviving-digit/cutout families.
This is limited corroboration: exact-width and exposure/viewport claims in
the provisional report exceed the evidence and are not adopted. It proves
neither raw counter execution nor a filter root cause. The parent assessment
is retained under `photo-discriminator/`; all 44 page grades remain unknown.

For author feedback: this count error is in our private MiSTer overlay, not in
Longshot’s canonical scripts. The earlier bulk-SSM retrieval limitation is also
a host acquisition limitation; neither is evidence of an author-package defect.

Private proof, commands, hashes, all originals, contact sheets and parent
acceptance are under
`local/overnight-20261103/resumed-20261104/d3-boundary-short-pulse-20261106/`.
MiSTer is restored to the same `fdf348c` French Plus BASIC/BTF-ready state;
the restoration completes 7 commands/one capture, viewed by the parent.

## Current CRTC0 and ASIC archive overlays

Count-corrected quiescent overlays now also complete on CRTC0 and CRTC3 with
44captures each,187commands each, no capture failures and verified CFG
restoration. They use the same exactfdf348cRBF/disk pins as the accepted CRTC1
run; only the declared type and image labels differ. Root read every one of
the88 R2/R3 headers against the original44-entry parameter table and viewed
all88 complete images on12 contact sheets. No bulk exit-marker image is used.

The capture identities are now current unknowns, rather than stale wrong-page
images. This closes acquisition identity only; digital-content and physical
CRT response are still separate acceptance work. Requested Full is applied
through verified CFG; the manual runs have no per-marker mode observations.
Private manifests, hashes, header/contact sheets and parent receipts are in
`resumed-20261104/milestone-release-20261106/{captures,inspection}/`
under `crtc0-D3-corrected-001`/`crtc3-D3-corrected-001` and their inspection
counterparts. These are native captures, not physical-display validation.
