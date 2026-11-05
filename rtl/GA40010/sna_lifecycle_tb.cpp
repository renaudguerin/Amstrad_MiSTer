// Public GA SNA lifecycle regressions: settled restored mode and prefetch exclusion.
// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §9.1 p47 / §9.3.4 pp56-67.
// The restore invalidation/reacquisition policy is an engineering contract;
// expected fresh pixels use the independent fixed-mode bitplane rules.
#include "Vsna_lifecycle_top.h"
#include "verilated.h"
#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <stdexcept>
struct Sample{unsigned tick,rgb,req,active;bool ce,load,event;};
static unsigned restored_mode_contract(){unsigned failed=0;
 const std::array<unsigned,16>inks={0,4,6,31,1,2,3,5,7,8,10,11,12,13,14,15};
 const std::array<unsigned,16>rgb={21,1,13,62,63,9,43,35,47,33,40,42,32,34,44,46};
 for(unsigned target=0;target<4;target++)for(unsigned held:{1u,9u}){
  auto ctx=std::make_unique<VerilatedContext>();auto d=std::make_unique<Vsna_lifecycle_top>(ctx.get());
  unsigned tick=0,events=0;std::vector<Sample> samples;
  d->display=1;d->clk=0;d->reset_n=0;d->raw_hs=0;d->sna_load=0;d->sna_mode=0;d->ram_byte=0xb6;
  for(unsigned i=0;i<5;i++)d->palette[i]=0;
  for(unsigned i=0;i<16;i++)d->palette[i/4]|=inks[i]<<((i%4)*8);
  d->palette[4]=4;
  auto clock=[&](bool capture=false){d->clk=0;d->eval();bool ce=d->ce16,load=d->load,event=d->mode_event;d->clk=1;d->eval();if(capture){events+=event;samples.push_back({tick,unsigned(d->rgb),unsigned(d->requested),unsigned(d->active),ce,load,event});}d->clk=0;d->eval();++tick;ctx->timeInc(1);};
  for(unsigned i=0;i<64;i++)clock();d->reset_n=1;
  // Real input sync pulses retire startup GA vertical blank; then HS stays absent.
  for(unsigned n=0;n<32;n++){d->raw_hs=1;for(unsigned i=0;i<256;i++)clock();d->raw_hs=0;for(unsigned i=0;i<3840;i++)clock();}
  d->sna_load=1;d->sna_mode=0;clock();d->sna_load=0;
  for(unsigned i=0;i<128;i++)clock();
  if(d->hcnt!=30||d->active!=0)throw std::runtime_error("unqualified initial GA blank/mode");
  d->sna_mode=target;d->sna_load=1;for(unsigned i=0;i<held;i++)clock();d->sna_load=0;
  unsigned applied=tick;
  for(unsigned i=0;i<8192;i++)clock(true);
  std::string name="mode"+std::to_string(target)+"-held"+std::to_string(held);
  if(events||d->requested!=target||d->hcnt!=30)throw std::runtime_error("absent-HS/restore qualification failed");
  unsigned l=0;for(auto&s:samples)if(s.tick>applied+128&&s.ce&&s.load){l=s.tick;break;}
  if(!l)throw std::runtime_error("fresh real LOAD absent");
  unsigned start=l+(target==2?8:12),width=target==2?4:target==1?8:16,bad=0;
  for(unsigned i=0;i<192;i++){
   unsigned x=(0xb6<<((i%32)/width))&255;
   unsigned pen=((x>>7)&1)|(((x>>3)&1)<<1)|(((x>>5)&1)<<2)|(((x>>1)&1)<<3);
   if(target==1||target==3)pen&=3;else if(target==2)pen&=1;
   if(samples.at(start+i-applied).rgb!=rgb[pen])bad++;
  }
  // Restored settled mode is an engineering normalization, not a silicon reset claim.
  std::cout<<name<<" requested="<<unsigned(d->requested)<<" active_after_128us="<<unsigned(d->active)<<" mode_events="<<events<<" public_RGB_bad="<<bad<<"/192\n";
  if(bad)failed++;
 }
 std::cout<<"SNA_SETTLED_MODE_CONTRACT: "<<(failed?"MISMATCH":"PASS")<<" failing_cases="<<failed<<"/8; engineering contract\n";return failed;}

static unsigned fresh_byte_contract(){unsigned failed=0;
 const std::array<unsigned,16>inks={0,4,6,31,1,2,3,5,7,8,10,11,12,13,14,15};
 for(unsigned offset=0;offset<32;offset++)for(unsigned held:{1u,9u}){
  auto ctx=std::make_unique<VerilatedContext>();auto d=std::make_unique<Vsna_lifecycle_top>(ctx.get());
  d->display=1;d->clk=0;d->reset_n=0;d->raw_hs=0;d->sna_load=0;d->sna_mode=0;d->ram_byte=0xff;
  for(unsigned i=0;i<5;i++)d->palette[i]=0;
  for(unsigned i=0;i<16;i++)d->palette[i/4]|=inks[i]<<((i%4)*8);d->palette[4]=4;
  unsigned tick=0;
  auto clock=[&](){d->clk=0;d->eval();d->clk=1;d->eval();d->clk=0;d->eval();ctx->timeInc(1);tick++;};
  for(unsigned i=0;i<64;i++)clock();d->reset_n=1;
  for(unsigned n=0;n<32;n++){d->raw_hs=1;for(unsigned i=0;i<256;i++)clock();d->raw_hs=0;for(unsigned i=0;i<3840;i++)clock();}
  d->sna_load=1;clock();d->sna_load=0;for(unsigned i=0;i<128;i++)clock();
  if(d->hcnt!=30||d->requested!=0||d->active!=0||d->rgb!=46)throw std::runtime_error("old-word/public-mode/blank not qualified");
  for(unsigned i=0;!(d->ce16&&d->load);i++){if(i>128)throw std::runtime_error("real LOAD absent");clock();}
  for(unsigned i=0;i<offset;i++)clock();
  d->ram_byte=0;d->sna_load=1;
  for(unsigned i=0;i<held;i++){clock();if(d->rgb)throw std::runtime_error("apply failed to normalize black");}
  d->sna_load=0;unsigned begin=tick,leaks=0,newpixels=0,first=0,events=0,loads=0;
  std::string name="offset"+std::to_string(offset)+"-held"+std::to_string(held);
  for(unsigned i=0;i<512;i++){
   bool ce=d->ce16,load=d->load,event=d->mode_event;clock();events+=event;loads+=ce&&load;
   unsigned v=d->rgb;if(v==21)newpixels++;else if(v!=0){if(!leaks)first=i;leaks++;}
  }
  if(events||d->requested||d->active||d->hcnt!=30||newpixels<192||loads<8)throw std::runtime_error("post-apply no-HS/fresh-data recovery unqualified");
  std::cout<<name<<" stale_RGB_samples="<<leaks<<" first="<<first<<" fresh_samples="<<newpixels<<" actual_LOADs="<<loads<<" mode_events="<<events<<'\n';
  if(leaks)failed++;
 }
 std::cout<<"SNA_FRESH_BYTE_CONTRACT: "<<(failed?"MISMATCH":"PASS")<<" failing_cases="<<failed<<"/64\n";return failed;}


// French ACCC v1.11 §9.1 p47: each byte contributes8Pixel-M2;
// §17.1 p177: BORDER follows the complete displayed characters.
// The firstpixel origin LOAD+8/12 is the existing fixed-byte oracle above.
static unsigned display_edge_contract(){
 unsigned failed=0;
 const std::array<unsigned,16>inks={0,4,6,31,1,2,3,5,7,8,10,11,12,13,14,15};
 const std::array<unsigned,16>rgb={21,1,13,62,63,9,43,35,47,33,40,42,32,34,44,46};
 constexpr unsigned border_ink=18,border_rgb=8;
 struct E{unsigned tick,rgb,req,active;bool ce,load,prefetch,buf,valid,event;};
 for(unsigned mode=0;mode<4;++mode)for(unsigned initial:{0u,1u}){
  auto ctx=std::make_unique<VerilatedContext>();auto d=std::make_unique<Vsna_lifecycle_top>(ctx.get());
  unsigned tick=0;std::vector<E> samples;
  d->display=initial;d->clk=0;d->reset_n=0;d->raw_hs=0;d->sna_load=0;d->sna_mode=0;d->ram_byte=0xff;
  for(unsigned i=0;i<5;i++)d->palette[i]=0;
  for(unsigned i=0;i<16;i++)d->palette[i/4]|=inks[i]<<((i%4)*8);d->palette[4]=border_ink;
  auto clock=[&](bool capture=false){
   d->clk=0;d->eval();bool ce=d->ce16,load=d->load,pref=d->prefetch,buf=d->buffered_display,valid=d->buffer_valid,event=d->mode_event;
   d->clk=1;d->eval();if(capture)samples.push_back({tick,unsigned(d->rgb),unsigned(d->requested),unsigned(d->active),ce,load,pref,buf,valid,event});
   d->clk=0;d->eval();++tick;ctx->timeInc(1);
  };
  for(unsigned i=0;i<64;i++)clock();d->reset_n=1;
  for(unsigned n=0;n<32;n++){d->raw_hs=1;for(unsigned i=0;i<256;i++)clock();d->raw_hs=0;for(unsigned i=0;i<3840;i++)clock();}
  d->sna_load=1;d->sna_mode=mode;clock();d->sna_load=0;
  for(unsigned i=0;i<256;i++)clock();
  const unsigned pen=mode==0?15:mode==2?1:3;
  const unsigned before=initial?rgb[pen]:border_rgb,after=initial?border_rgb:rgb[pen];
  if(d->requested!=mode||d->active!=mode||d->hcnt!=30||before==after||before==0||after==0)
   throw std::runtime_error("DEfixed mode/palette/blank setup guard");
  for(unsigned i=0;i<192;i++){clock();if(d->rgb!=before)throw std::runtime_error("DEfixed prefix lacks192stable masterclocks");}
  for(unsigned i=0;!(d->ce16&&d->load);++i){if(i>128)throw std::runtime_error("DE real LOAD absent");clock();}
  // One fixed public-input transition after a real LOAD, no phase search.
  clock(true);clock(true);d->display=!initial;const unsigned changed=tick;
  for(unsigned i=0;i<190;++i)clock(true);
  unsigned pref=0,load=0,events=0;
  for(const auto&s:samples){
   events+=s.event;
   if(s.tick>=changed&&s.prefetch&&!pref)pref=s.tick;
   if(pref&&s.tick>pref&&s.ce&&s.load&&!load){
    if(s.buf!=!initial||!s.valid)throw std::runtime_error("first DE LOAD has wrong incoming tag/validity");
    load=s.tick;
   }
   if(s.req!=mode||s.active!=mode)throw std::runtime_error("DE mode changed inside fixed window");
  }
  if(!pref||!load||events||load>=changed+128||d->hcnt!=30)throw std::runtime_error("DE incoming-prefetch/LOAD qualification failed");
  const unsigned expected=load+(mode==2?8:12);unsigned bad=0,first=0,observed=0;
  for(const auto&s:samples){
   const unsigned want=s.tick<expected?before:after;
   if(s.rgb!=want){if(!bad)first=s.tick;bad++;}
   if(!observed&&s.tick>=changed&&s.rgb==after)observed=s.tick;
  }
  std::cout<<"DISPLAY_EDGE mode="<<mode<<" direction="<<(initial?"exit":"entry")<<" prefetch="<<pref<<" LOAD="<<load
           <<" expected_public_edge="<<expected<<" observed_public_edge="<<observed<<" before="<<before<<" after="<<after
           <<" first_bad="<<first<<" RGB_bad="<<bad<<"/"<<samples.size()<<" actualMode="<<unsigned(d->active)<<"\n";
  if(bad)failed++;
 }
 std::cout<<"DISPLAY_EDGE_CONTRACT: "<<(failed?"MISMATCH":"PASS")<<" failing_cases="<<failed<<"/8\n";return failed;
}

int main(int argc,char**argv){Verilated::commandArgs(argc,argv);if(argc==2&&std::string(argv[1])=="--display-edge-only")return display_edge_contract()?1:0;unsigned failed=restored_mode_contract();failed+=fresh_byte_contract();failed+=display_edge_contract();return failed?1:0;}
