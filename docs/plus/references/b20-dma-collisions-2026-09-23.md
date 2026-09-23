# B20-6: connected DMA collision observation, 2026-09-23

**Disposition: retain production RTL; B20-6 remains open for hardware adjudication.**
Observed `asic_regs` connected to `asic_dma` at `f803e98e5c77b43e231876169bba21863cc994a8`.
No established behavioral defect, fail-before vector, production edit, MiSTer access,
synthesis or hardware-acceptance claim. The existing register comment is accurate;
this task corrects stale backlog/source-comparison prose only and records the observation.

## Source boundary

The [register decode and update](https://github.com/renaudguerin/Amstrad_MiSTer/blob/f803e98e5c77b43e231876169bba21863cc994a8/rtl/plus/asic_regs.v#L386-L470)
applies W1C and automatic-clear masks after OR-setting DMA requests. W1C is a bus
level, not a write-edge event. Automatic clear requires rising `intack`, IVR[0]=0,
and no raster pending; its channel mask comes from **old pending flags**, not the
new request. Thus “clear-dominant” does not mean an empty acknowledge clears a
request first arriving on that edge. The comment at lines 442–446 already correctly
describes implementation priority without claiming the hardware rule.

The [DMA engine](https://github.com/renaudguerin/Amstrad_MiSTer/blob/f803e98e5c77b43e231876169bba21863cc994a8/rtl/plus/asic_dma.v#L253-L256)
default-clears its INT/STOP outputs every master clock and sets the selected bit
when executing control instructions (channel 0 lines 445–451, channel 1 598–604,
channel 2 737–743). The connected register bank consumes those registered pulses
on the following edge. Its enable assignment masks STOP after either CPU data or
retained enables. A held CPU enable write therefore supplies the enable again
once the STOP pulse has ended.

The [motherboard](https://github.com/renaudguerin/Amstrad_MiSTer/blob/f803e98e5c77b43e231876169bba21863cc994a8/rtl/Amstrad_motherboard.v#L645-L708)
wires these feedback signals directly; `mem_wr = ~(WR_n | MREQ_n)` at line 257
has no DCSR one-shot conversion. Acknowledge is `plus_mode & ~M1_n & iorq`.
This establishes the level path, not whether each synthetic collision is reachable
under production T80, RAM arbitration and clock phasing.

[Amstrad's specification](../../specs/plus/_Arnold%20V_%20Specification%20-%20Issue%201.5%20-%2010th%20April%201990.md),
§2.6, defines STOP, CPU enable writes and W1C separately; the cited passage does
not specify simultaneous priority or the write sampling edge. The existing
[source comparison](scrapes-interrupt-findings-2026-09-22.md) and
[B20-5 source assessment](b20-dcsr-readback-2026-09-23.md) do not settle those
questions either. A model observation alone cannot supply the missing rule.

## Connected observation

Verilator 5.052 compiled the unchanged `asic_regs.v`, `plus_sprite_ram.v` and
`asic_dma.v`. All SAR/PPR bytes and write strobes, enables, INT and STOP feedback
were connected directly between register bank and DMA. Other inputs were zero
except clock/reset, the CPU page bus, HSYNC, DMA RAM data and positive CCLK enable
(one per 16 master clocks). No force, internal state assignment or injected DMA
request was used. RAM returned constant `4010` (INT) or `4020` (STOP); actual
fetch/execute sequencing generated each pulse. This is a module-boundary probe,
not a motherboard/T80 or memory-latency simulation. Raster inputs were zero.

For each channel, reset, write IVR=00 and enable only that channel. Launch a DMA
iteration by raising HSYNC for one master edge; wait for the relevant output
pulse, then arrange CPU write/ack before the following consuming edge. Allow 256
master clocks between iterations. W1C uses `(40 >> channel) | (1 << channel)`.
Held-write cases begin four clocks before launching the DMA iteration and retain
the bus level through pulse consumption. Values below are observed DCSR hex,
**not hardware expectations**.

| Event sampled after register edge | Ch0 | Ch1 | Ch2 |
|---|---|---|---|
| INT + matching W1C, same edge | 01 | 02 | 04 |
| INT arriving later inside held W1C | 01 | 02 | 04 |
| New INT after W1C released | 41 | 22 | 14 |
| Existing pending flag + new INT + first auto ack | 01 | 02 | 04 |
| Another INT while that ack remains held | 41 | 22 | 14 |
| No old flag + new INT + first auto ack | 41 | 22 | 14 |
| STOP + enable write, same edge | 00 | 00 | 00 |
| Next edge after releasing that write | 00 | 00 | 00 |
| STOP during held enable write | 00 | 00 | 00 |
| Next edge with enable write still held | 01 | 02 | 04 |

W1C continuously masks later requests while decoded; automatic acknowledge does
not repeatedly clear them. STOP clears the enable for one edge, but does not
prevent a still-decoded enable write from restoring it. No all-channel arbitration,
raster collision, combined INT|STOP, reset/snapshot collision, or second-ack bug
claim follows from this single-channel matrix.

One-shot generator, generated wrapper and logs reside at `/tmp/b20-6/` on the
task host (`generate.py`, `probe.sv`, `build.log`, `trace.log`). Executed command:

```sh
verilator --binary --timing -Wno-fatal --top-module probe --Mdir /tmp/b20-6/obj /tmp/b20-6/probe.sv rtl/plus/asic_regs.v rtl/plus/plus_sprite_ram.v rtl/plus/asic_dma.v
/tmp/b20-6/obj/Vprobe
```

Both commands exited zero; all 30 observations completed. This temporary probe
is not a committed regression bench or an expected-failure hardware vector.
Documentation-only delivery requires `git diff --check`, not a simulation gate
or an independent code review.

## Remaining discriminator

On original Plus hardware, phase a same-channel INT around DCSR W1C assertion,
interior and release, and around first/held automatic acknowledge with and without
an old pending flag. Phase STOP similarly against an enable write; observe whether
one CPU transaction can restore the enable after STOP. Record bus edges and flag/
enable transitions, not only a later ISR read. Establish production CPU reachability
before choosing a repair. No recommendation to edge-qualify writes or invert priority
is justified yet. Eerie retains ownership of interrupt RTL and hardware work; report
a source-backed failing vector and proposed correction to its coordinator first.
