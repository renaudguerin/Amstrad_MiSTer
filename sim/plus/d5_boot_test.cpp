#define main p10_unused_main
#include "p10_boot_test.cpp"
#undef main
// Exercise production model configuration through executed T80 ROM-select I/O.
// D5 handoff / Arnold V mapping: ROM7 stays disc on CPC models, GX maps it
// to BASIC; 128..255 directly select the low five bits on every Plus model.
void model_controls() {
    for (unsigned model : {1U, 2U, 3U}) {
        std::vector<unsigned> selects{0, 7};
        for (unsigned sel = 128; sel < 256; ++sel) selects.push_back(sel);
        std::vector<uint8_t> program{0xf3, 0x01, 0x00, 0xdf}; // DI; LD BC,DF00
        for (unsigned sel : selects) {
            for (uint8_t b : {uint8_t(0x3e), uint8_t(sel), uint8_t(0xed),
                              uint8_t(0x79), uint8_t(0x3a), uint8_t(0), uint8_t(0xc0)})
                program.push_back(b); // LD A,sel; OUT (C),A; LD A,(C000)
        }
        program.push_back(0x76); // HALT
        program.resize(16384, 0);
        std::vector<Chunk> chunks{{"cb00", program}};
        for (unsigned page = 1; page < 32; ++page) {
            char id[5]; std::snprintf(id, sizeof(id), "cb%02u", page);
            chunks.push_back({id, std::vector<uint8_t>(16384, uint8_t(page))});
        }
        Harness h;
        h.dut.d5_key = 0;
        h.dut.production_clocking = 1;
        h.dut.plus_model_i = model;
        h.initialize(); h.download(build_cpr_image(chunks)); wait_for_cpr_apply(h);
        size_t seen = 0; bool reading = false;
        for (unsigned tick = 0; tick < 2000000 && seen < selects.size(); ++tick) {
            h.tick();
            const bool read = !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n &&
                              h.dut.dbg_wait_n && h.dut.dbg_addr == 0xc000 &&
                              h.dut.dbg_cart_own && !h.dut.dbg_cart_stall;
            if (read && !reading) {
                const unsigned sel = selects[seen];
                // Both CPC Plus models require BASIC at ROM0.
                const unsigned expected = sel >= 128 ? (sel & 31) :
                    model == 1 ? 1 : sel == 7 ? 3 : 1;
                require(h.dut.d5_romsel == sel && h.dut.dbg_cart_own &&
                        h.dut.dbg_cart_page == expected,
                        "model/ROM control failed: model=" + std::to_string(model) +
                        " select=" + std::to_string(sel));
                ++seen;
            }
            reading = read;
        }
        require(seen == selects.size(), "timeout in model/ROM controls");
        std::cout << "PASS model=" << model << " ROM0/ROM7/all direct pages" << std::endl;
    }
}

int main(int argc,char **argv) {
 try {
  Verilated::commandArgs(argc,argv);
  if(argc == 2 && std::string(argv[1]) == "--controls") { model_controls(); return 0; }
  require(argc >= 2, "usage: d5_boot image.cpr [--classic-rom] [--464]");
  bool no_menu = false, model464 = false;
  for (int i = 2; i < argc; ++i) {
    const std::string option = argv[i];
    if (option == "--classic-rom") no_menu = true;
    else if (option == "--464") model464 = true;
    else throw TestFailure("unknown option: " + option);
  }
  const unsigned entry=no_menu?0xc1bc:0xc1b3, discpc=no_menu?0xc1dc:0xc1d3, errorpc=no_menu?0xc223:0xc21a;
  std::ifstream f(argv[1],std::ios::binary);
  require(bool(f),"CPR missing");
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
  Harness h; h.dut.d5_key=0; h.dut.production_clocking=1;
  h.dut.plus_model_i = model464 ? 3 : 2;
  h.initialize(); h.download(bytes); wait_for_cpr_apply(h);
  bool fetch=false,io=false,menu=false,disc=false,error=false,basic=false;
  uint64_t basicat=0; std::string output; bool ready=false,missing=false; uint64_t errorat=0; unsigned lastsel=256; unsigned rom_events=0, fetch_events=0;
  for(uint64_t n=0;n<320000000;n++) {
   h.tick();
   bool fi=!h.dut.dbg_m1_n&&!h.dut.dbg_mreq_n&&!h.dut.dbg_rd_n&&h.dut.dbg_wait_n;
   bool iow=!h.dut.dbg_iorq_n&&!h.dut.dbg_wr_n;
   if(iow&&!io&&! (h.dut.dbg_addr&0x2000) && h.dut.dbg_dout!=lastsel) {
    lastsel=h.dut.dbg_dout;
    if(rom_events++<100) std::cout<<"ROMSEL tick="<<std::dec<<n<<" value="<<std::hex<<lastsel<<" pc="<<h.dut.dbg_pc<<std::endl;
   }
   io=iow;
   if(fi&&!fetch) {
    unsigned a=h.dut.dbg_addr,p=h.dut.dbg_cart_page;
    // Firmware TXT OUTPUT jumpblock; CPU A is the actual character argument.
    if(a==0xbb5a) {
     unsigned ch=h.dut.d5_a;
     if(ch>=32&&ch<127) {output.push_back(char(ch)); std::cout<<"CHAR "<<char(ch)<<std::endl;}
     else if(ch==10||ch==13) output.push_back(' ');
     if(output.size()>512)output.erase(0,256);
     ready=output.find("Ready")!=std::string::npos;
     missing=output.find("missing")!=std::string::npos;
    }
    if(h.dut.dbg_cart_own && (a==entry||a==discpc||a==errorpc||a==0xce1b||a==0xc006||a==0xce06)) {
     if((a!=0xce06||!menu)&&fetch_events++<120) std::cout<<"FETCH tick="<<std::dec<<n<<" pc="<<std::hex<<a<<" page="<<p<<" romsel="<<unsigned(h.dut.d5_romsel)<<" byte="<<unsigned(h.dut.dbg_din)<<std::endl;
     if(!no_menu&&p==3&&a==0xce06&&!menu) {menu=true;h.dut.d5_key=0x605;}
     if(p==3&&a==discpc)disc=true;
     if(p==3&&a==errorpc&&!error){error=true;errorat=n;}
     if((menu||no_menu)&&a==0xc006&&h.dut.d5_romsel==0&&h.dut.dbg_din==0x31&&!basic) {basic=true;basicat=n;}
    }
   }
   fetch=fi;
   if(menu&&h.dut.dbg_pc==0xce1b)h.dut.d5_key=0x005;
   if(missing || ready || (basic&&n>basicat+32000000) || (error&&n>errorat+8000000))break;
   if(n%16000000==0)std::cout<<"PROGRESS tick="<<std::dec<<n<<" pc="<<std::hex<<h.dut.dbg_pc<<std::endl;
  }
  std::cout<<"RESULT menu="<<menu<<" basic_entry="<<basic<<" ready="<<ready<<" missing="<<missing<<" disc_branch="<<disc<<" error_branch="<<error<<" fdc_writes="<<std::dec<<h.fdc_writes.size()<<" pc="<<std::hex<<h.dut.dbg_pc<<std::endl;
  if(missing) throw TestFailure("FAIL: firmware emitted disc missing before Ready");
  if(!ready) throw TestFailure("FAIL: timeout without firmware Ready output");
  std::cout << "PASS: firmware Ready output" << std::endl;
  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<std::endl;return 1;}
}
