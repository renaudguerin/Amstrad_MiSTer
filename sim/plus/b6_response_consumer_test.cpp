// V1 F1: RGB/acquisition metadata/FIELD share the real output consumer stage.
// FRAME_FIELD is scored separately at its actual motherboard output boundary.
// This zero-response transport case does not establish acquisition/priming or
// classic nonzero response; receiver acquisition is owned by the standalone test.
#define B6_BOUNDARY_LIBRARY
#include "b6_video_boundary_test.cpp"

namespace {
struct Consumer {
    unsigned rgb, meta, field;
    bool operator==(const Consumer& b) const {
        return rgb==b.rgb && meta==b.meta && field==b.field;
    }
};
struct Sample {
    Consumer output;
    unsigned motherboard_frame;
    bool native;
};
Sample sample(const Harness& h) {
    const auto& d=h.dut;
    return {{d.b6_consumer_rgb,d.b6_consumer_meta,d.b6_consumer_field},
            d.b6_motherboard_frame,bool(d.dbg_video_ce16)};
}
std::vector<Sample> capture(bool plus,unsigned mode) {
    Harness h(plus,1);
    h.verify_mutation(0);
    h.dut.production_clocking=1;
    h.dut.b6_mode=mode;
    auto program=video_program(6); // accepted zero-response 256-clock GA tip
    // Public CPU write: enable interlace to exercise the real field tags.
    const std::vector<uint8_t> key{1,0,0xbc,0x3e,8,0xed,0x79,1,0,0xbd,0x3e};
    auto at=std::search(program.code.begin(),program.code.end(),key.begin(),key.end());
    if(at==program.code.end()) throw AuditFailure("R8 setup absent");
    at[key.size()]=3;
    for(unsigned a=0xc000;a<0x10000;++a)
        h.store(0,0x20000+a,uint8_t(0x13^((a>>1)&0x7f)^((a&1)?0xe8:0)));
    h.initialize(program,plus);
    if(plus) {
        h.download(build_cpr_image({{"cb00",program.code},
                                   {"cb03",std::vector<uint8_t>(16384,0)}}));
        h.wait_for_reset_release();
    }
    h.run(1800000);
    if(h.dut.b6_applied!=mode) throw AuditFailure("private mode not applied");
    std::vector<Sample> trace;
    trace.reserve(4096*640);
    // Observe output taps before the gamma/mixer edge. FRAME_FIELD remains
    // at the motherboard output, matching the direct production VGA_F1 wire.
    for(unsigned n=0;n<4096*640;++n) {
        trace.push_back(sample(h));
        h.tick();
    }
    return trace;
}
}
int main(int argc,char** argv) {
    Verilated::commandArgs(argc,argv);
    try {
        for(bool plus:{false,true}) {
            const auto raw=capture(plus,1), response=capture(plus,3);
            uint64_t checks=0,colored=0,frame_checks=0;
            unsigned changed_meta=0,changed_field=0,changed_frame=0;
            // Frozen V1 consumer contract: 64-dot presentation plus one held
            // renderer dot = 260 clocks relative to Raw, at production native
            // samples. Both paths include colour conversion. Tuple storage is
            // 261 registers; this is not a claim for arbitrary launch phases.
            for(size_t n=1024;n<raw.size();++n) {
                if(!raw[n].native) continue;
                // Coverage is also native-sampled: equality at every such
                // sample proves response edges without a second edge counter.
                // Inter-sample glitches are outside this consumer contract.
                changed_meta|=raw[n].output.meta^raw[n-4].output.meta;
                changed_field|=raw[n].output.field^raw[n-4].output.field;
                changed_frame|=raw[n].motherboard_frame^raw[n-4].motherboard_frame;
                if(!response[n].native) throw AuditFailure("native cadence diverged");
                const auto& want=raw[n-260].output; const auto& got=response[n].output;
                if(!(got==want)) {
                    std::cerr<<"F1 consumer owner="<<plus<<" clock="<<n
                        <<" rgb="<<std::hex<<got.rgb<<"/"<<want.rgb
                        <<" meta="<<got.meta<<"/"<<want.meta
                        <<" field="<<got.field<<"/"<<want.field
                        <<std::dec<<'\n';
                    throw AuditFailure("combined consumer must share260-clock Raw lag");
                }
                // Separate boundary, no invented output-pipeline register:
                // compare the physical motherboard FRAME_FIELD export only.
                if(response[n].motherboard_frame!=raw[n-260].motherboard_frame)
                    throw AuditFailure("motherboard FRAME_FIELD must share sampled260-clock Raw lag");
                ++frame_checks;
                ++checks;colored+=got.rgb!=0;
            }
            if(changed_meta!=15 || changed_field!=1 || changed_frame!=1 || !colored)
                throw AuditFailure("sampled comparisons missed RGB, metadata or field edges");
            std::cout<<"PASS F1 full-board owner="<<plus<<" consumer="<<checks
                     <<" motherboard_frame="<<frame_checks<<" meta_edges="<<changed_meta
                     <<" field_edges="<<changed_field<<" frame_edges="<<changed_frame<<'\n';
        }
    } catch(const std::exception& e) {
        std::cerr<<"B6 response: "<<e.what()<<'\n';return 1;
    }
}
