# CRTC1 VSYNC clock: pre-existing-pulse discriminator

A small original diagnostic narrows the AI/008D question without selecting
an RTL rule. Current production-T80/CRTC simulation lets a pre-existing
VSYNC expire on R0=0. Fresh AmSpirit keeps the three sampled bits high.
Both expire at R0=1 even with R2 unreachable. This distinguishes a general
recurring-HSYNC-only explanation from the zero-length case, subject to
the input and software-reference limits below. Physical-CPC results are
still required.

## Input and measured results

Each case restores ordinary R0=63/R2=46/R3=11/R4=38/R5=0/R6=25/R7=10/
R8=0/R9=7. The guest waits for raw PPI.B bit0 low then high, raises R4 to127
and R9 to31 to prevent a new C4=R7 visit during measurement, writes the
target R2, then writes R0 last. It does not rewrite R7 during the pulse.
There is no SHAKER code, firmware call after entry, counter patch, host
release or phase search. The diagnostic prints observations without a
PASS/FAIL label.

| Target R0 / R2 | Current production model | AmSpirit1.17.1 | Physical CPC |
|---|---|---|---|
| 63 /46, ordinary control | 111 | 111 | Pending |
| 1 /1, reachable HSYNC | 000 | 000 | Pending |
| 1 /63, unreachable after wrap | 000 | 000 | Pending |
| 0 /63, unreachable after wrap | 000 | 111 | Pending |

The original [AI caller](shaker-crtc1-008d-current-2026-11-05.md) separately
has physical1111 versus current canonical1000; its direct208 fourth bit
was1. Neither sparse set proves uninterrupted physical VSYNC between reads.

## Production-T80 qualification

The exact diagnostic runs through the existing production motherboard,
CPU/READY, CRTC and PPI model from c0939d8. A reset ROM contains only a
jump to the diagnostic at4000; the diagnostic owns its initialization.
There is no SNA or omitted-state restoration. All twelve real IN/store
identities qualify with stable VSYNC during each read, sequential result
addresses and normal RAM mapping. VSYNC is active at each target R0 write;
no subsequent rise/retrigger occurs in the measured model windows.

Actual read windows begin330/373/416µs after R0 acceptance. Their ends are
logged separately from the later result stores. The three short-line cases
first wrap after211.875/208.875/207.875µs, leaving at least118.125µs of
settled short lines before the first read. The R0=1/R2=1 case has separated
HSYNC edges; the unreachable cases each have one transient HSYNC during
initial C0 overflow, then none in the final64µs before the first read.
They must not be described as having no HSYNC from the initial write.

The final display sets R7=30 only after all measurements. This makes all
four rows visible; it does not change the measured R7=10 context. A
relocation-aware source comparison and the replay's complete measurement
records confirm unchanged values, timestamps and counter tuples across
that display-only repair. Final execution completes in12,108,939 master
clocks. The earlier display-limited prototype and mislabeled read-end CSV
are preserved; the final observer records actual read ends and first wraps.

## Reference and remaining alternatives

Fresh AmSpirit Lite1.17.1/core2491682 normal-boots the generated disk as
CPC6128/CRTC1/128K/French, monitor Off, then runs `RUN"VSCLK.BIN`.
The host requires natural idle PC4260, completion byteA5 and the first64
loaded code bytes before reading the twelve result bytes at42E1. Root
inspects the complete rendered table. The instance finishes paused with
scripts and breakpoints empty. Its hidden CRTC counters and HSYNC are
not exposed by this API, so the model's detailed pulse chronology is not
claimed as a direct AmSpirit measurement.

Focused Astra analysis agrees that zero-only count inhibition remains a
hypothesis. A minimum counting phase requiring C0=1 gives the same settled
outcomes. Pulse-age, recovery and onset-at-zero controls can distinguish
those mechanisms; the French source does not currently select one.
French ACCCv1.11 §14.2 p133 specifies fixed16-line Type1 width;
§16.4.2 p171 describes ordinary onset/end without naming this short-line
clock. §13.3 p115 keeps C4/C9 running at R0=0. No RTL change follows.

## Artifacts and native MiSTer result

Private source, builder, listing, disk and observations are in
`local/overnight-20261103/resumed-20261104/c1-vsync-clock-probe/`.
`replay/PARENT-QUALIFICATION.json` binds the final input, observer and
compiled model. `amspirit-002/` contains the guarded result/screenshot.
The old prototype is under `original-display-limited/`.

The display-corrected disk SHA256 is
`51d2a99f00863bf5a639589a38885620085b1930cf0485f454ad79e751a2f54a`;
the raw program SHA256 is
`453c3107ec4678dbe1ce651f748a1398189dd7558249d74e46972ac6b7a60ab8`.
The disk is installed on MiSTer as `VSCLK_51d2a99f.dsk`. After SSH access
recovers, source `c0939d8` completes the normal-boot diagnostic: ordinary
63/46 reads111; cases1/1,1/63 and0/63 each read000, matching simulation.
Two captures separated by two seconds are byte-identical, PNG SHA256
`5a7297303795d4a690771c3f63d3ea9218f41492f2539fddf3273ff4c023827f`.
The parent reads all four rows; the script completes13 commands/two captures
with no capture failure. `native-c093-004/PARENT-ACCEPTANCE.json` binds the
RBF/media/CFG, launch and screenshot evidence.

Attempts001/002 stop on SSH authentication. Attempt003 reaches the core but
rejects the unimplemented French `.` key mapping before sending input.
Attempt004 uses `RUN"VSCLK`, omitting the filename extension, and visibly
executes the intended program. Failed attempts remain preserved. The native
result confirms the FPGA/model side of the difference, not an original-CPC
rule. The original-CRTC1 measurement and author question remain open; no
zero-only or minimum-phase RTL policy is selected and no message is sent.

Technical information sourced from the Amstrad CPC CRTC Compendium by
Longshot (CC BY-NC-ND).
