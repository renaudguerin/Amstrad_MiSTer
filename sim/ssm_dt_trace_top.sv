// Bounded synthetic DT M1/Avalon observer contract; no CPU model.
module ssm_dt_trace_top (
 input clk, reset, trace_enable, m1_fetch, qualified,
 input [15:0] fetch_addr,
 input [7:0] bus_data,
 input [127:0] snapshot,
 input busy,
 output [28:0] addr,
 output [63:0] data,
 output we,
 output [31:0] event_count,
 output [7:0] dropped_count,
 output [895:0] samples,
 output [223:0] sample_ticks,
 output frozen
);
 ssm_marker marker (
 .clk(clk), .reset(reset), .enable(1'b1),
 .m1_fetch(m1_fetch), .bus_data(bus_data), .sync_observation(21'h12345),
 .ce_pix(1'b0), .hsync(1'b0), .vsync(1'b0), .field(1'b0),
 .last_code(), .event_count(event_count), .dropped_count(dropped_count), .event_stb(),
 .ddram_addr(addr), .ddram_din(data), .ddram_be(), .ddram_burstcnt(),
 .ddram_we(we), .ddram_busy(busy)
`ifdef SSM_S_PASSIVE_TRACE
 , .s_trace_enable(trace_enable), .s_trace_addr(fetch_addr),
 .s_trace_qualified(qualified), .s_trace_snapshot(snapshot)
`endif
 );
 assign frozen=marker.s_frozen;
 genvar k;
 generate for(k=0;k<7;k=k+1)begin
  assign samples[128*k+:128]=marker.s_samples[k];
  assign sample_ticks[32*k+:32]=marker.s_ticks[k];
 end endgenerate
endmodule
