# Original Plus cartridges on `41a1f27` — 2026-09-23

This run tests the **unchanged** Switchblade and Eerie Forest CPRs on the first
full-effort build containing both startup repairs. It closes their immediate
non-loading reports. Eerie Forest has a separate, repeatable later stall.

## Exact build and setup

- Source: integration commit `41a1f277b5ed71c629f5935f0e688a98a3e6680b`.
  [Actions run 35824567406](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35824567406)
  passed selected simulation, production-T80, full synthesis and required gate.
  Retained build provenance is `build_mode=clean_full`; TimeQuest's reported
  minimum setup slack is 0.770 ns and minimum hold slack is 0.217 ns. These
  reports do not establish timing on unconstrained external paths.
- `Amstrad_20260923_41a1f27.rbf` SHA-256:
  `6c36309368659edbbfe1044e09a804639f6b7ec9c02b68526ff7d488e122b331`.
  The Actions artifact and copy under MiSTer's `_Computer` directory matched.
- Explicit MiSTer CFG: 6128+ model and Full sync filter, SHA-256
  `13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
  After all captures, the original CFG was restored and its device SHA-256
  matched `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
  The saved CFG and native captures do not prove the live OSD setting.

## Switchblade

The device cartridge matched the original commercial CPR SHA-256
`d958e2b1eeebaa227aa33c4f0f5627fc238a2791fd177f7ebffefdf81a07ff78`.
Eight serial 768×273 native captures from one fresh load show the Gremlin
Amstrad intro, Wild Blade/Switchblade graphics and a recurring high-score
screen. The earlier `8b18ac0` run gave six identical black captures. The
immediate black-screen load failure is **hardware-reproduced fixed** on this
build. The captures do not establish joystick input, gameplay, audio or a
complete title cycle.

## Eerie Forest

The device cartridge matched the original CPR SHA-256
`72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215`.
It was not header-corrected. Eight serial native captures show the demon intro
and Logon System scene. This confirms that the parser accepts the original
complete-chunk file despite its overstated outer RIFF length, and that the
immediate non-loading failure is **hardware-reproduced fixed**.

The same run reaches a striped forest frame in capture 6; captures 6–8 are
byte-identical (SHA-256
`582fd77b999079edde860e0b3a2e1007c53716a12e91d1531ccb47f5465cf50e`).
A second fresh load, with the first capture after 45 seconds, produced four
more byte-identical frames with that same hash. AmSpirit Lite 1.15.1 on the
same original CPR advances from the Logon System scene into the animated
forest at frames 1,500 and 2,000. This comparison identifies a **later
progression or rendering residual**, not its RTL cause; native MiSTer captures
cannot distinguish a CPU stall from a static scaler buffer or prove raw sync.
Full demo completion, sound and exact physical output remain open.

## Retained evidence

The case JSON, manifests, and native PNGs for the two original-media runs,
Eerie Forest's delayed repeat, and the later AmSpirit comparison are under
ignored `local/task-archives/plus-compatibility-2026-09-23/4bd0/docs/specs/plus-load-2026-09-23/new-build-41a1f27/`
in the main checkout. The full-effort RBF is retained under ignored
`output_files/Amstrad_20260923_41a1f27.rbf` in the main checkout. The
corrected-header Eerie CPR was comparison media on the old build, not used
in these acceptance runs.
