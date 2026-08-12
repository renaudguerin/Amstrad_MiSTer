#include <verilated.h>

#include "Vplus_cartridge_memory.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kBase = 0x080000;
constexpr std::uint32_t kEnd = 0x0FFFFF;
#ifdef CLEAR_TEST_BYTES
constexpr unsigned kClearBytes = CLEAR_TEST_BYTES;
#else
constexpr unsigned kClearBytes = 32;
#endif

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

struct Transaction {
    bool write;
    std::uint8_t bank;
    std::uint32_t address;
    std::uint8_t data;
};

class TestBench {
public:
    TestBench() : memory_(kEnd - kBase + 1, 0xCC) {
        dut_.clk = 0;
        dut_.cold_reset = 0;
        dut_.detach = 0;
        dut_.load_begin = 0;
        dut_.load_commit = 0;
        dut_.load_abort = 0;
        dut_.load_valid = 0;
        dut_.load_page = 0;
        dut_.load_offset = 0;
        dut_.load_data = 0;
        dut_.cpu_valid = 0;
        dut_.cpu_page = 0;
        dut_.cpu_offset = 0;
        dut_.mem_ack = 0;
        dut_.mem_rdata = 0;
        dut_.eval();
    }

    ~TestBench() {
        dut_.final();
    }

    Vplus_cartridge_memory& dut() { return dut_; }
    const Vplus_cartridge_memory& dut() const { return dut_; }

    const std::vector<Transaction>& transactions() const {
        return transactions_;
    }

    std::uint8_t byte(std::uint32_t address) const {
        check_address(address);
        return memory_[address - kBase];
    }

    void set_byte(std::uint32_t address, std::uint8_t data) {
        check_address(address);
        memory_[address - kBase] = data;
    }

    // Advance one complete clock.  Backend acknowledgements are controlled
    // explicitly so every request can be held for deterministic wait states.
    void tick(bool acknowledge = false) {
        const bool request = dut_.mem_req != 0;
        if (acknowledge && !request) {
            throw TestFailure("test tried to acknowledge without mem_req");
        }

        if (request) {
            const Transaction current = current_transaction();
            if (!request_observed_) {
                pending_ = current;
                request_observed_ = true;
            } else {
                require(same_transaction(pending_, current),
                        "backend request fields changed before acknowledgement");
            }
            dut_.mem_rdata = byte(current.address);
        } else {
            require(!request_observed_,
                    "mem_req was withdrawn without acknowledgement");
            dut_.mem_rdata = 0;
        }

        dut_.mem_ack = acknowledge ? 1 : 0;
        dut_.clk = 1;
        dut_.eval();

        if (acknowledge) {
            transactions_.push_back(pending_);
            if (pending_.write) {
                set_byte(pending_.address, pending_.data);
            }
            request_observed_ = false;
        }

        dut_.clk = 0;
        dut_.eval();
    }

    void wait_for_request(unsigned limit = 20) {
        for (unsigned cycle = 0; cycle < limit && !dut_.mem_req; ++cycle) {
            tick();
        }
        require(dut_.mem_req != 0, "timed out waiting for mem_req");
    }

    Transaction pending_request() const {
        require(dut_.mem_req != 0, "expected an active backend request");
        return current_transaction();
    }

    void acknowledge_after(unsigned wait_cycles) {
        wait_for_request();
        for (unsigned cycle = 0; cycle < wait_cycles; ++cycle) {
            tick();
        }
        tick(true);
    }

private:
    static bool same_transaction(const Transaction& lhs,
                                 const Transaction& rhs) {
        return lhs.write == rhs.write && lhs.bank == rhs.bank &&
               lhs.address == rhs.address && lhs.data == rhs.data;
    }

    Transaction current_transaction() const {
        const Transaction transaction = {
            dut_.mem_write != 0,
            static_cast<std::uint8_t>(dut_.mem_bank),
            static_cast<std::uint32_t>(dut_.mem_addr),
            static_cast<std::uint8_t>(dut_.mem_wdata),
        };
        require(transaction.bank == 3, "backend request did not select bank 3");
        check_address(transaction.address);
        return transaction;
    }

    static void check_address(std::uint32_t address) {
        require(address >= kBase && address <= kEnd,
                "backend address escaped the cartridge reservation");
    }

    Vplus_cartridge_memory dut_;
    std::vector<std::uint8_t> memory_;
    std::vector<Transaction> transactions_;
    bool request_observed_ = false;
    Transaction pending_ = {};
};

void release_loader_request(TestBench& test) {
    test.dut().load_valid = 0;
    test.tick();
}

void begin_load_and_clear(TestBench& test, bool hold_begin = false) {
    const auto transaction_start = test.transactions().size();

    test.dut().load_begin = 1;
    test.tick();
    require(test.dut().busy == 1, "load_begin did not assert busy immediately");
    require(test.dut().image_valid == 0,
            "load_begin did not invalidate the old image immediately");
    require(test.dut().load_error == 0,
            "load_begin did not clear the session error");

    if (hold_begin) {
        test.tick();
        test.tick();
        require(test.dut().busy == 1, "held load_begin restarted or ended load");
    }
    test.dut().load_begin = 0;

    for (unsigned index = 0; index < kClearBytes; ++index) {
        test.wait_for_request();
        const auto request = test.pending_request();
        require(request.write, "clear issued a backend read");
        require(request.address == kBase + index,
                "clear address was not sequential across configured region");
        require(request.data == 0, "clear wrote a nonzero byte");
        test.acknowledge_after(index % 3);
    }

    test.tick();
    require(test.dut().mem_req == 0, "clear issued more than configured writes");
    require(test.transactions().size() == transaction_start + kClearBytes,
            "clear transaction count mismatch");
    require(test.dut().busy == 1,
            "loader stopped being busy when clear completed");
}

void write_loader_byte(TestBench& test, unsigned page, unsigned offset,
                       std::uint8_t data, unsigned wait_cycles = 2,
                       bool keep_held = false) {
    const auto transaction_start = test.transactions().size();
    test.dut().load_page = page;
    test.dut().load_offset = offset;
    test.dut().load_data = data;
    test.dut().load_valid = 1;
    test.tick();
    require(test.dut().load_ready == 0,
            "valid loader write completed before backend acknowledgement");
    test.wait_for_request();
    const auto request = test.pending_request();
    require(request.write, "loader byte issued a backend read");
    require(request.address == kBase + page * 0x4000U + offset,
            "loader byte used a non-canonical address");
    require(request.data == data, "loader write data mismatch");
    test.acknowledge_after(wait_cycles);
    require(test.dut().load_ready == 1,
            "loader did not pulse ready on acknowledged write");
    require(test.transactions().size() == transaction_start + 1,
            "loader write did not produce exactly one transaction");

    if (keep_held) {
        test.tick();
        require(test.dut().load_ready == 0,
                "held load_valid produced a second ready pulse");
        require(test.dut().mem_req == 0,
                "held load_valid produced a duplicate request");
        test.tick();
        require(test.transactions().size() == transaction_start + 1,
                "held load_valid produced duplicate traffic");
    }
    release_loader_request(test);
}

void commit_load(TestBench& test) {
    test.dut().load_commit = 1;
    test.tick();
    require(test.dut().image_valid == 0,
            "commit published without passing through pending state");
    test.dut().load_commit = 0;
    test.tick();
    require(test.dut().image_valid == 1, "commit did not publish the image");
    require(test.dut().busy == 0, "commit did not clear busy");
}

std::uint8_t cpu_read(TestBench& test, unsigned page, unsigned offset,
                      unsigned wait_cycles = 2, bool keep_held = false) {
    const auto transaction_start = test.transactions().size();
    test.dut().cpu_page = page;
    test.dut().cpu_offset = offset;
    test.dut().cpu_valid = 1;
    test.tick();

    if (!test.dut().image_valid) {
        require(test.dut().cpu_ready == 1,
                "EMPTY CPU read did not complete immediately");
        require(test.dut().cpu_data == 0xFF,
                "EMPTY CPU read did not return 0xff");
        require(test.transactions().size() == transaction_start,
                "EMPTY CPU read touched backend memory");
    } else {
        require(test.dut().cpu_ready == 0,
                "published CPU read completed before backend acknowledgement");
        test.wait_for_request();
        const auto request = test.pending_request();
        require(!request.write, "CPU read issued a backend write");
        require(request.address == kBase + page * 0x4000U + offset,
                "CPU read used a non-canonical address");
        test.acknowledge_after(wait_cycles);
        require(test.dut().cpu_ready == 1,
                "CPU read did not pulse ready on acknowledgement");
        require(test.transactions().size() == transaction_start + 1,
                "CPU read did not produce exactly one transaction");
    }

    const auto result = static_cast<std::uint8_t>(test.dut().cpu_data);
    if (keep_held) {
        test.tick();
        require(test.dut().cpu_ready == 0,
                "held cpu_valid produced a second ready pulse");
        require(test.dut().mem_req == 0,
                "held cpu_valid produced a duplicate request");
        test.tick();
        require(test.transactions().size() ==
                    transaction_start + (test.dut().image_valid ? 1 : 0),
                "held cpu_valid produced duplicate traffic");
    }
    test.dut().cpu_valid = 0;
    test.tick();
    return result;
}

void test_empty_and_complete_clear() {
    TestBench test;
    require(test.dut().image_valid == 0, "power-up image was not EMPTY");
    require(test.dut().busy == 0, "power-up service was busy");
    require(cpu_read(test, 0, 0, 0, true) == 0xFF,
            "EMPTY read value mismatch");

    for (unsigned index = 0; index <= kClearBytes; ++index) {
        test.set_byte(kBase + index, 0x80 + index);
    }
    begin_load_and_clear(test, true);

    for (unsigned index = 0; index < kClearBytes; ++index) {
        require(test.byte(kBase + index) == 0,
                "configured clear left a nonzero tail byte");
    }
    require(test.byte(kBase + kClearBytes) == 0x80 + kClearBytes,
            "clear exceeded the configured simulation region");
}

void test_boundaries_commit_and_reads() {
    TestBench test;
    begin_load_and_clear(test);

    write_loader_byte(test, 0, 0, 0x12, 3, true);
    write_loader_byte(test, 0, 0x3FFF, 0x34, 1);
    write_loader_byte(test, 31, 0, 0x56, 2);
    write_loader_byte(test, 31, 0x3FFF, 0x78, 4);
    require(test.byte(kBase) == 0x12, "page 0 first byte was not written");
    require(test.byte(kEnd) == 0x78,
            "page 31 final cartridge byte was not written");

    commit_load(test);
    require(cpu_read(test, 0, 0, 3, true) == 0x12,
            "CPU did not read page 0 payload");
    require(cpu_read(test, 0, 0x3FFF) == 0x34,
            "CPU did not read page 0 final offset");
    require(cpu_read(test, 31, 0) == 0x56,
            "CPU did not read page 31 payload");
    require(cpu_read(test, 31, 0x3FFF) == 0x78,
            "CPU did not read final cartridge address");
}

void test_invalid_addresses_fail_closed_and_recover() {
    TestBench test;
    begin_load_and_clear(test);
    const auto before_invalid = test.transactions().size();

    test.dut().load_page = 32;
    test.dut().load_offset = 0;
    test.dut().load_data = 0x91;
    test.dut().load_valid = 1;
    test.tick();
    require(test.dut().load_ready == 1,
            "invalid page was not consumed with a ready pulse");
    require(test.dut().load_error == 1,
            "invalid page did not set sticky session error");
    require(test.dut().mem_req == 0,
            "invalid page produced backend traffic after truncation");
    test.tick();
    require(test.dut().load_ready == 0 && test.dut().mem_req == 0,
            "held invalid request was consumed more than once");
    release_loader_request(test);

    test.dut().load_page = 0;
    test.dut().load_offset = 0x4000;
    test.dut().load_valid = 1;
    test.tick();
    require(test.dut().load_ready == 1,
            "invalid offset was not consumed with a ready pulse");
    require(test.dut().load_error == 1,
            "invalid offset lost sticky session error");
    require(test.dut().mem_req == 0,
            "invalid offset produced backend traffic after truncation");
    release_loader_request(test);
    require(test.transactions().size() == before_invalid,
            "rejected loader addresses touched backend memory");

    test.dut().load_commit = 1;
    test.tick();
    test.dut().load_commit = 0;
    test.tick();
    test.tick();
    require(test.dut().image_valid == 0 && test.dut().busy == 1,
            "errored load session was published by commit");
    require(test.dut().load_error == 1,
            "commit cleared the sticky load error");

    begin_load_and_clear(test);
    require(test.dut().load_error == 0,
            "new load_begin did not recover from sticky error");
    write_loader_byte(test, 2, 7, 0xA6);
    commit_load(test);
    require(cpu_read(test, 2, 7) == 0xA6,
            "recovered session did not publish normally");
}

void test_loader_priority_and_deferred_commit() {
    TestBench test;
    begin_load_and_clear(test);

    test.dut().cpu_page = 4;
    test.dut().cpu_offset = 9;
    test.dut().cpu_valid = 1;
    test.dut().load_page = 4;
    test.dut().load_offset = 9;
    test.dut().load_data = 0xD3;
    test.dut().load_valid = 1;
    test.tick();
    require(test.dut().mem_req == 1 && test.dut().mem_write == 1,
            "loader did not win priority over simultaneous CPU read");
    require(test.dut().cpu_ready == 0,
            "CPU completed while loader was active");

    test.tick();
    require(test.dut().cpu_ready == 0,
            "CPU was not backpressured during loader wait states");
    test.dut().load_commit = 1;
    test.tick();
    test.dut().load_commit = 0;
    require(test.dut().image_valid == 0 && test.dut().busy == 1,
            "commit published while loader write was outstanding");

    test.tick(true);
    require(test.dut().load_ready == 1,
            "outstanding loader write did not complete");
    require(test.dut().cpu_ready == 0,
            "CPU completed on the loader acknowledgement");
    test.dut().load_valid = 0;

    test.tick();
    require(test.dut().image_valid == 1 && test.dut().busy == 0,
            "deferred commit did not publish after outstanding work");
    require(test.dut().cpu_ready == 0,
            "CPU completed in the publication cycle");
    test.tick();
    require(test.dut().mem_req == 1 && test.dut().mem_write == 0,
            "held CPU request did not start after publication");
    test.acknowledge_after(2);
    require(test.dut().cpu_ready == 1 && test.dut().cpu_data == 0xD3,
            "post-publication CPU request returned wrong payload");
    test.dut().cpu_valid = 0;
    test.tick();
}

void test_control_dominance_and_one_shots() {
    {
        TestBench test;
        begin_load_and_clear(test);
        write_loader_byte(test, 1, 3, 0x41);
        commit_load(test);

        test.dut().cold_reset = 1;
        test.dut().load_abort = 1;
        test.dut().detach = 1;
        test.tick();
        require(test.dut().image_valid == 0 && test.dut().busy == 0,
                "detach did not dominate simultaneous abort and reset");

        // abort was consumed behind detach, so it cannot fire when detach is
        // released.  The still-asserted reset is harmless and preserves the
        // already-invalid image.
        test.dut().detach = 0;
        test.tick();
        require(test.dut().image_valid == 0 && test.dut().busy == 0,
                "hidden abort escaped after detach released");
        test.dut().load_abort = 0;
        test.dut().cold_reset = 0;
        test.tick();
    }

    {
        TestBench test;
        begin_load_and_clear(test);
        write_loader_byte(test, 2, 4, 0x52);
        commit_load(test);
        test.dut().cold_reset = 1;
        test.dut().load_abort = 1;
        test.tick();
        require(test.dut().image_valid == 0 && test.dut().busy == 0,
                "abort did not dominate reset and invalidate the image");
        test.dut().cold_reset = 0;
        test.tick();
        require(test.dut().image_valid == 0 && test.dut().busy == 0,
                "held abort fired again after simultaneous reset");
        test.dut().load_abort = 0;
        test.tick();
    }

    {
        TestBench test;
        test.dut().detach = 1;
        test.dut().load_begin = 1;
        test.tick();
        test.dut().detach = 0;
        test.tick();
        require(test.dut().busy == 0 && test.dut().mem_req == 0,
                "held begin fired after losing to detach");
        test.dut().load_begin = 0;
        test.tick();

        test.dut().load_abort = 1;
        test.dut().load_begin = 1;
        test.tick();
        test.dut().load_abort = 0;
        test.tick();
        require(test.dut().busy == 0 && test.dut().mem_req == 0,
                "held begin fired after losing to abort");
        test.dut().load_begin = 0;
        test.tick();
    }

    {
        TestBench test;
        test.dut().load_begin = 1;
        test.dut().load_commit = 1;
        test.tick();
        require(test.dut().busy == 1 && test.dut().image_valid == 0,
                "begin did not dominate simultaneous commit");
        test.dut().load_begin = 0;

        for (unsigned index = 0; index < kClearBytes; ++index) {
            test.acknowledge_after(index % 2);
        }
        test.tick();
        require(test.dut().busy == 1 && test.dut().image_valid == 0,
                "hidden held commit published after begin");
        test.dut().load_commit = 0;
        test.tick();
        commit_load(test);
    }
}

void test_cancelled_held_valids_require_release() {
    TestBench test;
    begin_load_and_clear(test);

    // A loader valid coincident with a replacing begin belongs to the old
    // context.  It must not become a write after the replacement clear.
    test.dut().load_page = 3;
    test.dut().load_offset = 5;
    test.dut().load_data = 0x75;
    test.dut().load_valid = 1;
    test.dut().load_begin = 1;
    test.tick();
    test.dut().load_begin = 0;
    for (unsigned index = 0; index < kClearBytes; ++index) {
        test.acknowledge_after(index % 2);
    }
    test.tick();
    test.tick();
    require(test.dut().mem_req == 0 && test.dut().load_ready == 0,
            "held loader valid reissued after begin cancellation");
    test.dut().load_valid = 0;
    test.tick();
    write_loader_byte(test, 3, 5, 0x75);
    commit_load(test);

    // The same rule applies to a CPU request cancelled by machine reset.
    test.dut().cpu_page = 3;
    test.dut().cpu_offset = 5;
    test.dut().cpu_valid = 1;
    test.tick();
    test.wait_for_request();
    test.dut().cold_reset = 1;
    test.tick();
    test.dut().cold_reset = 0;
    test.tick();
    test.tick(true);
    require(test.dut().cpu_ready == 0, "reset leaked cancelled CPU completion");
    test.tick();
    test.tick();
    require(test.dut().mem_req == 0 && test.dut().cpu_ready == 0,
            "held CPU valid reissued after reset cancellation");
    test.dut().cpu_valid = 0;
    test.tick();
    require(cpu_read(test, 3, 5) == 0x75,
            "CPU request did not rearm after explicit release");
}

void test_loader_ack_control_races() {
    // A replacing begin wins the ack logically, starts a fresh clear, and
    // consumes the still-held loader valid in the old context.
    {
        TestBench test;
        begin_load_and_clear(test);
        test.dut().load_page = 7;
        test.dut().load_offset = 13;
        test.dut().load_data = 0xB1;
        test.dut().load_valid = 1;
        test.tick();
        test.wait_for_request();
        test.dut().load_begin = 1;
        test.tick(true);
        require(test.dut().load_ready == 0 && test.dut().busy == 1,
                "begin/ack race leaked completion or stopped loading");
        test.dut().load_begin = 0;
        for (unsigned index = 0; index < kClearBytes; ++index) {
            test.acknowledge_after(index % 2);
        }
        test.tick();
        test.tick();
        require(test.dut().mem_req == 0,
                "held loader valid reissued after begin/ack race");
        test.dut().load_valid = 0;
        test.tick();
    }

    const auto run_invalidating_race = [](bool detach) {
        TestBench test;
        begin_load_and_clear(test);
        test.dut().load_page = 8;
        test.dut().load_offset = 14;
        test.dut().load_data = detach ? 0xD1 : 0xC1;
        test.dut().load_valid = 1;
        test.tick();
        test.wait_for_request();
        if (detach) {
            test.dut().detach = 1;
        } else {
            test.dut().cold_reset = 1;
        }
        test.tick(true);
        require(test.dut().load_ready == 0 && test.dut().image_valid == 0 &&
                    test.dut().busy == 0,
                detach ? "detach/ack race leaked loader completion"
                       : "reset/ack race leaked loader completion");
        test.dut().detach = 0;
        test.dut().cold_reset = 0;
        test.tick();
        test.tick();
        require(test.dut().mem_req == 0,
                "held loader valid reissued after invalidating ack race");
        test.dut().load_valid = 0;
        test.tick();
    };

    run_invalidating_race(true);
    run_invalidating_race(false);
}

void test_abort_detach_and_cold_reset() {
    TestBench test;
    begin_load_and_clear(test);
    write_loader_byte(test, 5, 11, 0x4B);
    commit_load(test);

    test.dut().cold_reset = 1;
    test.tick();
    test.dut().cold_reset = 0;
    test.tick();
    require(test.dut().image_valid == 1,
            "ordinary cold_reset discarded committed image_valid");
    require(test.byte(kBase + 5 * 0x4000U + 11) == 0x4B,
            "ordinary cold_reset changed SDRAM contents");
    require(cpu_read(test, 5, 11) == 0x4B,
            "image was unreadable after ordinary cold_reset");

    // A reset also cancels transient ownership without retracting the physical
    // request.  The backend may acknowledge it later, but no stale completion
    // may escape and the committed image remains published.
    test.dut().cpu_page = 5;
    test.dut().cpu_offset = 11;
    test.dut().cpu_valid = 1;
    test.tick();
    test.wait_for_request();
    test.dut().cold_reset = 1;
    test.tick();
    require(test.dut().mem_req == 1 && test.dut().image_valid == 1,
            "cold_reset retracted a request or invalidated committed image");
    test.dut().cold_reset = 0;
    test.tick();
    test.tick(true);
    require(test.dut().cpu_ready == 0,
            "cold_reset leaked completion from a cancelled CPU read");
    test.dut().cpu_valid = 0;
    test.tick();
    require(test.dut().image_valid == 1,
            "cancelled transient read changed committed image state");
    require(cpu_read(test, 5, 11) == 0x4B,
            "CPU could not issue a fresh read after reset cancellation");

    const auto original_byte = test.byte(kBase + 5 * 0x4000U + 11);
    const auto before_abort = test.transactions().size();
    begin_load_and_clear(test);
    test.dut().load_page = 6;
    test.dut().load_offset = 12;
    test.dut().load_data = 0x9E;
    test.dut().load_valid = 1;
    test.tick();
    test.wait_for_request();
    test.tick();
    test.dut().load_abort = 1;
    test.tick(true);
    require(test.dut().load_ready == 0,
            "abort/ack race leaked a loader completion pulse");
    require(test.dut().image_valid == 0 && test.dut().busy == 0,
            "abort/ack race did not leave the service EMPTY");
    require(test.dut().load_error == 0,
            "abort did not clear the session error state");
    require(test.byte(kBase + 6 * 0x4000U + 12) == 0x9E,
            "acknowledged physical write did not land in abort race");
    require(test.transactions().size() == before_abort + kClearBytes + 1,
            "abort issued a scrub or duplicate backend transaction");
    test.dut().load_abort = 0;
    test.dut().load_valid = 0;
    test.tick();
    require(cpu_read(test, 6, 12) == 0xFF,
            "aborted image remained CPU-visible");

    begin_load_and_clear(test);
    write_loader_byte(test, 5, 11, original_byte);
    commit_load(test);
    const auto before_detach = test.transactions().size();
    test.dut().detach = 1;
    test.tick();
    test.tick();
    require(test.dut().image_valid == 0 && test.dut().busy == 0,
            "detach did not invalidate the image exactly once");
    require(test.transactions().size() == before_detach,
            "detach scrubbed or otherwise touched backend memory");
    require(test.byte(kBase + 5 * 0x4000U + 11) == original_byte,
            "detach changed committed SDRAM contents");
    test.dut().detach = 0;
    test.tick();
}

void run_test(const char* name, void (*test)()) {
    test();
    std::cout << "PASS: " << name << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        run_test("EMPTY reads and complete parameterized clear",
                 test_empty_and_complete_clear);
        run_test("boundary writes, atomic commit, and CPU reads",
                 test_boundaries_commit_and_reads);
        run_test("invalid loader addresses fail closed and recover",
                 test_invalid_addresses_fail_closed_and_recover);
        run_test("loader priority and deferred commit",
                 test_loader_priority_and_deferred_commit);
        run_test("control dominance and overlapping one-shots",
                 test_control_dominance_and_one_shots);
        run_test("cancelled held valids require release",
                 test_cancelled_held_valids_require_release);
        run_test("loader acknowledgement control races",
                 test_loader_ack_control_races);
        run_test("abort, detach, and ordinary cold reset",
                 test_abort_detach_and_cold_reset);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "PASS: all Plus cartridge memory service tests\n";
    return 0;
}
