# D5 BASIC boot production input repair

**Source repair integrated; hardware acceptance pending.**

Scope: CPC Plus production `/EXP` configuration. The accepted source rationale
is in [architecture](architecture.md#additional-evidence-and-implementation-handoff).
The decoder's live low→page1/high→page3 polarity is preserved. The bounded repair
sets the input low for both 6128 Plus and 464 Plus; GX4000 continues to override
ordinary ROM selection. Both BASIC cartridges are executed separately on both
CPC Plus models; this does not establish their physical factory strap states.

## Reproduction

User-owned `Plus_EN.cpr` and `6128_FR.cpr` remain ignored under
`docs/plus/cartridges/06_System/`. No cartridge bytes are changed. Run:

```sh
make -C sim/plus d5-boot GHDL=/path/to/ghdl
make -C sim
make -C sim lint
```

The opt-in `d5-boot` target requires both exact SHA256 values recorded in the
[D5 handoff](architecture.md#production-t80-controlled-experiment); missing or
changed files fail the gate. Private firmware and GHDL are not added as
requirements of the ordinary simulation suite.

`prepare_d5_boot.py` regenerates the original experiment's production-T80
adapter in `obj_dir`. It copies the actual `plus_exp_n` assignment from
`Amstrad.sv` into the fixture (renaming its model input), adapts case-sensitive
GHDL port names/defaults and diagnostic taps, enables the real PPI/YM2149/HID,
and uses default wait timing with the shared production divider. It does not
rewrite the MMU. A missing production assignment fails generation.

The firmware monitor captures the actual T80 A register at executed TXT OUTPUT
(`BB5A`) calls. English uses a PS/2 F1 event through the menu branch; French is
menu-free with no key injection. Only actual `Ready` is success. `missing` is
an explicit failure; reaching the execution limit without `Ready` is a timeout
failure. BASIC entry/page/opcode and disc-branch observations are diagnostics,
not substitutes for the output requirement. ROM-select and fetch traces have
independent print budgets.

A synthetic CPR control executes ROM-select OUT instructions followed by upper
window reads on GX4000, 6128 Plus and 464 Plus: ROM0, ROM7 and all 128–255 direct
selections. This checks model input and MMU composition. The existing MMU suite
retains both live `/EXP` levels and GX4000 override tests.

## Local results

Before changing production RTL, the required boot monitor ran each unchanged
6128 image with the high production input. English emitted `disc missing`
with seven FDC writes; French emitted it with six. Both exited 1, rather than
classifying a timeout as a reproduced symptom. After the input change:

| Model | Firmware | TXT OUTPUT | Disc missing | FDC writes |
|---|---|---|---|---|
| 6128 Plus | Plus_EN | Ready | No | 4 |
| 6128 Plus | 6128_FR | Ready | No | 3 |
| 464 Plus | Plus_EN | Ready | No | 0 |
| 464 Plus | 6128_FR | Ready | No | 0 |

The 464 runs validate these two supplied firmware images in the 64K/no-FDC
configuration, not an inventory of 464-specific cartridges. Input correction
is relevant to both CPC Plus models; GX4000 retains its independent MMU override.

The final combined `make -C sim/plus d5-boot` gate passed all four firmware
runs and the three model-control sets after checking both cartridge hashes.
Validation used Verilator 5.052 and GHDL 6.0.0 LLVM. `make -C sim` and
`make -C sim lint` passed; the existing `fdc-payload-poll` XFAIL remains.
The three executed model-control sets passed. A scratch copy of the final
driver with its global execution bound reduced to one tick exited 1 with
`timeout without firmware Ready output`; no production/test-source timeout
bound was changed for the passing firmware runs.

Fresh Gemini 3.8 Flash high review returned **CLEAR**, run
`20260911T012620Z-3103-ead3`, exit 0 / clean. It inspected configuration
extraction, T80 adaptation and accumulator tap, firmware failure/success rules,
model controls and build isolation. No Opus call was made. The reviewer did
not rerun the full gates. Its broad “zero risk” language is limited here to
inspected isolation paths; it is not hardware acceptance. The fixture does
execute Plus video logic, but rendered text is not the success oracle.

Logs and the raw review are retained locally under ignored
`docs/references/d5-input-validation-2026-09-11/`.

## Acceptance boundary

This uses the production T80 VHDL translated with GHDL, CPR loader/service,
SDRAM controller/model, motherboard and real Plus I/O. The classic GA is still
stubbed. Full top-level elaboration, vendor video, rendered text, physical SDRAM
and real hardware are outside this fixture. Hardware BASIC `Ready` with an
empty drive requires a full-fit RBF and MiSTer retest. See
[current status](../current-status.md) for integration and build provenance.
