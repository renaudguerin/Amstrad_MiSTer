// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND). French v1.11 §7.2 pp39–41, §13.6.3 p125.
// Actual production T80/motherboard/PPI/ASIC: the CPU synchronizes through PPI,
// then executes ordinary timed instructions. No counter-selected CPU release.
#include "Vb18_snapshot_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }

// Assembled at 9000 from the small published FR §7.2 synchronization/wait
// routine (rendered pp40/41). No title/disk payload or runtime assembler.
// 9000: DI; LD B,F5; HL=19968-21; DE=-11; wait VS on/off/on, count its
// remaining width with ADD HL,DE / IN A,(C) / RRA / JR C, then EX DE,HL.
// 9022: drift the next PPI sample by 1us per 19969us frame until bit0 rises.
// 9031: DE=19968-11; JP wait_usec, returning to the caller's C0vs origin.
// 9037: wait_usec(DE), excluding CALL: shift DE by3, subtract5, loop8us,
// jump into 0..7 trailing NOPs, RET. AND7 clears carry before SBC HL,BC.
constexpr uint8_t calibration[] = {
    0xf3, 0x06, 0xf5, 0x21, 0xeb, 0x4d, 0x11, 0xf5, 0xff, 0xed, 0x78, 0x1f,
    0x30, 0xfb, 0xed, 0x78, 0x1f, 0x38, 0xfb, 0xed, 0x78, 0x1f, 0x30, 0xfb,
    0x19, 0xed, 0x78, 0x1f, 0x38, 0xfa, 0xeb, 0xcd, 0x37, 0x90, 0x06, 0xf5,
    0xed, 0x78, 0x1f, 0x38, 0x08, 0x11, 0xed, 0x4d, 0xcd, 0x37, 0x90, 0x18,
    0xf1, 0x11, 0xf5, 0x4d, 0xc3, 0x37, 0x90, 0x21, 0x63, 0x90, 0x06, 0x00,
    0x7b, 0xe6, 0x07, 0x4f, 0xed, 0x42, 0xcb, 0x3a, 0xcb, 0x1b, 0xcb, 0x3a,
    0xcb, 0x1b, 0xcb, 0x3a, 0xcb, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x00,
    0x1b, 0x7a, 0xb3, 0x00, 0xc2, 0x54, 0x90, 0xe9, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc9,
};

enum class Kind { out, outi, live_match, lower_terminal };
struct Case { const char* name; Kind kind; unsigned nominal_c0, terminal_c0; uint8_t payload; };
// FR §13.6.3 p125, rendered type3/4 chart: with old R0=3F, OUT(C)3C
// and OUTI3B extend the line (3F->40); OUT(C)3D and OUTI3C miss it
// (3F->00). Original Plus A4 photos corroborate the deadline and OUTI3C.
constexpr Case cases[] = {
    {"OUT(C) 3C early", Kind::out, 60, 64, 127},
    {"OUT(C) 3D late", Kind::out, 61, 0, 127},
    {"OUTI 3B early", Kind::outi, 59, 64, 127},
    {"OUTI 3C late", Kind::outi, 60, 0, 127},
    // Parent-selected model control: preserve live new equality too.
    // Unverified against the alternative of delaying register admission;
    // the inclusive-period rule alone does not settle this late write.
    // Enlarge early to127, then write63 while C0=63 (not previously terminal).
    {"live equality at C0=63", Kind::live_match, 61, 0, 63},
    // Software-reference control, NOT a French/hardware lowering golden:
    // authentic DSC4 first E2B has oldR0=63/currentC0=63, writes59 and
    // wraps, as observed in AmSpirit.
    {"lower59 on old terminal (software reference)", Kind::lower_terminal, 61, 0, 59},
};
struct Program {
    std::vector<uint8_t> bytes{0xf3,0x31,0xf0,0xbf,0x01,0,0xbc,0xed,0x49,0xcd,0,0x90};
    unsigned critical;
    explicit Program(const Case& c) {
        auto emit=[&](std::initializer_list<uint8_t> b){bytes.insert(bytes.end(),b);};
        if (c.kind == Kind::live_match) {
            emit({0x01,127,0xbd}); // 3us, then52 NOPs: early OUT at55.
            bytes.insert(bytes.end(),52,0);
            emit({0xed,0x49,0x0e,63}); // OUT4us + LD C,63 2us ->61.
        } else {
            emit({0x01,uint8_t(c.kind==Kind::outi ? 0x3f : c.payload),
                          uint8_t(c.kind==Kind::outi ? 0xbe : 0xbd)});
            const unsigned setup=c.kind==Kind::outi ? 6 : 3;
            if(c.kind==Kind::outi) emit({0x21,0,0xa0}); // own payload at A000.
            bytes.insert(bytes.end(),c.nominal_c0-setup,0);
        }
        critical=0x8000+bytes.size();
        emit({0xed,uint8_t(c.kind==Kind::outi ? 0xa3 : 0x49),0x76});
    }
    std::vector<uint8_t> snapshot() const {
        std::vector<uint8_t> s(0x20100,0);
        std::copy_n("MV - SNA",8,s.begin()); s[0x10]=3;s[0x11]=0x30;
        s[0x21]=0xf0;s[0x22]=0xbf;s[0x23]=0;s[0x24]=0x80;s[0x25]=1;
        s[0x40]=0x8e; // ROMs off, IFFs remain0, ordinary 312x64us frame.
        const uint8_t regs[]={63,40,46,14,38,0,25,30,0,7,0,0,0x30,0,0,0,0,0};
        std::copy(std::begin(regs),std::end(regs),s.begin()+0x43);
        s[0x59]=0x82;s[0x6b]=128;s[0x6d]=4;s[0xa4]=3;
        // Standard v3 counter origin; synchronization comes only from PPI.
        std::copy(bytes.begin(),bytes.end(),s.begin()+0x8100);
        std::copy(std::begin(calibration),std::end(calibration),s.begin()+0x9100);
        s[0xa100]=127;
        return s;
    }
};
struct Bench {
    Vb18_snapshot_top d;
    std::vector<uint8_t> memory=std::vector<uint8_t>(0x800000,0xff);
    uint64_t ticks=0;
    Bench() {
        d.pause=0;d.filter_request=0;
        d.model=0;d.crtc_type=1;d.save_req=0;d.sna_download=0;
        d.load_wr=0;d.load_addr=0;d.load_data=0;
        d.mem_din=0xff;d.cart_dout=0xff;d.cart_ack=0;d.ddram_busy=0;
    }
    void tick() {
        d.clk=0;d.eval();d.mem_din=memory.at(d.mem_addr);d.eval();
        if(d.mem_wr&&!d.owner_reset) memory.at(d.mem_addr)=d.mem_dout;
        if(d.load_mem_wr) memory.at(d.load_mem_addr)=d.load_data;
        d.clk=1;d.eval();d.clk=0;d.eval();++ticks;
    }
    void run(unsigned n) {while(n--)tick();}
    void load(const std::vector<uint8_t>& s) {
        d.model=0;d.crtc_type=1;d.reset_btn=1;run(1024);d.reset_btn=0;
        d.sna_download=1;run(4);
        for(unsigned i=0;i<s.size();++i){d.load_wr=1;d.load_addr=i;d.load_data=s[i];tick();d.load_wr=0;tick();}
        d.sna_download=0;
        unsigned n=0;while(!d.sna_load&&++n<1000)tick();
        require(d.sna_load,"SNA apply did not occur");tick();
        require((d.cpu_reg[2]&65535)==0x8000,"SNA PC did not restore");
        require(d.plus_hcc==0&&d.plus_row==0&&d.plus_raster==0,"SNA counter origin did not restore");
    }
};
bool exercise(const Case& c) {
    Bench b;Program p(c);b.load(p.snapshot());auto& d=b.d;
    bool old_start=false,aligned=false,critical=false,edge=false,wrote=false;
    unsigned critical_c0=0,return_c0=0,terminal=999,write_c0=999,old_r0=999;
    for(unsigned n=0;n<30000000&&d.cpu_halt_n;++n) {
        if(!d.owner_reset&&!d.sna_hold) {
            bool start=d.cpu_insn_start;
            if(start&&!old_start) {
                if(d.cpu_addr==0x800c){aligned=true;return_c0=d.plus_hcc;}
                if(d.cpu_addr==p.critical) {
                    critical=true;critical_c0=d.plus_hcc;
                    require(d.plus_row==30&&d.plus_raster==0,"PPI calibration missed the first VSYNC line");
                }
            }
            old_start=start;
        }
        const bool terminal_edge=critical&&!edge&&d.plus_cclk&&d.plus_hcc==63;
        unsigned pre_r0=d.plus_r0,pre_c0=d.plus_hcc;
        b.tick();
        if(critical&&!wrote&&pre_r0!=d.plus_r0&&d.plus_r0==c.payload){wrote=true;write_c0=pre_c0;old_r0=pre_r0;}
        if(terminal_edge){edge=true;terminal=d.plus_hcc;}
    }
    // INSN_START is T80's representable T2 observer, not a physical C0vs
    // origin. Post-RET63 (also observed in AmSpirit) and critical C0 are
    // diagnostic: AmSpirit's BEFORE-instruction stops differ by1 character.
    // The unchanged PPI program and EARLY/LATE counter consequences are the
    // independent deadline oracle; do not fit NOPs to either observer.
    bool pass=aligned&&critical&&edge&&wrote&&!d.cpu_halt_n&&
              d.plus_r0==c.payload&&terminal==c.terminal_c0;
    if(c.kind==Kind::live_match||c.kind==Kind::lower_terminal)
        pass &= write_c0==63 && old_r0==(c.kind==Kind::live_match ? 127u : 63u);
    std::cout<<(pass?"PASS ":"FAIL ")<<c.name<<": terminal="<<terminal
             <<" expected="<<c.terminal_c0<<", R0="<<unsigned(d.plus_r0)
             <<", write C0="<<write_c0<<", critical T2 C0="<<critical_c0
             <<", post-RET T2 C0="<<return_c0<<", clocks="<<b.ticks<<'\n';
    return pass;
}

bool zero_exit_transport() {
    // FR §13.6.3 p125 includes old R0=0 ->10. This exact synthetic SNA
    // was independently run in AmSpirit.
    // Those software-reference CPU boundary states, not an assumed raw
    // register-write phase, pin this degenerate zero-line transition.
    auto s=Program(cases[0]).snapshot();
    std::fill(s.begin()+0x100,s.end(),0); // construct input before loading
    s[0x43]=0;
    const uint8_t code[]={0xf3,0x01,0,0xbc,0xed,0x49,0x01,0,0xbd,
                          0x3e,10,0xed,0x79,0x76};
    std::copy(std::begin(code),std::end(code),s.begin()+0x8100);
    Bench b;b.load(s);auto& d=b.d;
    bool old_start=false,before=false,after=false;
    std::array<unsigned,4> pre{},post{};
    for(unsigned n=0;n<5000&&d.cpu_halt_n;++n) {
        if(!d.owner_reset&&!d.sna_hold) {
            bool start=d.cpu_insn_start;
            if(start&&!old_start) {
                const std::array<unsigned,4> state={d.plus_hcc,d.plus_row,d.plus_raster,d.plus_r0};
                if(d.cpu_addr==0x800b){before=true;pre=state;}
                if(d.cpu_addr==0x800d){after=true;post=state;}
            }
            old_start=start;
        }
        b.tick();
    }
    const bool pass=before&&after&&!d.cpu_halt_n&&
        pre==std::array<unsigned,4>{0,1,5,0}&&
        post==std::array<unsigned,4>{0,2,1,10};
    std::cout<<(pass?"PASS ":"FAIL ")<<"zero exit (source/software reference): before="
             <<pre[0]<<'/'<<pre[1]<<'/'<<pre[2]<<" R0="<<pre[3]
             <<", after="<<post[0]<<'/'<<post[1]<<'/'<<post[2]<<" R0="<<post[3]<<'\n';
    return pass;
}
}
int main(int argc,char** argv) {
    Verilated::commandArgs(argc,argv);unsigned passed=0,failed=0;
    for(const auto& c:cases)try {exercise(c)?++passed:++failed;}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<c.name<<": "<<e.what()<<'\n';}
    try {zero_exit_transport()?++passed:++failed;}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL zero exit: "<<e.what()<<'\n';}
    std::cout<<"asic-r0-terminal: "<<passed<<" passed, "<<failed<<" failed\n";
    return failed?1:0;
}
