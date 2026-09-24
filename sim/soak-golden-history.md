# Soak golden-hash history

`make -C sim soak` prints a rolling hash over the fixed-seed randomized CRTC stimulus (protocol
and sampled fields: `sim/README.md`, "Randomized equivalence soak"). This file is the only
record of the golden value: the top row is current, and each row says why it replaced the one
below. Unless a row says otherwise, the seed, stimulus and sampled projection were unchanged
and the hash moved because behaviour did.

When you re-mint, add a row at the top and cite the evidence document. A hash change on a
refactor commit means behaviour moved; stop and document why before proceeding.

| Hash | Date | Reason |
|---|---|---|
| `0xe99ab434a5e1cdb3` | 2026-09-22 | **Current.** F14 type-1 additional-line existence made independent of R5 (French ACCC v1.11 section 19.6.2 p.217), with the additional-line adjustment-entry guard. See `docs/investigations/hardware-runs/shaker-d1-d6-retest-2026-09-22.md`. |
| `0xb1cb70da95c2e44f` | 2026-09-11 | D1 canonical origin VSYNC/active-pulse phase ownership and D6 shared RFD parity (French sections 19.7.2 p.219, 11.6.1 p.90, 19.5.3 pp.209-210). The two VSYNC arm/phase bits joined the sampled projection. See `docs/accuracy/d1-d6-parity-repair-2026-09-11.md`. |
| `0x6e8258198d6e6137` | 2026-09-08 | B8-1 production-phase R5/R0 write-event retention (French sections 11.6 pp.89-92, 13.7 pp.126-128): old-value side effects survive register writes between character enables. See `docs/accuracy/b8-1-cpu-write-timing-2026-09-08.md`. |
| `0x2263c9fc44af4ee7` | 2026-09-01 | Reviewed interlace VSYNC lifecycle correction: type 1 no longer depends on type-0 C0 history; type 0 reconstructs an already-earned C0=2 qualification from the live counter when snapshot load or live type switch clears private history after C0=2. |
| `0xf96f243f594acecf` | 2026-09-01 | Candidate superseded in review: covered only clears before C0=2. |
| `0x8a2c2290bcef06a7` | 2026-09-01 | Type-0 preceding-line C0=2 qualification and blocked-comparison model (author-confirmed section 16.4.1.2). |
| `0x9d8cd95357d1d752` | 2026-09-01 | Q20 author-confirmed row-only C4 reset during type-1 R5=0 adjustment. |
| `0xd6bc1649ff2058a1` | 2026-08-31 | IA-6 type-0 R0=1 widening route (French section 13.7.2 pp.126-128): unsafe R0 1-to-larger write at C0=1 on a true last line preserves the old equality, enters additional management at C0=2 with C4=R4+1/C9 retained, counts R5=0 through effective target 31. The one-character pending action joined the sampled projection. |
| `0x21bbf9c29ab08413` | 2026-08-31 | IA-3 type-0 R6=0 first-frame-line conflict (French section 18.3.2 p.191): DISPLAY ENABLE on at character start, off 0.5 us later until live C0=R1; an R6 0-to-nonzero write before R1 cancels it. The new lifecycle latch joined the sampled projection. |
| `0x87a9d80a91381c9b` | 2026-08-31 | IA-1 controlled type-0 R3-terminal HSYNC restart (French sections 15.3.2-15.3.3 pp.150-151); the ~3.5-pixel restart maps to 14 master ticks only for the pinned `t33b` bus phase. Pending/count state joined the sampled projection. |
| `0x654a244c2cce6e0b` | 2026-08-31 | IA-2 type-1 frame-origin correction (French section 19.5.3 p.209): ParityC9 from the newly toggled ParityFrame; odd IVM frame starts at C9=1. |
| `0x32d468e81eac63c9` | 2026-08-30 | Reviewed F20 R2.JIT correction: type-0/type-1 starts move by 4/3 mode-2 pixels while trailing edges stay fixed. |
| `0x005deed28be80fa1` | 2026-08-30 | Pre-review F20 model (incorrectly width-preserving) plus sampled-state expansion. Rejected. |
| `0xc769ea4605afbe04` | 2026-08-30 | F13 half-character type-0 R1>R0 border pulse: no-skew C0=R0 event low only from nCLKEN to the following CLKEN; SKEW-DISPTMG 1/2 keeps the rounded full-character displacement. |
| `0x48146d2b681268ab` | 2026-08-26 | F16 type-0 post-IVM exit frozen C9.VMA comparison (section 19.8.1 p.221); F17 type-1 RFD on C9=R9 disables the VMA source while arming parity (section 11.6.1 p.89 case 2); F18 type-1 readable register matrix (section 21.2.2). |
| `0x85b3f8e847430495` | 2026-08-26 | F15 type-0 odd-R9 IVM counting (target R9+(ParityC9 xor R9.0), p.207/p.220 parity updates, section 19.5.2 VSYNC delay for odd R7 on odd ParityFrame). Even-R9 behaviour bit-identical. |
| `0x627bdc9923a60677` | 2026-08-26 | F14 additional interlace line on both types (type 0 after R5 adjustment when R8 in {1,3} and ParityR6 odd; type 1 defers adjustment end by one line under its conditions). |
| `0x63d9de100ac9f6f2` | 2026-08-25 | B-1 remediation: type-1 IVM VSYNC starts at the half-line tick on the ParityFrame-even frame (p.209) via a seam-latched fire decision. |
| `0xd620fce8b1c05b25` | 2026-08-25 | t24 closure: type-1 IVM VSYNC fires from the IVM-aware row-structure test on both parities; legacy field=1 MID-VSYNC arm no longer hijacks it (p.209 table). |
| `0x801a59096c192d26` | 2026-08-25 | F11h: type-1 section 20.3.2 row-0 VMA reload samples the post-edge register file (p.243 chronogram 2). |
| `0xa9e5026de83d287c` | 2026-08-25 | F10 review remediation (type-1 stage A writes C9.0, section 19.8.2 ParityC9 toggle at frame boundaries, stage-B write gated, IVM engages from reset/snapshot R8=3); toggle latches joined the sampled projection. |
| `0x83e80134f7705b46` | 2026-08-24 | F10 type-0 IVM (section 19.8.1 split C9/C9.VMA); seam-latched IVM mode and toggle status joined the projection. |
| `0x7d0e5c8bd984e899` | 2026-08-24 | F10 type-1 IVM (R8-toggle parity stages, section 19.8.2 counting); IVM/stage state joined the projection. |
| `0x1ac680cd2f12559a` | 2026-08-24 | F10 fixture-stage sampled-field expansion: shared interlace parity flops joined at reset values. No RTL change. |
| `0x512eaae74a628dca` | 2026-08-23 | A2 exact-edge R4 adjustment-reload suppression. |
| `0x6439f9805b20acaa` | 2026-08-23 | A1 removal of the type-1 adjustment-ending spurious VSYNC. |
| `0xae27f2c3c758ed87` | 2026-08-23 | F7 type-1 RFD. |
| `0xf5f8ae01ffdf928d` | 2026-08-23 | Sampled-field expansion (holdoff latch, type-1 status flops). No RTL change. |
| `0x326ea81358e7d88f` | 2026-08-23 | F6 Stage 1. |
| `0x5b5004ff70148443` | 2026-08-22 | Original mint from the unsplit core (`docs/plans/2026-08-22-accc-review-plan.md`). |
