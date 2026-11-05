// ====================================================================
//
//  Amstrad CPC Gate Array
//  Based on 40010-simplified_V03.pdf by Gerald
//  Pixel alignment follows the Amstrad CPC CRTC Compendium v1.11,
//  Serge Querne (Longshot / Logon System), https://shaker.logonsystem.eu
//  licensed CC BY-NC-ND 4.0; attribution required in distributed credits.
//
//  Copyright (C) 2020 Gyorgy Szombathelyi <gyurco@freemail.hu>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//============================================================================


// Abstract GA40010 output/serializer model; no literal silicon-stage claim.
// French ACCC v1.11 §§9.1, 9.3.4 pp47,56–67 defines the pixel rules.
// Byte/epoch identities below serve control; causal test metadata lives only
// in a separate simulation monitor, outside this production module.
module video (
 input clk, cen_16, input [7:0] S, input DISPEN_BUF, PHI_N,
 input [1:0] MODE, input MODE_SYNC, MODE_SYNC_EN,
 input [7:0] VIDEO, input [4:0] BORDER, input [4:0] INKR[16],
 input FORCE_BLANK, input MODEL_RESET, PHASE_P, PHASE_N,
 input [1:0] MODEL_MODE, input VIDEO_VALID,
 output reg BLUE_OE_N, BLUE, GREEN_OE_N, GREEN, RED_OE_N, RED
);
reg load=0, u1005=0;
wire u1008=load?DISPEN_BUF:u1005;
reg [7:0] byte_reg=0;
reg byte_valid=0;
reg [3:0] cursor=0, age=0;
reg [1:0] active_mode=0, pending_mode=0;
reg mode_pending=0;
reg [15:0] byte_id=0, epoch_id=0;
reg anchor_valid=0;
reg [3:0] anchor_cursor=0, anchor_age=0;
reg [15:0] anchor_byte_id=0;
reg [4:0] colour=0;
reg [15:0] token_epoch=0;
reg token_valid=0, permit=0, await_colour=1;
reg blank_stage1=1, blank_stage2=1;
wire mode_is_0=active_mode==0, mode_is_1=active_mode==1, mode_is_2=active_mode==2;
wire [7:0] shift_reg=cursor>=8?8'd0:(byte_reg<<cursor);
wire [3:0] cidx={shift_reg[1],shift_reg[5],shift_reg[3],shift_reg[7]};
wire [3:0] pen=mode_is_0?cidx:mode_is_2?{3'd0,cidx[0]}:{2'd0,cidx[1:0]};
wire [3:0] event_age=load?4'd8:age+4'd1;
wire refresh=(mode_is_2 || !event_age[0]);
wire colour_keep=!refresh;
wire ink_sel=refresh&&u1008;
wire border_sel=refresh&&!u1008;
function automatic advance(input [1:0] m,input [3:0] a);
 begin
  case(m)
   0,3: advance=(a==4);
   1: advance=(a==2||a==4||a==6);
   2: advance=(a>=1&&a<=7);
  endcase
 end
endfunction
function automatic [3:0] sat_add(input [3:0] c,input [3:0] n);
 reg [4:0] x;begin x={1'b0,c}+{1'b0,n};sat_add=x>8?4'd8:x[3:0];end
endfunction
function automatic [3:0] replay_steps(input [1:0] m,input [3:0] lo,hi);
 integer i;reg [3:0] n;begin n=0;for(i=1;i<=8;i=i+1)if(i>lo&&i<=hi&&advance(m,i[3:0]))n=n+1;replay_steps=n;end
endfunction
wire shift=advance(active_mode,event_age)&&!load;
wire keep=!shift&&!load;
wire [3:0] normal_cursor=sat_add(cursor,{3'd0,shift});
wire accept=cen_16&&mode_pending&&(PHASE_P||PHASE_N);
wire [15:0] anchor_epoch_after=PHASE_P?epoch_id+16'd1:epoch_id;
wire [15:0] pending_epoch_now=refresh?epoch_id:token_epoch;
wire pending_valid_now=refresh?byte_valid:token_valid;
wire anchor_valid_after=PHASE_P?byte_valid:anchor_valid;
wire invalidate=accept&&pending_valid_now&&anchor_valid_after&&(pending_epoch_now==anchor_epoch_after);
wire arrival=cen_16&&await_colour&&refresh&&byte_valid&&!accept;
wire [3:0] replay_cursor=sat_add(anchor_cursor,replay_steps(pending_mode,anchor_age,event_age));

always @(posedge clk) begin
 if(MODEL_RESET) begin
  byte_reg<=0;byte_valid<=0;cursor<=0;age<=0;
  // Snapshot mode is an explicit settled-header normalization, as in Plus.
  // The caller supplies zero for hardware reset and SNA_CONFIG for apply.
  // A later HSYNC is not guaranteed (e.g. R3=0).
  active_mode<=MODEL_MODE;pending_mode<=MODEL_MODE;mode_pending<=0;
  byte_id<=0;epoch_id<=0;
  anchor_valid<=0;anchor_cursor<=0;anchor_age<=0;anchor_byte_id<=0;
  colour<=0;
  token_epoch<=0;token_valid<=0;
  permit<=0;await_colour<=1;blank_stage1<=1;blank_stage2<=1;
  BLUE_OE_N<=0;BLUE<=0;GREEN_OE_N<=0;GREEN<=0;RED_OE_N<=0;RED<=0;
 end else begin
  if(cen_16) begin
   // Publish preceding colour/blank state; current token generation follows.
   if(blank_stage2) begin
    BLUE_OE_N<=0;BLUE<=0;GREEN_OE_N<=0;GREEN<=0;RED_OE_N<=0;RED<=0;
   end else begin
    BLUE_OE_N<=~((colour[1]|colour[2])&(colour[3]|colour[4]));BLUE<=colour[0];
    GREEN_OE_N<=(colour[1]&colour[2])|~(colour[1]|colour[2]|colour[3]|colour[4]);GREEN<=(~colour[2]&colour[0])|colour[1];
    RED_OE_N<=~(colour[1]|colour[2]|colour[3]|colour[4])|(colour[3]&colour[4]);RED<=(colour[0]&~colour[4])|colour[3];
   end
   blank_stage1<=FORCE_BLANK||!(permit||arrival);
   blank_stage2<=blank_stage1;
   if(arrival) begin permit<=1;await_colour<=0;end
   if(refresh) begin
    colour<=u1008?INKR[pen]:BORDER;
    token_epoch<=epoch_id;token_valid<=byte_valid;
   end
   if(load) begin byte_reg<=VIDEO;cursor<=0;age<=0;byte_valid<=VIDEO_VALID;byte_id<=byte_id+16'd1;end
   else begin cursor<=normal_cursor;age<=event_age;end
   if(PHASE_P) begin
    anchor_cursor<=normal_cursor;anchor_age<=event_age;
    anchor_byte_id<=byte_id;anchor_valid<=byte_valid;epoch_id<=epoch_id+16'd1;
   end
   if(accept) begin
    active_mode<=pending_mode;mode_pending<=0;
    if(!PHASE_P&&anchor_valid&&anchor_byte_id==byte_id)cursor<=replay_cursor;
    if(invalidate) begin blank_stage1<=1;blank_stage2<=1;permit<=0;await_colour<=1;end
   end
  end
  // Last assignment queues a coincident new request for a subsequent Q.
  if(MODE_SYNC_EN) begin pending_mode<=MODE;mode_pending<=1;end
 end
 // These are the unchanged endogenous load/display pipeline decisions.
 if(cen_16) begin load<=S[5]^S[6];u1005<=u1008;end
end
endmodule
