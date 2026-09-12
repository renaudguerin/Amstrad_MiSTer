// CPU writes disturb real CRTC/GA timing after acquisition. The SDRAM,
// renderer, color converter, gamma and mixer are production instances.
#define B6_BOUNDARY_LIBRARY
#include "b6_video_boundary_test.cpp"
#include <array>

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
        // A real CPU write labels the settled regime; no test drives CRTC state.
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
struct Samples {
    std::vector<uint8_t> timing;
    std::vector<uint32_t> rgb;
    std::vector<uint8_t> stage;
    std::array<uint64_t,stages> ticks{},hs_edges{},hs_high{},blank{},shift{},lit{};
    uint64_t tuple_bad=0,byte_bad=0,mask_bad=0,mask_checks=0,mixer_bad=0,mixer_checks=0;
};
Samples dynamic_run(unsigned machine,unsigned mode) {
    Harness h(machine==2,machine==1);
    h.verify_mutation(0);h.dut.production_clocking=1;h.dut.b6_mode=mode;
    for(unsigned a=0xc000;a<0x10000;a+=2) {
        h.store(0,0x20000+a,uint8_t(0x13^((a>>1)&0x7f)));
        h.store(0,0x20001+a,uint8_t(0xe8^((a>>3)&0x7f)));
    }
    const auto p=dynamic_program();h.initialize(p,machine==2);
    if(machine==2) {h.download(build_cpr_image({{"cb00",p.code},{"cb03",std::vector<uint8_t>(16384,0)}}));h.wait_for_reset_release();}
    Samples s;
    int stage=-1;unsigned age=0;bool old_hs=false;
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
        h.tick();++pipeline_age;
        if(stage>=0&&age++>20000) {
            ++s.ticks[stage];
            const bool hs=d.b6_raw_tuple&2;
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
    return s;
}
}
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    unsigned only=3;
    for(int i=1;i+1<argc;++i)if(std::string(argv[i])=="--machine")only=std::stoul(argv[++i]);
    bool bad=false;
    try {
        for(unsigned machine=0;machine<3;++machine) {
            if(only<3&&machine!=only)continue;
            Samples full;
            for(unsigned mode=0;mode<3;++mode) {
                auto s=dynamic_run(machine,mode);
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
                failed|=!s.hs_edges[0]||!s.hs_edges[1]||s.hs_edges[2]!=0||!s.shift[1];
                failed|=s.hs_edges[3]<=s.hs_edges[0]||!s.blank[5]||!s.lit[0];
                // Only type0's five-character HS trace is the deliberate visible
                // byte-shift discriminator. Type1 can shift outside display;
                // the contract does not require all machines/traces to differ.
                if(mode==1&&machine==0)failed|=!differences[1]||differences[2]!=0;
                if(mode)failed|=!s.mask_checks;
                std::cout<<"B6 dynamic "<<(failed?"FAIL":"PASS")<<" machine="<<machine<<" mode="<<mode
                         <<" tuple="<<s.tuple_bad<<" byte="<<s.byte_bad<<" mask="<<s.mask_bad<<"/"<<s.mask_checks
                         <<" final_RGB="<<s.mixer_bad<<"/"<<s.mixer_checks<<" timing_vs_Full="<<timing_bad<<'\n';
                for(unsigned k=0;k<stages;++k)std::cout<<"  "<<names[k]<<" ticks="<<s.ticks[k]<<" hs_edges="<<s.hs_edges[k]<<" hs_high="<<s.hs_high[k]<<" blank="<<s.blank[k]<<" shift_fetches="<<s.shift[k]<<" lit="<<s.lit[k]<<" Full_Raw_different="<<differences[k]<<'\n';
                bad|=failed;
            }
        }
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}
    return bad?1:0;
}
