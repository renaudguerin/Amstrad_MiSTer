# F14/F15 independent review — Codex (roster-codex exec), 2026-08-26

Scope: the branch diff `779abef..4819312` (F14 fixtures+RTL, F15
fixtures+RTL, docs) on `accuracy/f14-f15-interlace`. Reviewer: Codex via
`~/.claude/bin/roster-codex exec` (workspace-write sandbox), fresh
session, with the rendered ACCC pages named as the primary sources and
the gate/soak state stated in the brief. The reviewer independently
reproduced the gate (166 passes), lint, and soak `0x85b3f8e847430495`.

Verdict: **NOT CLEAR** on two blockings, plus two non-blockings. All four
remediated at `fe6876c`; each remediation carries its own bite test.

1. **BLOCKING — stale F15 delay state across a live type switch.**
   `e0_vsync_delay_suppress` was not type-qualified: an armed d1 clears on
   the first CLOCK edge after CRTC_TYPE rises, but the wrapper samples the
   pre-edge value on that same edge, so a type-1 field count tick landing
   exactly there lost its natural fire. Remediated: both delay outputs
   qualified with `!CRTC_TYPE` in the engine; new vector `t29e` arms d1,
   rewrites R8 to 0, switches type after phase 15, and requires the
   type-1 natural fire on the immediately following count tick (R0=3 puts
   the `hcc_next==R0/2` tick exactly there). Bite: dropping the qualifier
   fails exactly `t29e`.

2. **BLOCKING — odd-R9 switch-line rule unexercised.** Every t29 fixture
   entered IVM at reset and the t22 switch family is even R9, where the
   addition and OR target forms coincide; a broken odd-R9 `tog_enter`
   addend passed the suite. Remediated: new vector `t29d` pins the p.219
   overflow sentence — switching to IVM on the raw C9=7 line of an odd
   frame with R9=7 must not end the row (target R9+ParityFrame=8); C9
   overflows to 8 with the doubled display at C9.VMA=17. Bite: zeroing the
   odd-R9 addend fails exactly `t29d`.

3. **NON-BLOCKING — t29a-c still used fixture-only `expect_xfail_*`
   helpers** after their registry flags flipped to required (an artifact
   of a botched edit that was itself caught and repaired). Converted to
   plain expects.

4. **NON-BLOCKING — type-1 F14 only exercised at R8=3.** New vector
   `t28c` runs the same mechanism through INTERLACE SYNC (R8=1, R5=4,
   R9=7). Bite: restricting the gate to `R8==3` fails exactly `t28c`.

Post-remediation state: gate 169 passed / 0 failed, lint clean, soak
unchanged at `0x85b3f8e847430495` (the qualified one-edge window is
unreachable by the soak's tick phases).

The reviewer's positive findings, recorded for the next session: the
implemented F14/F15 counter rules match the rendered sources; the
positional engine instantiation aligns after the mid-list port insert;
the add-line flops are correctly CLKEN-gated (the reviewer specifically
checked the ungated-flop oscillation class); the even-R9 target form
reduces algebraically to the old OR form; and type-1 F14 is structurally
unreachable at R5=0, which is what keeps the t21-t24 walks undisturbed.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by
Longshot (CC BY-NC-ND).
