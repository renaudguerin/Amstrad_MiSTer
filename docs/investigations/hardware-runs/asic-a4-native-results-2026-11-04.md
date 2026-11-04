# ASIC A4 native result observation — 4 November 2026

The last four A4 rows already have VSYNC bits 0/0. The earlier report read the
native E/F glyphs incorrectly. Native result RAM, enlarged text and the
preserved replay now agree; no ASIC timing change is justified for that claim.
The first and third rows still differ from the older original-Plus photograph.

## Paired native control and observer

Both runs use e574a46, RBF SHA256
`f95bac8d60821cee62329d9314bd63811936a8102c892f47d484cfa74ae19df9`, Full sync,
6128+ and the French cartridge SHA256
`ab241580c9b6a9fa9aeae94ca6847ea70dca386d305e38fc2ac03fce603360cf`.
Original disk SHA256 is
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
The separately named A4-only diagnostic is
`17ea16ab0afb081fee3e75e9b3d5270443363cd9d2d745a5b6f3fc7f17de28bb`.

The reviewed observer occupies 30 bytes of unselected test8 code, with two
changed CALL operand bytes and 30 actual disk differences. It emits low six
bits of 74F8..7507 after all measurements/formatting and restores the CALL.
A strict current-CPU private replay proves the original measurement trajectory,
complete RAM apart from the exact code projection, marker order and normal
returns. The earlier stack-overlapping padding placement remains rejected.
This media is only for A4; its test8 is intentionally unusable.

The original run completes 11 commands/one capture/zero capture failures.
The diagnostic records one contiguous 16-byte burst, HH16 down to HH1,
followed by normal 0037: sequence 0..16, written17, no dropped/lost/restarted
records and 47 free ring slots. All 17 diagnostic PNGs are pixel-identical to
the original control. The original control is also pixel-identical to the
preserved 7ac11a2 A4 PNG, so this is not a new Plus fix on e574.

| Addresses, high byte first | Native low-six-bit bytes | VSYNC-bit pair |
|---|---|---|
|74F8/74F9|3E/3F|0/1|
|74FA/74FB|3F/3F|1/1|
|74FC/74FD|3E/3F|0/1|
|74FE/74FF|3F/3F|1/1|
|7500/7501|3E/3E|0/0|
|7502/7503|3E/3E|0/0|
|7504/7505|3E/3E|0/0|
|7506/7507|3E/3E|0/0|

The rendered words are FEFF/FFFF/FEFF/FFFF/FEFE/FEFE/FEFE/FEFE. The observer
does not transport bits6..7; the rendered full words and transported bit0 are
separate observations. Simulation F4F5/F5F5/F4F5/F5F5/F4F4/F4F4/F4F4/F4F4 has
the same VSYNC bits and different machine PPI inputs. No sync filter can alter
these CPU-read result bits.

## Runner failure and acceptance boundary

Keep the diagnostic's raw exit2/failed manifest. Although its single poll
received all 17 records including 0037, synchronously processing 17 screenshots
crossed the 120-second wait deadline, producing a false “no SSM0037 arrived”
error. The completed PNGs and raw ring metadata qualify the byte observation;
the run is not relabeled successful, and no repeat is needed to recover data.
The separate runner fix `ccd49d4` retains a failing-first batch/capture test;
223 offline hardware-loop tests and the selected 44-bench gate pass. It uses
poll completion time for the marker deadline, preserving genuine late-poll
and absent-target failures. No data-recovery repeat was required.

Parent independently viewed the original native page, enlarged its E/F glyphs,
viewed the physical Plus photograph and checked every diagnostic PNG by exact
RGB equality to that inspected control. The photograph has first-four 1/1 and
last-four 0/0; current native/replay/AmSpirit agree on first/third 0/1 instead.
The photo lacks the current additional C9 OUTI row, so exact preceding-history
and program-revision equivalence remain unproved. Preserve those two remaining
photo discrepancies without inventing a new R0/VSYNC rule.

Evidence: ignored `asic/a4-observer-8946/`, especially
`native-original-e574-r2/`, `native-instrumented-e574/` and
`native-observer-qualified.json`. Existing bounded R0 deadline acceptance is
unchanged; this does not establish full ASIC compliance.
