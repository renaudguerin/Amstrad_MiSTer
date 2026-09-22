// B16: select the running Plus model before load reset/apply releases and
// publish the same choice to Main. hps_io only latches a request on a rising
// status_set; status itself changes later when Main echoes that request.
// Keep changed fields locally until that echo, merging unrelated live OSD
// fields on every publication. F1 shares this publisher so a held key cannot
// hide a load request or undo a still-unacknowledged tape-sound toggle.
module plus_load_model (
 input clk,
 input [63:0] status,
 input fn_toggle,
 input cpr_apply,
 input cpr_image_valid,
 input sna_download,
 input sna_header_wr,
 input [7:0] sna_addr,
 input [7:0] sna_data,
 input sna_prepare,
 output [1:0] plus_model,
 output reg [63:0] status_in = 64'd0,
 output reg status_set = 1'b0
);
 reg [1:0] selected = 2'd0;
 reg model_pending = 1'b0;
 reg sound_pending = 1'b0;
 reg sound = 1'b0;
 reg publish_pending = 1'b0;
 reg request_in_flight = 1'b0;
 // Main echoes whole status words. Serialize publications so an older echo
 // cannot acknowledge a newer selection that happens to equal the old status.
 localparam [63:0] OWNED_FIELDS = (64'h3 << 33) | (64'h1 << 32) | (64'h1 << 20);
 reg old_fn = 1'b0;
 reg old_sna_download = 1'b0;
 reg [7:0] sna_version = 8'd0;
 reg [1:0] sna_plus_model = 2'd0;
 wire fn_edge = fn_toggle && !old_fn;
 wire select_sna = sna_prepare && !sna_download && (sna_version >= 8'd3) &&
                   (sna_plus_model != 2'd0);
 wire select_cpr = cpr_apply && cpr_image_valid && (plus_model == 2'd0);
 assign plus_model = model_pending ? selected : status[34:33];
 wire effective_sound = sound_pending ? sound : status[20];
 // Reset/apply, detach and snapshot-save are actions, not persisted settings.
 // Do not replay a momentary high value through a delayed Main echo.
 wire [63:0] effective_status = {status[63:35], plus_model,
                               1'b0,
                               status[31:21], effective_sound, status[19:0]} &
                               ~((64'h1 << 38) | 64'h1);
 always @(posedge clk) begin
  old_fn <= fn_toggle;
  old_sna_download <= sna_download;
  // A newly started or aborted restore must not reuse an older header.
  if (sna_download && !old_sna_download) begin
   sna_version <= 8'd0;
   sna_plus_model <= 2'd0;
  end
  if (sna_header_wr) begin
   if (sna_addr == 8'h10) sna_version <= sna_data;
   // docs/specs/formats/Snapshot (.SNA) file format.md, header offset 6D.
   // Classic/unknown headers preserve the current Plus model (separate policy).
   if (sna_addr == 8'h6d) begin
    case (sna_data)
     8'd4: sna_plus_model <= 2'd2;
     8'd5: sna_plus_model <= 2'd3;
     8'd6: sna_plus_model <= 2'd1;
     default: sna_plus_model <= 2'd0;
    endcase
   end
  end
  if (request_in_flight &&
      ((status & OWNED_FIELDS) == (status_in & OWNED_FIELDS)))
   request_in_flight <= 1'b0;
  if (!request_in_flight && !publish_pending && !status_set) begin
   if (model_pending && status[34:33] == selected) model_pending <= 1'b0;
   if (sound_pending && status[20] == sound) sound_pending <= 1'b0;
  end

  // Leave a low cycle between requests for hps_io's edge detector. Events
  // below win over this clear, so an event on the dispatch edge is retained.
  status_set <= 1'b0;
  if (publish_pending && !status_set && !request_in_flight) begin
   status_in <= effective_status;
   status_set <= 1'b1;
   request_in_flight <= 1'b1;
   publish_pending <= 1'b0;
  end
  if (fn_edge) begin
   sound <= ~effective_sound;
   sound_pending <= 1'b1;
   publish_pending <= 1'b1;
  end
  if (select_sna || select_cpr) begin
   selected <= select_sna ? sna_plus_model : 2'd2;
   model_pending <= 1'b1;
   publish_pending <= 1'b1;
  end
 end
endmodule
