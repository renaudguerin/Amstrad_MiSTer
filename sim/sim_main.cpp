//============================================================================
//  Verilator testbench for the Amstrad CPC CRTC (UM6845R / HD6845S).
//
//  ACCURACY REFERENCE
//
//  Technical information sourced from the "Amstrad CPC CRTC Compendium" by
//  Longshot (CC BY-NC-ND).
//
//  The expected values asserted by these vectors are derived from The Amstrad
//  CPC CRTC Compendium v1.10, Serge Querne (Longshot / Logon System),
//  https://shaker.logonsystem.eu -- licensed CC BY-NC-ND 4.0. Its attribution
//  directive requires this notice in the source of CRTC emulation modules and
//  in the credits of any distributed product built from them. Individual
//  vectors cite their ACCC section alongside the assertion.
//============================================================================

#include <verilated.h>
#include <verilated_vcd_c.h>

#include "VCRTC.h"
#include "VCRTC___024root.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr unsigned kClockTicksPerCharacter = 16;
constexpr unsigned kClkEnPhase = 0;
constexpr unsigned kNClkEnPhase = 8;

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class KnownDivergenceFailure : public TestFailure {
public:
    using TestFailure::TestFailure;
};

class TestBench {
public:
    // An empty trace_path disables VCD tracing (used by the soak harness,
    // which runs far too many cycles to record).
    explicit TestBench(const std::string& trace_path)
        : context_(std::make_unique<VerilatedContext>()),
          dut_(std::make_unique<VCRTC>(context_.get())),
          trace_(std::make_unique<VerilatedVcdC>()) {
        context_->traceEverOn(true);
        dut_->CLOCK = 0;
        dut_->CLKEN = 0;
        dut_->nCLKEN = 0;
        dut_->nRESET = 0;
        dut_->CRTC_TYPE = 0;
        idle_bus();
        dut_->SNA_LOAD = 0;
        dut_->SNA_ADDR = 0;
        for (unsigned word = 0; word < 5; ++word) {
            dut_->SNA_REGS[word] = 0;
        }

        if (!trace_path.empty()) {
            dut_->trace(trace_.get(), 99);
            trace_->open(trace_path.c_str());
        }
        eval_and_dump();
    }

    ~TestBench() {
        dut_->final();
        if (trace_->isOpen()) {
            trace_->close();
        }
    }

    TestBench(const TestBench&) = delete;
    TestBench& operator=(const TestBench&) = delete;

    void set_crtc_type(unsigned type) {
        if (type > 1) {
            fail("CRTC type 0 or 1", type);
        }
        dut_->CRTC_TYPE = type;
        eval_comb();
    }

    // Register values survive nRESET on the physical interface and in the RTL.
    // Program R8 before checking reset so FIELD does not depend on power-up data.
    void prepare_for_reset(unsigned type) {
        set_crtc_type(type);
        write_register(8, 0);
        reset();
    }

    void reset() {
        idle_bus();
        dut_->nRESET = 0;

        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
            clock_tick();
        }

        dut_->nRESET = 1;
        character_cycles_ = 0;
        eval_comb();
    }

    void select_register(std::uint8_t address) {
        bus_write(false, address);
    }

    void write_register(std::uint8_t address, std::uint8_t value) {
        select_register(address);
        bus_write(true, value);
    }

    void write_selected_register_at_nclken(std::uint8_t value) {
        while (tick_in_character_ != kNClkEnPhase) {
            clock_tick();
        }
        bus_write(true, value);
    }

    void write_selected_register_at_clken(std::uint8_t value) {
        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        bus_write(true, value);
    }

    void write_selected_register_now(std::uint8_t value) {
        bus_write(true, value);
    }

    void hold_selected_register_at_clken(std::uint8_t value,
                                         unsigned clock_ticks) {
        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 0;
        dut_->RS = 1;
        dut_->DI = value;
        for (unsigned tick = 0; tick < clock_ticks; ++tick) {
            clock_tick();
        }
        idle_bus();
        eval_comb();
    }

    void load_snapshot_registers(
        const std::array<std::uint8_t, 10>& registers) {
        for (unsigned word = 0; word < 5; ++word) {
            dut_->SNA_REGS[word] = 0;
        }
        for (unsigned address = 0; address < registers.size(); ++address) {
            const unsigned bit = address * 8;
            dut_->SNA_REGS[bit / 32] |=
                static_cast<std::uint32_t>(registers[address]) << (bit % 32);
        }
        dut_->SNA_ADDR = 7;
        dut_->SNA_LOAD = 1;
        clock_tick();
        dut_->SNA_LOAD = 0;
        eval_comb();
    }

    std::uint8_t read_register(std::uint8_t address) {
        select_register(address);
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 1;
        dut_->RS = 1;
        eval_comb();
        const std::uint8_t value = dut_->DO;
        idle_bus();
        eval_comb();
        return value;
    }

    std::uint8_t read_status() {
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 1;
        dut_->RS = 0;
        eval_comb();
        const std::uint8_t value = dut_->DO;
        idle_bus();
        eval_comb();
        return value;
    }

    void run_characters(std::uint64_t characters) {
        for (std::uint64_t character = 0; character < characters; ++character) {
            for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
                clock_tick();
            }
        }
    }

    void run_clock_ticks(std::uint64_t ticks) {
        for (std::uint64_t tick = 0; tick < ticks; ++tick) {
            clock_tick();
        }
    }

    // Soak-harness hook: invoked once after every CLKEN clock edge.
    void set_clken_sampler(std::function<void()> sampler) {
        clken_sampler_ = std::move(sampler);
    }

    // Monotonic character count; unlike character_cycles_ this survives
    // resets (the soak uses it for its volume report).
    std::uint64_t total_characters() const {
        return total_characters_;
    }

    // Fold every pin plus the key counter/arbitration state into the
    // caller's FNV-1a rolling hash (see "Randomized equivalence soak" in
    // sim/README.md). Field order and widths are part of the golden-hash
    // contract: any change here invalidates a minted hash.
    void soak_mix_sample(std::uint64_t& hash) const {
        const auto& r = *dut_->rootp;
        const auto mix = [&hash](std::uint64_t value) {
            hash ^= value;
            hash *= 1099511628211ULL;
        };
        mix(dut_->MA);
        mix(dut_->RA);
        mix(dut_->DE);
        mix(dut_->HSYNC);
        mix(dut_->VSYNC);
        mix(dut_->CURSOR);
        mix(dut_->FIELD);
        mix(dut_->DO);
        // Wrapper-owned shared counters...
        mix(r.CRTC__DOT__hcc);
        mix(r.CRTC__DOT__line);
        mix(r.CRTC__DOT__row);
        mix(r.CRTC__DOT__c5);
        mix(r.CRTC__DOT__in_adj);
        // ...and the type-0 engine's private arbitration latches. The hashed
        // values are unchanged by the type split; only the hierarchy paths
        // moved.
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_r4_adjust_switch);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_r9_live_compare);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_r9_at_r0_pending);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_c0_1_adjust);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_r0_zero_entry_consumed);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_zero_adj_entry);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_r5_adjust_override);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_r5_adjust_target);
        // Review issue 4 remediation (2026-08-23): the relocated partial-VSYNC
        // holdoff latch and the type-1 private status flops join the sampled
        // projection. The dev-time holdoff bug escaped exactly because this
        // latch was unsampled; expanding the field set re-mints the golden
        // hash by design (no RTL behaviour change).
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__type0_vsync_wait_line_start);
        mix(r.CRTC__DOT__crtc_type1_engine__DOT__r6_border_condition);
        mix(r.CRTC__DOT__crtc_type1_engine__DOT__status_bit5_r);
        // F10 fixture stage (2026-08-24): the shared interlace parity flops
        // join the sampled projection while they still hold reset values.
        // Field-set expansion only; no RTL behaviour change in this commit.
        mix(r.CRTC__DOT__parity_frame);
        mix(r.CRTC__DOT__parity_c9);
        mix(r.CRTC__DOT__parity_r6);
        // F10 type-1 behavior commit (2026-08-24): the IVM flag and toggle
        // stage machine join the sampled projection together with their
        // behavior; random R8 traffic now reaches the documented toggle
        // stages, so this mint covers both the field expansion and the
        // intended type-1 IVM behavior change.
        mix(r.CRTC__DOT__crtc_type1_engine__DOT__ivm);
        mix(r.CRTC__DOT__crtc_type1_engine__DOT__tog_stage);
        // Review N-10 (2026-08-25): the toggle direction latches complete
        // the stage machinery's sampled state.
        mix(r.CRTC__DOT__crtc_type1_engine__DOT__tog_enter);
        // F10 type-0 behavior commit: the seam-latched IVM mode and the
        // line-scoped toggle status join the projection with their
        // behavior; random R8 traffic now reaches the type-0 seams too.
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__ivm_disp_r);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__tog_line);
        mix(r.CRTC__DOT__crtc_type0_engine__DOT__tog_enter_line);
    }

    void expect_byte(const std::string& expectation,
                     std::uint8_t expected,
                     std::uint8_t actual) const {
        if (actual != expected) {
            std::ostringstream expected_text;
            std::ostringstream actual_text;
            expected_text << "0x" << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<unsigned>(expected);
            actual_text << "0x" << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned>(actual);
            fail(expectation + " == " + expected_text.str(), actual_text.str());
        }
    }

    void expect_low(const std::string& signal, std::uint8_t actual) const {
        if (actual != 0) {
            fail(signal + " low", static_cast<unsigned>(actual));
        }
    }

    void expect_high(const std::string& signal, std::uint8_t actual) const {
        if (actual != 1) {
            fail(signal + " high", static_cast<unsigned>(actual));
        }
    }

    void expect_vsync_low(const std::string& expectation) const {
        expect_low(expectation, dut_->VSYNC);
    }

    void expect_vsync_high(const std::string& expectation) const {
        expect_high(expectation, dut_->VSYNC);
    }

    void expect_hsync_low(const std::string& expectation) const {
        expect_low(expectation, dut_->HSYNC);
    }

    void expect_hsync_high(const std::string& expectation) const {
        expect_high(expectation, dut_->HSYNC);
    }

    void expect_field_low(const std::string& expectation) const {
        expect_low(expectation, dut_->FIELD);
    }

    void expect_ra(const std::string& expectation, std::uint8_t expected) const {
        expect_byte(expectation, expected, dut_->RA);
    }

    void expect_c4(const std::string& expectation, std::uint8_t expected) const {
        expect_byte(expectation, expected, dut_->rootp->CRTC__DOT__row);
    }

    void expect_c5(const std::string& expectation, std::uint8_t expected) const {
        expect_byte(expectation, expected, dut_->rootp->CRTC__DOT__c5);
    }

    // Whole-frame VSYNC observation for the section 28.1.1 R7 sweep: the pulse
    // is many characters wide, so one sample per character cannot miss it.
    bool vsync_within_characters(std::uint64_t characters) {
        for (std::uint64_t character = 0; character < characters; ++character) {
            run_characters(1);
            if (dut_->VSYNC != 0) {
                return true;
            }
        }
        return false;
    }

    void expect_vsync_observed(const std::string& expectation, bool seen) const {
        if (!seen) {
            fail(expectation, "no VSYNC pulse");
        }
    }

    void expect_no_vsync_observed(const std::string& expectation,
                                  bool seen) const {
        if (seen) {
            fail(expectation, "a VSYNC pulse");
        }
    }


    void expect_adjustment_active(const std::string& expectation) const {
        expect_high(expectation, dut_->rootp->CRTC__DOT__in_adj);
    }

    void expect_adjustment_inactive(const std::string& expectation) const {
        expect_low(expectation, dut_->rootp->CRTC__DOT__in_adj);
    }

    void expect_type0_arbitration_latches(const std::string& expectation,
                                          bool r4_switch,
                                          bool r9_live,
                                          bool r9_at_r0,
                                          bool c0_1_adjust = false,
                                          bool r0_zero_consumed = false,
                                          bool r5_override = false) const {
        expect_byte(expectation + " R4 switch", r4_switch,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_r4_adjust_switch);
        expect_byte(expectation + " live R9 compare", r9_live,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_r9_live_compare);
        expect_byte(expectation + " exact-R0 R9", r9_at_r0,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_r9_at_r0_pending);
        expect_byte(expectation + " C0=1 adjustment", c0_1_adjust,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_c0_1_adjust);
        expect_byte(expectation + " R0=0 entry consumed", r0_zero_consumed,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_r0_zero_entry_consumed);
        expect_byte(expectation + " R5 adjust override", r5_override,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_r5_adjust_override);
    }

    void expect_type0_r5_adjust_override(const std::string& expectation,
                                         bool expected) const {
        expect_byte(expectation, expected,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_r5_adjust_override);
    }

    void expect_type0_zero_adj_entry(const std::string& expectation,
                                     bool expected) const {
        expect_byte(expectation, expected,
                    dut_->rootp->CRTC__DOT__crtc_type0_engine__DOT__type0_zero_adj_entry);
    }

    void expect_type1_rfd_state(const std::string& expectation,
                                bool vma_flag,
                                bool parity_flag,
                                bool frame_parity) const {
        const auto& root = *dut_->rootp;
        expect_byte(expectation + " VMA-source flag", vma_flag,
                    root.CRTC__DOT__crtc_type1_engine__DOT__rfd_vma_flag);
        expect_byte(expectation + " parity-management flag", parity_flag,
                    root.CRTC__DOT__crtc_type1_engine__DOT__rfd_parity_flag);
        expect_byte(expectation + " frame parity", frame_parity,
                    root.CRTC__DOT__crtc_type1_engine__DOT__rfd_frame_parity);
    }

    void expect_type1_rfd_pending(const std::string& expectation,
                                  bool expected) const {
        expect_byte(expectation, expected,
                    dut_->rootp->CRTC__DOT__crtc_type1_engine__DOT__rfd_r0_pending);
    }

    // XFAIL variants of the counter/parity assertions (review A4 rename:
    // the old expect_known_* name read like a plain assertion). Identical
    // checks, but a violation throws KnownDivergenceFailure: in a test
    // registered with known_divergence=true that reports XFAIL; anywhere
    // else it is an ordinary failure. House pattern from the F4 fixture
    // commit: use these ONLY in fixture-first XFAIL commits; the behavior
    // commit replaces them with the plain expect_* forms and flips the
    // flag in the same change.
    void expect_xfail_byte(const std::string& expectation,
                           std::uint8_t expected,
                           std::uint8_t actual) const {
        if (actual != expected) {
            known_divergence(expectation + " == " + std::to_string(expected),
                             static_cast<unsigned>(actual));
        }
    }

    void expect_xfail_line(const std::string& expectation,
                           std::uint8_t expected) const {
        expect_xfail_byte(expectation, expected, dut_->rootp->CRTC__DOT__line);
    }

    void expect_xfail_c5(const std::string& expectation,
                         std::uint8_t expected) const {
        expect_xfail_byte(expectation, expected, dut_->rootp->CRTC__DOT__c5);
    }

    void expect_xfail_ra(const std::string& expectation,
                         std::uint8_t expected) const {
        expect_xfail_byte(expectation, expected, dut_->RA);
    }

    void expect_xfail_c4(const std::string& expectation,
                         std::uint8_t expected) const {
        expect_xfail_byte(expectation, expected, dut_->rootp->CRTC__DOT__row);
    }

    void expect_xfail_parity_frame(const std::string& expectation,
                                   bool expected) const {
        expect_xfail_byte(expectation, expected,
                          dut_->rootp->CRTC__DOT__parity_frame);
    }

    void expect_xfail_parity_c9(const std::string& expectation,
                                bool expected) const {
        expect_xfail_byte(expectation, expected,
                          dut_->rootp->CRTC__DOT__parity_c9);
    }

    void expect_xfail_line_parity(const std::string& expectation,
                                  bool expected) const {
        expect_xfail_byte(expectation, expected,
                          dut_->rootp->CRTC__DOT__line & 1);
    }

    void expect_line(const std::string& expectation,
                     std::uint8_t expected) const {
        expect_byte(expectation, expected, dut_->rootp->CRTC__DOT__line);
    }

    void expect_parity_frame(const std::string& expectation,
                             bool expected) const {
        expect_byte(expectation, expected,
                    dut_->rootp->CRTC__DOT__parity_frame);
    }

    void expect_parity_c9(const std::string& expectation,
                          bool expected) const {
        expect_byte(expectation, expected,
                    dut_->rootp->CRTC__DOT__parity_c9);
    }

    void expect_line_parity(const std::string& expectation,
                            bool expected) const {
        expect_byte(expectation, expected,
                    dut_->rootp->CRTC__DOT__line & 1);
    }

    // Internal-state accessors for the F10 fixture setup walkers.
    std::uint8_t c0() const {
        return dut_->rootp->CRTC__DOT__hcc;
    }

    std::uint8_t line_reg() const {
        return dut_->rootp->CRTC__DOT__line;
    }

    std::uint8_t c4_reg() const {
        return dut_->rootp->CRTC__DOT__row;
    }

    // Advance until the character counter equals target (early in that
    // character: just after the CLKEN edge that loaded it).
    void run_to_c0(std::uint8_t target) {
        while (c0() != target) {
            run_clock_ticks(1);
        }
    }

    // Advance whole lines until C9 equals target mid-line, bounded so a DUT
    // whose stepping cannot reach the value (pre-F10 approximation) returns
    // false instead of hanging.  Leaves the bench early in the character
    // phase kTargetC0 of the reached line.
    static constexpr std::uint8_t kF10TargetC0 = 4;
    bool run_to_line_mid(std::uint8_t target, unsigned max_lines) {
        for (unsigned guard = 0; guard < max_lines; ++guard) {
            run_to_c0(kF10TargetC0);
            if (line_reg() == target) {
                return true;
            }
            run_characters(1);
        }
        return line_reg() == target;
    }

    // Advance whole lines until the frame origin (C4=C9=0) recurs, bounded.
    // Used to reach a known ParityFrame phase; leaves the bench early in
    // character kF10TargetC0 of the first line of the new frame.
    bool run_to_frame_start(unsigned max_lines) {
        // One line per iteration (R0=63 in every fixture that calls this):
        // advance before checking so the reset-time frame origin does not
        // trivially satisfy the predicate.
        for (unsigned guard = 0; guard < max_lines; ++guard) {
            run_characters(64);
            run_to_c0(kF10TargetC0);
            if (c4_reg() == 0 && line_reg() == 0) {
                return true;
            }
        }
        return false;
    }

    std::uint16_t ma() const {
        return dut_->MA;
    }

    std::uint8_t ra() const {
        return dut_->RA;
    }

    void expect_ma(const std::string& expectation, std::uint16_t expected) const {
        if (dut_->MA != expected) {
            fail(expectation + " == " + std::to_string(expected),
                 static_cast<unsigned>(dut_->MA));
        }
    }

    void expect_xfail_ma(const std::string& expectation,
                         std::uint16_t expected) const {
        if (dut_->MA != expected) {
            known_divergence(expectation + " == " + std::to_string(expected),
                             static_cast<unsigned>(dut_->MA));
        }
    }

    void expect_xfail_vsync_high(const std::string& expectation) const {
        if (dut_->VSYNC == 0) {
            known_divergence(expectation, "VSYNC low");
        }
    }

    void expect_xfail_vsync_low(const std::string& expectation) const {
        if (dut_->VSYNC != 0) {
            known_divergence(expectation, "VSYNC high");
        }
    }

    void expect_xfail_adjustment_active(const std::string& expectation) const {
        if (!dut_->rootp->CRTC__DOT__in_adj) {
            known_divergence(expectation, "adjustment inactive");
        }
    }

    void expect_xfail_adjustment_inactive(const std::string& expectation) const {
        if (dut_->rootp->CRTC__DOT__in_adj) {
            known_divergence(expectation, "adjustment active");
        }
    }

    void expect_de_high(const std::string& expectation) const {
        expect_high(expectation, dut_->DE);
    }

    void expect_de_low(const std::string& expectation) const {
        expect_low(expectation, dut_->DE);
    }

    void expect_reset_outputs() const {
        expect_low("HSYNC after reset", dut_->HSYNC);
        expect_low("VSYNC after reset", dut_->VSYNC);
        expect_low("DE after reset", dut_->DE);
        expect_low("FIELD after reset", dut_->FIELD);
        expect_low("CURSOR after reset", dut_->CURSOR);
        expect_low("RA after reset", dut_->RA);
    }

    void expect_idle_bus_high() const {
        expect_byte("DO while the CRTC bus is inactive", 0xff, dut_->DO);
    }

    std::string timestamp() const {
        std::ostringstream text;
        text << "character " << character_cycles_ << ", tick "
             << tick_in_character_ << '/' << kClockTicksPerCharacter
             << ", trace-time " << trace_time_;
        return text.str();
    }

private:
    void idle_bus() {
        dut_->ENABLE = 0;
        dut_->nCS = 1;
        dut_->R_nW = 1;
        dut_->RS = 0;
        dut_->DI = 0;
    }

    void bus_write(bool register_data, std::uint8_t value) {
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 0;
        dut_->RS = register_data ? 1 : 0;
        dut_->DI = value;
        clock_tick();
        idle_bus();
        eval_comb();
    }

    void clock_tick() {
        const unsigned active_phase = tick_in_character_;
        dut_->CLKEN = active_phase == kClkEnPhase;
        dut_->nCLKEN = active_phase == kNClkEnPhase;

        dut_->CLOCK = 0;
        eval_and_dump();
        dut_->CLOCK = 1;
        eval_and_dump();
        if (clken_sampler_ && active_phase == kClkEnPhase) {
            clken_sampler_();
        }
        dut_->CLOCK = 0;
        eval_and_dump();

        tick_in_character_ =
            (tick_in_character_ + 1) % kClockTicksPerCharacter;
        if (active_phase == kClkEnPhase) {
            ++character_cycles_;
            ++total_characters_;
        }
    }

    void eval_comb() {
        dut_->eval();
    }

    void eval_and_dump() {
        dut_->eval();
        if (trace_->isOpen()) {
            trace_->dump(trace_time_++);
        }
    }

    template <typename Actual>
    [[noreturn]] void fail(const std::string& expected, Actual actual) const {
        std::ostringstream text;
        text << timestamp() << ": expected " << expected << ", actual " << actual;
        throw TestFailure(text.str());
    }

    template <typename Actual>
    [[noreturn]] void known_divergence(const std::string& expected,
                                       Actual actual) const {
        std::ostringstream text;
        text << timestamp() << ": expected " << expected << ", actual " << actual;
        throw KnownDivergenceFailure(text.str());
    }

    std::unique_ptr<VerilatedContext> context_;
    std::unique_ptr<VCRTC> dut_;
    std::unique_ptr<VerilatedVcdC> trace_;
    std::function<void()> clken_sampler_;
    vluint64_t trace_time_ = 0;
    std::uint64_t character_cycles_ = 0;
    std::uint64_t total_characters_ = 0;
    unsigned tick_in_character_ = 0;
};

struct TestCase {
    std::string name;
    std::string source_rule;
    bool known_divergence;
    std::function<void(TestBench&)> run;
};

void test_reset_and_idle_bus(TestBench& test) {
    for (unsigned type = 0; type <= 1; ++type) {
        test.prepare_for_reset(type);
        test.expect_reset_outputs();
        test.expect_idle_bus_high();
    }
}

void test_register_readback_table(TestBench& test) {
    constexpr std::array<std::uint8_t, 16> written = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0xea, 0xeb, 0xd2, 0xa5, 0xde, 0xaf,
    };

    for (unsigned type = 0; type <= 1; ++type) {
        test.set_crtc_type(type);
        for (unsigned address = 0; address < written.size(); ++address) {
            test.write_register(address, written[address]);
        }

        std::array<std::uint8_t, 32> expected{};
        if (type == 0) {
            expected[12] = 0x12;  // Stored R12 is six bits wide.
            expected[13] = 0xa5;
        }
        expected[14] = 0x1e;      // Stored R14 is six bits wide.
        expected[15] = 0xaf;
        if (type == 1) {
            expected[31] = 0xff;
        }

        for (unsigned address = 0; address < expected.size(); ++address) {
            std::ostringstream label;
            label << "type " << type << " register R" << address << " readback";
            test.expect_byte(label.str(), expected[address],
                             test.read_register(address));
        }

        // Only the five low address bits are decoded on both CRTC types.
        test.expect_byte("R12 modulo-32 alias", expected[12],
                         test.read_register(0xac));
        test.expect_byte("R31 modulo-32 alias", expected[31],
                         test.read_register(0xff));
    }
}

constexpr unsigned kF3LineCharacters = 8;
constexpr unsigned kF3MidlineHcc = 3;
constexpr unsigned kVsyncLines = 16;

void configure_f3_midline_fixture(TestBench& test,
                                  unsigned type,
                                  unsigned vertical_sync_width = 2) {
    test.set_crtc_type(type);

    // Hold C4 at zero for the whole measurement. R7 starts unequal to C4 so
    // that only the explicitly timed R7=0 write can arm VSYNC.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, kF3LineCharacters - 1}, {1, 4}, {2, 5},
        {3, static_cast<std::uint8_t>((vertical_sync_width << 4) | 1)}, {4, 3},
        {5, 0},                     {6, 3}, {7, 1}, {8, 0},    {9, 31},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();
}

void write_r7_zero_at_hcc(TestBench& test, unsigned hcc) {
    // reset() leaves C0 at zero immediately before a CLKEN edge. Driving the
    // data write on that edge makes the old C0 value seen by the RTL exactly
    // `hcc`, without inspecting an internal counter.
    test.run_characters(hcc);
    test.expect_vsync_low("VSYNC before the timed R7 write");
    test.write_selected_register_at_clken(0);

    // VSYNC is deliberately registered once more at the output pin. One raw
    // 16 MHz tick exposes the R7 handler's immediate assertion there.
    test.run_clock_ticks(1);
}

void test_type0_r7_hcc_blocked(TestBench& test, unsigned hcc) {
    configure_f3_midline_fixture(test, 0);
    write_r7_zero_at_hcc(test, hcc);

    std::ostringstream immediate;
    immediate << "type 0 R7=C4 write at C0=" << hcc << " is blocked";
    test.expect_vsync_low(immediate.str());

    // A blocked comparison is consumed: remaining on C4=R7 must not produce a
    // delayed pulse on either of the following lines.
    test.run_characters(2 * kF3LineCharacters);
    test.expect_vsync_low("type 0 blocked R7 comparison remains consumed");
}

void test_type0_r7_hcc0_blocked(TestBench& test) {
    test_type0_r7_hcc_blocked(test, 0);
}

void test_type0_r7_hcc1_blocked(TestBench& test) {
    test_type0_r7_hcc_blocked(test, 1);
}

void test_type0_r7_midline_duration_extended(TestBench& test) {
    configure_f3_midline_fixture(test, 0);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 0 C0>1 R7 write asserts VSYNC immediately");

    // R3h=2. From C0=3, R0-C0=4 characters remain before the first line
    // boundary. Type 0 begins loading/counting C3h there, so two *complete*
    // lines must then elapse before VSYNC ends.
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.expect_vsync_high("type 0 VSYNC at its first post-write line boundary");
    test.run_characters(kF3LineCharacters);
    test.expect_vsync_high(
        "type 0 partial line does not consume either R3h line");
    test.run_characters(kF3LineCharacters);
    test.expect_vsync_low("type 0 extended VSYNC ends after two complete lines");
}

void test_type1_r7_early_hcc_immediate(TestBench& test) {
    for (const unsigned hcc : {0U, 1U}) {
        configure_f3_midline_fixture(test, 1);
        write_r7_zero_at_hcc(test, hcc);

        std::ostringstream expectation;
        expectation << "type 1 R7=C4 write at C0=" << hcc
                    << " asserts VSYNC immediately";
        test.expect_vsync_high(expectation.str());
    }
}

void test_type1_r7_midline_partial_counts(TestBench& test) {
    configure_f3_midline_fixture(test, 1);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 1 mid-line R7 write asserts VSYNC immediately");

    // Type 1 always uses a 16-line VSYNC. Its current partial line is count 1:
    // at C0=3 the pulse therefore ends after 4 + 15*8 = 124 characters, four
    // characters (C0+1) shorter than a line-aligned 16*8-character pulse.
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.run_characters((kVsyncLines - 2) * kF3LineCharacters);
    test.expect_vsync_high("type 1 VSYNC through its fifteenth line boundary");
    test.run_characters(kF3LineCharacters);
    test.expect_vsync_low("type 1 partial-line VSYNC at its sixteenth boundary");
}

void configure_vsync_reentrancy_fixture(TestBench& test,
                                        unsigned type,
                                        unsigned vertical_total,
                                        unsigned max_scanline);

void test_r7_level_write_and_active_rearm(TestBench& test) {
    constexpr unsigned line_characters = 4;
    constexpr unsigned held_characters = 2;

    for (unsigned type = 0; type <= 1; ++type) {
        configure_vsync_reentrancy_fixture(test, type, 0, 0);
        test.run_characters(line_characters);
        test.expect_vsync_high("initial VSYNC before held equal R7 write");

        test.hold_selected_register_at_clken(
            0, held_characters * kClockTicksPerCharacter);
        test.expect_vsync_high("held equal R7 write does not disturb active VSYNC");
        test.run_characters(kVsyncLines * line_characters - held_characters);
        test.expect_vsync_low("held equal R7 write remains consumed at VSYNC end");
        test.run_characters(2 * line_characters);
        test.expect_vsync_low("held equal R7 write cannot re-trigger while equal");

        configure_vsync_reentrancy_fixture(test, type, 1, 0);
        test.run_characters(2 * line_characters);
        test.expect_vsync_high("initial VSYNC before different R7 write");
        test.hold_selected_register_at_clken(
            1, held_characters * kClockTicksPerCharacter);
        test.expect_vsync_high("different R7 write does not cancel active VSYNC");
        test.run_characters(kVsyncLines * line_characters - held_characters);
        test.expect_vsync_low("active VSYNC completes after different R7 write");
        test.run_characters(line_characters);
        test.expect_vsync_high("different R7 write permits the next genuine match");
    }
}

void test_type0_dynamic_vsync_width_extremes(TestBench& test) {
    for (const unsigned programmed_width : {0U, 1U, 15U}) {
        const unsigned effective_width = programmed_width == 0 ? 16 : programmed_width;
        configure_f3_midline_fixture(test, 0, programmed_width);
        write_r7_zero_at_hcc(test, kF3MidlineHcc);
        test.expect_vsync_high("type 0 dynamic VSYNC starts for width extreme");

        test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
        test.expect_vsync_high("type 0 partial line is excluded for width extreme");
        if (effective_width > 1) {
            test.run_characters((effective_width - 1) * kF3LineCharacters);
            test.expect_vsync_high("type 0 VSYNC survives all but its final full line");
        }
        test.run_characters(kF3LineCharacters);
        test.expect_vsync_low("type 0 width extreme ends after complete lines");
    }
}

void test_type0_r7_c0_2_at_line_boundary(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 2}, {1, 1}, {2, 1}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 1}, {8, 0},    {9, 31},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();

    write_r7_zero_at_hcc(test, 2);
    test.expect_vsync_high("type 0 R7=C4 write at C0=2 is not blocked");
    test.run_characters(3);
    test.expect_vsync_low("C0=2/R0 count boundary consumes no partial-line skip");
}

void configure_f3_interlace_fixture(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 15}, {1, 8}, {2, 10}, {3, 0x11}, {4, 0},
        {5, 0},  {6, 1}, {7, 1},  {8, 3},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();
    test.run_characters(16);
    test.expect_field_low("interlace fixture reaches the half-line-count field");
}

void expect_interlace_dynamic_vsync_end(TestBench& test,
                                        unsigned write_hcc,
                                        unsigned characters_to_end) {
    configure_f3_interlace_fixture(test);
    write_r7_zero_at_hcc(test, write_hcc);
    test.expect_vsync_high("interlace-field R7=C4 write asserts VSYNC");
    test.run_characters(characters_to_end - 1);
    test.expect_vsync_high("interlace-field VSYNC remains high before final count tick");
    test.run_characters(1);
    test.expect_vsync_low("interlace-field VSYNC ends on the expected count tick");
}

void test_type0_interlace_count_boundaries(TestBench& test) {
    // In the second field with R0=15, the count tick sees old C0=6
    // (hcc_next=R0/2).  Before/on/after exercise the shared predicate; C0=15
    // proves that hcc_last itself is not a count tick in this field.
    expect_interlace_dynamic_vsync_end(test, 5, 26);
    expect_interlace_dynamic_vsync_end(test, 6, 25);
    expect_interlace_dynamic_vsync_end(test, 7, 31);
    expect_interlace_dynamic_vsync_end(test, 15, 23);
}

void test_type0_pending_skip_clears_on_type_roundtrip(TestBench& test) {
    configure_f3_midline_fixture(test, 0, 1);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 0 mid-line VSYNC has a pending first-line skip");

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.set_crtc_type(0);
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.expect_vsync_low("live type round-trip clears the type 0 pending skip");
}

void test_type0_pending_skip_clears_on_snapshot_load(TestBench& test) {
    configure_f3_midline_fixture(test, 0, 1);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 0 mid-line VSYNC before snapshot load");

    const std::array<std::uint8_t, 10> snapshot_registers = {{
        kF3LineCharacters - 1, 4, 5, 0x11, 3, 0, 3, 0, 0, 31,
    }};
    test.load_snapshot_registers(snapshot_registers);
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.expect_vsync_low("snapshot load clears the derived pending-line skip");
}

void configure_vsync_reentrancy_fixture(TestBench& test,
                                        unsigned type,
                                        unsigned vertical_total,
                                        unsigned max_scanline) {
    test.set_crtc_type(type);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x01}, {4, vertical_total},
        {5, 0}, {6, 1}, {7, 0}, {8, 0},    {9, max_scanline},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();
}

void test_vsync_compare_lock_and_rearm(TestBench& test) {
    constexpr unsigned line_characters = 4;

    for (unsigned type = 0; type <= 1; ++type) {
        configure_vsync_reentrancy_fixture(test, type, 0, 0);
        test.expect_vsync_low("VSYNC before the first C4=R7 comparison");

        test.run_characters(line_characters);
        test.expect_vsync_high("R7=0/R4=0 initial VSYNC");
        test.run_characters(kVsyncLines * line_characters);
        test.expect_vsync_low("R7=0/R4=0 VSYNC completes once");
        test.run_characters(2 * line_characters);
        test.expect_vsync_low("unchanged C4=R7 truth does not refire VSYNC");

        // Rewriting R7 away and back changes the comparison truth and re-arms
        // mechanism 2. Return to R7=0 at C0=2, outside type 0's blocked window.
        test.write_selected_register_at_clken(1);
        test.run_characters(1);
        test.write_selected_register_at_clken(0);
        test.run_clock_ticks(1);
        test.expect_vsync_high("R7 truth-value change re-arms VSYNC");
    }
}

void test_vsync_reentrancy_bypass(TestBench& test) {
    constexpr unsigned line_characters = 4;
    constexpr unsigned frame_lines = 16;  // (R4+1) * (R9+1) = 2 * 8.

    for (unsigned type = 0; type <= 1; ++type) {
        configure_vsync_reentrancy_fixture(test, type, 1, 7);

        test.run_characters((frame_lines - 1) * line_characters);
        test.expect_vsync_low("R7=0/R4=1 VSYNC before the frame boundary");
        test.run_characters(line_characters);
        test.expect_vsync_high("R7=0/R4=1 initial VSYNC at frame boundary");

        // During each 16-line pulse C4 visits 1 and returns to 0. That false-
        // then-true transition re-arms VSYNC exactly as C3h expires, yielding
        // the documented continuous/infinite raw CRTC VSYNC.
        for (unsigned character = 0;
             character < 3 * frame_lines * line_characters;
             ++character) {
            test.run_characters(1);
            test.expect_vsync_high(
                "R7=0/R4=1/R9=7 VSYNC remains continuous across retriggers");
        }
    }
}

void test_type1_status_r6_zero_forced_border(TestBench& test) {
    test.set_crtc_type(1);

    // Eight characters per line, one scanline per row, four rows per frame.
    // R6=3 leaves rows 0..2 displayed after the first complete frame.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(6);
    test.reset();

    test.run_characters(32);  // Complete the initial frame and enable display.
    test.run_characters(10);  // Enter row 1, away from C0=R0.
    test.expect_de_high("DE before R6=0 forced border");
    test.expect_byte("type 1 status in displayed row", 0x00, test.read_status());

    // ACCC v1.10 section 21.3.3: setting R6=0 while C4>0 forces border,
    // but that special case must not set the C4==R6 status condition.
    test.write_selected_register_at_nclken(0);
    test.expect_de_low("DE after R6=0 forced border");
    test.expect_byte("type 1 status after R6=0 forced border at C4>0",
                     0x00, test.read_status());

    test.run_characters(5);
    test.expect_byte("type 1 status after sampling R6=0 forced border",
                     0x00, test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 R6=0 status before frame origin", 0x00,
                     test.read_status());
    test.run_characters(1);
    test.expect_byte("type 1 R6=0 status at frame origin", 0x00,
                     test.read_status());
}

void test_type1_status_waits_for_r0_sample(TestBench& test) {
    test.set_crtc_type(1);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(6);
    test.reset();

    test.run_characters(32);  // Display becomes active at the frame boundary.
    test.run_characters(10);  // Row 1, several characters before C0=R0.
    test.expect_byte("type 1 status before an R6-border write", 0x00,
                     test.read_status());

    // Matching R6 to current C4 activates border immediately. Status bit 5 is
    // a separate latch and must remain unchanged until C0=R0 is sampled.
    test.write_selected_register_at_nclken(1);
    test.expect_byte("type 1 status before the C0=R0 sample", 0x00,
                     test.read_status());

    test.run_characters(4);
    test.expect_byte("type 1 status one character before the C0=R0 sample",
                     0x00, test.read_status());

    test.run_characters(1);
    test.expect_byte("type 1 status after the C0=R0 sample", 0x20,
                     test.read_status());
}

void test_type1_status_samples_natural_r6_edge(TestBench& test) {
    test.set_crtc_type(1);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 2}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    test.run_characters(32);  // Complete the initial frame and clear status.
    test.expect_byte("type 1 status at the start of a displayed frame", 0x00,
                     test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 status before entering the R6 row", 0x00,
                     test.read_status());

    test.run_characters(1);
    test.expect_byte("type 1 status when the R6 row is sampled", 0x20,
                     test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 high status before frame origin", 0x20,
                     test.read_status());
    test.run_characters(1);
    test.expect_byte("type 1 high status clears at frame origin", 0x00,
                     test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 status before the next R6 row", 0x00,
                     test.read_status());
    test.run_characters(1);
    test.expect_byte("type 1 status set again before reset", 0x20,
                     test.read_status());
    test.reset();
    test.expect_byte("type 1 high status clears on reset", 0x00,
                     test.read_status());

    test.set_crtc_type(0);
    test.expect_byte("type 0 status remains unchanged", 0xff,
                     test.read_status());
}

void test_type1_status_clears_on_type_round_trip(TestBench& test) {
    test.set_crtc_type(1);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 2}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    test.run_characters(48);
    test.expect_byte("type 1 status before type round-trip", 0x20,
                     test.read_status());

    test.set_crtc_type(0);
    test.run_characters(1);
    test.expect_byte("type 0 status during type round-trip", 0xff,
                     test.read_status());

    test.set_crtc_type(1);
    test.expect_byte("type 1 status after type round-trip", 0x00,
                     test.read_status());
}

void configure_f5_r0_zero_fixture(TestBench& test,
                                  unsigned type,
                                  std::uint8_t horizontal_sync_position) {
    test.set_crtc_type(type);

    // R9 is deliberately nonzero: type 0 must freeze C9/RA at zero while
    // R0=0, whereas type 1 must continue through one-character lines.  R2 is
    // varied independently to exercise the pin-level HSYNC consequence.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 0}, {1, 0}, {2, horizontal_sync_position}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2},                       {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();
    test.expect_reset_outputs();
}

void test_type0_r0_zero_suppresses_nonzero_r2_hsync(TestBench& test) {
    constexpr unsigned horizontal_sync_position = 4;
    constexpr unsigned observation_characters = 260;
    constexpr unsigned former_free_running_r2_tick =
        (horizontal_sync_position - 1) * kClockTicksPerCharacter + 1;

    configure_f5_r0_zero_fixture(test, 0, horizontal_sync_position);
    test.expect_ra("type 0 R0=0 initial raster counter", 0);

    // ACCC v1.10 section 13.2.1: C0 remains pinned at zero, so it can never
    // reach a nonzero R2.  Sample every raw CLOCK tick, not merely character
    // boundaries, so a sub-character HSYNC pulse cannot escape the vector.
    for (unsigned tick = 0;
         tick < observation_characters * kClockTicksPerCharacter;
         ++tick) {
        test.run_clock_ticks(1);
        if ((tick + 1) % kClockTicksPerCharacter == 0) {
            test.expect_ra("type 0 R0=0 keeps C9/RA frozen", 0);
        }
        if (tick == former_free_running_r2_tick) {
            test.expect_hsync_low(
                "type 0 R0=0 suppresses the former free-running R2 edge");
        } else {
            test.expect_hsync_low("type 0 R0=0 cannot reach nonzero R2");
        }
    }
}

void test_type0_r0_zero_allows_r2_zero_hsync(TestBench& test) {
    configure_f5_r0_zero_fixture(test, 0, 0);

    // ACCC v1.10 section 15.3: type 0 does not restart HSYNC on the second
    // C0=R2=0 occurrence.  Its stopped C3l then permits a restart on the
    // third occurrence.  Sample every raw CLOCK tick so the brief low window
    // cannot be hidden by character-boundary observations.
    for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
        test.run_clock_ticks(1);
        test.expect_hsync_high(
            "type 0 first R2=0 occurrence keeps HSYNC high");
        test.expect_ra("type 0 R2=0 first occurrence keeps C9/RA frozen", 0);
    }

    test.run_clock_ticks(1);
    test.expect_hsync_high(
        "type 0 second R2=0 occurrence does not restart HSYNC");
    test.expect_ra("type 0 R2=0 second occurrence keeps C9/RA frozen", 0);
    for (unsigned tick = 1; tick < kClockTicksPerCharacter; ++tick) {
        test.run_clock_ticks(1);
        test.expect_hsync_low(
            "type 0 HSYNC stays off after the second R2=0 occurrence");
        test.expect_ra("type 0 R2=0 second occurrence keeps C9/RA frozen", 0);
    }

    test.run_clock_ticks(1);
    test.expect_hsync_low(
        "type 0 third R2=0 occurrence first clears stopped C3l");
    test.expect_ra("type 0 R2=0 third occurrence keeps C9/RA frozen", 0);
    for (unsigned tick = 1; tick < kClockTicksPerCharacter; ++tick) {
        test.run_clock_ticks(1);
        if (tick == 1) {
            test.expect_hsync_high(
                "type 0 third R2=0 occurrence restarts HSYNC");
        } else {
            test.expect_hsync_high(
                "type 0 third R2=0 occurrence keeps HSYNC high");
        }
        test.expect_ra("type 0 R2=0 third occurrence keeps C9/RA frozen", 0);
    }
}

void test_type0_r0_zero_resumes_after_nclken_write(TestBench& test) {
    constexpr unsigned horizontal_sync_position = 2;
    configure_f5_r0_zero_fixture(test, 0, horizontal_sync_position);

    // Prove the recovered, in-range R2 has not been reached while R0=0.  The
    // first released CLKEN is included; HSYNC must remain low through nCLKEN.
    for (unsigned tick = 0; tick < kNClkEnPhase; ++tick) {
        test.expect_hsync_low("type 0 R0=0 keeps recovered R2 out of range");
        test.run_clock_ticks(1);
        test.expect_ra("type 0 raster counter during R0=0 stall", 0);
    }

    // nCLKEN is tick 8, opposite CLKEN at tick 0.  Landing R0=3 there makes
    // the new total stable before the next character edge.
    test.write_selected_register_at_nclken(3);
    test.expect_ra("type 0 raster counter at R0 recovery write", 0);
    test.expect_hsync_low("type 0 HSYNC remains low at the recovery write");

    // The first post-write CLKEN resumes frozen C0 as 1, not 2.
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.expect_hsync_low("type 0 recovery remains below R2 before CLKEN");
    test.run_clock_ticks(1);
    test.expect_hsync_low("type 0 recovery advances C0 from zero to one");
    test.run_clock_ticks(1);
    test.expect_hsync_low(
        "type 0 recovery does not assert HSYNC before C0=R2");

    // On the second post-write CLKEN C0 reaches R2=2.  HSYNC registers on
    // the following raw CLOCK edge.
    test.run_clock_ticks(kClockTicksPerCharacter - 2);
    test.expect_hsync_low("type 0 recovery remains below R2 until CLKEN");
    test.run_clock_ticks(1);
    test.expect_hsync_low("type 0 recovery reaches R2 before HSYNC registers");
    test.run_clock_ticks(1);
    test.expect_hsync_high("type 0 recovery asserts HSYNC exactly at C0=R2");

    // From C0=R2, two further CLKENs reach C0=R0 and then wrap to zero.
    test.run_clock_ticks(2 * kClockTicksPerCharacter - 2);
    test.expect_ra("type 0 recovery before the first widened line end", 0);
    test.run_clock_ticks(1);
    test.expect_ra("type 0 recovery advances C9/RA after one R0=3 line", 1);
}

void test_type1_r0_zero_keeps_one_character_lines(TestBench& test) {
    configure_f5_r0_zero_fixture(test, 1, 4);

    // Type 1 has no R0=0 freeze: tick 0 completes each one-character line,
    // while C0=0 still cannot reach the nonzero R2 value.  Sample every raw
    // CLOCK tick so a sub-character HSYNC pulse cannot escape the guard.
    for (std::uint8_t raster = 1; raster <= 4; ++raster) {
        for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
            test.run_clock_ticks(1);
            test.expect_hsync_low("type 1 R0=0 cannot reach nonzero R2");
            if (tick == kClkEnPhase) {
                test.expect_ra(
                    "type 1 R0=0 advances C9/RA at each CLKEN", raster);
            }
        }
    }

    // The helper reaches nCLKEN by first executing the documented tick-0
    // CLKEN, completing one last R0=0 line (RA 4->5).  R0=3 is then stable
    // before subsequent CLKENs and produces a four-character line.
    test.write_selected_register_at_nclken(3);
    test.expect_ra("type 1 final one-character line before R0 widening", 5);
    test.run_characters(3);
    test.expect_ra("type 1 widened line before C0 reaches R0", 5);
    test.run_characters(1);
    test.expect_ra("type 1 widened line advances C9/RA at C0=R0", 6);
}

void test_type0_midline_r0_zero_free_runs_then_pins(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 0}, {2, 4}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();
    test.expect_reset_outputs();

    // Two complete characters leave C0=2.  The helper executes the next
    // CLKEN before reaching nCLKEN, so R0 becomes zero live at C0=3.
    test.run_characters(2);
    test.write_selected_register_at_nclken(0);
    const std::uint16_t ma_at_write = test.ma();
    const auto advanced_ma = [ma_at_write](unsigned characters) {
        return static_cast<std::uint16_t>(
            (ma_at_write + characters) & 0x3fff);
    };
    test.expect_ra("type 0 mid-line R0=0 write does not create a line", 0);

    // The live comparator cannot match zero while C0 is nonzero.  C0 must
    // continue to increment rather than clamp immediately; reaching R2=4 and
    // advancing MA on the first following CLKEN makes that externally visible.
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.expect_ma("type 0 MA holds until the first post-write CLKEN",
                   ma_at_write);
    test.run_clock_ticks(1);
    test.expect_ma("type 0 mid-line R0=0 continues from C0=3 to C0=4",
                   advanced_ma(1));
    test.expect_ra("type 0 free-run does not create a line at C0=4", 0);
    test.run_clock_ticks(1);
    test.expect_hsync_high("type 0 free-run still reaches the live R2=4");

    // From C0=4, another 251 character clocks reach C0=255, and the next
    // wraps the eight-bit counter to zero.  None is a true C0=R0 line end.
    test.run_characters(251);
    test.expect_ma("type 0 free-run advances MA through C0=255",
                   advanced_ma(252));
    test.expect_ra("type 0 free-run through C0=255 creates no false line", 0);
    test.run_characters(1);
    test.expect_ma("type 0 C0 overflow advances MA once before pinning",
                   advanced_ma(253));
    test.expect_ra("type 0 C0 overflow creates no false line", 0);

    // Once the overflow has produced C0=0, the repeated C0=R0 equality pins
    // C0 and must not reload/increment the visible memory address.
    test.run_characters(3);
    test.expect_ma("type 0 R0=0 pins MA after the eight-bit wrap",
                   advanced_ma(253));
    test.expect_ra("type 0 R0=0 pins C9 after the eight-bit wrap", 0);

    // Widen R0 at nCLKEN.  C0 resumes from zero, MA advances for C0=1..3,
    // and the first genuine C0=R0 boundary advances C9 exactly once.
    test.write_selected_register_at_nclken(3);
    test.expect_ma("type 0 MA remains pinned at the recovery write",
                   advanced_ma(253));
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.expect_ma("type 0 recovered MA waits for CLKEN", advanced_ma(253));
    test.run_clock_ticks(1);
    test.expect_ma("type 0 recovery resumes MA from frozen C0=0",
                   advanced_ma(254));
    test.run_characters(2);
    test.expect_ma("type 0 recovery advances MA through C0=3",
                   advanced_ma(256));
    test.expect_ra("type 0 recovery has not ended the widened line early", 0);
    test.run_characters(1);
    test.expect_ra("type 0 recovery ends one complete R0=3 line", 1);
}

void test_r0_zero_freeze_survives_type_round_trip(TestBench& test) {
    configure_f5_r0_zero_fixture(test, 0, 4);

    test.run_characters(2);
    test.expect_ra("type 0 begins the type round-trip frozen", 0);

    // CRTC_TYPE is a live input.  Type 1 treats R0=0 as one-character lines,
    // then returning to type 0 at C0=0 must immediately restore the freeze.
    test.set_crtc_type(1);
    test.run_characters(2);
    test.expect_ra("type 1 advances two R0=0 lines during round-trip", 2);
    test.set_crtc_type(0);
    test.run_characters(3);
    test.expect_ra("type 0 re-pins C9 after the type round-trip", 2);

    test.write_selected_register_at_nclken(3);
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.run_clock_ticks(1);
    test.expect_ra("round-trip recovery starts from the frozen raster", 2);
    test.run_characters(3);
    test.expect_ra("round-trip recovery completes one widened line", 3);
}

void test_type0_interlace_r0_zero_freezes_vsync_count(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 15}, {1, 8}, {2, 10}, {3, 0x11}, {4, 0},
        {5, 0},  {6, 1}, {7, 1},  {8, 3},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();

    // Start a one-count VSYNC in field=0 at C0=3.  Its partial-line guard is
    // consumed by the genuine C0=15->0 line tick, which also enters field=1.
    // Thus C3h=0 while VSYNC remains active at C0=0 in the half-line field.
    write_r7_zero_at_hcc(test, 3);
    test.expect_vsync_high("interlaced VSYNC starts before the R0=0 freeze");
    test.run_characters(12);
    test.expect_vsync_high("interlaced VSYNC reaches the next line active");
    test.expect_field_low("interlaced VSYNC remains in the half-line field");

    // Land R0=0 at nCLKEN while C0 is already zero.  A frozen type-0 C0 must
    // not masquerade as the field=1 half-line predicate on every CLKEN.
    test.select_register(0);
    test.write_selected_register_at_nclken(0);
    test.run_characters(3);
    test.expect_vsync_high(
        "type 0 interlaced R0=0 freeze does not consume C3h character-wise");
    // RA is the split C9.VMA now (F10, section 19.8.1): C9 frozen at 0 and
    // ParityC9 seeded from ParityFrame, which this register set freezes at
    // 0 because R6=1 > R4=0 (section 19.5.2 p.205: ParityR6 stops updating
    // when C4 never reaches R6, so every frame stays even).  The pre-F10
    // approximation OR'd the field toggle into bit 0 and expected 1; the
    // documented value for this configuration is 0.  The assertion's intent
    // -- the raster value holds constant through the freeze -- is unchanged.
    test.expect_ra("type 0 interlaced R0=0 keeps the raster frozen", 0);

    // Widening R0 restores the real half-line predicate.  With R0=7 it lands
    // at C0=2->3: no earlier character may end VSYNC, and that count ends it.
    test.write_selected_register_at_nclken(7);
    test.run_characters(2);
    test.expect_vsync_high("recovered interlaced VSYNC waits for R0/2");
    test.run_characters(1);
    test.expect_vsync_low("recovered interlaced VSYNC ends at R0/2");
}

void test_type0_r0_zero_c9_equal_single_c4_increment_deferred(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC 13.2.1's narrow deferred subcase: when C9 already equals R9 as
    // type 0 enters R0=0, the previously armed decision increments C4 once
    // on the second C0=0 occurrence, then freezes.  The first normal R0=3
    // line below reaches C4=1/C9=0; R0 is then changed at that exact C0=0.
    // R7=2 makes the deferred increment observable after recovery: a correct
    // frozen C4=2 must not start VSYNC on the first widened line, whereas the
    // current all-state freeze leaves C4=1 and produces a false 1->2 match.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 0}, {2, 2}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();

    // Stop immediately after the C0=3->0 boundary rather than running the
    // remainder of the character.  The raw phase-1 bus write changes R0 at
    // C0=0 without introducing another CLKEN.
    test.run_characters(3);
    test.run_clock_ticks(1);
    test.write_selected_register_now(0);
    test.run_characters(2);
    test.expect_vsync_low("deferred C9=R9 subcase stays quiet while frozen");
    test.write_selected_register_at_nclken(3);
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.run_clock_ticks(1);
    test.run_characters(3);
    test.run_clock_ticks(1);
    test.expect_vsync_low(
        "deferred C9=R9 entry increments C4 once before recovery");
}

void test_type0_adjustment_r4_write_switches_c9_to_r5(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.2: once the last-line state is established,
    // changing R4 from C0=2 through C0=R0 switches the line-end comparator
    // from C9/R9 to C9/R5.  With C4=R4=2, C9=R9=3, and R5=5, the next line
    // must therefore be C4=2/C9=4 in vertical adjustment, not C4=3/C9=0.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned lines_before_last = 11;
    test.run_characters(lines_before_last * line_characters);
    test.run_characters(2);  // Last-line state is now established; C0=2.
    test.write_selected_register_at_clken(1);
    test.run_characters(5);  // Complete C0=3..7 and enter the next line.

    test.expect_adjustment_active("type 0 R4 write enters vertical adjustment");
    test.expect_c4("type 0 R4 write keeps C4 on the entry line", 2);
    test.expect_ra("type 0 R4 write compares C9 with R5", 4);
}

void test_type0_adjustment_r9_write_uses_new_r9(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.2: an R9 write from C0=2 through
    // C0=R0-1 is compared with C9 on the current last-frame line. Here the
    // new R9=1 differs from C9=0, so C9 increments while C4 stays at zero.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 4}, {1, 3}, {2, 3}, {3, 0x11}, {4, 0},
        {5, 2}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(2);
    test.write_selected_register_at_clken(1);
    test.run_characters(2);

    test.expect_adjustment_active("type 0 R9 write retains vertical adjustment");
    test.expect_c4("type 0 R9 write does not increment C4", 0);
    test.expect_ra("type 0 R9 write increments C9 against the new R9", 1);
}

void test_type0_adjustment_accepts_r5_write_at_c0_2(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1 and 11.2.2: C0=2 is the final
    // arbitration point. An R5 write from zero to one there must select one
    // adjustment line, reset C9, and increment C4 on the following line.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    test.run_characters(2);
    test.write_selected_register_at_clken(1);
    test.run_characters(1);

    test.expect_adjustment_active("type 0 accepts R5>0 at C0=2");
    test.expect_c4("type 0 accepted R5 write increments C4", 1);
    test.expect_ra("type 0 accepted R5 write resets C9", 0);
}

void test_type0_adjustment_rejects_r5_write_after_c0_2(TestBench& test) {
    test.set_crtc_type(0);

    // The same v1.10 arbitration window closes once C0 has reached 3.
    // A later R5 write must not turn the already-decided frame boundary into
    // an adjustment line.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 4}, {1, 3}, {2, 3}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    test.run_characters(3);
    test.write_selected_register_at_clken(1);
    test.run_characters(1);

    test.expect_adjustment_inactive("type 0 rejects R5>0 after C0=2");
    test.expect_c4("type 0 late R5 write completes the frame", 0);
    test.expect_ra("type 0 late R5 write completes C9", 0);
}

void test_type0_adjustment_r9_write_at_r0_increments_c4_and_c9(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.2 exact-boundary case: the old C9/R9
    // equality increments C4 first. C4 then differs from R4, switching C9
    // to the R5 comparison, so C9 increments as well.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned lines_before_last = 11;
    test.run_characters(lines_before_last * line_characters);
    test.run_characters(7);
    test.write_selected_register_at_clken(4);

    test.expect_adjustment_active("type 0 exact-R0 R9 write enters adjustment");
    test.expect_c4("type 0 exact-R0 R9 write increments C4", 3);
    test.expect_ra("type 0 exact-R0 R9 write increments C9 against R5", 4);
}

void test_type0_adjustment_completion_resets_the_next_line(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(7);
    test.write_selected_register_at_clken(4);
    test.run_characters(8);

    // The exact-R0 split leaves C9=R5-1. The following line completes the
    // R5 count, re-establishes Last Line, and resets C4/C9 for the new frame.
    test.expect_adjustment_inactive("type 0 R5 completion leaves adjustment");
    test.expect_c4("type 0 R5 completion resets C4 on the following line", 0);
    test.expect_ra("type 0 R5 completion resets C9 on the following line", 0);
}

void test_type0_adjustment_captures_mid_character_r4_write(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(1);  // C0=1 at tick 0.
    test.write_selected_register_at_nclken(1);  // R4 write within C0=2.
    test.run_characters(6);

    test.expect_adjustment_active("type 0 captures an R4 write within C0=2");
    test.expect_c4("type 0 mid-character R4 write keeps C4", 2);
    test.expect_ra("type 0 mid-character R4 write compares C9 with R5", 4);
}

void test_type0_adjustment_captures_mid_character_r9_write_at_r0(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(6);  // C0=6 at tick 0.
    test.write_selected_register_at_nclken(4);  // R9 write within C0=R0.
    test.run_characters(1);

    test.expect_adjustment_active("type 0 captures an exact-R0 R9 bus write");
    test.expect_c4("type 0 mid-character exact-R0 R9 write increments C4", 3);
    test.expect_ra("type 0 mid-character exact-R0 R9 write increments C9", 4);
}

// ---------------------------------------------------------------------------
// t12: the documented R4=38/R9=7 worked example pair (ACCC v1.10 section
// 11.2.2, p.82 example 3; section 10.3.1, p.76). Two writes land on the same
// last line of the frame and must leave different counter states:
//   - an R9 write exactly at C0==R0 straddles the comparator switch: the old
//     C9/R9 match increments C4 first, then the changed C4/R4 result switches
//     C9 to the R5 comparison and C9 also increments -> C4==39, C9==8;
//   - an R9 write inside the C0 in [2, R0-1] window is consumed by the live
//     new-R9 comparison instead, so C4 is not yet incremented -> C4==38,
//     C9==8 (findings-review.md B4 companion case).
// Frame geometry: 38 full rows x 8 scanlines + the row-38 scanlines C9=0..6
// = 311 lines of 64 characters before the critical scanline. R7 is parked
// beyond any reachable C4 so VSYNC cannot disturb the sampling.
// ---------------------------------------------------------------------------
constexpr unsigned kT12CriticalLineCharacters = 311u * 64u;

void test_type0_worked_example_exact_r0_yields_39_8(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 38},
        {5, 16}, {6, 25}, {7, 63}, {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(kT12CriticalLineCharacters);
    test.run_characters(63);  // Enter character C0=R0 of the critical scanline.
    test.write_selected_register_at_clken(8);

    // ACCC v1.10 section 11.2.2, p.82: "we end up with C4==39 and C9==8."
    test.expect_adjustment_active("t12 exact-R0 write enters adjustment");
    test.expect_c4("t12 exact-R0 straddle leaves C4=39", 39);
    test.expect_ra("t12 exact-R0 straddle leaves C9=8", 8);
}

void test_type0_worked_example_window_write_yields_38_8(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 38},
        {5, 16}, {6, 25}, {7, 63}, {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(kT12CriticalLineCharacters + 10);  // C0=10, window open.
    test.write_selected_register_at_clken(8);
    test.run_characters(64 - 11);  // Consume characters 11..63 to the rollover.

    // ACCC v1.10 section 11.2.2, p.82 example 3: the windowed write leaves
    // C4 un-incremented; only C9 advances under the new-R9 comparison.
    test.expect_adjustment_active("t12 windowed write enters adjustment");
    test.expect_c4("t12 windowed write keeps C4=38", 38);
    test.expect_ra("t12 windowed write leaves C9=8", 8);
}

// ---------------------------------------------------------------------------
// t25: type-0 vertical-adjustment VRAM addressing -- the D1 p.81 correction
// pinned at pin level (ACCC v1.10 section 11.2.1 p.81 table, render-verified
// 2026-08-24; section 11.2.2 pp.81-83; section 20.2 p.241).
//
// Paper derivation against the p.81 worked example (R4=10, R5=16, R9=3,
// R1=40, R0=63):
//   - During type-0 adjustment C9's limit is R5, not R9 (section 11.2.2
//     p.81: "The new limit of C9 is no longer R9 at the end of the line,
//     but R5"), so C9 runs 0..15 across the sixteen adjustment lines and
//     never wraps at R9=3.
//   - The final VRAM address takes bits 13:11 from C9[2:0] (section 20.2
//     p.241: "Bits 11 to 13 From bits 0 to 2 of C9"; the motherboard
//     composes {MA[13:12], RA[2:0], MA[9:0]}), so the p.81 LINE column --
//     &0000,&0800,&1000,&1800,&2000,&2800,&3000,&3800, then &0000 again at
//     C9=8 -- is a period-8 cycle through eight distinct segments, one per
//     adjustment line.
//   - The PTR-VRAM column tracks the video pointer separately: constant
//     across runs of adjustment lines, advancing by exactly R1=40 words
//     across the single C0=R1 && C9==R9 crossing at line C9=3 (section
//     11.2.2 pp.82-83: "C9 continues to be compared with R9 to consider
//     the video pointer (VMA'=VMA) when C0=R1 and C9=R9"; p.83: "When C9
//     reaches R9 (=3), then the video pointer is updated with the one that
//     has been memorized when C0=R1 and C9=R9. (R1=40)"). A memorized value
//     of R1 words requires the pointer to scan per character inside the
//     adjustment line itself: DRAWN for the capture line by that p.83
//     sentence, extended to the other adjustment lines as the same uniform
//     mechanism (INFERRED; the type-1/2 tables' +40-per-character-row
//     progression corroborates it).
//   - Recorded boundary (NOT PINNED by the source): the tables normalize
//     PTR-VRAM to 0 at adjustment entry, so they cannot distinguish an
//     entry value of the last-row base from base+R1 (an entry-line capture
//     applied). This core keeps the plain-rule entry-line capture; only
//     source-supported deltas are asserted below.
// ---------------------------------------------------------------------------

// Final VRAM word address per the ACCC v1.10 section 20.2 p.241
// construction, as wired on the CPC board and modelled by
// Amstrad_motherboard.v: {MA[13:12], RA[2:0], MA[9:0]}.
std::uint16_t composed_vram_word(std::uint16_t ma, std::uint8_t ra) {
    return static_cast<std::uint16_t>(((ma & 0x3000) |
                                       ((ra & 0x07) << 11) |
                                       (ma & 0x03ff)) & 0x7fff);
}

constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 10> kT25Registers =
    {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 10},
        {5, 16}, {6, 2},  {7, 63}, {8, 0},    {9, 3},
    }};

// Eleven character rows x four scanlines (R9=3) x 64 characters bring the
// seam of the first adjustment line (C4=11, C9=0).
constexpr unsigned kT25AdjustmentStartCharacters = 11u * 4u * 64u;

void t25_program_and_reach_first_adjustment_line(TestBench& test) {
    test.set_crtc_type(0);
    for (const auto& [address, value] : kT25Registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();
    // One character into adjustment line C9=0.
    test.run_characters(kT25AdjustmentStartCharacters + 1u);
}

void test_type0_adjustment_segment_cycles_period_8(TestBench& test) {
    t25_program_and_reach_first_adjustment_line(test);

    for (unsigned c9 = 0; c9 < 16; ++c9) {
        if (c9 != 0) {
            test.run_characters(64);  // C0=1 of adjustment line C9.
        }
        test.expect_ra("t25a adjustment line C9 counts 0..15 against R5",
                       c9);
        // ACCC v1.10 section 11.2.1 p.81 LINE column via the section 20.2
        // p.241 bit assignment: segment = C9 mod 8, period 8, wrap at C9=8.
        const std::uint16_t composed =
            composed_vram_word(test.ma(), test.ra());
        test.expect_byte("t25a adjustment segment follows C9[2:0] (p.81 LINE)",
                         static_cast<std::uint8_t>(c9 & 0x07),
                         static_cast<std::uint8_t>(composed >> 11));
        if (c9 == 0) {
            // Observability caveat, pinned so it stays true: the adjustment
            // lines sit where DISPTMG is long off (R6=2 < C4=11), so this is
            // address-level accuracy, not a visible-display claim.
            test.expect_de_low("t25a adjustment lines have DE off");
        }
    }
}

void test_type0_adjustment_pointer_steps_and_scans(TestBench& test) {
    t25_program_and_reach_first_adjustment_line(test);

    std::uint16_t line_start[16];
    line_start[0] = test.ma();
    for (unsigned c9 = 1; c9 < 16; ++c9) {
        if (c9 == 4) {
            // Entering this iteration we sit at C0=1 of line C9=3, the
            // capture line.  The p.83 prose makes the memorized value the
            // pointer scanned to C0=R1: 40 words past the line start.
            test.run_characters(40);  // C0=41 of line C9=3.
            test.expect_ma("t25b adjustment pointer scans one word per "
                           "character (p.83 memorized-at-C0=R1)",
                           static_cast<std::uint16_t>(
                               (line_start[3] + 40) & 0x3fff));
            test.run_characters(24);  // C0=1 of line C9=4.
        }
        else {
            test.run_characters(64);  // C0=1 of adjustment line C9.
        }
        line_start[c9] = test.ma();
    }

    // ACCC v1.10 p.81 PTR-VRAM rows 1-4: no capture crossing before the
    // C9==R9 line, so lines C9=0..3 restart from the same pointer.
    for (unsigned c9 = 1; c9 < 4; ++c9) {
        test.expect_byte(
            "t25b adjustment lines before the crossing share one pointer "
            "(p.81 PTR-VRAM 0)",
            static_cast<std::uint8_t>(line_start[0] & 0xff),
            static_cast<std::uint8_t>(line_start[c9] & 0xff));
        test.expect_byte(
            "t25b adjustment lines before the crossing share one pointer, "
            "high byte (p.81 PTR-VRAM 0)",
            static_cast<std::uint8_t>((line_start[0] >> 8) & 0x3f),
            static_cast<std::uint8_t>((line_start[c9] >> 8) & 0x3f));
    }
    // p.81 PTR-VRAM rows 4->5 and p.83 prose: the single C0=R1 && C9==R9
    // crossing at line C9=3 advances the pointer by exactly R1=40 words.
    const std::uint16_t stepped =
        static_cast<std::uint16_t>((line_start[3] + 40) & 0x3fff);
    test.expect_byte("t25b capture crossing advances the pointer by R1 "
                     "(p.83)",
                     static_cast<std::uint8_t>(stepped & 0xff),
                     static_cast<std::uint8_t>(line_start[4] & 0xff));
    test.expect_byte("t25b capture crossing advances the pointer by R1, "
                     "high byte (p.83)",
                     static_cast<std::uint8_t>((stepped >> 8) & 0x3f),
                     static_cast<std::uint8_t>((line_start[4] >> 8) & 0x3f));
    // p.81 PTR-VRAM rows 5-16 stay at 40: C9 passes R9 once per adjustment,
    // so no further capture fires.
    for (unsigned c9 = 5; c9 < 16; ++c9) {
        test.expect_byte(
            "t25b adjustment lines after the crossing share one pointer "
            "(p.81 PTR-VRAM 40)",
            static_cast<std::uint8_t>(line_start[4] & 0xff),
            static_cast<std::uint8_t>(line_start[c9] & 0xff));
        test.expect_byte(
            "t25b adjustment lines after the crossing share one pointer, "
            "high byte (p.81 PTR-VRAM 40)",
            static_cast<std::uint8_t>((line_start[4] >> 8) & 0x3f),
            static_cast<std::uint8_t>((line_start[c9] >> 8) & 0x3f));
    }
}

void test_type0_adjustment_exit_reloads_frame_origin(TestBench& test) {
    t25_program_and_reach_first_adjustment_line(test);
    test.run_characters(15u * 64u);  // C0=1 of the last adjustment line.
    test.expect_ra("t25c last adjustment line holds C9=15", 15);

    // Section 11.2.2 p.81: reaching R5 re-establishes Last Line "so that C4
    // and C9 go to 0 on the next line", and section 20.3.1 p.242 reloads
    // both pointers from R12/R13 when C4=C9=C0=0. Note the page tensions
    // itself: its bold conclusion says only "C4=0 and C0=0" (no C9 term,
    // i.e. every line of C4=0). This core implements the narrow frame-
    // origin form (rtl/crtc_type0_engine.v reload = frame_new_w), reading
    // the page's own framing of the type-1 "while C4=0" rule as the lenient
    // outlier; the adjustment exit here only exercises the frame-origin
    // edge, so both readings agree on everything this vector asserts.
    test.run_characters(64);  // C0=1 of the new frame's first line.
    test.expect_c4("t25c adjustment exit resets C4 to 0", 0);
    test.expect_ra("t25c adjustment exit resets C9 to 0", 0);
    test.expect_ma("t25c frame origin reloaded from R12/R13 then scanned "
                   "once (section 20.3.1)", 1);
    const std::uint16_t composed =
        composed_vram_word(test.ma(), test.ra());
    test.expect_byte("t25c new frame restarts at the R12/R13 segment", 0,
                     static_cast<std::uint8_t>(composed >> 11));
}

// ---------------------------------------------------------------------------
// t26: the section 17.5 R1=0 acknowledgment deadline (ACCC v1.10 section
// 17.5.1 p.185, four chronograms, render-verified 2026-08-24; D1
// correction). The chronograms put the effective write cycle (the OUT's
// register-latch character, the orange end cell) at C0=3e, 3f, 0, 1 in
// turn on a line crossing an R0 wrap (the chronogram shows no frame
// involvement; the seam mechanism is line-generic, and the probes below
// cover both a plain mid-frame seam and a frame origin): latches through C0=0 are
// "just in time" (the new frame's first line honors R1=0 and shows
// border); the first too-late latch is C0=1 ("Update of R1 not
// considered" -- the line keeps the old R1's display).
//
// Paper-derived RTL mapping (both mechanisms already in the wrapper):
//   - latch during the old frame's characters: the line-seam DISPTMG check
//     compares hcc_next(=0) against the already-updated R1 and keeps
//     DISPTMG off from C0=0 (live C0=R1 semantics, section 6.1.3);
//   - latch during C0=0 itself: the R1 write-hit term (hcc==DI) blanks
//     DISPTMG mid-character -- character-granular DE shows border from
//     within C0=0 (the F13 half-character caveat applies);
//   - latch during C0=1: neither term fires (hcc==1 != 0, and the seam
//     check already ran with the old R1), so the old R1 governs the line
//     and DISPTMG ends at C0=old R1.
// The type 3/4 earlier deadline (section 17.5.2) is out of scope: the
// classic core models no type 3/4. The chronograms' observable is the
// DISPTMG/border state of the new frame's first characters; the VMA'
// consequences of R1=0 (section 17.4.3) are CRTC-2-specific and unpinned
// here.
// Fixture: R4=2/R9=0 frames of three 64-character lines; R6/R7 parked so
// DISPTMG is R1-gated only. Each vector probes all three latch positions
// around successive frame boundaries.
// ---------------------------------------------------------------------------
constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 10> kT26Registers =
    {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 2},
        {5, 0},  {6, 25}, {7, 63}, {8, 0},    {9, 0},
    }};

void t26_program(TestBench& test, unsigned type) {
    test.set_crtc_type(type);
    for (const auto& [address, value] : kT26Registers) {
        test.write_register(address, value);
    }
    test.select_register(1);
    test.reset();
}

void test_t26_r1_zero_deadline_type0(TestBench& test) {
    t26_program(test, 0);

    // vde only arms at a frame origin, so run one full frame first: every
    // probe below then runs with DISPTMG gated by R1 alone (DE == hde).
    test.run_characters(3u * 64u);  // C0=0 of frame 1, row 0.

    // The p.185 chronograms draw an R0 wrap, not a frame boundary; the seam
    // mechanism is line-generic. Probe a plain mid-frame line seam first:
    // the write lands during row 1's C0=63 and row 2 (same frame) starts in
    // border.
    test.run_characters(64);  // C0=0 of frame 1 row 1.
    test.run_to_c0(63);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(0);        // Row 2 origin, same frame.
    test.expect_de_low("t26a latch at C0=63 before a plain line seam: "
                       "border from C0=0 (section 17.5.1 is seam-generic)");

    // (A) Just in time, latch during the frame's last line at C0=63: the
    // frame-boundary seam compares hcc_next(=0) against the already-updated
    // R1=0 and keeps DISPTMG off from the new frame's first character.
    test.run_to_c0(10);
    test.write_selected_register_at_nclken(40);  // Restore for scenario A.
    test.run_to_c0(63);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(0);              // Frame 2 origin.
    test.expect_de_low("t26a latch at C0=63: seam sees R1=0, border from "
                       "C0=0 (section 17.5.1 just in time)");

    // (B) Just in time, latch during C0=0 itself: the write-hit term
    // (hcc==DI) blanks DISPTMG mid-character. Restore R1=40 first.
    test.run_to_c0(10);
    test.write_selected_register_at_nclken(40);  // hcc=10: no write-hit.
    test.run_to_c0(0);              // Row 1 origin.
    test.run_characters(128);       // Two lines on: frame 3 origin.
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(1);
    test.expect_de_low("t26a latch at C0=0 blanks by C0=1 (section 17.5.1 "
                       "just in time, boundary chronogram)");
    test.run_to_c0(2);
    test.expect_de_low("t26a border holds through the new frame's early "
                       "characters");

    // (C) First too-late latch: C0=1. The acknowledgment is missed, but the
    // live C0=R1 comparison now targets the new R1=0 (section 6.1.3), so no
    // mid-line DISPTMG-off can fire: the line displays past the old R1's
    // end, and R1=0 is honored from the next line.
    test.write_selected_register_at_nclken(40);  // hcc=2: no write-hit.
    test.run_to_c0(0);              // Row 1 origin.
    test.run_characters(128);       // Frame 4 origin.
    test.run_to_c0(1);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(2);
    test.expect_de_high("t26a latch at C0=1 is too late: display continues "
                        "(section 17.5.1)");
    test.run_to_c0(40);
    test.expect_de_high("t26a live C0=R1 targets the new R1: display passes "
                        "the old R1's end (section 6.1.3)");
    test.run_to_c0(0);              // The too-late line's wrap.
    test.expect_de_low("t26a R1=0 honored from the line after the too-late "
                       "write");
}

void test_t26_r1_zero_deadline_type1(TestBench& test) {
    t26_program(test, 1);

    test.run_characters(3u * 64u);

    test.run_characters(64);  // C0=0 of frame 1 row 1.
    test.run_to_c0(63);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(0);        // Row 2 origin, same frame.
    test.expect_de_low("t26b latch at C0=63 before a plain line seam: "
                       "border from C0=0 (section 17.5.1 is seam-generic)");

    test.run_to_c0(10);
    test.write_selected_register_at_nclken(40);
    test.run_to_c0(63);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(0);
    test.expect_de_low("t26b latch at C0=63: seam sees R1=0, border from "
                       "C0=0 (section 17.5.1 just in time)");

    test.run_to_c0(10);
    test.write_selected_register_at_nclken(40);
    test.run_to_c0(0);
    test.run_characters(128);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(1);
    test.expect_de_low("t26b latch at C0=0 blanks by C0=1 (section 17.5.1 "
                       "just in time, boundary chronogram)");
    test.run_to_c0(2);
    test.expect_de_low("t26b border holds through the new frame's early "
                       "characters");

    test.write_selected_register_at_nclken(40);
    test.run_to_c0(0);
    test.run_characters(128);
    test.run_to_c0(1);
    test.write_selected_register_at_nclken(0);
    test.run_to_c0(2);
    test.expect_de_high("t26b latch at C0=1 is too late: display continues "
                        "(section 17.5.1)");
    test.run_to_c0(40);
    test.expect_de_high("t26b live C0=R1 targets the new R1: display passes "
                        "the old R1's end (section 6.1.3)");
    test.run_to_c0(0);
    test.expect_de_low("t26b R1=0 honored from the line after the too-late "
                       "write");
}

void test_type0_adjustment_r4_write_at_r0_switches_c9_to_r5(TestBench& test) {
    test.set_crtc_type(0);

    // R4's documented window includes C0=R0. A same-edge write must affect
    // the imminent rollover, even though the register file itself uses NBA
    // semantics and still presents the previous R4 to the counter block.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(7);
    test.write_selected_register_at_clken(1);

    test.expect_adjustment_active("type 0 accepts an R4 write at C0=R0");
    test.expect_c4("type 0 exact-R0 R4 write keeps C4", 2);
    test.expect_ra("type 0 exact-R0 R4 write compares C9 with R5", 4);
}

void test_type0_adjustment_captures_mid_character_r4_write_at_r0(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(6);
    test.write_selected_register_at_nclken(1);
    test.run_characters(1);

    test.expect_adjustment_active("type 0 captures an R4 bus write within C0=R0");
    test.expect_c4("type 0 mid-character exact-R0 R4 write keeps C4", 2);
    test.expect_ra("type 0 mid-character exact-R0 R4 write uses R5", 4);
}

void test_type0_adjustment_latches_clear_on_snapshot_load(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(2);
    test.write_selected_register_at_nclken(1);
    test.expect_type0_arbitration_latches(
        "type 0 R4 write arms only the R4 comparator latch", true, false, false);

    const std::array<std::uint8_t, 10> snapshot_registers = {{
        7, 4, 5, 0x11, 2, 5, 2, 1, 0, 3,
    }};
    test.load_snapshot_registers(snapshot_registers);
    test.expect_type0_arbitration_latches(
        "snapshot load clears type 0 arbitration state", false, false, false);

    // Arm zero adjustment entry latch, verify it is true, then verify snapshot load clears it.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> zero_adj_registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : zero_adj_registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.run_characters(2);
    test.expect_type0_zero_adj_entry(
        "type 0 zero adjustment entry latch is armed before snapshot load", true);

    test.load_snapshot_registers(snapshot_registers);
    test.expect_type0_zero_adj_entry(
        "snapshot load clears zero adjustment entry latch", false);
}

void test_type0_adjustment_latches_clear_on_type_roundtrip(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(2);
    test.write_selected_register_at_nclken(4);
    test.expect_type0_arbitration_latches(
        "type 0 mid-line R9 write arms the live comparator latch", false, true, false);

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.expect_type0_arbitration_latches(
        "type 1 clears type 0 live-comparator state", false, false, false);

    test.set_crtc_type(0);
    test.write_register(9, 3);
    test.select_register(9);
    test.reset();
    test.run_characters(11 * 8);
    test.run_characters(6);
    test.write_selected_register_at_nclken(4);
    test.expect_type0_arbitration_latches(
        "type 0 exact-R0 R9 write arms the boundary latch", false, false, true);

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.set_crtc_type(0);
    test.expect_type0_arbitration_latches(
        "live type round-trip clears exact-R0 state", false, false, false);

    // Arm zero adjustment entry latch, verify it is true, then verify type round-trip clears it.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> zero_adj_registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : zero_adj_registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.run_characters(2);
    test.expect_type0_zero_adj_entry(
        "type 0 zero adjustment entry latch is armed before type round-trip", true);

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.set_crtc_type(0);
    test.expect_type0_zero_adj_entry(
        "live type round-trip clears zero adjustment entry latch", false);
}

void test_type0_r4_write_at_c0_1_enters_zero_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2: if Last Line was
    // established at C0=0, an R4 write at C0=1 that breaks C4==R4 makes the
    // current line the first adjustment line even when R5=0. The changed R4
    // keeps C4 at zero, while C9 advances from zero instead of being reset.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.expect_type0_arbitration_latches(
        "type 0 C0=1 R4 break arms only zero-adjustment entry", false, false,
        false, true);
    test.run_characters(2);

    test.expect_adjustment_active("type 0 C0=1 R4 break enters adjustment with R5=0");
    test.expect_c4("type 0 C0=1 R4 break keeps C4", 0);
    test.expect_ra("type 0 C0=1 R4 break increments C9", 1);
    test.expect_type0_arbitration_latches(
        "type 0 C0=1 entry latch clears at the line boundary", false, false,
        false);
    test.expect_type0_zero_adj_entry(
        "type 0 C0=1 R4 break sets zero adjustment entry latch", true);

    test.run_characters(4);
    test.expect_adjustment_inactive("type 0 C0=1 R4 zero adjustment completes");
    test.expect_type0_zero_adj_entry(
        "type 0 zero adjustment entry latch clears after completion", false);
}

void test_type0_r9_write_within_c0_1_enters_zero_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.write_selected_register_at_nclken(1);
    test.run_characters(3);

    test.expect_adjustment_active("type 0 C0=1 R9 break enters adjustment with R5=0");
    test.expect_c4("type 0 C0=1 R9 break keeps C4", 0);
    test.expect_ra("type 0 C0=1 R9 break increments C9", 1);
}

void test_type0_r4_write_at_c0_0_overrides_last_line(TestBench& test) {
    test.set_crtc_type(0);

    // At C0=0 the register write participates in the Last Line comparison
    // itself. Breaking C4==R4 here must select ordinary counting, not the
    // special C0=1 zero-adjustment route and not an immutable frame reset.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.write_selected_register_at_clken(1);
    test.run_characters(3);

    test.expect_adjustment_inactive("type 0 C0=0 R4 break does not enter adjustment");
    test.expect_c4("type 0 C0=0 R4 break selects ordinary C4 increment", 1);
    test.expect_ra("type 0 C0=0 R4 break resets matching C9", 0);
}

void test_type0_r0_one_runs_default_zero_adjustment_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.5: R0=1 never reaches
    // C0=2, so it cannot cancel the adjustment armed by C4==R4/C9==R9.
    // With R4=R9=R5=0, one two-character adjustment line at C4=1/C9=0
    // follows the frame line, then the next boundary resets both counters.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 1}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    test.run_characters(2);
    test.expect_adjustment_active("type 0 R0=1 enters default adjustment");
    test.expect_c4("type 0 R0=1 increments C4 once for adjustment", 1);
    test.expect_ra("type 0 R0=1 keeps C9 zero for R5=0 adjustment", 0);
    test.expect_type0_zero_adj_entry(
        "type 0 R0=1 sets zero adjustment entry latch", true);

    test.run_characters(2);
    test.expect_adjustment_inactive("type 0 R0=1 zero adjustment lasts one line");
    test.expect_c4("type 0 R0=1 completion resets C4", 0);
    test.expect_ra("type 0 R0=1 completion resets C9", 0);
    test.expect_type0_zero_adj_entry(
        "type 0 R0=1 clears zero adjustment entry latch after completion", false);
}

void test_type0_r0_zero_starts_default_zero_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6: with R0 already zero,
    // the repeated C0=0 equality freezes C9 but consumes the C4 increment
    // once on the second C0=0. Because C4=R4 and C9=R9 established Last
    // Line, the short line also takes the default adjustment route even
    // though R5=0.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 0}, {1, 0}, {2, 0}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();

    test.run_characters(1);
    test.expect_adjustment_active("type 0 R0=0 enters default adjustment");
    test.expect_c4("type 0 R0=0 consumes one C4 increment", 1);
    test.expect_ra("type 0 R0=0 freezes C9", 0);
    test.expect_type0_arbitration_latches(
        "type 0 R0=0 records that its entry increment was consumed", false,
        false, false, false, true);
    test.expect_type0_zero_adj_entry(
        "type 0 R0=0 sets zero adjustment entry latch", true);

    test.run_characters(3);
    test.expect_c4("type 0 R0=0 does not repeat the C4 increment", 1);
    test.expect_ra("type 0 R0=0 keeps C9 frozen", 0);

    test.write_selected_register_at_nclken(3);
    test.run_characters(4);
    test.expect_adjustment_inactive("type 0 R0=0 recovery completes adjustment");
    test.expect_c4("type 0 R0=0 recovery resets C4", 0);
    test.expect_ra("type 0 R0=0 recovery resets C9", 0);
    test.expect_type0_arbitration_latches(
        "type 0 R0=0 entry guard rearms after recovery", false, false, false);
    test.expect_type0_zero_adj_entry(
        "type 0 R0=0 clears zero adjustment entry latch after completion", false);
}

void test_type0_r0_zero_during_adjustment_freezes_c4(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 13.2.3: once adjustment is active, changing R0 to
    // zero at C0=0 freezes the existing C4/C9 values. The pre-adjustment
    // deferred C4 increment must not fire again merely because C9 equals R9.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 3}, {6, 1}, {7, 1}, {8, 0},    {9, 1},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();

    test.run_characters(11);
    test.expect_adjustment_active("type 0 reaches active adjustment before R0=0");
    test.expect_c4("type 0 adjustment has incremented C4 once", 1);
    test.expect_ra("type 0 adjustment is one character before C9=R9", 0);

    // The helper crosses the imminent CLKEN boundary, then writes at nCLKEN
    // while the resulting C0=0/C9=R9 character is active.
    test.write_selected_register_at_nclken(0);
    test.expect_ra("type 0 R0=0 write lands with C9=R9", 1);
    test.run_characters(1);
    test.expect_adjustment_active("type 0 R0=0 preserves active adjustment");
    test.expect_c4("type 0 R0=0 does not increment C4 again in adjustment", 1);
    test.expect_ra("type 0 R0=0 freezes adjustment C9", 1);
}

void test_type0_r0_one_c0_1_break_is_consumed_at_rollover(TestBench& test) {
    test.set_crtc_type(0);

    // With R0=1, C0=1 is also the line rollover. The equality-breaking R4
    // write must affect that rollover directly and be consumed there; retaining
    // the entry latch would force an extra adjustment line.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 1}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.expect_adjustment_active("type 0 R0=1 C0=1 break enters adjustment");
    test.expect_c4("type 0 R0=1 C0=1 break keeps C4", 0);
    test.expect_ra("type 0 R0=1 C0=1 break advances C9", 1);
    test.expect_type0_arbitration_latches(
        "type 0 R0=1 consumes the C0=1 entry latch at rollover", false, false,
        false);

    test.run_characters(2);
    test.expect_adjustment_inactive("type 0 R0=1 C0=1 adjustment completes next line");
    test.expect_c4("type 0 R0=1 C0=1 completion resets C4", 0);
    test.expect_ra("type 0 R0=1 C0=1 completion resets C9", 0);
}

void test_type0_adjustment_unequal_c9_counts_to_r5(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.1 / compendium lines 109-121 and 128-137:
    // CRTC 0 reuses C9 for vertical adjustment without a separate C5 counter.
    // Documented unequal fixture: R4=10, R5=16, R9=3, R1=40, R0=63.
    // Frame rows 0..10 each have 4 lines (C9=0..3). On the last line (C4=10, C9=3),
    // adjustment is entered at C0=2 (R5>0). C4 increments once to R4+1 (11) and
    // freezes for the whole adjustment, while reused C9 counts 0..15 (0..R5-1).
    // After C9=15, adjustment completes and frame reset follows (C4=0, C9=0).
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 10},
        {5, 16}, {6, 25}, {7, 0},  {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    constexpr unsigned line_characters = 64;
    constexpr unsigned frame_lines_before_adj = (10 + 1) * (3 + 1);  // 44 lines
    test.run_characters(frame_lines_before_adj * line_characters);

    // Reused C9 counts 0..15 continuously in adjustment while C4 freezes at 11.
    for (unsigned adj_line = 0; adj_line < 16; ++adj_line) {
        test.expect_adjustment_active(
            "type 0 vertical adjustment is active at C9=" + std::to_string(adj_line));
        test.expect_c4(
            "type 0 C4 freezes at R4+1 (11) at C9=" + std::to_string(adj_line), 11);
        test.expect_ra(
            "type 0 C9 advances past R9 to " + std::to_string(adj_line), adj_line);
        test.run_characters(line_characters);
    }

    // Line 16 (first line of next frame): adjustment complete, C4/C9 reset to 0.
    test.expect_adjustment_inactive("type 0 leaves vertical adjustment after C9=15");
    test.expect_c4("type 0 frame reset clears C4", 0);
    test.expect_ra("type 0 frame reset clears C9", 0);
}

void test_type0_active_adjustment_r5_zero_counts_through_31(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1, 11.2.1, and 11.2.2:
    // When Type 0 enters ordinary vertical adjustment with R5>0, C9 counts
    // lines in adjustment using equality semantics against R5-1.
    // If R5 is rewritten to 0 while vertical adjustment is already active and
    // C9 has reached a positive value (e.g. C9=1), the effective target
    // becomes R5-1 = 31 (5'b11111). C9 must not immediately reset on the next
    // line, but instead must count all the way through 31 and wrap, completing
    // vertical adjustment only after line C9=31.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 10},
        {5, 16}, {6, 25}, {7, 0},  {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    constexpr unsigned line_characters = 64;
    constexpr unsigned frame_lines_before_adj = (10 + 1) * (3 + 1);  // 44 lines
    test.run_characters(frame_lines_before_adj * line_characters);

    // Adj line 0: C9=0, C4=11.
    test.expect_adjustment_active("type 0 enters normal nonzero adjustment at C9=0");
    test.expect_c4("type 0 C4 freezes at 11 during adjustment", 11);
    test.expect_ra("type 0 C9 starts adjustment at 0", 0);
    test.expect_type0_zero_adj_entry(
        "ordinary adjustment entry does not set zero adjustment latch", false);
    test.run_characters(line_characters);

    // Adj line 1: C9 reaches 1 (C9 is positive).
    test.expect_adjustment_active("type 0 adjustment reaches C9=1");
    test.expect_c4("type 0 C4 remains 11 at C9=1", 11);
    test.expect_ra("type 0 C9 is positive (1) before R5=0 rewrite", 1);
    test.expect_type0_zero_adj_entry(
        "zero adjustment latch remains unset at C9=1", false);

    // Write R5=0 at a pinned C0 phase (C0=2).
    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.run_characters(line_characters - 3);

    // Adj line 2..31: C9 counts through 31 without immediate reset.
    for (unsigned adj_line = 2; adj_line <= 31; ++adj_line) {
        test.expect_adjustment_active(
            "type 0 adjustment remains active after R5=0 rewrite at C9=" + std::to_string(adj_line));
        test.expect_c4(
            "type 0 C4 freezes at 11 at C9=" + std::to_string(adj_line), 11);
        test.expect_ra(
            "type 0 C9 counts through 31 to " + std::to_string(adj_line), adj_line);
        test.expect_type0_zero_adj_entry(
            "zero adjustment latch remains unset during C9 overflow at C9=" + std::to_string(adj_line),
            false);
        test.run_characters(line_characters);
    }

    // Line 32 (first line of next frame): after C9=31, adjustment completes and frame resets.
    test.expect_adjustment_inactive("type 0 leaves vertical adjustment after C9=31");
    test.expect_c4("type 0 frame reset clears C4", 0);
    test.expect_ra("type 0 frame reset clears C9", 0);
    test.expect_type0_zero_adj_entry(
        "zero adjustment latch remains unset after frame reset", false);
}

void test_type0_adjustment_last_line_r5_zero_retargets_to_31(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1, 11.2.1, and 11.2.2:
    // When Type 0 enters ordinary vertical adjustment with R5=4, C9 counts
    // lines in adjustment (0..3) matching against R5-1 = 3.
    // On the current last line of vertical adjustment (C9=3, R5=4), writing
    // R5=0 at C0=2 must revise current-line completion: the effective target
    // becomes R5-1 = 31 (5'b11111). C9 must not end adjustment on this line,
    // but must retarget to 31, counting through lines 4..31 and completing
    // vertical adjustment only after line C9=31.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 4}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned frame_lines_before_adj = (2 + 1) * (3 + 1);  // 12 lines
    test.run_characters(frame_lines_before_adj * line_characters);

    // Adj lines 0..2: C9 counts 0, 1, 2.
    for (unsigned adj_line = 0; adj_line < 3; ++adj_line) {
        test.expect_adjustment_active(
            "type 0 vertical adjustment is active at C9=" + std::to_string(adj_line));
        test.expect_c4(
            "type 0 C4 freezes at R4+1 (3) at C9=" + std::to_string(adj_line), 3);
        test.expect_ra(
            "type 0 C9 is " + std::to_string(adj_line), adj_line);
        test.expect_type0_zero_adj_entry(
            "zero adjustment latch remains unset at C9=" + std::to_string(adj_line), false);
        test.run_characters(line_characters);
    }

    // Adj line 3: C9=3 (this would be the last line if R5 remained 4).
    test.expect_adjustment_active("type 0 reaches last nominal adjustment line at C9=3");
    test.expect_c4("type 0 C4 is 3 at C9=3", 3);
    test.expect_ra("type 0 C9 is 3 before R5=0 rewrite", 3);
    test.expect_type0_zero_adj_entry("zero adjustment latch is unset at C9=3", false);

    // Write R5=0 at C0=2.
    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.expect_type0_r5_adjust_override(
        "type 0 R5 write at C0=2 arms adjustment override latch", true);
    test.run_characters(line_characters - 3);

    // Adj line 4..31: C9 must retarget to 31 instead of ending at line 3.
    for (unsigned adj_line = 4; adj_line <= 31; ++adj_line) {
        test.expect_adjustment_active(
            "type 0 adjustment remains active after R5=0 rewrite at C9=" + std::to_string(adj_line));
        test.expect_c4(
            "type 0 C4 freezes at 3 at C9=" + std::to_string(adj_line), 3);
        test.expect_ra(
            "type 0 C9 counts through 31 to " + std::to_string(adj_line), adj_line);
        test.expect_type0_zero_adj_entry(
            "ordinary R5-to-zero write leaves zero adjustment latch false at C9=" + std::to_string(adj_line),
            false);
        test.expect_type0_r5_adjust_override(
            "R5 adjustment override latch clears at line rollover at C9=" + std::to_string(adj_line),
            false);
        test.run_characters(line_characters);
    }

    // Line 32 (first line of next frame): adjustment complete, C4/C9 reset to 0.
    test.expect_adjustment_inactive("type 0 leaves vertical adjustment after C9=31");
    test.expect_c4("type 0 frame reset clears C4", 0);
    test.expect_ra("type 0 frame reset clears C9", 0);
    test.expect_type0_zero_adj_entry(
        "zero adjustment latch remains unset after frame reset", false);
}

void test_type0_r5_same_line_rejected_write_does_not_retarget(TestBench& test) {
    test.set_crtc_type(0);

    // An accepted R5=0 write at C0=2 latches target 31 for the current
    // adjustment line. A second R5=4 write at C0=3 updates the register but
    // is outside the arbitration window; it must not replace that target
    // before the line rolls over.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 4}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned frame_lines_before_adj = (2 + 1) * (3 + 1);
    test.run_characters(frame_lines_before_adj * line_characters);
    test.run_characters(3 * line_characters);
    test.expect_adjustment_active("same-line R5 two-write fixture reaches adjustment C9=3");
    test.expect_ra("same-line R5 two-write fixture reaches C9=3", 3);

    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.write_selected_register_now(4);
    test.run_characters(line_characters - 3);

    test.expect_adjustment_active(
        "rejected C0=3 R5 write does not complete accepted R5=0 adjustment");
    test.expect_c4("accepted R5=0 target keeps C4 in adjustment", 3);
    test.expect_ra("accepted R5=0 target advances to C9=4", 4);
}

void test_type0_r5_same_line_rejected_zero_does_not_retarget(TestBench& test) {
    test.set_crtc_type(0);

    // Inverse ordering: accepted R5=4 (target 3) at C0=2 followed by a
    // rejected R5=0 write at C0=3 must still complete the current line at
    // C9=3, rather than retargeting it to 31.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 4}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned frame_lines_before_adj = (2 + 1) * (3 + 1);
    test.run_characters(frame_lines_before_adj * line_characters);
    test.run_characters(3 * line_characters);
    test.expect_adjustment_active("inverse same-line R5 fixture reaches adjustment C9=3");
    test.expect_ra("inverse same-line R5 fixture reaches C9=3", 3);

    test.run_characters(2);
    test.write_selected_register_at_clken(4);
    test.write_selected_register_now(0);
    test.run_characters(line_characters - 3);

    test.expect_adjustment_inactive(
        "rejected C0=3 R5=0 write does not retarget accepted R5=4 completion");
}

void test_type0_zero_adj_entry_r5_positive_extends_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2:
    // Entering special zero adjustment (e.g. via C0=1 R4 break with R5=0) arms
    // type0_zero_adj_entry for a one-line adjustment.
    // An accepted positive R5 write before C0=3 (e.g. R5=4 at C0=2) must
    // clear/suppress type0_zero_adj_entry and override stale line_last_r at
    // rollover, extending adjustment toward the new R5 (C9 counting through R5-1)
    // instead of sticky one-line completion.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    constexpr unsigned line_characters = 8;

    // Break C4==R4 at C0=1 on frame line to enter zero adjustment.
    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.run_characters(line_characters - 2);

    // Now on adjustment line: zero-entry is initially armed, C9 advanced to 1.
    test.expect_adjustment_active("type 0 enters special zero adjustment");
    test.expect_c4("type 0 zero adjustment keeps C4 at 0", 0);
    test.expect_ra("type 0 zero adjustment starts at C9=1", 1);
    test.expect_type0_zero_adj_entry(
        "zero adjustment entry latch is initially true", true);

    // Write R5=4 (positive) before C0=3 (at C0=2).
    test.select_register(5);
    test.run_characters(2);
    test.write_selected_register_at_clken(4);
    test.expect_type0_zero_adj_entry(
        "positive R5 write at C0=2 clears zero adjustment entry latch immediately", false);
    test.expect_type0_r5_adjust_override(
        "positive R5 write at C0=2 arms adjustment override latch", true);
    test.run_characters(line_characters - 3);

    // Line 1 completes rollover without ending adjustment; C9 advances to 2.
    test.expect_adjustment_active("type 0 positive R5 write extends adjustment");
    test.expect_c4("type 0 C4 remains 0 during extended adjustment", 0);
    test.expect_ra("type 0 C9 advances to 2 instead of sticky completion", 2);
    test.expect_type0_zero_adj_entry(
        "zero adjustment entry latch remains false", false);
    test.expect_type0_r5_adjust_override(
        "R5 adjustment override latch clears after line rollover", false);

    // Line 2: C9 counts to 3 (which is R5-1 = 3).
    test.run_characters(line_characters);
    test.expect_ra("type 0 C9 reaches 3 (new R5-1)", 3);
    test.expect_adjustment_active("type 0 adjustment active on final line C9=3");

    // Line 3: adjustment completes and frame resets after C9=3.
    test.run_characters(line_characters);
    test.expect_adjustment_inactive("type 0 leaves adjustment after C9=3");
    test.expect_c4("type 0 frame reset clears C4", 0);
    test.expect_ra("type 0 frame reset clears C9", 0);
    test.expect_type0_zero_adj_entry(
        "zero adjustment entry latch remains clear after frame reset", false);

    // Rejection check: write R5 positive after C0=2 (at C0=3) does NOT extend adjustment.
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.run_characters(line_characters - 2);

    test.expect_adjustment_active("type 0 enters special zero adjustment for rejection check");
    test.expect_ra("type 0 zero adjustment starts at C9=1", 1);
    test.expect_type0_zero_adj_entry(
        "zero adjustment entry latch is true before late write", true);

    // Write R5=4 at C0=3 (after the accepted window C0<=2).
    test.select_register(5);
    test.run_characters(3);
    test.write_selected_register_at_clken(4);
    test.expect_type0_r5_adjust_override(
        "late R5 write at C0=3 does NOT arm adjustment override latch", false);
    test.run_characters(line_characters - 4);

    // The late write is rejected for current line: adjustment ends after line 1.
    test.expect_adjustment_inactive("type 0 late R5 write after C0=2 does not extend adjustment");
    test.expect_c4("type 0 late R5 write resets C4", 0);
    test.expect_ra("type 0 late R5 write resets C9", 0);
}

// ---------------------------------------------------------------------------
// t07 / t08: F4 equality-only counter overflow and the section 28.1.1 CRTC
// identification boundaries.  Test-only checkpoint: these vectors encode the
// v1.10 rules, and the cases the current comparator shortcuts cannot satisfy
// are registered as named known divergences rather than weakened.
// ---------------------------------------------------------------------------

using RegisterProgram = std::array<std::pair<std::uint8_t, std::uint8_t>, 10>;

void program_registers(TestBench& test, const RegisterProgram& registers) {
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
}

// R9=7 frame with a long C4 limit, so a lowered R9 exercises C9 alone.
constexpr RegisterProgram kC9OverflowRegisters = {{
    {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 20},
    {5, 0}, {6, 20}, {7, 100}, {8, 0}, {9, 7},
}};

// R9=0 makes every character line a row, so a lowered R4 exercises C4 alone.
constexpr RegisterProgram kC4OverflowRegisters = {{
    {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 5},
    {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 0},
}};

constexpr unsigned kOverflowLineCharacters = 4;

// Reach C9=4 (C9-overflow fixture) or C4=4 (C4-overflow fixture) with C0 back
// at 0, ready for the register write that lowers the limit below the counter.
void run_to_fourth_line_start(TestBench& test) {
    test.run_characters(4 * kOverflowLineCharacters);
}

void run_lines(TestBench& test, unsigned lines) {
    test.run_characters(lines * kOverflowLineCharacters);
}

void test_type1_c9_counts_through_31_and_wraps(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 sections 10.3 and 10.3.2: C9 uses equality, never
    // magnitude.  R9 lowered below C9 leaves C9 counting up to 31, wrapping,
    // and only then meeting the new limit.  C4 must not move while C9 wraps.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 1 C9 overflow fixture reaches C9=4", 4);
    test.expect_c4("type 1 C9 overflow fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(2);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_ra("type 1 R9 below C9 keeps counting C9", 5);
    test.expect_c4("type 1 R9 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_ra("type 1 C9 reaches its 5-bit limit", 31);
    test.expect_c4("type 1 C9 overflow still does not increment C4", 0);

    run_lines(test, 1);
    test.expect_ra("type 1 C9 wraps past 31", 0);
    test.expect_c4("type 1 C9 wrap is not a C9=R9 match", 0);

    // 0 -> 1 -> 2 meets the new R9, so the third line resets C9 and moves C4.
    run_lines(test, 3);
    test.expect_ra("type 1 wrapped C9 meets the new R9", 0);
    test.expect_c4("type 1 wrapped C9 match increments C4", 1);
}

void test_type1_c9_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 section 10.3: "If R9 is rewritten to 0 while C9>0, C9 does
    // not snap to 0 - it overflows up to 31 then wraps."  Zero is an ordinary
    // limit value, so the same overflow applies as for the nonzero case above.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 1 zero-limit fixture reaches C9=4", 4);
    test.expect_c4("type 1 zero-limit fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_ra("type 1 R9=0 below C9 keeps counting C9", 5);
    test.expect_c4("type 1 R9=0 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_ra("type 1 R9=0 lets C9 reach its 5-bit limit", 31);

    run_lines(test, 1);
    test.expect_ra("type 1 R9=0 wraps C9 onto the new limit", 0);
    test.expect_c4("type 1 R9=0 wrap does not yet increment C4", 0);

    run_lines(test, 1);
    test.expect_ra("type 1 wrapped C9 matches R9=0", 0);
    test.expect_c4("type 1 wrapped C9 match increments C4 once", 1);
}

void test_type1_c4_counts_through_127_and_wraps(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 sections 12 and 12.3: outside vertical adjustment C4 uses
    // equality too.  R4 lowered below C4 leaves C4 counting to its 7-bit
    // limit, wrapping, and only then meeting the new limit.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 1 C4 overflow fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(1);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 1 R4 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 1 C4 reaches its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 1 C4 wraps past 127", 0);

    run_lines(test, 1);
    test.expect_c4("type 1 wrapped C4 is not yet the new limit", 1);

    run_lines(test, 1);
    test.expect_c4("type 1 wrapped C4 meets the new R4", 0);
}

void test_type1_c4_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 section 12.3: on type 1, R4=0 is an ordinary value with no
    // last-line latch, so writing it while C4>0 overflows C4 rather than
    // ending the frame.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 1 zero-limit C4 fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 1 R4=0 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 1 R4=0 lets C4 reach its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 1 R4=0 wraps C4 onto the new limit", 0);

    run_lines(test, 1);
    test.expect_c4("type 1 R4=0 holds C4 at the reached limit", 0);
}

void test_type0_c9_counts_through_31_and_wraps(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1 and 12.2: type 0 establishes the last-line
    // state while C0<2, so the R9 write lands at C0=0 and participates in that
    // comparison.  The new limit is below C9 and not a last line, so the live
    // general case applies: C9 counts through 31 and wraps.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 0 C9 overflow fixture reaches C9=4", 4);
    test.expect_c4("type 0 C9 overflow fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(2);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_ra("type 0 R9 below C9 keeps counting C9", 5);
    test.expect_c4("type 0 R9 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_ra("type 0 C9 reaches its 5-bit limit", 31);
    test.expect_c4("type 0 C9 overflow still does not increment C4", 0);

    run_lines(test, 1);
    test.expect_ra("type 0 C9 wraps past 31", 0);
    test.expect_c4("type 0 C9 wrap is not a C9=R9 match", 0);

    run_lines(test, 3);
    test.expect_ra("type 0 wrapped C9 meets the new R9", 0);
    test.expect_c4("type 0 wrapped C9 match increments C4", 1);
}

void test_type0_c9_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 10.3: R9=0 written while C9>0 must overflow exactly
    // like any other lowered limit.  C4=0 is far from R4=20, so no last-line
    // exception can apply and no adjustment route is open.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 0 zero-limit fixture reaches C9=4", 4);
    test.expect_c4("type 0 zero-limit fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_ra("type 0 R9=0 below C9 keeps counting C9", 5);
    test.expect_c4("type 0 R9=0 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_ra("type 0 R9=0 lets C9 reach its 5-bit limit", 31);

    run_lines(test, 1);
    test.expect_ra("type 0 R9=0 wraps C9 onto the new limit", 0);
    test.expect_c4("type 0 R9=0 wrap does not yet increment C4", 0);

    run_lines(test, 1);
    test.expect_ra("type 0 wrapped C9 matches R9=0", 0);
    test.expect_c4("type 0 wrapped C9 match increments C4 once", 1);
}

void test_type0_c4_counts_through_127_and_wraps(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2: outside adjustment, a C4 that has passed a
    // newly lowered R4 advances on ordinary C9=R9 row completions through 127
    // and wraps.  The R4 write lands at C0=0, inside the last-line window.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 0 C4 overflow fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(1);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 R4 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 0 C4 reaches its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 0 C4 wraps past 127", 0);

    run_lines(test, 1);
    test.expect_c4("type 0 wrapped C4 is not yet the new limit", 1);

    run_lines(test, 1);
    test.expect_c4("type 0 wrapped C4 meets the new R4", 0);
}

void test_type0_c4_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2: R4=0 is reached through equality plus the
    // Last Line / adjustment state, not through a magnitude special case.
    // Written while C4=4, it must let C4 overflow instead of ending the frame.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 0 zero-limit C4 fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 R4=0 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 0 R4=0 lets C4 reach its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 0 R4=0 wraps C4 onto the new limit", 0);
}

void test_type0_rlal_zero_limit_arms_last_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2: to arm Last Line while C4=C9=0, one limit must
    // already be zero and the other positive.  Here R9=0 already holds, so
    // writing the remaining positive limit R4 to 0 while C0<2 arms the state
    // and every following line repeats C4=C9=0.  This is the RLAL steady state
    // that removing the comparator shortcuts must preserve.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 2},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 0},
    }});
    test.select_register(4);
    test.reset();

    test.expect_c4("type 0 RLAL fixture starts at C4=0", 0);
    test.expect_ra("type 0 RLAL fixture starts at C9=0", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 zero-limit arm keeps C4 at zero", 0);
    test.expect_ra("type 0 zero-limit arm keeps C9 at zero", 0);
    test.expect_adjustment_inactive("type 0 zero-limit arm is not adjustment");

    run_lines(test, 4);
    test.expect_c4("type 0 zero-limit arm perpetuates C4=0", 0);
    test.expect_ra("type 0 zero-limit arm perpetuates C9=0", 0);
}

void test_type0_rlal_single_zero_limit_does_not_arm(TestBench& test) {
    test.set_crtc_type(0);

    // The same section rejects the loose "R4>0 or R9>0" reading: with R9=2
    // still positive, writing only R4=0 while C4=C9=0 leaves C9 unequal to R9,
    // so Last Line is not armed and C9 simply counts on.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 2},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 2},
    }});
    test.select_register(4);
    test.reset();

    test.expect_c4("type 0 single-zero-limit fixture starts at C4=0", 0);
    test.expect_ra("type 0 single-zero-limit fixture starts at C9=0", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 single zero limit does not arm Last Line", 0);
    test.expect_ra("type 0 single zero limit lets C9 count", 1);
    test.expect_adjustment_inactive(
        "type 0 single zero limit does not enter adjustment");

    // C9 still reaches R9 two lines later; only then does the now-equal R4=0
    // complete a frame, and C4 has never left zero.
    run_lines(test, 2);
    test.expect_ra("type 0 single zero limit completes the C9 count", 0);
    test.expect_c4("type 0 single zero limit never moves C4", 0);
}

void test_type0_rlal_first_line_delayed_arming(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2.1 first-line variant: on the first frame line
    // (C4=C9=0) the Last Line test has already run and found inequality, so a
    // later R9=0/R4 write cannot make that line the last one.  The documented
    // fix writes R4=1 with R9=0 after the C0<2 window: the live rollover
    // compares C9 with the new R9 and increments C4, so line 2 holds C4=1/C9=0
    // and arms Last Line in its own C0<2 window, resetting both on line 3.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 5},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 3},
    }});
    test.select_register(9);
    test.reset();

    test.expect_c4("type 0 delayed-arming fixture starts at C4=0", 0);
    test.expect_ra("type 0 delayed-arming fixture starts at C9=0", 0);

    // C0=2 and C0=3 are both outside the C0<2 window this rule needs.
    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.write_register(4, 1);
    test.run_characters(1);

    test.expect_c4("type 0 delayed arming increments C4 at the rollover", 1);
    test.expect_ra("type 0 delayed arming resets C9 against the new R9", 0);

    run_lines(test, 1);
    test.expect_c4("type 0 delayed arming resets C4 on the third line", 0);
    test.expect_ra("type 0 delayed arming resets C9 on the third line", 0);
}

void test_type0_rlal_from_the_genuine_last_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2.1, RLAL from the genuine last line: rewriting
    // R9 and R4 to 0 around the true frame end perpetuates C9=C4=0.  The same
    // section requires waiting until C0=2 before rewriting R9 once C9 has
    // become equal to it, so R9=0 lands at C0=2 of the last line and R4=0 at
    // C0=0 of the following line, inside its C0<2 arming window.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 2},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 1},
    }});
    test.select_register(9);
    test.reset();

    // Frame lines are (C4,C9) = (0,0) (0,1) (1,0) (1,1) (2,0) (2,1); the last
    // pair is the genuine last line.
    run_lines(test, 5);
    test.expect_c4("type 0 RLAL fixture reaches the last row", 2);
    test.expect_ra("type 0 RLAL fixture reaches the last line", 1);

    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.run_characters(1);
    test.expect_c4("type 0 genuine last line still completes the frame", 0);
    test.expect_ra("type 0 genuine last line still resets C9", 0);

    test.select_register(4);
    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 RLAL keeps C4 at zero", 0);
    test.expect_ra("type 0 RLAL keeps C9 at zero", 0);
    test.expect_adjustment_inactive("type 0 RLAL does not enter adjustment");

    run_lines(test, 4);
    test.expect_c4("type 0 RLAL perpetuates C4=0", 0);
    test.expect_ra("type 0 RLAL perpetuates C9=0", 0);
}

void test_type0_r9_write_before_r1_advances_next_row_ma(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 10.3.1 and section 17.1: live C9==R9 equality at C0==R1
    // latches VMA' (row_addr_save). When R9 is rewritten to 0 at C0=2 before
    // C0=R1 on a line where C9=0, row_addr_save captures the current pointer
    // and the line-end rollover resets C9/increments C4 to start the next row
    // at the advanced address rather than repeating the previous row.
    program_registers(test, {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 5},
        {5, 0}, {6, 4}, {7, 100}, {8, 0}, {9, 3},
    }});
    test.select_register(9);
    test.reset();

    test.expect_c4("type 0 live row_addr_save fixture starts at C4=0", 0);
    test.expect_ra("type 0 live row_addr_save fixture starts at C9=0", 0);

    const std::uint16_t initial_ma = test.ma();
    const auto expected_ma = [initial_ma](unsigned offset) {
        return static_cast<std::uint16_t>((initial_ma + offset) & 0x3fff);
    };

    // Run to C0=2, then write R9=0 before C0 reaches R1=4.
    test.run_characters(2);
    test.expect_ma("type 0 MA reaches offset 2 at C0=2", expected_ma(2));
    test.write_selected_register_at_clken(0);

    // Advance across the line end (R0=7, so 8 characters per line).
    // The write happened at C0=2 (taking 1 character tick to reach C0=3).
    // 4 characters remain to reach C0=7.
    test.run_characters(4);
    test.expect_ma("type 0 MA reaches offset 7 at C0=7", expected_ma(7));

    // At the next CLKEN, C0 rolls over to 0 of the next line (row 1, line 0).
    test.run_characters(1);
    test.expect_c4("type 0 delayed R9=0 increments C4 to 1", 1);
    test.expect_ra("type 0 delayed R9=0 resets C9 to 0", 0);
    test.expect_ma(
        "type 0 next-row MA reloads saved address rather than repeating old row",
        expected_ma(4));

    // Verify row 1 continues advancing MA.
    test.run_characters(4);
    test.expect_ma("type 0 row 1 MA reaches offset 8 at C0=4", expected_ma(8));

    test.run_characters(4);
    test.expect_c4("type 0 row advances to C4=2", 2);
    test.expect_ra("type 0 C9 remains 0 on row 2", 0);
    test.expect_ma("type 0 row 2 starts at MA offset 8", expected_ma(8));
}

// ACCC v1.10 section 28.1.1: R4=36, R9=7, R5=16 is the documented CRTC
// identification frame.  Use standard 64-character CPC horizontal timing so
// this counter-only oracle is directly comparable with hardware traces.
constexpr RegisterProgram kIdentificationRegisters = {{
    {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 36},
    {5, 16}, {6, 25}, {7, 0}, {8, 0}, {9, 7},
}};

constexpr unsigned kIdentificationFrameLines = (36 + 1) * (7 + 1) + 16;
constexpr unsigned kIdentificationLineCharacters = 64;
constexpr unsigned kIdentificationObserveCharacters =
    (kIdentificationFrameLines + 1) * kIdentificationLineCharacters;

bool run_identification_sweep(TestBench& test,
                              unsigned type,
                              std::uint8_t r7) {
    test.set_crtc_type(type);
    // Program R7 once with its swept value: an R7 write whose value equals the
    // current C4 is itself a VSYNC trigger, so a placeholder write would add a
    // pulse the sweep must not see.
    RegisterProgram registers = kIdentificationRegisters;
    registers[7].second = r7;
    program_registers(test, registers);
    test.reset();

    return test.vsync_within_characters(kIdentificationObserveCharacters);
}

void test_type0_identification_r7_36_fires(TestBench& test) {
    // Control for the sweep: C4 reaches 36 during ordinary counting, so the
    // fixture must produce a VSYNC before any overflow question arises.
    const bool seen = run_identification_sweep(test, 0, 36);
    test.expect_vsync_observed("type 0 R7=36 still triggers VSYNC", seen);
}

void test_type0_identification_r7_37_fires(TestBench& test) {
    // C4 repeats past R4 exactly once for the type-0 additional-line
    // management, so 37 is the last value R7 can reach.
    const bool seen = run_identification_sweep(test, 0, 37);
    test.expect_vsync_observed("type 0 R7=37 triggers VSYNC in adjustment", seen);
}

void test_type0_identification_r7_38_is_silent(TestBench& test) {
    // The type-0 half of the published discriminator: VSYNC ceases above 37.
    const bool seen = run_identification_sweep(test, 0, 38);
    test.expect_no_vsync_observed("type 0 R7=38 never triggers VSYNC", seen);
}

void test_type1_identification_r7_36_fires(TestBench& test) {
    const bool seen = run_identification_sweep(test, 1, 36);
    test.expect_vsync_observed("type 1 R7=36 still triggers VSYNC", seen);
}

void test_type1_identification_r7_37_fires(TestBench& test) {
    const bool seen = run_identification_sweep(test, 1, 37);
    test.expect_vsync_observed("type 1 R7=37 triggers VSYNC in adjustment", seen);
}

void test_type1_identification_r7_38_fires(TestBench& test) {
    // This is the direct cross-type discriminator: type 0 is already silent
    // at 38, while type 1 increments C4 again during adjustment.  That extra
    // counting depends on F8's separate C5 counter.
    const bool seen = run_identification_sweep(test, 1, 38);
    test.expect_vsync_observed("type 1 R7=38 triggers VSYNC in adjustment",
                               seen);
}

void test_type1_identification_r7_39_is_silent(TestBench& test) {
    // ACCC v1.10 sections 16.1/16.4.2 and review action A1: C4=37 on
    // adjustment entry and 38 after eight lines, but the sixteenth line ends
    // adjustment by taking C4 directly from 38 to 0.  C4 never reaches 39,
    // so the former final-row+1 comparator pulse was spurious.  This corrects
    // the older §28.1.1-based oracle in this vector; the source tension is
    // recorded in the session plan for the independent review pass.
    const bool seen = run_identification_sweep(test, 1, 39);
    test.expect_no_vsync_observed(
        "type 1 R7=39 is skipped at adjustment end", seen);
}

void test_type1_identification_r7_40_is_silent(TestBench& test) {
    // R7=40 is also unreachable; A1 moves the first silent value down to 39.
    const bool seen = run_identification_sweep(test, 1, 40);
    test.expect_no_vsync_observed("type 1 R7=40 never triggers VSYNC", seen);
}

// ---------------------------------------------------------------------------
// F8 worked example, zero-R5 bug, and Type 0 control tests
// ---------------------------------------------------------------------------

void test_type1_adjustment_c4_c9_c5_worked_example(TestBench& test) {
    test.set_crtc_type(1);

    // Compendium section 4.1 (ACCC v1.10 section 11.2.1, page 81):
    // Worked example: R4=10, R5=16, R9=3, R1=40, R0=63.
    // Total normal lines = (10 + 1) * (3 + 1) = 44 lines = 2816 characters.
    constexpr RegisterProgram kWorkedExampleRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 10},
        {5, 16}, {6, 25}, {7, 30}, {8, 0},    {9, 3},
    }};
    program_registers(test, kWorkedExampleRegisters);
    test.reset();

    // Advance through the normal frame to reach vertical adjustment entry.
    test.run_characters(44 * 64);

    // Compendium section 4.1 and ACCC v1.10 section 11.2.1 (page 81):
    // On CRTC 1, vertical adjustment counts 16 lines (c5 = 0..15).
    // C9 cycles 0..R9 (0,1,2,3, 0,1,2,3, 0,1,2,3, 0,1,2,3).
    // C4 increments each time C9 reaches R9=3:
    // lines 0..3:   c5=0..3,   C4=11, C9=0..3
    // lines 4..7:   c5=4..7,   C4=12, C9=0..3
    // lines 8..11:  c5=8..11,  C4=13, C9=0..3
    // lines 12..15: c5=12..15, C4=14, C9=0..3
    struct ExpectedLine {
        std::uint8_t c4;
        std::uint8_t c9;
        std::uint8_t c5;
    };

    const std::array<ExpectedLine, 16> expected_sequence = {{
        {11, 0, 0},  {11, 1, 1},  {11, 2, 2},  {11, 3, 3},
        {12, 0, 4},  {12, 1, 5},  {12, 2, 6},  {12, 3, 7},
        {13, 0, 8},  {13, 1, 9},  {13, 2, 10}, {13, 3, 11},
        {14, 0, 12}, {14, 1, 13}, {14, 2, 14}, {14, 3, 15},
    }};

    for (unsigned line_idx = 0; line_idx < 16; ++line_idx) {
        const auto& exp = expected_sequence[line_idx];
        const std::string line_tag = "line " + std::to_string(line_idx) + " (c5=" + std::to_string(exp.c5) + ")";
        test.expect_c4("CRTC 1 worked example " + line_tag + " C4", exp.c4);
        test.expect_ra("CRTC 1 worked example " + line_tag + " C9/RA", exp.c9);
        test.expect_c5("CRTC 1 worked example " + line_tag + " C5", exp.c5);
        test.run_characters(64);
    }

    // After 16 lines of adjustment (c5+1 == 16 == R5), adjustment ends.
    // C4 and C9 reset to 0 unconditionally for frame 1 start.
    test.expect_c4("CRTC 1 worked example frame 1 start C4=0", 0);
    test.expect_ra("CRTC 1 worked example frame 1 start C9=0", 0);
    test.expect_c5("CRTC 1 worked example frame 1 start C5=0", 0);
}

void test_type1_r5_zero_mid_adjustment_keeps_counting(TestBench& test) {
    test.set_crtc_type(1);

    // Compendium section 4.4 (ACCC v1.10 section 11.3.2, page 85):
    // Bug: R5 set to 0 during adjustment does NOT reset C4/end adjustment.
    // CRTC 1 compares C5+1 == R5 by equality, which cannot match R5=0.
    // C5 free-runs, C4 continues incrementing at C9==R9 wraps.
    // Only setting R5 to a reachable positive value clears adjustment.
    constexpr RegisterProgram kRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 10},
        {5, 16}, {6, 25}, {7, 30}, {8, 0},    {9, 3},
    }};
    program_registers(test, kRegisters);
    test.reset();

    // Advance 44 lines into the frame to enter vertical adjustment.
    test.run_characters(44 * 64);

    // On line 0 of adjustment: c5=0, C4=11, C9=0.
    test.expect_c4("adjustment line 0: C4=11", 11);
    test.expect_c5("adjustment line 0: c5=0", 0);

    // Run 2 lines to reach line 2 (c5=2, C4=11, C9=2).
    test.run_characters(2 * 64);
    test.expect_c5("adjustment line 2: c5=2", 2);

    // Mid-adjustment write: set R5 = 0.
    test.write_register(5, 0);

    // Advance through the remaining lines up to line 15 (13 lines = 13 * 64 chars).
    test.run_characters(13 * 64);
    test.expect_c4("adjustment line 15: C4=14", 14);
    test.expect_ra("adjustment line 15: C9=3", 3);
    test.expect_c5("adjustment line 15: c5=15", 15);

    // Line 16: With R5=0, adjustment does NOT end!
    // C9 wraps to 0, C4 increments to 15, c5 increments to 16.
    test.run_characters(64);
    test.expect_c4("adjustment line 16: C4 continues to 15", 15);
    test.expect_ra("adjustment line 16: C9 wraps to 0", 0);
    test.expect_c5("adjustment line 16: c5 advances to 16", 16);

    // Advance further through c5=31 (15 lines = 15 * 64 chars) to reach c5=31.
    test.run_characters(15 * 64);
    test.expect_c5("adjustment line 31: c5 reaches 31", 31);
    test.expect_c4("adjustment line 31: C4 reaches 18", 18);
    test.expect_ra("adjustment line 31: C9 reaches 3", 3);

    // Next line (32nd adjustment line): c5 wraps from 31 to 0! C4 increments to 19.
    test.run_characters(64);
    test.expect_c5("adjustment line 32: c5 wraps to 0", 0);
    test.expect_c4("adjustment line 32: C4 increments to 19", 19);
    test.expect_ra("adjustment line 32: C9 wraps to 0", 0);

    // Run 5 more lines to reach c5=5.
    test.run_characters(5 * 64);
    test.expect_c5("c5 reaches 5", 5);

    // Now write a reachable non-zero value: R5 = 8.
    test.write_register(5, 8);

    // Run 2 lines: line with c5=6, line with c5=7.
    test.run_characters(2 * 64);
    test.expect_c5("c5 reaches 7", 7);

    // At the end of line with c5=7, c5+1 == 8 == R5 matches!
    // Next line should be frame 1 start (C4=0, C9=0, c5=0).
    test.run_characters(64);
    test.expect_c4("frame 1 start: C4 resets to 0", 0);
    test.expect_ra("frame 1 start: C9 resets to 0", 0);
    test.expect_c5("frame 1 start: c5 resets to 0", 0);
}

void test_type0_adjustment_c4_frozen_c9_counts_to_r5(TestBench& test) {
    test.set_crtc_type(0);

    // Compendium section 4.1 and section 4.2 (ACCC v1.10 section 11.1-11.2, pages 80-83):
    // Type 0 worked example: R4=10, R5=16, R9=3, R1=40, R0=63.
    // Total normal lines = (10 + 1) * (3 + 1) = 44 lines = 2816 characters.
    // On CRTC 0, vertical adjustment has NO separate C5 counter:
    // C4 freezes at 11 (R4+1) for the ENTIRE 16-line adjustment.
    // C9 counts 0..15 continuously (exceeding R9=3) until reaching R5-1=15.
    constexpr RegisterProgram kWorkedExampleRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 10},
        {5, 16}, {6, 25}, {7, 30}, {8, 0},    {9, 3},
    }};
    program_registers(test, kWorkedExampleRegisters);
    test.reset();

    // Complete normal frame.
    test.run_characters(44 * 64);

    // For all 16 lines of adjustment, C4 must remain 11 while C9 counts 0..15.
    for (unsigned line_idx = 0; line_idx < 16; ++line_idx) {
        const std::string line_tag = "line " + std::to_string(line_idx);
        test.expect_c4("CRTC 0 control " + line_tag + " C4 frozen at 11", 11);
        test.expect_ra("CRTC 0 control " + line_tag + " C9/RA counts 0..15", line_idx);
        test.run_characters(64);
    }

    // Frame 1 start: C4=0, C9=0.
    test.expect_c4("CRTC 0 control frame 1 start C4=0", 0);
    test.expect_ra("CRTC 0 control frame 1 start C9=0", 0);
}

void test_type1_r4_zero_adjustment_vma_reloads_on_c4_one(TestBench& test);

// ---------------------------------------------------------------------------
// t20: Video Pointer R12/R13 reload coverage across normal and degenerate frames
// ---------------------------------------------------------------------------

void write_r12_r13_character(TestBench& test, std::uint8_t r12, std::uint8_t r13) {
    test.write_register(12, r12);
    test.write_register(13, r13);
    test.run_clock_ticks(12);
}

void write_r13_character(TestBench& test, std::uint8_t r13) {
    test.write_register(13, r13);
    test.run_clock_ticks(14);
}

void test_type1_r4_zero_adjustment_vma_reloads_on_c4_one(TestBench& test) {
    test.set_crtc_type(1);

    // Compendium section 4.3 (ACCC v1.10 section 11.2.4, pages 83-84):
    // If C4 was 0 immediately before adjustment began (R4=0), VMA loads
    // from R12/R13 (not VMA') for as long as C4==1 in adjustment.
    // R4=0, R9=3 (4 lines/row), R5=8 (8 lines adjust), R0=63, R1=40.
    constexpr RegisterProgram kR4ZeroRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 0},
        {5, 8},  {6, 25}, {7, 30}, {8, 0},    {9, 3},
    }};
    program_registers(test, kR4ZeroRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();

    // Complete initial frame (4 normal lines + 8 adjust lines = 12 lines of 64 chars)
    // to reach genuine frame start where frame_new asserts and loads R12/R13.
    test.run_characters(12 * 64);

    // Row 0 has 4 lines (C9=0..3). Total 4 * 64 = 256 characters.
    // Line 0: MA = 0x1234
    test.expect_ma("CRTC 1 R4=0 row 0 line 0 MA loaded from R12/R13", 0x1234);

    // Advance to line 3 (last line of row 0, C9=3=R9).
    test.run_characters(3 * 64);
    test.expect_c4("row 0 line 3 C4=0", 0);
    test.expect_ra("row 0 line 3 C9=3", 3);

    // On line 3 (C9=3=R9), advance past C0=R1=40 where VMA' latches VMA:
    // VMA' becomes 0x1234 + 40 = 0x125C.
    test.run_characters(50);

    // Write new R12/R13 (0x2050) at C0=50.
    write_r12_r13_character(test, 0x20, 0x50);

    // Complete line 3 (64 - 51 = 13 chars) to reach adjustment line 0 (C4=1, C9=0).
    test.run_characters(13);

    // In adjustment while C4==1: VMA reloads from R12/R13 (0x2050), NOT VMA' (0x125C)!
    test.expect_c4("adjustment line 0 C4=1", 1);
    test.expect_ra("adjustment line 0 C9=0", 0);
    test.expect_ma("adjustment line 0 (C4=1) reloads new R12/R13 (0x2050)", 0x2050);

    // Advance into line 0, write new R12/R13 (0x3078) at C0=10, complete line 0.
    test.run_characters(10);
    write_r12_r13_character(test, 0x30, 0x78);
    test.run_characters(53);

    // Line 1 of adjustment (C4=1, C9=1): VMA reloads new R12/R13 (0x3078).
    test.expect_c4("adjustment line 1 C4=1", 1);
    test.expect_ra("adjustment line 1 C9=1", 1);
    test.expect_ma("adjustment line 1 (C4=1) reloads new R12/R13 (0x3078)", 0x3078);

    // Complete lines 1 and 2 (2 * 64 chars) to reach line 3 of adjustment (C4=1, C9=3=R9).
    test.run_characters(2 * 64);
    test.expect_c4("adjustment line 3 C4=1", 1);
    test.expect_ra("adjustment line 3 C9=3", 3);

    // On line 3 (C4=1, C9=3=R9), advance past C0=R1=40 where VMA' latches VMA:
    // VMA' becomes 0x3078 + 40 = 0x30A0.
    test.run_characters(50);
    write_r12_r13_character(test, 0x01, 0x11);
    test.run_characters(13);

    // Line 4 of adjustment (c5=4, C4=2, C9=0):
    // Now C4 is 2 (no longer 1)! VMA must reload from VMA' (0x30A0), refusing R12/R13 (0x0111).
    test.expect_c4("adjustment line 4 C4=2", 2);
    test.expect_ra("adjustment line 4 C9=0", 0);
    test.expect_ma("adjustment line 4 (C4=2) reloads VMA' (0x30A0) refusing R12/R13 (0x0111)", 0x30A0);
}

void test_type1_adjustment_end_does_not_fire_unreached_r7(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 sections 16.1 and 16.4.2: VSYNC fires only when C4
    // actually reaches R7.  On paper, R4=1/R5=2/R9=0 produces normal C4
    // rows 0,1 followed by adjustment rows 2,3; the adjustment-ending
    // transition is C4=3 -> 0.  C4 never reaches R7=4, so no VSYNC pulse is
    // permitted on that boundary (review action A1).
    constexpr RegisterProgram kRegisters = {{
        {0, 7}, {1, 4}, {2, 6}, {3, 0x11}, {4, 1},
        {5, 2}, {6, 2}, {7, 4}, {8, 0},    {9, 0},
    }};
    program_registers(test, kRegisters);
    test.reset();

    constexpr unsigned normal_and_adjustment_characters = 4 * 8;
    const bool seen = test.vsync_within_characters(
        normal_and_adjustment_characters + 8);
    test.expect_no_vsync_observed(
        "type 1 adjustment end skips unreachable R7=final-row+1", seen);
}

void prepare_type1_a2_exact_r0_write(TestBench& test,
                                     std::uint8_t target_register,
                                     std::uint8_t value) {
    test.set_crtc_type(1);
    constexpr RegisterProgram kRegisters = {{
        {0, 7}, {1, 4}, {2, 6}, {3, 0x11}, {4, 0},
        {5, 2}, {6, 2}, {7, 20}, {8, 0},   {9, 1},
    }};
    program_registers(test, kRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();

    // Establish a genuine frame start, then reach C0=4 on the last normal
    // line (C4=0/C9=R9=1).  The C0=R1 save captures VMA'=0x1238 before
    // R12/R13 change to 0x2050.  Two more CLKENs reach C0=R0=7, where the
    // selected A2 write lands on the adjustment-entry rollover.
    constexpr unsigned frame_characters = (2 + 2) * 8;
    test.run_characters(frame_characters);
    test.expect_ma("A2 fixture starts the frame from R12/R13", 0x1234);
    test.run_characters(8 + 4);
    test.write_register(12, 0x20);
    test.write_register(13, 0x50);
    test.select_register(target_register);
    test.run_characters(2);
    test.write_selected_register_at_clken(value);
    test.expect_adjustment_active("A2 exact-R0 write enters adjustment");
    test.expect_c4("A2 exact-R0 write enters adjustment at C4=1", 1);
    test.expect_ra("A2 exact-R0 write resets C9 on adjustment entry", 0);
}

void test_type1_r4_write_at_adjustment_entry_suppresses_r12_reload(
    TestBench& test) {
    // ACCC v1.10 section 11.2.4 note, page 84: rewriting R4 to a nonzero
    // value exactly at C0=R0 while entering adjustment suppresses the
    // special VMA-from-R12/R13 behavior for C4=1.  The saved VMA'=0x1238
    // must win over the newly programmed R12/R13=0x2050.
    prepare_type1_a2_exact_r0_write(test, 4, 1);
    test.expect_ma(
        "type 1 exact-R0 R4>0 write suppresses the C4=1 R12/R13 reload",
        0x1238);
}

void test_type1_r9_write_at_adjustment_entry_keeps_r12_reload(
    TestBench& test) {
    // ACCC v1.10 section 11.2.4 note, page 84 (findings-review B5): an R9
    // write on the same exact C0=R0 edge does NOT cancel the C4=1 special
    // case.  Therefore the new R12/R13=0x2050, not VMA'=0x1238, must load.
    prepare_type1_a2_exact_r0_write(test, 9, 2);
    test.expect_ma(
        "type 1 exact-R0 R9 write retains the C4=1 R12/R13 reload",
        0x2050);
}

// ---------------------------------------------------------------------------
// t13: F7 -- CRTC-1 Rupture For Dummies (RFD)
// ---------------------------------------------------------------------------

constexpr RegisterProgram kRfdRegisters = {{
    {0, 7}, {1, 4}, {2, 6}, {3, 0x11}, {4, 1},
    {5, 0}, {6, 2}, {7, 20}, {8, 0},   {9, 1},
}};

void test_type1_rfd_write_away_from_r0_stays_unarmed(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.select_register(5);
    test.reset();

    // ACCC v1.10 section 11.6, page 87: RFD requires the R5 0->nonzero
    // write to land exactly at C0==R0.  On paper, this write lands at C0=3
    // while R0=7, so neither RFD flag nor its parity state may move.  This
    // is the directed never-triggered regression that protects the
    // bit-identical-when-unarmed contract; the randomized soak is not that
    // proof because it legitimately exercises the trigger.
    test.run_characters(3);
    test.write_selected_register_at_clken(1);
    test.expect_type1_rfd_state(
        "type 1 off-R0 R5 write leaves RFD unarmed", false, false, false);
}

void test_type1_rfd_alternates_save_by_frame_parity(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(5);
    test.reset();

    // ACCC v1.10 section 11.6 and 11.6.3, pages 87-90: with R5 previously
    // zero and C9=0 != R9=1, the 0->1 write landing at C0=R0=7 arms both
    // independent RFD flags.  The current (reset) parity is case 1, so the
    // C9=R9-at-C0=R1 save is suppressed during this frame.
    test.run_characters(7);
    test.write_selected_register_at_clken(1);
    test.expect_type1_rfd_state(
        "type 1 exact-R0 R5 write arms both RFD flags", true, true, false);
    test.expect_ma(
        "type 1 exact-R0 R5 write reloads R12/R13 on the arming rollover",
        0x1234);
    // Complete the section 11.6.3 recipe's OUT R5,0 step before the normal
    // frame end so this vector isolates RFD from vertical adjustment.
    test.write_register(5, 0);

    // Change the base during C9=R9.  With the parity-gated VMA' save blocked,
    // the RFD VMA-source flag reloads R12/R13 on the next row even though
    // C4 becomes 1 (section 11.6.1, pages 88-89, case 1).
    test.write_register(12, 0x20);
    test.write_register(13, 0x50);
    test.run_characters(8);
    test.expect_c4("RFD case 1 advances to nonzero C4", 1);
    test.expect_ra("RFD case 1 begins the next row at C9=0", 0);
    test.expect_ma("RFD case 1 reloads R12/R13 on a nonzero row", 0x2050);
    test.expect_type1_rfd_state(
        "RFD case 1 leaves the VMA-source flag armed", true, true, false);

    // The tiny fixture has four lines per frame.  Two more line endings
    // reach C4=C9=C0=0; odd R9 toggles parity (section 11.6.1, pp.88-89).
    test.run_characters(16);
    test.expect_c4("RFD parity boundary resets C4", 0);
    test.expect_ra("RFD parity boundary resets C9", 0);
    test.expect_type1_rfd_state(
        "RFD odd-R9 frame boundary selects case 2", true, true, true);

    // In case 2, the next C9=R9/C0=R1 comparison succeeds.  The actual
    // VMA' save clears only the VMA-source flag; parity management remains
    // armed for subsequent frames (section 11.6.1, pp.88-89).
    test.run_characters(13);
    test.expect_type1_rfd_state(
        "RFD case 2 successful VMA' save disarms only the source flag",
        false, true, true);
}

void test_type1_rfd_r1_gt_r0_bare_c9_disarms(TestBench& test) {
    test.set_crtc_type(1);
    RegisterProgram registers = kRfdRegisters;
    registers[0].second = 3;
    registers[1].second = 5;
    program_registers(test, registers);
    test.select_register(5);
    test.reset();

    // ACCC v1.10 section 11.6, page 87 (findings-review B6): with R1>R0,
    // C0 can never reach R1, so the bare C9==R9 match must disarm the
    // VMA-source state.  Trigger on C9=0, then enter C9=R9=1; no VMA' save
    // is possible in this geometry.
    test.run_characters(3);
    test.write_selected_register_at_clken(1);
    test.expect_type1_rfd_state(
        "R1>R0 exact-R0 write initially arms RFD", true, true, false);
    test.run_clock_ticks(1);
    test.expect_type1_rfd_state(
        "R1>R0 bare C9 match disarms the RFD VMA-source flag",
        false, true, false);
}

void test_type1_rfd_final_line_write_enters_adjustment(TestBench& test) {
    test.set_crtc_type(1);
    RegisterProgram registers = kRfdRegisters;
    registers[4].second = 0;
    registers[9].second = 0;
    program_registers(test, registers);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(5);
    test.reset();

    // ACCC v1.10 sections 11.4 (p.86) and 11.6 (p.87): R5 is evaluated
    // live at C0=R0.  On paper C4=R4=0 and C9=R9=0 make this the final
    // normal line, so the same R5 0->1 write must both arm RFD and select
    // one adjustment line; using the old stored R5 would incorrectly start
    // a new frame instead.
    test.run_characters(7);
    test.write_selected_register_at_clken(1);
    test.expect_adjustment_active(
        "exact-R0 RFD write on the final line enters adjustment");
    test.expect_c4("exact-R0 final-line RFD write advances C4", 1);
    test.expect_ra("exact-R0 final-line RFD write resets C9", 0);
    test.expect_c5("exact-R0 final-line RFD write starts C5 at zero", 0);
    test.expect_ma(
        "exact-R0 final-line RFD write reloads R12/R13 on that rollover",
        0x1234);
    test.expect_type1_rfd_state(
        "exact-R0 final-line R5 write arms both RFD flags",
        true, true, false);
}

// ---------------------------------------------------------------------------
// t13e-t13k: F7 residual -- CRTC-1 R0-widening RFD trigger route
// (ACCC v1.10 section 13.7.1.2 p.124; digest-01 section 8.6)
//
// Fixture geometry, derived on paper: R0=7 gives 8 characters per line
// (C0=0..7), R4=1 gives rows C4={0,1}, R9=1 gives lines C9={0,1}, so the
// frame is 4 lines and its true last line carries C4=1, C9=1 with R5=0.
// After reset, run_characters(24) lands on that line's first character and
// run_characters(7) reaches C0=R0=7.  A CLKEN-aligned write there lands on
// the exact C0==R0 rollover edge, matching the OUT(C),reg8 alignment the
// recipe requires.
// ---------------------------------------------------------------------------

void test_type1_rfd_r0_widen_without_cancel_ends_normally(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 section 13.7.1.2 p.124: widening R0 exactly at C0==R0 on
    // the last line opens a trigger window, but arming additionally needs
    // the last-line condition to be cancelled during the widened remainder.
    // With no R9/R4 rewrite the window must expire harmlessly at the
    // extended line's actual end (C0=9 under the new R0) and the frame must
    // restart through the ordinary path: paper says C4 returns to 0, C9 to
    // 0, MA reloads from R12/R13 at the frame boundary, and neither RFD
    // flag ever sets.  The restart is a genuine C4=C9=C0=0 boundary with
    // odd R9, so section 11.6.1 frame-parity tracking still toggles even
    // though nothing is armed.
    test.run_characters(24);
    test.expect_c4("R0-widen fixture reaches the last row", 1);
    test.expect_ra("R0-widen fixture reaches the last line", 1);
    test.run_characters(7);
    test.write_selected_register_at_clken(9);
    test.expect_type1_rfd_pending(
        "widening R0 write on the last line opens the trigger window", true);
    test.expect_type1_rfd_state(
        "opening the window alone does not arm RFD", false, false, false);
    // The comparator match is overridden this rollover, so C0 runs 7 -> 8
    // -> 9 and the deferred line end fires on the second following edge.
    test.run_characters(2);
    test.expect_c4("window expiry ends the frame normally (C4)", 0);
    test.expect_ra("window expiry ends the frame normally (C9)", 0);
    test.expect_ma(
        "window expiry keeps the ordinary frame-start reload", 0x1234);
    test.expect_type1_rfd_pending("expired window closes", false);
    test.expect_type1_rfd_state(
        "no cancellation means no RFD arm", false, false, true);
}

void test_type1_rfd_r0_widen_r9_cancel_arms_at_extended_end(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 section 13.7.1.2 p.124, R9 variant ("C9 != R9 by line
    // end"): widen R0 7->9 on the last line, then raise R9 to 3 inside the
    // widened remainder.  At the extended end C9=1 no longer equals R9=3,
    // so RFD arms exactly there -- not earlier -- and behaves per section
    // 11.6: both flags set, VMA reloads from R12/R13 on that same rollover,
    // while C9 advances past the old value without a row wrap (paper:
    // C4 stays 1, C9 becomes 2).  The raised R9 extends the row by two
    // lines; on the first C9==R9 line the case-1 parity state suppresses
    // the C0=R1 VMA' save, so the VMA-source flag survives the next frame
    // boundary and odd R9 toggles the frame parity there.
    test.run_characters(24);
    test.run_characters(7);
    test.write_selected_register_at_clken(9);
    test.write_register(9, 3);
    // C0 continues 7 -> 8 -> 9; the cancellation write lands inside the
    // widened remainder and arming fires at the deferred line end.
    test.run_characters(2);
    test.expect_type1_rfd_state(
        "R9-cancelled widened last line arms both RFD flags",
        true, true, false);
    test.expect_type1_rfd_pending("arming closes the window", false);
    test.expect_ma("armed RFD reloads R12/R13 at the extended end", 0x1234);
    test.expect_c4("cancelled condition does not end the frame", 1);
    test.expect_ra("C9 runs past the old R9 without wrapping", 2);
    test.run_characters(20);
    test.expect_c4("raised R9 defers the frame end by one line pair", 0);
    test.expect_ra("deferred frame boundary resets C9", 0);
    test.expect_ma("frame start reloads R12/R13 as usual", 0x1234);
    test.expect_type1_rfd_state(
        "case-1 save suppression leaves the source flag armed",
        true, true, true);
}

void test_type1_rfd_r0_widen_r4_cancel_arms_and_advances_c4(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 section 13.7.1.2 p.124, R4 variant ("C4 != R4 by line
    // end, C9==R9 still held"): widen R0 7->9 on the last line, then raise
    // R4 to 2 inside the widened remainder.  At the extended end C9==R9
    // still holds so the row boundary fires, but C4=1 no longer matches
    // R4=2, so instead of a frame restart C4 advances past the old total
    // (paper: C4=2, C9 wraps to 0) and RFD arms with its same-edge
    // R12/R13 reload onto that new row start.  The frame then continues
    // until C4 genuinely reaches the raised R4.
    test.run_characters(24);
    test.run_characters(7);
    test.write_selected_register_at_clken(9);
    test.write_register(4, 2);
    // C0 continues 7 -> 8 -> 9; the R4 rewrite lands inside the widened
    // remainder and arming fires at the deferred line end, whose row
    // boundary now advances C4 instead of restarting the frame.
    test.run_characters(2);
    test.expect_type1_rfd_state(
        "R4-cancelled widened last line arms both RFD flags",
        true, true, false);
    test.expect_type1_rfd_pending("arming closes the window", false);
    test.expect_c4("cancelled condition advances C4 past old R4", 2);
    test.expect_ra("row boundary still resets C9", 0);
    test.expect_ma(
        "armed RFD reloads R12/R13 at the continued row start", 0x1234);
    test.run_characters(20);
    test.expect_c4("frame ends once C4 reaches the raised R4", 0);
    test.expect_type1_rfd_state(
        "case-1 save suppression leaves the source flag armed",
        true, true, true);
}

void test_type1_rfd_r0_widen_restored_condition_does_not_arm(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 section 13.7.1.2 p.124 defines both variants by the state
    // at the line's actual end ("C9 != R9 by line end", "C4 != R4 by line
    // end"), so a condition cancelled and then restored inside the widened
    // remainder must not arm: paper has the frame restart normally at the
    // extended boundary with no RFD flags set.  That restart is a genuine
    // C4=C9=C0=0 boundary with odd R9, so section 11.6.1 frame-parity
    // tracking toggles even though nothing is armed.
    test.run_characters(24);
    test.run_characters(7);
    test.write_selected_register_at_clken(9);
    test.write_register(9, 0);
    test.write_register(9, 1);
    // C0 continues 7 -> 8 -> 9; both writes land inside the widened
    // remainder, and the restored condition makes the deferred line end
    // an ordinary frame restart.
    test.run_characters(2);
    test.expect_type1_rfd_pending("window closes either way", false);
    test.expect_type1_rfd_state(
        "restored last-line condition does not arm RFD",
        false, false, true);
    test.expect_c4("restored condition ends the frame normally", 0);
    test.expect_ra("restored condition resets C9", 0);
    test.expect_ma(
        "ordinary frame-start reload survives restore", 0x1234);
}

void test_type1_rfd_equal_r0_write_opens_no_window(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 section 13.7.1.2 p.124 arms only when R0 is *widened*.
    // An equal-value write on the same edge changes nothing about the
    // comparator, which used the old R0: the last line still ends at that
    // exact edge and the frame restarts through the ordinary path, whose
    // genuine C4=C9=C0=0 boundary toggles odd-R9 parity tracking
    // (section 11.6.1).  With no extension window, a later ordinary R9
    // rewrite can only act through the generic equality rules (paper: the
    // new frame's first line ends with C9=0 != R9=3, so C4 keeps its reset
    // value 0).
    test.run_characters(24);
    test.run_characters(7);
    test.write_selected_register_at_clken(7);
    test.expect_type1_rfd_pending(
        "equal-value R0 write opens no trigger window", false);
    test.write_register(9, 3);
    test.run_characters(8);
    test.expect_type1_rfd_state(
        "no window means no arm regardless of later writes",
        false, false, true);
    test.expect_c4("frame already restarted at the write edge", 0);
}

void test_type1_rfd_r0_widen_off_last_line_never_arms(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 sections 13.3 p.113 and 13.7.1.2 p.124: any-R0 acceptance
    // is generic type-1 behaviour, but the RFD window requires the exact
    // last-line precondition (C4==R4, C9==R9, R5==0).  run_characters(15)
    // lands on C0=7==R0 of line 1, where the line half of the precondition
    // holds (C9=1==R9) but the row half fails (C4=0!=R4): widening R0
    // there accepts the new total without opening a window, so a
    // subsequent R4 rewrite cannot arm anything.  Paper: the write edge is
    // an ordinary line end for line 1 -- C9==R9 holds, so C4 advances to 1
    // and C9 wraps to 0 while R0 becomes 9, making line 2 ten characters
    // long; its end then defers any further boundary (C9=0 != live R9).
    test.run_characters(15);
    test.expect_c4("off-last-line fixture sits in row 0", 0);
    test.expect_ra("off-last-line fixture sits in line 1", 1);
    test.write_selected_register_at_clken(9);
    test.expect_type1_rfd_pending(
        "widening off the last line opens no window", false);
    test.expect_c4("line-1 wrap advances C4 at the write edge", 1);
    test.expect_ra("line-1 wrap resets C9", 0);
    test.write_register(4, 2);
    test.run_characters(10);
    test.expect_type1_rfd_state(
        "no window on a mid-frame widening", false, false, false);
    test.expect_c4("deferred boundary leaves the advanced C4", 1);
}

void test_type1_rfd_r0_window_does_not_survive_type_round_trip(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.select_register(0);
    test.reset();

    // The trigger window is hidden type-1 state; CRTC_TYPE is a live input
    // whose round-trip contract (t02j/t06d/t09f/t16l) forbids carrying
    // hidden state across a dwell on the other type.  Open the window,
    // cancel the condition, dwell on type 0, return, and let the extended
    // line end: paper requires NO arm despite the cancellation being in
    // place, with the counters continuing seamlessly (C9 -> 2).
    test.run_characters(24);
    test.run_characters(7);
    test.write_selected_register_at_clken(9);
    test.expect_type1_rfd_pending("window open before the switch", true);
    test.write_register(9, 3);
    test.run_clock_ticks(2);
    test.set_crtc_type(0);
    // Hidden flops observe the type change on the next CLOCK edge.
    test.run_clock_ticks(1);
    test.expect_type1_rfd_pending(
        "type-0 dwell clears the hidden window", false);
    test.expect_type1_rfd_state(
        "type-0 dwell clears the RFD flags", false, false, false);
    test.set_crtc_type(1);
    // C0 continues 7 -> 8 -> 9 across the round trip; with the window
    // cleared the deferred line end must behave as a plain cancelled
    // non-event: no arm, C9 advancing past the old value.
    test.run_characters(2);
    test.expect_type1_rfd_state(
        "window does not survive the round trip", false, false, false);
    test.expect_c4("counters continue across the round trip", 1);
    test.expect_ra("counters continue across the round trip", 2);
}

void test_type1_rfd_r0_widen_line_gate_never_arms(TestBench& test) {
    test.set_crtc_type(1);
    program_registers(test, kRfdRegisters);
    test.select_register(0);
    test.reset();

    // Companion to t13j exercising the other half of the section
    // 13.7.1.2 p.124 last-line precondition.  Paper: run_characters(23)
    // lands on C0=7==R0 of line 2, where the row half holds (C4=1==R4)
    // but the line half fails (C9=0!=R9).  Widening there opens no
    // window; the write edge wraps ordinarily (C9=0 does not match R9, so
    // no row boundary -- C4 keeps its value while C9 advances to 1), R0
    // becomes 9, and a later cancellation-flavoured R9 rewrite still has
    // nothing to act in.  The extended line ends with C9 advancing to 2.
    test.run_characters(23);
    test.expect_c4("line-gate fixture sits in row 1", 1);
    test.expect_ra("line-gate fixture sits in line 2", 0);
    test.write_selected_register_at_clken(9);
    test.expect_type1_rfd_pending(
        "widening with C9!=R9 opens no window", false);
    test.expect_c4("ordinary wrap leaves the row untouched", 1);
    test.expect_ra("ordinary wrap advances C9", 1);
    test.write_register(9, 3);
    test.run_characters(10);
    test.expect_type1_rfd_state(
        "no window means the rewrite cannot arm", false, false, false);
    test.expect_c4("no row boundary occurred anywhere", 1);
    test.expect_ra("extended line end advances C9", 2);
}

void test_type1_rfd_r0_extend_blanks_from_c0_r1(TestBench& test) {
    test.set_crtc_type(1);
    RegisterProgram registers = kRfdRegisters;
    registers[1].second = 8;
    program_registers(test, registers);
    test.select_register(0);
    test.reset();

    // ACCC v1.10 section 6.1.3 p.33: DISPEN enables at C0==0 and disables
    // at C0==R1.  With R1=8==R0_old+1, the section 13.7.1.2 continuation
    // makes the extended line genuinely reach C0==R1 in its first widened
    // character, so the display must blank exactly there (review finding
    // F-1).  Paper: one full fixture frame runs first so the vertical
    // display state is set; the trigger write lands on C0=7 of the last
    // line, whose whole pre-extension length stays displayed because R1
    // exceeds R0; the suppressed wrap carries C0 into 8==R1 and DE must
    // drop on that same edge; the deferred end restarts the frame and
    // display resumes at the next C0=0.
    test.run_characters(32);
    test.run_characters(24);
    test.expect_ra("DE fixture reaches the last line", 1);
    test.run_characters(7);
    test.expect_de_high("C0=7 is still displayed when R1 exceeds R0");
    test.write_selected_register_at_clken(9);
    test.expect_type1_rfd_pending(
        "widening opens the expected window", true);
    test.expect_de_low("display blanks entering widened C0=8==R1");
    test.run_characters(1);
    test.expect_de_low("widened remainder stays blanked");
    test.run_characters(1);
    test.expect_de_high("deferred end restarts display at C0=0");
}

void test_type0_normal_frame_reloads_at_frame_start_only(TestBench& test) {
    test.set_crtc_type(0);

    // Normal frame: 64 characters/line (R0=63), 39 character rows (R4=38),
    // 8 scanlines/row (R9=7), no vertical adjust (R5=0).
    // Total frame = (38+1)*(7+1) = 312 lines of 64 characters.
    constexpr RegisterProgram kNormalRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 38},
        {5, 0},  {6, 25}, {7, 30}, {8, 0},    {9, 7},
    }};
    program_registers(test, kNormalRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();

    // Complete the initial frame (312 lines of 64 characters) to reach the
    // first genuine frame start (C4=0, C9=0, C0=0) where frame_new asserts.
    test.run_characters(312 * 64);

    // ACCC v1.10 section 20.3.1 (page 242) and section 17.4.1 (page 182):
    // On CRTC 0, frame start (C4=0, C9=0, C0=0) initializes VMA and VMA' with R12/R13.
    test.expect_ma("type 0 normal frame: initial frame start MA loaded from R12/R13", 0x1234);

    // Advance into line 0 (10 characters), then write new R12/R13 values (consumes 1 character at C0=10).
    test.run_characters(10);
    write_r12_r13_character(test, 0x20, 0x50);

    // Complete line 0 (64 characters total, 64 - 11 = 53 remaining) to reach line 1 start (C4=0, C9=1, C0=0).
    test.run_characters(53);

    // ACCC v1.10 section 17.4.1 (page 182) and section 20.3.1 (page 242):
    // On CRTC 0, lines within the first row (C4=0, C9>0) reload VMA from VMA' (0x1234),
    // NOT from R12/R13 (0x2050).
    test.expect_ma("type 0 normal frame: line 1 (C9=1) reloads VMA' (0x1234) ignoring R12/R13 (0x2050)", 0x1234);

    // Advance across the remainder of the frame (311 lines of 64 chars) to the next frame start (C4=0, C9=0, C0=0).
    test.run_characters(311 * 64);

    // ACCC v1.10 section 20.3.1 (page 242):
    // At the start of the new frame (C4=0, C9=0, C0=0), VMA and VMA' are loaded with R12/R13 (0x2050).
    test.expect_ma("type 0 normal frame: frame 1 start (C4=0, C9=0, C0=0) reloads new R12/R13", 0x2050);
}

void test_type1_normal_frame_reloads_every_line_of_row0(TestBench& test) {
    test.set_crtc_type(1);

    // Normal frame: 64 characters/line (R0=63), 39 character rows (R4=38),
    // 8 scanlines/row (R9=7), no vertical adjust (R5=0).
    constexpr RegisterProgram kNormalRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 38},
        {5, 0},  {6, 25}, {7, 30}, {8, 0},    {9, 7},
    }};
    program_registers(test, kNormalRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();

    // Complete the initial frame (312 lines of 64 characters) to reach the
    // first genuine frame start (C4=0, C9=0, C0=0).
    test.run_characters(312 * 64);

    // ACCC v1.10 section 20.3.2 (page 242) and section 17.4.2 (page 182):
    // On CRTC 1, line 0 of row 0 (C4=0, C9=0, C0=0) loads VMA with R12/R13 (0x1234).
    test.expect_ma("type 1 normal frame: line 0 (C4=0, C9=0) MA loaded from R12/R13", 0x1234);

    // Advance into line 0, write new R12/R13 during C0=10 (1 char), then complete line 0 (53 chars).
    test.run_characters(10);
    write_r12_r13_character(test, 0x20, 0x50);
    test.run_characters(53);

    // ACCC v1.10 section 17.4.2 (page 182) and section 20.3.2 (page 242):
    // On CRTC 1, VMA is reloaded from R12/R13 on EVERY line while C4=0.
    test.expect_ma("type 1 normal frame: line 1 (C4=0, C9=1) reloads new R12/R13 (0x2050)", 0x2050);

    // Advance into line 1, write new R12/R13 during C0=10 (1 char), then complete line 1 (53 chars).
    test.run_characters(10);
    write_r12_r13_character(test, 0x30, 0x78);
    test.run_characters(53);

    // ACCC v1.10 section 20.3.2 (page 242):
    // Line 2 (C4=0, C9=2) reloads new R12/R13 (0x3078).
    test.expect_ma("type 1 normal frame: line 2 (C4=0, C9=2) reloads new R12/R13 (0x3078)", 0x3078);

    // Advance through lines 2, 3, 4, 5, 6 (5 lines = 5 * 64 = 320 chars) to reach line 7 (C4=0, C9=7=R9).
    test.run_characters(5 * 64);
    test.expect_ma("type 1 normal frame: line 7 (C4=0, C9=7) reloads R12/R13 (0x3078)", 0x3078);

    // On line 7 (last line of row 0, C9=R9), advance past C0=R1=40 where VMA' latches VMA:
    // VMA at C0=R1 is 0x3078 + 40 = 0x30A0. VMA' becomes 0x30A0.
    test.run_characters(50);

    // Write new R12/R13 (0x0111) during C0=50 (1 char).
    write_r12_r13_character(test, 0x01, 0x11);

    // Complete line 7 (64 - 51 = 13 chars remaining) to reach line 8 start (row 1, C4=1, C9=0, C0=0).
    test.run_characters(13);

    // ACCC v1.10 section 17.4.2 (page 182) and section 20.3.2 (page 242):
    // Once C4 > 0, CRTC 1 no longer reloads from R12/R13 (0x0111); VMA reloads from VMA' (0x30A0).
    test.expect_ma("type 1 normal frame: row 1 (C4=1, C9=0) reloads VMA' (0x30A0) refusing R12/R13 (0x0111)", 0x30A0);
}

void test_type0_r0_three_reloads_every_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 13.8.1 (page 127) and section 20.3.1 (page 242):
    // R0=3, R4=0, R9=0, R5=0 (4 us lines).
    // C0 reaches 2, so the C0=2 disarm check cancels vertical adjustment.
    // C4 stays 0 on every line; each line is a new frame start (C4=0, C9=0, C0=0).
    // VMA and VMA' reload from R12/R13 on every 4 us line.
    constexpr RegisterProgram kR0ThreeRegisters = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kR0ThreeRegisters);
    test.write_register(12, 0x10);
    test.write_register(13, 0x20);
    test.reset();

    // Complete initial line (4 characters) to reach the first frame boundary.
    test.run_characters(4);

    // ACCC v1.10 section 20.3.1 page 242:
    // Line 1 start (C0=0): MA loaded from R12/R13 (0x1020).
    test.expect_ma("type 0 R0=3 line 1 MA loaded from R12/R13", 0x1020);

    // Advance to C0=1 (1 char), write new R13 during C0=1 (1 char), complete line 1 (4 - 2 = 2 chars).
    test.run_characters(1);
    write_r13_character(test, 0x45);
    test.run_characters(2);

    // ACCC v1.10 section 13.8.1 page 127:
    // Line 2 start (C0=0): reloads updated R12/R13 (0x1045).
    test.expect_ma("type 0 R0=3 line 2 MA reloaded with updated R13 (0x1045)", 0x1045);

    // Advance to C0=1 (1 char), write new R12/R13 during C0=1 (1 char), complete line 2 (2 chars).
    test.run_characters(1);
    write_r12_r13_character(test, 0x23, 0x67);
    test.run_characters(2);

    // ACCC v1.10 section 13.8.1 page 127:
    // Line 3 start (C0=0): reloads updated R12/R13 (0x2367).
    test.expect_ma("type 0 R0=3 line 3 MA reloaded with updated R12/R13 (0x2367)", 0x2367);

    // Advance 4 characters to line 4 start without writing new registers.
    test.run_characters(4);

    // ACCC v1.10 section 13.8.1 page 127:
    // Line 4 start (C0=0): reloads 0x2367 rather than advancing sequentially.
    test.expect_ma("type 0 R0=3 line 4 MA reloads 0x2367 at line start", 0x2367);
}

void test_type1_r0_three_reloads_every_line(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 section 13.8.1 (page 127) and section 20.3.2 (page 242):
    // R0=3, R4=0, R9=0, R5=0 (4 us lines).
    // CRTC 1 keeps C4=0 throughout; VMA reloads from R12/R13 on every 4 us line at C0=0.
    constexpr RegisterProgram kR0ThreeRegisters = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kR0ThreeRegisters);
    test.write_register(12, 0x10);
    test.write_register(13, 0x20);
    test.reset();

    // Complete initial line (4 characters) to reach the first line boundary.
    test.run_characters(4);

    // ACCC v1.10 section 20.3.2 page 242:
    // Line 1 start (C0=0): MA loaded from R12/R13 (0x1020).
    test.expect_ma("type 1 R0=3 line 1 MA loaded from R12/R13", 0x1020);

    // Advance to C0=1 (1 char), write new R13 during C0=1 (1 char), complete line 1 (2 chars).
    test.run_characters(1);
    write_r13_character(test, 0x45);
    test.run_characters(2);

    // ACCC v1.10 section 13.8.1 page 127:
    // Line 2 start (C0=0): reloads updated R12/R13 (0x1045).
    test.expect_ma("type 1 R0=3 line 2 MA reloaded with updated R13 (0x1045)", 0x1045);

    // Advance to C0=1 (1 char), write new R12/R13 during C0=1 (1 char), complete line 2 (2 chars).
    test.run_characters(1);
    write_r12_r13_character(test, 0x23, 0x67);
    test.run_characters(2);

    // ACCC v1.10 section 13.8.1 page 127:
    // Line 3 start (C0=0): reloads updated R12/R13 (0x2367).
    test.expect_ma("type 1 R0=3 line 3 MA reloaded with updated R12/R13 (0x2367)", 0x2367);

    // Advance 4 characters to line 4 start without writing new registers.
    test.run_characters(4);

    // ACCC v1.10 section 13.8.1 page 127:
    // Line 4 start (C0=0): reloads 0x2367 rather than advancing sequentially.
    test.expect_ma("type 1 R0=3 line 4 MA reloads 0x2367 at line start", 0x2367);
}

void test_type0_r0_one_reloads_every_second_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 13.8.2 (page 128), 13.2.5 (page 107), and 20.3.1 (page 242):
    // R0=1, R4=0, R9=0, R5=0 (2 us lines).
    // C0 never reaches 2, so the disarm check never runs and an uncancelled 1-line
    // vertical adjustment fires every frame.
    // Line 0 (C4=0, 2 us): frame line.
    // Line 1 (C4=1, 2 us): adjustment line.
    // Line 2 (C4=0, 2 us): frame 1 start. Reloads from R12/R13 (0x1100) at C0=0.
    //   During line 2: C0=0 (MA=0x1100), C0=1=R1 (MA=0x1101, VMA' latches 0x1101).
    // Line 3 (C4=1, 2 us): adjustment line. C4=1, so R12/R13 update (0x2233) is REFUSED.
    //   VMA reloads from VMA' (0x1101).
    // Line 4 (C4=0, 2 us): frame 2 start. Reloads from R12/R13 (0x2233) at C0=0.
    //   During line 4: C0=0 (MA=0x2233), C0=1=R1 (MA=0x2234, VMA' latches 0x2234).
    // Line 5 (C4=1, 2 us): adjustment line. R12/R13 update (0x3344) is REFUSED.
    //   VMA reloads from VMA' (0x2234).
    // Line 6 (C4=0, 2 us): frame 3 start. Reloads from R12/R13 (0x3344) at C0=0.
    // R12/R13 updates are therefore accepted only every 4 us (every second 2 us line).
    constexpr RegisterProgram kR0OneRegisters = {{
        {0, 1}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kR0OneRegisters);
    test.write_register(12, 0x11);
    test.write_register(13, 0x00);
    test.reset();

    // Complete initial frame + adjustment line (2 lines = 4 characters) to reach
    // frame 1 line 0 (C4=0, C0=0) where frame_new asserts.
    test.run_characters(4);

    // ACCC v1.10 section 20.3.1 page 242:
    // Line 2 start (C4=0, C0=0): MA loaded from initial R12/R13 (0x1100).
    test.expect_c4("type 0 R0=1 reaches frame 1 start (C4=0)", 0);
    test.expect_ma("type 0 R0=1 line 2 (C4=0) MA loaded from R12/R13", 0x1100);

    // Write new R12/R13 during C0=0 of line 2 (1 char), then advance remaining 1 char (C0=1) to line 3.
    // At C0=1=R1, VMA advances to 0x1101 and VMA' latches 0x1101.
    write_r12_r13_character(test, 0x22, 0x33);
    test.run_characters(1);

    // ACCC v1.10 section 13.2.5 page 107 and section 13.8.2 page 128:
    // Line 3 is in vertical adjustment (C4=1), so R12/R13 update is REFUSED.
    // VMA reloads from VMA' (0x1101), NOT from R12/R13 (0x2233).
    test.expect_c4("type 0 R0=1 line 3 enters uncancelled vertical adjustment (C4=1)", 1);
    test.expect_ma("type 0 R0=1 line 3 (C4=1) refuses R12/R13 update (reloads VMA' 0x1101)", 0x1101);

    // Advance 2 characters to line 4 start (C4=0, C0=0, frame 2 line 0).
    test.run_characters(2);

    // ACCC v1.10 section 13.2.5 page 107 and section 20.3.1 page 242:
    // Line 4 (C4=0, C0=0): new frame line accepts R12/R13 (0x2233).
    test.expect_c4("type 0 R0=1 line 4 returns to C4=0", 0);
    test.expect_ma("type 0 R0=1 line 4 (C4=0) accepts R12/R13 reload (0x2233)", 0x2233);

    // Write new R12/R13 during C0=0 of line 4 (1 char), then advance remaining 1 char (C0=1) to line 5.
    // At C0=1=R1, VMA advances to 0x2234 and VMA' latches 0x2234.
    write_r12_r13_character(test, 0x33, 0x44);
    test.run_characters(1);

    // ACCC v1.10 section 13.2.5 page 107:
    // Line 5 (C4=1): adjustment line refuses R12/R13 (0x3344); VMA reloads from VMA' (0x2234).
    test.expect_c4("type 0 R0=1 line 5 enters adjustment (C4=1)", 1);
    test.expect_ma("type 0 R0=1 line 5 (C4=1) refuses R12/R13 update (reloads VMA' 0x2234)", 0x2234);

    // Advance 2 characters to line 6 start (C4=0, C0=0, frame 3 line 0).
    test.run_characters(2);

    // ACCC v1.10 section 13.2.5 page 107 and section 20.3.1 page 242:
    // Line 6 (C4=0, C0=0): new frame line accepts R12/R13 (0x3344).
    test.expect_c4("type 0 R0=1 line 6 returns to C4=0", 0);
    test.expect_ma("type 0 R0=1 line 6 (C4=0) accepts R12/R13 reload (0x3344)", 0x3344);
}

void test_type1_r0_one_reloads_every_line(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 section 13.8.2 (page 128) and section 20.3.2 (page 242):
    // R0=1, R4=0, R9=0, R5=0 (2 us lines).
    // CRTC 1 keeps C4=0 throughout; VMA reloads from R12/R13 on EVERY 2 us line at C0=0.
    constexpr RegisterProgram kR0OneRegisters = {{
        {0, 1}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kR0OneRegisters);
    test.write_register(12, 0x11);
    test.write_register(13, 0x00);
    test.reset();

    // Complete initial line (2 characters) so CRTC 1 line_new fires.
    test.run_characters(2);

    // ACCC v1.10 section 20.3.2 page 242:
    // Line 1 start (C0=0): MA loaded from initial R12/R13 (0x1100).
    test.expect_c4("type 1 R0=1 line 1 C4 is 0", 0);
    test.expect_ma("type 1 R0=1 line 1 MA loaded from R12/R13", 0x1100);

    // Write new R12/R13 during C0=0 of line 1 (1 char), then advance remaining 1 char to line 2.
    write_r12_r13_character(test, 0x22, 0x33);
    test.run_characters(1);

    // ACCC v1.10 section 13.8.2 page 128 & section 20.3.2 page 242:
    // CRTC 1 keeps C4=0 and reloads VMA on every 2 us line.
    test.expect_c4("type 1 R0=1 keeps C4 at 0 on line 2", 0);
    test.expect_ma("type 1 R0=1 line 2 reloads R12/R13 (0x2233) immediately", 0x2233);

    // Write new R12/R13 during C0=0 of line 2 (1 char), then advance remaining 1 char to line 3.
    write_r12_r13_character(test, 0x33, 0x44);
    test.run_characters(1);

    // ACCC v1.10 section 13.8.2 page 128:
    test.expect_c4("type 1 R0=1 keeps C4 at 0 on line 3", 0);
    test.expect_ma("type 1 R0=1 line 3 reloads R12/R13 (0x3344) immediately", 0x3344);

    // Write new R12/R13 during C0=0 of line 3 (1 char), then advance remaining 1 char to line 4.
    write_r12_r13_character(test, 0x05, 0x67);
    test.run_characters(1);

    // ACCC v1.10 section 13.8.2 page 128:
    test.expect_c4("type 1 R0=1 keeps C4 at 0 on line 4", 0);
    test.expect_ma("type 1 R0=1 line 4 reloads R12/R13 (0x0567) immediately", 0x0567);
}

void test_type0_r0_zero_ignores_reload_after_hiccup(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 13.8.3 (page 129), 13.2.6 (page 108), and 20.3.1 (page 242):
    // R0=0, R4=0, R9=0, R5=0 (1 us lines).
    // On CRTC 0:
    // Character 0 (C0=0): frame start, C4=0, C9=0.
    // Character 1 (C0=0): "C4's last hiccup" increments C4 to 1, entering additional management.
    // C9 freezes at 0. All counters except C0 stop.
    // Because C4 remains stuck at 1, C4=0 never recurs while R0=0.
    // Section 13.8.3 p. 129: "R12 / R13 cannot be considered until C4 and C9 both go back to 0."
    // All subsequent R12/R13 writes are IGNORED; MA remains stuck at 0.
    constexpr RegisterProgram kR0ZeroRegisters = {{
        {0, 0}, {1, 0}, {2, 0}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kR0ZeroRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();

    // ACCC v1.10 section 13.2.6 page 108:
    // Character 0: initial reset state.
    test.expect_ma("type 0 R0=0 character 0 initial MA", 0);

    // Advance 1 character (1 us) to character 1: C4 takes last hiccup increment to 1.
    test.run_characters(1);
    test.expect_c4("type 0 R0=0 character 1 consumes last-hiccup C4 increment", 1);
    test.expect_ma("type 0 R0=0 character 1 MA remains 0", 0);

    // Advance 1 character (1 us) to character 2: all counters are frozen.
    test.run_characters(1);
    test.expect_c4("type 0 R0=0 character 2 C4 remains frozen at 1", 1);
    test.expect_ma("type 0 R0=0 character 2 MA remains 0", 0);

    // Write new R12/R13 values (0x2055) while frozen at R0=0 (consumes 1 character at character 2).
    write_r12_r13_character(test, 0x20, 0x55);

    // Advance 20 1-us lines.
    test.run_characters(20);

    // ACCC v1.10 section 13.8.3 page 129:
    // R12/R13 write is ignored because C4 is stuck at 1.
    test.expect_c4("type 0 R0=0 C4 remains 1 across 20 lines", 1);
    test.expect_ma("type 0 R0=0 ignores R12/R13 (0x2055) write while frozen (MA remains 0)", 0);

    // Write another R12/R13 value (0x3ABC).
    write_r12_r13_character(test, 0x3A, 0xBC);

    // Advance another 20 1-us lines.
    test.run_characters(20);

    // Negative assertion: MA remains 0 throughout.
    test.expect_ma("type 0 R0=0 continues ignoring R12/R13 (0x3ABC) write (MA remains 0)", 0);
}

// Review action item A3 (docs/review-debt.md): live-entry twin of t20g.
// t20g reaches the R0=0 state by cold reset, where no counter edge has run
// before the freeze pins C0, so the section 20.3.1 reload never fires and
// MA stays at its reset value. The actual section 13.2.6 setup is reached
// by writing R0=0 while the frame runs; this vector pins that path.
void test_type0_r0_zero_live_entry_reloads_vma_then_freezes(TestBench& test) {
    test.set_crtc_type(0);

    // Running fixture with the freeze conditions armed: R4=R9=R5=0 makes
    // every 8-character line (R0=7) a complete one-character-row frame, so
    // C4=C9=0 hold throughout and the seam equality tests (C4==R4, C9==R9)
    // are already satisfied when R0 is written to 0.
    constexpr RegisterProgram kLiveR0ZeroRegisters = {{
        {0, 7}, {1, 0}, {2, 0}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kLiveR0ZeroRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    // Leave R0 selected so the timed write below needs no bus traffic that
    // would disturb the character alignment (reset() re-aligns the phase).
    test.select_register(0);
    test.reset();

    // Land R0:=0 on the seventh character boundary, i.e. exactly on the
    // wrap edge of an 8-character line: the line end is evaluated with
    // the OLD R0 (the register file updates on the same edge), so this line
    // still ends normally. The seam condition C4=C9=C0=0 therefore recurs
    // one last time and ACCC v1.10 section 20.3.1 (page 242) loads both
    // VMA' and VMA from R12/R13 -- this is the worked example's "1st
    // C0==0 -> VMA reload" of section 13.2.6 (page 108), realized at the
    // live wrap edge. Immediately afterwards R0=0 pins C0 and freezes.
    // (Expectations deliberately start at this reload: the pre-wrap pointer
    // value depends on how many CLKEN edges elapsed around reset, which no
    // Compendium rule covers; the reload overwrites it wholesale.)
    test.run_characters(7);
    test.write_selected_register_at_clken(0);
    test.expect_ma("type 0 live R0=0 entry: wrap-edge reload loads R12/R13 (0x1234)",
                   0x1234);

    // ACCC v1.10 section 13.2.6 (page 108), live-entry form: on the first
    // repeated C0==0 the armed C9==R9 decision consumes its C4 increment
    // exactly once ("this IS end of frame -> C4->1, adjustment entered")
    // while C9 does not truly reset -- it freezes at 0.
    test.run_characters(1);
    test.expect_c4("type 0 live R0=0: first frozen C0==0 consumes the armed C4 increment",
                   1);
    test.expect_ra("type 0 live R0=0: C9 stays frozen at 0", 0);
    test.expect_ma("type 0 live R0=0: MA holds the reloaded 0x1234", 0x1234);

    // Further C0==0 cycles: everything stays frozen at C4=1, C9=0
    // (ACCC v1.10 section 13.2.6, page 108).
    test.run_characters(1);
    test.expect_c4("type 0 live R0=0: C4 remains frozen at 1", 1);
    test.expect_ma("type 0 live R0=0: MA remains 0x1234", 0x1234);

    // ACCC v1.10 section 13.8.3 (page 129): R12/R13 cannot be considered
    // until C4 and C9 both go back to 0 -- they never do while R0=0. Same
    // negative pair as t20g, now guarding a non-zero latched pointer.
    write_r12_r13_character(test, 0x20, 0x55);
    test.run_characters(19);
    test.expect_c4("type 0 live R0=0: C4 still 1 across 20 characters", 1);
    test.expect_ma("type 0 live R0=0 ignores R12/R13 0x2055 (MA remains 0x1234)",
                   0x1234);

    write_r12_r13_character(test, 0x3A, 0xBC);
    test.run_characters(20);
    test.expect_ma("type 0 live R0=0 ignores R12/R13 0x3ABC (MA remains 0x1234)",
                   0x1234);
}

// F11h closure, render-verified against ACCC v1.10 section 20.3.2 page 242:
// the second CRTC-1 chronogram draws the OUT R12,#30 bus activity spanning
// C0=62..1 across a row-0 line seam, so the register write lands on the
// 63->0 boundary edge itself, and OFFSET=#30xx is drawn from C0=0. The
// paired CRTC-0 chronogram (section 20.3.1, same page) with identical bus
// timing leaves OFFSET=#10xx. So the type-1 row-0 reload ("VMA is loaded
// with R12/R13 while C4=0") catches a write whose register update coincides
// with the reload edge, while the type-0 frame load misses it. Expectations
// derived on paper from the chronograms.
void test_type1_r12_write_on_row0_boundary_edge_reloads(TestBench& test) {
    test.set_crtc_type(1);

    // Normal frame: 64 characters/line (R0=63), 39 character rows (R4=38),
    // 8 scanlines/row (R9=7), no vertical adjust (R5=0): 312 lines/frame.
    constexpr RegisterProgram kNormalRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 38},
        {5, 0},  {6, 25}, {7, 30}, {8, 0},    {9, 7},
    }};
    program_registers(test, kNormalRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    // Leave R12 selected so the timed writes below need no bus traffic that
    // would disturb the character alignment (reset() re-aligns the phase).
    test.select_register(12);
    test.reset();

    // 312*64 edges reach the genuine frame start; 63 more land on cell 63
    // of line 0, so the write below consumes the line 0 -> line 1 boundary
    // edge itself. MA has incremented once per edge since the origin.
    test.run_characters(312 * 64 + 63);
    test.expect_ma("type 1 fixture reaches line 0 cell 63 running from R12/R13",
                   0x1234 + 63);
    test.write_selected_register_at_clken(0x30);

    // Section 20.3.2 chronogram 2: the same-edge write is caught -- the new
    // R12 (0x30) with the still-stored R13 (0x34) loads at this boundary.
    test.expect_ma(
        "type 1 same-edge R12 write reloads (new R12, stored R13)", 0x3034);

    // The next row-0 boundary reloads the stored pair; this also pins that
    // the pre-fix model only caught the write one full line later.
    test.run_characters(64);
    test.expect_ma("type 1 next row-0 boundary carries the written R12", 0x3034);

    // Mid-line writes keep the t20b behavior (regression continuity inside
    // this vector): the R13 write lands on the edge entering cell 10 of
    // line 2 (the phase-1 select consumes no CLKEN edge), and the next
    // boundary reloads the updated pair.
    test.select_register(13);
    test.run_characters(9);
    test.write_selected_register_at_clken(0x78);
    test.run_characters(54);
    test.expect_ma("type 1 mid-line R13 write still reloads at the boundary",
                   0x3078);

    // The frame origin is the same "C0 and C9 go to 0 and C4=0" event, so a
    // write landing exactly there is caught by the same rule. R12 is 6 bits
    // (DI[5:0]); the written 0x15 stays inside that range.
    test.select_register(12);
    test.run_characters(309 * 64 - 1);
    test.write_selected_register_at_clken(0x15);
    test.expect_ma(
        "type 1 same-edge R12 write at the frame origin reloads", 0x1578);
}

void test_type0_r12_write_on_frame_origin_edge_is_missed(TestBench& test) {
    test.set_crtc_type(0);

    // Same normal frame as t20a/t20j.
    constexpr RegisterProgram kNormalRegisters = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 38},
        {5, 0},  {6, 25}, {7, 30}, {8, 0},    {9, 7},
    }};
    program_registers(test, kNormalRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.select_register(12);
    test.reset();

    // 311*64+63 edges land on cell 63 of the frame's last line (line 311,
    // C9=7=R9 of row 38=R4), so the write below consumes the frame-origin
    // edge itself.
    test.run_characters(311 * 64 + 63);
    test.write_selected_register_at_clken(0x30);

    // Section 20.3.1 chronogram 2: the C4=C0=0 frame load samples the old
    // R12/R13; the identically-timed write type 1 catches is missed here.
    test.expect_ma(
        "type 0 same-edge R12 write at the frame origin is not caught",
        0x1234);

    // "The updates of R12 and R13 are considered immediately" at the next
    // C4=C0=0: one frame later the written R12 loads.
    test.run_characters(312 * 64);
    test.expect_ma("type 0 next frame origin loads the written R12", 0x3034);
}

void test_type1_r0_zero_reloads_every_line(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 sections 13.3 (page 113), 13.8.3 (page 129), and 20.3.2 (page 242):
    // R0=0, R4=0, R9=0, R5=0 (1 us lines).
    // On CRTC 1, R0=0 does not freeze counters; C9 and R4 continue to be managed normally.
    // C4 stays 0 throughout.
    // VMA is loaded with R12/R13 on EVERY 1 us line at C0=0.
    constexpr RegisterProgram kR0ZeroRegisters = {{
        {0, 0}, {1, 0}, {2, 0}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    program_registers(test, kR0ZeroRegisters);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();

    // Complete initial line (1 character) so line_new asserts.
    test.run_characters(1);

    // ACCC v1.10 section 20.3.2 page 242:
    // Line 1 start (C0=0): MA loaded from initial R12/R13 (0x1234).
    test.expect_ma("type 1 R0=0 line 1 MA loaded from R12/R13", 0x1234);

    // Write new R13 (0x35) during line 1 (1 char), then advance to line 2 start (1 char).
    write_r13_character(test, 0x35);
    test.run_characters(1);

    // ACCC v1.10 section 13.8.3 page 129 & section 20.3.2 page 242:
    // Line 2 start (C0=0): reloads updated R13 (0x1235).
    test.expect_c4("type 1 R0=0 keeps C4 at 0 on line 2", 0);
    test.expect_ma("type 1 R0=0 line 2 reloads updated R13 (0x1235)", 0x1235);

    // Write new R13 (0x36) during line 2 (1 char), then advance to line 3 start (1 char).
    write_r13_character(test, 0x36);
    test.run_characters(1);

    // Line 3 start (C0=0): reloads updated R13 (0x1236).
    test.expect_c4("type 1 R0=0 keeps C4 at 0 on line 3", 0);
    test.expect_ma("type 1 R0=0 line 3 reloads updated R13 (0x1236)", 0x1236);

    // Write new R12 (0x25) and R13 (0x80) during line 3 (1 char), then advance to line 4 start (1 char).
    write_r12_r13_character(test, 0x25, 0x80);
    test.run_characters(1);

    // Line 4 start (C0=0): reloads updated R12/R13 (0x2580).
    test.expect_c4("type 1 R0=0 keeps C4 at 0 on line 4", 0);
    test.expect_ma("type 1 R0=0 line 4 reloads updated R12/R13 (0x2580)", 0x2580);

    // Advance 5 characters (5 us) to line 9 start without writing new registers.
    test.run_characters(5);

    // ACCC v1.10 section 13.8.3 page 129:
    // Line 9 start (C0=0): continues reloading 0x2580 on every 1 us line.
    test.expect_c4("type 1 R0=0 keeps C4 at 0 on line 9", 0);
    test.expect_ma("type 1 R0=0 line 9 continues reloading 0x2580", 0x2580);
}

// ---------------------------------------------------------------------------
// t10: F6 -- spurious type-0 interline border byte when R1 > R0
//
// ACCC v1.10 section 17.6.2 (page 186): when R1 > R0 the C0=R1 DISPTMG-off
// comparison can never fire (C0 wraps at R0 first), so a type-0 CRTC
// substitutes C0=R0 as the border-start trigger. The source describes the
// resulting border byte as 0.5 us, but this character-granular Stage 1 bench
// deliberately pins DE low for the full 1 us character containing that
// trigger; these assertions are for that accepted approximation, not exact
// ACCC pin timing. F13 remains hardware-blocked for the half-character
// distinction. Type 1 emits nothing at all in this configuration (pages
// 186-187) -- the documented type discriminator of section 28.1.6. Section
// 19.2.4 (page 195) counts a programmed SKEW-DISPTMG delay from the
// substituted trigger as if C0=R1 had fired there, so delay=1/2 displaces the
// approximated full-character blank onto C0=0/C0=1 of the following line
// (delay arithmetic per section 19.2.3, pages 193-194), and SKEW mode 2'b11
// suppresses all DISPTMG output entirely.
// ---------------------------------------------------------------------------

constexpr unsigned kF6LineCharacters = 16;  // R0 = 15
constexpr unsigned kF6FrameLines = 39 * 8;  // R4 = 38 rows x (R9 = 7)+1 lines

RegisterProgram f6_registers(unsigned skew) {
    return {{
        {0, 15}, {1, 20}, {2, 10}, {3, 0x11}, {4, 38},
        {5, 0},  {6, 25}, {7, 30}, {8, static_cast<std::uint8_t>(skew << 4)},
        {9, 7},
    }};
}

// Walk one full 16-character line, sampling DE during each character. Must
// be entered mid character C0=0 (e.g. right after a wrap edge) and leaves
// the bench mid character C0=15.
void f6_expect_line_de(TestBench& test,
                       const std::array<bool, kF6LineCharacters>& display,
                       const std::string& context) {
    for (unsigned c0 = 0; c0 < kF6LineCharacters; ++c0) {
        std::ostringstream label;
        label << context << " [C0=" << c0 << "]";
        if (display[c0]) {
            test.expect_de_high(label.str());
        } else {
            test.expect_de_low(label.str());
        }
        if (c0 + 1 < kF6LineCharacters) {
            test.run_characters(1);
        }
    }
}

// Display runs through every character of the line; the only gap is the
// substituted-trigger border byte itself.
constexpr std::array<bool, kF6LineCharacters> kF6SpuriousByteAtR0 = {
    true, true, true, true, true, true, true, true,
    true, true, true, true, true, true, true, false,
};
constexpr std::array<bool, kF6LineCharacters> kF6AllDisplay = {
    true, true, true, true, true, true, true, true,
    true, true, true, true, true, true, true, true,
};
constexpr std::array<bool, kF6LineCharacters> kF6SpuriousByteDelayed1 = {
    false, true, true, true, true, true, true, true,
    true, true, true, true, true, true, true, true,
};
constexpr std::array<bool, kF6LineCharacters> kF6SpuriousByteDelayed2 = {
    true, false, true, true, true, true, true, true,
    true, true, true, true, true, true, true, true,
};

void f6_settle_one_frame(TestBench& test, unsigned skew) {
    // Run one complete frame (39 rows x 8 lines of 16 characters) plus two
    // more lines, so the frame-start reload of ACCC v1.10 section 17.4.1
    // (page 182) has established the R12/R13 base pointer AND both
    // SKEW-DISPTMG delay stages carry current-frame state rather than the
    // previous frame's tail-of-frame border rows (R6 < R4 here). Leaves the
    // bench mid character C0=0 of the third displayed line.
    program_registers(test, f6_registers(skew));
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.reset();
    test.run_characters((kF6FrameLines + 2) * kF6LineCharacters);
}

void test_type0_r1_gt_r0_spurious_border_byte(TestBench& test) {
    test.set_crtc_type(0);
    f6_settle_one_frame(test, 0);

    // ACCC v1.10 section 17.4.1 (page 182): frame start reloaded VMA/VMA'
    // from R12/R13; section 17.2 (page 179): with C0=R1 unreachable, every
    // line of the frame restarts from that same frozen base.
    test.expect_ma("type 0 R1>R0 line start holds the R12/R13 base", 0x1234);
    test.expect_de_high("type 0 R1>R0 displays at C0=0 despite R1>R0");

    // ACCC v1.10 section 17.6.1 (page 185-186): the VRAM pointer offset
    // continues normally into the border byte.
    test.run_characters(15);
    test.expect_ma("type 0 R1>R0 pointer keeps counting through the border byte",
                   0x1234 + 15);
    test.expect_de_low(
        "type 0 R1>R0 spurious border byte keyed on C0=R0 "
        "(ACCC v1.10 section 17.6.2 p.186)");

    // ACCC v1.10 section 17.6.2 (page 186): BORDER OFF on the character
    // following C0=R0. Section 17.2 (page 179): C0=R1 never fired, so VMA'
    // was never updated and this line restarts from the same base address
    // (character-line repetition).
    test.run_characters(1);
    test.expect_ma("type 0 R1>R0 next line restarts from the frozen VMA' base",
                   0x1234);
    test.expect_de_high(
        "type 0 R1>R0 BORDER OFF on the character following C0=R0 "
        "(ACCC v1.10 section 17.6.2 p.186)");

    // The byte recurs between every pair of character rows.
    f6_expect_line_de(test, kF6SpuriousByteAtR0,
                      "type 0 R1>R0 spurious border byte recurs per line");
    test.run_characters(1);
    f6_expect_line_de(test, kF6SpuriousByteAtR0,
                      "type 0 R1>R0 spurious border byte on the third line");
}

void test_type1_r1_gt_r0_no_border_byte(TestBench& test) {
    test.set_crtc_type(1);
    f6_settle_one_frame(test, 0);

    // ACCC v1.10 section 17.6.2 (pages 186-187): type 1 emits no border
    // byte between rows when R1 > R0 -- rows stay seamlessly contiguous
    // (section 28.1.6 type discriminator).
    f6_expect_line_de(test, kF6AllDisplay,
                      "type 1 R1>R0 emits no border byte");
    test.run_characters(1);
    f6_expect_line_de(test, kF6AllDisplay,
                      "type 1 R1>R0 emits no border byte on the next line");
}

void test_type0_spurious_byte_delayed_one_character(TestBench& test) {
    test.set_crtc_type(0);
    f6_settle_one_frame(test, 1);

    // ACCC v1.10 sections 19.2.4 (page 195) and 19.2.3 (pages 193-194):
    // the SKEW-DISPTMG delay is counted from the substituted trigger, so
    // delay=1 displaces the spurious byte onto C0=0 of the following line
    // and display resumes one character later than without skew.
    f6_expect_line_de(test, kF6SpuriousByteDelayed1,
                      "type 0 R1>R0 skew 1 displaces the border byte to C0=0");
    test.run_characters(1);
    f6_expect_line_de(test, kF6SpuriousByteDelayed1,
                      "type 0 R1>R0 skew 1 displaced byte recurs per line");
}

void test_type0_spurious_byte_delayed_two_characters(TestBench& test) {
    test.set_crtc_type(0);
    f6_settle_one_frame(test, 2);

    // Delay=2 displaces the spurious byte onto C0=1 (ACCC v1.10 sections
    // 19.2.4 page 195 and 19.2.3 pages 193-194).
    f6_expect_line_de(test, kF6SpuriousByteDelayed2,
                      "type 0 R1>R0 skew 2 displaces the border byte to C0=1");
    test.run_characters(1);
    f6_expect_line_de(test, kF6SpuriousByteDelayed2,
                      "type 0 R1>R0 skew 2 displaced byte recurs per line");
}

void test_type0_skew_non_output_blanks(TestBench& test) {
    test.set_crtc_type(0);
    f6_settle_one_frame(test, 3);

    // SKEW-DISPTMG mode 2'b11 is the non-output code: no DISPTMG at all is
    // generated, which also suppresses the spurious byte (ACCC v1.10
    // sections 19.1 page 192 and 19.2 page 193).
    const std::array<bool, kF6LineCharacters> blanked = {};
    f6_expect_line_de(test, blanked, "type 0 R1>R0 skew non-output blanks DE");
    test.run_characters(1);
    f6_expect_line_de(test, blanked,
                      "type 0 R1>R0 skew non-output stays blanked");
}

// ---------------------------------------------------------------------------
// t21: F10 type-1 IVM toggle parity table (ACCC v1.10 sections 19.5.3 p.208,
// 19.8.2 setup p.209, and the 16 SHAKER 22C/3 truth-table panels on
// pp.210-211; render-verified 2026-08-24 under the extract protocol).
//
// Each panel is one configuration of {initial ParityFrame, C4.0, R9.0, C9.0}
// with an OUT R8,3 ("on") followed by an OUT R8,0 ("off") mid-line.  The
// panels' callouts pin the parity state after each update stage, and the
// panels' C9 rows pin the C9.0 writes.  Both fit the documented two-stage
// rule exactly (all 64 callouts cross-checked against the pseudocode):
//
//   3rd us (stage A, the character edge after the write):
//     ParityC9 := C9.0 xor (C4.0 and not R9.0);  C9.0 := ParityC9
//   4th us (stage B, one character edge later):
//     entering IVM: if ParityFrame even, ParityC9 := C4.0 and not R9.0
//                   ParityFrame := ParityFrame and (ParityC9 xor (C4.0 and
//                                                     not R9.0))
//                   C9.0 := ParityC9
//     leaving IVM:  ParityFrame := ParityC9;  C9.0 := ParityC9
//
// Fixture timing convention (matches every panel's C9 row): the R8 write
// lands early in character C0=W, stage A applies at the character edge that
// starts C0=W+1 (the panel's "on"/"off" column), stage B at the edge that
// starts C0=W+2.  The panels' drawn Parity rows are internally inconsistent
// by one character on the ODD page (source drawing quirk, same tier as the
// flagged C9-column quirks of the pp.221-224 tables); the callouts and the
// C9 rows are the oracle, and this fixture asserts exactly those.
//
// Setup reaches each panel's precondition without IVM ever being active:
// R9=1 gives plain 2-line rows; a mid-line R9:=0 write then realizes the
// R9.0=0 panels with C9.0=1 (impossible to reach under R9=0 alone, since
// C9 resets at every row end); one R4=0 frame boundary toggles ParityFrame
// for the ODD panels (section 19.5.3: toggles every frame regardless of R8).
// ParityC9's drawn initial value never enters an expectation: stage A
// overwrites it first.

struct T21Panel {
    const char* shaker_tests;
    bool parity_frame_odd;
    bool c4_odd;
    bool r9_odd;
    bool c9_odd;
};

void t21_run_panel(TestBench& test, const T21Panel& panel) {
    const bool pf1 = panel.parity_frame_odd;
    const bool c4b = panel.c4_odd;
    const bool r9b = panel.r9_odd;
    const bool c9b = panel.c9_odd;
    const bool x_term = c4b && !r9b;            // C4.0 and not R9.0
    const bool stage_a = c9b ^ x_term;          // ParityC9 after stage A
    const bool stage_b_pf = pf1 && c9b;         // PF and (ParC9 xor X)
    const bool stage_b_pc9 = pf1 ? stage_a : x_term;
    const bool off_a = stage_b_pc9 ^ x_term;    // C9.0(now) xor X
    const std::string tag = std::string("t21 panel ") + panel.shaker_tests;

    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, pf1 ? 0 : 63},
        {5, 0},  {6, 63}, {7, 63}, {8, 0},    {9, 1},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    if (pf1) {
        // One R4=0 frame (2 lines) crosses the frame origin; ParityFrame
        // toggles to odd at C4=C9=C0=0 (section 19.5.3 p.208).  Then pin the
        // frame length out of the way so no later row end ends the frame.
        test.run_characters(2 * 64);
        test.write_register(4, 63);
    }

    // Row ends (2 lines each with R9=1) advance C4; stop one row short of
    // the R4=63 frame end.  C4 parity is now c4b and C9 restarts at 0.
    if (c4b) {
        test.run_characters(2 * 64);
    }

    // Reach C9=c9b mid-line.  With R9=1 the second line of a row holds C9=1;
    // for the R9.0=0 panels the mid-line R9:=0 write then realizes the
    // panel's register state with C9.0 already 1.
    if (c9b) {
        test.run_characters(64);
    }
    if (!r9b) {
        test.write_register(9, 0);
    }
    test.expect_c4(tag + " setup C4 parity", c4b);
    test.expect_line(tag + " setup C9", c9b);

    // OUT R8,3 with enough line left that both stages and the later OUT R8,0
    // complete inside the same line (the R9:=0 setup write above lands around
    // C0=5, so the toggle point sits at C0=20 rather than the panels' C0=4;
    // the panels pin the +1/+2 character stage offsets, not the absolute C0).
    // Stage A applies at the start of the character after the write (the
    // panel's "on" column), stage B at the start of the one after it.
    test.run_to_c0(20);
    test.select_register(8);
    test.write_selected_register_now(3);
    test.run_characters(1);
    test.expect_parity_c9(tag + " stage A ParityC9", stage_a);
    test.expect_line_parity(tag + " stage A C9.0", stage_a);
    test.expect_parity_frame(tag + " stage A ParityFrame held", pf1);
    test.run_characters(1);
    test.expect_parity_c9(tag + " stage B ParityC9", stage_b_pc9);
    test.expect_parity_frame(tag + " stage B ParityFrame", stage_b_pf);
    test.expect_line_parity(tag + " stage B C9.0", stage_b_pc9);

    // OUT R8,0 seven characters later; off stage A at the next character
    // edge, off stage B at the one after.
    test.run_to_c0(27);
    test.select_register(8);
    test.write_selected_register_now(0);
    test.run_characters(1);
    test.expect_parity_c9(tag + " off stage A ParityC9", off_a);
    // The leaving stage A plants the new parity into C9.0 immediately
    // (review B-1): the four X=1 panels draw the change in the 3rd-us
    // column, one character before the off cell.
    test.expect_line_parity(tag + " off stage A C9.0", off_a);
    test.run_characters(1);
    test.expect_parity_frame(tag + " off stage B ParityFrame", off_a);
    test.expect_line_parity(tag + " off stage B C9.0", off_a);
}

// The 16 panel configurations in pp.210-211 layout order: EVEN page first
// (initial parity even), ODD page second; within a page C4.0=0 quadrant
// above C4.0=1, R9.0=0 column left of R9.0=1, C9.0=0 panel above C9.0=1.
constexpr T21Panel kT21Panels[] = {
    {"19(S),23(W)", false, false, false, false},
    {"17(Q),21(U),25(Y1)", false, false, false, true},
    {"4(D),8(H)", false, true, false, false},
    {"2(B),6(F)", false, true, false, true},
    {"20(T),24(X)", false, false, true, false},
    {"18(R),22(V),26(Z1)", false, false, true, true},
    {"5(E),9(I)", false, true, true, false},
    {"3(C),7(G)", false, true, true, true},
    {"27(ZA),29(ZC)", true, false, false, false},
    {"16(P),25(Y2)", true, false, false, true},
    {"12(L),14(N)", true, true, false, false},
    {"1(A),10(J2)", true, true, false, true},
    {"28(ZB),30(ZD)", true, false, true, false},
    {"26(Z)", true, false, true, true},
    {"(M),15(O)", true, true, true, false},
    {"11(K2)", true, true, true, true},
};

void t21_run_indexed(TestBench& test, unsigned index) {
    t21_run_panel(test, kT21Panels[index]);
}

#define T21_TEST(name, idx)                                        \
    void name(TestBench& test) { t21_run_indexed(test, idx); }

T21_TEST(t21_body_00, 0)
T21_TEST(t21_body_01, 1)
T21_TEST(t21_body_02, 2)
T21_TEST(t21_body_03, 3)
T21_TEST(t21_body_04, 4)
T21_TEST(t21_body_05, 5)
T21_TEST(t21_body_06, 6)
T21_TEST(t21_body_07, 7)
T21_TEST(t21_body_08, 8)
T21_TEST(t21_body_09, 9)
T21_TEST(t21_body_10, 10)
T21_TEST(t21_body_11, 11)
T21_TEST(t21_body_12, 12)
T21_TEST(t21_body_13, 13)
T21_TEST(t21_body_14, 14)
T21_TEST(t21_body_15, 15)

#undef T21_TEST

// ---------------------------------------------------------------------------
// t22: F10 type-0 IVM entry/exit counting fixtures for even R9 (ACCC v1.10
// sections 19.8.1 pp.219-220 pseudocode and prose; the worked tables
// pp.221-224, render-verified 2026-08-24; all tables use R9=6).
//
// Documented model spliced from p.219-220 (all cited values from the tables'
// C9-VMA columns and the reliable segments of their C9 columns):
//
//   - C9.VMA = (C9 x 2 + ParityC9) mod 32 is the address-visible line value
//     while IVM is active ("the more significant bit is lost", p.219).
//   - Switch line (R8 written to 3 during it): the end-of-line test uses the
//     raw C9 against "R9 or ParityFrame"; the doubled value and doubled test
//     start at the next C0=0 (p.219 prose, every entry table).
//   - Steady IVM: row ends when C9.VMA == "R9 or ParityC9" (== R9 or
//     ParityFrame for the even R9 these tables use; the p.220 "R9 or
//     ParityC9" variant is indistinguishable here, odd-R9 is finding F15).
//   - Exit line (R8 written to 0 during it): the end test uses C9.VMA
//     against plain R9 (p.220 prose; p.223 bottom-right table shows the
//     matching case resetting C9 and incrementing C4; p.224 bottom-right
//     shows the C9.VMA = R9+1 case incrementing C9 without a row end -- the
//     documented p.220 worked example).
//   - After the exit line, plain C9-vs-R9 counting resumes immediately.
//
// Fixture scope notes: the post-exit row-end behavior (seven of eight exit
// tables run C9 to 7 with R9=6 without the predicted C4 increment) was
// resolved 2026-08-26 (author question Q19(b) -> finding F16) and is
// deliberately not asserted here; the settled C4>=1 blocks of the tables
// print the doubled value in the C9 column (flagged source quirk) so
// post-reset expectations follow the pseudocode (C9 restarts at 0 and steps
// by 1) with the tables' C9-VMA column as the cross-check.  Odd-R9
// alternation is finding F15 and is out of scope for these even-R9 tests.

struct T22Step {
    std::uint8_t c4;
    std::uint8_t c9;
    std::uint8_t ra;  // the tables' C9-VMA column; plain C9 off IVM display
};

// C9.VMA for a doubled-display line: ((C9 x 2) + ParityC9) mod 32
// (section 19.8.1 p.219).  parity is the frame's ParityC9 (0 even, 1 odd
// for the even-R9 tables these fixtures use).
constexpr std::uint8_t t22_vma(std::uint8_t c9, bool odd) {
    return static_cast<std::uint8_t>(((c9 << 1) | (odd ? 1u : 0u)) & 0x1Fu);
}

// Common t22 register frame: R4=63 keeps frame boundaries out of every
// sequence; R6 selects the frame parity (63 keeps ParityFrame even for the
// whole run -- with R4=63 the fixtures never reach row 63, so no capture
// fires and no frame boundary snapshots anything; 1 makes C4=R6 fire during
// the first frame so frame 2 is odd); R7=63 keeps VSYNC quiet.
void t22_configure(TestBench& test, bool odd_frame) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 63},
        {5, 0},  {6, odd_frame ? 1 : 63}, {7, 63}, {8, 0}, {9, 6},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    if (odd_frame) {
        // Run one full frame (64 rows x 7 lines) and stop at its origin:
        // ParityFrame snapshots the ParityR6 that C4=R6 flipped during the
        // frame, so frame 2 is the odd frame (section 19.5.2 p.205).
        test.run_to_frame_start(600);
        test.expect_c4("t22 odd-frame setup reaches frame origin", 0);
    }
}

// Walk a documented (C4, C9) sequence, one 64-character line per step,
// starting with the switch/exit line itself.
void t22_walk(TestBench& test, const char* tag,
              const std::vector<T22Step>& steps) {
    for (size_t i = 0; i < steps.size(); ++i) {
        if (i != 0) {
            test.run_characters(64);
        }
        const std::string prefix = std::string(tag) + " line " +
                                   std::to_string(i);
        test.expect_c4(prefix + " C4", steps[i].c4);
        test.expect_line(prefix + " C9", steps[i].c9);
        test.expect_ra(prefix + " RA (C9-VMA)", steps[i].ra);
    }
}

std::vector<T22Step> t22_tail_after_row_end(std::uint8_t c4, bool odd) {
    // After any row end: C9 restarts at 0 and steps by 1 (pseudocode p.219);
    // two settled lines are asserted, well before the next limit.  The
    // display stays doubled (ParityC9 is static for even R9 on type 0).
    return {{c4, 0, t22_vma(0, odd)},
            {c4, 1, t22_vma(1, odd)},
            {c4, 2, t22_vma(2, odd)}};
}

// Entry fixtures: write R8,3 early in character 4 of the line whose C9 equals
// `offset`, then walk the documented sequence.
void t22_run_entry(TestBench& test, bool odd_frame, std::uint8_t offset,
                   bool overflow) {
    t22_configure(test, odd_frame);
    // Odd offsets (C9=1/3) are unreachable under the pre-F10 stepping (C9
    // moves by 2 with bit 0 masked), so the setup walker reports the
    // divergence through the known-divergence assertion below.
    test.run_to_line_mid(offset, 40);
    test.expect_line("t22 entry setup reaches C9", offset);
    test.run_to_c0(TestBench::kF10TargetC0);
    test.select_register(8);
    test.write_selected_register_now(3);

    const std::uint8_t c4 = 0;
    std::vector<T22Step> steps;
    // Switch line: display and the limit value stay raw (p.219).
    steps.push_back({c4, offset, offset});
    if (overflow) {
        // C9 ran past the target on the switch line: the doubled value
        // (C9 x 2) mod 32 first reaches R9 (even frame) / R9+1 (odd frame)
        // again at C9=19 (38 mod 32 = 6), giving the documented 16-line run.
        for (std::uint8_t c9 = static_cast<std::uint8_t>(offset + 1);
             c9 <= 19; ++c9) {
            steps.push_back({c4, c9, t22_vma(c9, odd_frame)});
        }
    } else {
        // Steady IVM: the doubled value reaches the limit at C9 = 3 for the
        // offsets these fixtures use; intermediate lines step C9 by 1.
        std::uint8_t c9 = static_cast<std::uint8_t>(offset + 1);
        while (c9 < 3) {
            steps.push_back({c4, c9, t22_vma(c9, odd_frame)});
            c9 = static_cast<std::uint8_t>(c9 + 1);
        }
        steps.push_back({c4, 3, t22_vma(3, odd_frame)});
    }
    // Row end consumed the limit line: C4 increments, C9 restarts.
    auto tail = t22_tail_after_row_end(c4 + 1, odd_frame);
    steps.insert(steps.end(), tail.begin(), tail.end());
    t22_walk(test, "t22 entry", steps);
}

void t22_entry_even_0(TestBench& test) { t22_run_entry(test, false, 0, false); }
void t22_entry_even_1(TestBench& test) { t22_run_entry(test, false, 1, false); }
void t22_entry_even_2(TestBench& test) { t22_run_entry(test, false, 2, false); }
void t22_entry_even_3(TestBench& test) { t22_run_entry(test, false, 3, true); }
void t22_entry_even_4(TestBench& test) { t22_run_entry(test, false, 4, true); }
void t22_entry_even_5(TestBench& test) { t22_run_entry(test, false, 5, true); }
void t22_entry_odd_0(TestBench& test) { t22_run_entry(test, true, 0, false); }
void t22_entry_odd_1(TestBench& test) { t22_run_entry(test, true, 1, false); }
void t22_entry_odd_3(TestBench& test) { t22_run_entry(test, true, 3, true); }

void t22_entry_even_6(TestBench& test) {
    // p.223 top-left: switching at C9=6=R9 on the even frame matches the raw
    // test immediately, so the switch line itself ends the row.
    t22_configure(test, false);
    test.run_to_line_mid(6, 40);
    test.expect_line("t22 entry setup reaches C9", 6);
    test.run_to_c0(TestBench::kF10TargetC0);
    test.select_register(8);
    test.write_selected_register_now(3);
    const std::vector<T22Step> steps = {
        {0, 6, 6},  // switch line ends the row (raw C9 = 6 == R9)
        {1, 0, 0}, {1, 1, 2}, {1, 2, 4},
        {1, 3, 6},  // next doubled limit (C9.VMA = 6)
        {2, 0, 0}, {2, 1, 2}, {2, 2, 4},
    };
    t22_walk(test, "t22 entry even 6", steps);
}

void t22_entry_odd_6(TestBench& test) {
    // p.223 top-right: on the odd frame the raw test compares 6 against
    // R9 or ParityFrame = 7, fails, and C9 overflows -- the documented split
    // against the even frame's immediate reset.
    t22_configure(test, true);
    test.run_to_line_mid(6, 40);
    test.expect_line("t22 entry setup reaches C9", 6);
    test.run_to_c0(TestBench::kF10TargetC0);
    test.select_register(8);
    test.write_selected_register_now(3);
    std::vector<T22Step> steps = {{0, 6, 6}};
    for (std::uint8_t c9 = 7; c9 <= 19; ++c9) {
        steps.push_back({0, c9, t22_vma(c9, true)});
    }
    steps.push_back({1, 0, 1});
    steps.push_back({1, 1, 3});
    steps.push_back({1, 2, 5});
    steps.push_back({1, 3, 7});  // doubled limit on the odd frame: C9.VMA = 7
    steps.push_back({2, 0, 1});
    t22_walk(test, "t22 entry odd 6", steps);
}

// Exit fixtures, table-shaped (pp.223-224): IVM is entered on line 0 of
// row 0, that character row completes (C4=1 after its doubled limit), and
// the OUT R8,0 lands early in character 4 of a second-row line.  The walk
// starts at the exit line itself.
void t22_run_exit(TestBench& test, bool odd_frame, std::uint8_t exit_offset,
                  const std::vector<T22Step>& steps) {
    t22_configure(test, odd_frame);
    test.run_to_c0(TestBench::kF10TargetC0);
    test.select_register(8);
    test.write_selected_register_now(3);  // enter IVM on line 0
    // Row 0 runs C9 0..3 (doubled limit 6/7), then row 1 starts at C9=0:
    // 4 lines to the row end plus exit_offset lines into row 1.
    for (unsigned n = 0; n < 4 + exit_offset; ++n) {
        test.run_characters(64);
    }
    test.expect_c4("t22 exit setup reaches row 1", 1);
    test.expect_line("t22 exit setup reaches C9", exit_offset);
    test.run_to_c0(TestBench::kF10TargetC0);
    test.select_register(8);
    test.write_selected_register_now(0);
    t22_walk(test, "t22 exit", steps);
}

void t22_exit_even_at_limit(TestBench& test) {
    // p.223 bottom-right table: exiting on the second row's limit line
    // (C9.VMA = 6 = R9) still ends the row (parity dropped from the target
    // only): C9 resets to 0 and C4 increments; plain counting follows.
    t22_run_exit(test, false, 3, {{1, 3, 6}, {2, 0, 0}, {2, 1, 1}, {2, 2, 2}});
}

void t22_exit_odd_at_r9_plus_1(TestBench& test) {
    // p.224 bottom-right table and the p.220 worked example: exiting on the
    // line whose C9.VMA = 7 = R9+1 misses the parity-dropped test, so C9 is
    // incremented (to 4) instead of resetting.  The walk stops before the
    // post-exit row-end zone (Finding F16).
    t22_run_exit(test, true, 3, {{1, 3, 7}, {1, 4, 4}, {1, 5, 5}});
}

void t22_exit_odd_below_limit(TestBench& test) {
    // p.224 third table: exiting at C9.VMA = 5 < 6 misses; C9 continues
    // with the plain +1 stepping from the next line.
    t22_run_exit(test, true, 2, {{1, 2, 5}, {1, 3, 3}, {1, 4, 4}});
}

void t22_exit_even_below_limit(TestBench& test) {
    // p.223 bottom-left table: same shape on the even frame (C9.VMA = 4
    // < 6).  Stops before the table's post-exit tail (Finding F16).
    t22_run_exit(test, false, 2, {{1, 2, 4}, {1, 3, 3}, {1, 4, 4}});
}

void t22_exit_even_c9_0(TestBench& test) {
    // p.223 top exit table: exiting at C9.VMA = 0 misses; plain +1 follows.
    t22_run_exit(test, false, 0, {{1, 0, 0}, {1, 1, 1}, {1, 2, 2}});
}

void t22_exit_even_c9_1(TestBench& test) {
    // p.223 second exit table: exiting at C9.VMA = 2 misses.
    t22_run_exit(test, false, 1, {{1, 1, 2}, {1, 2, 2}, {1, 3, 3}});
}

void t22_exit_odd_c9_0(TestBench& test) {
    // p.224 top exit table: exiting at C9.VMA = 1 misses.
    t22_run_exit(test, true, 0, {{1, 0, 1}, {1, 1, 1}, {1, 2, 2}});
}

void t22_exit_odd_c9_1(TestBench& test) {
    // p.224 second exit table: exiting at C9.VMA = 3 misses.
    t22_run_exit(test, true, 1, {{1, 1, 3}, {1, 2, 2}, {1, 3, 3}});
}

// ---------------------------------------------------------------------------
// t23: F10 type-1 follow-up vectors from the independent review (B-2 and the
// self-found IVM-activation gap), plus the R8=1 RA pin (N-9).
//
// t23a derives its sequence by hand from the section 19.8.2 match branch
// (p.225) with R9=2 (even), R4=1, R5=0: a character row is the two lines
// C9=0,2 (doubled display 0,2 with ParityC9=0); the row end at C9=2 toggles
// ParityC9 and restarts C9 from it as one step, so row 1 runs C9=1,3 --
// wait, C9=1 pre-increments to 2 and (2 and ~1) == 2 matches immediately,
// so row 1 is the single line C9=1 and it is also the frame boundary
// (C4==R4=1).  At that boundary the match branch must toggle ParityC9
// (1 -> 0) AND restart C9 from the toggled value (C9=0); before the B-2
// fix the flop kept 1 while C9 took 0, and every later row ran with the
// two disagreeing.  The walk asserts C4/C9/RA per line plus ParityC9
// across the boundary.

void test_type1_ivm_frame_boundary_parity_continuity(TestBench& test) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 1},
        {5, 0},  {6, 63}, {7, 63}, {8, 3},    {9, 2},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    struct Step {
        std::uint8_t c4;
        std::uint8_t c9;
        std::uint8_t ra;
        bool pc9;
    };
    // Hand-derived from the p.225 match branch (see block comment).
    const std::array<Step, 6> steps = {{
        {0, 0, 0, 0},
        {0, 2, 2, 0},
        {1, 1, 1, 1},   // row 1: C9 restarted from the toggled ParityC9
        {0, 0, 0, 0},   // frame boundary: ParityC9 toggled 1 -> 0 (B-2)
        {0, 2, 2, 0},
        {1, 1, 1, 1},
    }};
    for (size_t i = 0; i < steps.size(); ++i) {
        if (i != 0) {
            test.run_characters(64);
        }
        const std::string prefix = "t23a line " + std::to_string(i);
        test.expect_c4(prefix + " C4", steps[i].c4);
        test.expect_line(prefix + " C9", steps[i].c9);
        test.expect_ra(prefix + " RA", steps[i].ra);
        test.expect_parity_c9(prefix + " ParityC9", steps[i].pc9);
        test.expect_line_parity(prefix + " C9.0 tracks ParityC9",
                                steps[i].pc9);
    }
}

// The IVM flag must also engage when R8=3 is already programmed -- through
// a snapshot load there is no toggle write to run the stage machine.  The
// engine tracks the register whenever no toggle stage is in flight.

void test_type1_ivm_engages_from_snapshot_r8_3(TestBench& test) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 1},
        {5, 0},  {6, 63}, {7, 63}, {8, 0},    {9, 2},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_characters(2);

    // Snapshot-load R8=3 (plus the same frame geometry as t23a).  IVM
    // counting must engage without any R8 toggle write.
    const std::array<std::uint8_t, 10> snapshot = {
        63, 40, 50, 0x00, 1, 0, 63, 63, 3, 2,
    };
    test.load_snapshot_registers(snapshot);

    // Same hand-derived sequence as t23a, from the load point.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 4> steps = {
        {{0, 0}, {0, 2}, {1, 1}, {0, 0}},
    };
    for (size_t i = 0; i < steps.size(); ++i) {
        if (i != 0) {
            test.run_characters(64);
        }
        const std::string prefix = "t23b line " + std::to_string(i);
        test.expect_c4(prefix + " C4", steps[i].first);
        test.expect_line(prefix + " C9", steps[i].second);
        test.expect_ra(prefix + " RA", steps[i].second);
    }
    // ParityC9 toggled at the frame-boundary row end (B-2 continuity).
    test.expect_parity_c9("t23b ParityC9 after the frame boundary", 1 - 1);
}

// N-9: INTERLACE SYNC (R8=1) offsets VSYNC by half a line but does not
// touch the raster address on either type -- pin RA == C9 with no field OR.

void test_interlace_sync_leaves_ra_plain(TestBench& test) {
    for (unsigned type = 0; type <= 1; ++type) {
        test.set_crtc_type(type);
        const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
            {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 63},
            {5, 0},  {6, 63}, {7, 63}, {8, 1},    {9, 3}}};
        for (const auto& [address, value] : registers) {
            test.write_register(address, value);
        }
        test.reset();
        for (unsigned line = 0; line < 5; ++line) {
            test.run_to_c0(TestBench::kF10TargetC0);
            test.expect_ra("t23c R8=1 RA stays raw C9 (type " +
                               std::to_string(type) + ", line " +
                               std::to_string(line) + ")",
                           static_cast<std::uint8_t>(line & 0x3));
            test.run_characters(64);
        }
    }
}

// t24: type-1 IVM VSYNC positions (ACCC v1.10 section 19.5.3 p.208 table).
//
// R9=8 (even -> the row-pair line count R9+1 is odd), R7 on a chosen C4,
// R8=3 held from a snapshot load (frame-boundary entry, no toggle stages,
// so the MID-VSYNC field-vs-ParityFrame residual stays out of scope), and
// R4=6: seven C4s per frame.  An odd C4 count is what makes consecutive
// frames alternate their line sequences (section 19.8.2 p.225: ParityC9
// toggles at every C9/R9 match including the frame-boundary wrap, so an
// odd number of matches per frame flips the frame-start C9) -- the table's
// even frame opens C4=0 at C9=0 (5-line C4 rows) and its odd frame opens
// at C9=1 (4-line C4 rows), 32 and 31 lines per frame respectively.  The
// table's VSYNC boxes pin the start line for every R7 on each frame
// parity; type 1 applies no delay correction (unlike CRTC 0/3/4,
// section 19.5.2 pp.206-207), so with R7 on an odd C4 the pulse sits one
// frame-line earlier on the odd frame -- the documented permanent 1-line
// VSYNC gap.

void test_type1_ivm_vsync_gap_r7_odd_c4(TestBench& test) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 6},
        {5, 0},  {6, 25}, {7, 1},   {8, 0},   {9, 8},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_characters(2);

    // Snapshot-load R8=3: IVM engages with ParityFrame=0, so the first
    // frame runs the table's EVEN FRAME column (C4=0 at C9=0,2,4,6,8).
    const std::array<std::uint8_t, 10> snapshot = {
        63, 40, 46, 0x11, 6, 0, 25, 1, 3, 8,
    };
    test.load_snapshot_registers(snapshot);

    // Even frame (32 lines): the VSYNC box for R7=1 sits at (C4,C9)=(1,1),
    // the first line of C4=1 -- frame-line 5 after the five C4=0 lines
    // (0,2,4,6,8).  Walk line by line, sampling mid-line; the pulse holds
    // 16 lines (type-1 fixed width) and ends before line 21.
    test.run_characters(30);
    test.expect_vsync_low("t24a even frame line 0: VSYNC quiet before the gap start");
    test.run_characters(34);
    for (unsigned line = 1; line <= 31; ++line) {
        test.run_characters(32);
        const bool expect_high = line >= 5 && line <= 20;
        if (expect_high) {
            test.expect_vsync_high("t24a even frame line " + std::to_string(line) +
                                   " (R7=1 gap start at line 5, 16 lines)");
        } else {
            test.expect_vsync_low("t24a even frame line " + std::to_string(line) +
                                  " outside the (1,1)-anchored pulse");
        }
        if (line == 5) {
            test.expect_c4("t24a even frame pulse starts on C4=1", 1);
            test.expect_ra("t24a even frame pulse starts at C9=1 (table box (1,1))", 1);
        }
        test.run_characters(32);
    }

    // Odd frame (31 lines): C4=0 runs only four lines (1,3,5,7), so the
    // first line of C4=1 is frame-line 4 and the table's box sits at
    // (1,0).  No delay correction: the pulse is one frame-line earlier
    // than on the even frame, and the 1-line gap repeats permanently.
    for (unsigned line = 0; line <= 30; ++line) {
        test.run_characters(32);
        const bool expect_high = line >= 4 && line <= 19;
        if (expect_high) {
            test.expect_vsync_high("t24a odd frame line " + std::to_string(line) +
                                   " (R7=1 gap start at line 4, 16 lines)");
        } else {
            test.expect_vsync_low("t24a odd frame line " + std::to_string(line) +
                                  " outside the (1,0)-anchored pulse");
        }
        if (line == 4) {
            test.expect_c4("t24a odd frame pulse starts on C4=1", 1);
            test.expect_ra("t24a odd frame pulse starts at C9=0 (table box (1,0))", 0);
        }
        test.run_characters(32);
    }
}

void test_type1_ivm_vsync_no_gap_r7_even_c4(TestBench& test) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 6},
        {5, 0},  {6, 25}, {7, 2},   {8, 0},   {9, 8},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_characters(2);
    const std::array<std::uint8_t, 10> snapshot = {
        63, 40, 46, 0x11, 6, 0, 25, 2, 3, 8,
    };
    test.load_snapshot_registers(snapshot);

    // Contrast from the same table: with R7=2 (even C4) both frames start
    // the pulse on frame-line 9 -- the first line of C4=2 arrives after
    // 5+4 lines on the even frame and 4+5 on the odd frame, so no gap.
    // Even frame box (2,0): C9=0; odd frame box (2,1): C9=1.  The VSYNC
    // assertions are known-divergence forms: the current model misses both
    // fires because vsync_line_fire tests the plain C9==R9 (false at the
    // (1,7) wrap) and the field=1 MID-VSYNC arm requires RA==0 (false at
    // (2,1)); the counters themselves are pinned by plain assertions.
    test.run_characters(30);
    test.expect_vsync_low("t24b even frame line 0: VSYNC quiet");
    test.run_characters(34);
    for (unsigned line = 1; line <= 31; ++line) {
        test.run_characters(32);
        const bool expect_high = line >= 9 && line <= 24;
        if (line == 9) {
            test.expect_c4("t24b even frame pulse starts on C4=2", 2);
            test.expect_ra("t24b even frame pulse starts at C9=0 (table box (2,0))", 0);
        }
        if (expect_high) {
            test.expect_vsync_high("t24b even frame line " + std::to_string(line) +
                                   " (R7=2 pulse from line 9, 16 lines)");
        } else {
            test.expect_vsync_low("t24b even frame line " + std::to_string(line) +
                                  " outside the (2,0)-anchored pulse");
        }
        test.run_characters(32);
    }
    for (unsigned line = 0; line <= 30; ++line) {
        test.run_characters(32);
        const bool expect_high = line >= 9 && line <= 24;
        if (line == 9) {
            test.expect_c4("t24b odd frame pulse starts on C4=2", 2);
            test.expect_ra("t24b odd frame pulse starts at C9=1 (table box (2,1))", 1);
        }
        if (expect_high) {
            test.expect_vsync_high("t24b odd frame line " + std::to_string(line) +
                                   " (R7=2 pulse from line 9, 16 lines)");
        } else {
            test.expect_vsync_low("t24b odd frame line " + std::to_string(line) +
                                  " outside the (2,1)-anchored pulse");
        }
        test.run_characters(32);
    }
}

// t24c: type-1 IVM MID-VSYNC half-line phase (ACCC v1.10 section 19.5.3
// p.208 prose: "If ParityFrame is even, then an additional line and a
// MID-VSYNC are scheduled. If ParityFrame is odd, then no additional line
// and no MID-VSYNC."; the type-0 Note on p.207 states the same half-line
// rule as "the VSYNC occurs in the middle of the line on C0 = R0/2").
// t24b's register set (R7=2): the pulse's first line is frame-line 9 on
// both parities. The ParityFrame-even frame must start the pulse at the
// half-line tick (low at C0=20, high at C0=40) and end it at the half-line
// tick 16 lines later (high at C0=20 of line 25, low at C0=40); the
// ParityFrame-odd frame starts and ends at line seams (high/high on line 9,
// low/low on line 25).

void test_type1_ivm_mid_vsync_half_line_phase(TestBench& test) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 6},
        {5, 0},  {6, 25}, {7, 2},   {8, 0},   {9, 8},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_characters(2);
    const std::array<std::uint8_t, 10> snapshot = {
        63, 40, 46, 0x11, 6, 0, 25, 2, 3, 8,
    };
    test.load_snapshot_registers(snapshot);

    // Sample helper: from a line start, land at C0=20, sample, land at
    // C0=40, sample, complete the line.
    test.run_characters(18);
    test.expect_vsync_low("t24c even frame line 0: VSYNC quiet");
    test.run_characters(44);
    for (unsigned line = 1; line <= 31; ++line) {
        test.run_characters(20);
        if (line == 9) {
            test.expect_vsync_low(
                "t24c even frame line 9 at C0=20: MID-VSYNC starts at the half-line tick");
        }
        if (line == 25) {
            test.expect_vsync_high(
                "t24c even frame line 25 at C0=20: pulse still up before the half-line tick");
        }
        test.run_characters(20);
        if (line == 9) {
            test.expect_vsync_high(
                "t24c even frame line 9 at C0=40: MID-VSYNC pulse is up");
        }
        if (line == 25) {
            test.expect_vsync_low(
                "t24c even frame line 25 at C0=40: pulse ends at the half-line tick");
        }
        test.run_characters(24);
    }
    for (unsigned line = 0; line <= 30; ++line) {
        test.run_characters(20);
        if (line == 9) {
            test.expect_vsync_high(
                "t24c odd frame line 9 at C0=20: seam start is already up");
        }
        if (line == 25) {
            test.expect_vsync_low(
                "t24c odd frame line 25 at C0=20: seam end already down");
        }
        test.run_characters(20);
        if (line == 9) {
            test.expect_vsync_high("t24c odd frame line 9 at C0=40: pulse is up");
        }
        if (line == 25) {
            test.expect_vsync_low("t24c odd frame line 25 at C0=40: pulse is down");
        }
        test.run_characters(24);
    }
}

// ---------------------------------------------------------------------------
// t27: F14 additional interlace line, type 0 (ACCC v1.10 section 19.6.1
// p.216; section 19.5.2 p.205; section 11.2 pp.83-84; section 19.3 p.199;
// Q10 resolution in accc-author-questions.md item 10).
//
// Paper derivation, every assertion traceable to a cited section:
//
//   - Gate: the line is appended "at the end of the frame (after the R5
//     lines if necessary)" iff an interlace mode is active (R8=1 or 3) and
//     ParityR6 is odd (section 19.6.1 p.216).  ParityR6 := ParityFrame xor 1
//     when C4 reaches R6, independent of R8 (section 19.5.2 p.205); with
//     R6>R4 the capture never fires and ParityR6 freezes, so the gate
//     persists: a line every frame if frozen odd, never if frozen even
//     (section 19.6.1 p.216, both branches stated explicitly).
//
//   - Position: after the R5 adjustment lines and before the frame origin;
//     the origin's C4/C9 reset, ParityFrame snapshot and VMA reload move to
//     the end of the additional line.  The even frame is 312 lines and the
//     following odd frame "inherits" the line for its 313-line 20032 us
//     duration (section 19.3 p.199).
//
//   - C4 accounting: "C4 is incremented only once for all additional lines
//     (R5 and interlace) and is equal to C4=R4+1" (section 19.6.1 p.216).
//     The type-0 adjustment already increments C4 to R4+1 at its entry
//     (section 11.2.2 p.81; the p.83 adjustment table), so the additional
//     line shares that single increment and the adjustment count continues
//     through it: "the counting is done as if this line had been added to
//     R5" (section 11.2 p.84), i.e. the additional line holds C9=R5.
//     (The p.84 R5=7/R5=8 worked example is the section 11.2.3 CRTC 1/2
//     accounting where C4 keeps incrementing at mid-period C9 wraps; the
//     type-0 single-increment rule of section 19.6.1 is what is implemented
//     here.)
//
// Fixture frame: R0=63 (64-character lines), R4=2 (three C4 rows), R9=2
// (three lines per row), R6=1 (ParityR6 captured at each C4=0->1 crossing),
// R7=63 (VSYNC quiet), R8=1.  INTERLACE SYNC arms the gate while keeping
// IVM counting out of the walk (the parity management is independent of R8,
// section 19.5.2 p.205), which isolates F14 from the F15 odd-R9 counting.
//
// Walk convention: every helper asserts at the CURRENT line's C0=4 character
// and then advances one 64-character line.

void t27_configure(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 2},
        {5, 0},  {6, 1},  {7, 63}, {8, 1},    {9, 2},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_to_c0(TestBench::kF10TargetC0);
}

// Assert the plain (C4, C9) row/line position at the current line start,
// then advance one line.
static void t27_step_plain(TestBench& test, const char* tag,
                           std::uint8_t c4, std::uint8_t c9) {
    test.expect_c4(std::string(tag) + " C4", c4);
    test.expect_line(std::string(tag) + " C9", c9);
    test.run_characters(64);
}

void t27_type0_addline_basic(TestBench& test) {
    t27_configure(test);
    // Frame 0 (ParityFrame even): three rows of three lines.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27a frame 0 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    // Section 19.6.1 p.216: the additional line at C4=R4+1 with the
    // adjustment count continued to C9=R5=0, adjustment still active.
    test.expect_c4("t27a frame 0 additional line C4=R4+1", 3);
    test.expect_line("t27a frame 0 additional line C9=R5", 0);
    test.expect_adjustment_active("t27a frame 0 additional line is an adjustment line");
    // The origin moves past the additional line: frame 1 opens with
    // ParityFrame := ParityR6 = 1 (section 19.5.2 p.205 snapshot).
    test.run_characters(64);
    test.expect_c4("t27a frame 1 opens after the additional line", 0);
    test.expect_line("t27a frame 1 opens at C9=0", 0);
    test.expect_parity_frame("t27a frame 1 is odd (ParityR6 snapshot)", 1);
    // Frame 1 (odd): its capture makes ParityR6 even, so no additional line.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27a frame 1 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27a frame 2 opens directly (no line on the odd frame)", 0);
    test.expect_line("t27a frame 2 opens at C9=0", 0);
    test.expect_parity_frame("t27a frame 2 is even", 0);
    // Frame 2 (even): ParityR6 odd again, line appended once more.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27a frame 2 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27a frame 2 additional line C4=R4+1", 3);
    test.expect_line("t27a frame 2 additional line C9=R5", 0);
    test.run_characters(64);
    test.expect_parity_frame("t27a frame 3 is odd again", 1);
}

// t27b: R5=2 -- the additional line sits after the two R5 adjustment lines
// ("after the R5 lines if necessary", section 19.6.1 p.216) and shares
// C4=R4+1 with them ("incremented only once", same section); its C9
// continues the adjustment count to C9=R5 ("as if this line had been added
// to R5", section 11.2 p.84).
void t27_type0_addline_after_r5_lines(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 2},
        {5, 2},  {6, 1},  {7, 63}, {8, 1},    {9, 2},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_to_c0(TestBench::kF10TargetC0);
    // Frame 0: rows 0..2, then the two R5 adjustment lines at C4=R4+1.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27b frame 0 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    t27_step_plain(test, "t27b frame 0 adjustment 0", 3, 0);
    t27_step_plain(test, "t27b frame 0 adjustment 1", 3, 1);
    // The additional line: C4 held at R4+1 (single increment), C9=R5.
    test.expect_c4("t27b additional line holds C4=R4+1", 3);
    test.expect_line("t27b additional line continues the count to C9=R5", 2);
    test.expect_adjustment_active("t27b additional line is an adjustment line");
    // Frame 1 opens after it, odd.
    test.run_characters(64);
    test.expect_c4("t27b frame 1 opens after the additional line", 0);
    test.expect_parity_frame("t27b frame 1 is odd", 1);
    // Frame 1 (odd): adjustment ends the frame directly, no additional line.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27b frame 1 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    t27_step_plain(test, "t27b frame 1 adjustment 0", 3, 0);
    t27_step_plain(test, "t27b frame 1 adjustment 1", 3, 1);
    test.expect_c4("t27b frame 2 opens directly (odd frame)", 0);
    test.expect_parity_frame("t27b frame 2 is even", 0);
}

// t27c: R6>R4 freeze with ParityR6 frozen ODD -- the gate persists and the
// additional line is generated every frame "as long as R6>R4 (and R8=3 or
// 1), whatever the parity of the C9's" (section 19.6.1 p.216); every origin
// snapshots ParityFrame := 1, so every frame is odd-parity.
void t27_type0_addline_freeze_odd(TestBench& test) {
    t27_configure(test);
    // Frame 0: capture fires (C4 reaches R6=1), line appended, frame 1 odd.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27c frame 0 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27c frame 0 additional line", 3);
    test.run_characters(64);
    test.expect_parity_frame("t27c frame 1 is odd", 1);
    // Freeze R6>R4 during frame 1's C4=0 row, before its C4=0->1 crossing:
    // ParityR6 keeps its odd value for the rest of the run.
    test.write_register(6, 63);
    // Frame 1 ends with an additional line despite being odd-parity.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27c frame 1 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27c frozen-odd frame 1 still gains the line", 3);
    test.run_characters(64);
    test.expect_parity_frame("t27c frame 2 snapshots the frozen odd parity", 1);
    // And frame 2 does too: the freeze persists.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27c frame 2 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27c frozen-odd frame 2 gains the line", 3);
    test.run_characters(64);
    test.expect_parity_frame("t27c frame 3 odd again", 1);
}

// t27d: R6>R4 freeze with ParityR6 frozen EVEN -- "all the frames will
// remain even and without additional line" (section 19.6.1 p.216).  The
// freeze write lands after frame 1's capture so ParityR6=0 is the frozen
// value.
void t27_type0_addline_freeze_even(TestBench& test) {
    t27_configure(test);
    // Frame 0: capture (odd), line appended, frame 1 odd -- as t27a.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27d frame 0 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27d frame 0 additional line", 3);
    test.run_characters(64);
    test.expect_parity_frame("t27d frame 1 is odd", 1);
    // Frame 1's capture fires at its C4=0->1 crossing -- the end of the
    // C4=0 row's last line (walk line 12, C9=R9) -- setting ParityR6 := 0.
    // The freeze write then lands during the C4=1 row (walk line 13).
    test.run_characters(64);
    test.run_characters(64);
    test.run_characters(64);
    test.write_register(6, 63);
    // Walk the rest of frame 1 (lines 13..18) and cross into frame 2: no
    // additional line, the origin follows the last row directly.
    t27_step_plain(test, "t27d frame 1 line", 1, 0);
    t27_step_plain(test, "t27d frame 1 line", 1, 1);
    t27_step_plain(test, "t27d frame 1 line", 1, 2);
    t27_step_plain(test, "t27d frame 1 line", 2, 0);
    t27_step_plain(test, "t27d frame 1 line", 2, 1);
    t27_step_plain(test, "t27d frame 1 line", 2, 2);
    test.expect_c4("t27d frozen-even frame 1 ends without the line", 0);
    test.expect_line("t27d frame 2 opens at C9=0", 0);
    test.expect_parity_frame("t27d frame 2 is even (frozen snapshot)", 0);
    // Frame 2: no capture possible, still no line.
    for (unsigned i = 0; i < 9; ++i) {
        t27_step_plain(test, "t27d frame 2 line", static_cast<std::uint8_t>(i / 3),
                       static_cast<std::uint8_t>(i % 3));
    }
    test.expect_c4("t27d frozen-even frame 2 ends without the line", 0);
    test.expect_parity_frame("t27d frame 3 stays even", 0);
}

// ---------------------------------------------------------------------------
// t28: F14 additional interlace line, type 1 (ACCC v1.10 section 19.6.2
// p.216; section 11.2.4 p.84; Q10 resolution in accc-author-questions.md
// item 10).
//
// Paper derivation:
//
//   - Gate: the line is added at the end of the frame (after the R5 lines)
//     iff an interlace mode is active (R8=1 or 3) and ParityFrame is even
//     (section 19.6.2 p.216).  The C4 increment for it happens "once again
//     on all even frames" when R9+1 is a multiple of R5 (same section) --
//     the adjudicated Q10 reading, which the fixture register set satisfies
//     (R9=7, R5=4: 8 = 2x4).  With R5=0 the multiple condition is vacuous,
//     so a type-1 frame without adjustment lines never gains one (this is
//     what keeps the t21-t24 IVM walks, all R5=0, undisturbed).
//
//   - Mechanics: type-1 adjustment rows already increment C4 past R4
//     (section 11.1; the p.83 table: adjustment rows at R4+1, R4+2, ...),
//     and the adjustment ends when C5+1 equals R5 by equality (section
//     11.3.2).  On a gated even frame the pending end instead runs one more
//     line: "C9 counts up to R9 and when it goes back to 0, C4 is
//     incremented without taking R4 into account" (section 11.2.4 p.84) --
//     the additional line holds C9=0 and C4 one past the last adjustment
//     row, then the frame origin follows.  This reproduces the section
//     11.2.3 worked example's R5=8 sub-case exactly (R4=37, R9=7: the R5
//     lines at C4=38, the additional line at C4=39); the example's R5=7
//     sub-case (8 % 7 != 0) has no type-1 line under the section 19.6.2
//     condition and is the CRTC 2 accounting (section 11.2.5), recorded as
//     a source-attribution residual in the F10 notes.
//
// Fixture frame: R0=63, R4=2 (three rows), R9=7 (type-1 IVM rows are the
// four lines C9=0,2,4,6 with ParityC9 held -- section 19.8.2 p.225), R5=4,
// R8=3 from a snapshot load (frame-boundary IVM entry, no toggle stages),
// R6/R7=63 (no capture, no VSYNC).  ParityFrame toggles at every type-1
// frame origin regardless of R8 (section 19.5.3 p.208), so frame 0 is even
// and frame 1 odd by construction.
//
// Walk (t28a): frame 0 = 12 row lines + 4 adjustment lines (C4=3, C9=0..3,
// C5=0..3), then the additional line (C4=4, C9=0, C5=0), then the origin
// (C4=0, ParityFrame=1).  Frame 1 (odd) runs the same 16 lines but ends
// directly: its last adjustment line is followed immediately by C4=0.

void t28_configure(TestBench& test, std::uint8_t r5) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x11}, {4, 2},
        {5, r5}, {6, 63}, {7, 63}, {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_characters(2);
    const std::array<std::uint8_t, 10> snapshot = {
        63, 40, 50, 0x11, 2, r5, 63, 63, 3, 7,
    };
    test.load_snapshot_registers(snapshot);
    test.run_to_c0(TestBench::kF10TargetC0);
}

static void t28_step_adjustment(TestBench& test, const char* tag,
                                std::uint8_t c9) {
    test.expect_c4(std::string(tag) + " C4", 3);
    test.expect_line(std::string(tag) + " C9", c9);
    test.expect_c5(std::string(tag) + " C5", c9);
    test.run_characters(64);
}

void t28_type1_addline_basic(TestBench& test) {
    t28_configure(test, 4);
    // Frame 0 (even): three IVM rows of four lines (C9 = 0,2,4,6).
    for (unsigned i = 0; i < 12; ++i) {
        t27_step_plain(test, "t28a frame 0 line", static_cast<std::uint8_t>(i / 4),
                       static_cast<std::uint8_t>((i % 4) * 2));
    }
    // Adjustment: C4=R4+1, C9 counts plainly, C5 counts the lines.
    for (unsigned i = 0; i < 4; ++i) {
        t28_step_adjustment(test, "t28a frame 0 adjustment", static_cast<std::uint8_t>(i));
    }
    // Section 19.6.2 p.216 + section 11.2.4 p.84: the additional line,
    // C4 incremented once more, C9 back at 0.
    test.expect_c4("t28a additional line C4 incremented once more", 4);
    test.expect_line("t28a additional line C9=0", 0);
    test.expect_c5("t28a additional line C5 restarts", 0);
    // Origin: ParityFrame toggles (section 19.5.3 p.208), adjustment ends.
    test.run_characters(64);
    test.expect_c4("t28a frame 1 opens after the additional line", 0);
    test.expect_line("t28a frame 1 opens at C9=0", 0);
    test.expect_parity_frame("t28a frame 1 is odd", 1);
    test.expect_adjustment_inactive("t28a adjustment ended at the origin");
    // Frame 1 (odd): same rows and adjustment, but no additional line --
    // the last adjustment line is followed directly by the origin.
    for (unsigned i = 0; i < 12; ++i) {
        t27_step_plain(test, "t28a frame 1 line", static_cast<std::uint8_t>(i / 4),
                       static_cast<std::uint8_t>((i % 4) * 2));
    }
    for (unsigned i = 0; i < 4; ++i) {
        t28_step_adjustment(test, "t28a frame 1 adjustment", static_cast<std::uint8_t>(i));
    }
    test.expect_c4("t28a frame 2 opens directly (odd frame)", 0);
    test.expect_line("t28a frame 2 opens at C9=0", 0);
    test.expect_parity_frame("t28a frame 2 is even", 0);
}

// t28c: the same mechanism reached through INTERLACE SYNC (R8=1) -- the
// section 19.6.2 gate is R8 in 1,3, and every other type-1 F14 vector runs
// R8=3 (review non-blocking 4).  Plain (non-IVM) rows of eight lines;
// otherwise identical to t28a.
void t28_type1_addline_interlace_sync(TestBench& test) {
    test.set_crtc_type(1);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x11}, {4, 2},
        {5, 4},  {6, 63}, {7, 63}, {8, 1},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_to_c0(TestBench::kF10TargetC0);
    // Frame 0 (even): three plain rows of eight lines (C9 = 0..7).
    for (unsigned i = 0; i < 24; ++i) {
        t27_step_plain(test, "t28c frame 0 line", static_cast<std::uint8_t>(i / 8),
                       static_cast<std::uint8_t>(i % 8));
    }
    for (unsigned i = 0; i < 4; ++i) {
        t28_step_adjustment(test, "t28c frame 0 adjustment", static_cast<std::uint8_t>(i));
    }
    // The additional line: C4 one past the last adjustment row, C9=0.
    test.expect_c4("t28c additional line C4 incremented once more", 4);
    test.expect_line("t28c additional line C9=0", 0);
    test.expect_c5("t28c additional line C5 restarts", 0);
    // Origin: ParityFrame toggles, adjustment ends.
    test.run_characters(64);
    test.expect_c4("t28c frame 1 opens after the additional line", 0);
    test.expect_parity_frame("t28c frame 1 is odd", 1);
    test.expect_adjustment_inactive("t28c adjustment ended at the origin");
    // Frame 1 (odd): no additional line -- direct origin.
    for (unsigned i = 0; i < 24; ++i) {
        t27_step_plain(test, "t28c frame 1 line", static_cast<std::uint8_t>(i / 8),
                       static_cast<std::uint8_t>(i % 8));
    }
    for (unsigned i = 0; i < 4; ++i) {
        t28_step_adjustment(test, "t28c frame 1 adjustment", static_cast<std::uint8_t>(i));
    }
    test.expect_c4("t28c frame 2 opens directly (odd frame)", 0);
    test.expect_parity_frame("t28c frame 2 is even", 0);
}

// t28b: condition control -- R5=3 does not divide R9+1=8, so even the
// ParityFrame-even frame 0 must end directly after its adjustment lines
// (section 19.6.2 p.216: the once-more increment requires the multiple).
// Required pass from the start: it pins the gate, not the mechanism.
void t28_type1_addline_condition_false(TestBench& test) {
    t28_configure(test, 3);
    for (unsigned i = 0; i < 12; ++i) {
        t27_step_plain(test, "t28b frame 0 line", static_cast<std::uint8_t>(i / 4),
                       static_cast<std::uint8_t>((i % 4) * 2));
    }
    for (unsigned i = 0; i < 3; ++i) {
        t28_step_adjustment(test, "t28b frame 0 adjustment", static_cast<std::uint8_t>(i));
    }
    test.expect_c4("t28b no additional line when R9+1 is not a multiple of R5", 0);
    test.expect_line("t28b frame 1 opens at C9=0", 0);
    test.expect_parity_frame("t28b frame 1 is odd", 1);
    test.expect_adjustment_inactive("t28b adjustment ended at the origin");
}

// ---------------------------------------------------------------------------
// t29: F15 type-0 odd-R9 IVM counting (ACCC v1.10 section 19.5.2 pp.205-206
// including the worked R9=7 example table (render-verified 2026-08-26);
// section 19.8.1 pp.219-220; the p.219 row-end gate adjudicated as
// `If R9.0=1` in author question Q19; Q19(b) post-exit behavior stays out
// of scope).
//
// Paper derivation from the p.206 table (both columns reproduced line for
// line by the model below):
//
//   - Row shape: a steady IVM row ends at the first C9.VMA at or past R9.
//     With R9=7: odd-parity rows (ParityC9=1) run 1,3,5,7 and end at R9
//     (four lines); even-parity rows run 0,2,4,6,8 and end at R9+1 (five
//     lines).  The p.220 prose form ("C9x2+ParityFrame equals R9 or
//     ParityC9") cannot terminate even-parity rows for odd R9 and is
//     superseded by the rendered table, exactly as the printed p.219
//     pseudocode line was superseded at Q19(a).
//   - Row end: C9 restarts at 0 and (R9 odd only) ParityC9 := C4.0(new)
//     xor ParityFrame -- the pseudocode's post-increment C4.0 -- which
//     alternates the row parity within a frame and re-anchors it to the
//     frame parity at each origin (the table's frame-start rows: even
//     frame C4=0 opens at C9.VMA 0, odd frame at C9.VMA 1).
//   - Switch line: raw C9 against R9 + ParityFrame (p.219 prose; the
//     overflow sentence "If C9=R9 and the parity is odd, then the test
//     C9=R9+1 is false" pins the addition form).  Even-R9 behavior is
//     bit-identical to the implemented "R9 or ParityFrame".
//   - VSYNC delay: with R7 odd the pulse starts one line later on the
//     ParityFrame-odd frame -- at the second line of C4=R7, where
//     C9.VMA=2 (p.205-206 prose and the table's VSYNC boxes; the physical
//     line offset of C4=R7 then matches between the frames).  Even R7
//     needs no correction (the frames already agree).  The within-line
//     phase follows the existing field mechanics; only the start line
//     moves.
//
// All three fixtures enter IVM with R8=3 programmed before reset (no
// toggle stages; the t23b/t24 convention), R0=63.

// Common per-line sample: assert C4, raw C9 and the composed RA at the
// current line start, then advance one 64-character line.
static void t29_step(TestBench& test, const char* tag,
                     std::uint8_t c4, std::uint8_t c9, std::uint8_t ra) {
    test.expect_c4(std::string(tag) + " C4", c4);
    test.expect_line(std::string(tag) + " C9", c9);
    test.expect_ra(std::string(tag) + " RA", ra);
    test.run_characters(64);
}

// t29a: even frame, steady state (R4=63 keeps the origin out of the walk).
// C4=0: C9.VMA 0,2,4,6,8 (row ends at R9+1=8); ParityC9 := 1^0 = 1.
// C4=1: C9.VMA 1,3,5,7 (ends at R9=7); ParityC9 := 0^0 = 0.  C4=2 repeats
// the even-parity row.  This is the p.206 table's PARITYFRAME=EVEN column.
void t29_type0_odd_r9_even_frame(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 63},
        {5, 0},  {6, 63}, {7, 63}, {8, 3},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_to_c0(TestBench::kF10TargetC0);
    // C4=0: five lines, even C9.VMA (section 19.5.2 p.206, even frame).
    t29_step(test, "t29a c4=0", 0, 0, 0);
    t29_step(test, "t29a c4=0", 0, 1, 2);
    t29_step(test, "t29a c4=0", 0, 2, 4);
    t29_step(test, "t29a c4=0", 0, 3, 6);
    t29_step(test, "t29a c4=0", 0, 4, 8);
    // Row end: ParityC9 = C4.0(new)=1 xor ParityFrame=0 (Q19 gate).
    t29_step(test, "t29a c4=1", 1, 0, 1);
    test.expect_parity_c9("t29a row-end ParityC9 update (odd R9)", 1);
    t29_step(test, "t29a c4=1", 1, 1, 3);
    t29_step(test, "t29a c4=1", 1, 2, 5);
    t29_step(test, "t29a c4=1", 1, 3, 7);
    // ParityC9 back to 0; the even-parity row shape repeats at C4=2.
    t29_step(test, "t29a c4=2", 2, 0, 0);
    test.expect_parity_c9("t29a row-end ParityC9 update alternates", 0);
    t29_step(test, "t29a c4=2", 2, 1, 2);
    t29_step(test, "t29a c4=2", 2, 2, 4);
    t29_step(test, "t29a c4=2", 2, 3, 6);
    t29_step(test, "t29a c4=2", 2, 4, 8);
}

// t29b: odd frame via a real frame boundary (R4=2, R6=1 so ParityR6
// alternates the snapshot each origin).  Frame 0 (even) ends with the F14
// additional line (C4=3, C9=0) and opens frame 1 odd.  Frame 1: C4=0 runs
// C9.VMA 1,3,5,7 (ParityC9 = frame parity = 1), C4=1 runs 0,2,4,6,8 --
// the p.206 table's PARITYFRAME=ODD column, steady rows.
void t29_type0_odd_r9_odd_frame(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 2},
        {5, 0},  {6, 1},  {7, 63}, {8, 3},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_to_c0(TestBench::kF10TargetC0);
    // Frame 0 (even): first row only, then hop to the F14 additional line.
    for (unsigned i = 0; i < 5; ++i) {
        t29_step(test, "t29b frame 0", 0, static_cast<std::uint8_t>(i),
                 static_cast<std::uint8_t>(i * 2));
    }
    // Skip C4=1 (4 lines) and C4=2 (5 lines): land on the additional line.
    test.run_characters(9 * 64);
    test.expect_c4("t29b frame 0 additional line (F14)", 3);
    test.expect_line("t29b frame 0 additional line C9=R5", 0);
    // Origin opens frame 1 with ParityFrame := ParityR6 = 1.
    test.run_characters(64);
    test.expect_c4("t29b frame 1 opens", 0);
    test.expect_parity_frame("t29b frame 1 is odd", 1);
    // Frame 1 C4=0: ParityC9 = frame parity = 1: C9.VMA 1,3,5,7.
    t29_step(test, "t29b frame 1 c4=0", 0, 0, 1);
    t29_step(test, "t29b frame 1 c4=0", 0, 1, 3);
    t29_step(test, "t29b frame 1 c4=0", 0, 2, 5);
    t29_step(test, "t29b frame 1 c4=0", 0, 3, 7);
    // Row end: ParityC9 = 1 xor 1 = 0; C4=1 runs the even-parity row.
    t29_step(test, "t29b frame 1 c4=1", 1, 0, 0);
    test.expect_parity_c9("t29b frame 1 row-end update", 0);
    t29_step(test, "t29b frame 1 c4=1", 1, 1, 2);
    t29_step(test, "t29b frame 1 c4=1", 1, 2, 4);
    t29_step(test, "t29b frame 1 c4=1", 1, 3, 6);
    t29_step(test, "t29b frame 1 c4=1", 1, 4, 8);
    // C4=2 re-derives ParityC9 = 0 xor 1 = 1.
    t29_step(test, "t29b frame 1 c4=2", 2, 0, 1);
    test.expect_parity_c9("t29b frame 1 alternation continues", 1);
    // Skip the rest of C4=2 (3 lines); frame 1 ends without an additional
    // line (its capture made ParityR6 even) and frame 2 opens even.
    test.run_characters(3 * 64);
    test.expect_c4("t29b frame 2 opens directly (odd frame)", 0);
    test.expect_parity_frame("t29b frame 2 is even", 0);
}

// t29c: the section 19.5.2 VSYNC delay-by-1-line correction.  R7=1 (odd):
// the even frame starts the pulse at the first line of C4=1 (C9.VMA=1);
// the odd frame delays it to the second line (C9.VMA=2, the documented
// fire condition).  Sampled at C0=36, after the half-line tick the field=1
// count uses: the delayed pulse reads high at VMA=2 (it started at that
// line's half-line tick) where the undelayed pulse would already read low.
void t29_type0_odd_r9_vsync_delay(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x21}, {4, 2},
        {5, 0},  {6, 1},  {7, 1},  {8, 3},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    test.run_to_c0(36);
    // Frame 0 (even, ParityFrame=0): no delay.  C4=0 runs five quiet lines.
    for (unsigned i = 0; i < 5; ++i) {
        test.expect_vsync_low("t29c even frame C4=0 line: pulse not yet due");
        test.run_characters(64);
    }
    // C4=1 (VMA 1,3,5,7): pulse starts at the first line, R3v=2 wide.
    test.expect_vsync_high("t29c even frame pulse starts at C4=1 first line");
    test.expect_c4("t29c even frame pulse is on C4=1", 1);
    test.expect_ra("t29c even frame pulse starts at C9.VMA=1", 1);
    test.run_characters(64);
    test.expect_vsync_high("t29c even frame pulse second line");
    test.run_characters(64);
    test.expect_vsync_low("t29c even frame pulse ended");
    test.run_characters(64);
    test.expect_vsync_low("t29c even frame pulse ended (2/2)");
    test.run_characters(64);
    // Skip the rest of frame 0 (C4=2's five lines), the F14 additional
    // line and the origin into frame 1.
    test.run_characters(7 * 64);
    test.expect_parity_frame("t29c frame 1 is odd", 1);
    // Frame 1 (odd, ParityFrame=1): C4=0 runs four quiet lines; three are
    // sampled here and the fourth (the first C4=1 line) in the block below.
    for (unsigned i = 0; i < 3; ++i) {
        test.expect_vsync_low("t29c odd frame C4=0 line");
        test.run_characters(64);
    }
    // C4=1 (VMA 0,2,4,6,8): the pulse is delayed one line -- quiet at
    // VMA=0, up from VMA=2's half-line tick (the documented C4=R7 /
    // C9.VMA=2 fire) through VMA=4, down for VMA=6..8.  R3v=2 counts two
    // half-line ticks on the field=1 frame, i.e. one full line.
    test.expect_vsync_low("t29c odd frame: no pulse at the first C4=1 line");
    test.expect_c4("t29c odd frame first C4=1 line is C4=1", 1);
    test.expect_ra("t29c odd frame first C4=1 line is C9.VMA=0", 0);
    test.run_characters(64);
    test.expect_vsync_high("t29c odd frame delayed pulse at C9.VMA=2");
    test.expect_ra("t29c odd frame delayed pulse line is C9.VMA=2", 2);
    test.run_characters(64);
    test.expect_vsync_high("t29c odd frame delayed pulse still up at C9.VMA=4");
    test.run_characters(64);
    test.expect_vsync_low("t29c odd frame pulse ended at C9.VMA=6");
    test.run_characters(64);
    test.expect_vsync_low("t29c odd frame quiet at C9.VMA=8");
}

// t29d: the odd-R9 switch line (section 19.8.1 p.219).  The switch line
// tests raw C9 against R9 + ParityFrame -- the addition form, pinned by
// the overflow sentence: on an odd frame with R9=7 the target is 8, so a
// switch landing on the raw C9=7 line does NOT end the row and C9
// overflows to 8 (an OR form would target 7, end the row, and reset C9).
// The write lands mid-line on the C4=0 row of an odd frame; the next line
// runs doubled (IVM on from its seam) with C9=8 and C9.VMA = 16+1 = 17.
void t29_type0_odd_r9_switch_line_overflow(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 63}, {1, 40}, {2, 50}, {3, 0x00}, {4, 2},
        {5, 0},  {6, 1},  {7, 63}, {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    // Frame 0 (even, R8=0): 24 plain lines, no additional line, origin
    // opens frame 1 odd (section 19.5.2 p.205 snapshot).
    test.run_characters(24 * 64);
    test.run_to_c0(TestBench::kF10TargetC0);
    test.expect_c4("t29d frame 1 opens", 0);
    test.expect_parity_frame("t29d frame 1 is odd", 1);
    // Hop to the C4=0 row's last line (raw C9=7) and switch to IVM there.
    test.run_characters(7 * 64);
    test.run_to_c0(20);
    test.select_register(8);
    test.write_selected_register_at_nclken(3);
    // The switch line must not end: target R9+ParityFrame = 8 != 7.
    test.run_characters(48);  // land at C0=4 of the next line
    test.expect_c4("t29d overflow: no row end at the switch line", 0);
    test.expect_line("t29d overflow: C9 runs past R9", 8);
    test.expect_ra("t29d overflow: doubled display from the frame parity", 17);
}

// t29e: the F15 delay state must not leak across a live type switch
// (review blocking 1).  An armed d1 clears on the first CLOCK edge after
// CRTC_TYPE rises, and the wrapper samples the pre-edge value on that same
// edge -- so the stale window is exactly one edge, reachable only when the
// switch happens between the last pre-count-tick posedge and the count
// tick itself.  The vector arms d1 at the C4=0->1 crossing of an odd
// frame (the natural type-0 fire point, suppressed by the documented
// one-line delay), rewrites R8 to 0 so the type-1 side takes the plain
// field branch, burns phases 1..15, switches type after phase 15's edge,
// and requires the type-1 natural VSYNC fire on the immediately following
// count tick (hcc_next==R0/2 with R0=3, row==R7=1, line==0).
void t29_type0_delay_arm_clears_on_type_switch(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 1}, {2, 1}, {3, 0x21}, {4, 2},
        {5, 0}, {6, 1}, {7, 1}, {8, 3},    {9, 1},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();
    // Frame 0 (even, ParityFrame=0): rows 0..2 run 2/1/2 lines plus the F14
    // additional line; the sixth line wrap opens frame 1 odd.  24
    // characters from the post-reset hcc=1 lands at C0=1 of frame 1's
    // first line.
    test.run_characters(24);
    test.expect_parity_frame("t29e frame 1 is odd", 1);
    // Rewrite R8 to 0 during frame 1's C4=0 row: the leaving toggle keeps
    // the plain-R9 target for the row-end match, and the type-1 side after
    // the switch takes the plain field branch (interlace bit 0 cleared).
    test.write_register(8, 0);
    // Cross the row-end edge (the natural type-0 fire point: row_next==R7,
    // line_last on C9.VMA=1): the fire is suppressed and d1 arms.  The hop
    // stops just past that edge, at C0=0 of the C4=1 row with row==R7 and
    // line==0.
    test.run_to_c0(0);
    // Burn phases 1..15 of this character; the next tick is the phase-0
    // CLKEN that increments hcc 0->1, i.e. the field count tick.
    test.run_clock_ticks(15);
    // Switch type after phase 15's posedge: the count tick is then the
    // first post-switch edge, sampling the stale pre-clear d1.
    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    // The fire sets VSYNC_r at that edge; the output register follows one
    // tick later.
    test.run_clock_ticks(2);
    test.expect_vsync_high("t29e type-1 natural fire survives the switch");
}

}  // namespace
//============================================================================
//  Randomized equivalence soak
//
//  See sim/README.md ("Randomized equivalence soak"). Deterministic
//  self-referential oracle: pseudo-random register traffic at arbitrary C0
//  values and CLKEN/nCLKEN tick phases over both CRTC types, with a rolling
//  FNV-1a hash over every pin plus key internal state sampled at each
//  CLKEN. The golden hash is minted from the unsplit core; a
//  behaviour-preserving refactor (the type-0/type-1 engine split) must
//  reproduce it exactly. This complements the directed vectors; it never
//  replaces them.
//============================================================================

// Fixed seed: part of the golden-hash contract. Changing it invalidates a
// minted hash and requires re-minting.
constexpr std::uint64_t kSoakSeed = 0xaccc5eed20260822ULL;
constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;

class SoakRng {
public:
    explicit SoakRng(std::uint64_t seed) : state_(seed) {}

    // splitmix64: small, fast, fully deterministic across platforms.
    std::uint64_t next() {
        state_ += 0x9e3779b97f4a7c15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    std::uint64_t below(std::uint64_t bound) {
        return next() % bound;
    }

private:
    std::uint64_t state_;
};

std::uint8_t soak_random_value(SoakRng& rng) {
    // Boundary-biased distribution: degenerate totals (R0=0, R9=0, R5=0)
    // reach the arbitration and freeze corners that directed vectors had to
    // be hand-guided into.
    const std::uint64_t roll = rng.below(100);
    if (roll < 40) {
        return static_cast<std::uint8_t>(rng.below(4));
    }
    if (roll < 65) {
        return static_cast<std::uint8_t>(rng.below(16));
    }
    if (roll < 90) {
        return static_cast<std::uint8_t>(rng.below(256));
    }
    constexpr std::array<std::uint8_t, 14> kBoundaries = {{
        0x00, 0x01, 0x02, 0x03, 0x0f, 0x1f, 0x3f,
        0x40, 0x7f, 0x80, 0xc0, 0xf0, 0xfe, 0xff,
    }};
    return kBoundaries[rng.below(kBoundaries.size())];
}

void soak_bus_op(TestBench& bench, SoakRng& rng) {
    const std::uint64_t roll = rng.below(100);

    if (roll < 60) {
        // Register data write at whatever phase we are in, or aligned to
        // the two special bus phases. Occasional alias addresses (>31)
        // exercise the five-bit decode.
        unsigned address = rng.below(16);
        if (rng.below(32) == 0) {
            address |= 32u + rng.below(32);
        }
        const std::uint8_t value = soak_random_value(rng);
        const std::uint64_t style = rng.below(100);
        if (style < 55) {
            bench.write_register(static_cast<std::uint8_t>(address), value);
        } else if (style < 78) {
            bench.select_register(static_cast<std::uint8_t>(address));
            bench.write_selected_register_at_clken(value);
        } else {
            bench.select_register(static_cast<std::uint8_t>(address));
            bench.write_selected_register_at_nclken(value);
        }
    } else if (roll < 70) {
        // Reads do not change the DUT but put DO into the hashed window.
        if (rng.below(4) == 0) {
            bench.read_status();
        } else {
            bench.read_register(static_cast<std::uint8_t>(rng.below(32)));
        }
    } else if (roll < 76) {
        // Register-pointer reselect only.
        bench.select_register(static_cast<std::uint8_t>(rng.below(32)));
    } else if (roll < 80) {
        // Held multi-cycle write, as used by the directed vectors.
        bench.select_register(static_cast<std::uint8_t>(rng.below(16)));
        bench.hold_selected_register_at_clken(
            soak_random_value(rng),
            kClockTicksPerCharacter * (1 + rng.below(3)));
    } else if (roll < 82) {
        std::array<std::uint8_t, 10> snapshot{};
        for (std::uint8_t& value : snapshot) {
            value = soak_random_value(rng);
        }
        bench.load_snapshot_registers(snapshot);
    } else {
        // Idle stretch so frames and adjustment runs develop.
        bench.run_characters(rng.below(64));
    }
}

void run_soak(std::uint64_t expected_hash, bool check_expected) {
    SoakRng rng(kSoakSeed);
    constexpr std::uint64_t kEventsPerType = 150000;

    std::uint64_t hash = kFnvOffsetBasis;
    std::uint64_t samples = 0;
    std::uint64_t characters = 0;

    for (unsigned type = 0; type <= 1; ++type) {
        TestBench bench({});
        bench.set_clken_sampler([&bench, &hash, &samples]() {
            ++samples;
            bench.soak_mix_sample(hash);
        });

        bench.set_crtc_type(type);
        bench.prepare_for_reset(type);
        // Random base frame so most segments start near short-line/short-
        // frame territory; the random traffic then degrades it freely.
        for (unsigned address = 0; address <= 15; ++address) {
            bench.write_register(static_cast<std::uint8_t>(address),
                                 soak_random_value(rng));
        }

        unsigned current_type = type;
        for (std::uint64_t event = 0; event < kEventsPerType; ++event) {
            const std::uint64_t lifecycle = rng.below(1024);
            if (lifecycle == 0) {
                bench.prepare_for_reset(current_type);
            } else if (lifecycle == 1) {
                current_type ^= 1;
                bench.set_crtc_type(current_type);
            }

            soak_bus_op(bench, rng);
            bench.run_clock_ticks(rng.below(kClockTicksPerCharacter * 7));
        }
        characters += bench.total_characters();
    }

    std::cout << "soak: seed 0x" << std::hex << std::setfill('0')
              << std::setw(16) << kSoakSeed << ", " << std::dec
              << characters << " characters, " << samples
              << " CLKEN samples\n"
              << "soak hash: 0x" << std::hex << std::setw(16) << hash
              << '\n';

    if (!check_expected) {
        return;
    }
    if (hash != expected_hash) {
        std::cerr << "soak hash MISMATCH: expected 0x" << std::hex
                  << std::setw(16) << expected_hash << ", got 0x" << std::setw(16)
                  << hash << '\n';
        throw TestFailure("soak hash mismatch");
    }
    std::cout << "soak hash matches expected\n";
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) != "--soak") {
            continue;
        }
        bool check_expected = false;
        std::uint64_t expected_hash = 0;
        if (i + 1 < argc && argv[i + 1][0] != '\0') {
            check_expected = true;
            expected_hash =
                std::stoull(std::string(argv[i + 1]), nullptr, 16);
        }
        try {
            run_soak(expected_hash, check_expected);
        } catch (const std::exception& error) {
            std::cerr << "soak failed: " << error.what() << '\n';
            return 1;
        }
        return 0;
    }

    const std::vector<TestCase> tests = {
        {"t00_reset_and_idle_bus", "CRTC pin-level reset/bus contract", false,
         test_reset_and_idle_bus},
        {"t01_register_readback", "ACCC v1.10 sections 21.2 and 28.1.9; F1/F11c/F11d",
         false, test_register_readback_table},
        {"t02a_type0_r7_hcc0_blocked", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_hcc0_blocked},
        {"t02b_type0_r7_hcc1_blocked", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_hcc1_blocked},
        {"t02c_type0_r7_midline_extended", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_midline_duration_extended},
        {"t02d_type1_r7_early_hcc_immediate", "ACCC v1.10 section 16.4.2; F3",
         false, test_type1_r7_early_hcc_immediate},
        {"t02e_type1_r7_midline_partial_counts", "ACCC v1.10 section 16.4.2; F3",
         false, test_type1_r7_midline_partial_counts},
        {"t02f_r7_level_write_and_active_rearm", "ACCC v1.10 sections 16.3-16.4; F3",
         false, test_r7_level_write_and_active_rearm},
        {"t02g_type0_dynamic_vsync_width_extremes", "ACCC v1.10 sections 14.2 and 16.4.1; F3",
         false, test_type0_dynamic_vsync_width_extremes},
        {"t02h_type0_r7_c0_2_at_line_boundary", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_c0_2_at_line_boundary},
        {"t02i_type0_interlace_count_boundaries", "ACCC v1.10 sections 16.4-16.5; F3",
         false, test_type0_interlace_count_boundaries},
        {"t02j_type0_pending_skip_type_roundtrip", "CRTC live CRTC_TYPE contract; F3/F11d",
         false, test_type0_pending_skip_clears_on_type_roundtrip},
        {"t02k_type0_pending_skip_snapshot_load", "CRTC snapshot-load contract; F3/F11d",
         false, test_type0_pending_skip_clears_on_snapshot_load},
        {"t03a_vsync_compare_lock_and_rearm", "ACCC v1.10 section 16.3; F11b",
         false, test_vsync_compare_lock_and_rearm},
        {"t03b_vsync_reentrancy_bypass", "ACCC v1.10 section 16.3; F11b",
         false, test_vsync_reentrancy_bypass},
        {"t06a_status_waits_for_r0_sample", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_waits_for_r0_sample},
        {"t06b_status_r6_zero_forced_border", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_r6_zero_forced_border},
        {"t06c_status_samples_natural_r6_edge", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_samples_natural_r6_edge},
        {"t06d_status_clears_on_type_round_trip", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_clears_on_type_round_trip},
        {"t09a_type0_r0_zero_suppresses_nonzero_r2_hsync",
         "ACCC v1.10 section 13.2.1; F5", false,
        test_type0_r0_zero_suppresses_nonzero_r2_hsync},
        {"t09b_type0_r0_zero_allows_r2_zero_hsync",
         "ACCC v1.10 sections 13.2.1 and 15.3; F5", false,
        test_type0_r0_zero_allows_r2_zero_hsync},
        {"t09c_type0_r0_zero_resumes_after_nclken_write",
         "ACCC v1.10 section 13.2.1; F5", false,
         test_type0_r0_zero_resumes_after_nclken_write},
        {"t09d_type1_r0_zero_keeps_one_character_lines",
         "ACCC v1.10 section 13.3; F5 regression guard", false,
         test_type1_r0_zero_keeps_one_character_lines},
        {"t09e_type0_midline_r0_zero_free_runs_then_pins",
         "ACCC v1.10 section 13.2.1; F5 live-write regression guard", false,
         test_type0_midline_r0_zero_free_runs_then_pins},
        {"t09f_r0_zero_freeze_survives_type_round_trip",
         "CRTC live CRTC_TYPE contract; F5/F11d", false,
         test_r0_zero_freeze_survives_type_round_trip},
        {"t09g_type0_interlace_r0_zero_freezes_vsync_count",
         "ACCC v1.10 sections 13.2.1 and 16.5; F3/F5 regression guard", false,
         test_type0_interlace_r0_zero_freezes_vsync_count},
        {"t09h_type0_r0_zero_c9_equal_single_c4_increment_deferred",
         "ACCC v1.10 sections 13.2.1 and 13.2.6; F5/F12", false,
         test_type0_r0_zero_c9_equal_single_c4_increment_deferred},
        {"t16a_type0_r4_write_switches_c9_to_r5",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12 guard", false,
         test_type0_adjustment_r4_write_switches_c9_to_r5},
        {"t12a_type0_worked_example_exact_r0_yields_39_8",
         "ACCC v1.10 section 11.2.2 p.82 example 3 and section 10.3.1 p.76; F9/B4",
         false, test_type0_worked_example_exact_r0_yields_39_8},
        {"t12b_type0_worked_example_window_write_yields_38_8",
         "ACCC v1.10 section 11.2.2 p.82 example 3; F9/B4 companion case",
         false, test_type0_worked_example_window_write_yields_38_8},
        {"t16b_type0_r9_write_uses_new_r9",
         "ACCC v1.10 section 11.2.2; F12", false,
         test_type0_adjustment_r9_write_uses_new_r9},
        {"t16c_type0_accepts_r5_write_at_c0_2",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12", false,
         test_type0_adjustment_accepts_r5_write_at_c0_2},
        {"t16d_type0_rejects_r5_write_after_c0_2",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12 guard", false,
         test_type0_adjustment_rejects_r5_write_after_c0_2},
        {"t16e_type0_r9_write_at_r0_increments_c4_and_c9",
         "ACCC v1.10 section 11.2.2; F9/F12", false,
         test_type0_adjustment_r9_write_at_r0_increments_c4_and_c9},
        {"t16f_type0_adjustment_completion_resets_the_next_line",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12", false,
         test_type0_adjustment_completion_resets_the_next_line},
        {"t16g_type0_captures_mid_character_r4_write",
         "ACCC v1.10 section 11.2.2; F12 bus-phase guard", false,
         test_type0_adjustment_captures_mid_character_r4_write},
        {"t16h_type0_captures_mid_character_r9_write_at_r0",
         "ACCC v1.10 section 11.2.2; F9/F12 bus-phase guard", false,
         test_type0_adjustment_captures_mid_character_r9_write_at_r0},
        {"t16i_type0_r4_write_at_r0_switches_c9_to_r5",
         "ACCC v1.10 section 11.2.2; F12 boundary guard", false,
         test_type0_adjustment_r4_write_at_r0_switches_c9_to_r5},
        {"t16j_type0_captures_mid_character_r4_write_at_r0",
         "ACCC v1.10 section 11.2.2; F12 bus-phase boundary guard", false,
         test_type0_adjustment_captures_mid_character_r4_write_at_r0},
        {"t16k_type0_arbitration_latches_clear_on_snapshot_load",
         "CRTC snapshot-load contract; F12 lifecycle guard", false,
         test_type0_adjustment_latches_clear_on_snapshot_load},
        {"t16l_type0_arbitration_latches_clear_on_type_roundtrip",
         "CRTC live CRTC_TYPE contract; F12 lifecycle guard", false,
         test_type0_adjustment_latches_clear_on_type_roundtrip},
        {"t16m_type0_r4_write_at_c0_1_enters_zero_adjustment",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2; F12", false,
         test_type0_r4_write_at_c0_1_enters_zero_adjustment},
        {"t16n_type0_r9_write_within_c0_1_enters_zero_adjustment",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2; F12 bus-phase guard", false,
         test_type0_r9_write_within_c0_1_enters_zero_adjustment},
        {"t16o_type0_r4_write_at_c0_0_overrides_last_line",
         "ACCC v1.10 sections 10.3.1 and 12.2; F12 C0=0 guard", false,
         test_type0_r4_write_at_c0_0_overrides_last_line},
        {"t16p_type0_r0_one_runs_default_zero_adjustment_line",
         "ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.5; F5/F12", false,
         test_type0_r0_one_runs_default_zero_adjustment_line},
        {"t16q_type0_r0_zero_starts_default_zero_adjustment",
         "ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6; F5/F12", false,
         test_type0_r0_zero_starts_default_zero_adjustment},
        {"t16r_type0_r0_zero_during_adjustment_freezes_c4",
         "ACCC v1.10 section 13.2.3; F5/F12 interaction guard", false,
         test_type0_r0_zero_during_adjustment_freezes_c4},
        {"t16s_type0_r0_one_c0_1_break_is_consumed_at_rollover",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 13.2.1; F5/F12 boundary",
         false, test_type0_r0_one_c0_1_break_is_consumed_at_rollover},
        {"t16t_type0_adjustment_unequal_c9_counts_to_r5",
         "ACCC v1.10 sections 11.2.1 and 11.2.2; F12/F4 vertical adjustment C9 reuse",
         false, test_type0_adjustment_unequal_c9_counts_to_r5},
        {"t16u_type0_active_adjustment_r5_zero_counts_through_31",
         "ACCC v1.10 sections 10.3.1, 11.2.1, and 11.2.2; F4/F12 zero-R5 wrap",
         false, test_type0_active_adjustment_r5_zero_counts_through_31},
        {"t16v_type0_adjustment_last_line_r5_zero_retargets_to_31",
         "ACCC v1.10 sections 10.3.1, 11.2.1, and 11.2.2; F4/F12 zero-R5 retarget",
         false, test_type0_adjustment_last_line_r5_zero_retargets_to_31},
        {"t16x_type0_r5_same_line_rejected_write_does_not_retarget",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F4/F12 R5 target latch",
         false, test_type0_r5_same_line_rejected_write_does_not_retarget},
        {"t16y_type0_r5_same_line_rejected_zero_does_not_retarget",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F4/F12 inverse R5 target latch",
         false, test_type0_r5_same_line_rejected_zero_does_not_retarget},
        {"t16w_type0_zero_adj_entry_r5_positive_extends_adjustment",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2; F12 zero-entry positive R5",
         false, test_type0_zero_adj_entry_r5_positive_extends_adjustment},
        {"t07a_type1_c9_counts_through_31_and_wraps",
         "ACCC v1.10 sections 10.3 and 10.3.2; F4", false,
         test_type1_c9_counts_through_31_and_wraps},
        {"t07b_type1_c9_zero_limit_overflows",
         "ACCC v1.10 section 10.3; F4 zero-limit equality rollover", false,
         test_type1_c9_zero_limit_overflows},
        {"t07c_type1_c4_counts_through_127_and_wraps",
         "ACCC v1.10 sections 12 and 12.3; F4", false,
         test_type1_c4_counts_through_127_and_wraps},
        {"t07d_type1_c4_zero_limit_overflows",
         "ACCC v1.10 section 12.3; F4 zero-limit guard", false,
         test_type1_c4_zero_limit_overflows},
        {"t07e_type0_c9_counts_through_31_and_wraps",
         "ACCC v1.10 sections 10.3.1 and 12.2; F4", false,
         test_type0_c9_counts_through_31_and_wraps},
        {"t07f_type0_c9_zero_limit_overflows",
         "ACCC v1.10 section 10.3; F4 zero-limit equality rollover", false,
         test_type0_c9_zero_limit_overflows},
        {"t07g_type0_c4_counts_through_127_and_wraps",
         "ACCC v1.10 section 12.2; F4", false,
         test_type0_c4_counts_through_127_and_wraps},
        {"t07h_type0_c4_zero_limit_overflows",
         "ACCC v1.10 section 12.2; F4 zero-limit equality rollover", false,
         test_type0_c4_zero_limit_overflows},
        {"t07i_type0_rlal_zero_limit_arms_last_line",
         "ACCC v1.10 section 12.2; F4/F12 RLAL guard", false,
         test_type0_rlal_zero_limit_arms_last_line},
        {"t07j_type0_rlal_single_zero_limit_does_not_arm",
         "ACCC v1.10 section 12.2; F4/F12 RLAL guard", false,
         test_type0_rlal_single_zero_limit_does_not_arm},
        {"t07k_type0_rlal_first_line_delayed_arming",
         "ACCC v1.10 sections 10.3.1 and 12.2.1; F4/F12 equality rollover", false,
         test_type0_rlal_first_line_delayed_arming},
        {"t07l_type0_rlal_from_the_genuine_last_line",
         "ACCC v1.10 section 12.2.1; F4/F12 RLAL guard", false,
         test_type0_rlal_from_the_genuine_last_line},
        {"t07m_type0_r9_write_before_r1_advances_next_row_ma",
         "ACCC v1.10 sections 10.3.1 and 17.1; F4 live row_addr_save", false,
         test_type0_r9_write_before_r1_advances_next_row_ma},
        {"t08a_type0_identification_r7_36_fires",
         "ACCC v1.10 section 28.1.1; F4 control", false,
         test_type0_identification_r7_36_fires},
        {"t08b_type0_identification_r7_37_fires",
         "ACCC v1.10 sections 28.1.1 and 11.2.1; F4/F12", false,
         test_type0_identification_r7_37_fires},
        {"t08c_type0_identification_r7_38_is_silent",
         "ACCC v1.10 section 28.1.1; F4 type-0 boundary", false,
         test_type0_identification_r7_38_is_silent},
        {"t08d_type1_identification_r7_36_fires",
         "ACCC v1.10 section 28.1.1; F4 control", false,
         test_type1_identification_r7_36_fires},
        {"t08e_type1_identification_r7_37_fires",
         "ACCC v1.10 section 28.1.1; F4/F8 boundary", false,
         test_type1_identification_r7_37_fires},
        {"t08f_type1_identification_r7_38_fires",
         "ACCC v1.10 sections 28.1.1 and 11.2.1; F8", false,
         test_type1_identification_r7_38_fires},
        {"t08g_type1_identification_r7_39_is_silent",
         "ACCC v1.10 sections 16.1 and 16.4.2; F8/A1 supersedes old oracle", false,
         test_type1_identification_r7_39_is_silent},
        {"t08h_type1_identification_r7_40_is_silent",
         "ACCC v1.10 section 28.1.1; F4/F8 type-1 boundary", false,
         test_type1_identification_r7_40_is_silent},
        {"t08i_type1_adjustment_c4_c9_c5_worked_example",
         "ACCC v1.10 section 11.2.1; F8 worked example sequence", false,
         test_type1_adjustment_c4_c9_c5_worked_example},
        {"t08j_type1_r5_zero_mid_adjustment_keeps_counting",
         "ACCC v1.10 section 11.3.2; F8 zero R5 free-run and recovery", false,
         test_type1_r5_zero_mid_adjustment_keeps_counting},
        {"t08k_type0_adjustment_c4_frozen_c9_counts_to_r5",
         "ACCC v1.10 section 11.2.1; F8 type-0 control", false,
         test_type0_adjustment_c4_frozen_c9_counts_to_r5},
        {"t08l_type1_r4_zero_adjustment_vma_reloads_on_c4_one",
         "ACCC v1.10 section 11.2.4; F8/section 4.3 VMA reload on C4=1", false,
         test_type1_r4_zero_adjustment_vma_reloads_on_c4_one},
        {"t08m_type1_adjustment_end_does_not_fire_unreached_r7",
         "ACCC v1.10 sections 16.1 and 16.4.2; F8/A1", false,
         test_type1_adjustment_end_does_not_fire_unreached_r7},
        {"t08n_type1_r4_write_at_adjustment_entry_suppresses_r12_reload",
         "ACCC v1.10 section 11.2.4 p.84; F8/A2", false,
         test_type1_r4_write_at_adjustment_entry_suppresses_r12_reload},
        {"t08o_type1_r9_write_at_adjustment_entry_keeps_r12_reload",
         "ACCC v1.10 section 11.2.4 p.84; F8/A2/B5", false,
         test_type1_r9_write_at_adjustment_entry_keeps_r12_reload},
        {"t13a_type1_rfd_write_away_from_r0_stays_unarmed",
         "ACCC v1.10 section 11.6 p.87; F7 never-triggered control", false,
         test_type1_rfd_write_away_from_r0_stays_unarmed},
        {"t13b_type1_rfd_alternates_save_by_frame_parity",
         "ACCC v1.10 sections 11.6.1-11.6.3 pp.87-90; F7", false,
         test_type1_rfd_alternates_save_by_frame_parity},
        {"t13c_type1_rfd_r1_gt_r0_bare_c9_disarms",
         "ACCC v1.10 section 11.6 p.87; F7/B6", false,
         test_type1_rfd_r1_gt_r0_bare_c9_disarms},
        {"t13d_type1_rfd_final_line_write_enters_adjustment",
         "ACCC v1.10 sections 11.4 p.86 and 11.6 p.87; F7 rollover race", false,
         test_type1_rfd_final_line_write_enters_adjustment},
        {"t13e_type1_rfd_r0_widen_without_cancel_ends_normally",
         "ACCC v1.10 sections 13.3 p.113 and 13.7.1.2 p.124; F7 residual",
         false, test_type1_rfd_r0_widen_without_cancel_ends_normally},
        {"t13f_type1_rfd_r0_widen_r9_cancel_arms_at_extended_end",
         "ACCC v1.10 section 13.7.1.2 p.124 (digest-01 section 8.6); F7 residual",
         false, test_type1_rfd_r0_widen_r9_cancel_arms_at_extended_end},
        {"t13g_type1_rfd_r0_widen_r4_cancel_arms_and_advances_c4",
         "ACCC v1.10 section 13.7.1.2 p.124 (digest-01 section 8.6); F7 residual",
         false, test_type1_rfd_r0_widen_r4_cancel_arms_and_advances_c4},
        {"t13h_type1_rfd_r0_widen_restored_condition_does_not_arm",
         "ACCC v1.10 section 13.7.1.2 p.124 variant definitions; F7 residual",
         false, test_type1_rfd_r0_widen_restored_condition_does_not_arm},
        {"t13i_type1_rfd_equal_r0_write_opens_no_window",
         "ACCC v1.10 section 13.7.1.2 p.124 widened-only gate; F7 residual",
         false, test_type1_rfd_equal_r0_write_opens_no_window},
        {"t13j_type1_rfd_r0_widen_off_last_line_never_arms",
         "ACCC v1.10 sections 13.3 p.113 and 13.7.1.2 p.124; F7 residual",
         false, test_type1_rfd_r0_widen_off_last_line_never_arms},
        {"t13k_type1_rfd_r0_window_does_not_survive_type_round_trip",
         "ACCC v1.10 section 13.7.1.2 p.124; F7 live-type contract", false,
         test_type1_rfd_r0_window_does_not_survive_type_round_trip},
        {"t13l_type1_rfd_r0_widen_line_gate_never_arms",
         "ACCC v1.10 sections 13.3 p.113 and 13.7.1.2 p.124; F7/F-2", false,
         test_type1_rfd_r0_widen_line_gate_never_arms},
        {"t13m_type1_rfd_r0_extend_blanks_from_c0_r1",
         "ACCC v1.10 sections 6.1.3 p.33 and 13.7.1.2 p.124; F-1", false,
         test_type1_rfd_r0_extend_blanks_from_c0_r1},
        {"t20a_type0_normal_frame_reloads_at_frame_start_only",
         "ACCC v1.10 sections 17.4.1 and 20.3.1; F11h", false,
         test_type0_normal_frame_reloads_at_frame_start_only},
        {"t20b_type1_normal_frame_reloads_every_line_of_row0",
         "ACCC v1.10 sections 17.4.2 and 20.3.2; F11h", false,
         test_type1_normal_frame_reloads_every_line_of_row0},
        {"t20c_type0_r0_three_reloads_every_line",
         "ACCC v1.10 sections 13.8.1 and 20.3.1; F11h", false,
         test_type0_r0_three_reloads_every_line},
        {"t20d_type1_r0_three_reloads_every_line",
         "ACCC v1.10 sections 13.8.1 and 20.3.2; F11h", false,
         test_type1_r0_three_reloads_every_line},
        {"t20e_type0_r0_one_reloads_every_second_line",
         "ACCC v1.10 sections 13.2.5, 13.8.2, and 20.3.1; F11h", false,
         test_type0_r0_one_reloads_every_second_line},
        {"t20f_type1_r0_one_reloads_every_line",
         "ACCC v1.10 sections 13.8.2 and 20.3.2; F11h", false,
         test_type1_r0_one_reloads_every_line},
        {"t20g_type0_r0_zero_ignores_reload_after_hiccup",
         "ACCC v1.10 sections 13.2.6, 13.8.3, and 20.3.1; F5/F12/F11h", false,
         test_type0_r0_zero_ignores_reload_after_hiccup},
        {"t20h_type1_r0_zero_reloads_every_line",
         "ACCC v1.10 sections 13.3, 13.8.3, and 20.3.2; F11h", false,
         test_type1_r0_zero_reloads_every_line},
        {"t20i_type0_r0_zero_live_entry_reloads_vma_then_freezes",
         "ACCC v1.10 sections 13.2.6, 13.8.3, and 20.3.1; F5/F12/F11h (A3)",
         false, test_type0_r0_zero_live_entry_reloads_vma_then_freezes},
        {"t20j_type1_r12_write_on_row0_boundary_edge_reloads",
         "ACCC v1.10 section 20.3.2 p.242 chronogram 2; F11h", false,
         test_type1_r12_write_on_row0_boundary_edge_reloads},
        {"t20k_type0_r12_write_on_frame_origin_edge_is_missed",
         "ACCC v1.10 section 20.3.1 p.242 chronogram 2; F11h", false,
         test_type0_r12_write_on_frame_origin_edge_is_missed},
        {"t10a_type0_r1_gt_r0_spurious_border_byte",
         "ACCC v1.10 sections 17.6.2, 17.6.1, and 17.2; F6", false,
         test_type0_r1_gt_r0_spurious_border_byte},
        {"t10b_type1_r1_gt_r0_no_border_byte",
         "ACCC v1.10 sections 17.6.2 and 28.1.6; F6 type discriminator", false,
         test_type1_r1_gt_r0_no_border_byte},
        {"t10c_type0_spurious_byte_delayed_one_character",
         "ACCC v1.10 sections 19.2.4 and 19.2.3; F6 skew placement", false,
         test_type0_spurious_byte_delayed_one_character},
        {"t10d_type0_spurious_byte_delayed_two_characters",
         "ACCC v1.10 sections 19.2.4 and 19.2.3; F6 skew placement", false,
         test_type0_spurious_byte_delayed_two_characters},
        {"t10e_type0_skew_non_output_blanks",
         "ACCC v1.10 sections 19.2 and 19.1; F6 suppression path", false,
         test_type0_skew_non_output_blanks},
        // t21: F10 type-1 IVM toggle parity table (ACCC pp.210-211 panels).
        // The six all-zero configurations are required controls; the ten
        // with nonzero expectations carry the XFAIL marker until the
        // type-1 F10 behavior commit.
        {"t21a_type1_ivm_toggle_19S_23W",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 19/23 (p.210); F10",
         false, t21_body_00},
        {"t21b_type1_ivm_toggle_17Q_21U_25Y1",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 17/21/25 (p.210); F10",
         false, t21_body_01},
        {"t21c_type1_ivm_toggle_4D_8H",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 4/8 (p.210); F10",
         false, t21_body_02},
        {"t21d_type1_ivm_toggle_2B_6F",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 2/6 (p.210); F10",
         false, t21_body_03},
        {"t21e_type1_ivm_toggle_20T_24X",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 20/24 (p.210); F10",
         false, t21_body_04},
        {"t21f_type1_ivm_toggle_18R_22V_26Z1",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 18/22/26 (p.210); F10",
         false, t21_body_05},
        {"t21g_type1_ivm_toggle_5E_9I",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 5/9 (p.210); F10",
         false, t21_body_06},
        {"t21h_type1_ivm_toggle_3C_7G",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 3/7 (p.210); F10",
         false, t21_body_07},
        {"t21i_type1_ivm_toggle_27ZA_29ZC",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 27/29 (p.211); F10",
         false, t21_body_08},
        {"t21j_type1_ivm_toggle_16P_25Y2",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 16/25 (p.211); F10",
         false, t21_body_09},
        {"t21k_type1_ivm_toggle_12L_14N",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 12/14 (p.211); F10",
         false, t21_body_10},
        {"t21l_type1_ivm_toggle_1A_10J2",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 1/10 (p.211); F10",
         false, t21_body_11},
        {"t21m_type1_ivm_toggle_28ZB_30ZD",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests 28/30 (p.211); F10",
         false, t21_body_12},
        {"t21n_type1_ivm_toggle_26Z",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 test 26 (p.211); F10",
         false, t21_body_13},
        {"t21o_type1_ivm_toggle_M_15O",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 tests (M)/15 (p.211); F10",
         false, t21_body_14},
        {"t21p_type1_ivm_toggle_11K2",
         "ACCC v1.10 sections 19.5.3 pp.208-209 and SHAKER 22C/3 test 11 (p.211); F10",
         false, t21_body_15},
        // t22: F10 type-0 IVM entry/exit counting for even R9 (ACCC
        // pp.219-224).  All diverge from the pre-F10 stepping approximation
        // (C9 stepping by 2 with bit 0 masked, halved limit) and carry the
        // XFAIL marker until the type-0 F10 behavior commit.
        {"t22a_type0_ivm_entry_even_c9_0",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=0, even frame); F10",
         false, t22_entry_even_0},
        {"t22b_type0_ivm_entry_even_c9_1",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=1, even frame); F10",
         false, t22_entry_even_1},
        {"t22c_type0_ivm_entry_even_c9_2",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=2, even frame); F10",
         false, t22_entry_even_2},
        {"t22d_type0_ivm_entry_even_c9_3",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=3, even frame overflow); F10",
         false, t22_entry_even_3},
        {"t22e_type0_ivm_entry_even_c9_4",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.222 (switch at C9=4, even frame); F10",
         false, t22_entry_even_4},
        {"t22f_type0_ivm_entry_even_c9_5",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.222 (switch at C9=5, even frame); F10",
         false, t22_entry_even_5},
        {"t22g_type0_ivm_entry_even_c9_6",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.223 top (switch at C9=6, even frame reset); F10",
         false, t22_entry_even_6},
        {"t22h_type0_ivm_entry_odd_c9_0",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=0, odd frame); F10",
         false, t22_entry_odd_0},
        {"t22i_type0_ivm_entry_odd_c9_1",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=1, odd frame); F10",
         false, t22_entry_odd_1},
        {"t22j_type0_ivm_entry_odd_c9_3",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.221 (switch at C9=3, odd frame overflow); F10",
         false, t22_entry_odd_3},
        {"t22k_type0_ivm_entry_odd_c9_6",
         "ACCC v1.10 section 19.8.1 pp.219-220 and table p.223 top (switch at C9=6, odd frame overflow); F10",
         false, t22_entry_odd_6},
        {"t22l_type0_ivm_exit_even_at_limit",
         "ACCC v1.10 section 19.8.1 p.220 and table p.223 bottom (exit at C9.VMA=R9); F10",
         false, t22_exit_even_at_limit},
        {"t22m_type0_ivm_exit_odd_at_r9_plus_1",
         "ACCC v1.10 section 19.8.1 p.220 worked example and table p.224 (exit at C9.VMA=R9+1); F10",
         false, t22_exit_odd_at_r9_plus_1},
        {"t22n_type0_ivm_exit_odd_below_limit",
         "ACCC v1.10 section 19.8.1 p.220 and table p.224 (exit at C9.VMA<R9, odd frame); F10",
         false, t22_exit_odd_below_limit},
        {"t22o_type0_ivm_exit_even_below_limit",
         "ACCC v1.10 section 19.8.1 p.220 and table p.223 bottom (exit at C9.VMA<R9, even frame); F10",
         false, t22_exit_even_below_limit},
        {"t22p_type0_ivm_exit_even_c9_0",
         "ACCC v1.10 section 19.8.1 p.220 and table p.223 top (exit at C9.VMA=0, even frame); F10/N-8",
         false, t22_exit_even_c9_0},
        {"t22q_type0_ivm_exit_even_c9_1",
         "ACCC v1.10 section 19.8.1 p.220 and table p.223 (exit at C9.VMA=2, even frame); F10/N-8",
         false, t22_exit_even_c9_1},
        {"t22r_type0_ivm_exit_odd_c9_0",
         "ACCC v1.10 section 19.8.1 p.220 and table p.224 top (exit at C9.VMA=1, odd frame); F10/N-8",
         false, t22_exit_odd_c9_0},
        {"t22s_type0_ivm_exit_odd_c9_1",
         "ACCC v1.10 section 19.8.1 p.220 and table p.224 (exit at C9.VMA=3, odd frame); F10/N-8",
         false, t22_exit_odd_c9_1},
        {"t23a_type1_ivm_frame_boundary_parity_continuity",
         "ACCC v1.10 section 19.8.2 p.225 match branch at a frame boundary; F10/B-2",
         false, test_type1_ivm_frame_boundary_parity_continuity},
        {"t23b_type1_ivm_engages_from_snapshot_r8_3",
         "ACCC v1.10 section 19.8.2 p.225 with R8=3 snapshot-loaded; F10 ivm seeding",
         false, test_type1_ivm_engages_from_snapshot_r8_3},
        {"t23c_interlace_sync_leaves_ra_plain",
         "ACCC v1.10 section 19.3.2.1 p.199 (INTERLACE SYNC does not touch the raster address); F10/N-9",
         false, test_interlace_sync_leaves_ra_plain},
        {"t24a_type1_ivm_vsync_gap_r7_odd_c4",
         "ACCC v1.10 section 19.5.3 p.208 table (R9=8 even, R7=1 odd) with section 19.8.2 p.225 alternation",
         false, test_type1_ivm_vsync_gap_r7_odd_c4},
        {"t24b_type1_ivm_vsync_no_gap_r7_even_c4",
         "ACCC v1.10 section 19.5.3 p.208 table (R9=8 even, R7=2 even) with section 19.8.2 p.225 alternation",
         false, test_type1_ivm_vsync_no_gap_r7_even_c4},
        {"t24c_type1_ivm_mid_vsync_half_line_phase",
         "ACCC v1.10 section 19.5.3 p.208 prose (MID-VSYNC on the ParityFrame-even frame); p.207 Note",
         false, test_type1_ivm_mid_vsync_half_line_phase},
        {"t25a_type0_adjustment_segment_cycles_period_8",
         "ACCC v1.10 sections 11.2.1 p.81 and 20.2 p.241; D1 correction",
         false, test_type0_adjustment_segment_cycles_period_8},
        {"t25b_type0_adjustment_pointer_steps_and_scans",
         "ACCC v1.10 sections 11.2.1 p.81 and 11.2.2 pp.82-83; D1 correction",
         false, test_type0_adjustment_pointer_steps_and_scans},
        {"t25c_type0_adjustment_exit_reloads_frame_origin",
         "ACCC v1.10 sections 11.2.2 p.81 and 20.3.1 p.242; D1 correction",
         false, test_type0_adjustment_exit_reloads_frame_origin},
        {"t26a_type0_r1_zero_write_deadline",
         "ACCC v1.10 section 17.5.1 p.185 chronograms; D1 correction",
         false, test_t26_r1_zero_deadline_type0},
        {"t26b_type1_r1_zero_write_deadline",
         "ACCC v1.10 section 17.5.1 p.185 chronograms; D1 correction",
         false, test_t26_r1_zero_deadline_type1},
        // t27: F14 additional interlace line, type 0 (ACCC v1.10 section
        // 19.6.1 p.216).  Fixture-first XFAIL pins; the behavior commit
        // flips them to required passes.
        {"t27a_type0_addline_basic",
         "ACCC v1.10 sections 19.6.1 p.216, 19.5.2 p.205 and 19.3 p.199; F14",
         false, t27_type0_addline_basic},
        {"t27b_type0_addline_after_r5_lines",
         "ACCC v1.10 section 19.6.1 p.216 (after the R5 lines, one increment) and 11.2 p.84; F14",
         false, t27_type0_addline_after_r5_lines},
        {"t27c_type0_addline_r6_gt_r4_freeze_odd",
         "ACCC v1.10 sections 19.6.1 p.216 and 19.5.2 p.205 (frozen-odd persistence); F14",
         false, t27_type0_addline_freeze_odd},
        {"t27d_type0_addline_r6_gt_r4_freeze_even",
         "ACCC v1.10 sections 19.6.1 p.216 and 19.5.2 p.205 (frozen-even persistence); F14",
         false, t27_type0_addline_freeze_even},
        // t28: F14 additional interlace line, type 1 (ACCC v1.10 section
        // 19.6.2 p.216).  t28a is the fixture-first XFAIL pair member;
        // t28b is the gate control and is required from the start.
        {"t28a_type1_addline_basic",
         "ACCC v1.10 sections 19.6.2 p.216 and 11.2.4 p.84; F14",
         false, t28_type1_addline_basic},
        {"t28b_type1_addline_condition_false",
         "ACCC v1.10 section 19.6.2 p.216 (R9+1 multiple of R5 gate); F14",
         false, t28_type1_addline_condition_false},
        {"t28c_type1_addline_interlace_sync",
         "ACCC v1.10 section 19.6.2 p.216 (gate R8 in 1,3); F14/review",
         false, t28_type1_addline_interlace_sync},
        // t29: F15 type-0 odd-R9 IVM counting (ACCC v1.10 section 19.5.2
        // pp.205-206 and section 19.8.1 with the Q19-adjudicated gate).
        // Fixture-first XFAIL pins; the behavior commit flips them.
        {"t29a_type0_odd_r9_even_frame",
         "ACCC v1.10 section 19.5.2 p.206 worked example (even frame column); F15",
         false, t29_type0_odd_r9_even_frame},
        {"t29b_type0_odd_r9_odd_frame",
         "ACCC v1.10 section 19.5.2 p.206 worked example (odd frame column); F15/F14",
         false, t29_type0_odd_r9_odd_frame},
        {"t29c_type0_odd_r9_vsync_delay",
         "ACCC v1.10 sections 19.5.2 pp.205-206 (odd-C4 R7 VSYNC delay); F15",
         false, t29_type0_odd_r9_vsync_delay},
        {"t29d_type0_odd_r9_switch_line_overflow",
         "ACCC v1.10 section 19.8.1 p.219 (switch line R9+ParityFrame, overflow); F15/review",
         false, t29_type0_odd_r9_switch_line_overflow},
        {"t29e_type0_delay_arm_clears_on_type_switch",
         "Live CRTC_TYPE contract with the F15 delay state; review blocking 1",
         false, t29_type0_delay_arm_clears_on_type_switch},
    };

    unsigned passed = 0;
    unsigned xfailed = 0;
    unsigned xpassed = 0;
    unsigned failed = 0;

    for (const TestCase& test : tests) {
        const std::string trace_path = "obj_dir/" + test.name + ".vcd";
        std::string failure;
        bool divergence_failure = false;
        {
            TestBench bench(trace_path);
            try {
                test.run(bench);
            } catch (const KnownDivergenceFailure& error) {
                divergence_failure = true;
                failure = error.what();
            } catch (const TestFailure& error) {
                failure = error.what();
            } catch (const std::exception& error) {
                failure = std::string("unexpected exception: ") + error.what();
            }
        }

        if (failure.empty()) {
            std::error_code remove_error;
            std::filesystem::remove(trace_path, remove_error);
            if (test.known_divergence) {
                ++xpassed;
                std::cout << "XPASS " << test.name << " [" << test.source_rule
                          << "] -- known-divergence marker can be removed\n";
            } else {
                ++passed;
                std::cout << "PASS  " << test.name << " [" << test.source_rule << "]\n";
            }
        } else if (test.known_divergence && divergence_failure) {
            std::error_code remove_error;
            std::filesystem::remove(trace_path, remove_error);
            ++xfailed;
            std::cout << "XFAIL " << test.name << " [" << test.source_rule << "]\n"
                      << "      " << failure << '\n';
        } else {
            ++failed;
            std::cerr << "FAIL  " << test.name << " [" << test.source_rule << "]\n"
                      << "      " << failure << "\n"
                      << "      VCD retained at " << trace_path << '\n';
        }
    }

    std::cout << "\nSummary: " << passed << " passed, " << xfailed << " xfailed, "
              << xpassed << " xpassed, " << failed << " failed\n";
    return failed == 0 && xpassed == 0 ? 0 : 1;
}
