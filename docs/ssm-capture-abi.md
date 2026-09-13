# SSM capture ABI (phase 2 prototype), version 1

Written 2026-09-12 for backlog B4. This is the contract between
[`rtl/ssm_sample_recorder.v`](../rtl/ssm_sample_recorder.v) and the host reader
[`scripts/hardware-loop/ssm_capture.py`](../scripts/hardware-loop/ssm_capture.py).
It is **separately versioned from the format-1 event ring** in
[`rtl/ssm_marker.v`](../rtl/ssm_marker.v), so an experimental capture build
cannot silently change what `ssm_ring.py` reads.

## Architecture and Scope

### Phase 1 vs Phase 2: Purpose and Tradeoffs

- **Phase 1 (Format-1 Event Ring, Production Default):**
  - **Mechanism:** Detects the 32-bit SSM sequence (`0xED 0xFF 0xHH 0xLL`) on the CPU bus in [`rtl/ssm_marker.v`](../rtl/ssm_marker.v) and appends a 16-byte event entry to a 64-entry circular ring buffer in DDR3 (`0x30000000`, 1,040 bytes total).
  - **Capture:** The host runner (`scripts/hardware-loop/csl_runner.py`) monitors the ring over SSH via `/dev/mem` mmap and, upon detecting a new marker, issues an asynchronous framebuffer screenshot command to `/dev/MiSTer_cmd`.
  - **Suitability:** Author Longshot confirmed (2026-09-12) that **all SHAKER test result screens are visually stable for multiple VSYNCs around the SSM marker**. Because results remain static until user keypress or timeout, Phase 1 captures provide 100% fidelity for the entire existing SHAKER suite with zero DDR3 bandwidth overhead, zero bus contention, and negligible memory footprint.

- **Phase 2 (Sample Recorder Prototype, Experimental):**
  - **Mechanism:** Continuously samples the raw 16 MHz pixel pipeline (RGB24, HSync, VSync, HBlank, VBlank, Field) in [`rtl/ssm_sample_recorder.v`](../rtl/ssm_sample_recorder.v) into rotating 2 MiB windows in DDR3 (`0x31000000`–`0x32100000`, 17 MiB total).
  - **Purpose:** Cycle-exact opcode slicing. It latches the exact dot-clock sample index and timestamp at the opcode fetch edge (`0xHH`), allowing reconstruction of sub-frame CRT beam state, mid-frame register modifications (e.g. split-screen rasters, dynamic palette swaps, scrolling tricks), and non-static demoscene effects without dependency on or latency from the MiSTer video scaler framebuffer.

### Why Phase 2 is Compile-Time Default OFF (`SSM_SAMPLE_RECORDER` undefined)

1. **DDR3 Memory Map Collision Risk (17 MiB Footprint):**
   - Phase 1 consumes only 1,040 bytes at `0x30000000`.
   - Phase 2 allocates 17 MiB (`0x31000000`–`0x32100000`) for 8 rotating sample windows (16 MiB payload) plus descriptor and record tables.
   - This address space has **not been formally audited or reserved** against the DE10-Nano HPS Linux kernel memory map (U-Boot `mem=` parameter, device-tree reservations, or MiSTer Main heap allocations). If the kernel allocates physical RAM pages within this range, FPGA DMA writes will silently corrupt kernel data structures or panic the system.
2. **Bus Bandwidth and Contention:**
   - Writing 16 MHz pixel beats (64-bit packed words) places continuous sustained write pressure on the shared Cyclone V DDR3 memory controller, competing directly with CPC core video/RAM arbitration and Linux HPS transactions.
3. **Unverified Timing Closure:**
   - The recorder subsystem (`ssm_recorder_subsystem.v`, `ssm_ddr_arb.v`, sample FIFOs) has not had a verified Quartus 17.0.2 timing closure run (setup/hold slack on `clk_sys` 64 MHz and DDR3 clocks) on physical Cyclone V hardware.
4. **Sufficiency of Phase 1:**
   - Because existing SHAKER tests do not exercise mid-frame dynamic raster state during marker emission, Phase 2 is not needed to validate SHAKER accuracy.

### Hardware Activation & Verification Checklist

To enable and test Phase 2 on physical hardware:

1. **Audit HPS Memory Map:**
   Inspect `/proc/iomem` and U-Boot boot arguments (`cat /proc/cmdline`) on the DE10-Nano. Ensure Linux physical memory is capped below `0x31000000` (e.g., via `mem=768M` in U-Boot or an explicit device-tree memory reservation) so kernel buffers cannot collide with the recorder payload.
2. **Enable RTL Define:**
   Uncomment or define `` `define SSM_SAMPLE_RECORDER 1 `` in [`Amstrad.sv`](../Amstrad.sv).
3. **Quartus Synthesis & Timing Verification:**
   Synthesize the core using Quartus 17.0.2 (via `build.yml` or local runner). Verify that setup/hold timing closes with zero negative slack on all clock domains (`clk_sys`, DDR3) and logic fits comfortably within Cyclone V LE limits.
4. **Hardware Deployment & Host Extraction:**
   Copy the RBF to `/media/fat/_Computer/Amstrad.rbf`. Run the live capture extractor:
   ```sh
   python3 scripts/hardware-loop/ssm_capture.py live --base 0x31000000 --out-dir /tmp/ssm_frames
   ```
   Verify:
   - Header magic matches `0x53534D43` (`"SSMC"`) and `flags[0]` indicates recorder ready.
   - Descriptors increment generations without unhandled overflows or dropped bursts.
   - Raw samples unpack into valid PPM images with correct CRT raster geometry.

---

All fields are little-endian. Every offset is a byte offset from `CAP_BASE`
(default `0x31000000`, a parameter). Every write the recorder makes is a
single 64-bit beat. The reader protocol requires atomic visibility of that commit
beat and ordered visibility of earlier writes. Simulation models those assumptions;
the HPS/device gate must establish them. A whole header or record read is not atomic.

## Region layout

| Offset | Size | Contents |
|---|---|---|
| `0x0000` | 64 B | recorder header |
| `0x0040` | `WINDOWS` × 32 B | window descriptors |
| `0x0200` | `CAPTURE_SLOTS` × 64 B | capture records |
| `PAYLOAD_OFF` (`0x00100000`) | `WINDOWS` × `WINDOW_SAMPLES` × 4 B | window payloads |

With the default parameters (`WINDOWS = 8`, `WINDOW_SAMPLES = 2^19`,
`CAPTURE_SLOTS = 16`) the payload is 16 MiB and the whole region spans
17 MiB from `CAP_BASE`.

Window `i`'s payload starts at `PAYLOAD_OFF + i * WINDOW_SAMPLES * 4`.

## Header, 8 × 64-bit words at `0x0000`

| Word | Bits | Field |
|---|---|---|
| 0 | 31:0 | `magic` = `0x53534D43` (`"SSMC"`) |
| 0 | 47:32 | `abi_version` = 1 |
| 0 | 63:48 | `flags`; bit 0 = recorder ready |
| 1 | 31:0 | `windows` |
| 1 | 63:32 | `window_samples` |
| 2 | 31:0 | `capture_slots` |
| 2 | 63:32 | `pin_prev` |
| 3 | 31:0 | `hold_ticks` |
| 3 | 63:32 | `epoch` |
| 4 | 63:0 | `payload_offset` |
| 5 | 31:0 | `descriptor_offset` (`0x40`) |
| 5 | 63:32 | `record_offset` (`0x200`) |
| 6 | 31:0 | `captures_published` |
| 6 | 63:32 | `image_loss_count` |
| 7 | 31:0 | `forced_expiry_count` |
| 7 | 63:32 | `capture_dropped` |

Word 0 is written **last** during initialisation, after words 1..7 and after
every descriptor has been set invalid. A host that sees the magic with the
ready flag set is therefore looking at a region whose descriptors all belong to
this run. Words 6 and 7 are refreshed after the data they describe.

`epoch` increments on each enable and applied-configuration restart within one FPGA configuration. It does not
survive reconfiguration and is not a globally unique session identity;
the host still needs the controlled startup handshake.

## Window descriptor, 4 × 64-bit words at `0x0040 + i * 32`

| Word | Bits | Field |
|---|---|---|
| 0 | 31:0 | `generation` |
| 0 | 63:32 | `state`: 0 = invalid/filling, 2 = sealed |
| 1 | 63:0 | `first_logical_sample` |
| 2 | 63:0 | `sample_count` |
| 3 | 31:0 | `flags`; bit 0 = samples were lost in this window |
| 3 | 63:32 | `epoch` |

Publication rules, which the host must rely on rather than re-derive:

* Reusing a window writes word 0 first, with the **new** generation and
  `state = invalid`. Only then is any payload of that window rewritten.
* Sealing writes words 1, 2, 3 and then word 0 with `state = sealed`, and only
  after every payload beat of that window has been accepted by the DDR3 port.
* A host read is valid only if it reads word 0, then the payload, then word 0
  again, and both reads show the **same generation and the sealed state**.
  Generation equality alone is not enough: a stable header over partial writes
  is exactly what the sealed state rules out.

Source-level ordering is what this establishes. Avalon acceptance does not by
itself prove HPS visibility across the interconnect; that remains a device gate.

## Capture record, 8 × 64-bit words at `0x0200 + (index mod CAPTURE_SLOTS) * 64`

| Word | Bits | Field |
|---|---|---|
| 0 | 63:0 | legacy event word A: `code` 15:0, `frame` 39:16, `line` 49:40, `hpos` 57:50, `field` 58 |
| 1 | 63:0 | legacy event word B: `seq` 15:0, `tick` 47:16 |
| 2 | 63:0 | `cut_logical_sample` |
| 3 | 31:0 | current window index |
| 3 | 63:32 | current window generation |
| 4 | 31:0 | previous window 0 index |
| 4 | 63:32 | previous window 0 generation |
| 5 | 31:0 | previous window 1 index |
| 5 | 63:32 | previous window 1 generation |
| 6 | 15:0 | `status` (16 bits) |
| 6 | 23:16 | `format` = 1 (8 bits) |
| 6 | 31:24 | `applied_config` (8 bits: `{plus_mode, mix[2:0], hq2x, pixel_rate_select, raw_crt, obs_native_cadence}`) |
| 6 | 63:32 | `epoch` (32 bits) |
| 7 | 31:0 | `capture_index` |
| 7 | 63:32 | `expiry_tick` |

`applied_config` bits (from `rtl/amstrad_video_output.sv:80`):

| Bit | Name | Meaning |
|---|---|---|
| 0 | `obs_native_cadence` | 1 when active video cadence is native dot clock (`~(pixel_rate_select \| (scale == 2'd1) \| raw_crt)`) |
| 1 | `raw_crt` | 1 when Raw CRT bypass is engaged |
| 2 | `pixel_rate_select` | 1 when adaptive pixel rate is enabled |
| 3 | `hq2x` | 1 when HQ2x filter scaler is enabled (`scale == 2'd1`) |
| 6:4 | `mix[2:0]` | Colour mix configuration |
| 7 | `plus_mode` | 1 when Amstrad Plus ASIC video mode is active |

`status` bits:

| Bit | Meaning |
|---|---|
| 0 | the capture has a current window at all |
| 1 | previous window 0 is present |
| 2 | previous window 1 is present |
| 3 | prehistory incomplete (fewer than `PIN_PREV` preceding windows existed) |
| 4 | a window in this set had lost samples when the capture was taken |
| 5 | the cut fell exactly on a window boundary, so the "current" window is the one holding the last included sample |

Words 0 and 1 are byte-identical to the format-1 event ring's record pair, so
the same decoding code serves both.

### Publication and Invalidation Rules

* **9-beat commit sequence:** When publishing a capture record, the recorder first
  writes word 7 with `64'hFFFF_FFFF_FFFF_FFFF` (invalidating any prior record identity in
  this slot),
  then writes words 0 through 6, then writes word 7 (`capture_index` and `expiry_tick`)
  as the commit beat. Only after word 7 completes does the recorder update
  `captures_published` in the header.
* **32-byte descriptor stride:** Window descriptors start at `0x0040` with 32 bytes
  per descriptor. The BusyBox reader uses 32-byte blocks (`bs=32`), so odd descriptor indices
  remain addressable without imposing a false 64-byte alignment requirement.
* **Window pool selection hierarchy:** When rotating into a new window, the recorder selects:
  1. Never-used windows (`pick_unused`).
  2. Unpinned windows outside the rolling history (`pick_nonhist`: `!cap_pin_mask && !rolling_history`).
  3. Under pressure: oldest pinned window outside rolling history (`pick_old_nonhist`).
  4. Unpinned windows inside rolling history (`pick_hist`: `!cap_pin_mask && rolling_history`).
  5. Oldest window overall (`pick_old`).
  Arriving marker pins during `R_OPEN` revalidate the selection to prevent destroying required prehistory.
* **Quiesce latch:** Disabling the recorder (`!active`) engages a quiesce pending latch
  that holds Avalon requests across `waitrequest`, completes in-flight beats cleanly,
  and flushes pipeline state through an explicit clear state (`X_INIT_CLR`) before idling.

A record is published when `captures_published` in the header reaches
`capture_index + 1`. Reading a slot whose index is at or beyond
`captures_published` reads a partially written or stale record.

`expiry_tick` is the core tick after which the pinned windows may be reused.
Compare it against the recorder's tick modulo 2^32; a capture read after that
point may find its windows' generations changed, which is a capture loss and
must be reported as one.

The live reader checks the expected commit identity before reading the record and
after retrieving all referenced payloads, with ready/epoch checks around the operation.
Each payload also requires unchanged sealed descriptor generation and epoch. Summary
status flags do not replace these identity checks. ABI v1 accepts `pin_prev` from 0 to 2.

An applied-configuration change revokes internal readiness and rejects samples and
markers on the transition edge. Previously accepted queues drain without new input;
initialization then clears published readiness/history and advances the epoch. The
first profiles require native cadence with Raw CRT, alternate rate and HQ2x off.
A native bit combined with any of those contradictory modes is unsupported.

## Sample word, 32 bits

| Bits | Field |
|---|---|
| 7:0 | R |
| 15:8 | G |
| 23:16 | B |
| 24 | HBlank |
| 25 | VBlank |
| 26 | HSync |
| 27 | VSync |
| 28 | FIELD (the core's own field signal, carried for cross-checking) |
| 31:29 | reserved, zero |

Two samples share one 64-bit write: the **earlier** sample is in the low half.

The sample at logical index `n` belonging to window `i` with
`first_logical_sample = f` lives at

```
byte offset = PAYLOAD_OFF + i * WINDOW_SAMPLES * 4 + (n - f) * 4
```

That address comes from the logical index, not from a count of samples that
were actually stored. If the input queue overflows, one packed word is not
written and the window's `flags` bit 0 is set; every later sample keeps its own
address, so the retained stream is never compacted in time. The host must treat
a window with that flag as unusable for a complete image while still retaining
its raw trace.

## What the host may conclude

* Samples are appended in time order. Nothing is addressed by sync, so an extra
  VSYNC or a repainted line is another sample rather than an overwrite.
* A capture's cut is exclusive: samples with logical index `< cut` are before
  the marker, and the sample consumed on the marker's own clock edge is not.
* `image_loss_count` (samples) and the event ring's `dropped` (markers) are
  separate. A run that lost either is not a complete run.
* Nothing here bounds how much history a given raster profile needs. Eight
  2 MiB windows hold about 65.5 ms of preceding samples at 16 MHz once filled.
  That is a capacity figure, not evidence that any SHAKER screen reconstructs.
