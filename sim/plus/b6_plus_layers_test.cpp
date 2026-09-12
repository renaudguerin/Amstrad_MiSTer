// Combined B6 boundary integration: executing CPU, SDRAM, ASIC scrolling and
// opaque sprites, colour conversion and the production native output chain.
#define B6_BOUNDARY_LIBRARY
#include "b6_video_boundary_test.cpp"
#include <deque>

namespace {
// Palette packing, sprite size/priority and scroll semantics are the fixture's
// inputs from docs/plus/references/asic-reference.md sections 5, 6 and 8.
// This is an output-boundary integration check, not a new ASIC timing model.
constexpr unsigned ink0=0x327, ink1=0xe91, border=0x4ad, sprite=0xc58;
unsigned expand(unsigned rgb) {
    return ((rgb>>8)*17<<16)|(((rgb>>4)&15)*17<<8)|((rgb&15)*17);
}
Program layers_program(unsigned scroll,bool sprites) {
    Program p=video_program(5);
    std::vector<uint8_t> extra;
    auto emit=[&](std::initializer_list<uint8_t> bytes) {extra.insert(extra.end(),bytes);};
    auto mem=[&](unsigned a,unsigned v) {emit({0x3e,uint8_t(v),0x32,uint8_t(a),uint8_t(a>>8)});};
    auto io=[&](unsigned a,unsigned v) {emit({1,uint8_t(a),uint8_t(a>>8),0x3e,uint8_t(v),0xed,0x79});};
    // R1=44 and R2=42: the second sprite crosses real force-blank and
    // display end. Coordinates remain below 768 (higher values are signed).
    io(0xbc00,1); io(0xbd00,44);
    io(0xbc00,2); io(0xbd00,42);
    io(0x7f00,0x82); // mode 2, ROM configuration retained
    auto palette=[&](unsigned index,unsigned rgb) {
        const unsigned grb=((rgb&0xf0)<<4)|((rgb>>8)<<4)|(rgb&15);
        mem(0x6400+index*2,grb&255); mem(0x6401+index*2,grb>>8);
    };
    palette(0,ink0); palette(1,ink1); palette(16,border); palette(21,sprite);
    // Real CPU fills both images through the same absolute-write instruction
    // path as the established harness; each source pixel occupies one byte.
    for(unsigned a=0x4000;a<0x4200;++a) mem(a,5);
    for(unsigned i=0;i<2;++i) {
        const unsigned x=i?700:100, base=0x6000+8*i;
        mem(base,x&255); mem(base+1,x>>8); mem(base+2,40); mem(base+3,0);
        mem(base+4,sprites?0x0d:0); // x4, y1: 64 by 16 dots
    }
    mem(0x6804,scroll); // SSCR: horizontal only, no border mask
    auto di=std::find(p.code.begin()+0x100,p.code.begin()+kLoopPc,uint8_t(0xf3));
    if(di==p.code.begin()+kLoopPc) throw AuditFailure("layers setup DI absent");
    // A separate cartridge setup area accommodates the explicit RAM writes.
    // Prime WZ before both immediate jumps (the inherited TV80 contract).
    const std::vector<uint8_t> jump{0xf3,0x3a,0,0x10,0xc3,0,0x10};
    std::copy(jump.begin(),jump.end(),di);
    emit({0x3a,0,8,0xc3,0,8});
    if(0x1000+extra.size()>p.code.size()) throw AuditFailure("layers setup overflow");
    std::copy(extra.begin(),extra.end(),p.code.begin()+0x1000);
    // Keep the measured CPU active on the register page, avoiding the
    // documented sprite-RAM access suppression while scoring solid images.
    for(unsigned i=0;i<kLoopLinks;++i) {
        p.code[kLoopPc+i*3+1]=0;
        p.code[kLoopPc+i*3+2]=0x68;
    }
    return p;
}

struct Layers {
    uint64_t byte_bad=0,word_bad=0,pixel_bad=0,color_bad=0,mixer_bad=0,sprite_bad=0;
    uint64_t dots=0,nonzero_words=0,asymmetric=0,scroll_changes=0,opaque=0;
    uint64_t blank_sprite=0,border_sprite=0,screen0=0,screen1=0,masked_color=0,mixed_color=0;
    std::vector<uint32_t> pixels;
    std::vector<uint32_t> final_pixels;
};
Layers layers_run(unsigned mode,unsigned scroll,bool sprites) {
    Harness h(true,0); h.verify_mutation(0);
    h.dut.production_clocking=1; h.dut.b6_mode=mode;
    const auto program=layers_program(scroll,sprites);
    for(unsigned a=0xc000;a<0x10000;a+=2) {
        h.store(0,0x20000+a,uint8_t(0x93^((a>>1)&0x7f)));
        h.store(0,0x20001+a,uint8_t(0x6c^((a>>3)&0x7f)));
    }
    h.initialize(program,true);
    h.download(build_cpr_image({{"cb00",program.code},{"cb03",std::vector<uint8_t>(16384,0)}}));
    h.wait_for_reset_release(); h.run(1900000);
    if(h.dut.b6_plus_scroll!=scroll || h.dut.dbg_video_modeq!=2)
        throw AuditFailure("CPU did not establish requested scroll/mode");
    Layers r;
    uint8_t history=0,even=0,odd=0;
    bool history_valid=false,even_valid=false,odd_valid=false;
    std::deque<unsigned> pens;
    // Expected values advance independently through each documented register.
    // GAMMA=1 disabled still has an input pixel register and output register;
    // mixer then samples gamma RGB each master edge, emitting on prior CE.
    unsigned rendered=0,converted=0,gamma_input=0,gamma_output=0,mixer_input=0,final_rgb=0;
    bool rendered_valid=false,converted_valid=false,gi_valid=false,go_valid=false,mi_valid=false,final_valid=false;
    bool raw_tag=false;
    for(unsigned n=0;n<1500000;++n) {
        const auto& d=h.dut;
        const bool ce=d.dbg_video_ce16;
        const bool fetch=d.b6_cpu_n&&!d.b6_ras_n&&!d.b6_cas_n;
        const unsigned word=d.dbg_video_vram_word;
        const bool high=d.b6_bs,shift=d.b6_shift;
        const unsigned expected_byte=mode==0&&shift?(high?word&255:history):(high?word>>8:word&255);
        const bool byte_valid=!(mode==0&&shift&&!high&&!history_valid);
        if(fetch&&mode==0&&shift&&high) {history=d.dbg_raw_de?word>>8:0;history_valid=true;}
        const unsigned oldword=d.dbg_video_plus_vidword, oldbyte=d.dbg_video_vram_byte;
        unsigned assembled=oldword;
        if(d.dbg_video_cclk_p) assembled=(assembled&0xff00)|oldbyte;
        if(d.dbg_video_cclk_n) assembled=(assembled&255)|(oldbyte<<8);
        unsigned expected_pixel=rendered,expected_color=converted;
        bool pixel_valid=rendered_valid,color_valid=converted_valid;
        const bool output_enable=d.b6_mixer_ce;
        if(output_enable) {final_rgb=mixer_input;final_valid=mi_valid;}
        mixer_input=gamma_output; mi_valid=go_valid;
        if(ce) {
            gamma_output=gamma_input; go_valid=gi_valid;
            gamma_input=converted; gi_valid=converted_valid;
            expected_color=raw_tag&&mode?0:expand(rendered);
            color_valid=rendered_valid;
            raw_tag=d.b6_raw_vblank;
            const unsigned dot=d.dbg_video_pixcnt;
            // Mode-2 byte layout: MSB first, one dot per bit, eight dots
            // per byte (asic-reference §7 / Grimware Byte/Pixel structure).
            const unsigned pen=((dot<8?even:odd)>>(7-dot%8))&1;
            const bool pen_valid=dot<8?even_valid:odd_valid;
            const bool delayed_valid=pens.size()>=scroll;
            const unsigned delayed=scroll&&delayed_valid?pens[pens.size()-scroll]:pen;
            pens.push_back(pen);
            if(pens.size()>16) pens.pop_front();
            const bool de=d.b6_plus_de, blank=d.b6_plus_hsync;
            const unsigned x=d.b6_plus_hp,y=d.b6_plus_line;
            const bool spr=sprites&&y>=40&&y<56&&((x>=100&&x<164)||(x>=700&&x<764));
            // Independent programmed palette/coordinate oracle, never SPR_RGB
            // or the renderer's PEN/PAL_ADDR outputs as expected values.
            expected_pixel=blank?0:!de?border:spr?sprite:delayed?ink1:ink0;
            pixel_valid=pen_valid&&delayed_valid&&n>128;
            if(pixel_valid) {
                ++r.dots;
                if ((d.b6_plus_sprite_en!=spr || (spr&&d.b6_plus_sprite_rgb!=sprite)) && r.sprite_bad<8)
                    std::cerr<<"SPR x="<<x<<" y="<<y<<" expected="<<spr<<" actual="<<unsigned(d.b6_plus_sprite_en)<<" rgb="<<std::hex<<d.b6_plus_sprite_rgb<<std::dec<<'\n';
                r.sprite_bad+=d.b6_plus_sprite_en!=spr || (spr&&d.b6_plus_sprite_rgb!=sprite);
                r.scroll_changes+=de&&!blank&&!spr&&pen!=delayed;
                r.opaque+=de&&!blank&&spr;
                r.blank_sprite+=blank&&spr;
                r.border_sprite+=!blank&&!de&&spr;
                r.screen0+=de&&!blank&&!spr&&!delayed;
                r.screen1+=de&&!blank&&!spr&&delayed;
                r.masked_color+=mode&&d.b6_pixel_vblank&&rendered;
            }
            // Input latches consume the OLD assembled word on this edge.
            if(d.dbg_video_cclk_n) {even=oldword&255;even_valid=true;}
            else if(dot==7) {odd=oldword>>8;odd_valid=true;}
            rendered=expected_pixel;rendered_valid=pixel_valid;
            converted=expected_color;converted_valid=color_valid;
        }
        h.tick();
        if(fetch) {
            r.byte_bad+=byte_valid&&h.dut.dbg_video_vram_byte!=expected_byte;
            r.nonzero_words+=word!=0;
            r.asymmetric+=(word&255)!=(word>>8);
        }
        r.word_bad+=h.dut.dbg_video_plus_vidword!=assembled;
        if(pixel_valid) {
            if (h.dut.dbg_video_rgb!=expected_pixel && r.pixel_bad<8)
                std::cerr<<"PIX expected="<<std::hex<<expected_pixel<<" actual="<<h.dut.dbg_video_rgb<<std::dec<<" x="<<h.dut.b6_plus_hp<<" y="<<h.dut.b6_plus_line<<'\n';
            r.pixel_bad+=h.dut.dbg_video_rgb!=expected_pixel;
            if(ce) r.pixels.push_back(h.dut.dbg_video_rgb);
        }
        if(color_valid) r.color_bad+=h.dut.b6_color_rgb!=expected_color;
        if(final_valid) {
            r.mixer_bad+=h.dut.b6_mixer_rgb!=final_rgb;
            if(output_enable) {
                r.mixed_color+=h.dut.b6_mixer_rgb!=0;
                r.final_pixels.push_back(h.dut.b6_mixer_rgb);
            }
        }
    }
    return r;
}
bool errors(const Layers&r) {
    return r.byte_bad||r.word_bad||r.pixel_bad||r.color_bad||r.mixer_bad||r.sprite_bad;
}
bool empty_case(const Layers&r,unsigned mode) {
    return !r.nonzero_words||!r.asymmetric||!r.scroll_changes||!r.opaque||!r.blank_sprite||
           !r.border_sprite||!r.screen0||!r.screen1||!r.mixed_color||(mode&&!r.masked_color);
}
uint64_t differences(const std::vector<uint32_t>&a,const std::vector<uint32_t>&b) {
    if(a.size()!=b.size()) throw AuditFailure("control changed pixel sample count");
    uint64_t result=0;
    for(size_t i=0;i<a.size();++i) result+=a[i]!=b[i];
    return result;
}
void report(const Layers&r,unsigned mode,unsigned scroll,bool sprites) {
    std::cout<<"B6 Plus layers mode="<<mode<<" scroll="<<scroll<<" sprites="<<sprites
             <<" bad byte/word/pixel/color/mixer/sprite="<<r.byte_bad<<'/'<<r.word_bad<<'/'
             <<r.pixel_bad<<'/'<<r.color_bad<<'/'<<r.mixer_bad<<'/'<<r.sprite_bad
             <<" nonzero/asymmetric="<<r.nonzero_words<<'/'<<r.asymmetric
             <<" scroll_differences="<<r.scroll_changes<<" opaque="<<r.opaque
             <<" blank_sprite="<<r.blank_sprite<<" border_sprite="<<r.border_sprite
             <<" screen_pens="<<r.screen0<<'/'<<r.screen1
             <<" mask="<<r.masked_color<<" mixer_color="<<r.mixed_color<<std::endl;
}
}
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    try {
        bool bad=false;
        if(argc==3&&std::string(argv[1])=="--mode") {
            const unsigned mode=std::stoul(argv[2]);
            auto r=layers_run(mode,5,true); report(r,mode,5,true);
            return errors(r)||empty_case(r,mode);
        }
        Layers raw;
        for(unsigned mode=0;mode<3;++mode) {
            auto r=layers_run(mode,5,true); report(r,mode,5,true);
            bad|=errors(r)||empty_case(r,mode);
            if(mode==1) raw=r;
            if(mode==2) bad|=raw.pixels!=r.pixels||raw.final_pixels!=r.final_pixels;
        }
        for(bool disable_sprite:{false,true}) {
            auto r=layers_run(1,disable_sprite?5:0,!disable_sprite);
            report(r,1,disable_sprite?5:0,!disable_sprite);
            const auto pixels=differences(raw.pixels,r.pixels);
            const auto final=differences(raw.final_pixels,r.final_pixels);
            std::cout<<"B6 negative control "<<(disable_sprite?"sprites disabled":"scroll zero")
                     <<" changed renderer/final RGB samples="<<pixels<<'/'<<final<<std::endl;
            bad|=!pixels||!final||errors(r);
        }
        return bad?1:0;
    } catch(const std::exception&e) {std::cerr<<"B6 Plus layers: "<<e.what()<<'\n';return 2;}
}
