# B8-4: coherent retained video words

The SDRAM controller now keys its retained video word by the full physical
word address and bank. An admitted CPU, cartridge or tape write to that key
invalidates it, so the next available video slot refetches the word even when
the raster address does not move. Initialization invalidates the key. The
admitted bank is captured alongside the address for every client.

CPU edge admission, cartridge acknowledgements, tape acknowledgements, client
priority and refresh scheduling are unchanged. Transactions are serialized:
a write cannot overlap an active video transaction; invalidation remains set
until a later video admission. This is a shared-memory coherence repair, not
a CRTC timing or title-specific fix.

## Physical memory evidence

The physical-DQ fixture accepts the production CPU `0x0100` alias at bank 0,
physical byte `0x20100`, initially containing word `0x3412`. Writing low byte
`0x56` must yield video word `0x3456` without an address change and preserve
the high byte. Original RTL retains `0x3412`; moving the address away and back
is a passing control. Original RTL also fails bank-only refetch and a write
admitted in the slot immediately after a video fetch. All three cases pass
with the repair. The next-slot check inspects the completed word without
advancing through the next arbitration edge.

## Production motherboard byte and pixel witness

The P10 fixture runs a straight-line CPU program: configure the raster and
palette, execute 256 NOPs, write `0xFF` to CPU `0x0000`, then HALT. The physical
SDRAM model observes the actual bank-0 WRITE to `0x20000`. The programmed
raster (`R0=0, R1=1, R2=255, R4=0, R6=1, R7=127, R9=0`) keeps the video word
address at zero before and after that write. The test forbids an address
excursion that could hide stale data through an ordinary refetch.

The opt-in production clock path registers `ce_16` and the SDRAM reference
from the same free-running divider, as in `Amstrad.sv`. At edge E with the
divider at zero, enables register high; consumers see them at E+1, when the
SDRAM slot counter becomes zero; E+2 admits the CPU request. The older
independently phased fixture path remains the default for existing tests.

The test snapshots enables and sources before each edge and asserts the
resulting destination state afterward. The verified chain is:

- Physical WRITE `0x20000=FF` at tick `4636429`, after 20851 baseline ticks.
- Retained video word becomes `0x00FF` at `4636442`, preserving the high byte.
- Pre-edge `cclk_p` and `mb.vram_d=FF` produce `plus_vidword[7:0]=FF` at `4636470`.
- Pre-edge `CLKEN & PIXEN` loads `vid_even=FF` and resets `pix_cnt=0` at `4636490`.
- The following qualifying PIXEN presents white RGB at `4636494`, four clocks later.

GA programming requests mode 2 with ROMs enabled (`0x82`), but without HSYNC
the pixel decoder remains in mode 0. Pens 1–15 are programmed white; mode-0
`FF` selects pen 15. The baseline is nonwhite grey (`0x666`), not the intended
black. This fixture therefore proves coherence and the observed byte-to-pixel
path; it does not certify palette-write behavior or mode-2 timing. No CPU-model
changes or branch-instruction assumptions are part of the program.

The strengthened test was also rerun with original SDRAM RTL: the physical
WRITE occurs, but word, consumed byte and shifter remain zero and RGB remains
grey. The repaired RTL was restored exactly afterward.

## Gates and independent review

Focused SDRAM and P10 tests, full `make -C sim`, aggregate lint and soak pass;
the soak remains `0x6e8258198d6e6137`. Final logs are retained locally under
ignored `docs/references/b8-4/review-correction/`. Original failing controller
and production logs are under `docs/references/b8-4/`; the strengthened
original-RTL run is preserved in the provider output under
`docs/references/b8-4/final-assertion/`.

Astra medium independently inspected the foreign-authored RTL and fixtures.
It found no RTL blocker and required two test corrections: actual pre/post-edge
load observation and removal of an extra arbitration tick. Its narrow
follow-up cleared both corrections. The requested Opus 5 review returned no
report because its session quota was exhausted; the coordinator authorized
the native cross-provider fallback. Earlier fixture diagnosis alone was not
used as final clearance.

This is simulation and source-review evidence. No Quartus fit, hardware test,
SHAKER/DSC4 or named-title closure is claimed. Dedicated new regressions for
cartridge/tape cache invalidation, high-address changes and bank changes during
admission remain coverage residuals; review found no defect in those paths.
