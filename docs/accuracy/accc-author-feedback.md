# ACCC v1.11 — Author Feedback & Clarifications (Round 2)

**Date:** August 28, 2026
**Reference:** *The Amstrad CPC CRTC Compendium* v1.11 (Longshot / Logon System, 27 August 2026)
**Target:** Longshot (Author of *The Amstrad CPC CRTC Compendium*)

This document compiles outstanding questions and technical clarifications gathered during the ongoing hardware accuracy audit and FPGA implementation of the MiSTer Amstrad core against **ACCC v1.11**.

*Note: Historical Round 1 feedback (which resulted in the publication of ACCC v1.11) is archived in [accc-author-feedback-round1-2026-08-27.md](accc-author-feedback-round1-2026-08-27.md).*

Technical information sourced from *The Amstrad CPC CRTC Compendium* by Longshot (CC BY-NC-ND 4.0).

---

## 1. §19.5.2 (p. 206) — Subsequent-Frame Recovery After Odd-$C_4$ IVM Activation

* **Location:** Chapter 19.5.2 (p. 206, Note).
* **Context in ACCC v1.11:**
  The Note in §19.5.2 discusses the effect of activating Interlace Video Mode ($R_8 \to 3$) on an odd character row ($C_4$):
  > *"If R8 goes to 3 on an odd C4, this can cause a phase of 1 line between the VSYNC of even and odd frames. Indeed, this VSYNC shift technique… only works properly to manage the line imbalance between an even C4 and an odd C4."*
  The text provides a worked example showing the line counts during the transition frame (e.g. $C_4=0$ identical at 8 lines; $C_4=1$ receives 5 even lines on an odd frame vs 4 odd lines on an even frame).
* **Question for Author:**
  The Compendium documents the transition-frame perturbation in detail, but is silent on what happens on subsequent frames.

  Does physical silicon immediately recover the steady-state interlace cadence on the very next frame (because parity state bits such as `ParityFrame` are re-anchored at each $C_4=C_9=C_0=0$ frame origin and `ParityC9` is recomputed per character end without carrying forward any timing phase shift), or does any internal latch state persist across subsequent frames?

* **Current Implementation Behavior:**
  Our core implements a memoryless parity model where parity state re-evaluates at the frame origin, causing the system to self-correct immediately after the transition frame and resume the steady 625-line cadence. We would appreciate confirmation of whether this matches observed hardware behavior.

---

## 2. §11.3.2 (p. 85) — $C_4$ Counter Behavior During Stuck $R_5=0$ Vertical Adjustment (CRTC 1)

* **Location:** Chapter 11.3.2 (p. 85, Type 1 Vertical Adjustment).
* **Context in ACCC v1.11:**
  In §11.3.2, describing what occurs on Type 1 (UM6845R) when $R_5$ is rewritten to 0 during active vertical adjustment:
  > *"But if R5 becomes zero during additional management, the state is not deactivated, C4 does not return to 0 and C5 loops. C4, however, continues to be compared to R4 to process the change from C4 to 0. The additional management, however, remains activated. Thus, if C5+1 reaches an R5>0, then the additional management changes C4 to 0 before deactivating its state."*

* **Question for Author:**
  Could you clarify the exact intended meaning of the sentence:
  *"C4, however, continues to be compared to R4 to process the change from C4 to 0"*?

  Specifically:
  1. Does $C_4$ reset to 0 mid-adjustment when it next matches $R_4$ (while $C_5$ continues looping and adjustment remains active)?
  2. Or does $C_4$ continue free-running past $R_4+1$, cycling through 127 and wrapping by 7-bit overflow while $C_5$ loops, with the reset $C_4 \to 0$ occurring only upon deactivating additional management when a reachable $R_5 > 0$ is encountered (as described in the following sentence: *"Thus, if C5+1 reaches an R5>0, then the additional management changes C4 to 0 before deactivating its state"*))?

* **Current Implementation Behavior:**
  Our core implements model (2): during the stuck $R_5=0$ adjustment, $C_4$ increments past $R_4+1$, free-running through 127 without resetting at $R_4$ while `in_adj=1`, and resets $C_4 \to 0$ when an $R_5 > 0$ write satisfies the $C_5+1 == R_5$ termination condition (verified by unit test `t08j`). If model (2) is what silicon executes, we suggest clarifying the wording in §11.3.2 in a future edition to avoid ambiguity.
