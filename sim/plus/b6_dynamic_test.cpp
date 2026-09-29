// CPU writes disturb real CRTC/GA timing; filter acquisition is observed, not assumed. The SDRAM,
// renderer, color converter, gamma and mixer are production instances.
#define B6_BOUNDARY_LIBRARY
#include "b6_video_boundary_test.cpp"
#include <array>
#include <fstream>
#include "Vp10_boot_test_top___024root.h"
#define FILTER(name) d.rootp->p10_boot_test_top__DOT__mb__DOT__crt_filter__DOT__##name
#define CRTC(name) d.rootp->p10_boot_test_top__DOT__mb__DOT__crtc__DOT__##name

namespace {
constexpr unsigned stages=6;
const char* names[stages]={"ordinary","short","missing","multiple","wide","restore"};
Program dynamic_program() {
    Program p=video_program(14);
    size_t pc=kLoopPc;
    auto emit=[&](unsigned v){p.code.at(pc++)=v;};
    auto io=[&](unsigned port,unsigned value){
        emit(1);emit(port&255);emit(port>>8);emit(0x3e);emit(value);emit(0xed);emit(0x79);
    };
    auto reg=[&](unsigned r,unsigned v){io(0xbc00,r);io(0xbd00,v);};
    for(unsigned stage=0;stage<stages;++stage) {
        if(stage==1) reg(3,0x25);              // five-character raw HS
        if(stage==2) {reg(2,255);reg(7,255);} // no horizontal/vertical equality
        if(stage==3) {reg(0,15);reg(1,8);reg(2,4);reg(3,0x22);reg(7,8);}
        if(stage==4) {reg(0,7);reg(2,2);reg(3,0x20);reg(6,4);}
        if(stage==5) {reg(0,63);reg(1,40);reg(2,50);reg(3,0x2e);reg(6,25);reg(7,30);}
        // A real CPU write labels the programmed regime; no test drives CRTC state.
        emit(0x3e);emit(stage+1);emit(0x32);emit(0);emit(0xb0);
        // Unrolled real memory reads avoid relying on the TV80 substitute's
        // relative-branch behavior. Each interval spans many complete lines.
        const unsigned reads[stages]={1200,1000,400,1000,800,0};
        for(unsigned wait=0;wait<reads[stage];++wait) {
            emit(0x3a);emit(0);emit(0x40); // LD A,(&4000)
        }
    }
    emit(0x76); // HALT: interrupts remain disabled
    return p;
}
// Passive snapshots immediately before h.tick(): the previous tick has left
// clk low and evaluated. No trace operation evaluates or drives the DUT.
// Tuple bits are HS,VS,HB,VB (3..0); raw HB is CRTC HS, raw HS/VS are GA.
// Counters are CE4 units, tick is a 64 MHz loop index after initialization.
// hSyncCount is regenerated phase BEFORE syncgen's blocking increment/reset;
// hSyncReg is a falling-edge classification arm, not a monitor-lock flag.
class SyncTrace {
    std::ofstream out;
    std::array<unsigned,19> previous{};
    unsigned previous_count=0;
    int previous_stage=-2;
    bool first=true;
public:
    SyncTrace(const std::string& prefix,unsigned machine,unsigned mode) {
        if(prefix.empty())return;
        out.exceptions(std::ios::failbit|std::ios::badbit);
        out.open(prefix+"-machine"+std::to_string(machine)+"-mode"+std::to_string(mode)+".csv");
        out<<"tick,stage,stage_name,stage_age,event_bits,requested,applied,crtc_hs,crtc_vs,ga_raw_tuple,filtered_tuple,selected_tuple,mixer_de,line_estimate,arm,no_hsync,hsync_mask,SHIFT,hs4,shift_internal,filter_hs,filter_old_hs,r3_hs_width,training_syncs,ce4,hSyncCount,line_time,hSyncCount2x,old_vs\n";
    }
    void sample(unsigned tick,int stage,unsigned age,Vp10_boot_test_top& d) {
        if(!out.is_open())return;
        const std::array<unsigned,19> state={d.b6_mode,d.b6_applied,
            d.dbg_raw_hsync,d.dbg_raw_vsync,d.b6_raw_tuple,d.b6_full_tuple,
            selected_tuple(d),d.b6_mixer_de,FILTER(hSyncSize),
            FILTER(hSyncReg),FILTER(no_hsync),FILTER(hsync_mask),
            d.b6_shift,FILTER(hs4),FILTER(shift),FILTER(hsync_i),
            FILTER(syncgen__DOT__old_hsync),CRTC(R3_h_sync_width),FILTER(syncs)};
        const unsigned count=FILTER(syncgen__DOT__hSyncCount);
        const bool ce4=d.rootp->p10_boot_test_top__DOT__mb__DOT__phi_en_n;
        // Event bits: 1 initial, 2 stage entry, 4 observed state change,
        // 8 CE4-qualified filter input edge, 16 counter reset/wrap observed.
        // Counter increments and CE toggles alone do not emit rows.
        const unsigned events=(first?1:0)|(stage!=previous_stage?2:0)|
            (state!=previous?4:0)|((ce4&&state[15]!=state[16])?8:0)|
            ((!first&&count<previous_count)?16:0);
        if(events) {
            out<<tick<<','<<stage<<','<<(stage<0?"setup":names[stage])<<','<<age<<','<<events;
            for(auto value:state)out<<','<<value;
            out<<','<<ce4<<','<<count<<','<<unsigned(FILTER(hsyncfilt__DOT__line_time))<<','
               <<unsigned(FILTER(syncgen__DOT__hSyncCount2x))<<','
               <<unsigned(FILTER(syncgen__DOT__old_vs))<<'\n';
        }
        previous=state;previous_stage=stage;previous_count=count;first=false;
    }
    void finish() {if(out.is_open())out.close();}
};
struct Samples {
    std::vector<uint8_t> timing;
    std::vector<uint32_t> rgb;
    std::vector<uint8_t> stage;
    std::array<uint64_t,stages> ticks{},hs_edges{},hs_high{},blank{},shift{},lit{};
    uint64_t tuple_bad=0,byte_bad=0,mask_bad=0,mask_checks=0,mixer_bad=0,mixer_checks=0;
    uint64_t short_pulses=0,short_width_bad=0;
    std::array<uint64_t,stages> armed_falls{},hs4_ticks{};
};
Samples dynamic_run(unsigned machine,unsigned mode,bool probe,const std::string& trace_prefix) {
    Harness h(machine==2,machine==1);
    h.verify_mutation(0);h.dut.production_clocking=1;h.dut.b6_mode=mode;
    for(unsigned a=0xc000;a<0x10000;a+=2) {
        h.store(0,0x20000+a,uint8_t(0x13^((a>>1)&0x7f)));
        h.store(0,0x20001+a,uint8_t(0xe8^((a>>3)&0x7f)));
    }
    const auto p=dynamic_program();h.initialize(p,machine==2);
    if(machine==2) {h.download(build_cpr_image({{"cb00",p.code},{"cb03",std::vector<uint8_t>(16384,0)}}));h.wait_for_reset_release();}
    SyncTrace trace(trace_prefix,machine,mode);
    Samples s;
    int stage=-1;unsigned age=0;bool old_hs=false;
    bool pulse_open=false;unsigned pulse_start=0;
    uint8_t history=0;bool history_valid=false,tag=false,tag_valid=false;
    // Disabled gamma still has its real sampled-pixel pipeline. Feed this
    // transport oracle from the independently observed production converter,
    // never from the output being checked. Mixer captures on its old CE.
    uint32_t gamma_input=0,gamma_output=0,mixer_r=0,vga=0;
    bool old_ce=false;unsigned pipeline_age=0;
    for(unsigned n=0;n<16000000;++n) {
        auto&d=h.dut;
        if(!d.dbg_mreq_n&&!d.dbg_wr_n&&d.dbg_addr==0xb000&&d.dbg_dout>=1&&d.dbg_dout<=stages) {
            int next=d.dbg_dout-1;if(next!=stage){std::cerr<<"stage "<<next<<" tick="<<n<<" pc="<<std::hex<<d.dbg_pc<<std::dec<<std::endl;stage=next;age=0;}
        }
        trace.sample(n,stage,age,d);
        // Compare the production diagnostic port against independently probed
        // state. This catches disconnected ports and misordered filter-state bits.
        const unsigned expected_observation = d.b6_mode | (d.b6_applied << 2) |
            (FILTER(hSyncSize) << 4) | (FILTER(hs4) << 13) | (d.b6_shift << 14) |
            (FILTER(no_hsync) << 15) | (FILTER(hsync_mask) << 16) |
            (FILTER(hSyncReg) << 17) | (FILTER(syncs) << 18);
        if(d.b6_sync_observation != expected_observation)
            throw AuditFailure("B22 motherboard sync observation mismatch tick="+std::to_string(n));
        // Requests are constant and applied during initialization/reset. Every
        // oracle below assumes this mode; fail explicitly if that premise fails.
        if(d.b6_applied!=mode)throw AuditFailure("B6 applied-mode mismatch machine="+
            std::to_string(machine)+" tick="+std::to_string(n)+" requested="+
            std::to_string(mode)+" applied="+std::to_string(d.b6_applied));
        const bool ce=d.dbg_video_ce16;
        const bool fetch=d.b6_cpu_n&&!d.b6_ras_n&&!d.b6_cas_n;
        const unsigned word=d.dbg_video_vram_word;
        const bool bs=d.b6_bs,shift=d.b6_shift;
        const unsigned expected_byte=mode==0&&shift?(bs?uint8_t(word):history):(bs?word>>8:uint8_t(word));
        const bool byte_valid=mode!=0||!shift||bs||history_valid;
        if(fetch&&mode==0&&shift&&bs){history=d.dbg_raw_de?word>>8:0;history_valid=true;}
        const bool blank=d.b6_raw_vblank,mask=tag;
        if(d.b6_mixer_ce)vga=mixer_r;
        mixer_r=gamma_output;
        if(ce&&!old_ce){gamma_output=gamma_input;gamma_input=d.b6_color_rgb;}
        old_ce=ce;
        const unsigned old_r3=CRTC(R3_h_sync_width), old_hs4=FILTER(hs4), old_size=FILTER(hSyncSize);
        const unsigned old_count=FILTER(syncgen__DOT__hSyncCount);
        if(probe&&stage<=1&&stage>=0&&age<40000&&d.rootp->p10_boot_test_top__DOT__mb__DOT__phi_en_n && FILTER(syncgen__DOT__old_hsync)!=FILTER(hsync_i))
            std::cerr<<"B22 machine="<<machine<<" mode="<<mode<<" edge tick="<<n<<" stage="<<stage<<" age="<<age<<" hs="<<unsigned(FILTER(hsync_i))
                     <<" count="<<old_count<<" size="<<unsigned(FILTER(hSyncSize))<<" reg="<<unsigned(FILTER(hSyncReg))<<'\n';
        const bool raw_hs_before=d.b6_raw_tuple&2;
        const bool armed_fall=d.rootp->p10_boot_test_top__DOT__mb__DOT__phi_en_n &&
            FILTER(syncgen__DOT__old_hsync) && !FILTER(hsync_i) && FILTER(hSyncReg);
        h.tick();++pipeline_age;
        if(probe && (old_r3!=CRTC(R3_h_sync_width)||old_hs4!=FILTER(hs4)||old_size!=FILTER(hSyncSize)))
            std::cerr<<"B22 machine="<<machine<<" mode="<<mode<<" tick="<<n<<" stage="<<stage<<" R3="<<old_r3<<"->"<<unsigned(CRTC(R3_h_sync_width))
                     <<" C0="<<unsigned(CRTC(hcc))<<" C3="<<unsigned(CRTC(hsc))
                     <<" count="<<old_count<<"->"<<unsigned(FILTER(syncgen__DOT__hSyncCount))
                     <<" size="<<old_size<<"->"<<unsigned(FILTER(hSyncSize))<<" hs4="<<old_hs4<<"->"<<unsigned(FILTER(hs4))<<" shift="<<unsigned(FILTER(shift))<<'\n';
        if(stage>=0&&age++>20000) {
            ++s.ticks[stage];
            const bool hs=d.b6_raw_tuple&2;
            s.armed_falls[stage]+=armed_fall;s.hs4_ticks[stage]+=FILTER(hs4);
            // Complete pulses wholly inside the measured short stage only.
            // French ACCC v1.11 §14.1 p.132: preprogrammed R3l is the
            // CRTC HSYNC duration in microseconds. Five at 64 MHz = 320 ticks.
            // This is raw CRTC HSYNC (the tuple's HBLANK), not GA C-HSYNC.
            if(stage==1) {
                if(hs&&!raw_hs_before){pulse_start=n;pulse_open=true;}
                if(!hs&&raw_hs_before&&pulse_open) {
                    ++s.short_pulses;s.short_width_bad+=n-pulse_start!=5*64;
                    pulse_open=false;
                }
            } else pulse_open=false;
            s.hs_edges[stage]+=hs&&!old_hs;s.hs_high[stage]+=hs;old_hs=hs;
            s.blank[stage]+=d.b6_raw_vblank;s.shift[stage]+=fetch&&shift;
            s.tuple_bad+=selected_tuple(d)!=(mode==2?d.b6_raw_tuple:d.b6_full_tuple);
            if(fetch&&byte_valid)s.byte_bad+=d.dbg_video_vram_byte!=expected_byte;
            if(ce&&mode&&tag_valid&&mask){++s.mask_checks;s.mask_bad+=d.b6_color_rgb!=0;}
            if(pipeline_age>32){++s.mixer_checks;s.mixer_bad+=d.b6_mixer_rgb!=vga;}
            if(d.b6_mixer_ce) {
                s.timing.push_back((selected_tuple(d)<<2)|(d.b6_mixer_de<<1)|d.b6_mixer_ce);
                s.rgb.push_back(d.b6_mixer_rgb);s.stage.push_back(stage);
                s.lit[stage]+=d.b6_mixer_de&&d.b6_mixer_rgb;
            }
        }
        if(ce){tag=blank;tag_valid=true;}
        if(stage==int(stages)-1&&age>1600000)break;
    }
    trace.finish();
    return s;
}
}
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    unsigned only=3;bool probe=false;std::string trace_prefix;
    for(int i=1;i<argc;++i)if(std::string(argv[i])=="--probe-sync")probe=true;
    for(int i=1;i+1<argc;++i)if(std::string(argv[i])=="--machine")only=std::stoul(argv[++i]);
    for(int i=1;i<argc;++i)if(std::string(argv[i])=="--trace-sync") {
        if(i+1==argc||std::string(argv[i+1]).empty()||std::string(argv[i+1]).front()=='-') {
            std::cerr<<"--trace-sync requires a file prefix\n";return 2;
        }
        trace_prefix=argv[++i];
    }
    bool bad=false;
    try {
        for(unsigned machine=0;machine<3;++machine) {
            if(only<3&&machine!=only)continue;
            Samples full;
            for(unsigned mode=0;mode<3;++mode) {
                auto s=dynamic_run(machine,mode,probe,trace_prefix);
                std::array<uint64_t,stages> differences{};
                uint64_t timing_bad=0;
                if(mode==0)full=s;
                if(mode==1) {
                    if(s.timing.size()!=full.timing.size()||s.stage!=full.stage)throw AuditFailure("CPU trace/sample alignment differs");
                    for(size_t i=0;i<s.timing.size();++i){
                        timing_bad+=s.timing[i]!=full.timing[i];
                        if((s.timing[i]&2)&&s.rgb[i]!=full.rgb[i])++differences[s.stage[i]];
                    }
                }
                bool failed=s.tuple_bad||s.byte_bad||s.mask_bad||s.mixer_bad||timing_bad||!s.mixer_checks;
                for(unsigned k=0;k<stages;++k)failed|=!s.ticks[k];
                failed|=!s.hs_edges[0]||!s.hs_edges[1]||s.hs_edges[2]!=0;
                failed|=!s.short_pulses||s.short_width_bad;
                // Type 1 has not reacquired after setup: hSyncReg is unarmed
                // throughout this short stage. R3=5 alone cannot promise SHIFT.
                // Keep shift coverage on the acquired type-0/Plus traces and
                // transport checks on every machine, without retiming the CPU.
                // Evidence: docs/investigations/video-boundary/b22-short-hsync-2026-09-23.md.
                // If type 1 later acquires, require coverage there too. Two
                // armed pulses leave fetch time after the first; hs4 history
                // can cancel shift and is a separate monitor-policy question.
                if(machine!=1||(s.armed_falls[1]>1&&!s.hs4_ticks[1]))failed|=!s.shift[1];
                failed|=s.hs_edges[3]<=s.hs_edges[0]||!s.blank[5]||!s.lit[0];
                // Only type0's five-character HS trace is the deliberate visible
                // byte-shift discriminator. Type1 may not yet have acquired;
                // the contract does not require all machines/traces to differ.
                if(mode==1&&machine==0)failed|=!differences[1]||differences[2]!=0;
                if(mode)failed|=!s.mask_checks;
                std::cout<<"B6 dynamic "<<(failed?"FAIL":"PASS")<<" machine="<<machine<<" mode="<<mode
                         <<" tuple="<<s.tuple_bad<<" byte="<<s.byte_bad<<" mask="<<s.mask_bad<<"/"<<s.mask_checks
                         <<" final_RGB="<<s.mixer_bad<<"/"<<s.mixer_checks<<" short_pulses="<<s.short_pulses<<" short_width_bad="<<s.short_width_bad<<" timing_vs_Full="<<timing_bad<<'\n';
                for(unsigned k=0;k<stages;++k)std::cout<<"  "<<names[k]<<" ticks="<<s.ticks[k]<<" hs_edges="<<s.hs_edges[k]<<" hs_high="<<s.hs_high[k]<<" blank="<<s.blank[k]<<" armed_falls="<<s.armed_falls[k]<<" hs4_ticks="<<s.hs4_ticks[k]<<" shift_fetches="<<s.shift[k]<<" lit="<<s.lit[k]<<" Full_Raw_different="<<differences[k]<<'\n';
                bad|=failed;
            }
        }
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}
    return bad?1:0;
}
