# T80 first-fetch IORQ reset — 4 November 2026

## Accepted CPU repair, hardware build not yet made

Resetting T80pa's delayed interrupt-cycle state to `11` removes a false IORQ
during the first ordinary opcode fetch after a fresh reset. A retained B18 test
fails before the change and passes after it through the actual SNA loader,
motherboard, PPI and production T80. Existing real interrupt acknowledgements
and VHDL/translated-Verilog equivalence pass their focused checks.

This is a one-line CPU reset repair, independent of CRTC/ASIC timing. The parent has accepted the cross-provider review and selected gate; focused
checks after removal of the old startup masks are recorded below.
No general gate, RBF build, device/API operation, commit, push or delegation was
performed by this unit. Other workers' edits were preserved.

## Scope: cold restore, not every SNA load

`rtl/T80/T80pa.vhd` declares the two-bit `IntCycleD_n` without an initializer.
The reset branch set the output strobes inactive but left this internal state
undefined. On M1/T1, it assigns `IORQ_n` from the old delayed bit before shifting
in the current interrupt-cycle state. Only M1/T3 subsequently sets the pair to
`11`.

Consequently the cold translated model starts with zero history and emits a
false IORQ during its first memory fetch; native VHDL exposed an unknown in the
same startup interval. A prior normal M1/T3 already flushes the state to `11`.
The existing six B18 save/restore cases execute a substantial program before
restoring, so they were warm and did not reproduce this cold failure. Do not
claim that all ordinary hardware SNA loads failed: a previously running CPU may
already have the correct history.

The repair adds `IntCycleD_n <= "11"` to the existing RESET branch. This agrees
with the wrapper's own normal M1/T3 idle assignment; no interrupt is fabricated
or suppressed by a new bus mask, opcode exception or snapshot-only seam.

## Independent contract and concrete failure

Zilog's **Z80 CPU User Manual UM008011-0816**, “Instruction Fetch” printed p.8
and “Interrupt Request/Acknowledge Cycle” printed p.12, distinguishes a normal
MREQ opcode fetch from the special acknowledged-interrupt M1, which uses IORQ
instead. The parent verified the manufacturer PDF at
<https://www.zilog.com/docs/z80/UM0080.pdf> (PDF pages 21 and 25, respectively).
This worker's attempt to reopen that URL redirected to an inaccessible
Littelfuse page; the primary-page verification is attributed to the parent.

The original SHAKER A menu snapshot begins at logical 4195 with bytes
`CA C4 41` (JP Z,41C4), saved F=30hex and Z clear. It should fall through 4198.
Before the repair the first M1 also asserted IORQ. At that address PPI-B is a
real bus responder; READY shaping allowed its F4 input to AND with RAM's CA,
producing C0 (RET NZ). The CPU loaded C0 into IR and popped screen bytes 3C,FC
from the saved SP=C000, reaching FC3C. The later ROM request was a consequence
of wrong execution, not a firmware dependency of the original A4 caller.

The original passive evidence remains under
`local/overnight-20261103/asic/a4-authentic-proof/run-before-reset-fix/`, including
`first-bus.csv`; the initial untapped run is `run-direct-first-attempt/`.
The observations are direct CPU bus/IR samples, not a comparison only at the
earlier INSN_START state.

## Retained fail-first regression

`sim/b18_snapshot_test.cpp::first_fetch_after_cold_restore` builds a minimal
standard SNA containing the same independent instruction contract: PC 4195,
JP Z with Z clear, ROMs disabled, PPI-B input and both IFFs clear. Its unchanged
program falls through to LD A,5A/HALT. It loads the SNA before any ordinary
fetch has warmed the delayed interrupt state.

The test requires no IORQ/memory-read overlap, first fetch 4195, next instruction
4198, and the program's final accumulator 5A. It uses the existing B18 top and
real production owners; no new top-level injection hook, PC trampoline, startup
NOP or RAM patch was added. This pins a cross-module reset/restore/PPI failure
with a credible path back through shared CPU, WAIT and SNA changes. It is the
single retained owner for this fresh-restore interaction, rather than a test
that simply reads the newly initialized register.

The existing `sim/TESTS.md` B18 row already covers the source paths and
production-T80 CI integration; only its description was expanded. No new bench
or workflow was introduced.

Before the production edit:

```sh
export CPATH="$(xcrun --show-sdk-path)/usr/include/c++/v1"
make -C sim b18-snapshot-test
```

Result, preserved in `first-fetch-before.log`:

```text
FAIL B18: cold SNA fetch overlaps IORQ and memory read at 4195
make: *** [b18-snapshot-test] Error 1
```

Only after that failing result was recorded was the production reset line added.
Afterwards the focused command was:

```sh
export CPATH="$(xcrun --show-sdk-path)/usr/include/c++/v1"
make -C sim b18-snapshot-test t80-irq-sample-test t80-trace-test
```

Result, preserved in `first-fetch-after.log`:

```text
PASS B18 cold first fetch: JP Z falls through 4198; no IORQ/memory overlap
B18 snapshot: PASS cold first fetch and 6 classic capture/restore cases
B18 host publication: PASS 6 torn-read/retry cases
Summary: 14 passed, 0 failed
PASS T80 native VHDL vs translated Verilog trace equivalence (139 cycles verified, bounded defined-field trace)
```

The 14 IRQ cases include held/early/late INT, WAIT-stretched execution, EI,
prefix completion, HALT wake, IM2 and NMI priority. Trace equivalence also
verifies real OUT(C),C and OUTI strobes. Thus resetting idle history does not
remove genuine acknowledge or ordinary I/O cycles.

The parent separately owns cleanup/strengthening of obsolete startup exceptions
in `t80_trace_test.cpp`, `t80_freeze_test.cpp` and comments in
`t80_irq_sample_test.cpp`. These focused results precede that cleanup; the parent
owns the final selected gate after review and final edits.

## Original A4 menu now resumes; partial authentic baseline obtained

The private A4 adapter was rebuilt with the regenerated production T80 netlist,
while retaining its original raw admission/equality ASIC source and unchanged
genuine menu SNA. No earlier counter or character-write hypothesis was combined
with this repair. Netlist SHA256 changed from
`bded028373578f038e518016585efdc21a669743789e52e47a0c2bf3d36e361a`
to `435eb2ac16780e0f60c62f76e098239658f2c5f643c7746541ee7c2b5dfeb565`.
The original netlist remains in `T80pa-before-reset.v`.

Header C0/C4/C9=38/12/3 and PC 4195 restore exactly; the original conditional now
falls through 4198. Real HID key 4 press 625hex at 1280000 master clocks enters
original A4 entry 71AA at 1300295, with its row 7/bit 0 key observed pressed.
Release 025hex is issued at 6400000 clocks. These are keyboard inputs through
the real scanner, not CPU/PC/RAM changes.

The clock-limited run completes the original seven top HCC measurements, whose
PPI differences are written by 71FD into 74EE..74F4. The original display lookup
maps 0 to OK and 1 to KO; source code labels the rows 39..3F. Results are:

| Original HCC label | Raw result | Display meaning | Raw C0 at R0←7F acceptance |
|---|---:|---|---:|
| 39 | 0 | OK | 59 |
| 3A | 0 | OK | 60 |
| 3B | 0 | OK | 61 |
| 3C | 0 | OK | 62 |
| 3D | 0 | OK | 63 |
| 3E | 1 | KO | 0 |
| 3F | 1 | KO | 1 |

The parent's original-Plus0037 reference photograph and fresh CPCSE comparison
require 3C=OK and 3D=KO. This genuine-caller baseline therefore reproduces the
device 3113323 mismatch at 3D without a synthetic M1/C0vs calibration. The same
test's R0 enlargement is the type3 deadline described by French ACCC
§13.6.3 p.125. It is independent of the unresolved lower-than-C0 rule and does
not validate either earlier private repair hypothesis.

The 128000000-master-clock (2s) cap was reached after seven top results and one
auxiliary R0 case. Final execution is normal code at A516, with no ROM/FDC
dependency. OUTI 3C and the final 0037 SSM marker were **not reached**. Runtime
exit 2 reflects that incomplete capture criterion, not a crash. Do not describe
this as a completed page, a screenshot comparison or a measured OUTI baseline.

`a4-baseline-score.{json,log}`, `run/{io,results,instructions,vsync}.csv`,
`source-provenance.json` and the post-fix hashes preserve the partial baseline.
The top 3D mismatch is now available for the parent's next source-derived
admission/consumer investigation; no ASIC fix is proposed in this CPU unit.

All private processes have exited. No device or emulator API was accessed.


## Parent review and final selection

Gemini3.8-high independently accepts the production reset and retained regression,
including the parent's removal of obsolete native-X and first-refresh exemptions.
No code changes were requested. Review run20261104T025342Z-87132-0765 completed0;
its full output is preserved at
`local/overnight-20261103/t80-first-fetch-review/gemini-review.log`.
The reviewer used the supplied focused logs and did not rerun tests. Its prose
misstates the PPI configuration port once asF400; the actual test correctly
restores PPI control82 through the snapshot header, and the real PPI-B response
is the relevant cross-module interaction.

After the last code/test edit, the single selected gate was:

```text
python3 sim/select_tests.py --run
select_tests: PASS 44 benches: test, crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, b18-save-admission-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_mode_latch_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Full output is `t80-first-fetch-review/selected-tests-final.log`. The existing
T80 native/freeze/IRQ benches are classified slow and are therefore run explicitly
for the strengthened assertions; no general slow sweep is implied. B18's focused
cold regression and six save/restore cases already passed after the production
edit, before these unrelated assertion cleanups. A synthesized RBF/device check
has not yet been made for this CPU change.


After the parent strengthened those existing assertions, the explicit final
command `make -C sim t80-trace-test t80-freeze-test t80-irq-sample-test` also
passes:139-cycle native/translated equivalence with defined startup IORQ, the
freeze/save handshake cases, and all14IRQ controls. Exact output is
`t80-first-fetch-review/strengthened-tests-final.log`. No code edits followed.


Exact-SHA CI on9de75d31ab92e182ccb86697434980b1f9221c2f also passes
[run41764626693](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41764626693):
simulation/lint, production-T80, synthesis-policy and required-gate all succeed.
Synthesis is correctly skipped for this branch push; no RBF was produced.
