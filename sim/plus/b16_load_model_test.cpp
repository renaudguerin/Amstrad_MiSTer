#include "Vb16_load_model_test_top.h"
#include "verilated.h"
#include <cstdlib>
#include <iostream>
#include <vector>
static void check(bool ok, const char* msg) {
 if (!ok) { std::cerr << "FAIL B16: " << msg << '\n'; std::exit(1); }
}
struct Bench {
 Vb16_load_model_test_top d;
 bool last_set = false;
 std::vector<uint64_t> requests;
 Bench() { d.status = 0; d.clk = 0; d.fn_toggle = 0; d.cpr_apply = 0; d.cpr_image_valid = 1;
  d.sna_download = 0; d.sna_header_wr = 0; d.sna_addr = 0; d.sna_data = 0;
  d.drain_busy = 0; tick(); }
 void tick() {
  d.clk = 0; d.eval();
  // Actual hps_io consumes only rising status_set, and Main echoes later.
  if (d.status_set && !last_set) requests.push_back(d.status_in);
  last_set = d.status_set;
  d.clk = 1; d.eval();
 }
 void ticks(int n=10) { while(n--) tick(); }
 void byte(int a, int v) { d.sna_addr=a; d.sna_data=v; d.sna_header_wr=1; tick(); d.sna_header_wr=0; }
 void start(int machine, int version=3) {
  d.sna_download=1; tick(); byte(0x10, version); byte(0x6d,machine);
 }
 void apply(int expected) {
  d.sna_download=0; d.drain_busy=1; ticks();
  d.drain_busy=0;
  bool loaded=false;
  for(int n=0;n<20;n++) {
   tick();
   if (!d.owner_reset_hold) check(d.plus_model==expected,"model must settle before owner reset releases");
   if (d.sna_load) { check(d.plus_model==expected,"model at CPU apply"); loaded=true; }
  }
  check(loaded && !d.sna_hold,"restore completes");
 }
};
int main(int argc,char**argv) {
 Verilated::commandArgs(argc,argv);
 { Bench b; b.d.cpr_image_valid=0; b.d.cpr_apply=1; b.tick(); b.d.cpr_apply=0; b.ticks();
   check(b.d.plus_model==0 && b.requests.empty(),"invalid CPR must preserve classic mode"); }
 // B16 contract: Off -> 6128+ before CPR reset countdown releases; no Main echo.
 for(int model=0;model<4;model++) {
  Bench b; b.d.status=(uint64_t(model)<<33) | (1ULL<<44) | (1ULL<<32) | (1ULL<<38) | 0x12341;
  b.tick(); b.d.cpr_apply=1; b.tick(); b.d.cpr_apply=0; b.ticks();
  check(b.d.plus_model==(model ? model:2),"CPR Off must select 6128+ without waiting for Main");
  if(!model) {
   check(b.requests.size()==1,"CPR publishes one update");
   check(b.requests.back()==((b.d.status | (2ULL<<33)) & ~((1ULL<<32)|(1ULL<<38)|1ULL)),"CPR preserves settings without replaying reset/save triggers");
   b.d.status=b.requests.back(); b.ticks();
   b.d.status=(b.d.status & ~(3ULL<<33)) | (3ULL<<33); b.ticks();
   check(b.d.plus_model==3,"manual selection works after echo");
  } else check(b.requests.empty(),"existing Plus model preserved without update");
 }
 // Header specification docs/specs/formats/Snapshot (.SNA) file format.md:
 // 6D types 4=6128+, 5=464+, 6=GX4000; zero/unknown retain menu policy.
 for(int initial=0;initial<4;initial++) for(int machine=0;machine<8;machine++) {
  Bench b; b.d.status=uint64_t(initial)<<33; b.tick(); b.start(machine);
  int expected=machine==4?2:machine==5?3:machine==6?1:initial;
  b.apply(expected);
  if(machine>=4 && machine<=6) {
   check(!b.requests.empty(),"Plus SNA publishes selection");
   check((b.requests.back()>>33 & 3)==unsigned(expected),"SNA publishes matching OSD model");
   b.ticks(100); check(b.d.plus_model==expected,"SNA model survives delayed echo");
  } else check(b.requests.empty(),"classic or unknown header leaves status alone");
 }
 // Repeated downloads must clear a prior header, and v1 reserved bytes cannot select Plus.
 { Bench b; b.start(4); b.d.sna_download=0; b.d.drain_busy=1; b.tick();
   b.start(0); b.apply(0); check(b.requests.empty(),"aborted Plus header cannot leak"); }
 { Bench b; b.start(6,1); b.apply(0); }
 { Bench b; b.start(6,2); b.apply(0); }
 // Fn held across a load must neither swallow the model request nor toggle twice.
 { Bench b; b.d.status=(1ULL<<32) | (1ULL<<45); b.d.fn_toggle=1; b.tick();
   b.d.cpr_apply=1; b.tick(); b.d.cpr_apply=0; b.ticks();
   check(b.d.plus_model==2,"Fn collision keeps immediate Plus selection");
   check(b.requests.size()==1,"publisher waits for Main before a second update");
   b.d.status=b.requests.front(); b.ticks();
   bool saw_model=false;
   for(auto r:b.requests) { if((r>>33 & 3)==2) saw_model=true; check(r & (1ULL<<20),"model request preserves pending Fn toggle"); check(!(r & (1ULL<<32)),"pending reset clear preserved"); }
   check(saw_model && b.requests.size()<=2,"edge-triggered HPS receives model without repeated held-key updates"); }
 // Two restores before Main acknowledges: a return to the original model
 // must not be mistaken for an acknowledgement of the newest request.
 { Bench b; b.d.status=1ULL<<33; b.tick(); b.start(4); b.apply(2);
   auto first=b.requests.back(); b.start(6); b.apply(1);
   check(b.requests.size()==1,"coalesce later load while previous request is outstanding");
   b.d.status=first; b.ticks();
   check(b.d.plus_model==1,"older echo must not override a newer SNA selection");
   check(b.requests.size()==2 && (b.requests.back()>>33 & 3)==1,"newest model published after old echo");
   b.d.status=b.requests.back(); b.ticks();
   b.d.status=3ULL<<33; b.ticks(); check(b.d.plus_model==3,"override retires after final echo"); }
 // The same equality trap applies to two F1 presses before the first echo.
 { Bench b; b.d.fn_toggle=1; b.tick(); b.d.fn_toggle=0; b.ticks();
   auto first=b.requests.back(); b.d.fn_toggle=1; b.tick(); b.d.fn_toggle=0; b.ticks();
   b.d.status=first; b.ticks();
   check(b.requests.size()==2 && !(b.requests.back() & (1ULL<<20)),"second F1 toggle survives stale echo"); }
 std::cout << "B16 load-model tests PASS\n";
}
