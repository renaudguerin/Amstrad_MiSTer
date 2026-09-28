// GA/register pulse controls complement production b20-bus-diag.
// Gerald's physical capture, 29 July 2017: raster06 then empty04 within M1.
// https://oldwiki.cpcwiki.eu/imgs/7/7e/IM2_Plus_Ack_Bug.png
// Separate M1 cycles and never-pending idle acknowledges retain the existing
// unspecified00 default. DMA2 auto/manual-clear controls pin pending priority.
// Snapshot setup supplies pending flags; no HSYNC occurs during these probes.

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

// Distinct M1 cycles are outside the measured split-ack scenario.
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

	// A new M1 window with no pending request retains the idle default.
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	auto second = b20_ack_pulse(dut, tick, 8, "A1 second");
	if (second[0] != 0x00)
		fail("B20 A1: second-empty acknowledge gave 0x" + hex_str(second[0], 2) +
		     ", expected current-model 0x00 -- behaviour moved, stop and document");
	b20_release(dut, tick, 4);
	uint8_t dcsr_after = dut.aregs_dcsr;
	if (dcsr_after & 0x80)
		fail("B20 A1: later CPU acknowledge did not replace raster provenance");
	if (dcsr_after & 0x70)
		fail("B20 A1: trailing acknowledge raised DMA flags 0x" + hex_str(dcsr_after, 2));

	std::printf("B20 A1: first=0x06 stable 8/8, DCSR bit7 0->1; idle 20 clocks quiet; "
	            "second=0x00 stable 8/8, DCSR bit7 1->0, DMA flags 0\n");

}

// Primary trace: two pulses within M1, first clears source; second empty is04.
// DCSR retains first-pulse provenance for the handler (regression candidate,
// docs/plus/ack-provenance-regression-2026-09-26.md); the physical trace only
// establishes the vectors. DMA manual-clear must keep DMA2/00 on both pulses.
void test_b20_double_pulse_dma(Vplus_p8_test_top& dut) {
    auto tick = [&]() { dut.clk=0; dut.eval(); dut.clk=1; dut.eval(); };
    for (int source=0; source<3; ++source) {
        b8_global_reset(dut,tick);
        B8Header h; B8ChunkParams p;
        h.intreq = source==0;
        p.dcsr = source==0 ? 0 : 0x14; // DMA2 flag+enable, no raster
        b8_stream_chunk_apply(dut,tick,p,h);
        if (source==2) {
            dut.aregs_cs=1; dut.aregs_mem_wr=1; dut.aregs_addr=0x2805; dut.aregs_din=1;
            tick(); dut.aregs_cs=0; dut.aregs_mem_wr=0; tick();
        }
        dut.ga_m1_n=0; dut.ga_iorq_n=0;
        auto first=b20_ack_pulse(dut,tick,4,"split first");
        if (first[0] != (source==0 ? 6:0)) fail("split first source wrong");
        if ((dut.aregs_dcsr & 0x10) != (source==2 ? 0x10:0)) fail("first pulse DMA clear incorrect");
        if (bool(dut.aregs_dcsr & 0x80) != (source==0)) fail("first DCSR provenance wrong");
        dut.ga_iorq_n=1;
        for (int i=0;i<3;++i) { tick(); if(dut.ack_vec_valid) fail("vector in gap"); }
        dut.ga_iorq_n=0;
        auto second=b20_ack_pulse(dut,tick,4,"split second");
        if(second[0] != (source==2 ? 0:4)) fail("second pulse must be empty04 or pending manual DMA2/00");
        if(bool(dut.aregs_dcsr & 0x80) != (source==0)) fail("second pulse overwrote first-ACK DCSR provenance");
        if((dut.aregs_dcsr & 0x10) != (source==2 ? 0x10:0)) fail("second pulse DMA clear incorrect");
        b20_release(dut,tick,4);
        std::printf("PASS split source=%d first=%02X second=%02X DCSR=%02X\n",source,first[0],second[0],dut.aregs_dcsr);
    }
}

// PA6e: asic-reference §7 [ARNOLD-REV §2.7, KT]: simultaneous requests
// arbitrate raster > DMA2 > DMA1 > DMA0. IVR=0 uses auto-clear; acknowledging
// one source must retain the others. DCSR flags are DMA0/1/2 at bits6/5/4.
// Thus vectors 06,00,02,04 leave flags 70,60,40,00 respectively.
void test_pa6e_simultaneous_priority(Vplus_p8_test_top& dut) {
    auto tick = [&]() { dut.clk=0; dut.eval(); dut.clk=1; dut.eval(); };
    b8_global_reset(dut, tick);
    B8Header h;
    // Restore a quiet, normal-length display with no pending sources. B4
    // cannot encode simultaneous raster+DMA pending: restore deliberately
    // attributes it to DMA when DCSR already explains the request.
    h.crtc[0] = 63;
    h.crtc[1] = 40;
    h.crtc[2] = 46;
    h.crtc[3] = 0x8E;
    h.crtc[4] = 38;
    h.crtc[6] = 25;
    h.crtc[7] = 30;
    h.crtc[9] = 7;
    B8ChunkParams p;
    b8_stream_chunk_apply(dut, tick, p, h);
    auto write = [&](uint16_t addr, uint8_t value) {
        dut.aregs_cs = 1;
        dut.aregs_mem_wr = 1;
        dut.aregs_addr = addr;
        dut.aregs_din = value;
        tick();
        dut.aregs_cs = 0;
        dut.aregs_mem_wr = 0;
        tick();
    };
    try {
        // Raise the raster request at PRI line 1 and each DMA flag by executing
        // INT+STOP live. All channels stop, so no later fetch can replenish flags.
        write(0x2800, 1);
        write(0x2C0F, 0x07);
        dut.dma_ram_data = 0x4030;
        dut.dma_test_hsync = 1;
        tick();
        dut.dma_test_hsync = 0;
        for (int i = 0; i < 20000; ++i) {
            tick();
            if (!dut.ga_int_n_out && (dut.aregs_dcsr & 0x70) == 0x70) break;
        }
        if (dut.int_n_merged || dut.ga_int_n_out || (dut.aregs_dcsr & 0x70) != 0x70)
            fail("PA6e setup: expected INT low, raster pending and DMA flags 70; INT_n=" +
                 std::to_string(dut.int_n_merged) + " raster_pending=" +
                 std::to_string(!dut.ga_int_n_out));
        const uint8_t vectors[] = {0x06, 0x00, 0x02, 0x04};
        const uint8_t flags[] = {0x70, 0x60, 0x40, 0x00};
        for (unsigned i = 0; i < 4; ++i) {
            if (dut.int_n_merged) fail("PA6e priority: pending source disappeared");
            dut.ga_m1_n = 0;
            dut.ga_iorq_n = 0;
            auto seen = b20_ack_pulse(dut, tick, 8, "PA6e simultaneous priority");
            if (seen[0] != vectors[i]) fail("PA6e priority: wrong vector at source " + std::to_string(i));
            b20_release(dut, tick, 4);
            if ((dut.aregs_dcsr & 0x70) != flags[i]) fail("PA6e priority: wrong residual DMA flags");
            if (bool(dut.aregs_dcsr & 0x80) != (i == 0)) fail("PA6e priority: wrong raster provenance");
        }
        if (!dut.int_n_merged) fail("PA6e priority: interrupt held after all sources cleared");
        std::printf("PASS PA6e: simultaneous raster/DMA2/DMA1/DMA0 vectors 06/00/02/04\n");
    } catch (const std::exception& e) {
        fail(std::string(e.what()) + " (actual vector=0x" + hex_str(dut.ack_vec_byte, 2) +
             ", DCSR=0x" + hex_str(dut.aregs_dcsr, 2) + ")");
    }
}

// Original-Plus PA7 overlap photographs require DMA04 then raster06 when
// compatible matures during the DMA acknowledge. The long synthetic pulse
// isolates the GA/register interaction; the unchanged CPR separately proves
// reachable production-T80 timing. No internal silicon phase is inferred.
void test_pa7_compatible_during_dma(Vplus_p8_test_top& dut) {
    auto tick = [&]() { dut.clk=0; dut.eval(); dut.clk=1; dut.eval(); };
    for (bool manual : {false, true}) {
        b8_global_reset(dut, tick);
        B8Header h;
        h.crtc[0]=63; h.crtc[1]=40; h.crtc[2]=49; h.crtc[3]=0x88;
        h.crtc[4]=38; h.crtc[6]=25; h.crtc[7]=30; h.crtc[9]=7;
        h.intcnt=51;
        B8ChunkParams p;
        p.dcsr=0x40; // pending DMA0, stopped; B4=0 leaves compatible idle
        b8_stream_chunk_apply(dut, tick, p, h);
        auto write = [&](uint16_t address, uint8_t value) {
            dut.aregs_cs=1; dut.aregs_mem_wr=1;
            dut.aregs_addr=address; dut.aregs_din=value;
            tick(); dut.aregs_cs=0; dut.aregs_mem_wr=0; tick();
        };
        if (manual) write(0x2805, 1);
        unsigned guard=0;
        while (dut.ga_raw_int_n_out && ++guard < 6000) tick();
        if (dut.ga_raw_int_n_out || !dut.ga_int_n_out)
            fail("PA7 setup: expected undelivered compatible request");
        dut.ga_m1_n=0; dut.ga_iorq_n=0;
        auto first=b20_ack_pulse(dut, tick, 100, "PA7 DMA across compatible delivery");
        if (first[0] != 4) fail("PA7: first source must be DMA0/04");
        if (dut.aregs_dcsr != (manual ? 0x40 : 0))
            fail("PA7: compatible maturation changed DMA clear/status provenance");
        if (dut.ga_int_n_out || dut.ga_raw_int_n_out)
            fail("PA7: compatible request lost inside DMA ACK");
        b20_release(dut, tick, 4);
        if (manual) write(0x2c0f, 0x40); // handler clears manual flag
        if (dut.ga_int_n_out) fail("PA7: DMA cleanup cleared compatible request");
        dut.ga_m1_n=0; dut.ga_iorq_n=0;
        auto second=b20_ack_pulse(dut, tick, 8, "PA7 retained raster");
        if (second[0] != 6 || dut.aregs_dcsr != 0x80)
            fail("PA7: retained raster must give vector06/status80");
        b20_release(dut, tick, 4);
        if (!dut.int_n_merged) fail("PA7: sources not retired after two ACKs");
        std::printf("PASS PA7: DMA%s vector04 stable across compatible maturation, then 06/80\n",
                    manual ? " manual" : " auto");
    }
}

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
			test_pa6e_simultaneous_priority(dut);
			test_pa7_compatible_during_dma(dut);
		}
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
	std::printf("All B20 GA/register acknowledge-vector diagnostics PASSED.\n");
	return 0;
}
