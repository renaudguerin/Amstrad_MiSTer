//
// TV80 8-Bit Microprocessor Core
// Based on the T80 core by Daniel Wallner (jesus@opencores.org)
//
// Copyright (c) 2004 Guy Hutchison (ghutchis@opencores.org)
//

`timescale 1ns / 1ps

module tv80_core #(
  parameter Mode = 0,
  parameter IOWait = 1
) (
  input          reset_n,
  input          clk,
  input          cen,
  input          wait_n,
  input          int_n,
  input          nmi_n,
  input          busrq_n,
  output         m1_n,
  output         iorq,
  output         noread,
  output         write,
  output reg     rfsh_n,
  output         halt_n,
  output reg     busak_n,
  output [15:0]  A,
  input  [7:0]   DInst,
  input  [7:0]   DI,
  output [7:0]   DO,
  output [2:0]   MC,
  output [2:0]   TS,
  input          out0,
  input          r800_mode,
  output         intcycle_n,
  input          DIRSet,
  input  [211:0] DIR,
  output [211:0] REG
);

  reg [2:0]  tstate;
  reg [2:0]  mcycle;
  reg [7:0]  ir;
  reg [15:0] pc;
  reg [15:0] sp;
  reg [15:0] wz;
  reg [7:0]  acc;
  reg [7:0]  flags;
  reg        intcycle_n_r;
  reg        iff1, iff2;
  reg [1:0]  im_mode;
  reg        halted;
  reg [2:0]  prefix_reg;
  // High while the byte fetched in the M1 just ending was itself a DD/ED/FD/CB
  // prefix, so the end-of-instruction handler keeps prefix_reg instead of
  // clearing it before the prefixed opcode has been fetched.
  reg        prefix_new;

  wire [2:0]  mc_max;
  wire [2:0]  ts_max;
  wire [2:0]  prefix;
  wire        inc_pc;
  wire        dec_pc;
  wire        inc_wz;
  wire        incdec_16;
  wire [2:0]  set_addr_to;
  wire        jump;
  wire        jump_e;
  wire        jump_xy;
  wire        call;
  wire        rst_p;
  wire        ldz;
  wire        ldw;
  wire        ldsphl;
  wire        mc_iorq;
  wire        special_g_n;
  wire        exchange_af;
  wire        exchange_rs;
  wire        exchange;
  wire        read_to_reg;
  wire        read_to_acc;
  wire [3:0]  set_bus_a;
  wire [3:0]  set_bus_b;
  wire [3:0]  alu_op;
  wire        save_alu;
  wire        preserve_c_n;
  wire        arith_16;
  wire        set_iff1;
  wire        set_iff2;
  wire        auto_reset_iff;
  wire [1:0]  set_im;
  wire        rot_int;
  wire        rot_int_n;
  wire        mc_write;
  wire        mc_noread;
  wire        read_f;
  wire        write_f;
  wire        tstate_wait_n;
  wire        auto_reset_pc;
  wire        special_g_reset_n;
  wire        reti_n;

  tv80_mcode mcode (
    .Mode(3'd0),
    .IR(ir),
    .MCycle(mcycle),
    .Prefix_In(prefix_reg),
    .MCycles(mc_max),
    .TStates(ts_max),
    .Prefix(prefix),
    .Inc_PC(inc_pc),
    .Dec_PC(dec_pc),
    .Inc_WZ(inc_wz),
    .IncDec_16(incdec_16),
    .Set_Addr_To(set_addr_to),
    .Jump(jump),
    .JumpE(jump_e),
    .JumpXY(jump_xy),
    .Call(call),
    .RstP(rst_p),
    .LDZ(ldz),
    .LDW(ldw),
    .LDSPHL(ldsphl),
    .IORQ(mc_iorq),
    .Special_G_N(special_g_n),
    .ExchangeAF(exchange_af),
    .ExchangeRS(exchange_rs),
    .Exchange(exchange),
    .Read_To_Reg(read_to_reg),
    .Read_To_Acc(read_to_acc),
    .Set_BusA_To(set_bus_a),
    .Set_BusB_To(set_bus_b),
    .ALU_Op(alu_op),
    .Save_ALU(save_alu),
    .PreserveC_N(preserve_c_n),
    .Arith16(arith_16),
    .Set_IFF1(set_iff1),
    .Set_IFF2(set_iff2),
    .Auto_Reset_IFF(auto_reset_iff),
    .Set_IM(set_im),
    .Rot_Int(rot_int),
    .Rot_Int_N(rot_int_n),
    .Write(mc_write),
    .NoRead(mc_noread),
    .Read_F(read_f),
    .Write_F(write_f),
    .TState_Wait_N(tstate_wait_n),
    .Auto_Reset_PC(auto_reset_pc),
    .Special_G_Reset_N(special_g_reset_n),
    .RetI_N(reti_n)
  );

  wire [7:0] alu_q;
  wire [7:0] alu_f;
  reg  [7:0] bus_a_val;
  reg  [7:0] bus_b_val;

  wire [7:0]  reg_doa, reg_dob, reg_doc;
  wire [15:0] reg_bc, reg_de, reg_hl;
  wire [7:0]  reg_a_out, reg_f_out;

  wire is_ld_dd_nn = (ir[7:6] == 2'b00) && (ir[3:0] == 4'b0001);
  wire write16_l = is_ld_dd_nn && (mcycle == 3'b010) && (tstate == 3'b011) && (ir[5:4] != 2'b11);
  wire write16_h = is_ld_dd_nn && (mcycle == 3'b011) && (tstate == 3'b011) && (ir[5:4] != 2'b11);

  tv80_reg registers (
    .clk(clk),
    .reset_n(reset_n),
    .cen(cen),
    .AddrA(ir[5:3]),
    .AddrB(ir[2:0]),
    .AddrC(3'b111),
    .Write(read_to_reg && (tstate == 3'b011) && !is_ld_dd_nn),
    .DI(save_alu ? alu_q : DI),
    .DOA(reg_doa),
    .DOB(reg_dob),
    .DOC(reg_doc),
    .BC(reg_bc),
    .DE(reg_de),
    .HL(reg_hl),
    .A_out(reg_a_out),
    .F_out(reg_f_out),
    .ExchangeAF(exchange_af && (tstate == 3'b011)),
    .ExchangeRS(exchange_rs && (tstate == 3'b011)),
    .Exchange(exchange && (tstate == 3'b011)),
    .Write16_Pair(ir[5:4]),
    .Write16_L(write16_l),
    .Write16_H(write16_h),
    .DIRSet(DIRSet),
    .DIR(DIR),
    .REG(REG)
  );

  tv80_alu alu (
    .ALU_Op(alu_op),
    .BusA(bus_a_val),
    .BusB(bus_b_val),
    .F_In(flags),
    .IR(ir[5:3]),
    .Arith16(arith_16),
    .Q(alu_q),
    .F_Out(alu_f)
  );

  always @* begin
    case (set_bus_a)
      4'b1000: bus_a_val = acc;
      4'b0001: bus_a_val = reg_doa;
      default: bus_a_val = reg_doa;
    endcase

    case (set_bus_b)
      4'b1000: bus_b_val = acc;
      4'b1100: bus_b_val = wz[15:8];
      4'b1101: bus_b_val = wz[7:0];
      default: bus_b_val = reg_dob;
    endcase
  end

  // Combinational bus outputs
  reg [15:0] addr_comb;
  reg [7:0]  data_comb;

  always @* begin
    if (mcycle == 3'b001) begin
      addr_comb = pc;
    end else begin
      case (set_addr_to)
        3'b000: addr_comb = pc;
        3'b001: addr_comb = reg_bc;
        3'b010: addr_comb = reg_de;
        3'b011: addr_comb = reg_hl;
        3'b100: addr_comb = sp;
        3'b110: addr_comb = wz;
        default: addr_comb = pc;
      endcase
    end

    case (set_bus_b)
      4'b0001: data_comb = reg_bc[15:8]; // bB
      4'b0010: data_comb = reg_bc[7:0];  // bC
      4'b0011: data_comb = reg_de[15:8]; // bD
      4'b0100: data_comb = reg_de[7:0];  // bE
      4'b0101: data_comb = reg_hl[15:8]; // bH
      4'b0110: data_comb = reg_hl[7:0];  // bL
      4'b1000: data_comb = acc;          // bA
      default: data_comb = reg_dob;
    endcase
  end

  assign A          = addr_comb;
  assign DO         = data_comb;
  assign MC         = mcycle;
  assign TS         = tstate;
  assign iorq       = (mcycle != 3'b001) && mc_iorq;
  assign write      = (mcycle != 3'b001) && mc_write;
  assign noread     = (mcycle != 3'b001) && mc_noread;
  assign intcycle_n = intcycle_n_r;
  assign m1_n       = (mcycle != 3'b001);
  assign halt_n     = ~halted;

  // The instruction ends on this CEN edge: the last M-cycle has reached its
  // final T-state. M1 ends at T4, every other M-cycle at T3.
  wire instr_end = ((tstate == 3'b011) && (mcycle != 3'b001) && (mcycle >= mc_max)) ||
                   ((tstate == 3'b100) && (mcycle >= mc_max));

  // Instruction sequencer & T-state / M-cycle state machine.
  //
  // T-state phasing follows T80.vhd, because T80pa's bus timing assumes it:
  //   * reset parks the counter at T0, so the first CEN edge produces a real
  //     T1. The wrapper only asserts MREQ/RD on the CEN_n edge *inside* T1;
  //     starting at T1 would skip that edge and the first opcode fetch would
  //     never drive the bus.
  //   * the opcode is latched at T2 ("DInst valid at beginning of T3" in
  //     T80pa). The wrapper drops MREQ/RD as T2 ends, and the Amstrad data
  //     mux stops sourcing the fetched byte one clk later, so latching at the
  //     end of T3 would sample open bus. T3 and T4 therefore see the decode
  //     of the byte this M1 fetched, which is what makes single-M-cycle
  //     instructions and the DD/ED/FD/CB prefixes work at all.
  always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
      tstate       <= 3'b000;
      mcycle       <= 3'b001;
      ir           <= 8'h00;
      pc           <= 16'h0000;
      sp           <= 16'hFFFF;
      wz           <= 16'h0000;
      acc          <= 8'hFF;
      flags        <= 8'hFF;
      intcycle_n_r <= 1'b1;
      rfsh_n       <= 1'b1;
      busak_n      <= 1'b1;
      iff1         <= 1'b0;
      iff2         <= 1'b0;
      im_mode      <= 2'b00;
      halted       <= 1'b0;
      prefix_reg   <= 3'b000;
      prefix_new   <= 1'b0;
    end else if (DIRSet) begin
      pc <= DIR[79:64];
      sp <= DIR[63:48];
      acc <= DIR[7:0];
      flags <= DIR[15:8];
    end else if (cen) begin
      case (tstate)
        3'b001: tstate <= 3'b010;
        3'b010: begin
          tstate <= 3'b011;
          if (mcycle == 3'b001) begin
            if (!intcycle_n_r) begin
              // Interrupt acknowledge M1: no opcode is fetched and PC stays
              // where it was. IFF1/IFF2 were already cleared on acceptance.
              ir     <= 8'h00;
              halted <= 1'b0;
            end else begin
              ir <= DInst;
              // HALT parks PC on its own opcode and keeps refetching it until
              // an interrupt breaks out, instead of running into whatever
              // follows.
              if (DInst == 8'h76) halted <= 1'b1;
              else                pc     <= pc + 16'd1;
            end
          end
        end
        3'b011: begin
          if (mcycle == 3'b001) begin
            // IR now holds the byte this M1 fetched, so the microcode's
            // Prefix output belongs to it.
            if (prefix != 3'b000 && prefix_reg == 3'b000) begin
              prefix_reg <= prefix;
              prefix_new <= 1'b1;
            end else begin
              prefix_new <= 1'b0;
            end
            tstate <= 3'b100;
          end else begin
            if (inc_pc) pc <= pc + 16'd1;
            if (dec_pc) pc <= pc - 16'd1;
            if (mcycle < mc_max) begin
              mcycle <= mcycle + 3'd1;
              tstate <= 3'b001;
            end else begin
              mcycle <= 3'b001;
              tstate <= 3'b001;
            end
          end
        end
        3'b100: begin
          if (mcycle < mc_max) begin
            mcycle <= mcycle + 3'd1;
            tstate <= 3'b001;
          end else begin
            mcycle <= 3'b001;
            tstate <= 3'b001;
          end
        end
        default: tstate <= 3'b001;
      endcase

      // Instruction actions at T3
      if (tstate == 3'b011) begin
        if (ldz) wz[7:0] <= DI;
        if (ldw) wz[15:8] <= DI;
        if (is_ld_dd_nn && (ir[5:4] == 2'b11)) begin
          if (mcycle == 3'b010) sp[7:0] <= DI;
          if (mcycle == 3'b011) sp[15:8] <= DI;
        end
        if (jump) pc <= wz;
        if (call) pc <= wz;
        // ACC is the accumulator of record: BusA/BusB source it for bA and the
        // OUT/store paths read it. A register-file write aimed at A (LD A,n,
        // INC A, ...) has to land here too or those paths see a stale byte.
        if (read_to_acc ||
            (read_to_reg && !is_ld_dd_nn && (ir[5:3] == 3'b111)))
          acc <= save_alu ? alu_q : DI;
        if (save_alu) begin
          flags <= alu_f;
          if (read_to_acc) acc <= alu_q;
        end
        if (set_iff1) iff1 <= 1'b1;
        if (set_iff2) iff2 <= 1'b1;
        if (auto_reset_iff) begin
          iff1 <= 1'b0;
          iff2 <= 1'b0;
        end
        if (set_im != 2'b00) im_mode <= set_im;
      end

      // End of instruction: prefix bookkeeping and interrupt acceptance.
      if (instr_end) begin
        if (prefix_new) begin
          // The M1 that just ended fetched the prefix byte itself; the
          // prefixed opcode has not been seen yet, so hold prefix_reg and do
          // not let an interrupt split the pair.
          prefix_new <= 1'b0;
        end else begin
          if (prefix == 3'b000) prefix_reg <= 3'b000;

          if (!intcycle_n_r) begin
            // IM 1 style restart. This harness core does not model the
            // interrupt stack push: no image it runs returns from a handler,
            // and the P10 boot vectors only observe the acknowledge cycle.
            pc           <= 16'h0038;
            intcycle_n_r <= 1'b1;
          end
          // T80 defers acceptance across EI and across a prefix byte.
          else if (iff1 && !int_n && (prefix == 3'b000) && !set_iff1) begin
            intcycle_n_r <= 1'b0;
            iff1         <= 1'b0;
            iff2         <= 1'b0;
            halted       <= 1'b0;
          end
        end
      end
    end
  end

endmodule
