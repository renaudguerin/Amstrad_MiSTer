// Classic CPC IRQ boundary regression: actual CRTC0/1 -> GA40010 -> T80pa.
// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND), French v1.11 §27.7.2 pp289-290, and Zilog
// UM008011-0816 Figure 9 p13 / HALT Figure 11 p15.
//
// Calibration masks only the SECOND request at the CPU pin. The real GA
// still generates it. The uninterrupted sled supplies pin-level instruction
// boundaries E; Zilog requires sampling at E-16. Thus the first permissible
// acknowledge boundary is the first E with E-16 AFTER the observed IRQ fall.
// No expected absolute timestamp or PC is copied from a candidate run.
// The held level spans the chosen sampling edge with nonzero setup time.
//
// The first HALT/interrupt aligns the sled. NOP and HALT are the robust ACCC
// controls; one NOP of padding places a repeated untaken RET NC in each of
// its two instruction phases. Exactly one RET phase must reject the first
// boundary after the request. This pins a shared CPU/GA/CRTC interaction.

#include "Vclassic_irq_phase_top.h"
#include "verilated.h"
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <utility>

struct IrqRunResult {
  int64_t hs_fall = -1, irq_fall = -1, entry = -1, ack = -1;
};

class ClassicIrqBench {
  Vclassic_irq_phase_top d;
  uint64_t t = 0;
  bool old_hs = false, old_irq = true, old_ack = false, old_m1 = true;
  bool seen_refresh = false, calibration;
  int ack_count = 0;
  int64_t last_hs_fall = -1, last_m1 = -1;
public:
  IrqRunResult res;
  std::vector<std::pair<int64_t, uint16_t>> fetch;

  void tick() {
    d.mask_cpu_irq = calibration && ack_count >= 1;
    d.clk = 0; d.eval();
    d.clk = 1; d.eval();
    const bool hs = d.crtc_hs_o, irq = d.ga_irq_o;
    const bool ack = !d.t80_m1_n && !d.t80_iorq_n && d.t80_mreq_n;
    if (!d.t80_rfsh_n) seen_refresh = true;
    if (old_hs && !hs) last_hs_fall = t;
    if (old_m1 && !d.t80_m1_n && !d.cpu_reset) {
      last_m1 = t;
      fetch.emplace_back(t, d.t80_a);
    }
    if (ack_count == 1 && old_irq && !irq && res.irq_fall < 0) {
      res.irq_fall = t;
      res.hs_fall = last_hs_fall;
    }
    if (seen_refresh && !old_ack && ack) {
      if (++ack_count == 2) { res.ack = t; res.entry = last_m1; }
    }
    old_hs = hs; old_irq = irq; old_ack = ack; old_m1 = d.t80_m1_n;
    ++t;
  }
  void run(uint64_t n) {
    while (n--) tick();
  }

  void put(uint16_t addr, uint8_t val) {
    d.prog_we = 1;
    d.prog_addr = addr;
    d.prog_data = val;
    tick();
    d.prog_we = 0;
    tick();
  }

  ClassicIrqBench(int crtc_type, uint8_t op, int pad_nops = 0, bool calibrate = false) : calibration(calibrate) {
    d.reset = 1;
    d.cpu_reset = 1;
    d.crtc_type = crtc_type;
    d.mask_cpu_irq = 0;
    d.sna_addr = 0;
    d.sna_load = 1;
    d.prog_we = 0;
    d.prog_addr = 0;
    d.prog_data = 0;
    d.peek_addr = 0;

    for (int i = 0; i < 5; i++) d.sna_regs[i] = 0;
    // Standard CPC CRTC configuration: R0=63, R1=40, R2=46, R3=0x8E (HSYNC=14), R4=38, R6=25, R7=30, R9=7
    const unsigned char r[18] = {
      63, 40, 46, 0x8E, 38, 0, 25, 30, 0, 7,
      0, 0, 0x30, 0x00, 0, 0, 0, 0
    };
    for (unsigned i = 0; i < 18; i++) {
      d.sna_regs[i / 4] |= (uint32_t(r[i]) << (8 * (i % 4)));
    }

    run(128);
    d.sna_load = 0;
    d.reset = 0;

    // Boot program at 0x0000:
    // DI (F3), LD SP, 0x00F0 (31 F0 00), IM 1 (ED 56), SCF (37), EI (FB), HALT (76)
    std::vector<uint8_t> boot = {0xF3, 0x31, 0xF0, 0x00, 0xED, 0x56, 0x37, 0xFB, 0x76};
    for (size_t i = 0; i < boot.size(); i++) {
      put(i, boot[i]);
    }

    // Interrupt 1 handler at 0x0038:
    // EI (FB), JP 0x0100 (C3 00 01)
    std::vector<uint8_t> isr1 = {0xFB, 0xC3, 0x00, 0x01};
    for (size_t i = 0; i < isr1.size(); i++) {
      put(0x0038 + i, isr1[i]);
    }

    // Sled at 0x0100:
    uint16_t sled_addr = 0x0100;
    for (int i = 0; i < pad_nops; i++) {
      put(sled_addr++, 0x00); // NOP padding to shift instruction phase
    }
    for (uint16_t a = sled_addr; a < 0x2000; a++) {
      put(a, op);
    }

    // Align CPU reset release to 1 us boundary
    while ((t % 64) != 0) tick();
    d.cpu_reset = 0;

    // Run until Interrupt 2 is acknowledged (or timeout)
    while (t < 600000) {
      if (calibration && res.irq_fall >= 0 && t > uint64_t(res.irq_fall + 512)) break;
      if (!calibration && ack_count >= 2) break;
      tick();
    }
    if ((!calibration && ack_count < 2) || res.irq_fall < 0) {
      throw std::runtime_error("Timeout waiting for 2nd interrupt acknowledge");
    }
  }
};

void check(bool ok, const std::string &what) {
  if (!ok) throw std::runtime_error(what);
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  unsigned passed = 0, failed = 0;
  for (int ct = 0; ct != 2; ++ct) {
    unsigned sensitive_ret_phases = 0;
    for (unsigned cell = 0; cell != 4; ++cell) {
      const uint8_t op = cell == 0 ? 0x00 : cell == 1 ? 0x76 : 0xD0;
      const int pad = cell == 3;
      const std::string name = "CRTC" + std::to_string(ct) + "/" +
          (cell == 0 ? "NOP" : cell == 1 ? "HALT" : "RET-NC") +
          "/pad" + std::to_string(pad);
      try {
        ClassicIrqBench cal(ct, op, pad, true), actual(ct, op, pad);
        check(cal.res.irq_fall == actual.res.irq_fall,
              "calibration changed the GA request phase");
        int64_t first = -1, expected = -1, prior = -1;
        for (auto event : cal.fetch) {
          if (event.first < cal.res.irq_fall) prior = event.first;
          if (event.first > cal.res.irq_fall && first < 0) first = event.first;
          if (event.first - 16 > cal.res.irq_fall) { expected = event.first; break; }
        }
        check(first >= 0 && expected >= 0, "missing uninterrupted M1 boundaries");
        if (cell < 2) check(expected == first, "ACCC NOP/HALT control lost robust phase");
        else if (expected != first) ++sensitive_ret_phases;
        if (cell == 0) {
          // In this uninterrupted NOP sled, T2 starts one CPU clock after
          // the preceding M1. Observe pins AFTER their generating edge;
          // reading combinational phi_p after that edge labels the next edge.
          check(prior >= 0, "missing NOP preceding IRQ");
          const int64_t n = prior + 16;
          std::cout << "MEASURE " << name << " HS-N=" << cal.res.hs_fall-n
                    << " IRQ-N=" << cal.res.irq_fall-n
                    << " N=" << n << " (NOP T2 physical rising edge)\n";
        }
        check(actual.res.entry == expected,
              "expected M1 entry=" + std::to_string(expected) +
              " from final-T sample, got=" + std::to_string(actual.res.entry) +
              " IRQ=" + std::to_string(actual.res.irq_fall));
        check(actual.res.ack > actual.res.entry, "missing pin acknowledge after M1 entry");
        std::cout << "PASS " << name << " entry=" << actual.res.entry
                  << " rejected_first=" << (expected != first) << "\n";
        ++passed;
      } catch (const std::exception &e) {
        std::cout << "FAIL " << name << ": " << e.what() << "\n"; ++failed;
      }
    }
    // Count is based on uninterrupted phases, independent of acceptance.
    if (sensitive_ret_phases != 1) {
      std::cout << "FAIL CRTC" << ct << ": RET padding failed to exercise both phases\n";
      ++failed;
    }
  }
  std::cout << "classic_irq_phase: " << passed << " passed, " << failed << " failed\n";
  return failed ? 1 : 0;
}
