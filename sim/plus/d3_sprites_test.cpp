// D3 production-cadence first-visible-row regression.
//
// The diagnostic contract is deliberately bounded: sprite attributes are
// static before the Y=10 window, the compare line advances sequentially, and
// the normal 64-character line provides uncontended service through the
// registered one-byte row-fetch port.  Under those conditions the first
// visible source row must be ready at the Y seam for every enabled sprite.
// Discontinuities, reset-at-Y, access invalidation, and short-line bandwidth
// are outside this contract; the existing asic_sprites vectors cover their
// separate semantics.
// Arnold V issue 1.5 section 2.1 specifies the 16x16 image, transparent
// colour zero and fixed sprite priority: with sprites 0..14 transparent,
// opaque sprite 15 contributes 16 pixels per unmagnified source row.

#include <verilated.h>

#include "Vasic_sprites.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& message) {
    throw TestFailure(message);
}

class Bench {
public:
    Vasic_sprites dut;
    uint8_t ram[16][256]{};
    unsigned vline = 0;

    Bench() {
        dut.CLOCK = 0;
        dut.PIXEN = 0;
        dut.CLKEN = 0;
        dut.HWRAP = 0;
        dut.nRESET = 0;
        dut.LINE = 0;
        dut.ROW = 0;
        for (unsigned i = 0; i < 5; ++i) {
            dut.SPR_X[i] = 0;
            dut.SPR_Y[i] = 0;
        }
        dut.SPR_MAG = 0;
        for (unsigned i = 0; i < 6; ++i)
            dut.SPR_PAL[i] = 0;
        dut.ACC_EN = 0;
        dut.ACC_IDX = 0;
        dut.spr_wr_en = 0;
        dut.spr_wr_addr = 0;
        dut.spr_wr_data = 0;
        dut.FQ_ACK = 0;
        dut.FQ_DATA = 0;
        dut.eval();
        reset_pulse();
    }

    ~Bench() { dut.final(); }
    Bench(const Bench&) = delete;
    Bench& operator=(const Bench&) = delete;

    static void put_field(uint64_t& word, unsigned lsb, unsigned width,
                          uint64_t value) {
        for (unsigned bit = 0; bit < width; ++bit) {
            const uint64_t mask = 1ULL << (lsb + bit);
            if ((value >> bit) & 1ULL)
                word |= mask;
            else
                word &= ~mask;
        }
    }

    template <typename W>
    static void put_field(W& word, unsigned lsb, unsigned width,
                          uint64_t value) {
        for (unsigned bit = 0; bit < width; ++bit) {
            const unsigned absolute = lsb + bit;
            const uint32_t mask = 1u << (absolute & 31);
            if ((value >> bit) & 1ULL)
                word[absolute >> 5] |= mask;
            else
                word[absolute >> 5] &= ~mask;
        }
    }

    void set_x(unsigned sprite, unsigned value) {
        put_field(dut.SPR_X, sprite * 10, 10, value & 0x3FF);
    }

    void set_y(unsigned sprite, unsigned value) {
        put_field(dut.SPR_Y, sprite * 9, 9, value & 0x1FF);
    }

    void set_mag(unsigned sprite, unsigned value) {
        put_field(dut.SPR_MAG, sprite * 4, 4, value & 0xF);
    }

    void set_colour(unsigned colour, unsigned value) {
        put_field(dut.SPR_PAL, (colour - 1) * 12, 12,
                  static_cast<uint64_t>(value & 0xFFF));
    }

    void reset_pulse() {
        dut.nRESET = 0;
        for (unsigned i = 0; i < 32; ++i)
            tick(false, false, false);
        dut.nRESET = 1;
        for (unsigned i = 0; i < 32; ++i)
            tick(false, false, false);
    }

    // One master-clock edge. A request observed before this edge is returned
    // on the following edge, matching the registered asic_regs service.
    void tick(bool pixen, bool clken, bool hwrap) {
        dut.FQ_ACK = ack_q_;
        dut.FQ_DATA = data_q_;
        dut.CLKEN = clken ? 1 : 0;
        dut.HWRAP = hwrap ? 1 : 0;
        dut.PIXEN = pixen ? 1 : 0;
        dut.LINE = (vline >> 3) & 0x7F;
        dut.ROW = vline & 0x1F;

        dut.CLOCK = 0;
        dut.eval();
        const bool request = dut.FQ_REQ != 0;
        const unsigned address = static_cast<unsigned>(dut.FQ_ADDR);
        dut.CLOCK = 1;
        dut.eval();

        ack_q_ = request;
        data_q_ = ram_byte(address);
    }

    struct Sample {
        bool enabled;
        unsigned index;
        unsigned window;
        unsigned rgb;
    };

    Sample run_dot(bool char_end, bool line_end) {
        tick(false, false, false);
        tick(false, false, false);
        tick(false, false, false);
        tick(true, char_end, line_end);
        return {
            dut.SPR_EN != 0,
            static_cast<unsigned>(dut.SPR_IDX),
            static_cast<unsigned>(dut.SPR_WIN),
            static_cast<unsigned>(dut.SPR_RGB),
        };
    }

    void run_line_idle() {
        for (unsigned character = 0; character < 64; ++character) {
            for (unsigned dot = 0; dot < 16; ++dot) {
                const bool char_end = dot == 15;
                const bool line_end = char_end && character == 63;
                run_dot(char_end, line_end);
            }
        }
        ++vline;
    }

    std::vector<Sample> run_line_sample() {
        std::vector<Sample> samples;
        samples.reserve(64 * 16);
        for (unsigned character = 0; character < 64; ++character) {
            for (unsigned dot = 0; dot < 16; ++dot) {
                const bool char_end = dot == 15;
                const bool line_end = char_end && character == 63;
                samples.push_back(run_dot(char_end, line_end));
            }
        }
        ++vline;
        return samples;
    }

    uint8_t ram_byte(unsigned address) const {
        const unsigned sprite = (address >> 7) & 0xF;
        const unsigned row = (address >> 3) & 0xF;
        const unsigned byte = address & 7;
        return static_cast<uint8_t>((ram[sprite][row * 16 + byte * 2 + 1] << 4) |
                                    ram[sprite][row * 16 + byte * 2]);
    }

private:
    bool ack_q_ = false;
    uint8_t data_q_ = 0;
};

void check_target_row(unsigned x) {
    Bench bench;
    constexpr unsigned target = 15;

    for (unsigned sprite = 0; sprite < 16; ++sprite) {
        bench.set_x(sprite, x);
        bench.set_y(sprite, 10);
        bench.set_mag(sprite, 0x5); // x1/y1
    }
    bench.set_colour(2, 0x222);
    bench.set_colour(3, 0x333);
    for (unsigned row = 0; row < 16; ++row)
        for (unsigned pixel = 0; pixel < 16; ++pixel)
            bench.ram[target][row * 16 + pixel] = row == 0 ? 2 : 3;

    // Attributes are stable for the complete line before the first visible
    // row. This is the required production contract, not a reset-at-Y case.
    while (bench.vline < 10)
        bench.run_line_idle();

    const auto first = bench.run_line_sample();
    const auto second = bench.run_line_sample();
    auto counts = [](const std::vector<Bench::Sample>& samples,
                     unsigned expected_rgb) {
        unsigned windows = 0;
        unsigned target_pixels = 0;
        unsigned wrong_rgb = 0;
        for (const auto& sample : samples) {
            if (sample.window & (1u << 15))
                ++windows;
            if (sample.enabled && sample.index == 15) {
                ++target_pixels;
                if (sample.rgb != expected_rgb)
                    ++wrong_rgb;
            }
        }
        return std::tuple<unsigned, unsigned, unsigned>(
            windows, target_pixels, wrong_rgb);
    };

    const auto [first_windows, first_pixels, first_wrong_rgb] =
        counts(first, 0x222);
    const auto [second_windows, second_pixels, second_wrong_rgb] =
        counts(second, 0x333);
    if (first_windows != 16 || first_pixels != 16 ||
        second_windows != 16 || second_pixels != 16 ||
        first_wrong_rgb != 0 || second_wrong_rgb != 0) {
        fail("D3 X=" + std::to_string(x) +
             ": expected target sprite 15 to render 16 pixels on both "
             "the first and second visible rows (first window=" +
             std::to_string(first_windows) + ", pixels=" +
             std::to_string(first_pixels) + ", wrong-rgb=" +
             std::to_string(first_wrong_rgb) + ", second window=" +
             std::to_string(second_windows) + ", pixels=" +
             std::to_string(second_pixels) + ", wrong-rgb=" +
             std::to_string(second_wrong_rgb) + ")");
    }
}

} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        for (const unsigned x : {0u, 16u, 32u, 64u, 256u})
            check_target_row(x);
        std::printf("PASS D3 first-visible-row prefetch at X=0,16,32,64,256\n");
        return 0;
    }
    catch (const TestFailure& error) {
        std::fprintf(stderr, "FAIL D3 first-visible-row prefetch: %s\n",
                     error.what());
        return 1;
    }
}
