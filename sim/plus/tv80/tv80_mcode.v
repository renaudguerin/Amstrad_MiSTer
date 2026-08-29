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

module tv80_mcode (
  input  [2:0]  Mode,
  input  [7:0]  IR,
  input  [2:0]  MCycle,
  input  [2:0]  Prefix_In,
  output [2:0]  MCycles,
  output [2:0]  TStates,
  output [2:0]  Prefix,
  output        Inc_PC,
  output        Dec_PC,
  output        Inc_WZ,
  output        IncDec_16,
  output [2:0]  Set_Addr_To,
  output        Jump,
  output        JumpE,
  output        JumpXY,
  output        Call,
  output        RstP,
  output        LDZ,
  output        LDW,
  output        LDSPHL,
  output        IORQ,
  output        Special_G_N,
  output        ExchangeAF,
  output        ExchangeRS,
  output        Exchange,
  output        Read_To_Reg,
  output        Read_To_Acc,
  output [3:0]  Set_BusA_To,
  output [3:0]  Set_BusB_To,
  output [3:0]  ALU_Op,
  output        Save_ALU,
  output        PreserveC_N,
  output        Arith16,
  output        Set_IFF1,
  output        Set_IFF2,
  output        Auto_Reset_IFF,
  output [1:0]  Set_IM,
  output        Rot_Int,
  output        Rot_Int_N,
  output        Write,
  output        NoRead,
  output        Read_F,
  output        Write_F,
  output        TState_Wait_N,
  output        Auto_Reset_PC,
  output        Special_G_Reset_N,
  output        RetI_N
);

  localparam aNone      = 3'b000;
  localparam aBC        = 3'b001;
  localparam aDE        = 3'b010;
  localparam aHL        = 3'b011;
  localparam aSP        = 3'b100;
  localparam aXY        = 3'b101;
  localparam aWZ        = 3'b110;

  localparam bNone      = 4'b0000;
  localparam bB         = 4'b0001;
  localparam bC         = 4'b0010;
  localparam bD         = 4'b0011;
  localparam bE         = 4'b0100;
  localparam bH         = 4'b0101;
  localparam bL         = 4'b0110;
  localparam bF         = 4'b0111;
  localparam bA         = 4'b1000;
  localparam bIX        = 4'b1001;
  localparam bIY        = 4'b1010;
  localparam bSP        = 4'b1011;
  localparam bHigh      = 4'b1100;
  localparam bLow       = 4'b1101;
  localparam bZero      = 4'b1110;
  localparam bOne       = 4'b1111;

  reg [2:0]  MCycles_r;
  reg [2:0]  TStates_r;
  reg [2:0]  Prefix_r;
  reg        Inc_PC_r;
  reg        Dec_PC_r;
  reg        Inc_WZ_r;
  reg        IncDec_16_r;
  reg [2:0]  Set_Addr_To_r;
  reg        Jump_r;
  reg        JumpE_r;
  reg        JumpXY_r;
  reg        Call_r;
  reg        RstP_r;
  reg        LDZ_r;
  reg        LDW_r;
  reg        LDSPHL_r;
  reg        IORQ_r;
  reg        Special_G_N_r;
  reg        ExchangeAF_r;
  reg        ExchangeRS_r;
  reg        Exchange_r;
  reg        Read_To_Reg_r;
  reg        Read_To_Acc_r;
  reg [3:0]  Set_BusA_To_r;
  reg [3:0]  Set_BusB_To_r;
  reg [3:0]  ALU_Op_r;
  reg        Save_ALU_r;
  reg        PreserveC_N_r;
  reg        Arith16_r;
  reg        Set_IFF1_r;
  reg        Set_IFF2_r;
  reg        Auto_Reset_IFF_r;
  reg [1:0]  Set_IM_r;
  reg        Rot_Int_r;
  reg        Rot_Int_N_r;
  reg        Write_r;
  reg        NoRead_r;
  reg        Read_F_r;
  reg        Write_F_r;
  reg        TState_Wait_N_r;
  reg        Auto_Reset_PC_r;
  reg        Special_G_Reset_N_r;
  reg        RetI_N_r;

  assign MCycles           = MCycles_r;
  assign TStates           = TStates_r;
  assign Prefix            = Prefix_r;
  assign Inc_PC            = Inc_PC_r;
  assign Dec_PC            = Dec_PC_r;
  assign Inc_WZ            = Inc_WZ_r;
  assign IncDec_16         = IncDec_16_r;
  assign Set_Addr_To       = Set_Addr_To_r;
  assign Jump              = Jump_r;
  assign JumpE             = JumpE_r;
  assign JumpXY            = JumpXY_r;
  assign Call              = Call_r;
  assign RstP              = RstP_r;
  assign LDZ               = LDZ_r;
  assign LDW               = LDW_r;
  assign LDSPHL            = LDSPHL_r;
  assign IORQ              = IORQ_r;
  assign Special_G_N       = Special_G_N_r;
  assign ExchangeAF        = ExchangeAF_r;
  assign ExchangeRS        = ExchangeRS_r;
  assign Exchange          = Exchange_r;
  assign Read_To_Reg       = Read_To_Reg_r;
  assign Read_To_Acc       = Read_To_Acc_r;
  assign Set_BusA_To       = Set_BusA_To_r;
  assign Set_BusB_To       = Set_BusB_To_r;
  assign ALU_Op            = ALU_Op_r;
  assign Save_ALU          = Save_ALU_r;
  assign PreserveC_N       = PreserveC_N_r;
  assign Arith16           = Arith16_r;
  assign Set_IFF1          = Set_IFF1_r;
  assign Set_IFF2          = Set_IFF2_r;
  assign Auto_Reset_IFF    = Auto_Reset_IFF_r;
  assign Set_IM            = Set_IM_r;
  assign Rot_Int           = Rot_Int_r;
  assign Rot_Int_N         = Rot_Int_N_r;
  assign Write             = Write_r;
  assign NoRead            = NoRead_r;
  assign Read_F            = Read_F_r;
  assign Write_F           = Write_F_r;
  assign TState_Wait_N     = TState_Wait_N_r;
  assign Auto_Reset_PC     = Auto_Reset_PC_r;
  assign Special_G_Reset_N = Special_G_Reset_N_r;
  assign RetI_N            = RetI_N_r;

  wire [1:0] DDCycle = 2'b00;
  wire [1:0] FDCycle = 2'b00;
  wire [1:0] EDCycle = 2'b00;
  wire [1:0] CBCycle = 2'b00;
  wire [2:0] IntCycle = 3'b000;

  always @* begin
    MCycles_r           = 3'b001;
    TStates_r           = 3'b000;
    Prefix_r            = 3'b000;
    Inc_PC_r            = 1'b0;
    Dec_PC_r            = 1'b0;
    Inc_WZ_r            = 1'b0;
    IncDec_16_r         = 1'b0;
    Set_Addr_To_r       = aNone;
    Jump_r              = 1'b0;
    JumpE_r             = 1'b0;
    JumpXY_r            = 1'b0;
    Call_r              = 1'b0;
    RstP_r              = 1'b0;
    LDZ_r               = 1'b0;
    LDW_r               = 1'b0;
    LDSPHL_r            = 1'b0;
    IORQ_r              = 1'b0;
    Special_G_N_r       = 1'b1;
    ExchangeAF_r        = 1'b0;
    ExchangeRS_r        = 1'b0;
    Exchange_r          = 1'b0;
    Read_To_Reg_r       = 1'b0;
    Read_To_Acc_r       = 1'b0;
    Set_BusA_To_r       = bNone;
    Set_BusB_To_r       = bNone;
    ALU_Op_r            = 4'b0000;
    Save_ALU_r          = 1'b0;
    PreserveC_N_r       = 1'b1;
    Arith16_r           = 1'b0;
    Set_IFF1_r          = 1'b0;
    Set_IFF2_r          = 1'b0;
    Auto_Reset_IFF_r    = 1'b0;
    Set_IM_r            = 2'b00;
    Rot_Int_r           = 1'b0;
    Rot_Int_N_r         = 1'b1;
    Write_r             = 1'b0;
    NoRead_r            = 1'b0;
    Read_F_r            = 1'b0;
    Write_F_r           = 1'b0;
    TState_Wait_N_r     = 1'b1;
    Auto_Reset_PC_r     = 1'b0;
    Special_G_Reset_N_r = 1'b1;
    RetI_N_r            = 1'b1;

    if (IntCycle != 3'b000) begin
      case (IntCycle)
        3'b001: begin
          MCycles_r = 3'b011;
          IORQ_r = 1'b1;
          Auto_Reset_IFF_r = 1'b1;
        end
        3'b010: begin
          Set_Addr_To_r = aSP;
          Write_r = 1'b1;
          Set_BusB_To_r = bHigh;
          Dec_PC_r = 1'b1;
        end
        3'b011: begin
          Set_Addr_To_r = aSP;
          Write_r = 1'b1;
          Set_BusB_To_r = bLow;
          RstP_r = 1'b1;
        end
        3'b100: begin
          IORQ_r = 1'b1;
          Auto_Reset_IFF_r = 1'b1;
          LDZ_r = 1'b1;
          MCycles_r = 3'b111;
        end
        3'b101: begin
          Set_Addr_To_r = aWZ;
          LDZ_r = 1'b1;
          Inc_WZ_r = 1'b1;
        end
        3'b111: begin
          Call_r = 1'b1;
        end
      endcase
    end

    if (Prefix_In == 3'b011) begin // ED Prefix
      case (IR[7:6])
        2'b01: begin
          case (IR[2:0])
            3'b000: begin // IN r, (C)
              MCycles_r = 3'b010;
              case (MCycle)
                3'b001: Inc_PC_r = 1'b1;
                3'b010: begin
                  Set_Addr_To_r = aBC;
                  IORQ_r = 1'b1;
                  Read_To_Reg_r = 1'b1;
                end
                default: ;
              endcase
            end
            3'b001: begin // OUT (C), r
              MCycles_r = 3'b010;
              case (MCycle)
                3'b001: Inc_PC_r = 1'b1;
                3'b010: begin
                  Set_Addr_To_r = aBC;
                  IORQ_r = 1'b1;
                  Write_r = 1'b1;
                  case (IR[5:3])
                    3'b000: Set_BusB_To_r = bB;
                    3'b001: Set_BusB_To_r = bC;
                    3'b010: Set_BusB_To_r = bD;
                    3'b011: Set_BusB_To_r = bE;
                    3'b100: Set_BusB_To_r = bH;
                    3'b101: Set_BusB_To_r = bL;
                    3'b111: Set_BusB_To_r = bA;
                    default: Set_BusB_To_r = bNone;
                  endcase
                end
                default: ;
              endcase
            end
            3'b010: begin // SBC/ADC HL, ss
              MCycles_r = 3'b001;
              Inc_PC_r = 1'b1;
              Arith16_r = 1'b1;
              Set_BusA_To_r = bH;
              ALU_Op_r = IR[3] ? 4'b0000 : 4'b0010; // ADC / SBC
              Save_ALU_r = 1'b1;
            end
            3'b101: begin // RETI / RETN
              MCycles_r = 3'b011;
              case (MCycle)
                3'b001: Inc_PC_r = 1'b1;
                3'b010: begin
                  Set_Addr_To_r = aSP;
                  LDZ_r = 1'b1;
                end
                3'b011: begin
                  Set_Addr_To_r = aSP;
                  LDW_r = 1'b1;
                  Jump_r = 1'b1;
                  RetI_N_r = ~IR[3];
                end
                default: ;
              endcase
            end
            3'b110: begin // IM 0/1/2
              Inc_PC_r = 1'b1;
              Set_IM_r = (IR[4:3] == 2'b11) ? 2'b10 : (IR[4:3] == 2'b10) ? 2'b01 : 2'b00;
            end
            default: Inc_PC_r = 1'b1;
          endcase
        end
        default: Inc_PC_r = 1'b1;
      endcase
    end else if (IR == 8'hED) begin
      Prefix_r = 3'b011;
      Inc_PC_r = 1'b1;
    end else if (IR == 8'hCB) begin
      Prefix_r = 3'b001;
      Inc_PC_r = 1'b1;
    end else if (IR == 8'hDD) begin
      Prefix_r = 3'b010;
      Inc_PC_r = 1'b1;
    end else if (IR == 8'hFD) begin
      Prefix_r = 3'b100;
      Inc_PC_r = 1'b1;
    end else begin
    case (IR[7:6])
      2'b00: begin
        case (IR[5:0])
          6'b000000: begin // NOP
          end
          6'b001000: begin // EX AF, AF'
            ExchangeAF_r = 1'b1;
          end
          6'b010000: begin // DJNZ e
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: begin
                Inc_PC_r = 1'b1;
                Set_BusA_To_r = bB;
                ALU_Op_r = 4'b1011; // DEC
                Save_ALU_r = 1'b1;
                Read_To_Reg_r = 1'b1;
              end
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: begin
                JumpE_r = 1'b1;
              end
              default: ;
            endcase
          end
          6'b011000: begin // JR e
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: JumpE_r = 1'b1;
              default: ;
            endcase
          end
          6'b100000, 6'b101000, 6'b110000, 6'b111000: begin // JR cc, e
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: JumpE_r = 1'b1;
              default: ;
            endcase
          end
          default: begin
            case (IR[3:0])
              4'b0001: begin // LD dd, nn
                MCycles_r = 3'b011;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    LDZ_r = 1'b1;
                    Inc_PC_r = 1'b1;
                  end
                  3'b011: begin
                    LDW_r = 1'b1;
                    Inc_PC_r = 1'b1;
                    Read_To_Reg_r = 1'b1;
                  end
                  default: ;
                endcase
              end
              4'b0011: begin // INC ss
                IncDec_16_r = 1'b1;
              end
              4'b1011: begin // DEC ss
                IncDec_16_r = 1'b1;
              end
              4'b0010: begin // LD (ss), A / LD A, (ss) / LD (nn), HL / LD HL, (nn)
                case (IR[5:4])
                  2'b00: begin // LD (BC), A
                    MCycles_r = 3'b010;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        Set_Addr_To_r = aBC;
                        Write_r = 1'b1;
                        Set_BusB_To_r = bA;
                      end
                      default: ;
                    endcase
                  end
                  2'b01: begin // LD (DE), A
                    MCycles_r = 3'b010;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        Set_Addr_To_r = aDE;
                        Write_r = 1'b1;
                        Set_BusB_To_r = bA;
                      end
                      default: ;
                    endcase
                  end
                  2'b10: begin // LD (nn), HL
                    MCycles_r = 3'b101;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        LDZ_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b011: begin
                        LDW_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b100: begin
                        Set_Addr_To_r = aWZ;
                        Write_r = 1'b1;
                        Set_BusB_To_r = bL;
                        Inc_WZ_r = 1'b1;
                      end
                      3'b101: begin
                        Set_Addr_To_r = aWZ;
                        Write_r = 1'b1;
                        Set_BusB_To_r = bH;
                      end
                      default: ;
                    endcase
                  end
                  2'b11: begin // LD (nn), A
                    MCycles_r = 3'b100;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        LDZ_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b011: begin
                        LDW_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b100: begin
                        Set_Addr_To_r = aWZ;
                        Write_r = 1'b1;
                        Set_BusB_To_r = bA;
                      end
                      default: ;
                    endcase
                  end
                endcase
              end
              4'b1010: begin // LD A, (BC) / LD A, (DE) / LD HL, (nn) / LD A, (nn)
                case (IR[5:4])
                  2'b00: begin // LD A, (BC)
                    MCycles_r = 3'b010;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        Set_Addr_To_r = aBC;
                        Read_To_Acc_r = 1'b1;
                      end
                      default: ;
                    endcase
                  end
                  2'b01: begin // LD A, (DE)
                    MCycles_r = 3'b010;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        Set_Addr_To_r = aDE;
                        Read_To_Acc_r = 1'b1;
                      end
                      default: ;
                    endcase
                  end
                  2'b10: begin // LD HL, (nn)
                    MCycles_r = 3'b101;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        LDZ_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b011: begin
                        LDW_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b100: begin
                        Set_Addr_To_r = aWZ;
                        Read_To_Reg_r = 1'b1;
                        Inc_WZ_r = 1'b1;
                      end
                      3'b101: begin
                        Set_Addr_To_r = aWZ;
                        Read_To_Reg_r = 1'b1;
                      end
                      default: ;
                    endcase
                  end
                  2'b11: begin // LD A, (nn)
                    MCycles_r = 3'b100;
                    case (MCycle)
                      3'b001: Inc_PC_r = 1'b1;
                      3'b010: begin
                        LDZ_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b011: begin
                        LDW_r = 1'b1;
                        Inc_PC_r = 1'b1;
                      end
                      3'b100: begin
                        Set_Addr_To_r = aWZ;
                        Read_To_Acc_r = 1'b1;
                      end
                      default: ;
                    endcase
                  end
                endcase
              end
              4'b0100, 4'b1100: begin // INC r
                Inc_PC_r = 1'b1;
                if (IR[5:3] == 3'b110) begin // INC (HL)
                  MCycles_r = 3'b011;
                  case (MCycle)
                    3'b001: Inc_PC_r = 1'b1;
                    3'b010: begin
                      Set_Addr_To_r = aHL;
                      Read_To_Acc_r = 1'b1;
                      ALU_Op_r = 4'b1010;
                    end
                    3'b011: begin
                      Set_Addr_To_r = aHL;
                      Write_r = 1'b1;
                      Set_BusB_To_r = bA;
                    end
                    default: ;
                  endcase
                end else begin
                  Set_BusA_To_r = {1'b0, IR[5:3]};
                  ALU_Op_r = 4'b1010; // INC
                  Save_ALU_r = 1'b1;
                  Read_To_Reg_r = 1'b1;
                end
              end
              4'b0101, 4'b1101: begin // DEC r
                Inc_PC_r = 1'b1;
                if (IR[5:3] == 3'b110) begin // DEC (HL)
                  MCycles_r = 3'b011;
                  case (MCycle)
                    3'b001: Inc_PC_r = 1'b1;
                    3'b010: begin
                      Set_Addr_To_r = aHL;
                      Read_To_Acc_r = 1'b1;
                      ALU_Op_r = 4'b1011;
                    end
                    3'b011: begin
                      Set_Addr_To_r = aHL;
                      Write_r = 1'b1;
                      Set_BusB_To_r = bA;
                    end
                    default: ;
                  endcase
                end else begin
                  Set_BusA_To_r = {1'b0, IR[5:3]};
                  ALU_Op_r = 4'b1011; // DEC
                  Save_ALU_r = 1'b1;
                  Read_To_Reg_r = 1'b1;
                end
              end
              4'b0110, 4'b1110: begin // LD r, n
                MCycles_r = 3'b010;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    Inc_PC_r = 1'b1;
                    if (IR[5:3] == 3'b110) begin
                      Set_Addr_To_r = aHL;
                      Write_r = 1'b1;
                      Read_To_Acc_r = 1'b1;
                    end else begin
                      Read_To_Reg_r = 1'b1;
                    end
                  end
                  default: ;
                endcase
              end
              4'b1001: begin // ADD HL, ss
                Inc_PC_r = 1'b1;
                Arith16_r = 1'b1;
                Set_BusA_To_r = bH;
                ALU_Op_r = 4'b0000;
                Save_ALU_r = 1'b1;
              end
              default: ;
            endcase
          end
        endcase
      end
      2'b01: begin // LD r, r' / HALT
        Inc_PC_r = 1'b1;
        if (IR == 8'h76) begin // HALT
          Dec_PC_r = 1'b1;
        end else if (IR[2:0] == 3'b110) begin // LD r, (HL)
          MCycles_r = 3'b010;
          case (MCycle)
            3'b001: Inc_PC_r = 1'b1;
            3'b010: begin
              Set_Addr_To_r = aHL;
              Read_To_Reg_r = 1'b1;
            end
            default: ;
          endcase
        end else if (IR[5:3] == 3'b110) begin // LD (HL), r
          MCycles_r = 3'b010;
          case (MCycle)
            3'b001: Inc_PC_r = 1'b1;
            3'b010: begin
              Set_Addr_To_r = aHL;
              Write_r = 1'b1;
              Set_BusB_To_r = {1'b0, IR[2:0]};
            end
            default: ;
          endcase
        end else begin // LD r, r'
          Set_BusB_To_r = {1'b0, IR[2:0]};
          Read_To_Reg_r = 1'b1;
        end
      end
      2'b10: begin // ALU A, r
        Inc_PC_r = 1'b1;
        if (IR[2:0] == 3'b110) begin // ALU A, (HL)
          MCycles_r = 3'b010;
          case (MCycle)
            3'b001: Inc_PC_r = 1'b1;
            3'b010: begin
              Set_Addr_To_r = aHL;
              Set_BusA_To_r = bA;
              ALU_Op_r = {1'b0, IR[5:3]};
              Save_ALU_r = 1'b1;
              Read_To_Acc_r = 1'b1;
            end
            default: ;
          endcase
        end else begin
          Set_BusA_To_r = bA;
          Set_BusB_To_r = {1'b0, IR[2:0]};
          ALU_Op_r = {1'b0, IR[5:3]};
          Save_ALU_r = 1'b1;
          Read_To_Acc_r = 1'b1;
        end
      end
      2'b11: begin
        case (IR[5:0])
          6'b001001: begin // RET
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                Set_Addr_To_r = aSP;
                LDZ_r = 1'b1;
              end
              3'b011: begin
                Set_Addr_To_r = aSP;
                LDW_r = 1'b1;
                Jump_r = 1'b1;
              end
              default: ;
            endcase
          end
          6'b000001: begin // POP qq
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                Set_Addr_To_r = aSP;
                LDZ_r = 1'b1;
              end
              3'b011: begin
                Set_Addr_To_r = aSP;
                LDW_r = 1'b1;
                Read_To_Reg_r = 1'b1;
              end
              default: ;
            endcase
          end
          6'b000011: begin // JP nn
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: begin
                LDW_r = 1'b1;
                Jump_r = 1'b1;
              end
              default: ;
            endcase
          end
          6'b010011: begin // OUT (n), A
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: begin
                Set_Addr_To_r = aWZ;
                IORQ_r = 1'b1;
                Write_r = 1'b1;
                Set_BusB_To_r = bA;
              end
              default: ;
            endcase
          end
          6'b011011: begin // IN A, (n)
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: begin
                Set_Addr_To_r = aWZ;
                IORQ_r = 1'b1;
                Read_To_Acc_r = 1'b1;
              end
              default: ;
            endcase
          end
          6'b001101: begin // CALL nn
            MCycles_r = 3'b101;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                LDZ_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b011: begin
                LDW_r = 1'b1;
                Inc_PC_r = 1'b1;
              end
              3'b100: begin
                Set_Addr_To_r = aSP;
                Write_r = 1'b1;
                Set_BusB_To_r = bHigh;
              end
              3'b101: begin
                Set_Addr_To_r = aSP;
                Write_r = 1'b1;
                Set_BusB_To_r = bLow;
                Call_r = 1'b1;
              end
              default: ;
            endcase
          end
          6'b000101: begin // PUSH qq
            MCycles_r = 3'b011;
            case (MCycle)
              3'b001: Inc_PC_r = 1'b1;
              3'b010: begin
                Set_Addr_To_r = aSP;
                Write_r = 1'b1;
                Set_BusB_To_r = bHigh;
              end
              3'b011: begin
                Set_Addr_To_r = aSP;
                Write_r = 1'b1;
                Set_BusB_To_r = bLow;
              end
              default: ;
            endcase
          end
          6'b110011: begin // DI
            Inc_PC_r = 1'b1;
            Auto_Reset_IFF_r = 1'b1;
          end
          6'b111011: begin // EI
            Inc_PC_r = 1'b1;
            Set_IFF1_r = 1'b1;
            Set_IFF2_r = 1'b1;
          end
          default: begin
            case (IR[2:0])
              3'b000: begin // RET cc
                MCycles_r = 3'b011;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    Set_Addr_To_r = aSP;
                    LDZ_r = 1'b1;
                  end
                  3'b011: begin
                    Set_Addr_To_r = aSP;
                    LDW_r = 1'b1;
                    Jump_r = 1'b1;
                  end
                  default: ;
                endcase
              end
              3'b010: begin // JP cc, nn
                MCycles_r = 3'b011;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    LDZ_r = 1'b1;
                    Inc_PC_r = 1'b1;
                  end
                  3'b011: begin
                    LDW_r = 1'b1;
                    Jump_r = 1'b1;
                  end
                  default: ;
                endcase
              end
              3'b100: begin // CALL cc, nn
                MCycles_r = 3'b101;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    LDZ_r = 1'b1;
                    Inc_PC_r = 1'b1;
                  end
                  3'b011: begin
                    LDW_r = 1'b1;
                    Inc_PC_r = 1'b1;
                  end
                  3'b100: begin
                    Set_Addr_To_r = aSP;
                    Write_r = 1'b1;
                    Set_BusB_To_r = bHigh;
                  end
                  3'b101: begin
                    Set_Addr_To_r = aSP;
                    Write_r = 1'b1;
                    Set_BusB_To_r = bLow;
                    Call_r = 1'b1;
                  end
                  default: ;
                endcase
              end
              3'b110: begin // ALU A, n
                MCycles_r = 3'b010;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    Inc_PC_r = 1'b1;
                    Set_BusA_To_r = bA;
                    ALU_Op_r = {1'b0, IR[5:3]};
                    Save_ALU_r = 1'b1;
                    Read_To_Acc_r = 1'b1;
                  end
                  default: ;
                endcase
              end
              3'b111: begin // RST p
                MCycles_r = 3'b011;
                case (MCycle)
                  3'b001: Inc_PC_r = 1'b1;
                  3'b010: begin
                    Set_Addr_To_r = aSP;
                    Write_r = 1'b1;
                    Set_BusB_To_r = bHigh;
                  end
                  3'b011: begin
                    Set_Addr_To_r = aSP;
                    Write_r = 1'b1;
                    Set_BusB_To_r = bLow;
                    RstP_r = 1'b1;
                  end
                  default: ;
                endcase
              end
              default: ;
            endcase
          end
        endcase
      end
    endcase
    end
  end

endmodule
