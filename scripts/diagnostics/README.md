# Diagnostic fixtures

These small source fixtures exercise timing boundaries used by the public simulation benches. The classic B25 and Plus PA7 probes check CPU, Gate Array and ASIC interactions; the Plus hardware probes provide reproducible inputs for associated tests.

Run the relevant target through `python3 sim/select_tests.py --run` or consult [sim/TESTS.md](../../sim/TESTS.md) for a specific bench. A passing fixture verifies its stated boundary, not overall software compatibility.

## FDC read/reset probe

Build the standalone 6128 Plus cartridge and its distinctly named input disk:

```sh
python3 scripts/diagnostics/fdc_read_probe.py --disk rtl/u765/test.dsk \
  --output-dir output_files/fdc-read-probe --hold-active
```

Mount `fdc-read-probe-media.dsk` in drive A, then cold-boot
`fdc-read-probe.cpr`. No keypress or firmware is needed. The probe starts the
motor, selects non-DMA operation, recalibrates to track 0, and issues
`46 00 00 00 41 02 41 1e ff`. It uses 512 unrolled status/INI service blocks without screen updates, compares
all 512 bytes with the input EDSK's C0/H0/R41/N2 payload, and displays hexadecimal
read/mismatch counts, first mismatch offset (`FFFF` means none), the last MSR,
and seven raw result bytes (`ST0 ST1 ST2 C H R N`). Missing data counts as a
mismatch; missing results remain `--`. Polling is bounded and error stage/code
are visible. PASS scores the payload and receipt of seven result bytes; it does
not assert particular status/CHRN values at the disputed automatic EOT boundary.

The optional `fdc-read-probe-hold-active.cpr` waits to observe MSR high nibble
`F0`, then consumes no data. It displays live MSR, a progressing sample counter,
and a sticky `LEFT F0 EVER` flag. A physical FDC may overrun during this pause.
After 65,535 samples it explicitly changes to `MONITOR STOPPED`; that screen is
a final sample, not evidence of current activity. Reset/reload the normal CPR
for the fresh-read check. Neither variant proves host ACK quarantine.

The generator writes a byte-identical disk copy, extracted expected-sector
file, assembler listings/symbols, and JSON recording media/sector/artifact
hashes and sector offset. Cartridge code runs in RAM `8000` below `BE00`, uses
`2000-21FF` for data and a `BFF0` stack, and owns a Mode 2 screen at `C000`.
Assembly/layout checks alone do not qualify cold-boot or original-machine FDC
behavior; those require separate emulator/hardware evidence.
