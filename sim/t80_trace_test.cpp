// Cycle-by-cycle equivalence verification: native VHDL simulation vs
// GHDL-synthesized Verilog netlist for production T80pa executing
// OUT(C),r and OUTI.
#include "VT80pa.h"
#include "verilated.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static const uint8_t ROM[36] = {
    0x00,                            // 0: NOP
    0x01, 0x05, 0xBD,                // 1-3: LD BC, 0xBD05
    0xED, 0x49,                      // 4-5: OUT (C), C
    0x01, 0x43, 0xBE,                // 6-8: LD BC, 0xBE43
    0x21, 0x20, 0x00,                // 9-11: LD HL, 0x0020
    0xED, 0xA3,                      // 12-13: OUTI
    0x76,                            // 14: HALT
    0x00, 0x00, 0x00, 0x00,          // 15-18
    0x00, 0x00, 0x00, 0x00,          // 19-22
    0x00, 0x00, 0x00, 0x00,          // 23-26
    0x00, 0x00, 0x00, 0x00,          // 27-30
    0x00,                            // 31
    0x42,                            // 32 (0x0020): data for OUTI
    0x00, 0x00, 0x00                 // 33-35
};

std::string hex4(uint16_t val) {
  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (val & 0xFFFF);
  return oss.str();
}

std::string hex2(uint8_t val) {
  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (val & 0xFF);
  return oss.str();
}

int main(int argc, char **argv) {
  std::string vhdl_log_path = "obj_dir/t80/vhdl_trace.log";
  if (argc > 1) vhdl_log_path = argv[1];

  std::ifstream vhdl_file(vhdl_log_path);
  if (!vhdl_file.is_open()) {
    std::cerr << "Cannot open VHDL trace file: " << vhdl_log_path << "\n";
    return 1;
  }

  std::vector<std::string> vhdl_lines;
  std::string line;
  while (std::getline(vhdl_file, line)) {
    if (line.rfind("CYC=", 0) == 0) {
      vhdl_lines.push_back(line);
    }
  }

  if (vhdl_lines.empty()) {
    std::cerr << "Error: No CYC= trace lines found in " << vhdl_log_path << "\n";
    return 1;
  }

  VT80pa dut;
  dut.RESET_n = 0;
  dut.CLK = 0;
  dut.CEN_p = 1;
  dut.CEN_n = 1;
  dut.WAIT_n = 1;
  dut.INT_n = 1;
  dut.NMI_n = 1;
  dut.BUSRQ_n = 1;
  dut.OUT0 = 0;
  dut.R800_mode = 0;
  dut.DIRSet = 0;
  for (int i = 0; i < 7; i++) dut.DIR[i] = 0;

  auto eval_bus = [&]() {
    uint16_t addr = dut.A;
    if (!dut.MREQ_n && !dut.RD_n && addr < sizeof(ROM)) {
      dut.DI = ROM[addr];
    } else {
      dut.DI = 0xFF;
    }
  };

  auto tick = [&]() {
    dut.CLK = 0;
    eval_bus();
    dut.eval();
    dut.CLK = 1;
    dut.eval();
  };

  unsigned compared = 0;
  unsigned mismatches = 0;
  bool saw_out = false;
  bool saw_outi = false;
  bool saw_halt = false;
  unsigned halt_cycle = 0;

  dut.RESET_n = 0;

  for (unsigned cycle = 1; cycle <= vhdl_lines.size(); cycle++) {
    if (cycle > 4) {
      dut.RESET_n = 1;
    }
    tick();

    std::string vhdl_line = vhdl_lines[cycle - 1]; // 1-to-1 mapping: cycle 1 -> vhdl_lines[0]

    // Parse VHDL signals
    // Format: CYC=<n> A=<h4> DO=<h2> M1=<b> MREQ=<b> IORQ=<b> RD=<b> WR=<b> HALT=<b>
    auto get_token = [&](const std::string &prefix) -> std::string {
      size_t p = vhdl_line.find(prefix);
      if (p == std::string::npos) return "";
      size_t end = vhdl_line.find(' ', p);
      if (end == std::string::npos) end = vhdl_line.length();
      return vhdl_line.substr(p + prefix.length(), end - (p + prefix.length()));
    };

    std::string vhdl_cyc_str = get_token("CYC=");
    if (vhdl_cyc_str.empty()) {
      std::cerr << "FAIL: Missing CYC= label at line " << cycle << ": " << vhdl_line << "\n";
      return 1;
    }
    unsigned vhdl_cyc = std::stoul(vhdl_cyc_str);
    if (vhdl_cyc != cycle) {
      std::cerr << "FAIL: Non-consecutive CYC label: expected " << cycle << ", got " << vhdl_cyc << " in line: " << vhdl_line << "\n";
      return 1;
    }

    std::string vhdl_a = get_token("A=");
    std::string vhdl_do = get_token("DO=");
    std::string vhdl_m1 = get_token("M1=");
    std::string vhdl_mreq = get_token("MREQ=");
    std::string vhdl_iorq = get_token("IORQ=");
    std::string vhdl_rd = get_token("RD=");
    std::string vhdl_wr = get_token("WR=");
    std::string vhdl_halt = get_token("HALT=");

    // Fail immediately if defined controls or address contain VHDL unknowns
    if (vhdl_m1.find('X') != std::string::npos ||
        vhdl_mreq.find('X') != std::string::npos ||
        vhdl_rd.find('X') != std::string::npos ||
        vhdl_wr.find('X') != std::string::npos ||
        vhdl_halt.find('X') != std::string::npos) {
      std::cerr << "FAIL: VHDL unknown on defined control at cycle " << cycle << ": " << vhdl_line << "\n";
      return 1;
    }
    if (vhdl_a.find('X') != std::string::npos) {
      std::cerr << "FAIL: VHDL unknown on address bus at cycle " << cycle << ": " << vhdl_line << "\n";
      return 1;
    }
    // IORQ_n has a known 3-cycle power-up unknown on CYC=6..8 in native VHDL T80pa due to uninitialized
    // IntCycleD_n before the first M1 T3 reload ("11"). Outside cycles 6..8, IORQ_n must NEVER be unknown.
    if ((cycle < 6 || cycle > 8) && vhdl_iorq.find('X') != std::string::npos) {
      std::cerr << "FAIL: VHDL unknown on IORQ_n at cycle " << cycle << ": " << vhdl_line << "\n";
      return 1;
    }
    // During an active write (WR_n=0), DO must never contain unknowns
    if (dut.WR_n == 0 && vhdl_do.find('X') != std::string::npos) {
      std::cerr << "FAIL: VHDL unknown on data output during active write at cycle " << cycle << ": " << vhdl_line << "\n";
      return 1;
    }

    // Check equivalence vs Verilog
    bool mismatch = false;
    if (hex4(dut.A) != vhdl_a) mismatch = true;
    if (std::to_string(int(dut.M1_n)) != vhdl_m1) mismatch = true;
    if (std::to_string(int(dut.MREQ_n)) != vhdl_mreq) mismatch = true;
    if (std::to_string(int(dut.RD_n)) != vhdl_rd) mismatch = true;
    if (std::to_string(int(dut.WR_n)) != vhdl_wr) mismatch = true;
    if (std::to_string(int(dut.HALT_n)) != vhdl_halt) mismatch = true;

    // Compare IORQ_n: compare every defined value. Only mask when vhdl_iorq is specifically
    // unknown ('X') during cycles 6..8 due to uninitialized IntCycleD_n.
    // If vhdl_iorq is defined (even during cycles 6..8), require exact match vs Verilog.
    if (vhdl_iorq.find('X') == std::string::npos) {
      if (std::to_string(int(dut.IORQ_n)) != vhdl_iorq) mismatch = true;
    }

    // Compare DO: when WR_n=0 (active write), DO must match exactly.
    // When WR_n=1 (outside write), if VHDL DO is legitimately uninitialized ("XX"),
    // mask invalid data; if VHDL has defined DO, require exact match.
    if (dut.WR_n == 0) {
      if (hex2(dut.DO) != vhdl_do) mismatch = true;
    } else if (vhdl_do.find('X') == std::string::npos) {
      if (hex2(dut.DO) != vhdl_do) mismatch = true;
    }

    if (mismatch) {
      std::cerr << "MISMATCH at cycle " << cycle << ":\n"
                << "  VHDL:    " << vhdl_line << "\n"
                << "  VERILOG: A=" << hex4(dut.A) << " DO=" << hex2(dut.DO)
                << " M1=" << int(dut.M1_n) << " MREQ=" << int(dut.MREQ_n)
                << " IORQ=" << int(dut.IORQ_n) << " RD=" << int(dut.RD_n)
                << " WR=" << int(dut.WR_n) << " HALT=" << int(dut.HALT_n) << "\n";
      ++mismatches;
      if (mismatches > 5) break;
    }
    ++compared;

    // Pin OUT(C),C execution (port 0xBD05, data 0x05)
    if (!dut.IORQ_n && !dut.WR_n && dut.A == 0xBD05 && dut.DO == 0x05) {
      saw_out = true;
    }
    // Pin OUTI execution (B decremented 0xBE -> 0xBD, port 0xBD43, data 0x42)
    if (!dut.IORQ_n && !dut.WR_n && dut.A == 0xBD43 && dut.DO == 0x42) {
      saw_outi = true;
    }

    if (!dut.HALT_n) {
      if (!saw_halt) {
        saw_halt = true;
        halt_cycle = cycle;
      }
    }
  }

  if (mismatches > 0) {
    std::cerr << "FAIL: " << mismatches << " mismatches in " << compared << " cycles\n";
    return 1;
  }
  if (!saw_out) {
    std::cerr << "FAIL: OUT (C), C bus strobe not observed\n";
    return 1;
  }
  if (!saw_outi) {
    std::cerr << "FAIL: OUTI bus strobe not observed\n";
    return 1;
  }
  if (!saw_halt) {
    std::cerr << "FAIL: HALT instruction was never reached (trace truncated, compared " << compared << " cycles)\n";
    return 1;
  }
  if (dut.HALT_n != 0) {
    std::cerr << "FAIL: Trace completed but DUT is not in HALT state (cycle " << compared << ")\n";
    return 1;
  }
  if (compared < halt_cycle + 2) {
    std::cerr << "FAIL: Trace truncated during HALT state; expected at least 2 post-HALT cycles (saw "
              << (compared - halt_cycle) << ")\n";
    return 1;
  }
  if (compared < 139) {
    std::cerr << "FAIL: Trace truncated; expected at least 139 cycles, got " << compared << "\n";
    return 1;
  }

  std::cout << "PASS T80 native VHDL vs translated Verilog trace equivalence ("
            << compared << " cycles verified, bounded defined-field trace)\n"
            << "  Confirmed reset startup: cycles 1..4 verified\n"
            << "  Confirmed defined IORQ_n and startup IntCycleD_n X-mask (cycles 6..8)\n"
            << "  Confirmed OUT(C),C strobe: A=0xBD05 DO=0x05 IORQ_n=0 WR_n=0\n"
            << "  Confirmed OUTI strobe:    A=0xBD43 DO=0x42 IORQ_n=0 WR_n=0 (B decremented & memory read)\n"
            << "  Confirmed HALT reached: cycle " << halt_cycle << ", full trace completed at cycle " << compared << "\n";
  return 0;
}
