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
- CPR/system reset cleared command-side request tokens independently of the SD
  arbiter. The first repair dropped the outward request and cleared quarantine as
  soon as ACK was sampled low. That remains ambiguous because `hps_io` polls
  `sd_rd`/`sd_wr` as held requests and may return the old ACK later, after u765 has
  issued a different mount, track-info, or sector request. The arbiter now retains
  the cancelled request and its ownership until the old ACK has risen and fallen,
  while rejecting its buffer burst. Only then may reset-driven metadata reload
  issue a fresh request.

The same global-ownership rule now gates track-info request issue and completion.
Without it, a short reset released before the cancelled sector ACK could enqueue,
discard, and then falsely complete an unaccepted track-info request behind the
still-busy sector owner. A two-track discriminator resets during track-1 READ
DATA, releases reset immediately, retires the old response later, and requires a
real track-0 metadata reload before sector 1 can be read. The pre-fix controller
timed out waiting for the new sector request; the gated controller completes the
payload, consumes all seven result bytes, and returns to idle.

The standalone harness uses production `CYCLES=4000` and the production
one-in-eight `ce_u765` cadence. In addition to the A0 write alias, EDSK
recognition, and SENSE DRIVE STATUS checks, it now creates a copyright-free
two-track EDSK with one sector per track in memory and runs a public-command
READ DATA transfer.
The expected LBA and all 512 payload bytes come from that independently built
image. Its reset discriminator interrupts the active sector request before ACK,
requires the request/LBA to remain owned, returns the cancelled burst while reset
is asserted, verifies the buffer quarantine, and then requires metadata reload
and a complete fresh READ DATA transfer.

The older mount/status path still accepts the tracked `rtl/u765/test.dsk`. The
locally supplied, ignored `docs/fdctest/fdctest/test.dsk` (SHA-256
`55d0516725d39acb7096ab22ea079e936ba86596889e5b8abe5cfb978b729b44`) is not a
committed fixture. Its source uses `&FB7E` for main status and `&FB7F` for data,
and explicitly covers ready, track 0, busy bits, DTL=0, READ ID, SCAN, and
status/error cases.

The controller's `SECTOR_SIZE` helper was also rewritten into portable Verilog
function-result syntax so Verilator and the Quartus 17-era compiler see an
explicit assignment on every call. The new READ DATA slice covers ordinary N=2;
N=0/DTL behavior still requires a separate discriminator before repair.

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
- Sector request issue/completion still tests only `sd_busy_sector`, not global
  arbiter ownership. A drive-1 metadata reload overlapping a drive-0 READ DATA
  could therefore discard the sector token and stream stale sector RAM. This is
  argued from the production state machines, not yet reproduced; it needs a
  two-drive overlap fixture before repair and cannot explain a drive-A-only run.
- The synthetic READ DATA gate consumes all seven result bytes and pins ST0,
  ST1, ST2, H, and N, but deliberately does not freeze C/R. The local NEC
  reference describes C/R updates when the processor terminates a command, while
  this fixture reaches automatic EOT without a TC seam. A focused termination
  discriminator or hardware trace is required before changing those fields.
- Controller reset invalidates cached sector metadata but does not explicitly
  reset the independent `sector_search_state`. A discriminator which resets
  during sector-info parsing is required before deciding whether its retained
  state is benign resumption or a second reload defect.
- WRITE DATA asserts the sector-RAM `buff_wr` strobe in one execution state and
  clears it in the next, but controller reset does not explicitly clear it.
  Reset in that one-state window could retain a stale port-B write; it needs a
  focused WRITE DATA/reset fixture before changing the write path.
- Production still hard-wires `available=2'b11`; SENSE DRIVE STATUS can report
  ready while media parsing or motor readiness will make a later command fail.
- A cancelled request now waits indefinitely for its ACK high/low completion.
  This closes the demonstrated late-response alias without inventing a timing
  bound, but unlike the earlier reset-and-retry behavior, a host-side failure
  which never acknowledges now leaves the controller blocked until power-cycle.
  This deliberate recovery tradeoff is safe against stale responses but cannot
  recover through the current level-only interface. A bounded fallback that can
  also reject a still-later response requires a host transaction epoch/tag.

The next useful expansion is READ ID plus selected fdctest status/DTL/busy
vectors. Full top-level CPU timing, a non-acknowledging-host recovery contract,
and real hardware remain separate acceptance layers.
