# B3 frame-harness foundation review — 2026-09-03

Read-only review of committed `a98590a33345f2c529fcd6d30676235d4193bfd7`,
principal commit `12672d928ee2e6cb5a8e7cae26af73c39ba8a13f` ("test: establish
B3 frame harness foundation"). Active working-tree diff ignored — the FDC
writer is preserved untouched. Verdict: **CLEAR**. No actionable correctness
bug, protocol race, or overclaiming narrative hazard in the committed B3
frame-harness foundation.

Core files examined: `sim/plus/p10_boot_test_top.v`,
`sim/plus/p10_boot_test.cpp`, `rtl/Amstrad_motherboard.v:98-114`,
`Amstrad.sv:685-712`, `rtl/sdram.v:121-280`, `rtl/Amstrad_MMU.v:80-91`.

## 1. Request-to-physical-command correlation

Source anchors: `sim/plus/p10_boot_test.cpp:708-752`,
`rtl/sdram.v:170-175,222-226`. Client arbitration occurs only at slot cycle
`q == STATE_IDLE`; a winning `vram_req` registers on `posedge clk`. The
harness samples `source_word_addr` prior to `h.tick()`, matching the setup
value latched into `sdram.v` register `a` on the arbitration edge. The
rising-edge detector arms a single transaction; overlapping grants are guarded
by `require(!transaction_pending)`. The sequencer emits `CMD_ACTIVE` at `q ==
STATE_START` and `CMD_READ` at `q == STATE_CONT`; the harness requires exactly
one ACTIVE before READ and ends the pending transaction at READ. READ completes
well before slot release, so no command skew or slot overlap is possible.

## 2. 15-bit word to 23-bit byte mapping and bank timing

Source anchors: `sim/plus/p10_boot_test_top.v:405-409`,
`Amstrad.sv:710-712`, `rtl/Amstrad_MMU.v:86`, `rtl/sdram.v:235-250`,
`sim/plus/p10_boot_test.cpp:731-745`. The motherboard drives 15-bit
`vram_addr` (`{ma_sel[13:12], ra_sel[2:0], ma_sel[9:0]}`); production
`Amstrad.sv:711` connects `.vram_addr({2'b10, vram_addr, 1'b0})`, zero-extended
to `{5'd0, 2'b10, vram_addr, 1'b0}` at the 23-bit `sdram.v` port — the fixture
wires the same expression explicitly, bit-equivalent. Bit 17 establishes base
`0x20000`, aligned with the CPU base-RAM mapping (`Amstrad_MMU.v:86`).
`physical_address` is reconstructed from the SDRAM pins
(`SDRAM_A <= a[21:9]` at ACTIVE; `SDRAM_A <= {~a[0] & wr, a[0] & wr, 2'b10,
a[22], a[8:1]}` at READ) and required equal to the expected address, proving
the motherboard VRAM address reaches the pins. `SDRAM_BA` updates at
`STATE_START` from the synchronously latched `vram_bank`; the bank is checked
inside `CMD_ACTIVE` — the earlier pre-command sampling issue is remediated.

## 3. Raw/selected timing vs shared filter-dependent payload

Source anchors: `sim/plus/p10_boot_test_top.v:132-152,715-735`,
`rtl/Amstrad_motherboard.v:677-684,731-761`. `dbg_raw_*` taps `mb.hs_sel`,
`mb.vs_sel`, `mb.de_sel` before `crt_filter`; `dbg_selected_*` taps the
motherboard monitor outputs after `crt_filter_output_select`. There is no
filter-independent "pre-filter" pixel payload: `crtc_shift` alters `vram_d`
byte assembly upstream of palette lookup and RGB generation
(`:679-684`), so both observation points see the shared filter-dependent
payload. The `dbg_video_*` labelling is accurate and does not misrepresent
pipeline stages. Raw/selected timing plus raster coordinates, VRAM bus taps,
and color outputs give the next capture slice its hook points without
invasive motherboard patches.

## 4. Scope discipline

Docs and comments consistently describe observability infrastructure only. No
claims of full-top `Amstrad.sv` synthesis/simulation (the harness instantiates
`Amstrad_motherboard` + `sdram.v` directly), ASCAL integration, production
VHDL T80 simulation (the Verilator-side CPU is the reduced TV80 surrogate
under the T80pa-shaped wrapper), or runtime CPR frame generation / golden
image comparison (deferred to the next B3 slice).

## Non-blocking future growth (deferred to later B3 slices)

- Filter-parameter sweep: the fixture defaults to `SYNC_FILTER = 2'd2`
  (Off/raw) so the shared default cannot mask the B7 ASIC-GA mutation;
  filtered-scanline capture benches can instantiate Full explicitly.
- Extended address coverage: `test_p10a_vram_client_wiring` requires 32 reads
  over ≥4 unique addresses during normal scanout; split-screen, R5 line
  offsets, or DMA audio/VRAM fetches can extend it as those subsystems join
  the capture writer.

## Retained limits

Off/raw default, 32-read/≥4-address coverage, no runtime CPR writer or image
oracle yet, TV80-surrogate CPU boundary (polling-instruction limits per
pending FDC findings), unfinished capture work preserved in stash
`0fe18a4513a47e4f21e0f504f002673a853388c3` — not part of the accepted
foundation; recover only owned paths. A CLEAR verdict is not hardware
closure. Published ACCC v1.11 unchanged.
