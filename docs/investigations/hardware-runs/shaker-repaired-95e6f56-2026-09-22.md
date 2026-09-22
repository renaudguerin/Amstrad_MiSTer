# Repaired SHAKER hardware acceptance — 2026-09-22

Build `95e6f56` fixes the previously observed B9/type1 first-page even-entry
64 µs deficit. All numeric rows on that page now match the real-CPC photograph.
The second page still differs at the C0=3F transition and middle-frame measurement.
This is partial hardware acceptance, not closure of D1/D6 or the interlace suite.

## Build and capture contract

- Exact source: `95e6f563317c1dfcc205573d4b5ad6fd2eabe8b9`.
- RBF: `Amstrad_20260922_95e6f56.rbf`; host and device SHA-256
  `a4f6a4f30758c2456a177a40a098167ab570214b311dc03149b72b72c499e0a7`.
- Coordinator-verified CI `35691075125`, all required jobs passing; full local
  Quartus 17.0.2 artifact `Amstrad-local-build-238-1-full`: setup +0.402 ns,
  hold +0.241 ns, zero TNS; 23,841 ALMs (57%).
- Device `shaker27.dsk`: SHA-256
  `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`.
- Classic CPC 6128, requested CRTC per case, Full sync filter applied by the CSL
  runner. Screen headings identify the CRTC; native PNGs do not verify the OSD.
- Task-authored timed CSL scripts, French keyboard layout; no observed SSM markers.
  Reference A/B/C/D ordering is identified from screen contents, not filename alone.
- Private evidence: `docs/screenshots/shaker-d1-d6-2026-09-22/`, including
  `repaired-b9-type1/`, `repaired-b9-type0/`, the CSL scripts and `reference/`.

## B9, type 1

Compared with real-CPC photographs
[A](https://shaker.logonsystem.eu/images/cpc/CPC/B9_CRTC1_A.webp) and
[B](https://shaker.logonsystem.eu/images/cpc/CPC/B9_CRTC1_B.webp).
Values are hexadecimal microseconds as printed. Each page was captured twice;
its repeats are pixel-identical. Independent image transcription agreed.

| State / rows | Real CPC | Repaired MiSTer | Result |
|---|---|---|---|
| A, both R7=0 blocks, entry lines 0–4 | 2740, 2760, 2780, 27A0, 27C0 | Same | Match; previous even-entry deficit fixed |
| A, nonzero-R7 block | 1820, 1840, 1860, 1880, 18A0 | Same | Match |
| A, ON/OFF controls | 43C0, 43C0, 25C0 | Same | Match |
| B, C9=0/C0=3F in each of three update-delay blocks | 2740 | 2780 | 64 µs too long |
| B, other update-delay rows | 2740, 2740, 2780, 2760 | Same | Match |
| B, four even+odd total measurements | 9C60 | 9C60 | Match |
| B, four MID FRAME SIZE measurements | 4F40 | 4E40 | 256 µs too short |

The page-1 PNG hash is
`16e3b86b5cab990228c21aad916f7d2d2df8c84ec51fa7f5a066b2022f38b1ef`;
page 2 is `778bf2ba48d1909797b27deeba2d493944f6d07e5792253f27cb7b306717eeab`.
The prior session did not capture page 2, so these observations alone do not
establish whether its residuals predate the F14 repair. No RTL cause is assigned.


## B9, type 0

The four-page timed script was prepared against reference states A–D. Execution
reached the first screenshot request, which timed out after 20 seconds without
producing a PNG. SSH remained healthy and the runner restored the original CFG
and removed its temporary files. A separate scaled screenshot request of the
same live state also produced no PNG before the next case started.

**Inconclusive:** no numeric comparison or claim about the rendered type-0 output
is possible. The script stopped before advancing to pages B–D. A screenshot-only
CSL probe was rejected by the runner's reset requirement before contacting the
device; the subsequent direct Main screenshot command was the actual scaled probe.
The failed manifest/log remain under `repaired-b9-type0/`.

## Bounded follow-up discriminators

The type-1 page-B residual should start from SHAKER B9's displayed
`R8 UPDATE DELAY` C9=0/C0=3F cases and the four `DELAY FOR EVEN+ODD FRAME`
R6 pairs (50/50, 7F/50, 50/7F, 7F/7F). Capture the actual instruction/write
phase, raw VSYNC and frame-origin state before selecting a cause or changing RTL.
The current `t28d`–`t28g` duration tests establish the additional-line behavior;
they do not reproduce these measured R8-write or middle-frame sequences. A new
fail-first case must come from the source rule and executed sequence. No speculative
repair or simulation-baseline change was made in this hardware acceptance batch.


## C4, type 1: identified states A–E

Ten 768×546 captures completed; each state/repeat pair is pixel-identical.
Comparison uses the real-CPC photographs [A](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_A.webp),
[B](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_B.webp),
[C](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_C.webp),
[D](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_D.webp) and
[E](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_E.webp).
A separate image analyst confirmed the transcription and structural comparison.

| State | Identifying captured text | Concrete difference from corresponding photo |
|---|---|---|
| A | PARITY TEST 3 (C4.0=1), ODD FRAME; ODD/EVEN PARITY (C4.0=1) | Reference's lower patterned field is replaced by a broad pale region containing B9 numeric text from approximately y=274 |
| B | PARITY TEST 1, EVEN FRAME, C4.0=1; EVEN R9 BEFORE IVM ON ODD C9 | Pale B9-text region across lower half and internal glyph band interrupt reference's continuous central bars |
| C | Same first heading; ODD R9 BEFORE IVM ON ODD C9 | Same lower-half structural discrepancy as B |
| D | First heading partly overprinted; EVEN R9 BEFORE IVM ON EVEN C9 | Bars extend farther down; pale numeric text remains at y≈506–545 and upper heading is overprinted |
| E | PARITY TEST 1, EVEN FRAME, C4.0=1; ODD R9 BEFORE IVM ON EVEN C9 | Numeric text remains at y≈506–545; extra glyph band around y≈245–274 interrupts central bars |

The reference itself intentionally contains top patterns, bars and bottom glyph
fragments. Those features alone are not a failure. **All five captured frames
have concrete structural discrepancies**, but their cause is unassigned: stills
cannot distinguish retained capture-buffer contents from generated core video,
or establish temporal/parity behavior. Do not infer CRTC timing correctness or
an RTL defect from these stills alone. States F onward were outside this bounded
capture. Next discriminator: matched physical-output observation and native
capture of the same identified state, before an RTL hypothesis.

## Restoration and delivery

The user reported a Wi-Fi reboot changed the device address and left a stale
negative DNS cache. On resumed contact, MENU and the original CFG already
matched; no user-state overwrite was needed. The earlier run's two persistent
CSL temporaries were removed, and its temporary MBC was already absent.

After this repaired-build batch, each runner's cleanup restored the original
CFG and removed its own temporary MGL/CFG files. Final live verification showed
MENU and SHA-256 `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
The temporary MBC was removed. Inputs used completed taps, with no held-key
operation left outstanding. **DEVICE_RELEASED** was sent to the coordinator.
The earlier report's open restoration obligation is superseded.

This follow-up changes documentation only, based on integrated `95e6f56`.
No RTL or test expectations changed, and no simulation rerun or code review is
required. Capture manifests retain their real success/failure outcomes.

## Capture identities


repaired-b9-type1:

- `B9_type1_page1.png`: `16e3b86b5cab990228c21aad916f7d2d2df8c84ec51fa7f5a066b2022f38b1ef`.
- `B9_type1_page2.png`: `778bf2ba48d1909797b27deeba2d493944f6d07e5792253f27cb7b306717eeab`.

repaired-c4-type1:

- `C4_type1_page1.png`: `03058f939a01859d5766febd4f7afbbac7478fd5136f7c5307657f0e38e83de9`.
- `C4_type1_stateB.png`: `801908ba223c74e54b210bd041202cccb43b20f97b865eb4c1e3e7dc723f54ec`.
- `C4_type1_stateC.png`: `c147c910a5071c88431cb04abaffe722ad0317f57063855110ad87bd9ca8e7ab`.
- `C4_type1_stateD.png`: `b376ab9d345c8616ac729a096a4f14bad14fce6793fa06a10920084f960a5f01`.
- `C4_type1_stateE.png`: `d5cbb7e78bb76e66a1d2d15a0a7a8a12046cecfdbe70c96397a2bca575b71ec3`.

A same-day follow-up covering the SSH transport fix, updated B9/C4 probes and
the source verification is recorded in
[capture-reliability-2026-09-22.md](capture-reliability-2026-09-22.md).
