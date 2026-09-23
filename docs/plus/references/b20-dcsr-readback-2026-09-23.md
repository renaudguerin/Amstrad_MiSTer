# B20-5: DCSR readback observation, 2026-09-23

**Disposition: retain RTL; B20-5 remains open for original-hardware adjudication.**
Register-boundary observation at `3b1b4117a7b0ceb26418087182c290508001f630`
finds live active-high DMA flags in both IVR modes. No settled hardware mismatch,
behavior change, fail-before claim, synthesis or MiSTer access results from this slice.

## Source assessment

The [source comparison](scrapes-interrupt-findings-2026-09-22.md), I6, identifies
S01 p.6's active-low wording and S22 p.7's automatic-mode freeze claim. Source IDs
and one-based PDF pages follow the [inventory](../../reference-ingestion/scrapes-2026-09-22.md).
Re-read the existing pdf-inspector Markdown for S01/S22/S28 (all native_complete,
no fallback pages), and visually checked S22 p.7 because its extraction interleaves
the table and prose. Its freeze claim explicitly includes bits 4–7; it provides
neither a read trace nor a frozen value or transition rule. Its EI/DI discussion
cannot identify whether peripheral flags change while CPU interrupts are masked.
S28 p.4 supplies write-clear advice, not a readback polarity measurement.

The original [Amstrad specification](../../specs/plus/_Arnold%20V_%20Specification%20-%20Issue%201.5%20-%2010th%20April%201990.md),
§§2.6–2.7, describes bits D6–D4 as set for active sound-channel interrupts and
D7 as the last raster acknowledge. This supports the existing active-high model
over S01's low-active wording; it does not establish automatic-mode readback.
[Kevin Thacker's hardware notes](../../specs/plus/Extra%20CPC%20Plus%20Hardware%20Information.md),
“DMA channels and interrupt control”, confirm channel-to-bit mapping and read
mirrors, without a polarity/freeze waveform. Their interrupt-vector section
separately reports problems with automatic clear. None of these observations
justifies inverting readback or inventing a frozen-value latch.

## Executed probe

Compiled actual `asic_regs.v` and `plus_sprite_ram.v` with Verilator 5.052;
reused the existing `Regs` bus driver without running its assertion suite.
For each channel and each IVR[0], reset, write IVR=`A0|mode`, enable all channels
with DCSR=`07`, inject one `dma_int_set` pulse, withhold acknowledge for 64 clocks,
then hold acknowledge across nine clocks plus diagnostic read clocks. Release it,
inject another request, then write the channel's W1C mask OR `07`.
All samples are page reads of offset `2C0F` (CPU address `6C0F`).

| IVR[0] | Channel | Before | Request / no ack | First / held / after ack | New request | W1C | Ack vector |
|---|---|---|---|---|---|---|---|
| 0 auto | 0 | 07 | 47 | 07 | 47 | 07 | A4 |
| 0 auto | 1 | 07 | 27 | 07 | 27 | 07 | A2 |
| 0 auto | 2 | 07 | 17 | 07 | 17 | 07 | A0 |
| 1 manual | 0 | 07 | 47 | 47 | 47 | 07 | A4 |
| 1 manual | 1 | 07 | 27 | 27 | 27 | 07 | A2 |
| 1 manual | 2 | 07 | 17 | 17 | 17 | 07 | A0 |

Values are hex observations, **not hardware expectations**. `dma_int_req` follows
nonzero DMA status; `vec_valid` is high only during acknowledge and the sampled
vector stays stable. Reads do not clear flags. Manual-mode re-request merely
reasserts an already-set flag. Automatic-mode request is visible before ack,
so a post-ack-only read would miss the distinction between live-and-cleared and
frozen readback.

This is a leaf-interface probe: it injects the DMA engine's request output and
ASIC acknowledge input, does not execute DMA instructions or T80 DI/EI, and does
not reproduce board acknowledge shaping. Withholding ack is not a claim about
CPU DI behavior. `int_pending` and `intack_raster` remain zero; bit 7 is therefore
zero throughout. B19 raster provenance/retention is preserved, not revalidated:
`asic_ga_timing.v` owns `last_raster`, latched on first acknowledge, while
`asic_regs.v` reflects it plus snapshot provenance. No collision priority or
multi-source arbitration conclusion is drawn (B20-6 / Eerie ownership).

## Remaining discriminator

On original Plus hardware, record DCSR before DMA INT, during pending request
with CPU acknowledgement inhibited, inside/after ISR acknowledgement, and after
explicit W1C, for each channel and both IVR modes. Also switch IVR mode while a
known flag is pending to distinguish forced value, retained value and live read.
Capture actual M1/IORQ acknowledgement separately from CPU DI/EI; control the A13
bug window using the B20-2/3 findings. Test raster provenance separately with a
raster acknowledge followed by a DMA acknowledge, preserving B19's accepted
behavior. Coordinate shared interrupt/DMA edits with Eerie after that evidence.

## Reproduction

The one-shot probe and build/trace logs are at `/tmp/b20-5-dcsr/` on the task host.
To reconstruct it, save this snippet as `/tmp/b20-5-dcsr/probe.cpp`:

```cpp
#define main existing_regs_suite_main
#include "asic_regs_test.cpp"
#undef main
int main(int argc, char** argv) {
 Verilated::commandArgs(argc, argv);
 for (unsigned mode=0; mode<2; ++mode) for(unsigned ch=0;ch<3;++ch) {
  Regs r;
  r.dut.intack=0; r.dut.int_pending=0; r.dut.intack_raster=0;
  r.reset_pulse(); r.wr(0x2805,0xA0|mode); r.wr(0x2C0F,7);
  auto sample=[&](const char* stage) {
   auto v=r.rd(0x2C0F);
   printf("mode=%u ch=%u %-18s DCSR=%02X req=%u vec=%02X valid=%u\n",mode,ch,stage,v,r.dut.dma_int_req,r.dut.vec_byte,r.dut.vec_valid);
  };
  auto request=[&]() {r.dut.dma_int_set=1u<<ch;r.tick();r.dut.dma_int_set=0;r.tick();};
  sample("before");request();sample("request");r.run(64);sample("no_ack_64_clocks");
  r.dut.intack=1;r.tick();sample("ack_first");r.run(8);sample("ack_held");
  r.dut.intack=0;r.tick();sample("after_ack");
  request();sample("new_request");r.wr(0x2C0F,(0x40u>>ch)|7);sample("write_clear");
 }
}
```

From the repository root:

```sh
verilator --cc --exe --build --language 1364-2001 --top-module asic_regs \
  --Mdir /tmp/b20-5-dcsr/obj -CFLAGS "-std=c++17 -O2 -I$PWD/sim/plus" \
  -o probe rtl/plus/asic_regs.v rtl/plus/plus_sprite_ram.v /tmp/b20-5-dcsr/probe.cpp
/tmp/b20-5-dcsr/obj/probe
```

Both build and execution exited 0. No disputed expectations were added to the
suite. The deliverable changes documentation only; selected simulation gate and
independent review are not required by repository policy.
