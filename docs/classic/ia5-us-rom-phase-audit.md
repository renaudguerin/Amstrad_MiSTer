# IA-5 U.S.-ROM GA interrupt/VSYNC phase audit

## Verdict

**CLOSE AS A HARDWARE DISCRIMINATOR; do not add a synthetic regression or change RTL.**

French ACCC v1.11 section 4.2 p.18 says that the U.S. ROM's R5=6 makes a
262-line frame, two lines longer than the 260 lines needed for five exact
52-HSYNC periods. It says the resulting Gate Array interrupt request occurs on
the same scanline as the CRTC VSYNC start, but before it. The English p.18 says
not the same line and is the edition error tracked as BL-005.

French section 27.6.1 pp.285-286 is a separate timing anchor: the Gate Array
counts the CRTC's end of HSYNC, and an interrupt request generally starts about
1 us after that end. These passages do not provide a complete U.S.-ROM boot
chronogram or an exact register-and-reset fixture for reconstructing the
historical phase in simulation.

## Current integration boundary

- `rtl/CRTC.v` produces the raw CRTC HSYNC and VSYNC pins.
- `rtl/GA40010/syncgen.v` increments its six-bit interrupt counter from the
  end of CRTC HSYNC, asserts `INT_N` from that counter, and resynchronizes the
  counter from its VSYNC-derived timing.
- `rtl/GA40010/ga40010_test.v` integrates those production blocks and exposes
  raw CRTC timing plus GA `INT_N` to a testbench.
- `rtl/Amstrad_motherboard.v` sends raw selected CRTC VSYNC to PPI Port B bit 0
  and the same classic CRTC syncs to the Gate Array.

The repository therefore can characterize the phase produced by its present
model. It does not execute or identify the physical U.S. ROM, exercise the LK4
selection and firmware initialization path, or contain a hardware capture that
independently fixes the disputed same-scanline ordering. A test that injects a
reconstructed register vector and copies its expected interrupt edge from
`syncgen.v` would be circular.

## Rejected synthetic recipe

A guarded Gemini 3.7 Flash high read-only audit reached the same hardware-only
verdict and correctly identified the relevant CRTC-to-GA signal path. It also
suggested a concrete R7/scanline vector. That vector is not accepted: its stated
R7 position and line-262 ordering do not themselves establish the claimed
same-line-before-VSYNC relationship. No test expectation is derived from it.
This rejection is deliberate evidence preservation, not an unresolved RTL bug.

## Hardware discriminator

Use a documented U.S. CPC 6128 configuration and record:

1. ROM identity/hash, LK4 state, CRTC type, Gate Array type, and the live
   R0/R2/R3/R4/R5/R7/R9 values after firmware initialization.
2. A simultaneous steady-state logic-analyzer capture of raw CRTC HSYNC, raw
   CRTC VSYNC, and GA/Z80 `INT_N`, with interrupts acknowledged so successive
   requests remain observable.
3. A pre-trigger window around CRTC VSYNC that identifies the exact HSYNC used
   as the scanline convention and measures the `INT_N` falling edge relative
   to the CRTC VSYNC edge at sub-character resolution.

The French claim is confirmed only if that capture places the GA request on the
same defined scanline and before raw CRTC VSYNC. A current-model simulation is
useful as a comparison trace, not as hardware confirmation.
