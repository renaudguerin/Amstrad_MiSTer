# Finding F19 — CRTC 2 $C_0=0$ Last Line Evaluation Timing ($R_4$ vs $R_9$)

Technical information sourced from *The Amstrad CPC CRTC Compendium* v1.11 by Longshot (CC BY-NC-ND 4.0).

---

## 1. Documented Hardware Rule Analysis

In **ACCC v1.11 §12.4.1 (p. 95)**:
> *"At the beginning of a line (C0==0), the comparison uses the **updated** value of R4, but the **previous** value of R9 (an update of R9 on C0==0 occurs too late for this evaluation).*
>
> *Consequently:*
> *- If C4<>R4 or C9<>R9 during this C0==0 evaluation, then the "Last Line" state is false. For instance, if C4==R4==38 and C9==R9==7 at the start of C0==0, updating R4 to 10 on position C0==0 will cause C4<>R4, setting the "Last Line" state to false. Conversely, modifying R9 on C0==0 will have no immediate effect on this evaluation.*
> *- If C4==R4 and C9==R9, then the "Last Line" state becomes true, except in two particular cases: o if the previous line was itself a last line o if a HSYNC starts on position C0==0."*

### Independent Review Adjudication (2026-08-28)
- The cited sentence is located in **§12.4 CRTC 2** (MC6845, p. 95) under subsection **§12.4.1 Last Line Concept**. It describes the specific internal state machine of CRTC 2 ("Last Line", "Last Line Management", "Previous Last Line").
- In contrast, **CRTC 0** (HD6845S / UM6845) is documented in **§12.2 (pp. 92–94)**:
  > *"As long as C0<2, the CRTC evaluates whether C9==R9 and C4==R4 to determine if the last line of the frame has been reached. It no longer repeats this test on the other values of C0>1. It is therefore not necessary to anticipate the programming of R4 (or R9) on the current line for the last line condition to be true on the following line. It is possible to modify R4 or R9 on the current line as long as C0<2 to validate the "Last Line" state (and thus validate the reset of C4 on the following line)."*
  > *"If R9 and/or R4 are positioned at 0 when C9=C4=0, this line will not be considered the last on the frame (the test took place when C0=0 and C0=1)."* (p. 93)
- Therefore, the $C_0=0$ previous-$R_9$ rule is specific to CRTC 2 and does **not** apply to CRTC 0.

---

## 2. Core Status & Verification

In `rtl/crtc_type0_engine.v`:
- `type0_c0_r4` and `type0_c0_r9` both evaluate same-edge bus writes on $C_0=0$ (`hcc == 0`), conforming to §12.2 pp. 92–94.
- Deterministic test vectors in `sim/sim_main.cpp`:
  - `t12c_type0_c0_r9_write_immediate_clears_last_line`: OUT R9, 10 on $C_0=0$ evaluates updated $R_9=10$ ($C_9=7 \ne 10$), clearing Last Line $\implies C_9$ increments to 8 without entering adjustment.
  - `t12d_type0_c0_r9_write_immediate_validates_last_line`: OUT R9, 7 on $C_0=0$ evaluates updated $R_9=7$ ($C_9=7==7, C_4=38==38$), validating Last Line $\implies C_9$ wraps to 0 and row enters vertical adjustment (`in_adj=1`).
  - `t12e_type0_c0_r4_write_immediate_clears_last_line`: OUT R4, 10 on $C_0=0$ evaluates updated $R_4=10$ ($C_4=38 \ne 10$), clearing Last Line $\implies C_4$ increments to 39 without entering adjustment.

### Gate Results
- `make -C sim`: 175 tests passed, all Plus suites passed.
- `make -C sim lint`: clean (exit 0).
- `make -C sim soak SOAK_EXPECT=0x48146d2b681268ab`: verified bit-identical golden soak hash.
