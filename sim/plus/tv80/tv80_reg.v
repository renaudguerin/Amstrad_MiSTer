//
// TV80 8-Bit Microprocessor Core
// Based on the T80 core by Daniel Wallner (jesus@opencores.org)
//
// Copyright (c) 2004 Guy Hutchison (ghutchis@opencores.org)
//

`timescale 1ns / 1ps

module tv80_reg (
  input             clk,
  input             reset_n,
  input             cen,
  input      [2:0]  AddrA,
  input      [2:0]  AddrB,
  input      [2:0]  AddrC,
  input             Write,
  input      [7:0]  DI,
  output reg [7:0]  DOA,
  output reg [7:0]  DOB,
  output reg [7:0]  DOC,
  output     [15:0] BC,
  output     [15:0] DE,
  output     [15:0] HL,
  output     [7:0]  A_out,
  output     [7:0]  F_out,
  input             ExchangeAF,
  input             ExchangeRS,
  input             Exchange,
  input      [1:0]  Write16_Pair, // 0: BC, 1: DE, 2: HL
  input             Write16_L,
  input             Write16_H,
  input             DIRSet,
  input     [211:0] DIR,
  output    [211:0] REG
);

  reg [7:0] r_a, r_f, r_b, r_c, r_d, r_e, r_h, r_l;
  reg [7:0] r_a_p, r_f_p, r_b_p, r_c_p, r_d_p, r_e_p, r_h_p, r_l_p;
  reg [15:0] r_ix, r_iy, r_sp, r_pc;
  reg [7:0]  r_i, r_r;
  reg        r_iff1, r_iff2;
  reg [1:0]  r_im;

  assign BC = {r_b, r_c};
  assign DE = {r_d, r_e};
  assign HL = {r_h, r_l};
  assign A_out = r_a;
  assign F_out = r_f;

  assign REG = {
    r_iff2, r_iff1, r_im, r_iy,
    r_h_p, r_l_p, r_d_p, r_e_p, r_b_p, r_c_p,
    r_ix,
    r_h, r_l, r_d, r_e, r_b, r_c,
    r_pc, r_sp, r_r, r_i,
    r_f_p, r_a_p, r_f, r_a
  };

  function [7:0] reg_read(input [2:0] addr);
    case (addr)
      3'b000: reg_read = r_b;
      3'b001: reg_read = r_c;
      3'b010: reg_read = r_d;
      3'b011: reg_read = r_e;
      3'b100: reg_read = r_h;
      3'b101: reg_read = r_l;
      3'b110: reg_read = r_f;
      3'b111: reg_read = r_a;
    endcase
  endfunction

  always @* begin
    DOA = reg_read(AddrA);
    DOB = reg_read(AddrB);
    DOC = reg_read(AddrC);
  end

  always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
      r_a <= 8'hFF; r_f <= 8'hFF;
      r_b <= 8'hFF; r_c <= 8'hFF;
      r_d <= 8'hFF; r_e <= 8'hFF;
      r_h <= 8'hFF; r_l <= 8'hFF;
      r_a_p <= 8'hFF; r_f_p <= 8'hFF;
      r_b_p <= 8'hFF; r_c_p <= 8'hFF;
      r_d_p <= 8'hFF; r_e_p <= 8'hFF;
      r_h_p <= 8'hFF; r_l_p <= 8'hFF;
      r_ix <= 16'hFFFF; r_iy <= 16'hFFFF;
      r_sp <= 16'hFFFF; r_pc <= 16'h0000;
      r_i <= 8'h00; r_r <= 8'h00;
      r_iff1 <= 1'b0; r_iff2 <= 1'b0;
      r_im <= 2'b00;
    end else if (DIRSet) begin
      {r_iff2, r_iff1, r_im, r_iy,
       r_h_p, r_l_p, r_d_p, r_e_p, r_b_p, r_c_p,
       r_ix,
       r_h, r_l, r_d, r_e, r_b, r_c,
       r_pc, r_sp, r_r, r_i,
       r_f_p, r_a_p, r_f, r_a} <= DIR;
    end else if (cen) begin
      if (ExchangeAF) begin
        r_a <= r_a_p; r_a_p <= r_a;
        r_f <= r_f_p; r_f_p <= r_f;
      end
      if (ExchangeRS) begin
        r_b <= r_b_p; r_b_p <= r_b;
        r_c <= r_c_p; r_c_p <= r_c;
        r_d <= r_d_p; r_d_p <= r_d;
        r_e <= r_e_p; r_e_p <= r_e;
        r_h <= r_h_p; r_h_p <= r_h;
        r_l <= r_l_p; r_l_p <= r_l;
      end
      if (Exchange) begin
        r_d <= r_h; r_h <= r_d;
        r_e <= r_l; r_l <= r_e;
      end
      if (Write16_L) begin
        case (Write16_Pair)
          2'b00: r_c <= DI;
          2'b01: r_e <= DI;
          2'b10: r_l <= DI;
          default: ;
        endcase
      end
      if (Write16_H) begin
        case (Write16_Pair)
          2'b00: r_b <= DI;
          2'b01: r_d <= DI;
          2'b10: r_h <= DI;
          default: ;
        endcase
      end
      if (Write) begin
        case (AddrA)
          3'b000: r_b <= DI;
          3'b001: r_c <= DI;
          3'b010: r_d <= DI;
          3'b011: r_e <= DI;
          3'b100: r_h <= DI;
          3'b101: r_l <= DI;
          3'b110: r_f <= DI;
          3'b111: r_a <= DI;
        endcase
      end
    end
  end

endmodule
