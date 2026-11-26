// The menu uses 0=Type 1, 1=Type 0, 2=Type 2 and 3=Type 4.
// The motherboard retains its original 0=Type 0, 1=Type 1 input values.
module classic_crtc_select (
    input        clk,
    input        reset_apply,
    input  [1:0] requested,
    input        plus_mode,
    input        sna_download_request,
    input        sna_hold,
    output [1:0] crtc_type,
    output       sna_unavailable,
    output       sna_download
);

reg [1:0] applied;
reg legacy_type1;
reg restore_legacy;

// A snapshot admitted in Plus may still complete after Plus is turned off.
// Disarm an applied experimental type and keep the last legacy type through
// the apply cycle. The experimental menu choice then waits for reset.
wire disarm_experimental = !plus_mode && applied[1] && sna_hold;
wire use_restore_legacy = disarm_experimental || restore_legacy;

always @(posedge clk) begin
    if (!requested[1] && !applied[1] && !restore_legacy)
        legacy_type1 <= !requested[0];
    if (disarm_experimental) applied <= 2'd0;
    else if (reset_apply && !sna_hold) applied <= requested;
    restore_legacy <= sna_hold && use_restore_legacy;
end

// Preserve live Type 0/1 changes and hold the last running type while an
// experimental choice waits for reset.
assign crtc_type = applied[1] && !disarm_experimental ? applied :
                   {1'b0, (requested[1] || use_restore_legacy) ? legacy_type1 : !requested[0]};

// Use the latched selection for admission. The admitted transfer itself
// drives sna_hold and can trigger fallback.
assign sna_unavailable = !plus_mode && applied[1];

// Decide once per upload. An external F6 transfer can arrive while the OSD
// entry is hidden. If the model changes mid-transfer, keep the original
// decision so the SNA parser never interprets a partial file as complete.
reg old_sna_download_request;
reg sna_admitted;
always @(posedge clk) begin
    old_sna_download_request <= sna_download_request;
    if (sna_download_request && !old_sna_download_request)
        sna_admitted <= !sna_unavailable;
end
assign sna_download = sna_download_request &&
                      (old_sna_download_request ? sna_admitted : !sna_unavailable);

initial begin
    applied = 2'd0;
    legacy_type1 = 1'b1;
    restore_legacy = 1'b0;
    old_sna_download_request = 1'b0;
    sna_admitted = 1'b0;
end

endmodule
