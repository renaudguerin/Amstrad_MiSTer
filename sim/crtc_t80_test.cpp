// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND).
// Bounded production-T80 executed-instruction validation harness for B8-1.
#include "Vcrtc_t80_top.h"
#include "verilated.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Bench {
  Vcrtc_t80_top d;
  unsigned ticks = 0;

  void eval() {
    d.clk = 0;
    d.eval();
  }

  void tick() {
    eval();
    d.clk = 1;
    d.eval();
    ++ticks;
    eval();
  }

  void run(unsigned n) {
    while (n--) tick();
  }

  template <class F> void until(F f) {
    for (unsigned n = 0; n < 2000000; n++) {
      eval();
      if (f()) return;
      tick();
    }
    throw std::runtime_error("timeout waiting for condition");
  }

  void check(bool ok, const char *why) {
    if (!ok) throw std::runtime_error(why);
  }

  void prog_byte(uint8_t addr, uint8_t data) {
    d.prog_we = 1;
    d.prog_addr = addr;
    d.prog_data = data;
    tick();
    d.prog_we = 0;
    tick();
  }

  void load_prog(uint8_t start_addr, const std::vector<uint8_t> &bytes) {
    for (size_t i = 0; i < bytes.size(); i++) {
      prog_byte(start_addr + i, bytes[i]);
    }
  }

  Bench(bool type = 1, unsigned reg = 5, unsigned r0 = 15, unsigned r6 = 3) {
    d.reset = 1;
    d.cpu_reset = 1;
    d.sna_load = 1;
    d.crtc_type = type;
    d.sna_addr = reg;
    d.prog_we = 0;

    for (int i = 0; i < 5; i++) d.sna_regs[i] = 0;
    const unsigned char r[18] = {
      static_cast<unsigned char>(r0),
      8, 10, 0x22, 3, 0,
      static_cast<unsigned char>(r6),
      2, 0, 3, 0, 0, 0x12, 0x34, 0, 0, 0, 0
    };
    for (unsigned i = 0; i < 18; i++)
      d.sna_regs[i / 4] |= unsigned(r[i]) << (8 * (i % 4));

    run(128);
    d.sna_load = 0;
    d.reset = 0;

    // Run until clean frame start
    until([&] { return d.row == 2; });
    until([&] { return d.row == 0; });
  }

  void pos(unsigned row, unsigned line, unsigned c0) {
    until([&] { return d.row == row && d.line == line && d.c0 == c0; });
  }

  unsigned seam() {
    unsigned margin = 0;
    while (!d.cclk_n) {
      tick();
      ++margin;
    }
    tick();
    return margin;
  }
};

// Case 1: Type 1 OUT (C), C for R5 RFD (French ACCC v1.11 §11.6 pp.89-92 and §13.7.1 p.126)
void test_out_r5_rfd() {
  Bench b(true, 5, 15, 3);
  // Program:
  // 0: LD BC, 0xBC05 (3 bytes: 01 05 BC) - select R5
  // 3: OUT (C), C    (2 bytes: ED 49)
  // 5: LD BC, 0xBD01 (3 bytes: 01 01 BD) - write 1 to R5
  // 8: OUT (C), C    (2 bytes: ED 49)    - critical data write
  // 10: HALT         (1 byte: 76)
  b.load_prog(0, {0x01, 0x05, 0xBC, 0xED, 0x49, 0x01, 0x01, 0xBD, 0xED, 0x49, 0x76});

  b.pos(1, 0, 2);
  unsigned start_tick = b.ticks;
  b.d.cpu_reset = 0;

  // Run through first write (select R5)
  while (!b.d.write_active && b.ticks - start_tick < 2000) b.tick();
  b.check(b.d.write_active, "selection write did not start");
  b.check(b.d.t80_a == 0xBC05, "selection write address mismatch");
  while (b.d.write_active) b.tick();
  b.check(b.d.crtc_addr == 5, "crtc_addr not set to 5 after selection write");

  // Track opcode fetch of the critical OUT (C), C instruction at PC = 8.
  // Reference: French ACCC v1.11 §13.7.1 p.126 ("OUT(C),reg8 (I/O sur la 3 ème µsec)").
  // The 3rd microsecond window corresponds to [128, 192) master ticks from the first
  // opcode fetch start (!t80_m1_n && !t80_mreq_n && !t80_rd_n && t80_a == 8).
  // The measured interval (observed 152 ticks = 2.375 µs) is an observation from simulation.
  unsigned fetch_tick = 0;
  while (!b.d.write_active && b.ticks - start_tick < 4000) {
    if (!b.d.t80_m1_n && !b.d.t80_mreq_n && !b.d.t80_rd_n && b.d.t80_a == 8 && fetch_tick == 0) {
      fetch_tick = b.ticks;
    }
    b.tick();
  }
  b.check(b.d.write_active, "critical data write did not start");
  b.check(fetch_tick != 0, "OUT (C), C opcode fetch at PC=8 not detected");
  b.check(b.d.t80_a == 0xBD01, "critical write address mismatch");
  b.check(b.d.t80_dout == 1, "critical write data mismatch");
  b.check(b.d.crtc_addr == 5, "critical write target reg mismatch");
  b.check(b.d.row == 1 && b.d.line == 0 && b.d.c0 == 15, "write not landed at row=1 line=0 c0=15");

  unsigned out_interval = b.ticks - fetch_tick;
  b.check(out_interval >= 128 && out_interval < 192, "OUT (C), C write not in 3rd microsecond [128, 192) per FR ACCC §13.7.1 p.126");

  // Assert first system-edge storage capture before next CLKEN decision
  b.check(b.d.r5 == 0, "r5 must hold old value 0 before first system edge");
  b.tick();
  b.check(b.d.r5 == 1, "r5 must capture new value 1 on first system clock edge");

  unsigned margin = b.seam();
  b.check(margin > 0, "positive margin required from first store to next CLKEN decision");
  std::cout << "[obs: interval=" << out_interval << "t, margin=" << margin << "t] " << std::flush;

  b.check(b.d.vma_flag == 1, "vma_flag must be armed on current engine");
  b.check(b.d.parity_flag == 1, "parity_flag must be armed on current engine");
  b.check(b.d.ma == 0x1234, "ma must be reloaded to 0x1234 on current engine");
}

// Case 2: Type 1 OUTI for R5 RFD (French ACCC v1.11 §11.6 pp.89-92 and §13.7.1 p.126)
void test_outi_r5_rfd() {
  Bench b(true, 5, 15, 3);
  // Program:
  // 0: LD BC, 0xBC05 (3 bytes: 01 05 BC) - select R5
  // 3: OUT (C), C    (2 bytes: ED 49)
  // 5: LD BC, 0xBE02 (3 bytes: 01 02 BE) - B=0xBE, C=0x02
  // 8: LD HL, 0x0040 (3 bytes: 21 40 00) - HL points to data
  // 11: OUTI         (2 bytes: ED A3)    - decrements B to 0xBD, writes (HL) to port 0xBD02
  // 13: HALT         (1 byte: 76)
  // RAM[0x40] = 0x01
  b.load_prog(0, {0x01, 0x05, 0xBC, 0xED, 0x49, 0x01, 0x02, 0xBE, 0x21, 0x40, 0x00, 0xED, 0xA3, 0x76});
  b.prog_byte(0x40, 0x01);

  b.pos(0, 3, 14);
  unsigned start_tick = b.ticks;
  b.d.cpu_reset = 0;

  // Run through first write (select R5)
  while (!b.d.write_active && b.ticks - start_tick < 2000) b.tick();
  while (b.d.write_active) b.tick();
  b.check(b.d.crtc_addr == 5, "crtc_addr not set to 5 after selection write");

  // Track opcode fetch of the critical OUTI instruction at PC = 11.
  // Reference: French ACCC v1.11 §13.7.1 p.126 ("OUTI (I/O sur la 5 ème µsec)").
  // The 5th microsecond window corresponds to [256, 320) master ticks from the first
  // opcode fetch start (!t80_m1_n && !t80_mreq_n && !t80_rd_n && t80_a == 11).
  // The measured interval (observed 264 ticks = 4.125 µs) is an observation from simulation.
  unsigned fetch_tick = 0;
  while (!b.d.write_active && b.ticks - start_tick < 4000) {
    if (!b.d.t80_m1_n && !b.d.t80_mreq_n && !b.d.t80_rd_n && b.d.t80_a == 11 && fetch_tick == 0) {
      fetch_tick = b.ticks;
    }
    b.tick();
  }
  b.check(b.d.write_active, "OUTI data write did not start");
  b.check(fetch_tick != 0, "OUTI opcode fetch at PC=11 not detected");
  b.check(b.d.t80_a == 0xBD02, "OUTI write address mismatch (B decremented to 0xBD, port 0xBD02)");
  b.check(b.d.t80_dout == 1, "OUTI write data mismatch (RAM[0x40])");
  b.check(b.d.crtc_addr == 5, "OUTI write target reg mismatch");
  b.check(b.d.row == 1 && b.d.line == 0 && b.d.c0 == 15, "OUTI write not landed at row=1 line=0 c0=15");

  unsigned outi_interval = b.ticks - fetch_tick;
  b.check(outi_interval >= 256 && outi_interval < 320, "OUTI write not in 5th microsecond [256, 320) per FR ACCC §13.7.1 p.126");

  // Assert first system-edge storage capture before next CLKEN decision
  b.check(b.d.r5 == 0, "r5 must hold old value 0 before first system edge");
  b.tick();
  b.check(b.d.r5 == 1, "r5 must capture new value 1 on first system clock edge");

  unsigned margin = b.seam();
  b.check(margin > 0, "positive margin required from first store to next CLKEN decision");
  std::cout << "[obs: interval=" << outi_interval << "t, margin=" << margin << "t] " << std::flush;

  b.check(b.d.vma_flag == 1, "vma_flag must be armed on current engine");
  b.check(b.d.parity_flag == 1, "parity_flag must be armed on current engine");
  b.check(b.d.ma == 0x1234, "ma must be reloaded to 0x1234 on current engine");
}

// Case 3: Type 0 OUT (C), C for R0 widening (French ACCC v1.11 §13.7.2 pp.126-128)
void test_out_r0_type0_widen() {
  Bench b(false, 0, 1, 3);
  // Program:
  // 0: LD BC, 0xBD0F (3 bytes: 01 0F BD) - write 15 to R0
  // 3: OUT (C), C    (2 bytes: ED 49)
  // 5: HALT          (1 byte: 76)
  b.load_prog(0, {0x01, 0x0F, 0xBD, 0xED, 0x49, 0x76});

  b.pos(3, 0, 1);
  unsigned start_tick = b.ticks;
  b.d.cpu_reset = 0;

  unsigned fetch_tick = 0;
  while (!b.d.write_active && b.ticks - start_tick < 2000) {
    if (!b.d.t80_m1_n && !b.d.t80_mreq_n && !b.d.t80_rd_n && b.d.t80_a == 3 && fetch_tick == 0) {
      fetch_tick = b.ticks;
    }
    b.tick();
  }
  b.check(b.d.write_active, "Type 0 write did not start");
  b.check(fetch_tick != 0, "Type 0 OUT (C), C opcode fetch at PC=3 not detected");
  b.check(b.d.t80_a == 0xBD0F, "Type 0 write address mismatch");
  b.check(b.d.t80_dout == 0x0F, "Type 0 write data mismatch");
  b.check(b.d.crtc_addr == 0, "Type 0 write target reg mismatch");
  b.check(b.d.row == 3 && b.d.line == 3 && b.d.c0 == 1, "Type 0 write not landed at row=3 line=3 c0=1");

  unsigned out_interval = b.ticks - fetch_tick;
  b.check(out_interval >= 128 && out_interval < 192, "Type 0 write not in 3rd microsecond [128, 192) per FR ACCC §13.7.1 p.126");

  // Assert first system-edge storage capture before next CLKEN decision
  b.check(b.d.r0 == 1, "r0 must hold old value 1 before first system edge");
  b.tick();
  b.check(b.d.r0 == 15, "r0 must capture new value 15 on first system clock edge");

  unsigned margin = b.seam();
  b.check(margin > 0, "positive margin required from first store to next CLKEN decision");
  std::cout << "[obs: interval=" << out_interval << "t, margin=" << margin << "t] " << std::flush;

  b.check(b.d.row == 4, "row must advance to 4 (R4+1) on current engine");
  b.check(b.d.line == 3, "line must retain C9=3 on current engine");
  b.check(b.d.c0 == 2, "c0 must advance to 2 without duplicating c0=1");
  b.check(b.d.in_adj == 1, "in_adj must be 1 on current engine");
}

// Case 4: Type 1 OUT (C), C for R0 widening (French ACCC v1.11 §13.6.2 p.124 and §13.7.1.2 p.126)
void test_out_r0_type1_widen() {
  Bench b(true, 0, 15, 3);
  // Program:
  // 0: LD BC, 0xBD3F (3 bytes: 01 3F BD) - write 63 to R0
  // 3: OUT (C), C    (2 bytes: ED 49)
  // 5: HALT          (1 byte: 76)
  b.load_prog(0, {0x01, 0x3F, 0xBD, 0xED, 0x49, 0x76});

  b.pos(3, 3, 9);
  unsigned start_tick = b.ticks;
  b.d.cpu_reset = 0;

  unsigned fetch_tick = 0;
  while (!b.d.write_active && b.ticks - start_tick < 2000) {
    if (!b.d.t80_m1_n && !b.d.t80_mreq_n && !b.d.t80_rd_n && b.d.t80_a == 3 && fetch_tick == 0) {
      fetch_tick = b.ticks;
    }
    b.tick();
  }
  b.check(b.d.write_active, "Type 1 R0 write did not start");
  b.check(fetch_tick != 0, "Type 1 OUT (C), C opcode fetch at PC=3 not detected");
  b.check(b.d.t80_a == 0xBD3F, "Type 1 R0 write address mismatch");
  b.check(b.d.t80_dout == 0x3F, "Type 1 R0 write data mismatch");
  b.check(b.d.crtc_addr == 0, "Type 1 R0 write target reg mismatch");
  b.check(b.d.row == 3 && b.d.line == 3 && b.d.c0 == 15, "Type 1 R0 write not landed at row=3 line=3 c0=15");

  unsigned out_interval = b.ticks - fetch_tick;
  b.check(out_interval >= 128 && out_interval < 192, "Type 1 write not in 3rd microsecond [128, 192) per FR ACCC §13.7.1 p.126");

  // Assert first system-edge storage capture before next CLKEN decision
  b.check(b.d.r0 == 15, "r0 must hold old value 15 before first system edge");
  b.tick();
  b.check(b.d.r0 == 63, "r0 must capture new value 63 on first system clock edge");

  unsigned margin = b.seam();
  b.check(margin > 0, "positive margin required from first store to next CLKEN decision");
  std::cout << "[obs: interval=" << out_interval << "t, margin=" << margin << "t] " << std::flush;

  b.check(b.d.row == 3, "row must remain 3 on current engine");
  b.check(b.d.line == 3, "line must remain 3 on current engine");
  b.check(b.d.c0 == 16, "c0 must continue past 15 to 16 on current engine");
  b.check(b.d.pending == 1, "pending must be 1 on current engine");
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  std::cout << "=== Production-T80 B8 Executed-Instruction Validation ===\n";

  unsigned passes = 0;
  unsigned failures = 0;

  auto run_case = [&](const std::string &name, auto func) {
    std::cout << "[TEST] " << name << "... " << std::flush;
    try {
      func();
      std::cout << "PASS\n";
      passes++;
    } catch (const std::exception &e) {
      std::cout << "FAIL: " << e.what() << "\n";
      failures++;
    }
  };

  run_case("Type 1 OUT (C), C for R5 RFD", test_out_r5_rfd);
  run_case("Type 1 OUTI for R5 RFD", test_outi_r5_rfd);
  run_case("Type 0 OUT (C), C for R0 widening", test_out_r0_type0_widen);
  run_case("Type 1 OUT (C), C for R0 widening", test_out_r0_type1_widen);

  std::cout << "Summary: " << passes << " passed, " << failures << " failed (total 4)\n";
  return (failures == 0) ? 0 : 1;
}
