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

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
