# Z80 INT sampling at the final T-state

The Z80 core must sample maskable INT on the rising edge **entering** the
instruction's final T-state. The instruction-boundary decision happens one
CPU clock later. The inherited T80 used the live pin at that later decision,
so a request arriving during the final T-state could interrupt an instruction
which should finish without accepting it; a short request covering the proper
sampling edge could be missed.

## Evidence and scope

[Zilog UM008011-0816](https://www.zilog.com/docs/z80/um0080.pdf), printed p12 and
Figure 9 p13, distinguishes those edges. Its HALT discussion and Figure 11
(pp14–15) corroborate the distinction. Gemini independently inspected the
rendered figures (`20260928T052709Z-73621-8f7e`); Opus independently checked the
T80pa physical-edge mapping and later inspected the diagrams alongside the
French ACCC (`20260928T054056Z-79675-eda6`). M1, memory strobes and refresh
anchor the physical phase; internal T-state names alone are insufficient.

[Upstream commit 670437e](https://github.com/mist-devel/T80/commit/670437ea23f09f993df9cf85021b0e2d20e01b9f)
removed the enabled-clock INT register in July 2018, alongside interrupt-acknowledge
and WAIT changes. This is inherited behaviour, not an Amstrad-local regression.
The commit does not explain the sampling-edge choice separately. The related
[TV80 core](https://github.com/hutch31/tv80/blob/master/rtl/core/tv80_core.v)
still registers INT before the instruction-boundary decision.

The correction registers INT on each enabled CPU rising edge and uses the
preceding sample at `T_Res`. Existing EI, prefix and NMI-priority gates remain.
Reset and `DIRSet` discard the sampled level; clock-enable holds retain it.
The change applies to Z80 modes 0/1 with R800 mode off. The unverified 8080,
Game Boy and R800 policies remain unchanged. There is no machine-specific CPU
pin delay or Plus-only CPU behaviour switch.

## Deterministic proof

`sim/t80_irq_sample_test.cpp` locates the following M1 edge with INT inactive.
The source-derived sampling edge is one CPU period earlier. Early/late pulses
provide eight master ticks of setup/hold around the respective candidate edge.
The cases cover untaken RET NC, multicycle LD A,(HL), and a WAIT-stretched RET.
A following DI prevents a later legitimate interrupt contaminating a late-pulse
negative result. Memory reads take priority over the wrapper's undefined initial
IORQ level, matching the production motherboard harnesses.

The final test source, compiled against a preserved pre-change production T80
netlist, reports **8 passed, 6 failed**: precisely the three early/late pairs.
Against the corrected production netlist it reports **14 passed, 0 failed**:

```sh
make -C sim t80-irq-sample-test CXX=/opt/homebrew/opt/llvm/bin/clang++
```

Preservation controls cover held requests, EI deferral, DD-prefix completion,
HALT wake, IM2 vector reads and NMI-before-INT delivery. Snapshot restoration is
checked through the existing production motherboard suite rather than a new
artificial DIRSet protocol. Focused results on the correction:

- `t80-freeze-test`: 18 cases pass, including held clocks and WAIT equivalence.
- `b18-snapshot-test`: six classic capture/restore and six host publication cases pass.
- `t80-trace-test`: native VHDL/translated Verilog agree across 139 cycles.
- `crtc-t80-test`: four executed-instruction phase cases pass.
- `sim/plus b20-ack-matrix`: all NOP/HALT/LDIR/WAIT cells pass; withdrawal races
  remain explicitly observational in that bench.

## Classic CPC acceptance

The synchronous `syncgen_sync` is the production GA interrupt generator;
`syncgen.v` is a simulation comparison shadow. The CPU correction can change
classic instruction acceptance and must not be certified solely by existing
suite results.

French ACCC v1.11 §27.7.2, pp289–290, distinguishes robust NOP/HALT timing from
instructions whose final T-state coincides with the request window. Opus's
source review predicts that correcting the CPU improves the sensitive untaken
RET NC case. It also raises a separate, unverified question about the classic
CRTC/GA output phase; that is not authority to alter the netlist-derived GA.
The ADD diagrams have an inconsistent Wait-row label, so their T-cycle rows
and prose carry the interpretation, with the uncertainty retained.

The actual-CRTC regression runs type 0 and type 1 with production GA40010,
normal WAIT and T80pa. An uninterrupted calibration masks only the second
request at the CPU pin; observed M1 boundaries plus the Zilog sampling rule
derive the permitted acknowledge edge. No absolute simulator timestamp is an
expectation. NOP/HALT are robust controls; one NOP of padding exercises both
untaken RET NC phases. The final source reports **6 passed, 2 failed** against
the pre-change T80 (both sensitive RET phases), and **8 passed, 0 failed**
after correction:

```sh
make -C sim classic-irq-phase-test CXX=/opt/homebrew/opt/llvm/bin/clang++
```

Measured against N (the uninterrupted NOP T2 rising edge), type0 HSYNC/INT
fall at N−7/N−6 master ticks; type1 at N−3/N−2. Both are visible to the CPU
at N. These are model measurements, not original-CPC measurements. They
support Opus's conclusion that the CPU correction improves the sensitive RET
case rather than cancelling a late GA request. The possible earlier-than-hardware
classic GA phase remains separate [B25 debt](backlog.md#b25-classic-ga-interrupt-phase-against-cpu-edges), particularly for INC HL timing.
Fresh final code review and the final selection gate are recorded in the PA7
follow-up.
No original classic CPC pin trace or RET NC parity-pair photograph has been
collected. CRTC 1 unit-to-unit output timing remains hardware debt. The Plus V5
photographs support software-visible timing but do not identify a unique raw
ASIC delay or independently measure classic CPC timing.

Private reproductions, primary-source page renders and logs are under ignored
`output_files/plus-hw-probes/pa7-followup/classic-evidence/`. Do not commit the
user-owned ACCC PDF or page images.
