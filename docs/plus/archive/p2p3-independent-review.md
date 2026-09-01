# Independent review — P2 ASIC page and P3 interrupts (`plus/p2-asic-regs`)

Two-pass cross-provider review ordered at end of session, 2026-08-25. Delta
reviewed: 76aae12..HEAD (19+ commits: P1 follow-ups, p1_video calibration,
P2 `asic_regs` + integration + RGB widening, P3 PRI/DCSR/IVR).

## Pass A — Claude Opus 5 xhigh (claude CLI, fresh session)

Scope: classic-mode invariance, PRI block, integration seams. Verdict per
area: NOT CLEAR / NOT CLEAR / NOT CLEAR.

Blockers, all verified real and remediated in `82f568a`:

1. **Undeclared top-level wires** — `plus_asic_dout`/`plus_asic_rd` used in
   `Amstrad.sv` without declarations: implicit 1-bit nets corrupted every
   ASIC-page read in Plus mode. Quartus warning 10236 appeared five times
   inside the green synthesis; no sim gate elaborates `Amstrad.sv`.
2. **Vector source collapse** — INT_N rises one edge into the acknowledge
   via combinational irqack, so an unsampled pending level dropped the
   raster source bits before the CPU latched. Fixed by sampling
   `ack_pending` at the intack rising edge.
3. **DCSR bit 7 inverted** — the merger cleared the level on the very
   acknowledge that should set it, breaking the documented
   read-DCSR-at-handler-head dispatch. Level now persists across a
   raster-sourced ack; clears only when one completes with nothing
   pending. Pinned by new `pr05`.
4. **`vec_valid` not plus_mode-gated** — classic acknowledge bytes were
   hijacked every interrupt (~300/s).
5. **ASIC page not plus_mode-gated** — combined with the mode-blind unlock
   detector and RMR2 capture, a classic program emitting the unlock
   sequence could hijack &4000-&7FFF reads. All gated; plus_mmu's
   `unlock_write` and RMR2 capture too.

Non-blocking notes accepted: PRI fire lost inside an ack window matches
netlist set-dominance for the classic path (real-ASIC PRI unverified);
`spr_ram` async read may land in MLAB (watch fitter); bench-top RGB ports
were still 2-bit (fixed same pass).

## Pass B — GPT-5.6 Sol high (roster-codex, fresh session)

Scope: `asic_regs.v` conformance against asic-reference sections 3/4/6/9
plus vector quality. Verdict: NOT CLEAR.

Blockers, both fixed in `f0b208a`:

1. **Reset did not dominate page writes** — the legacy-translate hoist had
   split the old else-if chain into a separate if, letting a write cycle
   coinciding with reset override reset-defined fields (IVR bit 0).
2. **DCSR flags 6:4 had no set path** — write-one-to-clear was
   unobservable and P7-unready. New `dma_int_set[2:0]` input OR-sets flags
   on any clock edge; &6C0F writes clear by ones; simultaneous set-and-
   clear resolves set-dominant; the term is reset-gated. Motherboard ties
   it low until P7.

Also landed from its untested-rules list: +6/+2 read mirror, Y-high
masking with nontrivial bus values (&FE/&01), even-byte write preserving
stored green, &6800-&6805 reads as open bus, entry-18 legacy isolation,
and the `pal_rdata` port comment corrected to {G,R,B}.

## Accepted residual items

- a06 samples unmapped boundaries rather than sweeping every address;
  full sweeps are impractical per-region and the decode is structural.
- SAR/PPR storage is written but unobservable until P7 consumes it.
- Motherboard-level no-write-through is enforced by suppression terms and
  mux inspection; no bench drives the full top-level memory path.
- Real-ASIC power-up contents of INKR/ink-select/palette remain named
  zero assumptions.
- The PRI compare resolves the reference's internal VC5 contradiction in
  favour of aliasing (both n and n+256 fire) per [QUASAR]; the choice is
  now documented at the RTL site.
