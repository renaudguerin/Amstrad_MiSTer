# B8-7: tape write lifetime (2026-09-08)

The tape download path now retains each write through synchronous completion.
The change covers: `rtl/sdram.v` tape request portion, the narrow
`Amstrad.sv` tape producer / `ioctl_wait` / address-ownership seam, the new
minimal helper `rtl/tape_write_queue.v` (wired in `files.qip` beside
`rtl/sdram.v`), and the physical-DQ fixture
(`sim/plus/sdram_cartridge_test.cpp`, `sdram_cartridge_test_top.v`, plus
Makefile). No classic CRTC, Plus FIELD/palette/snapshot, or TV80 change.

## Contract (fixed)

- Request/ACK lifetime: `sdram.v` acknowledges a tape **write** at
  `STATE_READ`, like the cartridge client. The synchronous producer therefore
  clears `tape_wr` at the READY edge and the next IDLE arbitration sees the
  cleared request — the completed write is not re-admitted. No other client's
  ack moved; arbitration is untouched.
- Latched payload: the accepted `tape_addr` **and** `tape_din` latch together
  at IDLE admission (`a`, `tape_write_data`). The CONT data stage uses the
  latched byte, never live `tape_din`.
- Backpressure: `tape_write_queue` asserts `tape_wait` while a byte is
  outstanding (`tape_wr && !tape_wr_ack`, gated by `tape_download`) into
  `ioctl_wait`. The sender honors backpressure: `sys/sys_top.v:259` freezes
  `rack`/`io_ack` under `ioctl_wait`, and `sys/hps_io.sv:633,688-693` emits
  each strobe plus address-advance once, with no HPS retry of a busy
  `ioctl_wr` — so a strobe ignored while busy would be lost, and `tape_wait`
  must cover the whole outstanding window. A strobe while busy is ignored;
  in the ACK cycle wait releases and a simultaneous ioctl strobe is accepted
  as the next byte.
- Ownership/drain: the drain owns a separate pending address latched at
  accept time. The owned mux is `tape_wr ? pending : (tape_download ? queued
  : play)`, and `Amstrad.sv` holds `tape_play_addr = 0` / `tape_reset = 1`
  while `tape_download | tape_pending`. A download end before the grant or at
  ACK keeps the pending address until the write completes.
- Clear vs reset: `clear` (`reset | Fn[2]`) affects only the logical
  last-address metadata (`tape_queued_addr`, observed as `tape_last_addr` and
  the progressbar max); the pending tuple drains unchanged. Machine `reset`
  dominates a simultaneous strobe and clears the held request, both address
  registers, and the payload. Reset cannot accept a new byte; a transfer
  already admitted by SDRAM may finish at its captured address.
- Reads unchanged: tape reads keep READY toggle/data timing. In production
  wiring, download/drain holds the player in reset (`restart_tape`) so
  `tzx_req == tzx_ack` and `tape_rd` stays 0. The fixture ties `tape_rd = 0`
  (parked input), so its zero bank-2 READ check pins parked input control
  only — it is not proof of the tzx player reset.

## Failing proof (before fix)

Behaviour-preserving extraction first (helper replicates the old overwrite /
immediate-mux / `wait = 0` contract), then four synchronous seam tests using
the real helper + real controller on one clock with the physical-DQ model
(C++ drives only download/ioctl, never `tape_wr`):

- single held byte duplicated the WRITE (saw 2, want 1);
- next payload one edge after ACK corrupted the previous address
  (`WRITE(120,35), WRITE(120,a6), WRITE(121,a6)` shape; no wait while busy);
- download end before grant (under held cart contention) and at ACK switched
  the mux to parked 0 and overwrote the header (extra WRITE, wrong address);
- simultaneous ACK+payload issued 3 WRITEs with the queued byte lost.

Before log (11 failures):
`docs/references/b8-7/provider-command-135.txt` (recovered provider output).

Independent review of the B8-7 delta after `7a58f88` also identified:

- P1 helper missing from `files.qip`: added
  `set_global_assignment -name VERILOG_FILE rtl/tape_write_queue.v` beside
  `rtl/sdram.v`.
- P1 Fn[2] clear redirected a delayed grant to header 0, and reset lost to a
  simultaneous strobe. Two new deterministic physical-DQ vectors, failing on
  the pre-fix helper (6 failures in
  `docs/references/b8-7/review-correction/before-fix.log`):
  - `test_tape_fn2_clear_preserves_pending_drain`: nonzero byte queued under
    cart-blocked grant, Fn[2] clear pulsed, download/cart released — requires
    exactly one WRITE to the original address, header preserved, logical last
    address 0;
  - `test_tape_reset_dominates_strobe`: reset+clear+strobe in one edge must
    hold no request and produce no WRITE (never a WRITE to 0).
- Fix: the accepted address latches into a separate internal `pending_addr`
  drained unchanged while `clear` touches only the logical metadata; `reset`
  dominantly clears held request, metadata, queue, and payload. The
  simultaneous ACK+normal-payload contract is unchanged.

After log: `docs/references/b8-7/review-correction/after-fix.log` (exit 0).
Full-gate logs in the same directory: `make-sim.log`, `lint.log`,
`soak.log`.

## Gates

- Focused SDRAM suite exit 0: B8-7, B8-4 coherence, cartridge/CPU/refresh
  and service vectors pass. Full simulation also retains the P10 passes and
  existing FDC XFAIL.
- `make -C sim` exit 0.
- `make -C sim lint` exit 0.
- `make -C sim soak SOAK_EXPECT=6e8258198d6e6137`: hash matches; tape path is
  outside the CRTC sampled projection, as expected.

## Hardware limits

No HPS cadence was measured and no CDT was loaded on hardware; the
simultaneous ACK+payload phase is verified against the `ioctl_wait`/producer
handshake in simulation, not against HPS timing. No Quartus synthesis, RBF,
or title/playback claim is made. Real-CDT end-to-end (download → header
intact → playback first bytes) remains hardware work.

## Independent review

Astra medium independently reviewed the foreign-authored tape lifecycle,
production wiring and tests. It required the production manifest entry and
separation of the pending address from clearable metadata, including dominant
reset. Its narrow follow-up cleared both corrections after the failing-before
and passing-after vectors were preserved. HPS backpressure is not strobe
replay, and the parked read-input control is not a player-reset test.

The coordinator authorized this native cross-provider fallback after Opus 5
exhausted its session quota. No hardware or Quartus clearance is implied by
the source review and simulation gates. Parent verification logs are retained
alongside the worker logs as `parent-full-sim.log`, `parent-lint.log` and
`parent-soak.log`.
