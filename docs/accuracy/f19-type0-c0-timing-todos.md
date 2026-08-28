# Finding F19 — CRTC Type-0 $C_0=0$ Last Line Evaluation Timing ($R_4$ vs $R_9$)

Technical information sourced from *The Amstrad CPC CRTC Compendium* v1.11 by Longshot (CC BY-NC-ND 4.0).

---

## 1. Documented Hardware Rule

In **ACCC v1.11 §12.2.3 (p. 95)**:
> *"At the beginning of a line (C0==0), the comparison uses the **updated** value of R4, but the **previous** value of R9 (an update of R9 on C0==0 occurs too late for this evaluation).*
>
> *Consequently:*
> *- If C4<>R4 or C9<>R9 during this C0==0 evaluation, then the "Last Line" state is false. For instance, if C4==R4==38 and C9==R9==7 at the start of C0==0, updating R4 to 10 on position C0==0 will cause C4<>R4, setting the "Last Line" state to false. Conversely, modifying R9 on C0==0 will have no immediate effect on this evaluation.*
> *- If C4==R4 and C9==R9, then the "Last Line" state becomes true, except in two particular cases: o if the previous line was itself a last line o if a HSYNC starts on position C0==0."*

---

## 2. Current RTL Implementation

In `rtl/crtc_type0_engine.v` (lines 416–423):
```verilog
wire       type0_r4_at_c0_write = register_write && addr == 5'd04 && hcc == 0;
wire       type0_r9_at_c0_write = register_write && addr == 5'd09 && hcc == 0;
wire       type0_r5_at_c0_write = type0_r5_write && hcc == 0;
wire [6:0] type0_c0_r4 = type0_r4_at_c0_write ? DI[6:0] : R4_v_total;
wire [4:0] type0_c0_r9 = type0_r9_at_c0_write ? DI[4:0] : R9_v_max_line;
wire [4:0] type0_c0_r5 = type0_r5_at_c0_write ? DI[4:0] : R5_v_total_adj;
```

And in `rtl/CRTC.v` (lines 348–353):
```verilog
if(hcc == 0 && !r0_frozen) begin
    line_last_r <= CRTC_TYPE ? e1_line_last : e0_c0_line_last;
    row_last_r <= CRTC_TYPE ? e1_row_last : e0_c0_row_last;
    frame_adj_r <= (CRTC_TYPE ? (e1_line_last & e1_row_last) :
                             (e0_c0_line_last & e0_c0_row_last)) & ~in_adj;
end
```

### Analysis of Divergence
1. **$R_4$ handling**: Current RTL uses `type0_r4_at_c0_write ? DI[6:0] : R4_v_total`, which immediately takes the updated $R_4$ value on $C_0=0$. This is **correct** per ACCC v1.11 p. 95.
2. **$R_9$ handling**: Current RTL uses `type0_r9_at_c0_write ? DI[4:0] : R9_v_max_line`. However, hardware evaluation of $R_9$ happens before the $C_0=0$ bus write takes effect. Therefore, `type0_c0_r9` should always evaluate against `R9_v_max_line` (the pre-edge / previous value).
3. **Rollover handling later in line**: Modifying $R_9$ on $C_0=0$ does take effect for later operations in the line (such as the line-end rollover comparison and mid-line writes), but does **not** retroactively alter the $C_0=0$ `Last Line` latch decision.

---

## 3. Implementation Plan & Action Items for Future Session

### TODO List
- [ ] **Deterministic Test Vector**:
  - Add test `t12_type0_c0_r9_write_late`: Set $C_4=R_4=38, C_9=R_9=7$. On $C_0=0$ of line 7, execute `OUT R9, 10`.
  - Assert that `Last Line` was latched as TRUE at $C_0=0$ using the previous $R_9=7$ value, rather than evaluating false.
  - Companion test: Set $C_4=R_4=38, C_9=7, R_9=10$. On $C_0=0$, write `OUT R9, 7`. Assert that `Last Line` evaluates FALSE at $C_0=0$ (using previous $R_9=10$), so `Last Line` is not armed at $C_0=0$.
- [ ] **RTL Update in `rtl/crtc_type0_engine.v`**:
  - Adjust `type0_c0_r9` definition to use `R9_v_max_line` directly for the seam $C_0=0$ `Last Line` test.
  - Verify that mid-line / rollover live comparisons (`type0_live_line_last`) continue using the updated register file.
- [ ] **Soak & Gate Verification**:
  - Run `make -C sim` and `make -C sim soak`.
  - Update golden hash if behavioral change occurs on $C_0=0$ $R_9$ writes.
