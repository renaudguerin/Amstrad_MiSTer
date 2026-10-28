# SHAKER divergence candidates against hardware photographs (2026-10-27)

**Current follow-up:** `01F4` is fixed and hardware-confirmed (see
[recheck](shaker-followup-recheck-2026-10-27.md)). On 2026-10-28,
`03AE`/`03B4` display OK through cold-boot direct-E walks on both released
`321f96a` and old `cec641c`, with identical PNGs. Authentic production-T80
replay matches the expected address. An RFD precondition can reproduce WRONG
or repeated-OK layouts depending on retained parity phase in simulation.
The old WRONG observations remain path-specific evidence; the same history
on original CRTC1 is needed before treating this mechanism as an RTL defect.
See the [evidence unit](../session-logs/classic-overnight-2026-10-28.md).

The six AmSpirit-divergence candidates from the
[2.7 B–E report](shaker27-modules-b-e-2026-10-15.md) §5 are compared here against the Logon
System **Real Hardware** set (`cpc_CPC`, rank-1 authority), all on CRTC 1. Photographs were
fetched with `scripts/shaker-ref/scrape_logon.py --arch cpc_CPC --crtc 0 --crtc 1 --crtc 3`:
1350 images, no misses. They are stored ignored under `docs/specs/shaker-ref-logon/cpc_CPC/`, with the index at
`index-cpc_CPC.json`. The scraper overwrites `index.json`, so the AmSpirit index was restored
from `index-amspirit_lite-1.15.json`. MiSTer captures are the October `cec641c` Full
captures named below. Composites (hardware | AmSpirit | MiSTer) are ignored under
`docs/references/shaker-hw-compare-2026-10-27/`.

| Page | Test | Hardware photo | AmSpirit | MiSTer | Matches hardware |
|---|---|---|---|---|---|
| `01F4` | BR/E RS2INC IN HSYNC | line 2 `R0=0/R3=1/R52 CALC=#FF` | `#FF` | `#34` | **AmSpirit** |
| `03AE` | DE/A VMA=R12/R13 UPD WHEN C4=0 | "YOUR EMULATOR IS OK" | OK | WRONG | **AmSpirit** |
| `03B4` | DE/C same, 3-line variant | OK | OK | WRONG | **AmSpirit** |
| `019D` | B6/A OUTI STORY | `OUTI ON R7 LAST CHANCE 5TH uSec ON C0=0 - RES: #1E :GOOD` | `#1E` GOOD | `#1F` WRONG! | **AmSpirit** |
| `008D` | AI/A LOST VSYNC ON R0=0/1 | `PPI.B=#5F` on every line (bit 0 high) | `#1F` (bit 0 high) | `#FF`/`#FE` alternating: bit 0 low on the "SAME LINE" rows of both R0=0 blocks | **AmSpirit** (on bit 0) |
| `02EA` | DR/A VSYNC R7 UPD / VSYNC PROTECTION | all values and bit-0 patterns | same | same | **both** |

Upper PPI port B bits (`#5F` hardware, `#1F` AmSpirit, `#FF` MiSTer) are machine
configuration inputs: cassette, printer busy, /EXP, 50 Hz link and distributor. They are not CRTC
results, so only bit 0 (VSYNC) is compared.

The MiSTer captures come from the 2.6-path cells (`B1-26-full-a1`, `D1-26-full-a5`) and the
2.7 cell `A1-27-full-a1`. The 2.6 scripts use fixed waits, so a capture could in principle
show a different phase of an animated page. These six are static result pages, and the text
diff is the verdict on them.

## Consequence

Five of the six are genuine CRTC 1 divergences in the core, now backed by hardware rather
than by emulator agreement. Each has a hardware-confirmed expected value to derive a vector
from:

- `01F4`: R0=0/R3=1 R52 increment during HSYNC, expected `#FF` (rule claim to read in the
  ACCC HSYNC/R52 sections before writing the vector).
- `03AE`/`03B4`: VMA reload from R12/R13 when C4=0 after R4=0/R9 changes on the first lines
  of the frame (§20.3 video pointer).
- `019D`: OUTI-timed R7 write at the 5th µs of C0=0 must produce VSYNC result `#1E`.
- `008D`: VSYNC must stay asserted through the R0=0/R0>2 same-line cases (§16.4, R0=0).

`02EA` needs no action.
