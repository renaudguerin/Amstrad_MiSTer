// Observable boundary proof. No receiver recurrence supplies expectations.
#include "Vcrt_response.h"
#include "verilated.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <set>
#include <vector>

static void require(bool ok, const char* why, int t) {
    if (!ok) { std::fprintf(stderr,"FAIL t=%d: %s\n",t,why); std::exit(1); }
}
static unsigned tuple(const Vcrt_response& d) {
    return (d.hs_out<<5)|(d.vs_out<<4)|(d.hblank_out<<3)|
           (d.vblank_out<<2)|(d.field_out<<1)|d.frame_field_out;
}
struct Sample { unsigned rgb, tag, timing; };
// Rich transport uses unsupported short acquisition lines: receiver response must
// remain naturally zero. Adjacent words and line crossings have distinct RGB12
// and tag values, detecting address/owner errors hidden by a two-colour edge.
static void rich_transport() {
    Vcrt_response d;
    std::vector<Sample> input;
    unsigned rng=0x83126ad9, rgb=0, tag=0;
    int fill=0, checks=0;
    bool old_owner=false;
    for(int t=0;t<20000;t++) {
        bool ce=t%4==0, owner=t>=11000;
        bool reset=t<16 || (t>=14000 && t<14016);
        bool clear=t>=6000 && t<6005;
        if(ce) { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; rgb=rng&4095; tag=(rng>>17)&1; }
        unsigned timing=(((t%128)<12)<<5)|(((t/199)&1)<<4)|
            (((t%128)<40)<<3)|(((t/113)&1)<<2)|(((t/71)&1)<<1)|((t/97)&1);
        input.push_back({rgb,tag,timing});
        d.clk=0; d.ce_16=ce; d.reset=reset; d.context_clear=clear; d.owner=owner;
        d.ga_hs=(t%128)>=48 && (t%128)<80; d.ga_vs=0; d.source_missing=0;
        d.rgb12=rgb; d.raw_vblank=tag; d.hs=(timing>>5)&1; d.vs=(timing>>4)&1;
        d.hblank=(timing>>3)&1; d.vblank=(timing>>2)&1;
        d.field=(timing>>1)&1; d.frame_field=timing&1;
        d.eval(); d.clk=1; d.eval();
        bool invalidate=reset || clear || owner!=old_owner;
        old_owner=owner;
        if(invalidate) {
            fill=0;
            require(!d.available && d.raw_vblank_out && d.rgb12_out==0,
                    "held reset/context/owner must blank and invalidate RGB history",t);
        } else if(ce) {
            require(bool(d.available)==(fill>=64),"refill uses pre-write history availability",t);
            if(fill>=64) {
                require(d.rgb12_out==input[t-256].rgb,"tagged PRNG RGB12 continuous/short-line transport",t);
                require(d.raw_vblank_out==input[t-256].tag,"tagged PRNG blank coincidence",t);
                checks++;
            } else require(d.raw_vblank_out && d.rgb12_out==0,"refill samples explicitly blank",t);
            if(fill<128) fill++;
        }
        if(t>=256) require(tuple(d)==input[t-256].timing,"tuple continues through lifecycle invalidation",t);
    }
    std::printf("PASS rich transport: %d PRNG RGB/tag samples across short lines, reset/mode and synthetic owner change\n",checks);
}
static void priming_owner_check() {
    Vcrt_response control, d;
    constexpr int L=1024, prime=8*L+4;
    for(int t=0;t<24*L;t++) {
        int x=(t-4+L)%L;
        for(auto* p:{&control,&d}) {
            p->clk=0; p->ce_16=t%4==0; p->reset=t<4; p->context_clear=0;
            p->owner=0; p->source_missing=0; p->hs=t>=4 && x<16;
            p->vs=0; p->hblank=x<64; p->vblank=0; p->field=0; p->frame_field=0;
            p->rgb12=(t/4)%4095; p->raw_vblank=0; p->ga_vs=0;
            p->ga_hs=(x>=512 && x<768) || (p==&d && t>=prime-100 && t<prime+100);
            p->eval(); p->clk=1; p->eval();
        }
        if(t>=prime+256 && t%4==0)
            require(control.rgb12_out==d.rgb12_out,"open tip at priming must retain prior-window ownership",t);
    }
    std::puts("PASS priming: prior-window open tip cannot leak into first admitted window");
}
static void capacity_deadline_check() {
    // Independent worst-capacity chronology: older/newer deadlines are257
    // clocks apart; old-owned width256 falls two clocks before the new one.
    // Both results precede HS17. At the -192 origin the old-window late
    // target clips to absolute zero; leaking the tip into the new window
    // would instead create a spurious negative phase after origin recovery.
    constexpr int L=641, rise=15*L+68+192+321-1;
    Vcrt_response a,b;
    auto shift=[](int r) { return r==14 ? 192 : (r==16 ? -192 : 0); };
    int checks=0;
    for(int t=0;t<24*L;t++) {
        bool hs=false;
        for(int r=0;r<24;r++) if(t>=r*L+4+shift(r) && t<r*L+20+shift(r)) hs=true;
        int r=t/L, x=t%L;
        for(auto* d:{&a,&b}) {
            d->clk=0; d->ce_16=t%4==0; d->reset=t<4;
            d->context_clear=0; d->owner=0; d->source_missing=0;
            d->hs=hs; d->vs=0; d->hblank=1; d->vblank=0; d->field=0; d->frame_field=0;
            d->rgb12=(t/4)%4095; d->raw_vblank=0; d->ga_vs=0;
            // Omit overlapping ordinary line16 tip in both streams.
            d->ga_hs=(r!=16 && x>=68 && x<324) || (d==&b && t>=rise && t<rise+256);
            d->eval(); d->clk=1; d->eval();
        }
        if(t%4==0 && t>=17*L+4+256 && t<18*L+4+256) {
            require(b.rgb12_out==a.rgb12_out && b.rgb12_out==((t-256)/4)%4095,
                    "two-record old-owner crossing must survive257-clock deadline spacing",t);
            checks++;
        }
    }
    require(checks>100,"capacity proof nonvacuous",0);
    std::puts("PASS capacity: L641, coordinates+192/0/-192, crossing old owner, two-clock completion margin");
}
static void sprite_transport() {
    Vcrt_response d;
    std::vector<Sample> input;
    int enabled=0, transported=0, before=0, after=0, fill=0;
    bool previous_owner=0;
    for(int t=0;t<28*4096;t++) {
        int x=t%4096;
        d.clk=0; d.ce_16=t%4==0; d.reset=t<16; d.context_clear=0;
        d.owner=!(t>=14*4096 && t<18*4096); d.source_missing=0; d.ga_hs=x>=512 && x<768; d.ga_vs=0;
        d.hs=x<32; d.vs=0; d.hblank=x<384; d.vblank=0;
        d.field=(t/4096)&1; d.frame_field=(t/8192)&1;
        d.rgb12=(t/4)%4095; d.raw_vblank=(t/4)&1;
        d.eval();
        input.push_back({d.fixture_rgb,unsigned(d.raw_vblank),unsigned(d.fixture_sprite)});
        if(d.ce_16 && d.fixture_sprite) {
            require(d.fixture_rgb==0x214,"real enabled sprite preserves all RGB12 palette channels",t);
            enabled++;
            if(t<14*4096) before++;
            if(t>=18*4096) after++;
        }
        d.clk=1; d.eval();
        if(d.reset || bool(d.owner)!=previous_owner) {
            fill=0;
            require(!d.available && d.raw_vblank_out,"active sprite owner switch invalidates history",t);
        } else if(d.ce_16) {
            require(bool(d.available)==(fill>=64),"active sprite owner switch refill",t);
            if(fill>=64) {
                require(d.rgb12_out==input[t-256].rgb,"real sprite leaf/test compositor fixed RGB transport",t);
                require(d.raw_vblank_out==input[t-256].tag,"sprite color carries coincident independent blank tag",t);
                if(input[t-256].timing) transported++;
            }
            fill++;
        }
        previous_owner=d.owner;
    }
    require(enabled>=128 && transported>=128 && before>=32 && after>=32,"enabled sprite transport before/after live owner switch must be nonvacuous",0);
    std::printf("PASS Plus leaf: %d enabled real sprite pixels through test compositor and RGB/tag history\n",transported);
}
static void long_period_alias_check() {
    Vcrt_response d;
    constexpr int L=65536+4096;
    for(int t=0;t<11*L;t++) {
        int x=t%L;
        d.clk=0; d.ce_16=t%4==0; d.reset=t<16; d.context_clear=0; d.owner=0;
        d.source_missing=0; d.hs=x>=4 && x<20; d.vs=0;
        d.hblank=1; d.vblank=0; d.field=0; d.frame_field=0;
        d.ga_hs=x>=516 && x<649; d.ga_vs=0;
        d.rgb12=(t/4)%4095; d.raw_vblank=0;
        d.eval(); d.clk=1; d.eval();
        if(t>272 && t%4==0)
            require(d.available && d.rgb12_out==((t-256)/4)%4095,
                    "unsupported long period cannot alias healthy period across timestamp rollover",t);
    }
    std::puts("PASS long period: 65536+4096 cannot prime through modular alias");
}
static void strictly_prior_check() {
    Vcrt_response d;
    constexpr int L=1024;
    int same_tick=0, subsequent=0;
    for(int t=0;t<13*L;t++) {
        int x=(t-4+L)%L;
        d.clk=0; d.ce_16=t%4==0; d.reset=t<4; d.context_clear=0; d.owner=0;
        d.source_missing=0; d.hs=t>=4 && x<16; d.vs=0;
        d.hblank=1; d.vblank=0; d.field=0; d.frame_field=0;
        d.ga_hs=x>=512 && x<645; d.ga_vs=0;
        d.rgb12=(t/4)%4095; d.raw_vblank=0;
        d.eval(); d.clk=1; d.eval();
        // Primed at HS8, the first window closes exactly at HS9. That
        // result cannot affect HS9's pixels; HS10 must first show movement.
        if(t%4==0 && t>=9*L+4+256 && t<10*L+4+256) {
            require(d.rgb12_out==((t-256)/4)%4095,"same-HS completion is not a strictly prior result",t);
            same_tick++;
        }
        if(t%4==0 && t>=10*L+4+256 && t<11*L+4+256) {
            require(d.rgb12_out<((t-256)/4)%4095,"prior completed result reaches next eligible HS",t);
            subsequent++;
        }
    }
    require(same_tick>200 && subsequent>200,"strict-prior proof nonvacuous",0);
    std::puts("PASS strict-prior: equal-clock finalization waits for subsequent acquisition HS");
}
struct Case {
    int width=256, extra=0, shift=0, lifecycle=0;
    bool move_ga=false;
};
static std::array<int,104> observable_case(const Case& c) {
    Vcrt_response d;
    constexpr int L=4096;
    std::array<int,104> edges{}; edges.fill(-1);
    auto origin=[&](int r) { return r*L + (r>=24 ? c.shift : 0); };
    int acquired=0;
    for(int t=0;t<104*L;t++) {
        while(acquired<103 && t>=origin(acquired+1)) acquired++;
        int x=t-origin(acquired), row=acquired;
        int gr=t/L, gx=t%L;
        if(c.move_ga && gr>=24) gx-=c.shift;
        bool band=gr>=24 && gr<48;
        bool secondary=band && ((c.extra==1 && gx>=2368 && gx<2624) ||
            (c.extra==2 && gx>=3584 && gx<3840) ||
            (c.extra==3 && gx>=2368 && gx<2375) ||
            (c.extra==4 && gx>=2368 && gx<2625) ||
            (c.extra==5 && gx>=2368 && gx<2624));
        d.clk=0; d.ce_16=t%4==0; d.reset=t<16 || (c.lifecycle==1 && row==48 && x<320);
        d.context_clear=c.lifecycle==2 && row==48 && x<4;
        d.owner=c.lifecycle==3 && row>=48;
        d.source_missing=c.lifecycle==4 && row>=48 && row<56;
        d.hs=x<32; d.vs=0; d.hblank=x<384 || x>=3600; d.vblank=0;
        d.field=(row/3)&1; d.frame_field=(row/5)&1;
        d.rgb12=x<1600 ? 0x123 : 0xdea; d.raw_vblank=0;
        d.ga_hs=(gx>=512 && gx<512+c.width && !(c.extra==6 && band)) || secondary;
        d.ga_vs=c.extra==5 && band && gx>=2400 && gx<2700;
        d.eval(); d.clk=1; d.eval();
        if((d.reset || d.context_clear) && t>=16)
            require(!d.available && d.raw_vblank_out,"primed lifecycle clears RGB availability/tag",t);
        int src=t-256;
        if(t%4==0 && src>=0) {
            int r=src/L;
            while(r>0 && src<origin(r)) r--;
            while(r<103 && src>=origin(r+1)) r++;
            int dot=(src-origin(r))/4;
            if(dot>=300 && dot<500 && d.available && d.rgb12_out==0xdea && edges[r]<0) edges[r]=dot;
        }
    }
    return edges;
}
static void receiver_controls() {
    // Measured ordinary widths, independently derived midpoint positions:
    // (133-256)/8=-15.375 dots, (197-256)/8=-7.375; edge moves right.
    for(auto pair:{std::pair<int,int>{133,415},{197,407},{256,400}}) {
        Case c; c.width=pair.first; auto e=observable_case(c);
        for(int r=88;r<100;r++) require(e[r]==pair.second,"ordinary width midpoint / timestamp rollover",r*4096);
    }
    Case early; early.extra=2; auto e=observable_case(early);
    require(e[40]>400 && e[26]<e[40],"early extras give graded opposite signed movement",40*4096);
    require(e[56]<e[47] && e[88]==400,"early-extra recovery",88*4096);
    // A seven-clock glitch, a cap+1 overlong tip and a VS-straddling tip
    // all have no qualified monitor observation; pin state must rearm low.
    for(int kind:{3,4,5}) {
        Case c; c.extra=kind; auto z=observable_case(c);
        for(int r=20;r<100;r++) require(z[r]==400,"glitch/overlong/VS tip rejection with low rearm",r*4096);
    }
    Case no_tip; no_tip.width=133; no_tip.extra=6; auto held=observable_case(no_tip);
    require(held[25]>400 && held[40]==held[25] && held[47]==held[25],"missing observations hold absolute sweep phase",47*4096);
    Case ruler; ruler.shift=128; auto fixed=observable_case(ruler);
    require(fixed[25]==432 && fixed[88]==432,"stationary absolute sweep keeps cumulative ruler correction",88*4096);
    ruler.move_ga=true; auto translated=observable_case(ruler);
    require(translated[25]>400 && translated[88]==400,"common ruler+monitor translation recovers once",88*4096);
    Case unsupported; unsupported.shift=193; auto native=observable_case(unsupported);
    require(native[25]==400 && native[88]==400,"unsupported ruler step re-primes at native position",25*4096);
    for(int kind:{1,2,3,4}) {
        Case c; c.width=133; c.lifecycle=kind; auto z=observable_case(c);
        require(z[40]==415,"lifecycle fixture must already be primed/displaced",40*4096);
        require(z[49]==400,"reset/mode/owner/source loss clears prior response",49*4096);
        require(z[88]==415,"lifecycle resumes healthy priming",88*4096);
    }
    std::puts("PASS receiver controls: ordinary widths, signed pulls, exact cap/rejection, ruler correction, primed lifecycle and rollover");
}
static void line_offset_clear_boundary() {
    constexpr int L=4096;
    for(int scenario:{0,1,2}) {
        Vcrt_response d;
        std::array<int,104> edges{}; edges.fill(-1);
        for(int t=0;t<104*L;t++) {
            int row=t/L, x=t%L;
            d.clk=0; d.ce_16=t%4==0; d.reset=t<16; d.context_clear=0; d.owner=0;
            // Case0: off-native missing flag changes during a displaced line.
            // Case1: missing begins exactly on HS (clear branch must own it).
            // Case2: HS stops after a one-clock-shifted final edge; watchdog
            // expires off native cadence, then acquisition resumes at HS90.
            int cut=80*L+(scenario==0 ? 1001 : 0);
            d.source_missing=scenario!=2 && t>=cut && t<82*L+1001;
            d.hs=scenario==2 ? ((row<80 || row>=90) ? x<32 : (row==80 && x>=1 && x<33)) : x<32;
            d.vs=0; d.hblank=x<384 || x>=3600; d.vblank=0;
            d.field=(row/3)&1; d.frame_field=(row/5)&1;
            d.ga_hs=x>=512 && x<645; d.ga_vs=0;
            d.rgb12=x<1600 ? 0x123 : 0xdea; d.raw_vblank=0;
            d.eval(); d.clk=1; d.eval();
            if(t%4==0 && t>272) {
                require(d.available,"receiver-only clear must preserve populated RGB history",t);
                require(!d.raw_vblank_out,"receiver-only clear preserves source tag history",t);
                int src=t-256, r=src/L, dot=(src%L)/4;
                if(dot>=300 && dot<500 && d.rgb12_out==0xdea && edges[r]<0) edges[r]=dot;
            }
        }
        require(edges[79]==415,"clear-boundary fixture must be settled and displaced",79*L);
        if(scenario==0) {
            require(edges[80]==415,"off-HS source absence must preserve entire emitted line offset",80*L);
            require(edges[81]==400,"next own HS commits native fallback after source absence",81*L);
        } else if(scenario==1) {
            require(edges[80]==400,"HS coincident with receiver clear commits native fallback",80*L);
        } else {
            require(edges[80]>400 && edges[85]==edges[80],"off-HS watchdog expiry must hold emitted line offset",85*L);
            require(edges[90]==400,"first resumed HS commits watchdog fallback",90*L);
        }
    }
    std::puts("PASS receiver clear: off-native absence/watchdog hold line offset until own HS, exact-HS clear commits fallback");
}
static int oracle(const char* path) {
    std::ifstream file(path,std::ios::binary);
    require(bool(file),"open oracle file",0);
    Vcrt_response d;
    uint32_t words[2]; int t=0;
    while(file.read(reinterpret_cast<char*>(words),sizeof(words))) {
        uint32_t p=words[0], expected=words[1];
        d.clk=0; d.ce_16=p&1; d.reset=(p>>1)&1; d.context_clear=(p>>2)&1;
        d.owner=(p>>3)&1; d.ga_hs=(p>>4)&1; d.ga_vs=(p>>5)&1;
        d.source_missing=(p>>6)&1; d.raw_vblank=(p>>7)&1;
        d.hs=(p>>8)&1; d.vs=(p>>9)&1; d.hblank=(p>>10)&1;
        d.vblank=(p>>11)&1; d.field=(p>>12)&1; d.frame_field=(p>>13)&1;
        d.rgb12=(p>>14)&4095;
        d.eval(); d.clk=1; d.eval();
        uint32_t actual=d.rgb12_out|(d.raw_vblank_out<<12)|(d.available<<13)|
            (d.hs_out<<14)|(d.vs_out<<15)|(d.hblank_out<<16)|(d.vblank_out<<17)|
            (d.field_out<<18)|(d.frame_field_out<<19);
        uint32_t mask=t>=256 ? 0xfffff : 0x3fff;
        if((actual&mask)!=(expected&mask)) {
            std::fprintf(stderr,"FAIL oracle t=%d inputs=%08x expected=%05x actual=%05x\n",t,p,expected,actual);
            return 1;
        }
        t++;
    }
    require(t>256 && file.eof(),"complete nonempty oracle records",t);
    std::printf("PASS oracle: %s %d master ticks, public RGB/tag/availability/tuple/fields\n",path,t);
    return 0;
}
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    if(argc==3 && std::string(argv[1])=="--oracle") return oracle(argv[2]);
    rich_transport();
    receiver_controls();
    priming_owner_check();
    capacity_deadline_check();
    sprite_transport();
    long_period_alias_check();
    strictly_prior_check();
    line_offset_clear_boundary();
    Vcrt_response control, band;
    constexpr int L=4096, delay=256, lines=104;
    std::vector<Sample> input;
    std::array<int,lines> edges{}; edges.fill(-1);
    int parity=0, tuple_checks=0;
    for(int t=0;t<lines*L;t++) {
        int row=t/L, x=t%L;
        // Stationary rich RGB12 edge, independent tag and field transitions.
        unsigned rgb=x<1600 ? 0x123 : 0xdea;
        unsigned tag=(row%7==3 && x>=2300 && x<2900);
        unsigned timing=((x<32)<<5)|((row>=90)<<4)|
            ((x<384 || x>=3600)<<3)|((row>=90)<<2)|
            (((row/3)&1)<<1)|((row/5)&1);
        input.push_back({rgb,tag,timing});
        bool disturbed=row>=24 && row<48;
        for(auto* d:{&control,&band}) {
            d->clk=0; d->ce_16=(t%4==0); d->reset=t<16;
            d->context_clear=0; d->owner=0; d->source_missing=0;
            d->rgb12=rgb; d->raw_vblank=tag;
            d->hs=(timing>>5)&1; d->vs=(timing>>4)&1;
            d->hblank=(timing>>3)&1; d->vblank=(timing>>2)&1;
            d->field=(timing>>1)&1; d->frame_field=timing&1;
            d->ga_vs=0;
            d->ga_hs=(x>=512 && x<768) ||
                (d==&band && disturbed && x>=2368 && x<2624);
            d->eval(); d->clk=1; d->eval();
        }
        // At output edge t the complete tuple must equal input edge t-256.
        if(t>=delay) {
            require(tuple(control)==input[t-delay].timing,"fixed tuple+both fields transport",t);
            require(tuple(band)==tuple(control),"GA stimulus must not change acquisition tuple",t);
            tuple_checks++;
        }
        if(t%4==0 && t>delay+16) {
            require(control.available,"natural zero response must refill",t);
            require(control.rgb12_out==input[t-delay].rgb,"zero-response RGB12 transport",t);
            require(control.raw_vblank_out==input[t-delay].tag,"zero-response source blank tag",t);
            parity++;
            int source=t-delay;
            int outrow=source/L, outx=(source%L)/4;
            if(outx>=300 && outx<500 && band.rgb12_out==0xdea && edges[outrow]<0)
                edges[outrow]=outx;
        }
    }
    std::printf("PASS transport: %d full-RGB12/tag samples, %d matched tuple+field clocks\n",parity,tuple_checks);
    for(int r:{20,24,25,26,30,40,47,48,50,56,72,88})
        std::printf("edge row %d = %d (baseline 400)\n",r,edges[r]);
    for(int r=18;r<23;r++) require(edges[r]==400,"undisturbed upper rows retain registration",r*L);
    std::set<int> moved;
    for(int r=25;r<48;r++) { require(edges[r]<=400,"late extra tips move edge left",r*L); moved.insert(edges[r]); }
    require(edges[40]<400,"late extra tips must move stationary RGB edge left",40*L);
    require(moved.size()>=3,"movement must develop over multiple lines",48*L);
    require(edges[56]>edges[47],"edge must recover after extra-tip band",56*L);
    require(edges[88]==400,"lower steady rows recover registration",88*L);
    std::puts("PASS crt-response: signed band movement, multi-line settling and recovery");
}
