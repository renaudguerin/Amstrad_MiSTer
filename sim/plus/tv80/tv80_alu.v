//
// TV80 8-Bit Microprocessor Core
// Based on the T80 core by Daniel Wallner (jesus@opencores.org)
//
// Copyright (c) 2004 Guy Hutchison (ghutchis@opencores.org)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

`timescale 1ns / 1ps

module tv80_alu (
  input  [3:0]  ALU_Op,
  input  [7:0]  BusA,
  input  [7:0]  BusB,
  input  [7:0]  F_In,
  input  [2:0]  IR,
  input         Arith16,
  output [7:0]  Q,
  output [7:0]  F_Out
);

  reg [7:0]  Q_r;
  reg [7:0]  F_Out_r;
  reg [8:0]  Result;
  reg        C, N, P, X, H, Y, Z, S;
  reg [8:0]  daa_res;
  reg [3:0]  daa_adj;
  reg        daa_c;

  assign Q     = Q_r;
  assign F_Out = F_Out_r;

  // Parity lookup
  function parity(input [7:0] val);
    parity = ~^val;
  endfunction

  always @* begin
    C = F_In[0];
    N = F_In[1];
    P = F_In[2];
    X = BusA[3];
    H = F_In[4];
    Y = BusA[5];
    Z = F_In[6];
    S = F_In[7];
    Result = 9'd0;
    daa_res = 9'd0;
    daa_adj = 4'd0;
    daa_c = 1'b0;
    daa_c = 1'b0;

    case (ALU_Op)
      4'b0000: begin // ADD / ADC
        Result = {1'b0, BusA} + {1'b0, BusB} + {8'd0, (ALU_Op[0] ? F_In[0] : 1'b0)};
        C = Result[8];
        H = ({1'b0, BusA[3:0]} + {1'b0, BusB[3:0]} + {4'd0, (ALU_Op[0] ? F_In[0] : 1'b0)}) > 5'd15;
        P = (BusA[7] == BusB[7]) && (Result[7] != BusA[7]); // Overflow
        N = 1'b0;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0001: begin // ADC
        Result = {1'b0, BusA} + {1'b0, BusB} + {8'd0, F_In[0]};
        C = Result[8];
        H = ({1'b0, BusA[3:0]} + {1'b0, BusB[3:0]} + {4'd0, F_In[0]}) > 5'd15;
        P = (BusA[7] == BusB[7]) && (Result[7] != BusA[7]);
        N = 1'b0;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0010: begin // SUB / SBC
        Result = {1'b0, BusA} - {1'b0, BusB} - {8'd0, (ALU_Op[0] ? F_In[0] : 1'b0)};
        C = Result[8];
        H = BusA[3:0] < (BusB[3:0] + {3'd0, (ALU_Op[0] ? F_In[0] : 1'b0)});
        P = (BusA[7] != BusB[7]) && (Result[7] != BusA[7]); // Overflow
        N = 1'b1;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0011: begin // SBC
        Result = {1'b0, BusA} - {1'b0, BusB} - {8'd0, F_In[0]};
        C = Result[8];
        H = BusA[3:0] < (BusB[3:0] + {3'd0, F_In[0]});
        P = (BusA[7] != BusB[7]) && (Result[7] != BusA[7]);
        N = 1'b1;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0100: begin // AND
        Result = {1'b0, BusA & BusB};
        C = 1'b0;
        H = 1'b1;
        P = parity(Result[7:0]);
        N = 1'b0;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0101: begin // XOR
        Result = {1'b0, BusA ^ BusB};
        C = 1'b0;
        H = 1'b0;
        P = parity(Result[7:0]);
        N = 1'b0;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0110: begin // OR
        Result = {1'b0, BusA | BusB};
        C = 1'b0;
        H = 1'b0;
        P = parity(Result[7:0]);
        N = 1'b0;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b0111: begin // CP
        Result = {1'b0, BusA} - {1'b0, BusB};
        C = Result[8];
        H = BusA[3:0] < BusB[3:0];
        P = (BusA[7] != BusB[7]) && (Result[7] != BusA[7]);
        N = 1'b1;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = BusB[3];
        Y = BusB[5];
      end

      4'b1010: begin // INC
        Result = {1'b0, BusA} + 9'd1;
        H = (BusA[3:0] == 4'hF);
        P = (BusA == 8'h7F);
        N = 1'b0;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b1011: begin // DEC
        Result = {1'b0, BusA} - 9'd1;
        H = (BusA[3:0] == 4'h0);
        P = (BusA == 8'h80);
        N = 1'b1;
        Z = (Result[7:0] == 8'd0);
        S = Result[7];
        X = Result[3];
        Y = Result[5];
      end

      4'b1100: begin // DAA
        daa_adj = 4'd0;
        daa_c = F_In[0];
        if (F_In[4] || (BusA[3:0] > 4'd9)) daa_adj[1:0] = 2'b10; // +6
        if (F_In[0] || (BusA[7:4] > 4'd9) || ((BusA[7:4] == 4'd9) && (BusA[3:0] > 4'd9))) begin
          daa_adj[3:2] = 2'b10; // +0x60
          daa_c = 1'b1;
        end
        if (F_In[1]) begin // N flag
          daa_res = {1'b0, BusA} - {1'b0, daa_adj[3:2], 2'b00, daa_adj[1:0], 2'b00};
        end else begin
          daa_res = {1'b0, BusA} + {1'b0, daa_adj[3:2], 2'b00, daa_adj[1:0], 2'b00};
        end
        Result = daa_res;
        C = daa_c;
        H = BusA[4] ^ daa_res[4];
        P = parity(daa_res[7:0]);
        Z = (daa_res[7:0] == 8'd0);
        S = daa_res[7];
        X = daa_res[3];
        Y = daa_res[5];
      end

      default: begin
        Result = {1'b0, BusA};
      end
    endcase

    if (ALU_Op == 4'b0111) begin // CP does not store result in A
      Q_r = BusA;
    end else begin
      Q_r = Result[7:0];
    end

    F_Out_r = {S, Z, Y, H, X, P, N, C};
  end

endmodule
