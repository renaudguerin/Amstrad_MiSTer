# V1: CTM644 AFC circuit constraints

The CPCSyntaxError candidate's **73-line settling constant is not derived from
the CTM644 circuit**. The Sanyo sample supports the product 4.7 ms, but its
resistor differs from the CTM service drawing, and that product does not specify
the complete AFC loop response. Preserve it as an empirical model parameter.

## Primary sources and verified connections

Read the rendered Sanyo application circuit, No.725 p.3, and the Amstrad CTM644
schematic, printed p.29 / PDF p.26. Amstrad's parts lists, printed pp.27–28 /
PDF pp.24–25, independently confirm the principal values.

| Pin-1 network | Sanyo sample | CTM644 drawing |
|---|---|---|
| Series branch | 1 µF + 4.7 kΩ | C414 1 µF/50 V + R418 8.2 kΩ |
| Shunt capacitor | 0.01 µF | C413 0.01 µF |
| Connection to oscillator pin 2 | 56 kΩ | R417 56 kΩ |

The series branches return to the **positive decoupled pin-15 supply**, not
directly to ground. Treating that supply as AC ground is an approximation.
Pin 2 also has capacitance and H-hold loading; its 56 kΩ connection contributes
to pin-1 loading. The CTM drawing and parts list disagree on H-hold potentiometer
values, so one exact bias resistance is not established for every revision.

## What the products establish

Assume a stiff supply, omit pin-2 loading and internal IC impedance, and let
Cs=1 µF, Cf=0.01 µF. The passive pin-1 driving-point impedance is

```text
Z(s) = (R + 1/(s Cs)) || 1/(s Cf)
     = (1+s R Cs) / [s(Cs+Cf)(1+s R Cs Cf/(Cs+Cf))]
```

This has an integrator, a finite zero and a finite pole. The zero time constants
are 4.7/8.2 ms (73.4/128.1 normal 64 µs lines); the finite pole time constants
are about 46.5/81.2 µs. Neither is automatically the closed-loop settling time.
An imposed ideal voltage step gives another valid interpretation of R·Cs as
the series capacitor's charging time, not the oscillator phase response.

A physical response prediction still needs detector/output impedance, oscillator
control gain, loading and horizontal-phase feedback. Do not replace 73 with 128
and call the model circuit-derived, or change RTL based on these products alone.

## Reproduction and provenance

[Sanyo LA7800 datasheet](https://www.rcscomponents.kiev.ua/datasheets/la7800-datasheet.pdf),
SHA-256 `620ae7ba5342ac215fc8ab285338ef79e813a1a77a63cd810a0ece9122bcdca4`;
[Amstrad service manual](https://www.cpcmania.com/docs/Manuals/CPC6128%20Service%20Manual.pdf),
SHA-256 `97cfae7c8eff4b8022c3d6bae61d452ac5824a33ad5ab3535c9d5ec6b1add78e`.
Manufacturer documents are hosted by mirrors. Original PDFs, PDF Inspector
reports and relevant renders remain ignored in `continuation/monitor-circuit/`
under the V1 private evidence archive. No production change follows this check.
