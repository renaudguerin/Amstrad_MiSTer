# SHAKER follow-up recheck, 2026-10-27

The Mac mini rechecked the previously delivered `925b5bb` RBF on MiSTer after
repairing the temporary MBC helper. The CRTC0 wedge no longer prevents B1 from
reaching `00FC`. CRTC1 `01F4` passes on both `925b5bb` and `321f96a`.

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

## CRTC1 BR/E: hardware-confirmed fix

The `925b5bb` and `321f96a` captures both show `CALC=#34` for R0=1 and
`CALC=#FF` for R0=0, matching the Logon original-CPC photograph. A prior
transcription mistakenly read the second row as `#34`. The final PNGs from
both builds are identical, SHA-256
`cb92619e8b78022a8d416bf570f5edc80b05dda2b9039355ffd2eea7118967fe`.

The 14:10 replay on `321f96a` used classic CRTC1A, Plus off, Full sync, SSM
on and French keyboard. The RBF SHA-256 is
`6bc437243db9ef5f9db2349f420520a46364dee4b7954ccc95e96f55ba36400d`.
Its manifest is `docs/references/01f4-final-window-2026-10-27/manifest.json`:
18 commands, six captures, zero failures and nine SSM records.

A scratch production-T80/GA replay of the authentic pair supports the fix:
the R0=1 case reaches the IRQ handler, while R0=0 keeps HSYNC high and reaches
the `FF` fallback. The fixture does not capture the first formatted value and
omits ROM banking, keyboard, PSG/PlayCity and prior menu history; it is a
path and edge observation, not a full-machine formatter test. Sources and
traces are under `.roster-scratch/01f4-analysis/`.

## Retained evidence and restoration

Private captures and manifests are under
`docs/references/shaker-followup-2026-10-27/{b1-crtc0-r2,br-crtc1}/` (the
repository's `docs/references` alias resolves to `docs/specs`). The first
CRTC0 attempt used `/usr/bin/python3`, which lacked Pillow; it failed before
operating the device. The working host `python3` has Pillow 13.0.0.

The successful manifests verify original CFG restoration to SHA-256
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`.
A final device read confirmed that hash and `CORENAME=MENU`. No captures,
reference photographs, media, helpers or toolchains are committed.
