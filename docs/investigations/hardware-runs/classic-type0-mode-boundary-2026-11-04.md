# Type0 mixed-mode boundary: qualified failure — 4 November 2026

The retained Type0 Mode1→2 XFAIL is a real source discrepancy. A further
non-aliased control also exposes premature Mode0→1 decoding. No mode timing
change has been made from this investigation.

## Mode-qualified oracle

French ACCC v1.11 §9.3.4.3.3 p62 and p63 diagram(3) explicitly retain the last
half of old Mode1 pixel B at position4, followed by new Mode2 bits b5,b4,b3 at
positions5..7. Mode2's advance shortens the byte suffix; it does not remove B.
For source byte5B, the expected pen sequence is1,0,1,1. The independently
decoded byte origin places those slots at master ticks971,975,979,983.
Type0 instead emits new0,1,1 at971/975/979 and an extra bit at983. Type1 hides
the old slot and produces the correct three-bit suffix.

The actual-T80/READY private vector uses six distinct bytes and full fixed-mode
controls. Its controlled phase/release rule is frozen, not a natural caller
checkpoint; no new release phase or NOP count was searched. At native tick963,
Type0 already sees new Mode2 and overrides the old alternating hold/shift
controls. Type1 retains the old hold for that sample. That difference reaches
the decoder/serializer and then the exposed RGB slot.

## The aliased control was insufficient

The first Mode0→1 control appeared to preserve the old quarter-pixel required
by §9.3.4.3.2 pp60–61. Astra-high's focused read found that old Mode0 pen10
and wrongly decoded new Mode1 pen2 shared RGB13 in its modulo-four palette.
The result established suffix cadence, not old-mode preservation. The earlier
claim that the failure was specific to entering Mode2 is withdrawn.

A single new private build changes only pen10's ink to white31, leaving pen2
on ink6 and all CPU, VRAM and RTL timing unchanged. Full192-clock fixed-mode
controls pass. Type0's old slot should now be RGB62, but is RGB13: pen2 was
decoded instead of old pen10. Type1's old slot is blank; both types' new pen1/2
suffix cadence remains correct. The original aliased proof remains frozen.

## Architecture limit

The direct Mode2 override is present in Gerald's schematic. Its asynchronous
MODE_SYNC-clocked mode flops and native-clocked controls do not establish a
new publication delay, and the drawing itself is marked unverified against a
real40010. The missing observation is which native sampling edge first sees
the new mode at the colour mux and hold/shift inputs, relative to HSYNC fall
and LOAD. Neither a typed GA delay nor a common delayed event was implemented.
The Compendium's output rules remain the required behavior.

CPCSyntaxError's logical byte-position mode switch corroborates the output
oracle, but supplies no independent master-clock phase measurement. The one
focused Astra consultation confirmed the source oracle, caught the palette
alias and proposed the non-aliased discriminator; it made no implementation.
Persisted native metadata confirms gpt-6-astra/high.

Frozen evidence is under ignored
`local/overnight-20261103/classic/type0-mode-boundary/`, including primary
French renders, Gerald's inspector/fallback report and figures, source copies,
the actual-T80 traces, `ASTRA-BRIEF.md`, and `nonaliased-control/`.

## C-HSYNC discriminator

A focused Opus read found no literal stage/NBA translation defect in Gerald's
single native-clock video/control/color pipeline. Under the frozen traces, the
source0→1 old-quarter/suffix constraints require mode visibility after963 and
by967, while current Type0/1 visibility is961/965. This rules out a **simple
common constant delay** in that clock model; it does not identify a new gate.
The independent GA publication/CSYNC phase remains the issue, rather than a
justified typed GA workaround.

French §16.2.2 p166 specifies C-HSYNC deactivation1–2Pixel-M2 after CRTC HSYNC
end (or the4µs cap). Current `syncgen_sync.v` instead clears `hdelay` directly
through `hsync_n`; its synchronous event follows that modeled edge. Which
physical end phase should feed MODE_SYNC is not established by that literal
unverified drawing. The printed p166 chronogram and prose must be reconciled
before deriving an exact quantization phase; no universal delay follows here.

Parent also inspected higher-resolution primary pp56–57. Ordinary Type0
reactivation/mode switch are36/37 and Type1's37/37. In their R3.JIT rows both
are40/40. These are extra source output constraints; they are not evidence
that raw HSYNC end simply moved four dots or that one two-dot clock grid fits
all cases. A new publication model must explain ordinary and JIT together,
without fitting only the missing old-mode slot. Source render, crop and full
Opus reasoning are frozen in `resumed-20261104/type0-mode-order-review/`.
No new mode experiment or production edit followed this read.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).

## Old-pixel completion is insufficient

A second focused architecture read rejects a universal “finish the old pixel”
rule. At the observed ordinary handoff, Mode0→1 changes the colour mux while the
existing hold/shift decisions remain the same, so control-only deferral cannot
preserve old pen10. The mode-dependent control equations differ at other phases. Any common handoff
would need to govern both mux and control evaluation. Ordinary old-at963 /
new-at967 fits both transitions on paper, but does not select a circuit rule.

The independent R3.JIT rows on French pp56–57 switch at 40, inside the old
Mode1/Mode0 pixels ending at 40. Completing those pixels would switch at 41.
A LOAD exception could fit that position, but simultaneous serializer load
and colour consumption do not establish which mode owns the sample. Explicit
R3.JIT mixed-mode byte provenance is the next useful control; existing R2.JIT
blanking checks are a different stimulus. No mode handoff has been implemented.
Consultation and verified model metadata: ignored
`resumed-20261104/mode-completion-architecture/`.

## Genuine R3.JIT source-qualified mixed-mode control

A source-qualified actual-T80/READY private suite is complete in
`local/overnight-20261103/resumed-20261104/r3jit-mode-provenance/`
(`HANDOFF.md`, `SOURCE-PLAN.md`), with no timing or model changes. It qualifies
an actual write OUT(C),E R3 14→2 at C0=22 / phase 8 (C0 = R2+2 with R2=20),
hsc 2, old HS active, fulfilling French primary sources §9.3.4.4.2/.3 pp.66–67
and §14.5.4 pp.140–142. All six full 192-clock fixed-mode controls pass on both
CRTC types. Six distinct bytes B6, 6D, E3, 9C, 5B, A5 begin at line byte
38 = 2*(R2−1), with all 16 pens mapped to distinct RGBs and blank distinct from
every pen.

| Case | Type0 | Type1 |
|---|---|---|
| Actual write 14→2 at 22 / phase 8 / hsc 2 / HS active | Qualified | Qualified |
| Complete 192-clock fixed controls 0/1/2 | Pass | Pass |
| Source 40→47 A5 Mode2 bit sequence | Pass | Pass |
| Source 40 0→1 required 5B C pen 2 / RGB 13 | Fails: pen 1 / RGB 1 | Fails: pen 1 / RGB 1 |
| Required blank release 40 / tick 859 | Fails: 38 / tick 851 | Fails: 39 / tick 855 |

In 0→1, at tick 819 the serializer loaded 5B. At tick 851, simultaneous LOAD
consumes old SR = B6 (5B shifted once), pre-CIDX 13; the Mode1 mux writes pen 1
from 5B's (b2,b6), the ordinary B pair, rather than required JIT C (b1,b5).
At 855 colour is held, and RGB 859 reads stored ink 4. Thus the wrong pen 1
comes from **5B old-SR colour**, not an early A5 appearance (which also starts
with pen 1); the causal ledger resolves this alias.

In 1→2, tick 851 loads A5 while colour consumes old SR. Refresh 855 consumes A5
bit 7; RGB 859 reads that result, as required. The following 7-bit suffix
(source 40..47) agrees on both types. However, blank release fails on both
(Type0 releases at 38 / tick 851, Type1 at 39 / tick 855, versus required 40 /
tick 859). The 1→2 result provides an informative pixel-ownership control, but
is not a whole-case pass.

Preliminary invalid composites were caused by a port list shift when the frozen
wrapper was initially paired with a newer Type0 engine; coherent frozen engines
were restored, giving zero missing-pin warnings under `-Werror-PINMISSING`.
Prior universal old-pixel completion is rejected, and no rule or timing fix has
been chosen.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).

## Causal barrier after the R3.JIT control

A focused Gemini4Argon read identifies the third Mode1 shift decision at839,
before the actual R3 write840. No later MODE_SYNC delay alone can make old5B
advance from B to required C by the colour sample851. The passing1→2 suffix
starts after LOAD ofA5, so it is not a mixed-byte handoff proof. A late-half
shift schedule and a byte-phase cursor remain unimplemented alternatives;
fixed-mode equivalence is a proposed constraint, not a new passed simulation.

Parent reread Frenchp166: its plotted CSYNC is the combined signal during
the second HSYNC of VSYNC, not an isolated MODE_SYNC/C-HSYNC trace for both
CRTC types. It cannot simply be renamed as the required clock edge. The
1–2Pixel-M2 prose and chip tolerance do not independently choose a fitted
quantizer. No new timing candidate follows. Full source/read-only report is
under ignored resumed-20261104/r3jit-next-discriminator/.

## Non-aliased 2→1 controls and abstract-model boundary

A further paired ordinary/JIT proof uses 5D followed by A5, retaining each
family's actual-T80/READY schedule. All 12 complete fixed-mode controls pass.
Ordinary Type0 loses the old partial pixel and has wrong zero-fill provenance;
Type1 passes. JIT Type0's visible pen is correct, but Type1 emits pen1 instead
of the source-required zero-filled pen 0. Its causal ledger attributes that
value to old 5D shifted seven times, not the equal first A5 pen. Both JIT blank
releases remain early. No behavioral candidate followed.

One focused Astra-medium assessment corrects an assumption: the new-mode token
already occupies masked source 39 before visible 40. Visible release therefore
does not establish the internal mode edge. A universal abstract token model is
allowed; identifying literal silicon stages is unnecessary. A general causal
rule for residual-byte ownership/publication has not yet been derived. French
§9.3.4.4.1 p65 additionally requires JIT2→0 residual 5 (pen 5 for 5D), versus
JIT2→1 residual 6 (pen 0). Any proposed shared model must satisfy both destination
cadences without recipe-specific phase constants. This is a paper acceptance
constraint, not a newly executed 2→0 test.

Frozen paired proof and assessment: resumed-20261104/r3-2to1-zero-fill-proof/
and r3-stateful-model/. The consultation made no production change.
