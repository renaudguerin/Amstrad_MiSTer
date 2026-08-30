# FDC integration review — 2026-08-30

## Current position

The corrected shared classic/Plus address decoder is present at integration tip
`5a8f801`, but it did not exercise the real controller or mounted-media path.
This pass adds a production-timed u765 regression target to the normal simulation
gate and fixes two independently reproduced integration defects:

- CPC writes with A0=0 were selected by the board decoder but rejected by the
  u765 instance. The CPC I/O map uses A0 only to choose status/data on reads;
  both writes address the data register. `Amstrad.sv` now maps `A0 | io_wr` into
  the controller, with a black-box command/parameter regression through A0=0.
- CPR/system reset cleared command-side request tokens but not the independent SD
  arbiter. Reset during an unacknowledged mount request left `sd_rd` and
  `sd_busy_mount` asserted indefinitely. The arbiter now cancels outward requests,
  ACK history, and all ownership flags on reset. It then quarantines delayed ACK
  and buffer-write traffic until ACK has been observed low before issuing a fresh
  request, preventing a cancelled TRACKINFO burst from entering the sector buffer.
  The regression injects both delayed ACK and stale buffer-write traffic and
  requires no retry while they remain active, followed by a fresh request.

The standalone harness now uses production `CYCLES=4000` and the production
one-in-eight `ce_u765` cadence. It verifies reset recovery, the A0 write alias,
EDSK recognition, and SENSE DRIVE STATUS ready/track-0 bits. Both the tracked
`rtl/u765/test.dsk` and the locally supplied `docs/fdctest/fdctest/test.dsk`
(SHA-256 `55d0516725d39acb7096ab22ea079e936ba86596889e5b8abe5cfb978b729b44`)
pass this mount/status path. The supplied fdctest source uses `&FB7E` for main
status and `&FB7F` for data, and explicitly covers ready, track 0, busy bits,
DTL=0, READ ID, SCAN, and status/error cases.

The controller's `SECTOR_SIZE` helper was also rewritten into portable
Verilog function-result syntax so Verilator and the Quartus 17-era compiler see
an explicit assignment on every call. This is not claimed as a disk fix: the
current transaction slice does not yet exercise non-default N values, so a
READ DATA/DTL expansion remains required before assigning title causality.

## What this does and does not explain

These are real defects, but neither is proven to be the cause of The Demo's
reported `disc missing`: the hardware session did not record the RBF SHA, disk
hash, FDC menu setting, or first motor/status/data port. A pre-`1d1795b` RBF also
contained the already-corrected classic decoder regression, which remains a
credible alternative. The next hardware run must capture those identifiers and
the first FDC port sequence before assigning causality.

## Residual review findings

The new gate is deliberately a first real transaction slice, not a claim that
the controller passes fdctest:

- `rtl/u765/u765.sv` still handles only the first image-signature byte before
  trusting geometry. Invalid/truncated-image and same-drive remount cases remain
  untested.
- The D1 main-status busy expression uses `seek_state[0] == 2` in the drive-1
  term. A focused two-drive seek discriminator is required before changing it.
- For N=0, `i_bytes_to_read` is assigned DTL directly. The supplied fdctest has
  explicit DTL=0 cases; a full read-data transaction must pin the real zero-DTL
  result before repair.
- SCAN commands are stubs and FORMAT does not modify media, both already marked
  TODO in the controller.
- Drive-B EDSK sector-length parsing uses command-selected `ds0` in one path
  rather than the drive currently being scanned. This cannot explain an ordinary
  drive-A failure but needs a drive-B fixture.
- Production still hard-wires `available=2'b11`; SENSE DRIVE STATUS can report
  ready while media parsing or motor readiness will make a later command fail.

The next useful expansion is a bounded READ ID/read-sector transaction against
the fdctest image, followed by selected fdctest status/DTL/busy vectors. Full
top-level CPU timing and real hardware remain separate acceptance layers.
