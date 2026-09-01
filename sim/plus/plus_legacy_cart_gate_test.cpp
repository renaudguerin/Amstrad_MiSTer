#include "Vplus_legacy_cart_gate.h"
#include "verilated.h"

#include <cstdlib>
#include <iostream>

namespace {

void expect(bool condition, const char *message) {
	if (!condition) {
		std::cerr << "FAIL legacy-cart lifecycle: " << message << '\n';
		std::exit(1);
	}
}

void tick(Vplus_legacy_cart_gate &dut) {
	dut.clk = 0;
	dut.eval();
	dut.clk = 1;
	dut.eval();
}

} // namespace

int main(int argc, char **argv) {
	Verilated::commandArgs(argc, argv);
	Vplus_legacy_cart_gate dut;

	// FPGA configuration starts without an uploaded Dandanator image.
	dut.clk = 0;
	dut.plus_mode = 0;
	dut.dandanator_download = 0;
	dut.dandanator_detach = 0;
	dut.dandanator_nce = 0;
	dut.eval();
	expect(!dut.dandanator_loaded,
	       "configuration must start without a loaded classic cartridge");
	expect(!dut.dandanator_active,
	       "an unpopulated classic cartridge must not own SDRAM");

	// The production lifecycle marks an image loaded on the download falling
	// edge. Ordinary machine resets are deliberately not inputs here: physical
	// expansion hardware survives a soft reset.
	dut.dandanator_download = 1;
	tick(dut);
	dut.dandanator_download = 0;
	tick(dut);
	expect(dut.dandanator_loaded,
	       "a completed Dandanator download must persist its loaded state");
	expect(dut.dandanator_active,
	       "a loaded classic cartridge must remain available in classic mode");

	// Hardware B13 lifecycle discriminator: after that persistent mapping, the
	// user selects a Plus machine and loads/applies a CPR. The selected machine
	// must own its cartridge path even though the classic image remains loaded.
	dut.plus_mode = 1;
	dut.eval();
	expect(dut.dandanator_loaded,
	       "selecting Plus must preserve the classic image for a later switch back");
	expect(!dut.dandanator_active,
	       "persistent classic cartridge state poisoned Plus CPR memory ownership");

	// Returning to classic mode is a machine switch, not an implicit eject.
	dut.plus_mode = 0;
	dut.eval();
	expect(dut.dandanator_active,
	       "Plus isolation must preserve the loaded classic cartridge image");

	dut.dandanator_nce = 1;
	dut.eval();
	expect(!dut.dandanator_active,
	       "an unselected classic cartridge must release SDRAM");

	// Explicit detach is the only runtime unload boundary. It clears the image
	// state on the rising edge and remains clear after the menu signal drops.
	dut.dandanator_detach = 1;
	tick(dut);
	dut.dandanator_detach = 0;
	tick(dut);
	expect(!dut.dandanator_loaded && !dut.dandanator_active,
	       "explicit detach must clear persistent classic cartridge state");

	std::cout << "PASS legacy-cart lifecycle: persistent classic state is Plus-isolated\n";
	return 0;
}
