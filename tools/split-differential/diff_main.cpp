// Differential debug harness (throwaway, not committed): pre-split reference
// core vs split core under soak-style stimulus; both models are driven in
// lockstep and compared after every CLKEN edge.
#include <verilated.h>

#include "VCRTC.h"
#include "VCRTC___024root.h"
#include "VUM6845R_REF.h"
#include "VUM6845R_REF___024root.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>

constexpr unsigned kTicksPerChar = 16;
constexpr unsigned kClkEnPhase = 0;
constexpr unsigned kNClkEnPhase = 8;

struct Sample {
    std::uint16_t ma;
    std::uint8_t ra, de, hsync, vsync, cursor, field, do_;
    std::uint8_t hcc, line5, row7, c5, in_adj;
    std::uint8_t l_r4sw, l_r9live, l_r9atr0, l_c01, l_r0zc, l_zeroadj, l_r5ovr;
    std::uint8_t r5target;
    std::uint8_t line_last_r, row_last_r, frame_adj_r, fieldq, from_row0;
    std::uint8_t wait_latch, status5, vsync_r, vde, vde_r, allow, hde, hsc;
    std::uint8_t row_addr_h, row_addr_l, row_addr_r_h, row_addr_r_l;
};

class Rng {
public:
    explicit Rng(std::uint64_t seed) : s_(seed) {}
    std::uint64_t next() {
        s_ += 0x9e3779b97f4a7c15ULL;
        std::uint64_t z = s_;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
    std::uint64_t below(std::uint64_t n) { return next() % n; }
private:
    std::uint64_t s_;
};

std::uint8_t soak_random_value(Rng& rng) {
    const std::uint64_t roll = rng.below(100);
    if (roll < 40) return static_cast<std::uint8_t>(rng.below(4));
    if (roll < 65) return static_cast<std::uint8_t>(rng.below(16));
    if (roll < 90) return static_cast<std::uint8_t>(rng.below(256));
    constexpr std::array<std::uint8_t, 14> kB = {{
        0x00, 0x01, 0x02, 0x03, 0x0f, 0x1f, 0x3f,
        0x40, 0x7f, 0x80, 0xc0, 0xf0, 0xfe, 0xff,
    }};
    return kB[rng.below(kB.size())];
}

class DualBench {
public:
    explicit DualBench(VCRTC* a_, VUM6845R_REF* b_) : a(a_), b(b_) {
        idle_bus();
        a->SNA_LOAD = 0; b->SNA_LOAD = 0;
        a->SNA_ADDR = 0; b->SNA_ADDR = 0;
        for (unsigned w = 0; w < 5; ++w) {
            a->SNA_REGS[w] = 0;
            b->SNA_REGS[w] = 0;
        }
        eval();
    }

    void set_crtc_type(unsigned t) {
        a->CRTC_TYPE = t; b->CRTC_TYPE = t; eval();
    }

    void reset() {
        idle_bus();
        a->nRESET = 0; b->nRESET = 0;
        while (phase != kClkEnPhase) tick();
        for (unsigned t = 0; t < kTicksPerChar; ++t) tick();
        a->nRESET = 1; b->nRESET = 1;
        eval();
    }

    void write_register(std::uint8_t address, std::uint8_t value) {
        select_register(address);
        bus_write(true, value);
    }
    void select_register(std::uint8_t address) { bus_write(false, address); }
    void write_selected_at_clken(std::uint8_t value) {
        while (phase != kClkEnPhase) tick();
        bus_write(true, value);
    }
    void write_selected_at_nclken(std::uint8_t value) {
        while (phase != kNClkEnPhase) tick();
        bus_write(true, value);
    }
    void hold_selected_at_clken(std::uint8_t value, unsigned ticks) {
        while (phase != kClkEnPhase) tick();
        drive_bus(true, value);
        for (unsigned t = 0; t < ticks; ++t) tick();
        idle_bus();
        eval();
    }
    void load_snapshot(const std::array<std::uint8_t, 10>& regs) {
        for (unsigned w = 0; w < 5; ++w) {
            a->SNA_REGS[w] = 0; b->SNA_REGS[w] = 0;
        }
        for (unsigned ad = 0; ad < regs.size(); ++ad) {
            const unsigned bit = ad * 8;
            const std::uint32_t v = std::uint32_t(regs[ad]) << (bit % 32);
            a->SNA_REGS[bit / 32] |= v;
            b->SNA_REGS[bit / 32] |= v;
        }
        a->SNA_ADDR = 7; b->SNA_ADDR = 7;
        a->SNA_LOAD = 1; b->SNA_LOAD = 1;
        tick();
        a->SNA_LOAD = 0; b->SNA_LOAD = 0;
        eval();
    }
    void run_ticks(std::uint64_t n) {
        for (std::uint64_t t = 0; t < n; ++t) tick();
    }

    unsigned cur_phase() const { return phase; }

    std::function<void()> on_tick_end;

private:
    void idle_bus() {
        a->ENABLE = 0; a->nCS = 1; a->R_nW = 1; a->RS = 0; a->DI = 0;
        b->ENABLE = 0; b->nCS = 1; b->R_nW = 1; b->RS = 0; b->DI = 0;
    }
    void drive_bus(bool rs, std::uint8_t value) {
        a->ENABLE = 1; a->nCS = 0; a->R_nW = 0; a->RS = rs; a->DI = value;
        b->ENABLE = 1; b->nCS = 0; b->R_nW = 0; b->RS = rs; b->DI = value;
    }
    void bus_write(bool rs, std::uint8_t value) {
        drive_bus(rs, value);
        tick();
        idle_bus();
        eval();
    }
    void tick() {
        const unsigned active = phase;
        a->CLKEN = active == kClkEnPhase;
        a->nCLKEN = active == kNClkEnPhase;
        b->CLKEN = a->CLKEN;
        b->nCLKEN = a->nCLKEN;
        a->CLOCK = 0; b->CLOCK = 0; eval();
        a->CLOCK = 1; b->CLOCK = 1; eval();
        a->CLOCK = 0; b->CLOCK = 0; eval();
        phase = (phase + 1) % kTicksPerChar;
        if (on_tick_end) on_tick_end();
    }
    void eval() { a->eval(); b->eval(); }

    VCRTC* a;
    VUM6845R_REF* b;
    unsigned phase = 0;
};

static VCRTC* a;
static VUM6845R_REF* b;

Sample sample_new() {
    auto* r = a->rootp;
    Sample s{};
    s.ma = a->MA; s.ra = a->RA; s.de = a->DE; s.hsync = a->HSYNC;
    s.vsync = a->VSYNC; s.cursor = a->CURSOR; s.field = a->FIELD; s.do_ = a->DO;
    s.hcc = r->CRTC__DOT__hcc; s.line5 = r->CRTC__DOT__line;
    s.row7 = r->CRTC__DOT__row; s.c5 = r->CRTC__DOT__c5;
    s.in_adj = r->CRTC__DOT__in_adj;
    s.l_r4sw = r->CRTC__DOT__crtc_type0_engine__DOT__type0_r4_adjust_switch;
    s.l_r9live = r->CRTC__DOT__crtc_type0_engine__DOT__type0_r9_live_compare;
    s.l_r9atr0 = r->CRTC__DOT__crtc_type0_engine__DOT__type0_r9_at_r0_pending;
    s.l_c01 = r->CRTC__DOT__crtc_type0_engine__DOT__type0_c0_1_adjust;
    s.l_r0zc = r->CRTC__DOT__crtc_type0_engine__DOT__type0_r0_zero_entry_consumed;
    s.l_zeroadj = r->CRTC__DOT__crtc_type0_engine__DOT__type0_zero_adj_entry;
    s.l_r5ovr = r->CRTC__DOT__crtc_type0_engine__DOT__type0_r5_adjust_override;
    s.r5target = r->CRTC__DOT__crtc_type0_engine__DOT__type0_r5_adjust_target;
    s.wait_latch = r->CRTC__DOT__crtc_type0_engine__DOT__type0_vsync_wait_line_start;
    s.line_last_r = r->CRTC__DOT__line_last_r;
    s.row_last_r = r->CRTC__DOT__row_last_r;
    s.frame_adj_r = r->CRTC__DOT__frame_adj_r;
    s.fieldq = r->CRTC__DOT__field;
    s.from_row0 = r->CRTC__DOT__crtc1_adj_from_row0;
    s.status5 = r->CRTC__DOT__crtc_type1_engine__DOT__status_bit5_r;
    s.vsync_r = r->CRTC__DOT__VSYNC_r;
    s.vde = r->CRTC__DOT__vde;
    s.vde_r = r->CRTC__DOT__vde_r;
    s.allow = r->CRTC__DOT__vsync_allow;
    s.hde = r->CRTC__DOT__hde; s.hsc = r->CRTC__DOT__hsc;
    s.row_addr_h = r->CRTC__DOT__row_addr >> 8;
    s.row_addr_l = r->CRTC__DOT__row_addr & 0xff;
    s.row_addr_r_h = r->CRTC__DOT__row_addr_r >> 8;
    s.row_addr_r_l = r->CRTC__DOT__row_addr_r & 0xff;
    return s;
}

Sample sample_ref() {
    auto* r = b->rootp;
    Sample s{};
    s.ma = b->MA; s.ra = b->RA; s.de = b->DE; s.hsync = b->HSYNC;
    s.vsync = b->VSYNC; s.cursor = b->CURSOR; s.field = b->FIELD; s.do_ = b->DO;
    s.hcc = r->UM6845R_REF__DOT__hcc; s.line5 = r->UM6845R_REF__DOT__line;
    s.row7 = r->UM6845R_REF__DOT__row; s.c5 = r->UM6845R_REF__DOT__c5;
    s.in_adj = r->UM6845R_REF__DOT__in_adj;
    s.l_r4sw = r->UM6845R_REF__DOT__type0_r4_adjust_switch;
    s.l_r9live = r->UM6845R_REF__DOT__type0_r9_live_compare;
    s.l_r9atr0 = r->UM6845R_REF__DOT__type0_r9_at_r0_pending;
    s.l_c01 = r->UM6845R_REF__DOT__type0_c0_1_adjust;
    s.l_r0zc = r->UM6845R_REF__DOT__type0_r0_zero_entry_consumed;
    s.l_zeroadj = r->UM6845R_REF__DOT__type0_zero_adj_entry;
    s.l_r5ovr = r->UM6845R_REF__DOT__type0_r5_adjust_override;
    s.r5target = r->UM6845R_REF__DOT__type0_r5_adjust_target;
    s.wait_latch = r->UM6845R_REF__DOT__unnamedblk1__DOT__type0_vsync_wait_line_start;
    s.line_last_r = r->UM6845R_REF__DOT__line_last_r;
    s.row_last_r = r->UM6845R_REF__DOT__row_last_r;
    s.frame_adj_r = r->UM6845R_REF__DOT__frame_adj_r;
    s.fieldq = r->UM6845R_REF__DOT__field;
    s.from_row0 = r->UM6845R_REF__DOT__crtc1_adj_from_row0;
    s.status5 = r->UM6845R_REF__DOT__status_bit5;
    s.vsync_r = r->UM6845R_REF__DOT__VSYNC_r;
    s.vde = r->UM6845R_REF__DOT__vde;
    s.vde_r = r->UM6845R_REF__DOT__vde_r;
    s.allow = r->UM6845R_REF__DOT__unnamedblk1__DOT__vsync_allow;
    s.hde = r->UM6845R_REF__DOT__hde; s.hsc = r->UM6845R_REF__DOT__hsc;
    s.row_addr_h = r->UM6845R_REF__DOT__row_addr >> 8;
    s.row_addr_l = r->UM6845R_REF__DOT__row_addr & 0xff;
    s.row_addr_r_h = r->UM6845R_REF__DOT__row_addr_r >> 8;
    s.row_addr_r_l = r->UM6845R_REF__DOT__row_addr_r & 0xff;
    return s;
}

void dump(const char* tag, const Sample& s) {
    std::printf("  %-4s ma=%04x ra=%2u de=%u hs=%u vs=%u cur=%u fld=%u do=%02x\n",
                tag, s.ma, s.ra, s.de, s.hsync, s.vsync, s.cursor, s.field, s.do_);
    std::printf("       hcc=%3u line=%2u row=%3u c5=%2u in_adj=%u llr=%u rlr=%u far=%u fldq=%u frow0=%u\n",
                s.hcc, s.line5, s.row7, s.c5, s.in_adj, s.line_last_r, s.row_last_r,
                s.frame_adj_r, s.fieldq, s.from_row0);
    std::printf("       latches: r4sw=%u r9live=%u r9@r0=%u c01=%u r0zc=%u zeroadj=%u r5ovr=%u tgt=%u wait=%u st5=%u\n",
                s.l_r4sw, s.l_r9live, s.l_r9atr0, s.l_c01, s.l_r0zc,
                s.l_zeroadj, s.l_r5ovr, s.r5target, s.wait_latch, s.status5);
    std::printf("       vsync_r=%u vde=%u vde_r=%u allow=%u hde=%u hsc=%u ra=%02x%02x rar=%02x%02x\n",
                s.vsync_r, s.vde, s.vde_r, s.allow, s.hde, s.hsc,
                s.row_addr_h, s.row_addr_l, s.row_addr_r_h, s.row_addr_r_l);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    a = new VCRTC{"new"};
    // globals set here
    b = new VUM6845R_REF{"ref"};
    DualBench bench(a, b);
    Rng rng(0xaccc5eed20260822ULL);
    constexpr std::uint64_t kEventsPerType = 150000;

    std::uint64_t samples = 0;
    bool diverged = false;
    struct Trace {
        unsigned phase; std::uint8_t clken, nclken, en, cs, rnw, rs, di;
        std::uint8_t wr7hit, wfire_n, wfire_r, cnt_n, cnt_r;
        std::uint8_t vr_n, vr_r, al_n, al_r, wt_n, wt_r, hcc, snal, rst;
    };
    Trace hist[40]; unsigned hist_n = 0;
    auto snap_trace = [&](Trace& t) {
        auto* ra_ = a->rootp; auto* rb_ = b->rootp;
        t.phase = bench.cur_phase();
        t.clken = a->CLKEN; t.nclken = a->nCLKEN;
        t.en = a->ENABLE; t.cs = a->nCS; t.rnw = a->R_nW; t.rs = a->RS; t.di = a->DI;
        t.snal = a->SNA_LOAD; t.rst = a->nRESET == 0;
        t.hcc = ra_->CRTC__DOT__hcc;
        bool wh = (a->ENABLE & a->RS & !a->nCS & !a->R_nW && ra_->CRTC__DOT__addr == 7);
        t.wr7hit = wh;
        t.wt_n = ra_->CRTC__DOT__crtc_type0_engine__DOT__type0_vsync_wait_line_start;
        t.wt_r = rb_->UM6845R_REF__DOT__unnamedblk1__DOT__type0_vsync_wait_line_start;
        t.vr_n = ra_->CRTC__DOT__VSYNC_r; t.vr_r = rb_->UM6845R_REF__DOT__VSYNC_r;
        t.al_n = ra_->CRTC__DOT__vsync_allow;
        t.al_r = rb_->UM6845R_REF__DOT__unnamedblk1__DOT__vsync_allow;
        // post-edge values only; fire/count reconstructed approximately below
        t.wfire_n = t.wfire_r = 255; t.cnt_n = t.cnt_r = 255;
    };
    bench.on_tick_end = [&]() {
        ++samples;
        Trace t; snap_trace(t);
        hist[hist_n % 40] = t; ++hist_n;
        Sample x = sample_new();
        Sample y = sample_ref();
        if (!diverged && 0 != std::memcmp(&x, &y, sizeof(Sample))) {
            diverged = true;
            std::printf("DIVERGENCE at sample %llu (phase %u)\n",
                        (unsigned long long)samples, bench.cur_phase());
            dump("NEW", x);
            dump("REF", y);
            {
                unsigned n = hist_n < 40 ? (unsigned)hist_n : 40u;
                unsigned start = hist_n - n;
                std::printf("--- last %u edges: ph c n en/cs/rnw/rs DI snal rst hcc | wr7 vr_n al_n wt_n | vr_r al_r wt_r\n", n);
                for (unsigned i = start; i < hist_n; ++i) {
                    const Trace& t = hist[i % 40];
                    std::printf("  s=%3u ph=%2u c%u n%u %u%u%u%u %02x s%u r%u hcc=%3u | %u %u %u %u | %u %u %u\n",
                                i, t.phase, t.clken, t.nclken, t.en, t.cs, t.rnw, t.rs,
                                t.di, t.snal, t.rst, t.hcc,
                                t.wr7hit, t.vr_n, t.al_n, t.wt_n,
                                t.vr_r, t.al_r, t.wt_r);
                }
            }
        }
    };

    for (unsigned type = 0; type <= 1 && !diverged; ++type) {
        bench.set_crtc_type(type);
        bench.write_register(8, 0);
        bench.reset();

        for (unsigned address = 0; address <= 15; ++address) {
            bench.write_register(address, soak_random_value(rng));
        }

        unsigned current_type = type;
        for (std::uint64_t event = 0; event < kEventsPerType && !diverged; ++event) {
            const std::uint64_t lifecycle = rng.below(1024);
            if (lifecycle == 0) {
                bench.reset();
            } else if (lifecycle == 1) {
                current_type ^= 1;
                bench.set_crtc_type(current_type);
            }

            const std::uint64_t roll = rng.below(100);
            if (roll < 60) {
                unsigned address = rng.below(16);
                if (rng.below(32) == 0) address |= 32u + rng.below(32);
                const std::uint8_t value = soak_random_value(rng);
                const std::uint64_t style = rng.below(100);
                if (style < 55) {
                    bench.write_register(address, value);
                } else if (style < 78) {
                    bench.select_register(address);
                    bench.write_selected_at_clken(value);
                } else {
                    bench.select_register(address);
                    bench.write_selected_at_nclken(value);
                }
            } else if (roll < 70) {
                if (rng.below(4) != 0) rng.below(32);
            } else if (roll < 76) {
                bench.select_register(rng.below(32));
            } else if (roll < 80) {
                const std::uint8_t address = rng.below(16);
                const std::uint8_t value = soak_random_value(rng);
                const unsigned held = kTicksPerChar * (1 + rng.below(3));
                bench.select_register(address);
                bench.hold_selected_at_clken(value, held);
            } else if (roll < 82) {
                std::array<std::uint8_t, 10> snap{};
                for (auto& v : snap) v = soak_random_value(rng);
                bench.load_snapshot(snap);
            } else {
                bench.run_ticks(rng.below(64) * kTicksPerChar);
            }

            bench.run_ticks(rng.below(kTicksPerChar * 7));
        }
        if (!diverged) {
            std::printf("type %u clean through %llu events (%llu samples)\n",
                        type, (unsigned long long)kEventsPerType,
                        (unsigned long long)samples);
        }
    }
    if (!diverged) std::printf("no divergence\n");
    return diverged ? 1 : 0;
}
