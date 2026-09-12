// Experimental SSM sample recorder vectors (backlog B4, phase 2).
//
// Every expectation here is derived from docs/ssm-capture-abi.md and the
// pinned contracts in docs/csl-ssm-implementation-plan.md, not read back out
// of the simulator. The properties worth the vectors are the ones a careful
// reading of the RTL cannot settle on its own:
//
//   * publication *order* -- a descriptor invalidated before its payload is
//     reused, and sealed only after the last payload beat of its window;
//   * that a lost packed word leaves a hole at its own address instead of
//     sliding later samples earlier in time;
//   * that a cut landing exactly on a window boundary pins the window holding
//     the last included sample, not the empty new one;
//   * that pool exhaustion forces an expiry and is counted, and never stalls
//     the sample stream.
//
// The harness logs every accepted DDR3 beat, so ordering is checked directly
// rather than inferred from settled bytes.

#include <verilated.h>
#include "Vssm_recorder_top.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& message) {
    throw TestFailure(message);
}

template <typename A, typename B>
void expect_eq(const A& got, const B& want, const std::string& what) {
    if (static_cast<uint64_t>(got) != static_cast<uint64_t>(want)) {
        std::ostringstream os;
        os << what << ": got 0x" << std::hex << static_cast<uint64_t>(got)
           << ", expected 0x" << static_cast<uint64_t>(want);
        fail(os.str());
    }
}

void expect_true(bool ok, const std::string& what) {
    if (!ok) fail(what);
}

// Region map, from docs/ssm-capture-abi.md with the harness's parameters.
constexpr uint32_t kCapWord = 0x31000000u >> 3;
constexpr uint32_t kHdr     = kCapWord;
constexpr uint32_t kDesc    = kCapWord + 8;      // +0x40, 4 words per window
constexpr uint32_t kRec     = kCapWord + 64;     // +0x200, 8 words per record
constexpr uint32_t kPay     = kCapWord + 512;    // PAYLOAD_OFF 0x1000
constexpr uint32_t kMagic   = 0x53534D43u;       // "SSMC"
constexpr unsigned kWindows = 4;
constexpr unsigned kWinSamples = 32;
constexpr unsigned kWinWords = kWinSamples / 2;

constexpr uint64_t kStateInvalid = 0;
constexpr uint64_t kStateSealed = 2;

// Sample flag bits, packed above the 24 colour bits.
constexpr unsigned kHbl = 1, kVbl = 2, kHs = 4, kVs = 8, kFld = 16;

uint32_t sample_word(uint8_t r, uint8_t g, uint8_t b, unsigned flags) {
    return uint32_t(r) | (uint32_t(g) << 8) | (uint32_t(b) << 16)
         | (uint32_t(flags) << 24);
}

uint32_t desc_word(unsigned win, unsigned k) { return kDesc + win * 4 + k; }
uint32_t rec_word(unsigned slot, unsigned k) { return kRec + slot * 8 + k; }
uint32_t pay_word(unsigned win, unsigned idx) { return kPay + win * kWinWords + idx; }

struct Beat {
    uint32_t addr;
    uint64_t data;
};

class Harness {
public:
    explicit Harness(bool start_enabled = true)
        : dut_(std::make_unique<Vssm_recorder_top>()) {
        dut_->clk = 0;
        dut_->reset = 1;
        dut_->enable = start_enabled;
        dut_->smp_ce = 0;
        dut_->smp_r = 0; dut_->smp_g = 0; dut_->smp_b = 0;
        dut_->smp_hbl = 0; dut_->smp_vbl = 0;
        dut_->smp_hs = 0; dut_->smp_vs = 0; dut_->smp_field = 0;
        dut_->applied_config = 0;
        dut_->cap_stb = 0;
        dut_->cap_rec_a = 0; dut_->cap_rec_b = 0; dut_->cap_cut = 0;
        dut_->ddr_stall = 0;
        dut_->eval();
        tick(8);
        dut_->reset = 0;
    }

    void tick(unsigned n = 1) {
        for (unsigned i = 0; i < n; ++i) {
            // A beat is accepted on a cycle where the request is asserted and
            // the slave is not stalling, which is the state before this edge.
            if (dut_->ddr_we_o && !dut_->ddr_busy_o) {
                beats.push_back({uint32_t(dut_->ddr_addr_o), uint64_t(dut_->ddr_din_o)});
            }
            dut_->clk = 1;
            dut_->eval();
            dut_->clk = 0;
            dut_->eval();
        }
    }

    Vssm_recorder_top* operator->() { return dut_.get(); }

    void wait_ready(unsigned budget = 4000) {
        for (unsigned i = 0; i < budget; ++i) {
            tick();
            if (dut_->ready) {
                tick(32);   // let window 0 open
                return;
            }
        }
        fail("the recorder never became ready");
    }

    // One native dot: the enable is one clock wide and dots are four apart,
    // which is the ce_16 cadence on the 64 MHz core clock.
    void dot(uint8_t r, uint8_t g, uint8_t b, unsigned flags = 0, unsigned gap = 3) {
        dut_->smp_r = r; dut_->smp_g = g; dut_->smp_b = b;
        dut_->smp_hbl = (flags & kHbl) ? 1 : 0;
        dut_->smp_vbl = (flags & kVbl) ? 1 : 0;
        dut_->smp_hs = (flags & kHs) ? 1 : 0;
        dut_->smp_vs = (flags & kVs) ? 1 : 0;
        dut_->smp_field = (flags & kFld) ? 1 : 0;
        dut_->smp_ce = 1;
        tick(1);
        dut_->smp_ce = 0;
        tick(gap);
    }

    void dots(unsigned count, uint8_t r, uint8_t g, uint8_t b, unsigned flags = 0) {
        for (unsigned i = 0; i < count; ++i) dot(r, g, b, flags);
    }

    void marker(uint16_t code, uint64_t cut) {
        dut_->cap_rec_a = code;
        dut_->cap_rec_b = 0;
        dut_->cap_cut = cut;
        dut_->cap_stb = 1;
        tick(1);
        dut_->cap_stb = 0;
        tick(1);
    }

    uint64_t cut_now() const { return dut_->sample_count; }

    // Settled memory, replayed from the beats in order.
    std::map<uint32_t, uint64_t> memory() const {
        std::map<uint32_t, uint64_t> out;
        for (const Beat& b : beats) out[b.addr] = b.data;
        return out;
    }

    int first_beat(uint32_t addr) const {
        for (size_t i = 0; i < beats.size(); ++i)
            if (beats[i].addr == addr) return int(i);
        return -1;
    }

    int last_beat_where(uint32_t addr, uint64_t data) const {
        int found = -1;
        for (size_t i = 0; i < beats.size(); ++i)
            if (beats[i].addr == addr && beats[i].data == data) found = int(i);
        return found;
    }

    int last_payload_beat(unsigned win) const {
        int found = -1;
        for (size_t i = 0; i < beats.size(); ++i) {
            const uint32_t a = beats[i].addr;
            if (a >= pay_word(win, 0) && a < pay_word(win, kWinWords)) found = int(i);
        }
        return found;
    }

    std::vector<Beat> beats;

private:
    std::unique_ptr<Vssm_recorder_top> dut_;
};

uint64_t need(const std::map<uint32_t, uint64_t>& mem, uint32_t addr,
              const std::string& what) {
    auto it = mem.find(addr);
    if (it == mem.end()) fail(what + ": word 0x" + std::to_string(addr) + " was never written");
    return it->second;
}

// --- initialisation ---------------------------------------------------------

void test_every_descriptor_is_invalid_before_the_magic_appears() {
    Harness h;
    h.wait_ready();
    const int magic_at = h.last_beat_where(kHdr, (uint64_t(1) << 48) | (uint64_t(1) << 32) | kMagic);
    expect_true(magic_at >= 0, "the header magic was never written");
    expect_eq(uint32_t(h.beats[magic_at].data), kMagic, "header magic value");
    expect_eq((h.beats[magic_at].data >> 48) & 0xFFFF, 1, "ready flag in the header");
    expect_true(h.beats.size() > 0, "no beats recorded");
    expect_eq(h.beats[0].addr, kHdr, "first beat must target header word 0");
    expect_eq(h.beats[0].data, 0, "first beat must clear header word 0");
    for (unsigned w = 0; w < kWindows; ++w) {
        const int at = h.first_beat(desc_word(w, 0));
        expect_true(at >= 0 && at < magic_at,
                    "descriptor " + std::to_string(w) +
                        " was not invalidated before the magic was published");
        // Only the very first write of a descriptor is the init one.
        expect_eq(h.beats[at].data >> 32, kStateInvalid,
                  "initial descriptor state for window " + std::to_string(w));
    }
}

void test_header_describes_the_configured_geometry() {
    Harness h;
    h.wait_ready();
    const auto mem = h.memory();
    const uint64_t w1 = need(mem, kHdr + 1, "header word 1");
    expect_eq(uint32_t(w1), kWindows, "window count");
    expect_eq(w1 >> 32, kWinSamples, "samples per window");
    const uint64_t w4 = need(mem, kHdr + 4, "header word 4");
    expect_eq(w4, 0x1000, "payload offset");
}

// --- sample placement --------------------------------------------------------

void test_samples_are_packed_in_time_order_at_logical_addresses() {
    Harness h;
    h.wait_ready();
    // Distinct colours and distinct flag patterns, so a swapped half or a
    // normalised sync bit is visible.
    const unsigned flags[8] = {0, kHs, kHbl, kVs | kFld, kVbl, kHs | kHbl, 0, kFld};
    for (unsigned i = 0; i < 8; ++i) h.dot(uint8_t(i + 1), uint8_t(0x10 + i), 0x20, flags[i]);
    h.tick(200);
    const auto mem = h.memory();
    for (unsigned word = 0; word < 4; ++word) {
        const uint64_t got = need(mem, pay_word(0, word), "payload word");
        const uint64_t lo = sample_word(uint8_t(2 * word + 1), uint8_t(0x10 + 2 * word),
                                        0x20, flags[2 * word]);
        const uint64_t hi = sample_word(uint8_t(2 * word + 2), uint8_t(0x11 + 2 * word),
                                        0x20, flags[2 * word + 1]);
        // The earlier sample occupies the low half.
        expect_eq(got, lo | (hi << 32), "packed word " + std::to_string(word));
    }
}

// --- rotation and publication order -----------------------------------------

void test_seal_follows_the_last_payload_beat_of_its_window() {
    Harness h;
    h.wait_ready();
    h.dots(kWinSamples, 0x40, 0x50, 0x60);
    h.tick(400);
    const int last_pay = h.last_payload_beat(0);
    expect_true(last_pay >= 0, "window 0 wrote no payload");
    const int seal = h.last_beat_where(desc_word(0, 0), (kStateSealed << 32) | 1u);
    expect_true(seal >= 0, "window 0 was never sealed with generation 1");
    expect_true(seal > last_pay, "the seal was published before the last payload beat");

    const auto mem = h.memory();
    expect_eq(need(mem, desc_word(0, 1), "window 0 first sample"), 0, "window 0 first sample");
    expect_eq(need(mem, desc_word(0, 2), "window 0 sample count"), kWinSamples,
              "window 0 sample count");
    expect_eq(uint32_t(need(mem, desc_word(0, 3), "window 0 flags")), 0,
              "window 0 reported a loss it did not have");
}

void test_reuse_invalidates_the_descriptor_before_rewriting_its_payload() {
    Harness h;
    h.wait_ready();
    // Five windows' worth through a four-window pool: window 0 is reused.
    h.dots(kWinSamples * 5, 0x11, 0x22, 0x33);
    h.tick(2000);
    // Window 0's second life carries generation 5 (1, 2, 3, 4, then 5).
    const int reopen = h.last_beat_where(desc_word(0, 0), (kStateInvalid << 32) | 5u);
    expect_true(reopen >= 0, "window 0 was never reopened with a new generation");
    int first_pay_after = -1;
    for (size_t i = size_t(reopen); i < h.beats.size(); ++i) {
        const uint32_t a = h.beats[i].addr;
        if (a >= pay_word(0, 0) && a < pay_word(0, kWinWords)) {
            first_pay_after = int(i);
            break;
        }
    }
    expect_true(first_pay_after > reopen,
                "window 0's payload was rewritten before its descriptor was invalidated");
    // And nothing wrote that payload between the old seal and the reopen.
    const int old_seal = h.last_beat_where(desc_word(0, 0), (kStateSealed << 32) | 1u);
    expect_true(old_seal >= 0, "window 0's first seal is missing");
    for (int i = old_seal; i < reopen; ++i) {
        const uint32_t a = h.beats[size_t(i)].addr;
        expect_true(!(a >= pay_word(0, 0) && a < pay_word(0, kWinWords)),
                    "a sealed window's payload was written before it was invalidated");
    }
}

// --- captures ----------------------------------------------------------------

void test_a_capture_records_its_cut_and_window_set() {
    Harness h;
    h.wait_ready();
    h.dots(10, 0x01, 0x02, 0x03);
    const uint64_t cut = h.cut_now();
    expect_eq(cut, 10, "ten dots advanced the logical count by ten");
    h.marker(0x0101, cut);
    h.tick(400);
    const auto mem = h.memory();
    expect_eq(need(mem, rec_word(0, 2), "capture cut"), cut, "capture cut");
    const uint64_t w3 = need(mem, rec_word(0, 3), "capture current window");
    expect_eq(uint32_t(w3), 0, "capture current window index");
    expect_eq(w3 >> 32, 1, "capture current window generation");
    const uint64_t status = uint32_t(need(mem, rec_word(0, 6), "capture status"));
    expect_eq(status & 1, 1, "capture usable bit");
    expect_eq((status >> 3) & 1, 1, "no prehistory yet, so the record says incomplete");
    expect_eq((status >> 5) & 1, 0, "this cut is not on a window boundary");
    expect_eq(h->captures_published, 1, "one capture published");
    expect_eq(uint32_t(need(mem, kHdr + 6, "header counters")), 1,
              "header publishes the capture count");
}

void test_two_markers_in_one_window_keep_their_own_states() {
    Harness h;
    h.wait_ready();
    h.dots(8, 0xE0, 0x00, 0x00);            // red
    const uint64_t cut_a = h.cut_now();
    h.marker(0x0201, cut_a);
    h.dots(8, 0x00, 0x00, 0xE0);            // blue
    const uint64_t cut_b = h.cut_now();
    h.marker(0x0202, cut_b);
    h.tick(600);
    const auto mem = h.memory();
    expect_eq(need(mem, rec_word(0, 2), "first cut"), 8, "first cut");
    expect_eq(need(mem, rec_word(1, 2), "second cut"), 16, "second cut");
    expect_eq(uint32_t(need(mem, rec_word(0, 3), "first window")),
              uint32_t(need(mem, rec_word(1, 3), "second window")),
              "both markers share one window");
    // The window still holds both states, so a host cut can reproduce each.
    expect_eq(uint32_t(need(mem, pay_word(0, 0), "red pair")),
              sample_word(0xE0, 0, 0, 0), "the pre-marker red survived the later blue");
    expect_eq(uint32_t(need(mem, pay_word(0, 4), "blue pair")),
              sample_word(0, 0, 0xE0, 0), "the post-marker blue is present too");
}

void test_a_cut_on_a_window_boundary_names_the_window_that_holds_it() {
    Harness h;
    h.wait_ready();
    h.dots(kWinSamples, 0x70, 0x70, 0x70);
    h.tick(200);                       // let the rotation finish
    const uint64_t cut = h.cut_now();
    expect_eq(cut, kWinSamples, "the cut sits exactly on the boundary");
    h.marker(0x0303, cut);
    h.tick(400);
    const auto mem = h.memory();
    const uint64_t w3 = need(mem, rec_word(0, 3), "boundary capture window");
    expect_eq(uint32_t(w3), 0, "the boundary cut names window 0, not the empty window 1");
    expect_eq(w3 >> 32, 1, "and window 0's generation");
    const uint64_t status = uint32_t(need(mem, rec_word(0, 6), "boundary status"));
    expect_eq((status >> 5) & 1, 1, "the boundary case is flagged");
}

void test_a_cross_window_marker_pins_the_preceding_window() {
    Harness h;
    h.wait_ready();
    h.dots(kWinSamples, 0xF0, 0x00, 0x00);   // window 0, red
    h.dots(8, 0x00, 0xF0, 0x00);             // window 1, green
    const uint64_t cut = h.cut_now();
    h.marker(0x0404, cut);
    h.tick(600);
    const auto mem = h.memory();
    const uint64_t w3 = need(mem, rec_word(0, 3), "current window");
    const uint64_t w4 = need(mem, rec_word(0, 4), "previous window 0");
    expect_eq(uint32_t(w3), 1, "the cut is in window 1");
    expect_eq(uint32_t(w4), 0, "its prehistory names window 0");
    expect_eq(w4 >> 32, 1, "window 0's generation travels with the capture");
    const uint64_t status = uint32_t(need(mem, rec_word(0, 6), "status"));
    expect_eq((status >> 1) & 1, 1, "previous window 0 is present");
    expect_eq((status >> 2) & 1, 0, "there is no second preceding window yet");
}

void test_a_third_capture_with_a_stalled_writer_is_reported_not_dropped_silently() {
    Harness h;
    h->ddr_stall = 15;
    h.wait_ready(20000);
    h.dots(4, 1, 2, 3, 0);
    const uint64_t cut = h.cut_now();
    h.marker(0x0501, cut);
    h.marker(0x0502, cut);
    h.marker(0x0503, cut);
    h.tick(200);
    expect_eq(h->capture_alloc, 2, "two captures fit the queue");
    expect_eq(h->capture_dropped, 1, "the third is counted as dropped");
}

// --- loss --------------------------------------------------------------------

void test_queue_overflow_leaves_holes_rather_than_compacting_time() {
    Harness h;
    h->ddr_stall = 15;
    h.wait_ready(20000);
    // Dots keep arriving at native cadence while the port is held off, so the
    // eight-entry queue cannot keep up.
    for (unsigned i = 0; i < kWinSamples; ++i) {
        h.dot(uint8_t(i + 1), uint8_t(0x80 + i), 0, 0);
    }
    h.tick(6000);
    expect_true(h->image_loss_count > 0, "the stalled port did not force a loss");
    const auto mem = h.memory();
    // Whatever survived must sit at the address its logical index demands.
    unsigned survived = 0;
    for (unsigned word = 0; word < kWinWords; ++word) {
        auto it = mem.find(pay_word(0, word));
        if (it == mem.end()) continue;
        ++survived;
        const uint64_t lo = sample_word(uint8_t(2 * word + 1), uint8_t(0x80 + 2 * word), 0, 0);
        const uint64_t hi = sample_word(uint8_t(2 * word + 2), uint8_t(0x81 + 2 * word), 0, 0);
        expect_eq(it->second, lo | (hi << 32),
                  "payload word " + std::to_string(word) + " holds another word's samples");
    }
    expect_true(survived > 0, "nothing at all reached DDR3");
    expect_true(survived < kWinWords, "no word was actually lost, so this proves nothing");
    const uint64_t flags = uint32_t(need(mem, desc_word(0, 3), "window flags"));
    expect_eq(flags & 1, 1, "the lossy window was not flagged");
}

// --- pool --------------------------------------------------------------------

void test_pool_exhaustion_forces_the_oldest_expiry_and_keeps_recording() {
    Harness h;
    h.wait_ready();
    // A marker in every window pins three windows at a time; with four in the
    // pool and a hold far longer than this vector, the pool runs out.
    for (unsigned w = 0; w < 6; ++w) {
        h.dots(kWinSamples / 2, uint8_t(0x10 * (w + 1)), 0, 0);
        h.marker(uint16_t(0x0600 + w), h.cut_now());
        h.dots(kWinSamples / 2, uint8_t(0x10 * (w + 1)), 0, 0);
        h.tick(200);
    }
    expect_true(h->forced_expiry_count > 0,
                "six pinned markers through a four-window pool forced no expiry");
    // The stream never stopped: every dot advanced the logical count.
    expect_eq(h->sample_count, 6 * kWinSamples, "the sample stream was stalled");
}

void test_an_expired_pin_is_reused_without_a_forced_expiry() {
    Harness h;
    h.wait_ready();
    h.dots(4, 0x22, 0x22, 0x22);
    h.marker(0x0701, h.cut_now());
    h.tick(4000);                       // HOLD_TICKS is 2000
    h.dots(kWinSamples * 5, 0x33, 0x33, 0x33);
    h.tick(2000);
    expect_eq(h->forced_expiry_count, 0,
              "a pin that had already expired still forced an expiry");
}

// --- lifecycle ---------------------------------------------------------------

void test_disable_holds_a_stalled_write_and_completes_it_once() {
    Harness h;
    h->ddr_stall = 15;
    h.tick(4);
    unsigned guard = 0;
    while (!h->ddr_we_o) {
        h.tick();
        if (++guard > 4000) fail("the recorder never asserted a request");
    }
    const uint32_t before = h->ddr_write_count;
    const uint64_t held_addr = h->ddr_addr_o;
    const uint64_t held_din = h->ddr_din_o;
    h->enable = 0;
    for (unsigned i = 0; i < 4; ++i) {
        h.tick();
        if (!h->ddr_we_o) break;
        expect_eq(h->ddr_addr_o, held_addr, "held address moved during the stall");
        expect_eq(h->ddr_din_o, held_din, "held data moved during the stall");
    }
    h.tick(400);
    expect_eq(h->ddr_we_o, 0, "the request was never released");
    expect_eq(h->ddr_write_count - before, 1, "the held write completed exactly once");
    expect_eq(h->ready, 0, "the recorder still claims to be ready while disabled");
}

void test_disabled_recorder_writes_nothing() {
    Harness h(/*start_enabled=*/false);
    h.tick(400);
    h.dots(16, 0x99, 0x99, 0x99);
    h.tick(400);
    expect_eq(h->ddr_write_count, 0, "a disabled recorder touched DDR3");
    expect_eq(h.beats.size(), 0, "a disabled recorder issued a beat");
    expect_eq(h->sample_count, 0, "a disabled recorder counted samples");
}

void test_reenable_starts_a_new_epoch_and_reinitialises() {
    Harness h;
    h.wait_ready();
    h.dots(8, 1, 2, 3);
    const uint32_t epoch_a = h->epoch;
    h->enable = 0;
    h.tick(64);
    h->enable = 1;
    h.wait_ready();
    expect_eq(h->epoch, epoch_a + 1, "the enable epoch did not advance");
    expect_eq(h->sample_count, 0, "the logical count did not restart");
    // The whole descriptor table is invalidated again before the magic.
    const int magic_at = h.last_beat_where(kHdr, (uint64_t(1) << 48)
                                                 | (uint64_t(1) << 32) | kMagic);
    expect_true(magic_at >= 0, "the second run published no magic");
    for (unsigned w = 0; w < kWindows; ++w) {
        int at = -1;
        for (int i = magic_at; i >= 0; --i) {
            if (h.beats[size_t(i)].addr == desc_word(w, 0)
                && (h.beats[size_t(i)].data >> 32) == kStateInvalid
                && uint32_t(h.beats[size_t(i)].data) == 0) {
                at = i;
                break;
            }
        }
        expect_true(at >= 0 && at < magic_at,
                    "window " + std::to_string(w) +
                        " was not reinitialised before the second run's magic");
    }
}

void test_one_clock_disable_during_stalled_write_restarts_lifecycle() {
    Harness h;
    h.wait_ready();
    h->ddr_stall = 15;
    h.dot(0x11, 0x22, 0x33);
    h.dot(0x44, 0x55, 0x66);
    // Wait until recorder has an active write request stalled
    unsigned guard = 0;
    while (!(h->ddr_we_o && h->ddr_busy_o)) {
        h.tick();
        if (++guard > 4000) fail("recorder write was never stalled");
    }
    // 1-clock disable pulse while stalled
    h->enable = 0;
    h.tick();
    h->enable = 1;
    expect_eq(h->ready, 0, "ready must drop immediately on short disable");
    // Release stall and let reinitialization complete
    h->ddr_stall = 0;
    h.tick(200);
    h.wait_ready();
    expect_eq(h->sample_count, 0, "sample count restarts at zero after short disable");
}

void test_capture_record_publication_sequence_order() {
    Harness h;
    h.wait_ready();
    h.dots(8, 0x11, 0x22, 0x33);
    h.marker(0x0101, 8);
    h.tick(200);
    // Find publication beats for slot 0: rec_word(0, 0..7) and header word 6
    int inval_7_idx = -1;
    int word_0_idx = -1;
    int word_6_idx = -1;
    int commit_7_idx = -1;
    int hdr_6_idx = -1;

    for (size_t i = 0; i < h.beats.size(); ++i) {
        const auto& b = h.beats[i];
        if (b.addr == rec_word(0, 7)) {
            if (b.data == 0xFFFF'FFFF'FFFF'FFFFull) {
                if (inval_7_idx < 0) inval_7_idx = int(i);
            } else {
                commit_7_idx = int(i);
            }
        } else if (b.addr == rec_word(0, 0)) {
            if (word_0_idx < 0) word_0_idx = int(i);
        } else if (b.addr == rec_word(0, 6)) {
            if (word_6_idx < 0) word_6_idx = int(i);
        } else if (b.addr == kHdr + 6) {
            hdr_6_idx = int(i);
        }
    }

    expect_true(inval_7_idx >= 0, "word 7 was not invalidated with 0xFFFFFFFF");
    expect_true(word_0_idx > inval_7_idx, "word 0 was written before word 7 invalidation");
    expect_true(word_6_idx > word_0_idx, "word 6 was written before word 0");
    expect_true(commit_7_idx > word_6_idx, "word 7 commit was not written after words 0-6");
    expect_true(hdr_6_idx > commit_7_idx, "header count was updated before word 7 commit");
}

void test_config_change_quiesces_coincident_and_continuing_input() {
    Harness h;
    h->applied_config = 0x81;
    h.wait_ready();
    h.dots(8, 0x11, 0x22, 0x33);
    h->ddr_stall = 15;
    h.marker(0x0303, h->sample_count); // old accepted work must still drain
    h.tick(2);
    const auto samples = h->sample_count;
    const auto captures = h->capture_alloc;
    const auto epoch = h->epoch;
    h->applied_config = 0x91; // supported mix change, native cadence unchanged
    h->smp_ce = 1;
    h->cap_stb = 1;
    h->cap_cut = samples;
    h.tick();
    expect_eq(h->sample_count, samples, "coincident new-config sample entered old history");
    expect_eq(h->capture_alloc, captures, "coincident new-config marker entered old epoch");
    expect_eq(h->ready, 0, "configuration transition must revoke readiness immediately");
    h->smp_ce = 0;
    h->cap_stb = 0;
    // Continue native-rate traffic while accepted old work is backpressured.
    for (unsigned i = 0; i < 5; ++i) {
        h.tick(3);
        h->smp_ce = 1; h->cap_stb = 1;
        h.tick();
        h->smp_ce = 0; h->cap_stb = 0;
    }
    expect_eq(h->sample_count, samples, "new samples replenished the draining queue");
    expect_eq(h->capture_alloc, captures, "new captures replenished the draining queue");
    h->ddr_stall = 0;
    h.tick(400);
    h.wait_ready();
    expect_eq(h->epoch, epoch + 1, "configuration drain did not start a fresh epoch");
    expect_eq(h->sample_count, 0, "new epoch did not start with empty history");
    expect_eq(h->captures_published, 0, "old capture survived as a new-epoch record");
}

void test_applied_config_change_across_windows_invalidates_history() {
    Harness h;
    h->applied_config = 1;
    h.wait_ready();
    // Fill window 0 and move into window 1
    h.dots(kWinSamples + 4, 0x11, 0x22, 0x33);
    h.tick(200);
    const uint32_t epoch_before = h->epoch;
    // Change applied_config
    h->applied_config = 2;
    h.tick(40);
    // Queue should drain in X_IDLE and trigger reinitialization with new epoch
    h.wait_ready();
    expect_eq(h->epoch, epoch_before + 1, "config change did not advance epoch");
}

void test_applied_config_change_during_stalled_publication_retains_event_config() {
    Harness h;
    h->applied_config = 0x01;
    h.wait_ready();
    h.dots(8, 0x11, 0x22, 0x33);
    h->ddr_stall = 15;
    h.marker(0x0202, 8);
    // While the capture is queued, change applied_config
    h->applied_config = 0x02;
    h.tick(10);
    h->ddr_stall = 0;
    h.tick(400);
    // Find the published record word 6: bits 31:24 must be 0x01 (event-time applied_config)
    int rec6_idx = -1;
    for (size_t i = 0; i < h.beats.size(); ++i) {
        if (h.beats[i].addr == rec_word(0, 6)) {
            rec6_idx = int(i);
        }
    }
    expect_true(rec6_idx >= 0, "record word 6 was not published");
    uint8_t cfg_latched = uint8_t((h.beats[rec6_idx].data >> 24) & 0xFF);
    expect_eq(cfg_latched, 0x01, "event-time applied_config was not retained");
}

struct Test {
    const char* name;
    void (*run)();
};

const Test kTests[] = {
    {"config change quiesces coincident and continuing input",
     test_config_change_quiesces_coincident_and_continuing_input},
    {"every descriptor is invalid before the magic appears",
     test_every_descriptor_is_invalid_before_the_magic_appears},
    {"header describes the configured geometry", test_header_describes_the_configured_geometry},
    {"samples are packed in time order at logical addresses",
     test_samples_are_packed_in_time_order_at_logical_addresses},
    {"seal follows the last payload beat of its window",
     test_seal_follows_the_last_payload_beat_of_its_window},
    {"reuse invalidates the descriptor before rewriting its payload",
     test_reuse_invalidates_the_descriptor_before_rewriting_its_payload},
    {"a capture records its cut and window set", test_a_capture_records_its_cut_and_window_set},
    {"two markers in one window keep their own states",
     test_two_markers_in_one_window_keep_their_own_states},
    {"a cut on a window boundary names the window that holds it",
     test_a_cut_on_a_window_boundary_names_the_window_that_holds_it},
    {"a cross-window marker pins the preceding window",
     test_a_cross_window_marker_pins_the_preceding_window},
    {"a third capture with a stalled writer is reported",
     test_a_third_capture_with_a_stalled_writer_is_reported_not_dropped_silently},
    {"queue overflow leaves holes rather than compacting time",
     test_queue_overflow_leaves_holes_rather_than_compacting_time},
    {"pool exhaustion forces the oldest expiry and keeps recording",
     test_pool_exhaustion_forces_the_oldest_expiry_and_keeps_recording},
    {"an expired pin is reused without a forced expiry",
     test_an_expired_pin_is_reused_without_a_forced_expiry},
    {"disable holds a stalled write and completes it once",
     test_disable_holds_a_stalled_write_and_completes_it_once},
    {"one-clock disable during stalled write restarts lifecycle",
     test_one_clock_disable_during_stalled_write_restarts_lifecycle},
    {"capture record publication sequence order",
     test_capture_record_publication_sequence_order},
    {"applied config change across windows invalidates history",
     test_applied_config_change_across_windows_invalidates_history},
    {"applied config change during stalled publication retains event config",
     test_applied_config_change_during_stalled_publication_retains_event_config},
    {"disabled recorder writes nothing", test_disabled_recorder_writes_nothing},
    {"re-enable starts a new epoch and reinitialises",
     test_reenable_starts_a_new_epoch_and_reinitialises},
};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    unsigned failures = 0;
    for (const Test& test : kTests) {
        try {
            test.run();
            std::cout << "PASS " << test.name << "\n";
        } catch (const std::exception& exc) {
            std::cout << "FAIL " << test.name << ": " << exc.what() << "\n";
            ++failures;
        }
    }
    std::cout << (failures ? "SSM recorder tests FAILED: " : "SSM recorder tests passed: ")
              << (sizeof(kTests) / sizeof(kTests[0]) - failures) << "/"
              << sizeof(kTests) / sizeof(kTests[0]) << "\n";
    return failures ? 1 : 0;
}
