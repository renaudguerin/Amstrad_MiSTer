# Eerie Forest: malformed outer RIFF length

The local Eerie Forest CPR is rejected before CPU execution because its outer
RIFF length extends 128 bytes beyond EOF. All 32 cartridge pages are present.
Changing only the outer length allows the existing parser to commit and the
production-T80 fixture to execute the title. This establishes a media-container
failure; it does not establish complete title or hardware acceptance.

## Exact input and correction

Investigation base: `b9edac29688344d1c2ac00bfcd265f9c153f7cb3`.
Input: ignored `local/test_media/cartridges/Eerie_Forest_(Logon_System_2017).cpr`.

| Property | Original | Corrected comparison copy |
|---|---|---|
| File size | 524,556 bytes | unchanged |
| SHA-256 | `72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215` | `852ddae9bb083c3a41044a567ee5ed2eb013accc3c305d45e1fdd3eb4d357d50` |
| RIFF length at offsets 4–7, little endian | 524,676 (`84 01 08 00`) | 524,548 (`04 01 08 00`) |
| Declared end, length + 8 | 524,684 | 524,556 |

The `AMS!` container has contiguous `cb00` through `cb31`, each declaring and
containing 16,384 bytes. The final chunk starts at `0x7c104`, and its payload
ends exactly at EOF. There are no missing cartridge payload bytes. The actual
repair changes just byte 4 (`84` to `04`); neither chunk headers nor ROM data
change. The original remains untouched.

To produce the comparison copy, run from the repository root with an unused
output filename; no copyrighted cartridge data is committed:

```python
from pathlib import Path
import hashlib
import struct

source = Path("local/test_media/cartridges/Eerie_Forest_(Logon_System_2017).cpr")
image = bytearray(source.read_bytes())
assert hashlib.sha256(image).hexdigest() == (
    "72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215"
)
struct.pack_into("<I", image, 4, len(image) - 8)
output = Path("/tmp/eerie-riff-corrected.cpr")
with output.open("xb") as stream:
    stream.write(image)
assert hashlib.sha256(image).hexdigest() == (
    "852ddae9bb083c3a41044a567ee5ed2eb013accc3c305d45e1fdd3eb4d357d50"
)
```

## Local discriminator

`plus_cpr_parser.v` commits on download completion only when it reached
`STATE_DONE`, has a block, and has no pending write/error. With the original
image it instead waits at the beginning of another chunk (`STATE_CHUNK_ID`),
because the final complete page ends before the declared RIFF boundary.
Dropping `cpr_download` then aborts.

A one-shot synthetic probe reused `plus_cpr_parser_test.cpp`'s fixture and built
32 full pages filled with their page index. Its two inputs differ only in the
outer length: the original title's 524,676 versus the correct 524,548. Both
accept all payload bytes; their publication results are:

```text
ORIGINAL writes=524288 commits=0 aborts=1
CORRECTED writes=524288 commits=1 aborts=0
```

The scratch probe is `sim/plus/obj_dir/eerie/parser_probe.cpp`, executable
`sim/plus/obj_dir/eerie/parser/parser_probe`. It is deliberately not a new suite
vector: no RTL rule changed, and the malformed-input policy is already tested.

A separate scratch D5 fixture uses the generated production T80, production
clocking and READY, 6128+ model, existing motherboard/MMU/SDRAM composition and
unchanged title payload. Original input fails with
`CPR parser aborted while applying download`. Corrected input commits and runs
for 128,000,000 master ticks, reaching RAM execution around `C12D` and recurring
Gate Array mode writes. This is execution evidence, not an image comparison or
proof that later effects work. The existing D5 fixture's peripheral and external
memory modelling limits still apply.

Reproduction artifacts are ignored under `sim/plus/obj_dir/eerie/`: `trace.cpp`,
`trace.mk`, `build.log`, `corrected-trace.log`, and the corrected CPR. Build with
`make -C sim/plus -f obj_dir/eerie/trace.mk eerie-build`; run
`sim/plus/obj_dir/eerie/build/eerie_trace <CPR>`.

## Acceptance boundary

The parent task performed the paired real-MiSTer check on 2026-09-23:

- RBF `8b18ac0`, SHA-256
  `26185f485ac72a6be7e3ee17f9fc2e57a2e09c595ba8061d53a9aca90c960da2`.
- Explicit CFG: 6128+ and Full; identical build for both media variants.
- Original CPR: immediate failure; no screenshot file produced in two loads,
  with 15-second and 45-second capture deadlines. Main remained running.
  Absence of a screenshot alone is not proof of a parser fault; the local
  parser discriminator supplies that evidence.
- Corrected CPR: six serial native captures changed continuously; capture 1
  shows the Eerie demon intro and capture 6 the Logon System scene.
- AmSpirit accepts the original malformed image and reaches intro/later scenes
  at frames 100/500. This demonstrates emulator tolerance, not RIFF validity.

The parent supplied device evidence at `/tmp/plus-eerie-riff-fixed-20260923`
(six PNGs and `manifest.json`) and `/tmp/plus-eerie-baseline-20260923`
(`manifest.json`), and owns durable capture retention and device cleanup.
The earlier [title retest](../investigations/hardware-runs/plus-titles-8b18ac0-2026-09-23.md)
remains valid for the original malformed image. Boot/progression is now
hardware-confirmed with the corrected container; full demo completion and
all audiovisual effects were not assessed.

No production code is changed. Prefer repairing this known media header over
relaxing the parser's deliberate malformed-container policy. If compatibility
with malformed originals is later required, specify that policy separately:
EOF after complete pages cannot generally prove that further declared chunks
were never intended.
