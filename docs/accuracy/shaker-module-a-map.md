# SHAKER Module A: test map and coverage gaps

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

Working document. Purpose: turn "no Module A progress" into a per-entry statement, and choose
the next implementation on evidence rather than on the order of the audit table.

Sources, both now firsthand rather than inferred:

- `shaker/menu-transcriptions.md` — the module menus read off a running SHAKER 2.6.
- `shaker/shaker-accc-crossref.md` — every menu entry in modules A–E mapped to its ACCC v1.10
  section and page. That file is dated evidence and its citations are **not** independently
  re-checked; confirm a quote against the PDF before turning it into RTL or a vector.

Modules B–E are covered only in the cross-reference. This file stays focused on Module A
because that is what has been run on hardware.

## How Module A results must be judged

Results are read by **visual comparison against Logon System's reference photographs of real
hardware**, published per CRTC type on `shaker.logonsystem.eu`. There is no numeric score and
no published pass/fail table, and the ACCC explains the behaviour without stating the expected
picture per test.

This invalidates how the first two hardware sessions were run. Comparing our core against the
stock core answers "did anything change", not "is it correct": if both cores are wrong the same
way the pictures match and the session reports no progress, which is what was observed at
`1a1233f` and `5ddddef`. The reference photograph for the selected CRTC type is the oracle; the
stock core is only a regression baseline.

## Module A entries, with our coverage

Names and test counts are verbatim from the menu. ACCC sections are from the cross-reference.

| Entry | Tests | ACCC | Our coverage |
|---|---|---|---|
| **(5)** R13 UPDATE IN 4 USEC SCREENS (R0=3) | 5 | §13.8.1 p.127, §20.3 p.242 | **Mechanism vectors exist**: `t20c`/`t20d` (both types, R0=3, MA asserted per line, commit `90aed07`). Unproven: the 5-phase Z80 write alignment the entry probes. |
| **(6)** R13 UPDATE IN 2 USEC SCREENS (R0=1) | 5 | §13.8.2 p.128, §13.2.5 p.107 | **Mechanism vectors exist**: `t20e`/`t20f`. Same residual gap. |
| **(7)** R13 UPDATE IN 1 USEC SCREENS (R0=0) | 5 | §13.8.3 p.129, §13.2.6 p.108 | **Mechanism vectors exist**: `t20g`/`t20h` (type 0 ignores reload after the hiccup; type 1 reloads every line). Same residual gap. |
| **(U)** R4 & R9 CHECKING | 54 | §10.3 pp.74–79, §12 pp.92–101 | **Implemented.** This is the F4/F12 work just hardware-tested. |
| **(I)** VSYNC CONDITIONS | 413 | §16.4 pp.168–170, §15.4 pp.152–154 | **Implemented**, via F3 and F11b. Largest entry in the module by a wide margin. |
| **(P)** R6 STORIES | 13 | §18 pp.188–191 | Partial, via F4 equality-only rollover and F12. |
| **(O)** R1 STORIES | 8 | §17 pp.175–187, §17.6 pp.185–186 | **Deferred by decision** (F6). The type-0 half-character border byte cannot be expressed through the character-granular CRTC-to-Gate-Array interface. |
| **(T)** R2 UPD DURING & AFTER HSYNC | 6 | §15.3 pp.148–151 | Believed correct via F11a, but only width semantics were reasoned about. No during-pulse vectors. |
| **(Y)** R3 UPD DURING HSYNC | 8 | §14.4 pp.134–140, §14.4.4 pp.138–140 | Same as (T). R3.JIT sub-microsecond behaviour untested. |
| **(4)** UPDATE CRTC R0 TIMING | 17 | §13.6 pp.122–123, §4.4.3 p.26 | Partial, via F5/F12. Probes `OUT (C),r` versus `OUTI` phase, which our vectors do not distinguish. |
| **(2)** SKEW DISP ON R0 RUPTURE | 4 | §19.2 pp.193–197 | Partial. Skew path is F11g and believed correct; the R0-rupture interaction is untested. |
| **(TAB)** HSYNC START POSITION | 4 interactive | §14.6 pp.141–142 | Untested. Sub-microsecond blanking start differs per type (CRTC 0 pixel 5, CRTC 1 pixel 6). |
| **(1)** UPDATE VRAM VS CRTC | 79 | §8 pp.43–44 | **Out of reach.** Z80 write versus fetch phase inside the 1 MHz cycle; not representable at the current character-granular interface. |
| **(8)** GATE ARRAY PIXELISATION | — | §9.1 pp.46–47 | **Out of CRTC scope** (F11i). Lives in the netlist-derived `GA40010`. |
| **(9)** GATE ARRAY INKERISATION | 3 | §9.2 pp.48–50 | Out of CRTC scope. |
| **(E)** GATE ARRAY MODERISATION | — | §9.3 pp.51–52 | Out of CRTC scope. |
| **(R)** MODE UPD >HSYNC DELAY< (2.1.0) | 3 | §9.3.1 p.51, §14.3 p.132 | Out of CRTC scope apart from correct HSYNC edges. |
| **(CAPS)** INTERACTIVE TEST MODE X TO Y | 16 interactive | §9.3.4 pp.53–72 | Out of CRTC scope. Gate Array pixel residue across mode splits. |
| **(3)** CRTC 2 RVMB | 22 | §17.4.3 pp.183–184 | **Out of project scope.** CRTC 2 only. |
| **(COPY)** CRTC 2 OFFSET | — | §17.4.3 p.183, §20.3.3 p.243 | Out of project scope. |

Module A totals over 600 individual tests. Roughly two thirds sit in `(I) VSYNC CONDITIONS`
alone, and a further 79 in `(1)`, which we cannot reach at all.

## Why the flat hardware result is consistent

Three independent reasons, none of which implies the F4/F12 work is wrong:

1. The oracle was the stock core, not the reference photographs, so shared inaccuracy is
   invisible.
2. "A few tests" is a very small sample of 600+, and the entries actually moved by F4/F12 are
   `(U)` and parts of `(P)`.
3. Of the 20 entries, 8 are Gate Array, CRTC 2, or sub-character scope and cannot move at all
   from type 0/1 CRTC work.

## R12/R13: the strongest next candidate

Entries (5), (6) and (7) are 15 tests probing one mechanism at three character widths, and the
ACCC rules for them are unusually crisp.

**R0=3, 4 µs frames.** Types 0 and 1 behave identically. C0 reaches 2, so CRTC 0 passes its
C0=2 evaluation and disarms the default vertical adjustment (R5=0). C4 stays 0, and VMA takes
a new R12/R13 on each C0=0.

**R0=1, 2 µs frames.** The types diverge. C0 never reaches 2, so CRTC 0's disarm check never
runs and an *uncancelled* one-line vertical adjustment fires every frame, leaving C4=1 for the
second 2 µs period. ACCC §13.2.5 p.107: R12/R13 can therefore be considered only every 4 µs on
CRTC 0, against every 2 µs on types 1, 2, 3 and 4.

**R0=0, 1 µs frames.** The types diverge further. On CRTC 0, C0 never reaches 1, so C9 freezes
at whatever value it held, C4 takes one last increment ("C4's last hiccup", §13.2.6 p.108) and
then every counter but C0 stops. With C4 stuck at 1 the C4=0 condition never recurs and
R12/R13 writes are ignored entirely. On CRTC 1, C9 and R4 keep being managed, C4 stays 0, and
VMA reloads on every 1 µs line (§13.3 p.113, §20.3.2 p.242).

### Where our RTL actually stands

Updated 2026-08-22 (P3 audit; earlier line refs had drifted and the R12/R13 gap is closed at
mechanism level). The general-case reload rules are right, the degenerate-case mechanisms are
present from the F5/F12 work, and `t20a`-`t20h` (commit `90aed07`) now assert MA across
normal frames and R0=3/1/0 on both types:

- `rtl/UM6845R.v:433` `CRTC0_reload = ~CRTC_TYPE & frame_new` — type 0 reloads at frame start.
- `rtl/UM6845R.v:430-432` type 1 reloads while C4=0 (`crtc1_row0_reload`) plus the ACCC
  §11.2.4 adjustment-entry path loading VMA from R12/R13 while C4==1 — matching §20.3.2.
- `rtl/UM6845R.v:196` `r0_frozen = !CRTC_TYPE && !R0_h_total && !hcc` plus
  `type0_r0_zero_entry_consumed` implement the R0=0 freeze and the last-hiccup increment.
- `rtl/UM6845R.v:392` `if(hcc == 2) frame_adj_r <= frame_adj_r & |type0_effective_r5;` is the
  C0=2 disarm check, so at R0=1 it correctly never runs and the adjustment stays armed.

What the local vectors do **not** cover is the entry's remaining dimension: the five Z80
instruction-phase alignments (`OUT (C),r` 3rd µs vs `OUTI` 5th µs). The harness models write
timing as a character boundary plus a ±1-tick offset, which distinguishes JIT-vs-late but not
all five phases. So this is now **partial coverage**: mechanism proven, phase alignment not.

## Next actions

1. ~~Write R12/R13 reload vectors before touching RTL~~ Done: `t20a`-`t20h` landed in
   `90aed07`. Remaining sub-scope: instruction-phase alignment if a hardware divergence
   points there.
2. Only if a vector or hardware result fails, open a finding and fix it.
3. Next hardware session: record every entry run by name with its result, set the OSD CRTC
   selection deliberately, confirm the footer reports the type selected, and judge against
   the Shakerland reference photographs for that type. Target list for the next session:
   Module A `(U)`, `(P)`, `(5)`-`(7)`; Module E `(3)`, `(2)`, `(1)`; Module B `(RETURN)`;
   Module D `(E)` — see `current-status.md` and the roadmap's per-checkpoint SHAKER list.
   SHAKER sessions stay manual and milestone-targeted; they are not part of the automated
   Verilator + CI loop.
4. Cheap direct check of shipped work: Module E `(3) CRTC 0 C4/C9 COUNTER LOGIC BUG`
   (§10.3.1.2 pp.75–76, §11.2.2 pp.81–82) targets exactly the F4/F12 counter logic. Module E
   `(2)` targets CRTC 1 VMA treatment on adjustment lines, adjacent to the R12/R13 work above.
