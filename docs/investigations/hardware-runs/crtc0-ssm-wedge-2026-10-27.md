# CRTC0 SSM-marker wedge after B1/A1 `00FB` (2026-10-27)

**Question** (from [the 2.7 B–E report](shaker27-modules-b-e-2026-10-15.md) §3): after a
sync-breaking page on CRTC0, does the CPU stall or wedge (a core bug), or does it keep running
while only capture lock is lost?

**Answer, for B0-27 `00FB`:** the CPU is not wedged. It is alive with interrupts disabled,
spinning in SHAKER's VSYNC-wait loop, and VSYNC never comes. In the saved state the CRTC0
row counter sits at C4=30 in two snapshots taken about 10 s apart. Under the ACCC counting
rule the frame should have ended many times in that interval. The missing markers and the
capture starvation are therefore both downstream of a CRTC0 that stops producing frames.
That is a core-behaviour candidate, not a capture artefact. It is not yet proven: this run did
not include a simulation reproduction, and the exact register sequence of the B1/A1 test was not
extracted.

## Method

The production-T80 replay suggested in the brief was replaced by a direct device observation.
SSM markers in this build are computed at runtime (no literal `ED FB ED 00` in `SHAKE27B.BIN`),
so a faithful replay would need the whole menu walk. The B18 save path freezes the running
machine and publishes a full SNA v3, with CPU registers, RAM and CRTC counters, which answers
the question directly.

1. `csl_runner.py` walked `CSL_27/MODULE B/SHAKER-B-0.CSL` on delivered `cec641c`
   (`40f7d66b…`, `shaker27.dsk` `65eb43e1…`, Full, `--ssm --max-wait 120
   --tolerate-capture-timeout`, MBC `0e99082b…` re-provisioned to `/tmp`). As in October,
   it failed with `no SSM #00FC arrived within 120s`: 2 capture records, 2 tolerated
   failures.
2. With the core still running, the OSD `Save snapshot` (F12, nine Down, Enter, through the B17
   replay helper) and `sna_pull.py` produced `B0-27-wedge.sna` (`6c46893b…`). A later save gave
   `B0-27-wedge-3.sna` (`0d6d466f…`). The `-2`/`-4` pulls repeat generations 1 and 2. After the
   first save the OSD cursor stays on the item, so the second replay navigated elsewhere. The
   CFG hash was unchanged afterwards (`f8c6879a…`), and the device was returned to MENU.
3. A native `screenshot` command in the wedged state produced no file, which is consistent
   with no VSYNC.

Evidence (ignored): `docs/references/shaker-ssm-wedge-2026-10-27/` in the main checkout.

## Observations

| SNA | PC | IFF | R0–R9 | C0 | C4 | C9 | VSYNC/HSYNC flags |
|---|---|---|---|---|---|---|---|
| 1 | `9F5B` | 0 | 63 40 46 14 38 0 127 0 0 7 | 9 | 30 | 24 | 00 |
| 3 (≈10 s later) | `9F5D` | 0 | same | 54 | 30 | 7 | 02 (HSYNC) |

Code at `9F52` (RAM in the SNA): `DI; LD B,F5; LD HL,4DEB; LD DE,FFF5`, then
`9F5B IN A,(C); RRA; JR NC,9F5B`, a wait for PPI port B bit 0 (VSYNC) to rise. It is followed
by the falling-edge and rising-edge waits and a counted loop (`ADD HL,DE`). With interrupts
off, nothing else can leave this loop.

## Reading against the rule

ACCC v1.11 FR §10.3.1.1 p.76 (CRTC 0): when R9 is set below C9, C9 "va compter jusqu'à sa
valeur maximale (31) avant de reboucler à 0, et ce jusqu'à atteindre la valeur de R9". After
that, C4 increments at the C0=0 seam with C9==R9. From SNA 1 (C9=24, R9=7, R4=38, R7=0,
R5=0) the current row should end within 16 lines. The frame should then end at C4=38, and
VSYNC should fire at C4=R7=0 at the next frame origin. That is well under 20 ms, and the
program would leave `9F5B`. Instead C4 is 30 in both samples. Two samples cannot prove that
C4 never moves. The chance of an ordinary 39-row cycle landing on C4=30 twice is small, and
the 120 s with no VSYNC and no marker is the stronger evidence.

The `00FB` page is "INTERLACE C4/C9 COUNTERS". It exercises R8/R9 counter handling, which is
where the type-0 engine keeps state the SNA does not serialize: IVM parity, the last-line and
additional-management latches, and the `vsync_allow` consumption. The leading hypothesis is a
latch left set by the test that stops C4 from advancing, or stops the frame from ending,
after R8 returns to 0.

## Next step (not done tonight)

1. Extract the B1/A1 register-write sequence from `SHAKE27B.BIN`. The marker emitter computes
   the code, so find the test through the module's dispatch table rather than by a byte
   search for the marker.
2. Replay that sequence in the CRTC pin bench (`sim/sim_main.cpp`, type 0) and check that C4
   advances and VSYNC fires once R8=0 and R9=7. A vector that fails there is the finding.
   A plain "R9 lowered below C9" case is expected to pass, because §10.3.1.1 is implemented.
3. The same SNA method applies to the other stall classes (C0 `029E`, D0 `02EA`→`0303`,
   B1/C1/C3/D1/D3/E-27). A per-cell SNA would show which ones are VSYNC waits.
