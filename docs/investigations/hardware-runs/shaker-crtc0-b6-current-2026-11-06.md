# CRTC0 B6: four confirmed differences

Current `fdf348c` CRTC0 B6/`019D` differs on four of six rows from the
[original-CPC photograph](https://shaker.logonsystem.eu/images/cpc/CPC/B6_CRTC0_A.webp).
The native program also marks those four rows WRONG. This was previously
unknown coverage, not an established regression. No RTL repair is selected.

| Original row | Physical CRTC0 | Native fdf348c | Current CPCSyntaxError CRTC0 |
|---|---:|---:|---:|
| R0.JIT, fifth µs, C0=0 | 27 | 13 | 27 |
| R0.JIT, fourth µs, C0=0 | 14 | 14 | 14 |
| R0.JIT, fifth µs, C0=1 | 26 | 26 | 26 |
| C0=0, R0=0 | 27 | 26 | 27 |
| R7 last chance, fifth µs | 1E | 1F | 1E |
| R7 last chance, fourth µs | 1E | 1F | 1E |

Values are hexadecimal program results. The native six-value sequence is the
same as the current CRTC1 sequence, but the photographed expectations differ
by type. CRTC1's known residual is its fifth-microsecond R7 row alone. These
are upstream CPU/CRTC/PPI observables, without a sync-filter or CRT-curvature
explanation.

A single fresh isolated CPCSyntaxError v0.12.0 normal-boot run, direct original
Module B/key6, matches all six physical results. Original 2.7 disk and the
matching binary/source pins are verified; default package UK firmware, no
errata and no chaining are explicit. The marker image is readable and the later
image is black acquisition-phase evidence, retained without calling execution
failed. The parent views both and the full-resolution physical/native pair.
This is a useful matching peer path, not a physical sampling-aperture measurement.

The R0 rows complement the [new CRTC0 A4 discriminator](shaker-crtc0-a4-ai-photo-check-2026-11-06.md).
First qualify that original caller against French §7.2's successful-IN anchor
and §13.6.1's conditional counter continuation. Then bind the original type0
B6 write/counter/PPI ordering. Do not transpose the type1 counter-unwrap rule
or infer an E-fall-only register policy from these six numbers.

The native image is the current canonical CRTC0 B milestone walk, applied Full
mode and exact RBF SHA256
`17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`.
Private inputs, commands, media/binary/source pins, result images and parent
assessment are under
`local/overnight-20261103/resumed-20261104/c0-b6-cpcse-20261106/`;
the raw native capture remains in the milestone cell directory. The maintained
ledger changes this page from unknown to broken. No new bus delay, vector,
simulation gate, build or physical-hardware claim follows from this comparison.
