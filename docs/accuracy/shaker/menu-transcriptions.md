# SHAKER 2.6 menu transcriptions

Verbatim transcription of five SHAKER 2.6 screen captures taken on 2026-08-19. The PNGs
themselves are deliberately not committed; they are kept locally and are referenced below by
filename so a capture can be matched back to its transcription. These replace the earlier unverified subtest list that came from secondary
web sources. Spelling, capitalisation, spacing, and the parenthesised test counts are as shown
on screen.

Every screen carries the same three footer lines:

```
READ THE COMPENDIUM: HTTPS://SHAKER.LOGONSYSTEM.EU
C0vs=0 DEFINED WHEN VSYNC PPI.PORTB.0=1
CRTC 1
```

The final line is SHAKER's own CRTC detection result. All five captures read `CRTC 1` because
they were taken on an emulator configured for CRTC type 1. This is the expected value for that
configuration, not a detection anomaly. It does mean the menus alone say nothing about how SHAKER
identifies our core, and that a CRTC-0 session is still needed to read the type-0 menus.

The header line on each screen is `CPC SHAKER 2.6 MODULE <x> / LONGSHOT. LOGON SYSTEM`.

Counts in parentheses are SHAKER's own, e.g. `(79 TST)` means that entry runs 79 tests. Entries
with no count printed did not show one.

## Module A — `Screenshot 2026-08-19 at 15.51.59.png`

```
(1) UPDATE VRAM VS CRTC (79 TST)
(2) SKEW DISP ON R0 RUPTURE (4 TST)
(3) CRTC 2 RVMB (22 TST)
(4) UPDATE CRTC R0 TIMING (17 TST)
(5) R13 UPDATE IN 4 USEC SCREENS (R0=3) (5 TST)
(6) R13 UPDATE IN 2 USEC SCREENS (R0=1) (5 TST)
(7) R13 UPDATE IN 1 USEC SCREENS (R0=0) (5 TST)
(8) GATE ARRAY PIXELISATION
(9) GATE ARRAY INKERISATION (3 TST)
(E) GATE ARRAY MODERISATION
(R)    MODE UPD >HSYNC DELAY< (2.1.0)(3 TST)
(CAPS) INTERACTIVE TEST MODE X TO Y (16 INTERACTIVE TST)
(TAB)  HSYNC START POSITION (4 INTERACTIVE TST)
(T) R2 UPD DURING & AFTER HSYNC (6 TST)
(Y) R3 UPD DURING HSYNC (8 TST)
(U) R4 & R9 CHECKING (54 TST)
(I) VSYNC CONDITIONS (413 TST)
(O) R1 STORIES (8 TST)
(P) R6 STORIES (13 TST)
(COPY) CRTC 2 OFFSET
```

## Module B — `Screenshot 2026-08-19 at 15.52.25.png`

```
(1) INTERLACE C4/C9 COUNTERS (Y=PARITY CRT 0/2/3/4:0)(Z/X)=R9 BASE (07)
(2) INTERLACE CRTC 2 C9 STRANGER THING
(3) FAKE VSYNC ON CRTC 2
(4) CRTC 2 FIND C0 MIN
(5) CRTC 2 RLAL
(6) CRTC 1 BUG OUTI R0
(7) CRTC 0 BUG OVF C4
(9) INTERLACE VM (27 TST) (Y PARITY CRT 0/2/3/4)
(O) CRTC 1-A OR 1-B?
(RETURN) R5 STORIES
(F0) BOUNGA!
(CAPS) RVNI LTD
(P) ANALYZER / FORCED STAB CRTC 0 R0=0  (4 CONF)
(R) INTERRUPT DELAY FROM R2 (18 CALC)
(U) CRT 000 (CRTC 0)
(I) R3 JIT (8 TST)
(T) R3 JIT INTERACTIVE MODE ANALYZER
(S) CRTC 1 : BE00 CHECK
(CTRL) R5 SCANNER / (COPY) R5 T2
(TAB) R5 BENCH (INTERACTIVE)
(0) VERTICAL SCROLL SUB-PIXEL 1/8,1/16,1/32,1/64,1/128
```

Entry `(8)` is absent from the screen; the list jumps from `(7)` to `(9)`.

## Module C — `Screenshot 2026-08-19 at 15.52.42.png`

Two-column layout: a CRTC-scope column, then the test name.

```
(1) CRTC 1    : RFD & PARITY STORY
(2) CRTC 1    : R8 IVM ON ODD C9
(3) CRTC 1    : PARITY SWITCH STATUS
(4) CRTC 1    : IVM ON/OFF
(5) CRTC 0.2  : PARITY CHECK SELECT
(6) CRTC 2    : C9.IVM SWITCH
(7) CRTC 2    : LAST LINE COND (OPEN TO OTHER CRTC'S)
(8) CRTC 2    : ADD LINE ON PARITY BUG
(9) CRTC 2    : ADD LINE RQ & TRIGGER
(T) CRTC 2    : VMA' ON R1=0 (OPEN TO OTHER CRTC'S)
(RETURN) CRT2 : GHOST VSYNC VS LAST LINE (OPEN TO OTHER CRTC'S)
(E) ALL       : ADD LINE R5 ON LAST LINE
(P) ALL       : ADD LINE R8
(S) ALL       : R5 AND INTERLACE MANAGEMENT
(O) ALL       : INTERLACE VSYNC NIGHTMARE
(Y) ALL       : CRTC 3/4 PARITY
(R) CRTC 0    : R9/R4 UPD LAST LIMIT
```

## Module D — `Screenshot 2026-08-19 at 15.53.07.png`

```
(U) CRTC 3/4 : STATUS (DEADLOCK MAY HAPPEN IF BAD EMULATION)
(I) SHAKER KILLER 2 (WARNING : NOT RELIABLE ON CRTC 1)
(R) VSYNC TORTURE (LOCK MECHANISM)
(T) VSYNC GATE ARRAY
(H) HSYNC GATE ARRAY
(1) CSYNC4 VS 2xCSYNC2
(2) R2.JIT >> NO CSYNC UPD
(3) 2xCSYNC RELATIVE
(4) CSYNC MULTIPLES
(5) WIP (SOME R3.JIT)
(6) HARDWARE SCROLL 1 PIXEL MODE 1/0 (NO BUFFERING)
(7) R2 OSCILLATION STORY
(8) NO HSYNC FOR XX LINES
(9)    CRTC 1 : RFD ROUND 2
(E)    CRTC 1 : OFS UPD IN ADD MANAGEMENT
(CAPS) CRTC 1-OUTI R0 AUTOPSY
(P)    CRTC 2 : VMA' BUG
(S) SPLITBORDER POS
(COPY) RESET CPC
```

## Module E — `Screenshot 2026-08-19 at 15.53.41.png`

```
(1) R5 STORIES 2ND ROUND
(2) CRTC 1 VMA TRT C4=R4=0 ON ADJ LINE C4=1 ON NO-EXTENT FRAME
(3) CRTC 0 C4/C9 COUNTER LOGIC BUG
```

The Module E capture is cropped to the top of the screen and shows no footer. Only three
entries are visible; whether more exist below the crop is unknown.

## Notes for the next hardware session

- Module A's per-entry test counts total well over 600 individual tests, dominated by
  `(I) VSYNC CONDITIONS (413 TST)` and `(1) UPDATE VRAM VS CRTC (79 TST)`. "A few Module A
  tests" therefore covered a small fraction of the module.
- Record which menu entries were run, and set the OSD CRTC selection deliberately. Confirm the
  footer reports the type you selected: a mismatch would mean our CRTC identification behaviour
  diverges from hardware, which ACCC chapter 28 (CRTC IDENTIFICATION, p.292) covers in ten
  separate detection methods.
- Several entries are interactive rather than pass/fail, marked `INTERACTIVE TST`.
