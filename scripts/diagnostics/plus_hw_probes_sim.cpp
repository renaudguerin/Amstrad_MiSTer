// Production-T80 diagnostic: complete CRTC-origin frames, passive bus/timing taps.
// Images and traces are implementation observations, not hardware expectations.
#define main p10_unused_main
#include "p10_boot_test.cpp"
#undef main
#include <fstream>
#include <sstream>

int main(int argc, char **argv) {
	try {
		Verilated::commandArgs(argc, argv);
		require(argc >= 4, "usage: probe image.cpr out_prefix frames [--overlap] [tap_frame...]");
		const unsigned frames = std::stoul(argv[3]);
		const bool overlap = argc > 4 && std::string(argv[4]) == "--overlap";
		std::vector<unsigned> taps;
		for (int i = overlap ? 5 : 4; i < argc; ++i) taps.push_back(std::stoul(argv[i]));
		Harness h;
		h.dut.d5_key = 0;
		h.dut.production_clocking = 1;
		h.dut.plus_model_i = 2;
		h.initialize();
		h.download(read_cpr_file(argv[1]));
		wait_for_cpr_apply(h);
		std::vector<uint16_t> cur(1024 * 400), last(1024 * 400);
		std::ostringstream ev_cur, ev_last, acquisition, navigation;
		std::ofstream overlap_events;
		if (overlap) overlap_events.open(std::string(argv[2]) + "-overlap-events.txt");
		unsigned done_frame = 0, old_state = 0xffff, old_vector = 255, old_dma_set = 0;
		unsigned frame = 0, origin = 0, line = 0, last_origin = 0;
		bool old_vs = false, old_fire = false, old_ack = false, old_hs = false;
		bool old_wr = false, old_int = true, old_split = false, old_latch = false, old_io_wr = false;
		bool old_c52 = false, old_classicn = true, old_progn = true, old_intcycle = true, old_insn = false;
		unsigned old_hcc = 0, old_test = 255;
		bool key_down = false;
		unsigned key_frame = 0;
		auto where = [&](auto &d) {
			std::ostringstream s;
			s << "tick=" << h.cycles << " origin=" << origin << " line=" << line
			  << " c0=" << unsigned(d.g_hcc) << " dot=" << unsigned(d.dbg_video_pixcnt)
			  << " vc=" << unsigned(d.g_vc) << " rc=" << unsigned(d.g_rc)
			  << " adj=" << unsigned(d.g_adj) << " intcnt=" << unsigned(d.g_intcnt)
			  << " mc=" << unsigned(d.dbg_mcycle) << " ts=" << unsigned(d.dbg_tstate)
			  << " intcyc=" << unsigned(d.g_intcycle);
			if (overlap) {
				auto ram = [&](unsigned a) { auto it=h.memory.find(0x20000U+a); return it==h.memory.end()?255U:unsigned(it->second); };
				s << " case=" << ram(0xbf02) << " rep=" << ram(0xbf03) << " mark=" << ram(0xbf30);
			}
			return s.str();
		};
		auto byte = [&](unsigned a) {
			auto it = h.memory.find(0x20000U + a);
			return it == h.memory.end() ? 255U : unsigned(it->second);
		};
		// Bound absence of VSYNC as well as ordinary screen failures.
		const uint64_t deadline = h.cycles + uint64_t(frames + 10) * 2000000;
		while (frame < frames && h.cycles < deadline) {
			auto &d = h.dut;
			const unsigned hcc = d.g_hcc;
			const bool line_start = hcc == 0 && old_hcc != 0;
			if (line_start) {
				++line;
				if (d.g_vc == 0 && d.g_rc == 0 && !d.g_adj) {
					last.swap(cur);
					std::fill(cur.begin(), cur.end(), 0);
					if (overlap) overlap_events << ev_cur.str();
					ev_last.str(ev_cur.str());
					ev_cur.str("");
					last_origin = origin++;
					line = 0;
				}
				ev_cur << "LINE " << where(d) << " ma=" << std::hex << d.dbg_video_ma
				       << " stored=" << d.g_store << " ssa=" << d.g_ssa << std::dec
				       << " splt=" << unsigned(d.g_splt) << '\n';
			}
			old_hcc = hcc;
			if (d.dbg_video_ce16) {
				const unsigned x = hcc * 16 + d.dbg_video_pixcnt;
				if (x < 1024 && line < 400) cur[line * 1024 + x] = d.dbg_video_rgb;
			}
			const bool wr = d.dbg_asic_wr;
			if (wr && !old_wr) {
				const unsigned a = 0x4000 | d.dbg_asic_addr;
				if ((a >= 0x6800 && a <= 0x6805) || (overlap && a >= 0x6c00 && a <= 0x6c0f) || a == 0x6400 || a == 0x6401 || a == 0x6421)
					ev_cur << "WR " << std::hex << a << '=' << unsigned(d.dbg_asic_val)
					       << std::dec << ' ' << where(d) << '\n';
			}
			old_wr = wr;
			const bool io_wr = !d.dbg_iorq_n && !d.dbg_wr_n;
			if (io_wr && !old_io_wr && (d.dbg_addr & 0xff00) == 0x7f00)
				ev_cur << "IO_WR " << std::hex << d.dbg_addr << '=' << unsigned(d.dbg_dout)
				       << std::dec << ' ' << where(d) << '\n';
			old_io_wr = io_wr;
			const bool hs = d.dbg_raw_hsync;
			if (hs != old_hs) ev_cur << (hs ? "HS_RISE " : "HS_FALL ") << where(d) << '\n';
			old_hs = hs;
			const bool fire = d.g_fire;
			if (fire && !old_fire) ev_cur << "FIRE " << where(d) << '\n';
			old_fire = fire;
			const bool irq = d.g_int_n;
			if (irq != old_int) ev_cur << (irq ? "INT_RELEASE " : "INT_ASSERT ") << where(d) << '\n';
			old_int = irq;
			if (overlap) {
				unsigned state = d.g_ack | (d.g_clear << 1) | (d.g_delivery << 2) | (d.g_dcsr << 3);
				if (state != old_state) ev_cur << "STATE " << where(d)
					<< " ack=" << unsigned(d.g_ack) << " clear=" << unsigned(d.g_clear)
					<< " m1n=" << unsigned(d.g_m1n) << " waitn=" << unsigned(d.dbg_cpu_waitn) << " delivery=" << unsigned(d.g_delivery)
					<< " dcsr=" << std::hex << unsigned(d.g_dcsr) << std::dec << '\n';
				old_state = state;
				if (d.g_ack && unsigned(d.g_vector) != old_vector)
					ev_cur << "VECTOR " << where(d) << " value=" << std::hex << unsigned(d.g_vector) << std::dec << '\n';
				old_vector = d.g_ack ? unsigned(d.g_vector) : 255;
				if (d.g_dma_set && !old_dma_set) ev_cur << "DMA_SET " << where(d) << " mask=" << unsigned(d.g_dma_set) << '\n';
				old_dma_set = d.g_dma_set;
			}
			const bool ack = d.dbg_int_ack;
			if (ack && !old_ack) ev_cur << "ACK " << where(d) << " pc=" << std::hex << d.dbg_pc << " addr=" << d.dbg_addr << std::dec << '\n';
			old_ack = ack;
			// PA7 cause chain, all passive edge taps: 52-counter event (C52),
			// latch attribution (CLASSIC/PROG), CPU sampling (INTSAMPLE =
			// intcycle_n fall) versus bus acknowledge (ACK above).
			const bool c52 = d.g_c52;
			if (c52 && !old_c52) ev_cur << "C52 " << where(d) << '\n';
			old_c52 = c52;
			const bool classicn = d.g_classicn;
			if (classicn != old_classicn)
				ev_cur << (classicn ? "CLASSIC_RELEASE " : "CLASSIC_ASSERT ") << where(d) << '\n';
			old_classicn = classicn;
			const bool progn = d.g_progn;
			if (progn != old_progn) ev_cur << (progn ? "PROG_RELEASE " : "PROG_ASSERT ") << where(d) << '\n';
			old_progn = progn;
			const bool intcycle = d.g_intcycle;
			if (intcycle != old_intcycle)
				ev_cur << (intcycle ? "INTSAMPLE_END " : "INTSAMPLE ") << where(d)
				       << " pc=" << std::hex << d.dbg_pc << " addr=" << d.dbg_addr << std::dec << '\n';
			old_intcycle = intcycle;
			// Bounded fetch trace: instruction starts only inside the PA7
			// reference/compatible windows in both follow-up passes, with
			// PC and master tick. Never a per-master-cycle log.
			const bool insn = d.g_insn;
			if (!overlap && old_test >= 30 && old_test <= 35 && insn && !old_insn &&
			    ((line >= 6 && line <= 10) || (line >= 66 && line <= 72) ||
			     (line >= 84 && line <= 88) || (line >= 144 && line <= 150)))
				ev_cur << "FETCH " << where(d) << " pc=" << std::hex << d.dbg_pc << " addr=" << d.dbg_addr << std::dec << '\n';
			if (overlap && insn && !old_insn && d.dbg_pc >= 0x2c00 && d.dbg_pc < 0x2e00)
				ev_cur << "FETCH " << where(d) << " pc=" << std::hex << d.dbg_pc << " addr=" << d.dbg_addr << std::dec << '\n';
			old_insn = insn;
			const bool split = d.g_split;
			if (split && !old_split) ev_cur << "SPLIT " << where(d) << " ssa=" << std::hex << d.g_ssa << std::dec << '\n';
			old_split = split;
			const bool latch = d.dbg_cpu_di_latch;
			if (latch && !old_latch && !d.dbg_rd_n) {
				const unsigned a = d.dbg_cpu_di_addr;
				if ((d.dbg_cpu_di_io_rd && (a & 0xff00) == 0x7f00) ||
				    (a == 0x5000 || a == 0x6800 || a == 0x6420 || a == 0x6421 || a == 0x4000 || a == 0x6404))
					acquisition << "CPU_SAMPLE addr=" << std::hex << a
					            << " data=" << unsigned(d.dbg_cpu_di_mb_bus)
					            << " cpu_di=" << unsigned(d.dbg_cpu_di_reg) << std::dec
					            << " io=" << unsigned(d.dbg_cpu_di_io_rd)
					            << " mapped=" << unsigned(d.dbg_asic_page_on) << ' ' << where(d) << '\n';
			}
			old_latch = latch;
			h.tick();
			h.bank0_writes.clear();
			const unsigned test = byte(0xbf00);
			if (test != old_test) {
				navigation << "TEST " << test << ' ' << where(d) << '\n';
				old_test = test;
			}
			const bool vs = d.dbg_raw_vsync;
			if (vs && !old_vs) {
				++frame;
				if (overlap && byte(0xbf01) == 0x80) {
					if (!done_frame) done_frame = frame;
					bool pending_key = key_down;
					for (unsigned t : taps) pending_key |= t >= frame;
					if (frame >= done_frame + 2 && !pending_key) break;
				} else if (overlap) {
					done_frame = 0;
				}
				for (unsigned t : taps)
					if (t == frame) { d.d5_key = 0x629; key_down = true; key_frame = frame; }
				if (key_down && frame >= key_frame + 3) { d.d5_key = 0x029; key_down = false; }
			}
			old_vs = vs;
		}
		std::ofstream img(std::string(argv[2]) + ".ppm", std::ios::binary);
		img << "P6\n1024 400\n255\n";
		for (auto v : last) {
			const char c[3] = {char((v >> 8 & 15) * 17), char((v >> 4 & 15) * 17), char((v & 15) * 17)};
			img.write(c, 3);
		}
		std::ofstream(std::string(argv[2]) + "-events.txt") << "Complete CRTC origin " << last_origin << '\n' << ev_last.str();
		std::ofstream(std::string(argv[2]) + "-acquisition.txt") << acquisition.str();
		std::ofstream(std::string(argv[2]) + "-navigation.txt") << navigation.str();
		std::ofstream results(std::string(argv[2]) + "-results.txt");
		results << "test=" << byte(0xbf00) << " origin=" << last_origin << " saved BF20-BF3F:" << std::hex;
		for (unsigned a = 0xbf20; a < 0xbf40; ++a) results << ' ' << std::setw(2) << std::setfill('0') << byte(a);
		results << '\n';
		if (overlap) {
			overlap_events << ev_cur.str();
			std::ofstream table(std::string(argv[2]) + "-overlap-results.json");
			table << "{\"page\":" << byte(0xbf00) << ",\"status\":" << byte(0xbf01) << ",\"records\":[";
			for (unsigned row=0; row<17; ++row) {
				if (row) table << ',';
				table << '[';
				for (unsigned col=0; col<16; ++col) { if (col) table << ','; table << byte(0xb000+row*16+col); }
				table << ']';
			}
			table << "]}\n";
		}
		require(origin >= 2 && (overlap ? byte(0xbf01) == 0x80 && done_frame && frame >= done_frame + 2 : frame == frames),
		        "missing settled frame/VSYNC or completed overlap page");
		std::cout << "DONE frames=" << frame << " origin=" << last_origin << " test=" << byte(0xbf00)
		          << " pc=" << std::hex << unsigned(h.dut.dbg_pc) << std::endl;
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
