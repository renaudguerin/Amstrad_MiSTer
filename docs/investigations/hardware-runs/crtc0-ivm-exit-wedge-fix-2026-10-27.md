# CRTC 0 wedge after SHAKER B1/A1 `00FB`: frozen IVM-exit comparator (2026-10-27)

**Finding:** the wedge in the [2026-10-27 SNA report](crtc0-ssm-wedge-2026-10-27.md) is a type-0
engine state that never releases. When the program leaves interlace-video mode (IVM) with a
C9.VMA value that differs from R9, the engine compares that frozen value against R9 forever.
The row never ends, so C4 and VSYNC stop. On real CRTC 0 the program continues.

## Test sequence (decoded from `SHAKE27B.BIN`)

B1/A1 is menu key `1` (table `0x4338`, first screen at `0x7527`). Before the measured frame,
`sub_789C` selects the CRTC 0 parity-detect routine `0x7705`/`0x7722`, which runs on the menu
register state (R0=63, R4=38, R7=30, R9=7):

1. Sync on a VSYNC rising edge (`9F52`), then write R6=127. C4 can no longer reach R6, so
   ParityFrame stops toggling.
2. Sync again. VSYNC starts at C4=30, C9=0.
3. At +13 µs, write R9=6.
4. At +398 µs (C9=6, the last line of the row), write R8=3 to enter IVM.
5. At +452 µs (next line, C9=7), write R8=0 to leave IVM.
6. At +466 µs, write R9=7.

Next comes the B1 table write at `0x75F2` (R0..R9 = 63 40 46 14 38 0 127 0 0 7), which is exactly
the register set in the wedged SNA. Two 50 ms delays follow, then the VSYNC wait at `0x7604`
(`9F5B`). Instruction timing is verified from the bytes. The µs offsets come from a timing
model of the code and are good to about ±1 µs.

## Reproduction

The [replay](#proof) in the type-0 pin bench reproduces the SNA state. After step 5 the
engine freezes C9.VMA=15 (`exit_frozen_vma`) with ParityFrame odd. Plain R9 is 7, so the
frozen comparison never matches. C9 then cycles 0..31 indefinitely at C4=30 and VSYNC never
returns. This matches the device observation: C4=30 in both SNAs, and C9 differs between them.

## Rule and hardware

French ACCC v1.11 §19.8.1 p.221 describes the exit. When R8 returns to 0, the line-end test
uses C9.VMA (which includes parity) against R9 (without parity). If they differ, C9
increments. The recipe to make C9 return to 0 is to program R9 with C9.VMA before the end of
the line. Finding F16 implemented this from the pp.224–225 exit tables: the frozen C9.VMA keeps
being compared, and the run-on windows continue through C9=7. **The source does not say when
that frozen comparison ends.** The tables stop at C9=7, and the core's "until a match"
lifetime was an extrapolation.

Real hardware settles that the comparison does end. The Logon System CRTC 0 photographs of
SHAKER B1 show page `00FB` (its screen text) and page `00FC` (a stable test picture), and
then the rest of B1 (`00FD`…`03EF`). A real CRTC 0 therefore keeps producing frames after this
exact sequence. The photographs were fetched with
`scripts/shaker-ref/scrape_logon.py --arch cpc_CPC --crtc 0 --module B --test B1`.

## Change

`rtl/crtc_type0_engine.v` now releases the frozen post-exit comparison when C9 wraps from 31
to 0 without a row end. From the C9=0 line on, the plain C9==R9 test applies again. This is
the general §10.3.1.1 p.76 path ("C9 va compter jusqu'à sa valeur maximale (31) avant de
reboucler à 0, et ce jusqu'à atteindre la valeur de R9"). The exit window itself (F16, t22l–t22s,
t30a/t30b) is unchanged.

**Inference, not a source rule:** releasing at the wrap is the least-assumption point that ends
the wedge without touching the documented window. Hardware proves that a release exists, not
where it happens. A hardware vector that times the first frame after a non-matching exit
would settle it. It could be a SHAKER-like routine that measures the frame length after the
exit, or the `00FC` screen geometry compared with a MiSTer capture.

## Proof

- Fail before: `t30c_type0_ivm_exit_frozen_mismatch_resumes_frames` reported "character 96722
  … expected type 0 resumes frames after a non-matching IVM exit … high, actual 0". It replays
  steps 1–6 and requires a VSYNC within three frames, and is the only failure in
  `make -C sim crtc-test`.
- Pass after: `make -C sim crtc-test` reports `Summary: 233 passed, 0 xfailed, 0 xpassed,
  0 failed`. `python3 sim/select_tests.py --run` reports `select_tests: PASS 2 benches:
  crtc-test, crtc-cpu-phase-test`. Lint is clean.
- Soak on this branch (base `3bf1c53`): `0xe99ab434a5e1cdb3` became `0x7358b7e99971f5d6`. This
  is an intended behaviour change, because the random stimulus reaches non-matching IVM exits.
  Merged with master's `01F4` change (`0x0bb996495fb3207e`), the combined hash is
  `0xde0a16d0ececc573`.

## Residuals

- The release point (C9 wrap) is inferred; see Change.
- The MiSTer recheck of SHAKER B1 on CRTC 0 waits for an RBF with this change. Pass condition:
  B1 walks past `00FB`, and `00FC` matches the photograph.
- D1-27 at `0305` (stall B) has not been checked against this fix.
