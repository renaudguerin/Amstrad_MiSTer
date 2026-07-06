# ACCC v1.9 Digest 03 — Display Registers (R1, R6, R8), Video Pointer (R12/R13), Read Registers, CRTC/CPC ID

Source: "The Amstrad CPC CRTC Compendium" v1.9 (Logon System), chapters 17–22, 28–29.
Scope: **CRTC type 0 (HD6845S/UM6845) and type 1 (UM6845R) only.** CRTC 2/3/4 behavior is
noted only where it sharpens the type-0/type-1 contrast (one line, prefixed "cf. CRTC2/3/4:").

Counter names used throughout, matching the compendium's own notation:
- `C0` = horizontal character counter (0..R0)
- `C4` = character-row counter (vertical, in character rows)
- `C9` = scanline-within-row counter (0..R9)
- `VMA` = current 14-bit video memory address counter (per-scanline address)
- `VMA'` = row-start address latch (holds the address to reload VMA with at start of next row)
- `DISPTMG`/`DISPEN`/`DISP ENABLE` = the CRTC's display-enable output pin (same signal, different datasheets)

---

## 17. Register R1 (horizontal displayed)

### 17.1 General (§17.1, p.172-173)
- R1 = number of **displayed** CRTC characters per line; CRTC always generates R0+1 characters/line, of which R1 are shown.
- DISPTMG goes OFF (border) when `C0=R1`; DISPTMG goes back ON when `C0=0` (i.e. after `C0=R0` wraps).
- C0 can never exceed R0 (it's clocked by the R0 comparator), so if R1 > R0 the condition `C0=R1` is simply never true during normal counting.
- If `C0` returns to 0 via **8-bit overflow** (wrapping past 255, not via the R0 compare) DISPTMG is **not** re-enabled — verified for CRTC 0; unverified for others.
- Not managed by CRTC2 during HSYNC (out of scope, noted for contrast only).
- If R1=0: no characters are ever displayed on any CRTC type (border permanently on, video pointer still counts).
- Border begins exactly at `C0=R1` (assuming SKEW-DISPTMG not in use on type 0 — see §19.2).

### 17.1 VMA/VMA' pointer model (§17.1, p.173)
- CRTC always displays from `VMA`; VMA increments on **every** character cell processed (displayed or not — i.e. also while border is shown).
- **Row-end latch:** when `C0=R1 AND C9=R9` → `VMA' := VMA` (snapshot current address as "next row's start").
- **Row-start reload:** when `C0=0` (start of line) → `VMA := VMA'`, **except** for the first character row of a frame, where behavior is type-dependent on `C4=0` (see §17.4).

### 17.2 R1 vs R0 relationship (§17.2, p.174-178)
- **R1 ≤ R0** (normal case): VMA'←VMA happens once per row on the `C9=R9` scanline at `C0=R1`; VMA←VMA' happens at `C0=0` of the next scanline. Standard incrementing addresses row to row.
- **R1 > R0**: `C0` never reaches R1 (C0 wraps at R0 first), so **VMA' is never updated from VMA** — the row-end latch condition is never satisfied. Consequence: **character-line repetition** — every scanline of every row re-displays the same base address (only C9/C5-derived row-select bits differ; the first 10 address bits from VMA are frozen). ⚠ Practical effect: whatever address was last latched into VMA' (from R12/R13 at start of frame, see §17.4) is reused for **all** rows for the rest of the frame.
- Note 2 (p.176): if conditions allow the address update, **modifying R12/R13 mid-frame** can be used deliberately to force new "lines" each time C0 returns to 0, working around the repetition — this is the mechanism many effects use to fake per-line addressing when R1>R0. Type 0 can still additionally emit a spurious border byte in this configuration (see §17.6.2); type 0 can suppress it via R8 SKEW-DISPTMG (§19.2).

### 17.3 Dynamic R1 update (§17.3, p.176, diagrams p.177-178)
- The condition `C0=R1` is evaluated **continuously/immediately** within a line — not latched once. If R1 is rewritten mid-line such that a **later** C0 value now equals the new R1, the comparison re-triggers.
- First time `C0=R1` becomes true → DISPTMG OFF (border starts), VMA keeps counting.
- **If R1 is written again during the same line** such that `C0=R1` becomes true again exactly when `C9=R9`, the VMA'←VMA row-end latch **fires again**, updating the pointer — even though only border was ever displayed on screen for that line/row. This is the "R1 border trick" used to move the pointer without displaying data.
- ⚠ VERIFY p.180: worked example ("VMA' pointer repeated=960/1000") shows the precise interaction of writing R1 exactly on `C0=R1` (JIT) vs one cycle later (`C0=R1+1`): writing R1 exactly when `C0=R1` is being evaluated **suppresses** that particular match (condition no longer true after the write), so VMA' is *not* updated on that pass; writing it one cycle later (after the compare already fired) **does** allow the update. This "write during the exact compare cycle" nuance is timing-critical and worth a dedicated testbench check — the diagrams are compressed and easy to misread.

### 17.4 VMA'/VMA when C4=0 (§17.4, p.179-181) — KEY TYPE-0 vs TYPE-1 DIVERGENCE
This is the single most important dynamic-update rule to get right per CRTC type.

**Type 0 (§17.4.1, p.179):**
- First scanline of first character row of a frame (`C4=C9=C0=0`): **VMA' is updated from R12/R13 first**, then **VMA is updated from VMA'** (two-stage: R12/R13 → VMA' → VMA).
- If R1>R0 throughout the frame, VMA' is never subsequently updated (per §17.2), so on the *next* frame, when `C4=C9=C0=0` recurs, VMA' is reloaded from R12/R13 again and **all displayed lines within the new frame become identical**, equal to the R12/R13 pointer (since the per-row latch that would normally advance it never fires).
- Practical rule for the Verilog model: **type 0 loads VMA' from {R12,R13} at C4=C9=C0=0, then immediately copies VMA'→VMA in the same cycle/step** (i.e., R12/R13 effectively lands directly in VMA for that first scanline too, via the two-stage path).

**Type 1 (§17.4.2, p.179-181):**
- First scanline of first character row of a frame (`C4=C0=0`, **C9 not required to be 0!**): **VMA is updated directly from R12/R13** (NOT via VMA' as on type 0).
- Consequence: on type 1, **R12/R13 can be rewritten on every scanline while C4=0** (i.e., for all C9 = 0..R9 of the first character row) and each rewrite takes effect on VMA directly — a per-scanline offset trick unique to type 1 that does not work the same way on type 0.
- Exception/extension: if C4=0 happens again during **additional-row management** (because R4=0 and R5>0), C4 stays 1 for the first additional rows (R9 or R5 rows, whichever smaller) — offset can be modified per-scanline during that C4=1 stretch too, but not beyond it if R5>R9+1.
- If R1>R0 for the whole frame on type 1: condition `C0=R1` never fires, so VMA' is never updated and stays frozen at whatever it last was when `C0` reached R1 with `C9=R9`. Meanwhile VMA **is** still reloaded from VMA' whenever `C9=0 AND C4>0`. Net effect: **first row of the frame uses R12/R13 directly; every subsequent row uses the last value VMA' was frozen at** — different rows can show different (but frozen) addresses, unlike type 0 where all rows collapse to the same address.
- VMA' is 14 bits; continued incrementing past the 10-bit "usable" range can ripple into the Overscan Bits (§20.5) and shift video pages.
- **R1 update timing at the exact JIT instant** (p.179 bottom): if R1 is rewritten *exactly* when `C0=R1` (before/during that comparator evaluation), the compare is missed (VMA' not updated that pass). If rewritten one cycle later when `C0=R1+1` (i.e. C0 already past old R1), the *new* comparison condition (`C0=R1` with new R1 value) can still be satisfied and updates VMA'. ⚠ VERIFY p.180 (worked numeric example with R1=64→255, C4=24/25 boundary) — timing is subtle, condense to: "write ordering relative to the comparator's evaluation edge determines whether that pass's VMA' latch fires."

*(CRTC2/3/4 for contrast only: type 2 has a documented priority-ordering bug between the "last line of frame" test and the C0=R1 test when R1=0, involving a partial AND-only update bug on VMA'. Out of scope for type 0/1 modeling — see p.180-181 if ever needed.)*

### 17.5 Acknowledgment of R1=0 (§17.5, p.182) — timing/deadline table
- **Type 0 (and type 1, type 2 grouped together in the doc):** writing `R1,0` needs to land **early enough** relative to `C0` reaching the *old* R0 value to be honored for that line; the diagram (p.182) shows: writes landing at `C0=3d`/`3e` = "too late" (border still shows that line as before); write landing at `C0=3f` (i.e. C0=R0, the last character) = "just in time" → no border byte inserted, R1=0 honored starting next line.
- ⚠ VERIFY p.182: the exact cycle-accurate boundary (which C0 value is the last "too late" vs "just in time" one) is best re-derived directly against the Verilog's C0 compare stage rather than trusted from the compressed ASCII diagram — the table only gives relative character positions (3b..3f, 0..5) without an explicit rule sentence for type 0/1, unlike CRTC3/4 which get an explicit contrast paragraph below it.
- **CRTC3/4 contrast (one line):** need one extra character's lead time vs type 0/1/2 for the same OUT to land "just in time" (their acknowledgment window is shifted by one position — see p.182 diagram, bottom half).

### 17.6 Interline border (§17.6, p.182-184)
- **R1=R0 and C0=R0, all CRTC types (0 through 4):** exactly 1 µs (one character) of border is generated on the last character (`C0=R0`) even though R1=R0 — i.e. **the very last displayed character position always shows border for that cycle**, because the DISPTMG-off condition `C0=R1` and `C0=R0`'s natural wraparound coincide, forcing a 1-char border blip before the next row's `C0=0` reload. VRAM pointer offset continues normally into the next character-row's start (§17.6.1, p.182-183).
- **R1>R0 and C0=R0 — type 0 (and type 2 for contrast):**
  - Border activation is **anticipated by 0.5 µs**: type 0 raises "BORDER ON" to the Gate Array as soon as `C0=R0` is reached (since `C0=R1` will never be reached, R1>R0), i.e. **one byte of border (0.5 µs) is generated between character rows** even though R1 was programmed to allow more display width (§17.6.2, p.183).
  - This holds for **any value of R0**, including R0=0 (alternates 1 DISP-ON byte / 1 DISP-OFF byte every character).
  - "BORDER OFF" is sent on the character **following** `C0=R0` (i.e. lasts exactly one character cell).
  - Type 0 can **suppress this spurious border byte** via the R8 SKEW-DISPTMG delay functions (§19.2, p.190) — cf. type 2 which has no SKEW-DISPTMG and therefore cannot suppress it.
  - VRAM pointer gets "stuck" at VMA' in this configuration unless R12/R13 are used to force a reload (Note 2, p.183).
- **R1>R0 and C0=R0 — type 1 (grouped with 3/4 for contrast):** **no border byte is generated between rows at all** — rows can be seamlessly contiguous in the displayable area (used for "frame merging"/plagiarism effects per the text, p.183-184).
- **Implementation rule for the model:** when R1>R0, type 0 must synthesize a border pulse keyed on `C0=R0` (not on `C0=R1`, which never fires); type 1 must **not** synthesize any such pulse — this is a clear behavioral fork that a naive "just compare C0 to R1" implementation will get wrong for type 0.

---

## 18. Register R6 (vertical displayed)

### 18.1 General (§18.1, p.185)
- Border activates when `C4=R6` (checked on the character row's **first** scanline, C9 value irrelevant to the trigger — "true whatever the value of C9").
- **Except CRTC3/4**, R6 is considered **immediately** on the current C0 (i.e. mid-scanline, not just at row boundaries) — relevant for type 0/1.
- DISPTMG goes OFF on `C4=R6` the same way it does on `C0=R1`. Normal recovery only at next full frame (`C4=C9=C0=0`).
- VMA/VMA' pointer keeps counting through R6-triggered border exactly as through R1-triggered border.

### 18.2 R6 deadlines and priorities (§18.2, p.185-186)
- DISPTMG state is arbitrated by two condition groups: "R1 conditions" and "R6 conditions". While R6 condition is unsatisfied, R1 conditions drive DISPTMG. **Once R6 condition is satisfied, R1 conditions are ignored** — R6-border has priority and is "sticky" until the shared reset condition `C4=C9=C0=0`.

**Type 0 (§18.2.2, p.185):**
- Except on the very first line of a frame (`C4=C9=0`), setting `R6=C4` (i.e. R6 written such that it now equals the current C4) causes **immediate and definitive** border activation, lasting until next frame. Condition `C4=R6` is checked immediately regardless of C0.
- Also true during vertical-adjustment (R5) extra character rows.
- `R6=0` on the first line (`C4=C9=0`) is a special conflict case — see §18.3.2.

**Type 1 (§18.2.3, p.186):**
- Same immediate/definitive activation rule as type 0 for `C4=R6` mid-frame.
- **Additionally**, R6=0 is specially treated (as with other type-1 registers defaulting to 0): **R6=0 alone triggers border even without needing `C4=R6`** — UNLESS `C4=0` also holds, in which case the coincidence `C4=R6=0` makes the border **definitive** until `C4=C9=C0=0`.
- ⚠ Exception (p.186): if `C4=R6=0` is true on the **last character of the frame**, the resulting border is **not** irreversible — because the CRTC does not re-evaluate `C4=R6=0` again immediately at the very start of the new frame (it was "already evaluated" on the previous frame's tail); the new frame re-authorizes display, and border only persists because `R6=0` still holds by itself (not because of the C4=R6 equivalence) — an update of R6≠0 cancels it in this specific carry-over case. Per the text: "the update of R6=0 on the previous frame is considered as soon as C0=R0 which precedes the new frame."
- In all *other* cases (R6 set to 0 while `C4≠0`): border activates while R6=0, deactivates as soon as `R6>0` AND its new value differs from current C4 (else border re-activates via normal C4=R6 equivalence).

### 18.3 R6 conflicts (§18.3, p.186-188)

**General (§18.3.1, p.186-187):** R1=0 has an analogous but resolved conflict (deactivation `C0=0`/R0-wrap wins over activation `C0=R1=0`) — deactivation wins. R6's conflict resolution is much less clean.

**Type 0 conflict at C4=R6=0, C9=0 (§18.3.2, p.187):**
- This conflict **only exists when R6=0** (no conflict for R6>0).
- When it occurs: DISPTMG toggles **ON at the start of the CRTC character, OFF again 0.5 µs later**, on **every** CRTC character of that first line — producing a byte-by-byte alternation of border/character bytes on the whole first scanline (VMA still counts through this normally).
- Mechanism: each character cell, `BORDER R6 := true` (because `C4=R6 AND C9=0`), then immediately `BORDER R6 := false` (because it's re-evaluated as a "new frame" state, `C4=C9=0`).
- This alternation **only** happens if the R1-condition doesn't also force border (i.e., border-via-R1 is false) and the conflict conditions (`C4=R6=C9=0`) hold.
- If R6 is subsequently updated to a value >0, the alternation stops — border does **not** stay permanently on (unlike the "normal" C4=R6 sticky rule) for the rest of that first line, **but** since R6 was 0 at least once, border **does** become definitive/final once `C0=R1` is reached later on this same line.
- If C0=R1 is prevented from firing on this `C4=C9=0` line (e.g. by setting R1>R0) and R6 is no longer 0, border deactivates on the **following** line — i.e. the whole `C4=R6=0` conflict-border is cancellable if handled entirely within the first line.
- Enables byte-precise border/character alternation effects (the compendium's "Mode <write your nickname here>" joke) — achievable even with R0=0 per the text, though then C4/C9 can't count further.

**Type 1 conflict at R6=0 (§18.3.3, p.188):**
- Border activates for as long as **register value** R6=0, evaluated per-C0 (allows precise per-character targeting, same mechanism as normal R6=0 handling, §18.2.3).
- **However**, if the update to R6=0 happens to coincide with `C4=R6=0` (i.e. it's the first line-character of a new frame), the border becomes **true and sticky for the rest of the frame** (not just the alternation-style transient) — this is the key divergence from type 0's alternation behavior at the same coincidence point.

**Implementation note:** type 0 gives byte-level alternating border/display at `C4=R6=C9=0` conflict; type 1 gives a one-shot sticky border-for-rest-of-frame at the same coincidence. A model that treats R6=0 identically on both types will visibly diverge on this specific edge case.

---

## 19. Register R8 (skew/DISPTMG, interlace)

### 19.1 R8 bit layout (§19.1, p.189)

| CRTC | b7 | b6 | b5 | b4 | b3 | b2 | b1(i1) | b0(i0) |
|---|---|---|---|---|---|---|---|---|
| 0 | Sc | Sc | Sd | Sd | x | x | i | i |
| 1 | x | x | x | x | x | x | i | i |

- `i1:i0` (bits 1:0) = **Interlace mode**, same encoding both types: `00`=no interlace, `01`=Interlace Sync Mode, `10`=no interlace, `11`=Interlace Sync & Video Mode (IVM).
- `Sd` (bits 4:3) = **Skew DISPTMG** — **type 0 only** (bits are "x"/unused on type 1): `00`=non-skew, `01`=1-char skew, `10`=2-char skew, `11`=non-output.
- `Sc` (bits 6:5) = Skew CUDISP (cursor output skew) — type 0 only, same encoding; not used on CPC (no hardware cursor).
- **Type 1 has no SKEW-DISPTMG/SKEW-CUDISP function at all** — bits 3-7 are don't-care on type 1.

### 19.2 SKEW-DISPTMG functions — **type 0 ONLY** (§19.2, p.190-194)
These bits (Sd, R8 bits 3:2 in the `00xx` masks below) do not exist on type 1 — skip this whole section for a type-1 model except to confirm it's a no-op.

- **BORDER ON** (`R8 = 001100xx`, §19.2.1, p.190): forces DISPTMG OFF immediately (border shown regardless of R1/R6 state). VMA keeps incrementing; VMA' is still updated normally at `C0=R1 AND C9=R9`(sic — text says "C9=C0=0", likely a typo for the row-end condition; treat as the standard VMA' latch condition). Does **not** affect R6-border state — can still transition among the other 3 R8 sub-states. ⚠ Note in source: bits 0-1 (interlace) are apparently ignored while this is active — "requires further investigation," flagged by the author themselves.
  - Type-1 analog: setting R6=0 forces DISPEN off directly, but **unlike** the type-0 BORDER-ON function, if `C4=0` simultaneously, type 1's R6=0 hits the sticky/definitive-border path (§18.2.3) — a side effect type-0's BORDER-ON function avoids.
- **BORDER OFF** (`R8 = 000000xx`, §19.2.2, p.190): stops managing the other SKEW sub-functions (returns to normal C0=R1/C0=R0 comparator-driven border).
- **BORDER DELAY +1 / +2** (`R8 = 000100xx` / `001000xx`, §19.2.3, p.190-191): delays the R1-border transitions by 1 or 2 CRTC characters:
  - Baseline (no delay): DISPTMG-off (border start) at `C0=R1`; DISPTMG-on (border end) at the character following `C0=R0` (i.e. `C0=0`).
  - **Delay=1**: border ends on 2nd character after `C0=R0` (i.e. at `C0=1`); border starts on 1st character after `C0=R1` (i.e. at `C0=R1+1`). If R1=R0, border starts at `C0=0`.
  - **Delay=2**: border ends at `C0=2`; border starts at `C0=R1+2`. If R1=R0, border starts at `C0=1`.
  - The VMA'→VMA row-end/row-start latch timing (on `C9=R9`/`C0=R1`) is **unaffected** by this delay — only the visible border edges shift, not the pointer bookkeeping (§19.2.3, p.191).
  - With delay active, 1 or 2 *extra* characters are displayed at the right edge of the line (using addresses already fetched from the advancing VMA, which will be reloaded from VMA' at line start as usual).
  - **R8 changes affecting this delay take effect immediately, mid-line** (§19.2.3, p.191) — worked diagram at p.191 (R1=59,R0=63) shown but condensed here: writing R8 mid-line changes where the border edge appears on the *same* line, live.
- **No-condition case R1>R0** (§19.2.4, p.192): since `C0=R1` never fires, type 0 (and type 2) substitute `C0=R0` as the effective border-start trigger (spurious border byte, per §17.6.2). **Type 1 (and 3/4)** do not substitute anything — no BORDER-ON signal is sent at all in this configuration. When a SKEW-DISPTMG delay is programmed on type 0 in this R1>R0 configuration, the delay is still counted from C0 transitions as if the (never-reached) `C0=R1` condition had fired at the substituted point.
- **Disintegration of the border via double R8 write (type 0 only)** (§19.2.5, p.193-194): a 1-2 µs window exists (using SKEW-DISPTMG) during which **two back-to-back R8 writes on the same/adjacent line can cancel a spurious border byte** entirely — type 2 cannot do this (no SKEW-DISPTMG). Four documented sub-cases (§19.2.5.1-4, p.193-194) depending on exactly which C0 value the two OUTs land on relative to R0/R1 — condensed: **whether the border byte appears or is cancelled depends on which of the two R8 writes is "seen" by the comparator logic in the cycle it fires**, i.e. a last-write-wins-if-in-time race. ⚠ VERIFY p.193-194 if implementing this level of R8 detail — the 4 sub-diagrams each show a 1-cycle boundary case that's easy to mis-transcribe from the compressed text.

### 19.3 Interlace functions — general (§19.3, p.195-199)
- Two programmable interlace modes, same 2-bit encoding on all CRTC types: **Interlace Sync Mode** (`R8 bits1:0 = 01`) and **Interlace Sync & Video Mode / IVM** (`= 11`).
- Interlace works by delaying VSYNC by half a line on `C4=R7` for the even-numbered frame (MID-VSYNC, taking `C0=R0/2` as reference), plus adding **one extra scanline** at the end of the odd frame's construction (parity-dependent — see §19.5/19.6) so lines end up correctly ordered on a real interlaced CRT. On CPC, whether this actually produces a stable interlaced image depends on the Gate Array's HSYNC/VSYNC recombination logic (§16.6, out of scope here).
- **Interlace Sync Mode** (§19.3.2.1, p.196): same video data displayed on even and odd frames (doubles apparent resolution by filling gaps, no new data) — CRTC registers need no reprogramming between frames.
- **IVM / Interlace Sync & Video Mode** (§19.3.2.2, p.197-198): even frame shows **even C9** scanlines, odd frame shows **odd C9** scanlines — genuinely different data per frame-half. Registers must be programmed as if building a 624-line frame (the 625th line is handled automatically). Note: UM6845R datasheet figure describing this mode is **incorrect**; the UM6845(non-R)/HD6845S figure is correct (confirms UM6845 = type 0 lineage). Line 0 alternates with a border line in this mode (not with line -1, since there isn't one) due to the even frame's VSYNC repositioning being a full line ahead of the odd frame's.

### 19.4 Vertical interlace programming (§19.4, p.200-201) — per type register-value adjustments

**Type 0 (§19.4.1, p.200):**
- **Interlace Sync mode:** program R9 = N-1 (N = lines per character row), R4/R5/R6/R7 as if characters have N lines (unchanged from non-interlace).
- **IVM mode:** program R9 = N-2. R4/R6/R7 programmed as if characters have **half** as many lines. R5 unaffected (always a literal line count). If N is odd, a balancing algorithm applies (see §19.5.2). If R9=0 in IVM, means at least 2 total lines displayed (1 per frame-half).
- Example: 8-line character row → R9=6 in IVM.

**Type 1 (§19.4.2, p.200):**
- **Both modes:** program R9 = N-1 always (same formula in Sync and IVM — this is the key contrast with type 0/3/4, which use N-2 for IVM specifically).
- **Sync mode:** R4/R5/R6/R7 as if C4 characters contain N lines.
- **IVM mode:** R4/R6/R7 as if characters contain **half** as many lines; R5 unaffected. Odd-N balancing per §19.5.3.

### 19.5 Parity (§19.5, p.202-213)
Parity state determines: extra end-of-frame line, MID-VSYNC generation on even frames, and even/odd C9 calculation.

**Type 0 parity states (§19.5.2, p.202-205):**
- `ParityFrame` = `ParityR6` snapshot taken at `C4=C9=C0=0` (defines parity of frame's first C9).
- `ParityC9` = bit 0 of the C9 value used for VMA construction in interlace mode.
- `ParityR6 := ParityFrame XOR 1` when `C4` reaches `R6` — this anticipates next frame's parity. **If R6>R4** (C4 never reaches R6), ParityR6 stops updating and ParityFrame freezes at its last value.
- ParityR6 management is **independent of R8's value** (keeps running even outside interlace mode).
- C9 parity itself is only actively managed **when R8=3** (full IVM).
- When R9 is **even** (IVM mode, N-2 formula ⇒ odd total line-count N): parity is identical regardless of C4 (simple case).
- When R9 is **odd** in IVM: total line count per character is odd ⇒ line-count imbalance between frames is compensated by **alternating even/odd-only scanline sets per C4** within the same frame: `ParityC9 = C4.bit0 XOR ParityFrame` (computed whenever R8=3). On even frame, C9 parity tracks C4 parity directly; on odd frame it's inverted. This keeps the total-lines-per-C4 balanced (worked example p.203: R9=7, even frame C4=0 → 5 even lines then 4 odd lines = 9 total; odd frame C4=0 → 4 odd then 5 even = 9 total).
- **VSYNC delay-by-1-line correction**: if R7 lands on an **odd C4** in this odd-R9 balancing scheme, VSYNC is delayed by 1 line — occurs when `C4=R7 AND C9.VMA=2` on odd C4s — to avoid phase-shifting the VSYNC relative to its position on even frames (worked tables p.203-204). ⚠ VERIFY p.203-204: the two large parity/C4/C9/VSYNC tables are central to getting odd-R9 IVM right; recommend deriving a standalone truth table from the pseudocode below rather than transcribing the ASCII tables directly.
- Note (p.204): if R8 is switched to 3 on an **odd C4** rather than at frame start, the VSYNC-delay correction can itself become imbalanced for that transition frame only — self-correcting on subsequent frames.
- Determining current parity live: possible via the "counting bug" that appears when IVM is activated on `C9=R9` with odd parity, or deactivated on `C9.VMA=R9+1` (§19.8.1) — i.e. parity is externally observable through characteristic C9 miscounts at mode-transition boundaries.

**Type 1 parity states (§19.5.3, p.205-206):**
- Only **two** parity states (simpler than type 0's three): `ParityFrame` (toggles every frame at `C4=C9=C0=0`, regardless of R8) and `ParityC9` (toggles every C4 increment **only if R9 is even**).
- R9 even (odd total line count) ⇒ same even/odd-per-C4 alternation concept as type 0's odd-R9 case, but **triggered by R9-even instead of R9-odd** — this parity(R9) trigger condition is inverted vs type 0. Formula/pseudocode differs (see below).
- **Key type-1 vs type-0 divergence:** type 1 has **no VSYNC delay-by-1-line correction** for odd C4s — positioning R7 on an odd C4 when R9 is even **will** create a permanent 1-line VSYNC gap between even/odd frames (no self-correction mechanism) (worked table p.205, "EVEN FRAME"/"ODD FRAME" with `+32` markers).
- **R8 toggle timing is cycle-precise and documented exactly** (§19.5.3, p.206) — this is implementation-critical:
  - Updates occur on the **3rd and 4th µs of the `OUT(C),C`** instruction that writes R8.
  - **3rd µs**, R8 transitions 3↔0 (entering/leaving IVM): `ParityC9 := C9.bit0`; then `ParityC9 := ParityC9 XOR (C4.bit0 AND NOT R9.bit0)`. (Only applies when R9 even per the AND term.) Current ParityFrame is **not** touched at this point.
  - **4th µs, when IVM becomes active (R8→3):**
    ```
    if ParityFrame == EVEN:
        ParityC9 = C4.bit0 AND (NOT R9.bit0)
    ParityFrame = ParityFrame AND (ParityC9 XOR (C4.bit0 AND NOT R9.bit0))
    ```
  - **4th µs, when IVM goes idle (R8→0):** `ParityFrame := ParityC9`.
  - Net simple rule: toggling IVM on then off on an **even C9 line** always forces parity to EVEN, regardless of R9 — the easiest way to pin parity deterministically on type 1.
  - R9's bit 0 is read **immediately** by IVM activation (affects C9 right away); deactivating IVM can likewise instantly change C9 and the C4-row-end test.
  - ⚠ VERIFY p.207-209: extensive per-case truth tables ("SHAKER 22C/3" test-suite references) enumerate all combinations of `R9.bit0 × C4.bit0 × previous-C9.bit0 × initial-parity`; treat the pseudocode above as authoritative and use these tables only as regression-test fixtures if cycle-exact type-1 IVM parity is ever implemented.

### 19.6 Additional interlace line (§19.6, p.213-214)

**Type 0 (§19.6.1, p.213):** extra line appended at end of frame (after R5 lines) **iff** interlace active (R8=1 or 3) **and** `ParityR6` is odd. (ParityR6 odd ⟺ last time C4 reached R6 it was on an even frame — see §19.5.2.) If R6>R4 (ParityR6 frozen), the frame's extra-line behavior also freezes at whatever it last was — can force **every** subsequent frame to get (or never get) the extra line depending which state it froze in. C4 increments **only once** across all additional lines (R5 vertical-adjust lines + the interlace extra line combined) — final C4 = R4+1.

**Type 1 (§19.6.2, p.213):** extra line appended **iff** interlace active (R8=1 or 3) **and** `ParityFrame` is even (simpler condition than type 0 — no ParityR6 indirection). C4 increments each time `C9=R9`; additionally, if `R9+1` is a multiple of R5, C4 increments **once more** on even frames only (to keep the R4/C9 line accounting consistent under interlace).

### 19.7 MID-VSYNC (§19.7, p.215)
- General: VSYNC normally fires when `C4=R7` at any C0 (exception: CRTC3/4 require `C4=C9=C0=0` too — not applicable to type 0/1).
- With interlace active, on the frame where `ParityFrame` is even, VSYNC is deferred to fire when **`C0` reaches `R0/2`** instead of C0=0 — this is MID-VSYNC.

**Type 0 (grouped with 1, 2 in §19.7.2, p.215):**
- MID-VSYNC fires when `C4=R7` **and** `ParityFrame` is even (R8=1 or 3).
- `ParityFrame` assignment takes **priority over** VSYNC management: if R7=0, the `C4=C9=C0=0`-triggered ParityFrame flip is processed *first*, then the (now updated) parity is used for the C4/R7 comparison on the same instant — so R7=0 + even-after-flip parity **does** produce MID-VSYNC timing (`C0=R0/2`) even at the very start of frame.
- "MID-VSYNC therefore always takes place when ParityFrame is even, including when R7=0" — no exception on type 0.

*(Type 3/4 contrast, one line: on those, when R7=0, the C4/R7 VSYNC comparison is processed **before** the ParityFrame flip, inverting which frame gets the MID-VSYNC edge — not applicable to type 0/1.)*

### 19.8 Counting in Interlace Videomode — per-type C9 arithmetic (§19.8, p.216-231)

**Type 0 (§19.8.1, p.216-221):**
- `C9` (the "raw" counter) keeps incrementing normally regardless of IVM.
- For **address construction**, `C9` is logically shifted left 1 bit and OR'd with `ParityC9` to form `C9.VMA` — i.e. `C9.VMA = (C9 << 1) | ParityC9`; each raw C9 increment therefore advances the address-visible row-line count by 2.
- End-of-character-row test, from the line **following** the one where R8 went to 3:
  ```
  if ((C9*2) | ParityFrame) == (R9 + ParityFrame):     // test uses C9.VMA-with-frame-parity vs R9-with-frame-parity
      if C4 == R4:
          C4 = 0
          if ParityR6: ParityFrame ^= 1
      else:
          C4 += 1
      if R9.bit0 == 0:                    // R9 even -> parity flips per new C4
          ParityC9 = C4.bit0 ^ ParityFrame
      C9 = 0
      C9.VMA = (C9*2) | ParityC9
  else:
      C9 += 1
      C9.VMA = (C9*2) | ParityC9
  ```
- **On the very line where R8 transitions to 3**: the end-test uses the **old, un-doubled C9** compared directly against `R9 OR ParityFrame` (not C9.VMA) — deliberately, "to prevent the C9 used for display from switching mid-line." Parity itself, however, **is** considered immediately. This can cause **C9 to overflow past R9** on the transition line if `C9=R9` and parity is odd (test `C9 == R9+1` fails, so C9 doesn't reset and instead becomes `R9+1`).
- **On the line where R8 returns to 0** (leaving IVM): test switches to comparing `C9.VMA` (which already encodes parity) directly against plain `R9` (parity dropped from the R9 side) — i.e. `C9.VMA == R9`. This asymmetry (parity-adjusted R9 while entering IVM vs plain R9 while leaving) is the single most bug-prone edge in type-0 IVM and directly affects the `VMA'←VMA` row-end latch too, since that latch's `C9=R9` test shares this same C9/C9.VMA ambiguity (§19.8.1, p.216-217, explicit note).
- ⚠ VERIFY p.218-221: full worked tables for entering/exiting IVM on even/odd frames at every possible C9 offset when R8 changes — treat the pseudocode above as authoritative; use tables only as regression fixtures.

**Type 1 (§19.8.2, p.222):**
- Counting done in two stages, depends on R9 parity.
- **While in IVM (C0→0 boundary test):**
  ```
  C9 = C9 + (R9.bit0 == 0 ? 1 : 0)          // pre-increment only if R9 even
  if (C9 & 0b11110) == (R9 & 0b11110):       // compare ignoring bit 0 (parity)
      C4 management (increment, or C4=0 if C4==R4)
      if R9.bit0 == 0:
          ParityC9 ^= 1
      C9 = ParityC9
  else:
      C9 = C9 + 1 + R9.bit0
  ```
- **Once R8 returns to 0** (leaving IVM), counting resumes the plain non-interlace algorithm immediately: `if C9==R9: C9=0, C4 mgmt; else: C9+=1` — no lingering doubled/parity-adjusted comparisons the way type 0 has. This is notably **simpler** than type 0's exit behavior.
- No separate "C9.VMA" concept is introduced for type 1 in the text — C9 itself already carries the parity-adjusted value directly (unlike type 0's split C9/C9.VMA representation).

*(CRTC2/type3-4 IVM counting algorithms are documented in the source (§19.8.3-4) but are out of scope here — they use a distinct `C9.IVM` sub-counter concept not shared with type 0/1.)*

---

## 20. Registers R12/R13 (video pointer)

### 20.1-20.2 General & calculation (§20.1-20.2, p.237)
- R12(high)/R13(low) form the 14-bit frame-start address, combined with C9 to build the actual 16-bit VRAM pointer sent to the Gate Array:
  - VRAM pointer bit 0 = always 0 (CRTC operates in word/2-byte units).
  - VRAM pointer bits 1-10 = CRTC-VMA bits 0-9.
  - VRAM pointer bits 11-13 = C9 bits 0-2.
  - VRAM pointer bits 14-15 = CRTC-VMA bits 12-13 ("Overscan Bits", see §20.5).
- Whenever VMA/VMA' update conditions are met (per §17.4), `VMA[13:0] := {R12,R13}[13:0]`.

### 20.3 Update conditions per type (§20.3, p.238-239)

**Type 0 (§20.3.1, p.238):**
- Both `VMA'` and `VMA` are (re-)initialized from `{R12,R13}` when `C4=0 AND C9=0 AND C0=0` simultaneously.
- R12/R13 writes are **considered immediately** (no extra latch delay on the write itself — only the load-into-VMA moment is gated by the C4=C9=C0=0 condition above).
- Degenerate case noted: with R4=R9=0 and R0=1 ("2 µs frames"), C4 alternates 0/1 every 2 characters via the R5-additional-line mechanism, so this reload condition can recur **every 4 µs** — i.e. R12/R13 effectively become live-updatable at very high frequency in this extreme configuration.

**Type 1 (§20.3.2, p.238-239):**
- `VMA` (only — **not** VMA') is initialized from `{R12,R13}` whenever `C0=0 AND C9=0 AND C4=0` — textually the same trigger condition as type 0's, but **only VMA is touched, VMA' is untouched** by this path (consistent with the two-stage vs single-stage distinction already established in §17.4).
- R12/R13 writes considered immediately.
- **Known real-world consequence** (p.238, explicit game reference): because type 1 keeps applying `VMA=R12/R13` throughout the entire C4=0 character row (not just the very first scanline), software written for/tested against type 0 that changes R12/R13 "too early" while still nominally in row C4=0 can have that new address **prematurely override** the row it was meant for, on type 1 only. Cited bug: *007 The Living Daylights* (Domark, 1987) — vegetation/decor corruption from a score-zone address write landing during C4=0 on a type-1 machine. **This is a direct behavioral divergence a naive model must reproduce**: type 0 only re-applies R12/R13 at the single instant `C4=C9=C0=0`; type 1 re-applies it (to VMA, immediately) on **every** write that occurs anywhere within `C4=0` (any C9, any C0=0 boundary) — see worked timing diagram p.238 (`OUT R12,#30` mid-row, offset changes on the very next character despite still being within C4=0 on type 1, whereas the analogous type-0 diagram would only pick it up at exact `C4=C9=C0=0`).

*(Type 2/3&4 update-condition contrasts also documented at §20.3.3-4, p.239, out of scope — type 2 has the priority-bug already flagged in §17.4; type 3/4 match type 0's two-stage {R12,R13}→VMA'→VMA behavior.)*

### 20.4 Deadlines (§20.4, p.239)
- Text defers detailed cycle-accurate deadline tables to the R0-register chapter (not in this agent's page range — flag for cross-reference against whichever digest covers R0/R9, likely compendium-02 or similar). Diagrams on p.239 show OUT R12 timing relative to C0 approaching R0/R1 for type 0/1 — consistent with the C4=C9=C0=0 (type0) vs C4=0-any-C9/C0=0 (type1) rules above; no new rule beyond §20.3.

### 20.5 Overscan Bits (§20.5, p.240) — applies identically to type 0/1
- VMA is logically a 14-bit counter; bits 11-13 of the final VRAM pointer are supplied from **C9**, not from VMA, even though VMA internally counts through that range.
- When VMA's lower 10 bits overflow (carry out), the carry ripples into **R12 bits 2-3** — bits **not otherwise used** for the visible pointer construction — which are actually **VRAM pointer bits 14-15** ("Overscan Bits"). This is how the CRTC can autonomously page-flip across 16K boundaries without CPU intervention, given a big enough R1×R6 frame.
- R12 bit-pair encoding (also the CPC "start address" convention):

  | R12[7:6] | Start Address | R12[5:4] | Managed size |
  |---|---|---|---|
  | 00 | &0000 | 00 | 16K |
  | 01 | &4000 | 01 | 16K |
  | 10 | &8000 | 10 | 16K |
  | 11 | &C000 | 11 | 32K |

- Note: on CRTC3, R12 bit 3 is dual-purposed as the 8th bit of printer-port data (ASIC quirk, not applicable to type 0/1).

---

## 21. Read registers

### 21.1 General (§21.1, p.241)
Two independent read mechanisms exist: (a) reading back a selected register's stored value, and (b) reading an internal status word. Availability of both differs per CRTC type — **this is a primary CRTC-type fingerprinting surface.**

### 21.2 Reading register contents (§21.2, p.241-242)

Register select port: `&BC00` (both types). Register data port: `&BF00` (both types, for read).

**Type 0 (§21.2.1, p.241):** only 5 LSBs of the register-select number are decoded (upper 3 bits ignored ⇒ aliasing every 32). Readable registers at `&BF00`:

| Register | Contents |
|---|---|
| R12 | Display start address (high) |
| R13 | Display start address (low) |
| R14 | Cursor address (high) |
| R15 | Cursor address (low) |
| R16 | Light pen (high), read-only |
| R17 | Light pen (low), read-only |

- Any other register number (masked to 0-31) reads back **0**.
- R14/R15 (cursor, unused on CPC) can still be written and read back — useful as generic scratch storage, e.g. to stash a CRTC-type tag.

**Type 1 (§21.2.2, p.241-242):** also 5-LSB register decode. Readable registers at `&BF00`:

| Register | Contents |
|---|---|
| R14 | Cursor address (high) |
| R15 | Cursor address (low) |
| R16 | Light pen (high), read-only |
| R17 | Light pen (low), read-only |

- **KEY DETECTION FACT: type 1 CANNOT read back R12/R13** (they are absent from its readable set) — this is the single cleanest CRTC0-vs-CRTC1 discriminator via register readback. A model must NOT expose R12/R13 read data on type 1's `&BF00` port.
- Any register number **not** in {14,15,16,17,31} (mod 32) reads back **0** on type 1.
- **Register 31 special case (type 1 only):** reading register 31 (or any number whose bits 0-4 are all 1, mod-32 aliasing) returns a **non-zero** value — author observed 127 or 255 — described as "probably defined by UMC but not used on this model." This is the "dummy register" referenced in §21.4.

### 21.3 Reading status (§21.3, p.242-244)

**General (§21.3.1, p.242):** Only type 1 has a genuine status register, at dedicated port `&BE00`.

**Type 0 (§21.3.2, p.243):**
- **No status register exists at all.** Reading `&BE00` on type 0 is **explicitly documented as unreliable/undefined** for CRTC-type detection purposes — author's own hardware "randomly returns 255 or 127" on this port. ⚠ Do not model a deterministic status value for type 0's `&BE00` — the correct behavior is "unreliable/floating," not a fixed constant.

**Type 1 (§21.3.3, p.243):**
- Status register lives at `&BE00`.
- UMC docs mark bits 0-4 and bit 7 as unused; repeated reads return 0 on those bits (should be modeled as hard-wired 0, not floating).
- **Bit 5** = BORDER-R6 condition, but **only updated/latched at `C0=R0`** (not continuously): `0`=false (`C4=C9=C0=0` — i.e. not in R6-border), `1`=true (`C4=R6 AND C9=C0=0` at the moment of the C0=R0 check). ⚠ Important nuance: bit 5 being 0 does **not** guarantee characters are currently displayed — border can still be active via the independent R1-condition; bit 5 only reflects the R6-specific condition, sampled once per line.
- Also: if R6 is set to 0 while C4>0 specifically to *force* border (the "R6=0 special case" from §18.2.3/18.3.3), that state is **not reflected** in status bit 5 — bit 5 stays 0 in that case (since the underlying `C4=R6` equivalence, which is what bit 5 actually tracks, is not what's driving the border here). ⚠ VERIFY p.243-244: two worked timing diagrams show bit 5's transition to 1/0 exactly at the C0=R0 sample point across a VSYNC boundary — condensed correctly above, but re-derive against C0=R0 sampling in the Verilog rather than trusting the ASCII diagram positions verbatim.

*(Type 3/4 status registers R10/R11 documented in detail at §21.3.4, p.243-245 — out of scope for type 0/1 modeling; noted only because §21.3.1 states the &BE00 port mirrors &BF00 on those types, unlike type 0's floating/type 1's dedicated-status behavior.)*

### 21.4 Dummy register 31 (§21.4, p.244)
- Register 31 is a genuine addressable register **only on UM6845E** (not present on CPC — different CRTC lineage than types 0/1/2/3/4 used on Amstrad hardware). Included here only for the type-1 cross-reference:
- **Type 1 (UM6845R):** register 31 does not functionally exist, but bit 7 of "the status register that would refer to it" (i.e., reading register 31 via the normal register-read path, not the status path) returns **non-zero** — this is the same fact as §21.2.2's "register 31 returns 127/255" behavior. This is NOT the status register bit 7 from §21.3 — it's the register-content read path.
- **Type 0 (UM6845):** no status register exists at all (confirmed again here), so "bit 7 of the status register" is moot — register 31 read simply falls into the generic "unrecognized register → 0" rule from §21.2.1.

---

## 22. Other registers (R10/R11 cursor, R14-R17)

(§22, p.246 — brief chapter, not much CRTC-type-specific behavior beyond what's already covered in §21.)

- R10 (cursor start/blink) and R11 (cursor end) are **not implemented in the CPC's rendering path** (no hardware cursor is used on CPC text/graphics modes) — this chapter doesn't cover their internal operation in detail.
- Practical CPC-relevant points:
  - Any register that isn't wired to CPC video hardware (R10/R11 cursor control, R14/R15 cursor address) can still be **written and read back** as generic storage — useful, e.g., to tag CRTC type in software without affecting display, *provided* the CRTC type in question supports readback of that register (see §21.2 tables — R14/R15 readable on both type 0 and type 1; R10/R11 readability differs — not listed as readable on either type 0 or type 1 per the §21.2 tables, only appearing in the type 3/4 table).
  - Pin 19 (CUDISP/CURSOR) → expansion port pin 46 (some expansions use this).
  - Pin 3 (LPSTB/LPEN, light pen strobe) → expansion port pin 47.
- No dynamic-update or type-divergence rules are given for R10/R11/R14-R17 beyond the readback-table facts already captured in §21.2.

---

## 28. CRTC identification (concrete test cases — type 0 vs type 1)

Each subsection below is a runnable acceptance-test recipe: **I/O sequence → expected result, type 0 vs type 1.**

### 28.1.1 Via C4/C9 overflow (§28.1.1, p.288)
- **Test:** program a frame with `R4=36, R9=7, R5=16` (total 312 lines: `(36+1)*(7+1)+16=312`), then vary R7 upward and observe whether VSYNC still fires (`C4` reaching `R7`).
  - **Type 0:** VSYNC stops occurring once `R7>37` (C4 overflows/repeats past R4 exactly once in extra management, extending the effective C4 range by 1 vs type 1/2).
  - **Type 1:** VSYNC stops occurring once `R7>39` (C4 overflow happens multiple times, extending range further — grouped with type 2 in the text).
  - **Discriminator:** boundary at R7=37 (type 0) vs R7=39 (type 1) for this specific R4/R9/R5 combination — a clean numeric test.

### 28.1.3 Via VSYNC activation timing when C0>0 (§28.1.3, p.288)
- **Test:** force `C4=R7` to become true while `C0>0` (mid-line), and observe the VSYNC line-counter's starting value.
  - **Type 0:** VSYNC line-counter starts from **0**.
  - **Type 1:** VSYNC line-counter starts from **1**.
  - (CRTC3/4 contrast: no VSYNC occurs at all in this condition — clean 3-way discriminator if needed, but not required for 0-vs-1.)

### 28.1.4 Via VSYNC length (§28.1.4, p.288)
- **Test:** program R7 with a value C4 reaches only **after** C0 already exceeds 0 that line (i.e. R7 latched "late"), then measure resulting VSYNC pulse length.
  - **Type 0:** type 0 (grouped with 3/4) supports **programmable** VSYNC length (via R8's related mechanism / documented elsewhere) — VSYNC length is not fixed at 16 lines in this scenario.
  - **Type 1:** VSYNC that starts "late" (R7 programmed before C4=R7 is reached) **always lasts exactly 16 lines**, non-programmable in this scenario.
  - **Discriminator:** measure VSYNC active duration under this trigger condition — 16 lines fixed (type 1) vs type-0's variable-length behavior.

### 28.1.5 Via HSYNC length with R3=0 (§28.1.5, p.288)
- **Test:** program `R3=0` (HSYNC width field) and observe whether any HSYNC pulse (and consequently the CPC's raster interrupt, which is HSYNC-driven) occurs at all.
  - **Type 0:** **no HSYNC occurs** when R3=0 — i.e., **no CPC interrupts fire** either, since the Gate Array interrupt counter is clocked by HSYNC.
  - **Type 1:** same as type 0 — **no HSYNC occurs** when R3=0 (grouped together in the text, contrasted against type 2/3/4 which **do** produce a 16 µs HSYNC when R3=0).
  - **Note:** this test does **not** discriminate type 0 from type 1 (both behave identically) — useful only to rule out type 2/3/4. Listed here because it's easy to mis-apply as a 0-vs-1 test; it is not.

### 28.1.6 Via border, visually (§28.1.6, p.289)
- Relevant type-0-specific facts (type 1 does not exhibit these): type 0 exhibits the `R6=0`-at-`C4=C9=0` byte-alternation conflict (§18.3.2); type 0 creates a spurious border byte when `C0` reaches `R0` with `R1>R0` (§17.6.2); type 0 can suppress/delay border via R8 SKEW-DISPTMG bits, which **do not exist on type 1** (§19.2).
  - **Test:** set `R1=R0+1`, observe an extra ~0.5µs border byte between character rows on type 0 that is **absent** on type 1 (§17.6.2/28.1.6 combined) — direct visual/logic-analyzer discriminator.
  - **Test:** write `R8=0x0C` (BORDER-ON, `001100xx`) on type 0 → display forces off immediately; the identical write on type 1 is a **no-op** (bits 3+ are don't-care, only bits 0-1 interlace mode apply) — reading back R8 won't help since it's write-only either way, but the *visual/DISPTMG effect* differs and is externally observable.

### 28.1.7 Via interlace mode C4-counting rate (§28.1.7, p.289)
- **Test:** enter IVM (R8=3), set `R9=6` on type 0 or `R9=7` on type 1 (each type's respective "N-2"/"N-1" formula for an underlying 8-line character), **without changing R7**, and measure VSYNC frequency.
  - **Expected on both type 0 and type 1:** VSYNC now occurs **twice as fast** as before entering IVM, because `C4` reaches `R7` twice as fast (rows are now 4 lines instead of 8 from C4's perspective). This confirms "is this CRTC 0 or 1" as a *pair* (both behave the same way) versus type 2, which does **not** speed up C4 counting under IVM — so this test discriminates {0,1} vs {2}, not 0 vs 1 specifically. Included here because the source groups it under "CRTC identification" generally; **not** a 0-vs-1 discriminator by itself.

### 28.1.8 Via status register at &BE00 (§28.1.8, p.289) — PRIMARY 0-vs-1 TEST
- **Test:** read `&BE00` repeatedly at a precisely-timed instant (specifically: testing the transition of bit 6, described as always-1 per §21.3... cross-check bit numbering against §21.3.3 which discusses bit 5 for BORDER-R6; the identification chapter references "bit 6" transition testing — likely referring to a fixed/always-1 bit used as a liveness check that only a genuine status register would hold stable, or possibly a documentation-numbering mismatch between chapters. ⚠ VERIFY p.289 cross-reference against p.243 bit numbering before hard-coding bit index).
  - **Type 1:** `&BE00` is a genuine, live status register — the targeted bit transitions in a well-defined, reproducible way tied to CRTC internal state (per §21.3.3's bit-5 BORDER-R6 rule, sampled at C0=R0).
  - **Type 0:** `&BE00` has **no status register** — reads are described elsewhere (§21.3.2) as returning "randomly 255 or 127" — i.e. floating bus / undefined value, NOT a value that transitions in sync with any CRTC condition.
  - **Concrete acceptance test:** poll `&BE00` across many frames at a fixed timing relative to a known raster position. **Type 1** must show bit 5 flipping deterministically at `C0=R0` in sync with the programmed R6/C4 state (§21.3.3). **Type 0** must show a value that does **not** correlate with any internal CRTC state (model this as returning the floating-bus/last-driven-value byte, e.g. from a prior OUT to the address/data bus, rather than a fixed constant — do not hardcode 0xFF).

### 28.1.9 Via register read at &BF00 — PRIMARY 0-vs-1 TEST, register-set comparison (§28.1.9, p.289)
- **Test:** select each register 0-31 via `&BC00`, then read `&BF00`, and tabulate which numbers return non-zero.
  - **Type 0:** non-zero (real) data returned for R12, R13, R14, R15, R16, R17 (mod-32 aliasing — e.g. selecting register 108 behaves as register 12 since `108 mod 32 = 12`). All others return 0.
  - **Type 1:** non-zero data returned for R14, R15, R16, R17 **only** — **R12 and R13 read back as 0** (this is the headline discriminator — type 1's R12/R13 are write-only). Additionally, register 31 (and anything aliasing to bits 0-4 all set, mod 32) returns a **non-zero garbage-ish constant** (127 or 255 observed) even though it's not a "real" register — this is unique to type 1 and doubles as a second independent signal.
  - **Concrete acceptance test:**
    1. `OUT &BC00, 12` / `OUT &BC00, 13` with known R12/R13 values previously written; `IN A,(&BF00)`.
       - Type 0 expected: readback equals last-written value.
       - Type 1 expected: readback = 0, regardless of what was written.
    2. `OUT &BC00, 31`; `IN A,(&BF00)`.
       - Type 0 expected: 0.
       - Type 1 expected: non-zero (127 or 255 per hardware sample).

### 28.1.10 Via R10/R11 status registers (§28.1.10, p.289)
- Type 3/4 only — not applicable to type 0/1 discrimination. Noted only to avoid confusing it with the &BE00/&BF00 tests above, which ARE the correct type-0/1 discriminators.

---

## 29. CPC identification (§29, p.290)

Out of CRTC-type scope proper, but included per the assignment brief since it affects which CRTC type a given machine is paired with:

- **CPC PLUS**: ASIC 40489, paired with **CRTC 3**.
- **CPC 6128 PLUS "LowCost"/GX4000-derived boards**: ASIC 40226, paired with **CRTC 4**.
- Neither is type 0 or type 1 — both use ASIC-integrated CRTC variants (out of this doc's scope), but the identification methods below are useful to rule out "am I even looking at a type 0/1 machine" before applying the §28 tests:

### 29.1.1 Extended-features unlock (§29.1.1, p.290)
- **Test:** send the CPC Plus ASIC unlock sequence to the CRTC I/O port.
  - CPC PLUS: extended features activate.
  - CPC LowCost and all standard CPCs (which is where type 0/1 CRTCs live): **no extended functions activate**, sequence is a no-op. **A machine that responds to this sequence is never a type-0/1-CRTC machine** — useful as a pre-filter before running the §28 type-0-vs-1 tests.

### 29.1.2 PPI Port C bug (§29.1.2, p.290)
- **Test:** use the 8255 PPI command register to configure port C, then check whether port C's output bits reset to 0.
  - CPC PLUS: bits **do not** reset to 0 (ASIC's PPI emulation bug).
  - CPC LowCost and standard CPCs (type 0/1 territory): bits correctly reset to 0 (real 8255 PPI, or LowCost's case, no PPI-command-register misbehavior noted for this specific test — LowCost has a real PPI too per §29.1.3).

### 29.1.3 PPI Port B bug (§29.1.3, p.290)
- **Test:** program port B as output, write a value, read it back.
  - Standard CPCs (type 0/1 territory) and (per the standard 8255 spec) CPC PLUS: read-back should return the last-written value.
  - CPC LowCost: reportedly reads back the **input-pin** value regardless of the programmed direction (PPI misbehavior) — flagged by the author as unverified/needing confirmation of which physical PPI chip was in the tested unit.
  - Not relevant to type-0-vs-1 discrimination directly, but confirms machine identity if a LowCost board is suspected.

---

## Summary: highest-risk rules for a naive behavioral model

1. **R12/R13 → VMA/VMA' load path differs structurally between types** (§17.4, §20.3): type 0 is `{R12,R13} → VMA' → VMA` gated strictly at `C4=C9=C0=0`; type 1 is `{R12,R13} → VMA` directly, gated at `C4=0` for **any** C9/C0=0 boundary within that row — not just the frame's very first scanline. Modeling both with a single "load at C4=C9=C0=0" condition breaks type 1's documented per-scanline offset capability and misses the *007 Living Daylights* class of bug.
2. **Type 1's register readback of R12/R13 must return 0**, not the stored value — this is the primary software CRTC-detection vector (§21.2.2, §28.1.9) and is trivial to get backwards if the model reuses one generic "register read" path for both types.
3. **Type 0's `&BE00` (status) must be modeled as undefined/floating, not a fixed constant** — real hardware returns 127 or 255 unpredictably; hardcoding either value could make a detection routine falsely identify a type-0 emulation as type 1 or vice versa (§21.3.2, §28.1.8).
4. **R1>R0 border-byte generation is type-dependent**: type 0 synthesizes a spurious 0.5µs border pulse keyed on `C0=R0` (with SKEW-DISPTMG able to suppress/delay it); type 1 emits **no** such pulse at all (§17.6.2, §19.2.4).
5. **R6=0 conflict behavior at `C4=R6=0,C9=0`** diverges sharply: type 0 alternates border/display byte-by-byte for that whole line; type 1 latches a **sticky border for the rest of the frame** (§18.3.2 vs §18.3.3).
6. **Dynamic R1 rewrite timing at the exact `C0=R1` instant** is a genuine race (write lands before vs. after the comparator's evaluation edge) that changes whether the VMA'←VMA row-end latch fires that pass (§17.3/17.4, p.179-180) — easy to get "off by one cycle" in RTL.
7. **IVM entry/exit C9 arithmetic differs fundamentally**: type 0 maintains a split `C9`/`C9.VMA` representation with an asymmetric entry-vs-exit comparison rule (parity-adjusted R9 on entry, plain R9 on exit) that can cause C9 to visibly overflow past R9 on the transition line; type 1's algorithm is simpler and has no such split-representation asymmetry (§19.8.1 vs §19.8.2).
8. **Type-1 R8 IVM parity updates are cycle-position-specific** (3rd vs 4th µs of the OUT instruction) with different formulas at each — a model that applies both formulas simultaneously, or at the wrong µs boundary, will desync parity from real hardware (§19.5.3).
9. **VSYNC-delay-by-1-line self-correction for odd R9 in IVM exists on type 0 but NOT on type 1** — type 1 will show a genuine, uncorrected 1-line VSYNC drift between even/odd frames under those conditions; a model that "helpfully" applies type-0's correction logic to type 1 will produce output that never appears on real type-1 hardware (§19.5.2 vs §19.5.3).
10. **C0=R1=0 acknowledgment deadline (§17.5)** and the R1 dynamic-update JIT edge case are both under-specified in the extracted ASCII diagrams — flagged ⚠ VERIFY, recommend deriving the exact cycle boundary from the Verilog's own C0 comparator stage rather than the compendium's compressed tables.

## Pages flagged ⚠ VERIFY (poor extraction / compressed diagrams, re-check against PDF directly if implementing that rule)
- p.180 — R1 dynamic-update JIT-write example (VMA' pointer repeated 960 vs 1000 tables)
- p.182 — R1=0 acknowledgment deadline diagram (exact "too late" vs "just in time" C0 boundary for type 0/1)
- p.193-194 — R8 SKEW-DISPTMG double-write "border disintegration" 4 sub-cases (type 0 only)
- p.203-204 — Type 0 odd-R9 IVM parity/VSYNC-delay worked tables
- p.207-209 — Type 1 R8-toggle parity truth tables (SHAKER test-suite cross-references)
- p.243-244 — Type 1 status register bit 5 transition diagrams (BORDER-R6 sampling at C0=R0)
- p.289 — CRTC identification "via status register &BE00, bit 6" (bit-index cross-reference against §21.3.3's bit-5 discussion is ambiguous in the extracted text)
