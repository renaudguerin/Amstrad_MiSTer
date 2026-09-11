// B6 reuses B7's executing TV80 program generator and SDRAM bus service.
// No byte assembler, SDRAM return-window model or pixel renderer is copied.
#define main b7_unused_main
#include "b7_dark_silicon_audit.cpp"
#undef main
#include <algorithm>

namespace {
Program video_program(unsigned width) {
    Program p = build_program();
    // Adapt the existing generator's explicit CRTC writes. Each match is a
    // complete OUT (C),A instruction sequence, not a patch to production state.
    const uint8_t regs[] = {63,40,50,uint8_t(0x20|width),38,0,25,30,0,7};
    for (unsigned r=0; r<10; ++r) {
        std::vector<uint8_t> key{1,0,0xbc,0x3e,uint8_t(r),0xed,0x79,1,0,0xbd,0x3e};
        auto at=std::search(p.code.begin(),p.code.end(),key.begin(),key.end());
        if(at==p.code.end()) throw AuditFailure("CRTC setup instruction absent");
        at[key.size()]=regs[r];
    }
    // The B7 ownership audit deliberately enables DMA and split/scroll;
    // disable their register writes for B6's ordinary video trace.
    for(auto key: {std::vector<uint8_t>{0x3e,1,0x32,0x0f,0x6c},
                  std::vector<uint8_t>{0x3e,5,0x32,1,0x68},
                  std::vector<uint8_t>{0x3e,0x34,0x32,4,0x68}}) {
        auto at=std::search(p.code.begin(),p.code.end(),key.begin(),key.end());
        if(at==p.code.end()) throw AuditFailure("B7 video setup instruction absent");
        at[1]=0;
    }
    // Screen at C000: video seeding must not overwrite the CPU's ROM/RAM.
    // Reuse the existing unused setup gap before its fixed measured loop.
    auto di=std::find(p.code.begin()+0x100,p.code.begin()+kLoopPc,uint8_t(0xf3));
    if(di==p.code.begin()+kLoopPc) throw AuditFailure("setup DI absent");
    const std::vector<uint8_t> screen{1,0,0x7f,0x3e,0,0xed,0x79,
                                    0x3e,0x4b,0xed,0x79,0x3e,1,0xed,0x79,
                                    0x3e,0x52,0xed,0x79,0x3e,0x10,0xed,0x79,
                                    0x3e,0x4b,0xed,0x79,1,0,0xbc,0x3e,12,0xed,0x79,
                                    1,0,0xbd,0x3e,0x30,0xed,0x79};
    p.code.insert(di,screen.begin(),screen.end());
    // Insertion must leave the fixed loop at its original address.
    p.code.erase(p.code.begin()+kLoopPc,p.code.begin()+kLoopPc+screen.size());
    return p;
}

struct Result {
    uint64_t tuple_bad=0, byte_bad=0, assembly_bad=0;
    uint64_t pending_bad=0, pending_checks=0, reset_bad=0, reset_checks=0;
    uint64_t mask_bad=0, mask_checks=0;
    uint64_t transition_checks=0, transition_waits=0, transition_bad=0;
    uint64_t returned_low=0, qualified_prime=0, native_low=0, native_high=0;
    uint64_t reset_policy_checks=0, reset_policy_bad=0;
    uint64_t shift_fetches=0, asymmetric=0, colored=0, windows=0, bad_width=0;
    uint64_t raw_tuple_diff=0, full_tag_bad=0, applied_bad=0;
    std::vector<uint8_t> de;
};

unsigned selected_tuple(const Vp10_boot_test_top& d) {
    return (d.dbg_selected_hsync<<3)|(d.dbg_selected_vsync<<2)|
           (d.dbg_selected_hblank<<1)|d.dbg_selected_vblank;
}

// A persistent request exercises liveness as well as exclusion: a latch that
// never applies any request must fail the bounded commit wait. The reference
// advances from pre-edge Full VBLANK and real bus ownership, independently of
// the observed applied-mode register.
void live_transitions(Harness& h,bool plus,Result& r,uint8_t& history) {
    unsigned applied=0;
    bool returned=false, saw_low=false, saw_prime=false;
    auto step=[&]() {
        const auto& d=h.dut;
        const unsigned requested=d.b6_mode==3?0:d.b6_mode;
        const bool reset=d.dbg_reset;
        const bool eligible=(d.b6_full_tuple&1)&&!d.b6_cpu_n;
        const unsigned next=reset||eligible?requested:applied;
        const bool fetch=d.b6_cpu_n&&!d.b6_ras_n&&!d.b6_cas_n;
        const bool shifted=d.b6_shift, high=d.b6_bs, de=d.dbg_raw_de;
        const unsigned word=d.dbg_video_vram_word;
        const uint8_t byte=applied==0&&shifted?
            (high?uint8_t(word):history):(high?word>>8:word);
        uint8_t next_history=history;
        if(fetch&&applied==0&&shifted&&high) next_history=de?word>>8:0;
        const bool commit=next!=applied;
        if(reset||commit) next_history=0;
        if(requested!=applied&&!eligible&&!reset) ++r.transition_waits;
        if(fetch&&!reset&&applied==2&&shifted) {
            if(high) ++r.native_high; else ++r.native_low;
        }
        if(returned&&fetch&&!reset&&applied==0&&shifted) {
            if(!high&&!saw_low) {++r.returned_low;saw_low=true;}
            if(high&&de&&(word>>8)&&!saw_prime) {
                ++r.qualified_prime;saw_prime=true;
            }
        }
        const unsigned oldword=d.dbg_video_plus_vidword;
        const unsigned oldbyte=d.dbg_video_vram_byte;
        unsigned assembled=d.dbg_video_cclk_p?(oldword&0xff00)|oldbyte:oldword;
        if(d.dbg_video_cclk_n) assembled=(assembled&255)|(oldbyte<<8);
        h.tick();
        r.transition_bad+=h.dut.b6_applied!=next||h.dut.b6_raw_crt!=(next==2);
        r.transition_bad+=selected_tuple(h.dut)!=
            (next==2?h.dut.b6_raw_tuple:h.dut.b6_full_tuple);
        r.transition_bad+=h.dut.b6_history!=next_history;
        if(fetch&&!reset) r.transition_bad+=h.dut.dbg_video_vram_byte!=byte;
        if(plus) r.transition_bad+=h.dut.dbg_video_plus_vidword!=(reset?0:assembled);
        if(commit) {
            ++r.transition_checks;
            if(next==0) {returned=true;saw_low=false;saw_prime=false;}
        }
        history=next_history;
        applied=next;
    };
    auto wait_for=[&](auto predicate,const char* failure) {
        for(unsigned n=0;n<1600000;++n) {
            if(predicate()) return;
            step();
        }
        throw AuditFailure(failure);
    };
    for(unsigned phase:{0U,1U}) {
        auto active_fetch=[&]() {
            return !(h.dut.b6_full_tuple&1)&&h.dut.dbg_raw_de&&
                h.dut.b6_cpu_n&&!h.dut.b6_ras_n&&!h.dut.b6_cas_n&&
                h.dut.b6_bs==phase;
        };
        wait_for(active_fetch,"no active request phase in live transition");
        h.dut.b6_mode=2;
        wait_for([&](){return applied==2;},"RawCRT request never reached its legal boundary");
        // Wait through active display, so return-to-Full is also a real
        // deferred request and native low/high fetches cannot be vacuous.
        wait_for(active_fetch,"no active RawCRT phase for return request");
        h.dut.b6_mode=0;
        wait_for([&](){return applied==0;},"Full request never reached its legal boundary");
        // Ordinary Full VBLANK may prime zero. Score the first shifted low
        // against the independent history, then require a real DE-qualified
        // nonzero high half later; do not invent nonzero blank-period history.
        wait_for([&](){return saw_low&&saw_prime;},"return never exercised shifted low and qualified prime");
    }
}

Result run_video(bool plus,unsigned type,unsigned mode,unsigned width) {
    Harness h(plus,type);
    h.verify_mutation(0);
    h.dut.production_clocking=1;
    h.dut.b6_mode=mode;
    const auto program=video_program(width);
    // Physical SDRAM video region; C000-CFFF etc are disjoint from setup.
    for(unsigned a=0xc000;a<0x10000;a+=2) {
        h.store(0,0x20000+a,uint8_t(0x13 ^ ((a>>1)&0x7f)));
        h.store(0,0x20001+a,uint8_t(0xe8 ^ ((a>>3)&0x7f)));
    }
    h.initialize(program,plus);
    if(plus) {
        h.download(build_cpr_image({{"cb00",program.code},
                                   {"cb03",std::vector<uint8_t>(16384,0)}}));
        h.wait_for_reset_release();
    }
    h.run(1800000); // setup, filter acquisition, then complete programmed frames
    std::cout<<"SETUP machine="<<(plus?2:type)<<" PC="<<std::hex<<h.dut.dbg_pc<<" RGB="<<h.dut.dbg_video_rgb<<std::dec<<std::endl;
    Result r;
    uint8_t history=0;
    bool history_valid=false, old_de=h.dut.b6_mixer_de, complete=false;
    unsigned window=0;
    bool blank_tag=false,tag_valid=false;
    for(unsigned n=0;n<1400000;++n) {
        const auto &d=h.dut;
        const bool fetch=d.b6_cpu_n&&!d.b6_ras_n&&!d.b6_cas_n;
        const bool bs=d.b6_bs, shift=d.b6_shift;
        const unsigned word=d.dbg_video_vram_word;
        const uint8_t native=bs?word>>8:word;
        // Pre-edge oracle: byte policy in docs/b6-video-boundary.md. Never
        // recover expected history from the observed serializer output.
        const uint8_t expected=(mode==0&&shift)?(bs?uint8_t(word):history):native;
        const bool check=!(mode==0&&shift&&!bs&&!history_valid);
        if(fetch&&mode==0&&shift&&bs) {
            history=d.dbg_raw_de?word>>8:0;
            history_valid=true;
        }
        const bool cp=d.dbg_video_cclk_p, cn=d.dbg_video_cclk_n;
        const unsigned oldword=d.dbg_video_plus_vidword, oldbyte=d.dbg_video_vram_byte;
        const unsigned assembled=(cp?(oldword&0xff00)|oldbyte:oldword);
        const unsigned expected_word=cn?((assembled&255)|(oldbyte<<8)):assembled;
        const bool ce=d.dbg_video_ce16, raw_blank=d.b6_raw_vblank;
        const bool expected_mask=blank_tag;
        h.tick();
        if(ce) {
            // Renderer samples raw blank on this edge. The converter consumes
            // the preceding dot's tag alongside that dot's registered RGB.
            // Both raw modes carry the additional vertical mask; Full never
            // does (docs/b6-video-boundary.md, "Pixel and output pipeline").
            if(mode&&tag_valid&&expected_mask) {
                ++r.mask_checks;
                r.mask_bad+=h.dut.b6_color_rgb!=0;
            }
            if(!mode) r.full_tag_bad+=h.dut.b6_pixel_vblank!=0;
            blank_tag=raw_blank;
            tag_valid=true;
        }
        if(fetch) {
            r.byte_bad+=check&&h.dut.dbg_video_vram_byte!=expected;
            r.shift_fetches+=shift;
            r.asymmetric+=(word&255)!=(word>>8);
        }
        if(plus) r.assembly_bad+=h.dut.dbg_video_plus_vidword!=expected_word;
        const unsigned tuple=(h.dut.dbg_selected_hsync<<3)|(h.dut.dbg_selected_vsync<<2)|
                             (h.dut.dbg_selected_hblank<<1)|h.dut.dbg_selected_vblank;
        if(mode<2) r.tuple_bad+=tuple!=h.dut.b6_full_tuple;
        // Raw CRT must really leave the Full tuple: an unchanged selected
        // tuple here would mean the selection is dead, not that it is safe.
        else r.raw_tuple_diff+=tuple!=h.dut.b6_full_tuple;
        // One applied selection owns every policy, and the steady trace above
        // never crosses a boundary, so it must equal the request throughout.
        r.applied_bad+=h.dut.b6_applied!=mode||h.dut.b6_raw_crt!=(mode==2);
        if(h.dut.b6_mixer_ce) {
            const bool de=h.dut.b6_mixer_de;
            r.de.push_back(de);
            // Unmodified sys/video_mixer.sv emits IMPLICIT warnings for
            // R_in/G_in/B_in (generate scope, line134) under Verilator.
            // Its DE/CE is real and usable; numeric RGB is checked at the
            // real converter instead. Mixer RGB is not verified here.
            r.colored+=de&&h.dut.b6_color_rgb!=0;
            if(de&&!old_de) {window=0;complete=true;}
            if(de) ++window;
            if(!de&&old_de&&complete) {
                ++r.windows;
                // 64 us line / 16 MHz dots = 1024; Full blank is 16 us,
                // hence 768 enabled acquisitions in each complete DE window.
                if(window!=768) ++r.bad_width;
            }
            old_de=de;
        }
    }
    // Complete both fetch-phase requests on classic type0 and Plus. Ordinary
    // trace controls finish first and therefore remain comparable by tick.
    if((plus||type==0)&&mode==0&&width==5) {
        live_transitions(h,plus,r,history);
        for(unsigned n=0;n<100000;++n) {
            const auto &d=h.dut;
            bool fetch=d.b6_cpu_n&&!d.b6_ras_n&&!d.b6_cas_n;
            if(fetch&&d.b6_shift&&d.b6_bs) {
                history=d.dbg_raw_de?d.dbg_video_vram_word>>8:0;
                history_valid=true;
            }
            if(fetch&&d.b6_shift&&!d.b6_bs&&history_valid&&history&&
               !(d.b6_full_tuple&1)&&uint8_t(d.dbg_video_vram_word)!=history) {
                // Mid-pair and outside Full VBLANK: the applied mode must
                // stay Full, so this accepted low phase still emits history.
                h.dut.b6_mode=2;
                h.tick();
                ++r.pending_checks;
                r.pending_bad+=h.dut.dbg_video_vram_byte!=history;
                h.dut.b6_mode=0;
                break;
            }
            h.tick();
        }
        // Prime real history again and assert reset just before an accepted
        // shifted low half. This avoids letting a reset-time high fetch
        // naturally overwrite stale history before it can be observed.
        for(unsigned n=0;n<100000;++n) {
            const auto &d=h.dut;
            const bool fetch=d.b6_cpu_n&&!d.b6_ras_n&&!d.b6_cas_n;
            if(fetch&&d.b6_shift&&d.b6_bs)
                history=d.dbg_raw_de?d.dbg_video_vram_word>>8:0;
            if(fetch&&d.b6_shift&&!d.b6_bs&&history&&!(d.b6_full_tuple&1)) {
                h.dut.reset_btn=1;
                // Follow the real reset sequencer. Its asynchronous request
                // is not identical to the motherboard's reset signal.
                for(unsigned k=0;k<256;++k) {
                    if(h.dut.dbg_reset) {
                        const bool primed=h.dut.b6_history!=0;
                        h.tick();
                        if(primed) {++r.reset_checks;r.reset_bad+=h.dut.b6_history!=0;}
                        // Reset applies requests without a Full-VBLANK wait.
                        // Exercise RawCRT and then reserved3 while holding the
                        // same real reset; 3 must normalize to Full immediately.
                        for(unsigned requested:{2U,3U}) {
                            const bool bypass=!(h.dut.b6_full_tuple&1)&&h.dut.dbg_reset;
                            h.dut.b6_mode=requested;
                            h.tick();
                            const unsigned expected=requested==3?0:requested;
                            ++r.reset_policy_checks;
                            r.reset_policy_bad+=!bypass||h.dut.b6_applied!=expected||
                                h.dut.b6_raw_crt!=(expected==2)||h.dut.b6_history!=0||
                                selected_tuple(h.dut)!=(expected==2?h.dut.b6_raw_tuple:h.dut.b6_full_tuple);
                        }
                        h.dut.b6_mode=0;
                        break;
                    }
                    h.tick();
                }
                h.dut.reset_btn=0;
                break;
            }
            h.tick();
        }
    }
    return r;
}
}
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    bool bad=false;
    unsigned only_machine=3,only_width=0;
    for(int i=1;i+1<argc;++i) {
        if(std::string(argv[i])=="--machine") only_machine=std::stoul(argv[++i]);
        else if(std::string(argv[i])=="--width") only_width=std::stoul(argv[++i]);
    }
    try {
        for(unsigned machine=0;machine<3;++machine) for(unsigned width:{14U,5U}) {
            if(only_machine<3&&machine!=only_machine) continue;
            if(only_width&&width!=only_width) continue;
            Result full;
            for(unsigned mode=0;mode<3;++mode) {
                auto r=run_video(machine==2,machine==1,mode,width);
                if(mode==0) full=r;
                uint64_t de_bad=0;
                if(mode==1) {
                    if(r.de.size()!=full.de.size()) throw AuditFailure("different pixel sample counts");
                    for(size_t i=0;i<r.de.size();++i) de_bad+=r.de[i]!=full.de[i];
                }
                const bool nonvacuous=r.asymmetric&&r.colored&&r.windows&&
                                      (width!=5||r.shift_fetches)&&
                                      (mode!=2||r.raw_tuple_diff)&&
                                      (mode==0||r.mask_checks);
                const bool state_case=(machine==0||machine==2)&&mode==0&&width==5;
                const bool failed=r.mask_bad||r.full_tag_bad||r.applied_bad||
                                  r.pending_bad||r.reset_bad||r.transition_bad||r.reset_policy_bad||
                                  (state_case&&(!r.pending_checks||!r.reset_checks||r.transition_checks!=4||
                                   !r.transition_waits||r.returned_low!=2||r.qualified_prime!=2||
                                   !r.native_low||!r.native_high||r.reset_policy_checks!=2))||r.tuple_bad||r.byte_bad||r.assembly_bad||de_bad||
                                  !nonvacuous||(mode==0&&r.bad_width);
                std::cout << "B6 " << (failed?"FAIL":"PASS") << " machine=" << machine
                          << " mode=" <<mode<< " hs_width="<<width
                          << " raw_vertical_mask="<<r.mask_bad<<"/"<<r.mask_checks
                          << " full_tag="<<r.full_tag_bad<<" applied="<<r.applied_bad
                          << " raw_tuple_diff="<<r.raw_tuple_diff
                          << " pending="<<r.pending_bad<<"/"<<r.pending_checks
                          << " reset="<<r.reset_bad<<"/"<<r.reset_checks
                          << " transition="<<r.transition_bad<<"/"<<r.transition_checks
                          << " waits="<<r.transition_waits
                          << " returned_low="<<r.returned_low<<" qualified_prime="<<r.qualified_prime
                          << " native_halves="<<r.native_low<<"/"<<r.native_high
                          << " reset_policy="<<r.reset_policy_bad<<"/"<<r.reset_policy_checks
                          << " tuple="<<r.tuple_bad<<" byte="<<r.byte_bad
                          << " plus_assembly="<<r.assembly_bad<<" DE_vs_Full="<<de_bad
                          << " full_width_bad="<<r.bad_width<<" windows="<<r.windows
                          << " shift_fetches="<<r.shift_fetches<<" asymmetric="<<r.asymmetric
                          << " colored="<<r.colored<<std::endl;
                bad|=failed;
            }
        }
    } catch(const std::exception&e) {std::cerr<<"B6 fixture: "<<e.what()<<'\n';return 2;}
    return bad?1:0;
}
