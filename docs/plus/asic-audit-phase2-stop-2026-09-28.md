# ASIC audit phase-2 stop — 2026-09-28

Branch: `plus/asic-audit-findings`, base `2bf1d13158acf585d4dba69a579078e8f3f14ae5`.
Reference corrections RC1-RC8: `6885db6`. Production RTL is unchanged.

## Focused evidence

Command (Homebrew LLVM is needed on this host; default Apple C++ could not find
`<cstddef>` before any test ran):

```sh
make -C sim/plus CXX=/opt/homebrew/opt/llvm/bin/clang++ run/asic_video_tests run/plus_mmu_tests run/asic_dma_tests run/asic_pri_tests b20-ack-diag
```

The video suite (70 tests, including PA5a/b), MMU (PA6a), DMA (16 tests, including
PA6c REPEAT 0 preserving live loop context), and PRI suite passed. PA6e's counter
vector observed 40→8 before acknowledge and 44 subsequent HSYNC falls before CPC
interrupt delivery. KT's separate “not closer than 32 lines” inference is not a
general consequence of clearing bit 5 (31 would remain 31); no RTL change is proposed.

The existing B20 acknowledge vectors passed. The new vector stopped with:

```text
FAIL: PA6e priority: wrong vector at source 0
make: *** [b20-ack-diag] Error 1
```

It expected raster vector `06`; its message did not print the actual byte. The
retained log is `output_files/asic-audit-phase2/focused.log` (ignored).

## Interpretation and next action

`sim/plus/plus_p8_test_top.v:321` uses
`hdr_ga_int_pending = hdr_int_pending & ~aregs_dma_int_req`, matching the
motherboard snapshot restoration policy (`rtl/Amstrad_motherboard.v:553`).
The test restored header raster-pending together with DCSR `70`; the DMA request
therefore suppressed restoration of raster pending. This did **not** create
simultaneous sources. The arbitration chain implies DMA2 vector `00` for that
setup, but the actual byte was not logged. This failure is not evidence against
the specified raster > DMA2 > DMA1 > DMA0 priority.

Keep expected vectors `06,00,02,04`. Create simultaneous requests through live
events after restoration, and assert the raster and DMA preconditions before
acknowledge. Do not change snapshot policy or adjust expected priority to the
observed setup. The user requested a stop on any failed new test; no repair or
rerun was performed after this failure.

PA6b/d's synthetic-cartridge D5 checks are written but unrun. All test changes
remain uncommitted and provisional, pending corrected setup, validation and a
fresh cross-provider code review. The final `python3 sim/select_tests.py --run`
and `make -C sim soak SOAK_EXPECT=0xe99ab434a5e1cdb3` have **not** run.

## Probe handoff status

Phase 3 remains unimplemented. Read-only planning identified designs for PA1,
PA2, PA3, PA4 and PA7; Muse Spark 1.3 xhigh reviewed those designs, but this is
not a code review or hardware-ready cartridge. No new screens await hardware yet.
PA2 must observe adjustment lines directly and use PA3's terminal-split band as
a visibility control; a frame-carry observation alone would conflate PA2 and PA3.
