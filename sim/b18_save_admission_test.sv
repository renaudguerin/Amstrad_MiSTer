// The adapter injects the exact save_admit RHS from Amstrad.sv.
module b18_save_admission_test;
    timeunit 1ns;
    timeprecision 1ns;

    reg clk = 1'b0;
    reg reset = 1'b0;
    reg ioctl_download = 1'b0;
    reg sna_hold = 1'b0;
    reg sna_load = 1'b0;
    reg plus_mode = 1'b0;
    reg mf2_en = 1'b0;
    reg cart_mem_req = 1'b0;
    reg dan_download = 1'b0;
    reg dan_detach = 1'b0;
    reg dan_nce = 1'b1;
    reg save_req = 1'b0;
    reg rampage_ok = 1'b1;
    reg release_req = 1'b0;
    reg insn_start = 1'b0;
    reg halt_n = 1'b1;
    reg [211:0] cpu_reg = 212'd0;

    wire dan_eeprom_loaded;
    wire dan_ena;
    wire save_hold;
    wire captured;
    wire refused;
    wire cancelled;
    wire busy;
    wire save_admit_out;
    integer failures = 0;

    plus_legacy_cart_gate legacy_cart_gate (
        .clk(clk),
        .plus_mode(plus_mode),
        .dandanator_download(dan_download),
        .dandanator_detach(dan_detach),
        .dandanator_nce(dan_nce),
        .dandanator_loaded(dan_eeprom_loaded),
        .dandanator_active(dan_ena)
    );

    // Replaced by prepare_b18_save_admission.py using the production RHS.
    wire save_admit = 1'b0;
    assign save_admit_out = save_admit;

    sna_save_capture save_capture (
        .clk(clk),
        .reset(1'b0),
        .save_req(save_req),
        .admit(save_admit),
        .rampage_ok(rampage_ok),
        .release_req(release_req),
        .insn_start(insn_start),
        .halt_n(halt_n),
        .cpu_reg(cpu_reg),
        .hw_hdr({1080{1'b0}}),
        .hold(save_hold),
        .captured(captured),
        .refused(refused),
        .cancelled(cancelled),
        .busy(busy),
        .header()
    );

    task automatic step;
        #1 clk = 1'b0;
        #1 clk = 1'b1;
        #1 clk = 1'b0;
        #1;
    endtask

    task automatic check(input logic condition, input string message);
        if (!condition) begin
            $display("FAIL B18 save admission: %s", message);
            failures = failures + 1;
        end
    endtask

    task automatic request_save;
        save_req = 1'b1;
        step();
        save_req = 1'b0;
        #1;
    endtask

    task automatic instruction_boundary;
        insn_start = 1'b1;
        step();
        insn_start = 1'b0;
        #1;
    endtask

    task automatic load_cartridge;
        dan_download = 1'b1;
        step();
        dan_download = 1'b0;
        step();
    endtask

    task automatic detach_cartridge;
        dan_detach = 1'b1;
        step();
        dan_detach = 1'b0;
        step();
    endtask

    task automatic release_save;
        release_req = 1'b1;
        step();
        release_req = 1'b0;
        step();
    endtask

    task automatic check_control(input string label);
        check(!save_admit_out, {label, " blocks save admission"});
    endtask

    initial begin
        // A normal empty classic machine remains eligible and captures at the
        // next instruction boundary.
        request_save();
        check(busy, "eligible classic request must arm");
        instruction_boundary();
        check(captured && save_hold, "ordinary classic save must capture");
        release_save();

        // Complete a Dandanator download while a save is armed, with /NCE
        // inactive. The attachment transition must cancel the pending save;
        // a later boundary must not capture it.
        request_save();
        check(busy, "empty classic machine must arm before cartridge appears");
        load_cartridge();
        check(dan_eeprom_loaded, "download falling edge must mark cartridge loaded");
        step();
        check(cancelled && !busy, "new cartridge attachment must cancel armed save");
        instruction_boundary();
        check(!captured && !save_hold, "later boundary must not capture canceled save");

        release_save(); // Isolate later cases even if cancellation regresses.

        // Both selected and unselected loaded cartridges block admission.
        dan_nce = 1'b1;
        #1;
        check(!dan_ena, "inactive Dandanator select must leave dan_ena low");
        check(!save_admit_out, "loaded cartridge blocks admission with inactive select");
        request_save();
        check(refused && !busy, "loaded unselected cartridge save must be refused");

        instruction_boundary();
        check(!captured && !save_hold, "refused unselected request must not capture");
        release_save();

        dan_nce = 1'b0;
        #1;
        check(dan_ena, "selected loaded Dandanator must assert dan_ena");
        check(!save_admit_out, "loaded cartridge blocks admission with active select");
        request_save();
        check(refused && !busy, "loaded selected cartridge save must be refused");

        // Explicit detach clears attachment state and restores classic saves.
        detach_cartridge();
        check(!dan_eeprom_loaded, "explicit detach must clear loaded state");
        check(save_admit_out, "detached classic machine should admit saving");
        request_save();
        instruction_boundary();
        check(captured && save_hold, "save must capture after explicit detach");
        release_save();

        plus_mode = 1'b1;
        #1;
        check_control("Plus mode");
        request_save();
        check(refused && !busy, "Plus save request must be refused");
        plus_mode = 1'b0;

        reset = 1'b1; #1; check_control("reset"); reset = 1'b0;
        ioctl_download = 1'b1; #1; check_control("download"); ioctl_download = 1'b0;
        sna_hold = 1'b1; #1; check_control("snapshot hold"); sna_hold = 1'b0;
        sna_load = 1'b1; #1; check_control("snapshot load"); sna_load = 1'b0;
        mf2_en = 1'b1; #1; check_control("Multiface"); mf2_en = 1'b0;
        cart_mem_req = 1'b1; #1; check_control("cartridge request"); cart_mem_req = 1'b0;

        if (failures != 0) begin
            $display("FAIL B18 save admission: %0d assertion(s)", failures);
            $fatal(1);
        end
        $display("PASS B18 save admission: attachment state, cancellation and classic controls");
        $finish;
    end
endmodule
