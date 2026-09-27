// Production-model run of the Plus hardware-probe cartridge: the prepared D5 fixture
// (production T80, motherboard, ASIC, SDRAM cartridge path, production clocking).
// Dumps the last complete frame as a PPM in C0 geometry (x = C0*16 + dot,
// y = scanline since frame start) and logs PRI/SPLT/SSA/SSCR/pen-0 writes, raster
// requests and interrupt acknowledges of that frame. Optional key taps (space)
// exercise screen navigation. Diagnostic only; built by plus_hw_probes_sim.py.
#define main p10_unused_main
#include "p10_boot_test.cpp"
#undef main
#include <fstream>
#include <sstream>

int main(int argc, char **argv) {
	try {
		Verilated::commandArgs(argc, argv);
		require(argc >= 4, "usage: probe image.cpr out_prefix frames [tap_frame...]");
		const unsigned frames = std::stoul(argv[3]);
		std::vector<unsigned> taps;
		for (int i = 4; i < argc; ++i) taps.push_back(std::stoul(argv[i]));
		Harness h;
		h.dut.d5_key = 0;
		h.dut.production_clocking = 1;
		h.dut.plus_model_i = 2;
		h.initialize();
		h.download(read_cpr_file(argv[1]));
		wait_for_cpr_apply(h);

		std::vector<uint16_t> cur(1024 * 400), last(1024 * 400);
		std::ostringstream ev_cur, ev_last;
		unsigned frame = 0, line = 0;
		bool old_vs = false, old_fire = false, old_ack = false, old_hs = false, old_wr = false;
		unsigned old_hcc = 0;
		bool key_down = false;
		unsigned key_frame = 0;
		auto where = [&](auto &d) {
			std::ostringstream s;
			s << "line=" << line << " c0=" << unsigned(d.g_hcc) << " dot=" << unsigned(d.dbg_video_pixcnt);
			return s.str();
		};
		while (frame < frames) {
			auto &d = h.dut;
			const unsigned hcc = d.g_hcc;
			if (hcc == 0 && old_hcc != 0) ++line;
			if (hcc == 0 && d.g_vc == 0 && d.g_rc == 0) line = 0;
			old_hcc = hcc;
			if (d.dbg_video_ce16) {
				const unsigned x = hcc * 16 + d.dbg_video_pixcnt;
				if (x < 1024 && line < 400) cur[line * 1024 + x] = d.dbg_video_rgb;
			}
			const bool wr = d.dbg_asic_wr;
			if (wr && !old_wr) {
				const unsigned a = 0x4000 | d.dbg_asic_addr;
				if (a == 0x6800 || a == 0x6801 || a == 0x6802 || a == 0x6803 || a == 0x6804 || a == 0x6401)
					ev_cur << "WR " << std::hex << a << "=" << unsigned(d.dbg_asic_val) << std::dec << " " << where(d) << '\n';
			}
			old_wr = wr;
			const bool hs = d.dbg_raw_hsync;
			if (hs && !old_hs) ev_cur << "HSYNC " << where(d) << '\n';
			old_hs = hs;
			const bool fire = d.g_fire;
			if (fire && !old_fire) ev_cur << "FIRE " << where(d) << '\n';
			old_fire = fire;
			const bool ack = d.dbg_int_ack;
			if (ack && !old_ack) ev_cur << "ACK " << where(d) << '\n';
			old_ack = ack;
			h.tick();
			h.bank0_writes.clear();
			const bool vs = d.dbg_raw_vsync;
			if (vs && !old_vs) {
				++frame;
				last.swap(cur);
				std::fill(cur.begin(), cur.end(), 0);
				ev_last.str(ev_cur.str());
				ev_cur.str("");
				for (unsigned t : taps)
					if (t == frame) { h.dut.d5_key = 0x629; key_down = true; key_frame = frame; }
				if (key_down && frame >= key_frame + 3) { h.dut.d5_key = 0x029; key_down = false; }
			}
			old_vs = vs;
		}
		std::ofstream img(std::string(argv[2]) + ".ppm", std::ios::binary);
		img << "P6\n1024 400\n255\n";
		for (auto v : last) {
			const char c[3] = {char((v >> 8 & 15) * 17), char((v >> 4 & 15) * 17), char((v & 15) * 17)};
			img.write(c, 3);
		}
		std::ofstream(std::string(argv[2]) + "-events.txt") << ev_last.str();
		std::cout << "DONE frames=" << frame << " pc=" << std::hex << unsigned(h.dut.dbg_pc) << std::endl;
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
