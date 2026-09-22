// B20-2/B20-3 acknowledge-vector diagnostic (test-only, no RTL change).
//
// Scope: GA + asic_regs response to synthetic acknowledge stimulus driven
// through plus_p8_test_top's ga_m1_n/ga_iorq_n. The stimulus is a testbench
// bus wiggle, NOT production CPU timing and NOT the S29 board-level
// double-pulse mechanism (LK106/IC116/A13 glue, S29 p.3). It pins what the
// register file does when presented with acknowledge shapes; whether hardware
// ever presents the second shape is a separate board question (I2).
//
// Authority split (read the recommendation critically):
// - Physical claims (double acknowledge exists; second defaults to DMA0 /
//   offset 4; bug needs A13=0 and a memory-using instruction) are cited to
//   S29 (Plus Vectored Interrupt Bug, CPCWiki) only: S29 pp.3-4 for the two
//   acknowledges and the DMA0 default, S29 p.3 for A13 and instruction class.
// - CPU-model assertions (single IORQ fall per accepted interrupt, WZ latch
//   timing, WAIT stretch) are code-based (rtl/T80/T80pa.vhd, rtl/T80/T80.vhd)
//   and belong to the sibling B20 matrix bench, not to this file.
//
// Cases:
// - A1: two genuinely distinct acknowledges (full M1+IORQ release between).
//   First must be raster 0x06 with within-ack stability; second (nothing
//   pending, IVR[0]=0) is the S29 source scenario: S29 pp.3-4 expects DMA0
//   0x04, the current model yields DMA2 0x00. That mismatch is the named
//   EXPECTED-MISMATCH; the binary exits 0 while it persists and fails loudly
//   if it disappears or a first-ack invariant breaks.
// - A2: double pulse inside one M1 window (M1 held, IORQ low-high-low) with
//   DMA0 pending from SNA restore and IVR[0]=0. Records vector before/after,
//   DMA0 auto-clear and DCSR bit 7. All required-pass; the raster+DMA0
//   combination is NOT constructed here because the B4 attribution rule
//   (header pending explained by restored DCSR belongs to DMA) would credit a
//   joint restore to DMA alone -- documented, not worked around.
// - Idle probe: informational only. An isolated acknowledge with nothing ever
//   pending reports the current idle vector without any S29 comparison, so
//   arbitrary idle semantics stay unconstrained (no global fallback change).
//
// No HSYNC stimulus is applied between apply and acknowledges, so the DMA
// engine cannot raise dma_int_set concurrently with the acknowledge-edge
// clear (asic_regs.v:430 clear-dominant collaboration, comment at :416 notes
// set-dominant prose). DMA flag changes below come only from SNA restore and
// the acknowledge auto-clear path.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Vplus_p8_test_top.h"
#include "verilated.h"

namespace {

void fail(const std::string& msg) {
	throw std::runtime_error(msg);
}

std::string hex_str(uint32_t val, int width) {
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%0*X", width, val);
	return std::string(buf);
}

// --- Minimal SNA-controller setup copy (production path, as in plus_p8_test
// B8-5). Only what the acknowledge setup needs: global reset, header stream,
// chunk apply. Palette/sprite payloads are zero; only PRI/SPLT/SSA/SSCR/IVR
// (=0), SAR/PPR/DCSR, RMR2/unlock/seq-state matter here.

struct B8Header {
	uint8_t version   = 3;
	uint8_t inksel    = 0;
	uint8_t palette[17] = {0};
	uint8_t ga_config = 0x80;
	uint8_t crtc_sel  = 0;
	uint8_t crtc[18]  = {0};
	uint8_t romsel    = 0;
	uint8_t hcc       = 0;
	uint8_t line      = 0;
	uint8_t raster    = 0;
	uint8_t vta       = 0;
	uint8_t hsw       = 0;
	uint8_t vsw       = 0;
	uint8_t b0        = 0;
	uint8_t vsdelay   = 0;
	uint8_t intcnt    = 0;
	uint8_t intreq    = 0;
};

struct B8ChunkParams {
	uint8_t sar_lo[3] = {0x11, 0x44, 0x77};
	uint8_t sar_hi[3] = {0x22, 0x55, 0x88};
	uint8_t ppr[3] = {0x10, 0x20, 0x30};
	uint8_t dcsr = 0x00;
	uint16_t loop_cnt[3] = {0, 0, 0};
	uint16_t loop_addr[3] = {0, 0, 0};
	uint16_t pause_cnt[3] = {0, 0, 0};
	uint8_t pause_presc[3] = {0, 0, 0};
	uint8_t rmr2 = 0x19;
	uint8_t unlock = 1;
	uint8_t seq_state = 0;
	uint8_t pal_lo[32] = {0};
	uint8_t pal_hi[32] = {0};
};

void b8_ctl_quiesce(Vplus_p8_test_top& dut) {
	dut.reset = 0;
	dut.seam_machine_reset = 0;
	dut.seam_plus_asic_reset = 0;
	dut.seam_plus_mode = 1;
	dut.seam_sna_load = 0;
	dut.seam_use_ctl = 1;
	dut.sna_download = 0;
	dut.cpc_plus_chunk_start = 0;
	dut.cpc_plus_byte_wr = 0;
	dut.cpc_plus_byte_data = 0;
	dut.aregs_cs = 0;
	dut.aregs_mem_rd = 0;
	dut.aregs_mem_wr = 0;
	dut.aregs_addr = 0;
	dut.aregs_din = 0;
	dut.aregs_pal_raddr = 0;
	dut.dma_test_hsync = 0;
	dut.dma_ram_data = 0;
	dut.video_crtc_cs = 0;
	dut.video_crtc_rd = 0;
	dut.video_crtc_rs = 0;
	dut.video_crtc_din = 0;
	dut.mmu_test_A = 0;
	dut.mmu_test_mem_rd = 0;
	dut.mmu_test_io_wr = 0;
	dut.mmu_test_D = 0;
	dut.hdr_wr = 0;
	dut.hdr_addr = 0;
	dut.hdr_data = 0;
	dut.ga_iorq_n = 1;
	dut.ga_m1_n = 1;
	dut.ga_A = 0;
	dut.ga_D = 0;
	dut.pal_probe_sel = 1;
	dut.eval();
}

void b8_stream_header(Vplus_p8_test_top& dut,
                      const std::function<void()>& tick,
                      const B8Header& h) {
	auto put = [&](uint8_t addr, uint8_t data) {
		dut.hdr_wr = 1;
		dut.hdr_addr = addr;
		dut.hdr_data = data;
		tick();
		dut.hdr_wr = 0;
	};
	put(0x10, h.version);
	put(0x2e, h.inksel);
	for (int i = 0; i < 17; ++i) put(0x2f + i, h.palette[i]);
	put(0x40, h.ga_config);
	put(0x42, h.crtc_sel);
	for (int i = 0; i < 18; ++i) put(0x43 + i, h.crtc[i]);
	put(0x55, h.romsel);
	put(0xa9, h.hcc);
	put(0xab, h.line);
	put(0xac, h.raster);
	put(0xad, h.vta);
	put(0xae, h.hsw);
	put(0xaf, h.vsw);
	put(0xb0, h.b0);
	put(0xb2, h.vsdelay);
	put(0xb3, h.intcnt);
	put(0xb4, h.intreq);
	tick();
}

void b8_global_reset(Vplus_p8_test_top& dut,
                     const std::function<void()>& tick) {
	b8_ctl_quiesce(dut);
	dut.reset = 1;
	tick(); tick();
	dut.reset = 0;
	tick();
}

void b8_stream_chunk_apply(Vplus_p8_test_top& dut,
                           const std::function<void()>& tick,
                           const B8ChunkParams& p,
                           const B8Header& hdr = B8Header()) {
	auto stream_byte = [&](uint8_t b) {
		while (dut.sna_ioctl_wait) tick();
		dut.cpc_plus_byte_wr = 1;
		dut.cpc_plus_byte_data = b;
		tick();
		dut.cpc_plus_byte_wr = 0;
	};

	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	tick();
	b8_stream_header(dut, tick, hdr);
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;

	for (int i = 0; i < 2048; ++i) stream_byte(0x00);
	for (int i = 0; i < 128; ++i) stream_byte(0x00);
	for (int i = 0; i < 32; ++i) {
		stream_byte(p.pal_lo[i]);
		stream_byte(p.pal_hi[i]);
	}
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00);
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00);
	for (int i = 0; i < 10; ++i) stream_byte(0x00);
	for (int c = 0; c < 3; ++c) {
		stream_byte(p.sar_lo[c]);
		stream_byte(p.sar_hi[c]);
		stream_byte(p.ppr[c]);
		stream_byte(0x00);
	}
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00);
	stream_byte(p.dcsr);
	for (int c = 0; c < 3; ++c) {
		stream_byte(p.loop_cnt[c] & 0xFF);
		stream_byte((p.loop_cnt[c] >> 8) & 0xFF);
		stream_byte(p.loop_addr[c] & 0xFF);
		stream_byte((p.loop_addr[c] >> 8) & 0xFF);
		stream_byte(p.pause_cnt[c] & 0xFF);
		stream_byte((p.pause_cnt[c] >> 8) & 0xFF);
		stream_byte(p.pause_presc[c]);
	}
	stream_byte(p.rmr2);
	stream_byte(p.unlock);
	stream_byte(p.seq_state);

	dut.sna_download = 0;

	int loads = 0;
	int last_load = 0;
	for (int i = 0; i < 4000; ++i) {
		tick();
		int cur = dut.ctl_sna_load ? 1 : 0;
		if (cur && !last_load) ++loads;
		last_load = cur;
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 &&
		    i > 10)
			break;
	}
	if (loads != 1)
		fail("B20 setup: expected exactly one sna_load pulse, saw " + std::to_string(loads));
	tick(); tick();
}

void b8_stream_plain_apply(Vplus_p8_test_top& dut,
                           const std::function<void()>& tick,
                           const B8Header& hdr = B8Header()) {
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	b8_stream_header(dut, tick, hdr);
	for (int i = 0; i < 3; ++i) tick();
	dut.sna_download = 0;
	int loads = 0;
	int last = 0;
	for (int i = 0; i < 1000; ++i) {
		tick();
		int cur = dut.ctl_sna_load ? 1 : 0;
		if (cur && !last) ++loads;
		last = cur;
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 &&
		    i > 10)
			break;
	}
	if (loads != 1)
		fail("B20 setup: plain SNA produced " + std::to_string(loads) + " loads, expected 1");
	tick(); tick();
}

// One acknowledge pulse sampled every clock: returns the vector bytes seen.
// Asserts vec_valid throughout and within-ack stability (the source field is
// sampled on the first acknowledge edge and held, asic_regs.v:618-638).
std::vector<uint8_t> b20_ack_pulse(Vplus_p8_test_top& dut,
                                   const std::function<void()>& tick,
                                   int clocks, const std::string& where) {
	std::vector<uint8_t> seen;
	for (int i = 0; i < clocks; ++i) {
		tick();
		if (!dut.ack_vec_valid)
			fail("B20 " + where + ": vec_valid dropped mid-acknowledge at clock " +
			     std::to_string(i) + " (synthetic intack still asserted)");
		seen.push_back(dut.ack_vec_byte);
	}
	for (std::size_t i = 1; i < seen.size(); ++i) {
		if (seen[i] != seen[0])
			fail("B20 " + where + ": vector moved within one acknowledge (0x" +
			     hex_str(seen[0], 2) + " -> 0x" + hex_str(seen[i], 2) +
			     "); source must hold for the ack duration");
	}
	return seen;
}

void b20_release(Vplus_p8_test_top& dut,
                 const std::function<void()>& tick, int clocks) {
	dut.ga_m1_n = 1;
	dut.ga_iorq_n = 1;
	for (int i = 0; i < clocks; ++i) {
		tick();
		if (dut.ack_vec_valid)
			fail("B20 release: vec_valid asserted with no acknowledge (intack idle)");
	}
}

// --- Case A1: two genuinely distinct acknowledges; second-empty S29 scenario.
void test_b20_two_distinct_acks(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);

	B8Header h;
	h.intreq = 1; // restored pending raster (SNA B4)
	B8ChunkParams p;
	p.dcsr = 0x00; // no DMA flags, no enables; chunk IVR lands at 0x00
	b8_stream_chunk_apply(dut, tick, p, h);

	if (dut.aregs_ivr != 0x00)
		fail("B20 A1 setup: IVR is 0x" + hex_str(dut.aregs_ivr, 2) + ", expected 0x00");
	if (dut.ga_int_n_out != 0)
		fail("B20 A1 setup: no pending raster after B4=1 restore");
	if (dut.int_n_merged != 0)
		fail("B20 A1 setup: aggregate INT_n level lost after restore");
	if (dut.aregs_dcsr & 0x70)
		fail("B20 A1 setup: unexpected DMA flags 0x" + hex_str(dut.aregs_dcsr, 2));
	if (dut.aregs_dcsr & 0x80)
		fail("B20 A1 setup: DCSR bit 7 set before any acknowledge");

	// First acknowledge: raster vector 0x06, stable across the pulse.
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	auto first = b20_ack_pulse(dut, tick, 8, "A1 first");
	if (first[0] != 0x06)
		fail("B20 A1: first acknowledge gave 0x" + hex_str(first[0], 2) +
		     ", expected raster 0x06 (first-vector invariant broken)");
	b20_release(dut, tick, 4);
	if (!dut.ga_int_n_out)
		fail("B20 A1: first acknowledge did not retire the raster interrupt");
	if (dut.int_n_merged != 1)
		fail("B20 A1: aggregate INT_n not retired after raster acknowledge");
	if (!(dut.aregs_dcsr & 0x80))
		fail("B20 A1: first raster acknowledge did not set DCSR bit 7");

	// Idle window: nothing may move without stimulus (arbitrary idle
	// semantics preserved -- no phantom vectors, no provenance drift).
	for (int i = 0; i < 20; ++i) {
		tick();
		if (dut.ack_vec_valid)
			fail("B20 A1 idle: vec_valid asserted without acknowledge");
		if (!(dut.aregs_dcsr & 0x80))
			fail("B20 A1 idle: DCSR bit 7 drifted without stimulus");
		if (dut.ga_int_n_out == 0)
			fail("B20 A1 idle: raster interrupt re-asserted without stimulus");
	}

	// Second, distinct acknowledge with nothing pending. Current model pins
	// 0x00 (DMA2/offset 0, asic_regs.v:636 idle fallback); S29 pp.3-4
	// documents DMA0/offset 4 (0x04) for exactly this post-raster scenario.
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	auto second = b20_ack_pulse(dut, tick, 8, "A1 second");
	if (second[0] != 0x00)
		fail("B20 A1: second-empty acknowledge gave 0x" + hex_str(second[0], 2) +
		     ", expected current-model 0x00 -- behaviour moved, stop and document");
	b20_release(dut, tick, 4);
	uint8_t dcsr_after = dut.aregs_dcsr;
	if (dcsr_after & 0x80)
		fail("B20 A1: trailing acknowledge left DCSR bit 7 set (current model clears "
		     "last_raster on an empty second edge; behaviour moved)");
	if (dcsr_after & 0x70)
		fail("B20 A1: trailing acknowledge raised DMA flags 0x" + hex_str(dcsr_after, 2));

	std::printf("B20 A1: first=0x06 stable 8/8, DCSR bit7 0->1; idle 20 clocks quiet; "
	            "second=0x00 stable 8/8, DCSR bit7 1->0, DMA flags 0\n");
	std::printf("B20 A1 EXPECTED-MISMATCH: S29 pp.3-4 documents DMA0/offset4 (0x04) for the "
	            "post-raster second acknowledge; current model yields DMA2/offset0 (0x00)\n");
}

// --- Case A2: double pulse inside one M1 window with DMA0 pending.
void test_b20_double_pulse_dma(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);

	B8Header h; // B4=0: GA line stays clear; DMA path owns the pending level
	B8ChunkParams p;
	p.dcsr = 0x41; // ch0 flag + ch0 enable, stat 0; chunk IVR lands at 0x00
	b8_stream_chunk_apply(dut, tick, p, h);

	if (dut.aregs_ivr != 0x00)
		fail("B20 A2 setup: IVR is 0x" + hex_str(dut.aregs_ivr, 2) + ", expected 0x00");
	if (dut.ga_int_n_out != 1)
		fail("B20 A2 setup: GA line unexpectedly pending (B4 attribution broken)");
	if (dut.int_n_merged != 0)
		fail("B20 A2 setup: aggregate INT_n level lost for DMA-sourced pending flag");
	if (!(dut.aregs_dcsr & 0x40))
		fail("B20 A2 setup: DMA0 flag not restored (DCSR 0x" + hex_str(dut.aregs_dcsr, 2) + ")");
	if (dut.aregs_dcsr & 0x80)
		fail("B20 A2 setup: DCSR bit 7 set with no raster ever pending");

	// First pulse: DMA0 vector 0x04 while M1 held low throughout.
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	auto first = b20_ack_pulse(dut, tick, 4, "A2 first");
	if (first[0] != 0x04)
		fail("B20 A2: first pulse gave 0x" + hex_str(first[0], 2) + ", expected DMA0 0x04");

	// Gap: IORQ released, M1 held. No vector may show; the first edge with
	// IVR[0]=0 auto-clears DMA0 (S29's "DMA affected" shape, code path
	// asic_regs.v:370-373).
	dut.ga_iorq_n = 1;
	for (int i = 0; i < 3; ++i) {
		tick();
		if (dut.ack_vec_valid)
			fail("B20 A2 gap: vec_valid asserted between pulses of one M1 window");
	}

	// Second pulse, same M1 window: empty default after the auto-clear.
	dut.ga_iorq_n = 0;
	auto second = b20_ack_pulse(dut, tick, 4, "A2 second");
	if (second[0] != 0x00)
		fail("B20 A2: second pulse gave 0x" + hex_str(second[0], 2) +
		     ", expected current-model empty 0x00 after DMA0 auto-clear");
	b20_release(dut, tick, 4);

	if (dut.aregs_dcsr & 0x40)
		fail("B20 A2: DMA0 flag survived its acknowledge with IVR[0]=0 (auto-clear broken)");
	if (dut.aregs_dcsr & 0x80)
		fail("B20 A2: DCSR bit 7 set with no raster ever pending");
	if (dut.int_n_merged != 1)
		fail("B20 A2: aggregate INT_n not retired after DMA acknowledge");

	std::printf("B20 A2: first=0x04 stable 4/4, gap quiet 3/3, second=0x00 stable 4/4; "
	            "DMA0 auto-cleared (IVR[0]=0), DCSR bit7 stayed 0\n");
}

// --- Idle probe: informational only, no S29 comparison.
void test_b20_idle_probe(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);
	b8_stream_plain_apply(dut, tick);

	// Define IVR explicitly through the CPU page port (&6805).
	dut.aregs_cs = 1;
	dut.aregs_mem_wr = 1;
	dut.aregs_mem_rd = 0;
	dut.aregs_addr = 0x2805;
	dut.aregs_din = 0x00;
	tick();
	dut.aregs_cs = 0;
	dut.aregs_mem_wr = 0;
	dut.eval();
	if (dut.aregs_ivr != 0x00)
		fail("B20 idle setup: IVR page write did not land");

	uint8_t vec = 0xFF;
	int valid_clocks = 0;
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	for (int i = 0; i < 4; ++i) {
		tick();
		if (dut.ack_vec_valid) {
			vec = dut.ack_vec_byte;
			++valid_clocks;
		}
	}
	b20_release(dut, tick, 4);
	// Informational: current idle vector, deliberately NOT compared to S29
	// (S29's DMA0 claim is scoped to the post-raster window, not to every
	// idle acknowledge). No assertion constrains future idle semantics.
	std::printf("B20 IDLE (informational): isolated acknowledge with nothing ever pending gave "
	            "0x%s on %d/4 valid clocks (current model; no S29 comparison)\n",
	            hex_str(vec, 2).c_str(), valid_clocks);
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		{
			Vplus_p8_test_top dut;
			test_b20_two_distinct_acks(dut);
		}
		{
			Vplus_p8_test_top dut;
			test_b20_double_pulse_dma(dut);
		}
		{
			Vplus_p8_test_top dut;
			test_b20_idle_probe(dut);
		}
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
	std::printf("All B20 GA/register acknowledge-vector diagnostics PASSED "
	            "(A1 mismatch vs S29 pp.3-4 remains open by design).\n");
	return 0;
}
