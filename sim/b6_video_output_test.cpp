// Exercise the production output chain with retained processing settings and
// a real video_freak crop window. The source is a controlled timing tuple;
// real CRTC/GA/ASIC/SDRAM production is covered by b6_video_boundary_test.cpp.
#include "Vamstrad_video_output.h"
#include "verilated.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>

struct Bench {
    Vamstrad_video_output d;
    uint64_t clocks=0, native_checks=0, raw_windows=0, obs_checks=0;
    // B4 phase 2: check the observation tap the sample recorder consumes.
    // It is outputs only, so every assertion the displayed-path tests already
    // make still has to hold beside these.
    bool check_obs=false;
    Bench() {
        d.ce_16=0; d.raw_crt=0; d.pixel_vblank=0; d.plus_mode=1;
        d.mode=0; d.scale=0; d.ar=0; d.integer_scale=0; d.mix=0;
        d.r4=13; d.g4=7; d.b4=2; d.hs=0; d.vs=0; d.hbl=1; d.vbl=1;
        d.pixel_rate_select=0; d.forced_scandoubler=0; d.field_in=0;
        d.vcrop_en=1; d.HDMI_WIDTH=1920; d.HDMI_HEIGHT=1080;
        d.HDMI_FREEZE=0; d.progress_pix=0; d.gamma_bus=0;
    }
    void require(bool good, const char* why) {
        if(!good) throw std::runtime_error(why);
    }
    unsigned line(unsigned y, bool frame_sync=true, bool check_native=false) {
        unsigned acquired=0;
        for(unsigned x=0;x<4096;++x) {
            const unsigned dot=x/4;
            d.ce_16=(x%4==0);
            d.hbl=(dot>=768); d.hs=(dot>=800&&dot<864);
            d.vbl=frame_sync&&(y>=300);
            d.vs=frame_sync&&(y>=304&&y<307);
            d.field_in=frame_sync&&(y>=304);
            const bool sampled=check_obs&&d.ce_16;
            const unsigned p_hs=d.hs,p_vs=d.vs,p_hbl=d.hbl,p_vbl=d.vbl,p_field=d.field_in;
            d.CLK_VIDEO=0; d.eval(); d.CLK_VIDEO=1; d.eval(); ++clocks;
            if(check_obs) {
                require(d.obs_native_cadence==1,"supported settings flagged as non-native");
                require(d.obs_ce_pix==d.ce_16,"tap is not on the native dot enable");
            }
            if(sampled) {
                ++obs_checks;
                // color_mix registers the tuple beside the pixel it belongs
                // to, so the tap must show the values driven at this enable
                // and not a stage earlier or later.
                require(d.obs_hs==p_hs&&d.obs_vs==p_vs&&d.obs_hbl==p_hbl&&d.obs_vbl==p_vbl&&d.obs_field==p_field,
                        "tap sync and FIELD tuple is not aligned with its pixel");
                require(d.obs_r==0xdd&&d.obs_g==0x77&&d.obs_b==0x22,
                        "tap lost the converted 24-bit colour");
            }
            if(check_native) {
                ++native_checks;
                require(d.VGA_R==0xdd&&d.VGA_G==0x77&&d.VGA_B==0x22,
                        "Raw CRT lost final RGB through gamma/mixer");
                require(d.CE_PIXEL==d.ce_16,"Raw CRT lost native dot cadence");
                require(d.VGA_SL==0,"Raw CRT retained scanline effect");
            }
            if(d.CE_PIXEL&&d.VGA_DE) ++acquired;
        }
        if(check_native) {
            // Acquisition width is counted at the real output, after the
            // converter, gamma metadata, mixer and final crop selection.
            require(acquired==768,"Raw CRT retained crop or resampled DE");
            ++raw_windows;
        }
        return acquired;
    }
    void frame() {for(unsigned y=0;y<320;++y) line(y);}
};
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    try {
        Bench b;
        for(unsigned f=0;f<4;++f) b.frame();
        // The capture profile's settings: native dot rate, no HQ2x, and no
        // Raw CRT to obtain them. Two frames under the tap's own assertions.
        b.check_obs=true;
        for(unsigned f=0;f<2;++f) b.frame();
        b.check_obs=false;
        // A real 300-line input has acquired a 270-line crop. Count a whole
        // settled frame: this precondition prevents a bypass test passing
        // simply because the crop never became active.
        uint64_t cropped=0;
        for(unsigned y=0;y<320;++y) cropped+=b.line(y);
        b.require(cropped==270*768,"Full crop control did not acquire 270 lines");
        // Move past the retained crop's last line and then stop VSYNC. The
        // zero CROP_SIZE requested in Raw CRT cannot update video_freak's
        // retained vcrop until another VSYNC; only the actual DE bypass can
        // pass these otherwise-cropped lines.
        for(unsigned y=0;y<301;++y) b.line(y);
        b.require(b.line(301,false)==0,"stopped-VSYNC crop control not closed");
        b.d.raw_crt=1;
        b.d.pixel_rate_select=1; b.d.scale=1; b.d.forced_scandoubler=1;
        for(unsigned y=0;y<3;++y) b.line(y,false); // drain old pixel settings
        for(unsigned y=0;y<16;++y) b.line(y,false,true);
        b.d.scale=3; // independent conflicting scanline setting
        for(unsigned y=0;y<3;++y) b.line(y,false);
        for(unsigned y=0;y<16;++y) b.line(y,false,true);
        // Missing HSYNC as well must not silently choose a filtered or
        // resampled mode. DE recovery is not promised for missing HSYNC;
        // cadence and the scanline setting are still testable.
        for(unsigned n=0;n<4096;++n) {
            b.d.ce_16=(n%4==0); b.d.hs=0; b.d.vs=0; b.d.hbl=0; b.d.vbl=0;
            b.d.CLK_VIDEO=0;b.d.eval();b.d.CLK_VIDEO=1;b.d.eval();
            b.require(b.d.CE_PIXEL==b.d.ce_16&&b.d.VGA_SL==0,"sync loss changed Raw CRT policy");
        }
        // Leaving the mode restores the retained scanline selection. The
        // user's settings have never been overwritten by the output chain.
        b.d.raw_crt=0;b.d.eval();
        b.require(b.d.VGA_SL==3,"leaving Raw CRT lost retained scanlines");
        // Settings the capture profile does not support are reported as
        // unsupported rather than silently satisfied by Raw CRT, which also
        // changes acquisition geometry.
        b.d.scale=0;b.d.pixel_rate_select=1;b.d.raw_crt=0;b.d.eval();
        b.require(b.d.obs_native_cadence==0,"adaptive pixel rate accepted as native");
        b.d.pixel_rate_select=0;b.d.scale=0;b.d.raw_crt=1;b.d.eval();
        b.require(b.d.obs_native_cadence==0,"Raw CRT alone accepted as supported native profile");
        b.d.pixel_rate_select=0;b.d.raw_crt=0;b.d.scale=1;b.d.eval();
        b.require(b.d.obs_native_cadence==0,"HQ2x accepted as native");
        b.d.scale=0;b.d.eval();
        b.require(b.d.obs_native_cadence==1,"supported settings rejected");
        b.require(b.d.obs_applied_config==0x81,"applied config mismatch for native profile");

        // Test FIELD alignment with ce_pix / observation tap
        b.d.ce_16=0; b.d.field_in=0;
        b.d.CLK_VIDEO=0; b.d.eval(); b.d.CLK_VIDEO=1; b.d.eval();
        b.d.ce_16=1;
        b.d.CLK_VIDEO=0; b.d.eval(); b.d.CLK_VIDEO=1; b.d.eval();
        b.require(b.d.obs_field==0,"initial obs_field must be 0");
        // Change field_in when ce_pix/ce_16 is low
        b.d.ce_16=0; b.d.field_in=1;
        b.d.CLK_VIDEO=0; b.d.eval(); b.d.CLK_VIDEO=1; b.d.eval();
        b.require(b.d.obs_field==0,"obs_field changed without ce_pix");
        // Now assert ce_16
        b.d.ce_16=1;
        b.d.CLK_VIDEO=0; b.d.eval(); b.d.CLK_VIDEO=1; b.d.eval();
        b.require(b.d.obs_field==1,"obs_field did not register on ce_pix");
        std::cout<<"B6 output PASS: Full crop="<<cropped/768
                 <<" lines; Raw CRT native checks="<<b.native_checks
                 <<", uncropped 768-dot windows="<<b.raw_windows
                 <<", tap samples checked="<<b.obs_checks<<'\n';
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"B6 output FAIL: "<<e.what()<<'\n';return 1;
    }
}
