# SHAKER D3: held Space across handler entry — 4 November 2026

**Superseded capture diagnosis, 6 November:** the private 100 ms overlay sends
one extra Space before 0329. Correcting that count, with unchanged timing,
now yields 44 correctly bound native captures. See the [count proof and current
acceptance](shaker-d3-count-correction-2026-11-06.md). The source-level held-key
mechanism below remains possible, but does not establish the cause of that
specific offset. No canonical author-script defect is inferred.

The missing host capture is an input/capture problem until a faithful page-state
comparison proves otherwise. A focused Astra-medium source analysis resolves one
ambiguity: R2=35/R3=4 is not inherently transient. A held Space can advance that
page after entering the next handler; a long release between host commands does
not exclude a single press spanning both handlers.

## Verified program behavior

The extracted new SHAKE27D payload loads at 3A00, length 27060, SHA-256
`65a90825dc674953b5dd9d4918bddf8079ea44877d89b08542b7fabb6b8d5ec5`.
Relevant code/data are byte-identical to the separately retained SHAKE26D listing
at the same addresses: D3 5AC4–5BEC, menu 41E1–4209, handler enter/exitA05C–A0CE,
keyboard 9FDC–9FF2 andA303–A332, emittersA34D–A3A8, pointer table and relocated
SSM records. This was checked against bytes, rather than assumed from the older
listing. Inputs remain ignored under`package/d3-transition-question/`.

- All four R3 entries use the same loop.5B3A selects R3=4;5B7C initializes
  HL=2E34;5BA1–5BA5 increments R2 to35 and records it at 5B87.
- 5BDB calls keyboard routine 9FE0.9FEE–9FF2 reads Space's active-low bit from
  A338 into carry.5BE7 RET NC leaves the page while Space is down.
- Each new R2 page initially patches5BDE to SCF, forcing the first sample to
  continue. It then replaces SCF with NOP. This guarantees an initial loop,
  not a release-before-next-advance latch.
- After R2=27,5B9D entersA073. The handler-exit route also accepts Space's
  level; the following R3 handler does not require a fresh key edge.
- The SSM emitter at 5B8D runs after 5BA0 returns. It can capture the departing
  parameter state before the subsequent loop changes it. Native host retrieval
  arrives later than a synchronous emulator SSM screenshot.

With Space released,35 remains in its loop. No automatic timeout advances it.
The observed36 therefore needs an accepted keyboard sample or an execution
error outside that ordinary path. Static analysis establishes the possible held-
level mechanism; it does not measure which sample occurred on MiSTer.

## Captures and limits

The bulk70ms overlay loses advances; a200ms probe repeats them. A100ms short
probe reaches35→3A→3B, but its full walk matches only the first 21 headers before
one extra advance at the R3=4 transition. Separate70ms Space calls, each with
MBC's two-second open/close release interval, still show that transition offset.
All runs and ledgers are retained; none supplies44 accepted parameter states.
The separate-call run completes348 commands/44 captures/zero capture failures;
completion is not page-state correctness.

The canonical new D script contains five50-space strings; the private manual
and old event-paced overlays use marker-specific counts. Keep those script
histories distinct. Their relevant D3 program code is the same, but the input
and synchronous-capture semantics differ. No author script or core defect is
established by these host runs.

## Cheapest next discriminator

Trace the boundary once, instead of repeating the full44-page walk. At handler
entry 5B3A and the return from 9FE0/PC5BDE, record R3 immediate 5B74, R2 immediate
5B87, Space byteA338, carry and whether5BDE contains SCF(37) or NOP(00).
For R3=4/R2=35, a later sample with Space bit7 clear followed by5BE7 establishes
held-level advancement. No available MiSTer CPU-debugger trace surface was
established in this analysis.

Authentic-binary synchronous SSM capture in an emulator is the proven alternative.
A state-triggered debugger pause before the keyboard sample, followed by release
and resumed stable-frame capture, would also preserve the binary. Passive video
around the transition can expose a brief state on hardware. No such new capture
mechanism was implemented, and no further full timing sweep is recommended here.
