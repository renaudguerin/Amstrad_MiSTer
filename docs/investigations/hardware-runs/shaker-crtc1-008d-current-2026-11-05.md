# CRTC1 AI/008D: short-line VSYNC still differs

A fresh original-program check on full build208bacf confirms the known
short-line discrepancy. For each of the two R0=0 blocks, the four printed
PPI VSYNC bits are **1/0/0/1** on MiSTer, versus **1/1/1/1** on the
[original-CPC photograph](https://shaker.logonsystem.eu/images/cpc/CPC/AI_CRTC1_A.webp).
Both R0=1 blocks read1/1/1/1. These are sampled raw-PPI results, so changing
the post-GA sync filter cannot repair them. The samples alone do not prove
a continuously high physical pulse between reads.

The targeted native run completes13commands/two inspected captures/one
SSM record without failures. Marker and later PNGs are byte-identical.
The normal-boot input is ModuleA, keyI, wait008D, on the unchanged author
disk6b9ff106…0eb61, CRTC1, FR layout and Full filter. RBF/source/hash/timing
are recorded in the [208 acceptance](crtc0-ivm-exit-table-correction-2026-11-05.md).

Fresh isolated CPCSyntaxError0.12.0, matching22de765 source and the same
disk, packaged UK firmware and `--no-chain --no-errata`, gives1/0/0/0 in
each R0=0 block. It fails the physical comparison and supplies no validated
replacement rule. Its two BMPs are byte-identical. Matched normal-boot
AmSpirit1.17.1/core2491682, FR/128K/CRTC1, gives1/1/1/1 throughout, matching
the photographed bits. Different upper PPI input bits are outside this
comparison. Root inspects all six emulator/device images and the physical
008D and neighboring008E photographs.

AmSpirit's original CSL completed and captured the page before a missing
state-directory error. Cleanup paused the instance and cleared scripts and
breakpoints. State/SNA collection was then completed on that paused run
without replay or guest changes; its new settled image is RGB-identical to
the script image, with different PNG bytes. No exact CPU-phase equivalence
is claimed across that cleanup. The live instance is left paused on AI008D.

The current author question's old eight-line argument was wrong. French
ACCCv1.11 **§14.2 p133** specifies a fixed **16-line** VSYNC for CRTC1/2,
ignoring R3's upper nibble. **§16.4.2 is on French p171** (English p170);
it describes onset and ordinary line16 termination, without explicitly
identifying the width-counter clock in the short/no-HSYNC case. Root reads
the position-aware native Markdown and fresh renders133/170/171 against
PDF SHA28f25c73…02c3b; the existing inspector report is native_partial with
fallback pages40/41/182/183/280. Q22 is corrected without inventing an
author answer. Historical October timing estimates remain historical.

A focused source survey finds line-end clocks in CPCEC c025aab and
konCePCja f15cdcd. Caprice32 a0d78f0 here models Type0 and cannot decide
a Type1 rule. Root checks the cited executable counter blocks; no fresh
original-program runs of those three peers were performed. Their algorithms
supply no hardware-matching alternate clock. HSYNC gating, another
horizontal-phase/minimum-line gate and intervening retriggering remain
possibilities, not selected repairs. The neighboring008E photo also has
all four samples high for R0=2/3; it does not by itself choose a clock.

A genuine AmSpirit preinitializer snapshot (PC6491/SP4063, original return
6411) now completes the untouched four-group caller under current c0939d8
production T80 and public classic-v2 restoration. Its accepted writes,
16 actual PPI reads, CPU/raw-store identities and32 formatter-digit stores
qualify. It prints **1000/1111/1000/1111** in61,383,676 master clocks,
below the unchanged400m cap. The first run's exit1 is preserved; no guest,
phase, parity, counter or cap adjustment follows. Root reads the passive
observer/receipt and verifies the67-artifact manifest and source bindings.

For the first zero-length group, R0=0 is accepted at C0/C4/C9=0/29/7.
The implemented pulse rises56 master clocks later, loads the existing
width counter15, then expires after16 further count ticks spaced64 master
clocks:1024 master clocks, or16µs. HSYNC stays low throughout. No second
rise/reload occurs through the fourth read. The public CRTC pin follows
logical rise/fall by five master clocks. Both zero-length groups share
this history. These observations explain current RTL expiry; they do not
identify the physical width clock. The R0=1 pulse ends after restoration
to R0=63, so that run is not a fixed-R0=1 width oracle.

The replay's fourth bit0 differs from native208's fourth bit1. Its S2/S3
photo mismatch is qualified, but complete native state/phase equivalence
and the fourth-sample mechanism are not. Classic-v2 omits hidden beam,
parity and divider state. The unchanged original caller synchronizes
naturally; this does not restore those omitted states. A passive reference
capture immediately after64EC, before restoration A304, could locate that
provenance gap without changing guest timing.

Focused independent Astra-medium judgment also finds no selected RTL rule.
A small physical discriminator should start a pulse under ordinary timing
before the short-line interval, use R8=0, and exclude a new C4=R7 match.
At fixed R0=1/R3-low=1, compare reachable R2=1 with unreachable R2; add
R0=0 with unreachable R2. Hold each interval at least64µs and continuously
record raw HSYNC/VSYNC and write timing, preferably raster address too.
If R7 is changed to exclude new matches, first verify under ordinary timing
that this preserves an active pulse. Verify separated HSYNC pulses rather
than assuming them: French **§15.3.2 p150** explicitly makes
R0=0/R2=0/R3=1 an infinite-HSYNC case. French **§13.3 p115** discusses
other counters at R0=0 without selecting a VSYNC-width clock.

Expiry only with HSYNC would support an HS-dependent qualification; expiry
in both R0=1 cases while R0=0 holds would support a zero-length condition.
If both R0=1 cases hold, a longer-line pair can test minimum-phase effects.
Expiry of a pre-existing pulse at R0=0 would instead distinguish the AI
onset/admission context. None alone proves an internal silicon topology.
The paired probe remains unexecuted and is not an RTL test oracle.

Keep
[Q22](../../classic/accc-author-questions.md) unsent and separate from B9
and the Type0 inactive-R7 protection fix. No new public test or RTL change
is justified by the source survey, and the historical photo denominator
is unchanged. Evidence, logs, scripts, photograph provenance and parent
receipt are under `local/overnight-20261103/resumed-20261104/c1-ai-008d-current/`.

Technical information sourced from the Amstrad CPC CRTC Compendium by
Longshot (CC BY-NC-ND).
