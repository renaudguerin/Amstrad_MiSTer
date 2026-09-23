# Eerie Forest: malformed outer RIFF length

The old parser rejected the local Eerie Forest CPR before CPU execution
because its outer RIFF length extends 128 bytes beyond EOF. All 32 cartridge pages are present.
A header-only correction boots on the existing hardware build, establishing the
container mismatch as the immediate failure cause. The requested compatibility
change accepts the original image at a complete-chunk EOF. The
[exact-build hardware retest](../investigations/hardware-runs/plus-cartridge-originals-41a1f27-2026-09-23.md)
boots the unchanged original; a later striped forest frame remains static.

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

## Requested core compatibility

The parser accepts physical EOF while awaiting the first byte of a new chunk,
provided at least one cartridge byte was supplied, all prior chunks (including
odd-length padding) completed, and the backend has no pending write or error.
The existing exact-RIFF-end path remains valid. It does not infer or fill
missing payload bytes, and partial chunk IDs, lengths, data and padding still
abort. Streaming extent checks and the RIFF size limit remain in force. In particular,
an overstatement of only 1–7 bytes still aborts at the preceding boundary: it
cannot contain another eight-byte chunk header. This narrow allowance handles
Eerie's +128-byte case; it is not general repair of every incorrect RIFF length.

This is a deliberate compatibility allowance for complete chunks under an
incorrect outer size. It cannot distinguish an overstated size from a file
which lost an entire later chunk; accepting that ambiguity is the requested
tradeoff. It must not extend to a partially supplied chunk. The related
[AmstradDiagnostics PR 18](https://github.com/llopis/amstrad-diagnostics/pull/18)
describes a different error: a declared 16 KiB payload was 20 bytes short.
That malformed image remains rejected by this policy.

The synthetic complete-32-page regression and partial-chunk controls belong in
the maintained parser suite because EOF publication interacts with parser
state and backend flow control. The one-shot output above records the old
strict behavior; it is not the new acceptance expectation.

Validation after the compatibility change:

- `make -C sim/plus run/plus_cpr_parser_tests` failed before the RTL edit with
  `FAIL: synthetic Eerie Forest CPR with overstated RIFF length failed to commit`;
  the same command passed afterward, ending `PASS: all Plus CPR parser tests`.
  Gemini implementation evidence is in
  `/tmp/agents-roster-runs/20260923T053652Z-13323-7f2d/output.log`.
- `python3 sim/select_tests.py --run` ended
  `select_tests: PASS 2 benches: run/plus_cpr_parser_tests, run/p0_boot_tests`.
  Log: `/tmp/eerie-selected-gate.log`. No RTL/test edits followed this gate.
- Rebuilt the scratch production-T80 fixture with the modified parser and ran
  the original CPR for 128,000,000 master ticks. It completed successfully.
  `cmp sim/plus/obj_dir/eerie/corrected-trace.log
  sim/plus/obj_dir/eerie/original-fixed-trace.log` returned 0: recorded fetch,
  I/O and periodic PC observations are identical to the header-corrected image
  on the old parser. This compares the trace projection, not every internal
  state bit or rendered pixel.
- Fresh read-only Astra review of the Gemini-authored RTL/tests returned
  **CLEAR, no must-fix findings**. It confirmed the publication guards and
  zero-length/padded transitions. Its documentation clarification for the
  retained 1–7-byte rejection is included above. It noted optional coverage
  for EOF with the final byte still pending at the new boundary; the unchanged
  `!load_valid` guard is correct by inspection, while the existing pending-byte
  regression covers mid-payload EOF. No review debt remains for this diff.

The [exact synthesized-build retest](../investigations/hardware-runs/plus-cartridge-originals-41a1f27-2026-09-23.md)
boots the unchanged original CPR through the Logon System scene. A later
striped forest frame repeats after 45 seconds; full demo progression is open.
