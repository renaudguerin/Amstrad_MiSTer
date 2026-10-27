# CRTC 1 infinite HSYNC, SHAKER `01F4` (2026-10-27)

**Finding:** on CRTC 1, when C0 equals R2 at the position C0=R2+R3l, HSYNC does not end. The
core ended it. With R0=0, R2=0, R3l=1 this gives a permanent HSYNC on real hardware and a
1 µs on / 1 µs off pulse train in the core.

## Hardware evidence

SHAKER 2.7 module B, page `01F4` ("BR/E R52 INC IN HSYNC"). The Logon System CRTC 1
photograph reads `R0=0/R3=1/R52 CALC=#FF`. AmSpirit also reads `#FF`; the `cec641c` MiSTer
capture reads `#34` ([photo comparison](shaker-hw-photo-compare-2026-10-27.md)).

## What the test does

Read from `SHAKE27B.BIN` (loads at `0x3900`), routine `502A`, called twice:

1. Write R2=0 and R3=`0x0A`, then wait 50 ms.
2. Lock onto VSYNC with the shared `9F52` routine. Take two `HALT`s, then point the IM1
   vector at the result handler `5062`.
3. Write R3=1, then R0 (`ED 49` gives R0=1 on the first call, `ED 71` gives R0=0 on the
   second), and jump into a sled of about 3328 NOPs at `0x0100`.
4. When the next interrupt arrives, the handler pops the interrupted PC and prints
   `CALC = (PC low byte) >> 1`. If no interrupt arrives, the sled falls through to `50A1`
   (`DI; LD H,FF`) and prints `#FF`.

The Gate Array raises an interrupt when R52 wraps after 52 HSYNC ends. With R0=1, a line lasts
2 µs and one HSYNC ends per line, so 52 ends take 104 µs. PC low is `0x68`, which prints
`#34`, the value every CRTC type expects. With R0=0 the test expects `#34` on CRTC 0 and
`#FF` on the others. `#FF` means no HSYNC end at all within 3328 µs.

The disassembly and timing were taken from the bytes in this session. The interrupt-acknowledge
length (5 µs) and the exact C0 at which the R0 write lands are inferred. The verdict does not
depend on either.

## Rule

ACCC v1.11 FR §15.3.1 p.150: on CRTC 1, 2, 3 and 4, "il y a un bug de gestion si C0=R2 sur
C0=R2+R3".

FR §15.3.2 p.150 (HSYNC INFINIE) uses this exact example. With R0=0, R2=0 and R3=1, C0 equals
R2 again on the second character. On the non-0 CRTCs the HSYNC does not end; C3 counts to 15,
wraps to 0 and then 1, and if C0 still equals R2 the HSYNC carries on. FR §27.1 p.283, in the R52
chapter, cites the same R0=R2=0, R3=1 case as a way to make an infinite HSYNC.

FR §15.3.4 p.152 (CRTC 1, 2): when C0=R2 at C0=R2+R3l, C3l overflows and the CRTC HSYNC
continues until C3l wraps back to R3l. CRTC 1 still has time to produce an "invisible" HSYNC
end and reassert it at once. The Gate Array then resets its character counter and sends a
second monitor HSYNC.

## Change

`rtl/CRTC.v` now holds HSYNC high on type 1 at the end position when C0 equals R2. The rule
sits in the wrapper's type-1 `hsync_off` select rather than in the type-1 engine: the
`crtc_t80_old` negative-control bench (CI `production-t80`) compiles the current wrapper
against historic engine files, so the engine port list is fixed. A first version added an
engine port and broke that build on `f180f30`; the fix-forward is behaviour-identical (soak
hash reproduced). C3l is
not reset, so its 4-bit counter wraps and the same test runs again when it next reaches R3l.
Type 0 is unchanged.

The "invisible" HSYNC end of §15.3.4 is not modelled. Its only documented effect is the Gate
Array's character-counter reset. The hardware `#FF` shows that R52 does not count it, and a
sub-character pulse into the GA netlist would count. This is a known residual: the second
monitor HSYNC at the re-entry position, which is visible only as monitor timing.

## Proof

- Fail before: `make -C sim crtc-test` failed `t09m_type1_r0_zero_r2_zero_infinite_hsync` with
  "character 2, tick 5/16 … expected … HSYNC never ends … high, actual 0". The control
  `t09n_type1_r0_one_r2_zero_hsync_still_ends` passed. Summary: 233 passed, 1 failed.
- Pass after: `make -C sim crtc-test` reports `Summary: 234 passed, 0 xfailed, 0 xpassed,
  0 failed`. `python3 sim/select_tests.py --run` reports `select_tests: PASS 3 benches:
  crtc-test, crtc-cpu-phase-test, ga40010-test`. `make -C sim lint` is clean.
- Soak: the hash moved from `0xe99ab434a5e1cdb3` to `0x0bb996495fb3207e` with the seed and
  stimulus unchanged. This is an intended behaviour change: the randomized stimulus reaches
  C0=R2 at the type-1 HSYNC end position.

## Residuals

- The hardware check of the new RBF on SHAKER `01F4` waits for the next master synthesis.
- The §15.3.4 GA re-sync blip is not modelled; see Change above.

## MiSTer recheck

The [2026-10-27 recheck](shaker-followup-recheck-2026-10-27.md) on the exact
`925b5bb` RBF still reads `CALC=#34` for R0=0, against original CRTC1 `#FF`.
The pin-level rule remains implemented; integrated CRTC/GA/IRQ acceptance fails.
