# SHAKER follow-up recheck, 2026-10-27

The Mac mini rechecked the previously delivered `925b5bb` RBF on MiSTer after
repairing the temporary MBC helper. The CRTC0 wedge no longer prevents B1 from
reaching `00FC`. The CRTC1 `01F4` numerical discrepancy remains.

## Identity and operation

- RBF: `/media/fat/_Computer/Amstrad_20261027_925b5bb.rbf`, SHA-256
  `9ba5e34be1bd6d0eba4d9dcf6a46b686648b6abacef1b093903252d72aed77eb`.
- CRTC0: the supplied `CSL_27/MODULE B/SHAKER-B-0.CSL`, stopped and captured
  after line 18 (`wait_ssm 0x00FC`).
- CRTC1: the supplied B-1 script's boot and BR sequence (key `R`, then four
  spaces), ending at `wait_ssm 0x01F4`. This avoids walking unrelated tests.
- Full sync filter, SSM enabled; the script selects the CRTC type. The private
  disk and scripts are unchanged. The capture manifests retain media hashes,
  applied CFG bytes and all transport actions.
- Helper: `/tmp/mbc-fixed`, SHA-256
  `f2b3a4f3cbd98c7a13f8770eb4c893bc352eae9d204f92f664f5eaad6c697330`;
  see the [build repair](mister-mbc-cross-build.md#known-build-specific-crash-and-local-repair-2026-10-27).
  The original `/tmp/mbc` remains untouched.

## CRTC0 B1: progress restored

The run completed with four captures, zero capture failures and three SSM
records (`00FA`, `00FB`, `00FC`). The final pattern capture SHA-256 is
`d20eecd2bf8f3e510757e5dac9c1f8843049d253f84da84ce032f598a7370d54`.
It has the same main column/stripe and split-pattern arrangement as Logon
System's original-CPC photograph `CRTC0/B1_00FC_R9E6-P-ODD_A2.webp`.
The native scaler image has a noisy bottom strip and cannot establish exact
analogue geometry or the inferred comparator release point.

The [wedge repair](crtc0-ivm-exit-wedge-fix-2026-10-27.md) therefore passes its
bounded progress check. Author question 21, concerning the release edge after a
non-matching IVM exit, remains open.

## CRTC1 BR/E: numerical residual confirmed

Both the SSM capture and a second explicit capture show:

- `R0=1/R3=1/R52 CALC=#34` (expected `#34`).
- `R0=0/R3=1/R52 CALC=#34` (original CRTC1 expects `#FF`).

Their SHA-256 is
`cb92619e8b78022a8d416bf570f5edc80b05dda2b9039355ffd2eea7118967fe`.
Logon System's original-CPC photograph `CRTC1/BR_01F4_E.webp` reads `#FF` on
the second row. The [pin-level infinite-HSYNC repair](crtc1-infinite-hsync-01f4-2026-10-27.md)
is insufficient to close the integrated CRTC/GA/interrupt result. This is a
failed hardware acceptance check, despite the simulation vectors passing.

The run completed with six captures, zero capture failures and five SSM records.
All ten PNGs across the two runs were inspected. The other BR pages are route
context, not a new acceptance claim for their timing rules.

## Production replay narrows the `01F4` residual

A scratch replay of authentic BR caller `4FD6`, including both calls to `502A`,
uses production T80pa, GA40010 `syncgen_sync`, normal WAIT and the current CRTC.
The first R0=1 case reaches IRQ handler `5062`. The second R0=0 case keeps raw
HSYNC high throughout the roughly 3335 µs NOP sled and reaches fallback `50A1`,
which sets H=`FF`. No HSYNC falling edge occurs during that second sled, and no
HSYNC-off event is pending when R0 becomes 0. The fixture does not capture the
first formatted value, so this is a path/edge observation rather than an exact
`#34/#FF` formatter regression.

The raw CRTC HSYNC feeds the GA interrupt path directly; video filtering cannot
explain this difference. The relevant CRTC, GA, T80 and motherboard sources
match `925b5bb`. The retained MGL names that RBF explicitly. These checks do not
prove the physical FPGA image or full-machine state matches the fixture.

The fixture loads the private module at `3900` into flat 64 KiB RAM and boots
with DI, SP=`4065`, IM1, detected type byte `(BF03)=1`, and a jump to `4FD6`.
Its CRTC defaults are `3F 28 2E 8E 26 00 19 1E 00 07`. Authentic setup,
synchronization, HALTs, writes and the first formatter execute; ROM banking,
keyboard, PSG/PlayCity and prior menu history are absent. Sources, full entry
contract and traces remain under `.roster-scratch/01f4-analysis/`.

There is no failing production vector or justified RTL repair yet. The next
hardware discriminator should retain the authentic R0-write phase, then park
the CPU while R0=0 remains active. One proposed private-payload patch replaces
three bytes at `0102`, after `ED 71` at `0100`, with `DI; JR $` (`F3 18 FE`).
Snapshot the actual CRTC type, R0/R2/R3, C0, HS/HSC, GA R52 and IRQ before the
result handler restores R0=63; repeat with R0=1 as the pulse control. This
proposal was not run. Compare the captured state before deciding whether the
missing difference is runtime history, loading/synthesis, or a CRTC/GA rule.

## Retained evidence and restoration

Private captures and manifests are under
`docs/references/shaker-followup-2026-10-27/{b1-crtc0-r2,br-crtc1}/` (the
repository's `docs/references` alias resolves to `docs/specs`). The first
CRTC0 attempt used `/usr/bin/python3`, which lacked Pillow; it failed before
operating the device. The working host `python3` has Pillow 13.0.0.

Both successful manifests verify original CFG restoration to SHA-256
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`.
A final device read confirmed that hash and `CORENAME=MENU`. No captures,
reference photographs, media, helpers or toolchains are committed.
