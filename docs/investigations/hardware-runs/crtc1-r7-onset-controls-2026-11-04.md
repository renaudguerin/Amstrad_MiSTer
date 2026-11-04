# CRTC1 R7 onset controls — 4 November 2026

Two independent normal-boot AmSpirit guests match French ACCC1.11 §16.2.1 p162:
OUT(C) black starts at the ninth pixel of the preceding VMA word; OUTI at the
fifth. Both trigger raw PPI VSYNC. The local production-T80/GA replay preserves
the four-pixel relationship but provisionally starts two pixels early. This
makes write-observation timing testable; it does not identify a physical aperture
or justify an eight-master-tick delay. No production R7 change is made.

## Guest and source qualification

Parent read the French rendered page directly. Type1/2 write-induced onset is
9/5; ordinary Type1 onset is6. Type0's separate JIT rule defers one microsecond
and is not substituted for this Type1 control. The original guest programs use
standard312×64µs timing, Mode2, R7initial30, R6=25 and AA00 throughout16KiB VRAM.
Both palette levels and border are nonblack. Published §7.2 PPI synchronization
aligns the guest; R7 then hides at127 before next-frame row10. The matched
OUT(C)/OUTI blocks equalize documented instruction durations, not fitted NOPs.

AmSpirit1.17.1/core2491682 loads original DSK/BIN through normal6128/CRTC1/128K/
French BASIC. CPR is unsuitable here because its loader switches to Plus. No
host RAM, PC, counter or instruction patch; no counter-conditioned CPU release.
The original guest-PC stop11A5 preserves an unblanked baseline after its own
synchronizer, then the guest continues naturally to11C3/11C2 and its original
PPI result store. Pauses freeze the entire machine; they do not advance the
hardware while holding CPU state. Code, mapping, DI, registers and **entire16KiB
AA00 pattern** are verified at both stops; original RUN input finishes naturally.

## Optical result

Both images use the same full uncropped LIVE pipeline. Headers identify a
1024×350 source raster, delivered as1024×700 PNG (two identical rows per line).
The first JIT line is source y175, PNGrows350/351; beamx819. Future-beam shading
is excluded. All640 independently decoded baseline samples match AA00, with
160red samples; active origin234. The preceding word starts730.

| Variant | First black x | One-based word ordinal | Source | PPI bit0 |
|---|---:|---:|---:|---:|
| OUT(C),A |738|9|9|1|
| OUTI |734|5|5|1|

Every RGB sample before black matches the same-run baseline; black continues to
the paused beam on both duplicate rows. First-black detection uses a dark
threshold separated from both nonblack palette values, without an expected-edge
search. The word phase comes from the verified baseline, not a local simulator
counter or a screenshot registration fit. This establishes the **software
optical endpoint**; it does not expose the reference's raw VSYNC pin edge.

Parent executed the guarded root-only request once: result
`complete-two-original-optical-captures`, all6checkpoints complete. Frozen code,
media hashes, operations, paused SNA/state, baseline/live images and independent
optical ledger are in ignored
`resumed-20261104/r7-onset-controls/reference-request/runs/run-001/`.
Classic SNA is version2; hidden C0/C4/C9 bytes are absent, not zero observations.

## Local path and next falsifier

The copied actual-T80/READY/motherboard/PPI/GA probe starts from an original
custom ROM and reaches the same guest naturally; it is not an AMSDOS/FDC replay.
Raw VSYNC, GA blank, RGB and video address are separately observed. Relative to
the nominal C0=32 edge, OUT(C)/OUTI raw VSYNC are+10/−6masterticks, RGB+20/+4:
the relative16tick/four-pixel difference is correct. Absolute word ordinals
look7/3. Backend data phase, shared vertical blanking and write observation
remain separate possible owners; no internal timestamp is made golden.

A focused Astra high follow-up recommends **one ordinary NJIT control** on the
same path, using early R7=10 with the same instruction schedule. Its independent
expected ordinal is6. If local NJIT also starts two pixels early, investigate
the shared path; if NJIT=6 while JIT=7/3, the discrepancy is more specific to
write-induced timing or CPU/bus binding. With R1=40 the preceding NJIT word is
in border: extrapolate the independently verified16-pixel word lattice across
horizontal wrap, and do not claim AA00 is directly visible beneath that edge.
That control is complete. Same guest timing, one guest immediate changed127→10:
AmSpirit Type1 has baseline origin234, extrapolated border-word origin218 and
firstblack223, ordinal6. A fresh Type0 run has the same origins and firstblack222,
ordinal5. Both complete original code/VRAM/DI/PPI/input guards. All preblack
samples match baseline and black continues to the paused beam on both doubled
PNG rows. No direct AA00-under-border claim is made.

Frozen local actual-T80 traces give NJIT5 for **both** types. The private
`check_njit.py` recomputes the word lattice from post-CE16 samples and exits1:
Type0source5 passes, Type1source6 fails. This isolates a Type1 ordinary phase
question more strongly than a shared GA correction: shifting common blanking
would spoil the matching Type0 path. JIT7/3 versus9/5 is still a separate
observation; the B6 equality/carry defect is not solved by moving an output edge.

The pair is now red in private copies of the existing fast actual-CRTC/GA
fixture: Type0=5passes, Type1expected6/actual5fails. The normal target retains
its existing controls. A private uniform Type1 pin-publication phase probe is
authorized after that red; no production timing edit is accepted. `VSYNC_r` owns logical pulse count/rearm while
`VSYNC <= VSYNC_r` is the current shared one-master-tick published pin. PPI,
GA and filter consume that pin. Any publication hypothesis must preserve both
edges/pulse width and reset/SNA/type context; the sixth-pixel endpoint alone
is not a literal flip-flop or off-edge oracle. No timing implementation follows
from this read. Private controls/checker/impact ledger are in
`resumed-20261104/r7-njit-control/`; reference runs are `run-001` (Type1) and
`crtc0-001` (Type0).

The [original019D failure](crtc1-outi-019d-2026-11-04.md) supplies a separate
raw-PPI constraint. Only later **observation of equality against then-current
C4** can reject its fifth case after carry. Delaying an already accepted true
equality or merely its output cannot remove that unwanted pulse. The new optical
endpoint alone cannot select which internal boundary is wrong.

“B8” is the audit fixture name, not an original SHAKER B8 caller. Its synthetic
R5 first-store margin remains a regression contract, not independent silicon
proof. The authentic BCTRL R5 scanner found here uses OUT(C),C, not OUTI. Keep
R5/R0 deadlines unchanged; do not certify an OUTI margin with that other opcode.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
