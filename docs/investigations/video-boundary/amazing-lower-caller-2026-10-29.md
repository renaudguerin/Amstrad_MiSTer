# Amazing lower-region caller replay

Follow-up to [the two-boot device evidence](amazing-lower-evidence-2026-10-29.md),
2026-10-29. This diagnostic now reproduces the dense lower corruption before
MiSTer's mixer/scaler. The first located model mechanism is the lower character-row
pointer; a hardware-rule violation and a production repair remain unproven.

## Execution and input contract

A, the sole device/AmSpirit operator, captured fresh uninterrupted Amazing startup
at physical bank7:C000 and bank7:C012. C000 is the entry before initialization;
C012 follows C6C5 initialization and precedes C7B6/CF04 synchronization. These
are the exact rev2 disk and 6128/CRTC1 configuration in the earlier report.
C012 SNA SHA-256:
`ff2525dc5d002b337cdb647d7ae0331afd4143907b87a5412d2df9be4a05cd54`.
Its 128K payload matches the eight separately captured RAM banks byte for byte.

Two input limitations must remain explicit:

- Byte59 is `54`, a BSR-shaped value. Production `i8255.v` loads a snapshot's
  control byte literally into MODE; this sets B as output, retaining saved
  latch `1F`. Both memory backends consequently poll CF12 forever even though
  raw CRTC VSYNC toggles. A separately named diagnostic changes only byte59
  from `54` to `82`. The published AmSpirit API does not expose live PPI MODE
  (A inspected its registry/function list without advancing the machine).
  Thus `82` remains an assumption, not an exporter diagnosis or a proposed
  production snapshot fix. Ordinary BSR writes preserve MODE in `i8255.v`.
- The CPU is held in independent reset during load. A temporary reset
  trampoline executes NOP, LD SP,saved-SP, JP entry-PC; original bytes are
  restored upon entry. NOP absorbs the known initial T80 IntCycleD_n overlap
  where simultaneous IORQ/MREQ admits the CRTC's zero read response. This is
  a fixture-entry precaution, not a newly established production CPU defect.
  Other CPU registers start reset. Authentic C000 POP AF retrieves type1
  from the original stack. Reset A' is FF rather than the captured 00; C005
  EX AF,AF' stores this in drive fields C1E2/C1E7.

The full C000 replay executes C6C5, reaches C012 at tick109630831 with C186=1,
then runs the actual C7B6/CF04 synchronization. At C012 its generated C2-mapped
ranges `3F40..3F69`, `4600..4BFF`, `9500..99FF` match the independent AmSpirit
capture exactly. All 128K differs only at the two drive fields above. No FDC
read occurs in the retained windows; unsupported FBxx reads now abort explicitly.
This validates a useful caller reconstruction, not complete boot/hidden-state
or original-CPC equivalence.

## Paired memory comparison

The ideal-memory adapter and the production `rtl/sdram.v` adapter share the
production T80pa/GA/CRTC/MMU path. The latter uses the existing P10 SDRAM pin
model, production clock-divider/clkref phase, address reconstruction and byte
masks. Runtime CPU and video transactions traverse the controller; snapshot
upload directly fills the chip model while the CPU is held, bypassing the
production upload queue. No physical SDRAM timing conclusion follows.

With the same diagnostic C012 input, the first three caller frames and a
three-frame window starting at caller128 produce byte-identical caller traces,
I/O/physical RAM-write traces, fetch traces, native GA RGB samples and final
128K RAM across both backends. The first window is blank/fade; caller128 has
coherent partially typed lower text. These are bounded comparisons, not a
claim that SDRAM cannot contribute to any title defect.

Caller C015 repeats at 1277952 master ticks, or19.968ms. RGB uses one six-byte
sample per four 64MHz master ticks; each retained frame has319488 samples.
Observed Full mode and SHIFT remain0 throughout these windows. RGB is a native
GA level-pair trace drawn into a fixed1024x312 raster, not a monitor simulation.

## Located wrong address

The production-controller replay starting at C012 was advanced without changing
execution to caller512. Its three-frame window reproduces the dense lower
pattern seen on the MiSTer. The 515 observed caller entries retain the same
cadence. All119808 recorded fetch words in this window match RAM at their
requested physical addresses; there are no base64K CPU writes in that window.
The corruption therefore has an address/state discriminator inside this model,
rather than a mismatched returned word at those fetches.

The routine map matters: C234's generated chain is the upper effect. Normal
lower display follows its return at C05D, which programs R4=17 and R9=7.
R12/R13 already select3000. Immediately after C05D, row0 starts at MA3000 in
both specimens, preserving WELCOME. The following character rows differ:

| Lower row C4, C9=0 | Readable caller128 | Corrupt caller512 |
| --- | --- | --- |
| 0 | 3000 | 3000 |
| 1 | 3030 | 0C34 |
| 2 | 3060 | 0C34 |
| 3 | 3090 | 0C34 |
| 4..7 | continue +R1=48 | repeat0C34 |

For the first retained caller512 frame, C05D is fetched at tick663650159;
row0 MA3000 at663650175; row1 MA0C34 at663682943. Those are replay ticks, not
MiSTer event timestamps. The separate `phys` column at a C0-wrap sample can
still carry the preceding registered VRAM address: MA is the row-pointer
observation here; use accepted-fetch rows for physical RAM transactions.

## Retained-state discriminator

The full C000 observational replay first arms the RFD parity flag in caller176
at tick345206655, latest observed M1 PC3F4A, C0/C4/C9=0/0/3, hidden tupleF.
At345209790 a CLKEN-qualified C0=R1=48 save-base is true but save is false;
C4/C9=0/3, saved pointer2FC9, current pointer0C35. Current bus is idle at the
arm sample, so the last M1 address is not proof of the causative I/O edge.

Importantly, the next retained callers177..179 still have coherent lower rows
3030,3060,3090,...3150. Arming the flag, or observing an upper-region suppressed
save, is insufficient to explain the later lower corruption. The subsequent full-C000 observation reaches the first dense lower row in
caller217. At C05D tick397836143 the hidden tuple is2 (PF=0,PC9=0,RFD parity
armed,VMA source disarmed). The new lower origin changes it toE. At397867966,
C0/C4/C9=48/0/7, current MA3030, save-base=1 but save=0. At397868927,
C0/C4/C9=0/1/0, both saved and current pointers are0C34. The next retained
three-frame window, caller218 onward, repeats0C34 in lower rows1..7. This
confirms the immediate model mechanism; it does not decide whether arming,
retention or parity evolution violates the physical chip's rule. Last-IORQ
context in this trace can precede bus settling and must not be interpreted as
a decoded write. The separate I/O trace supplies causative transactions.

French ACCC v1.11 §11.6 pp89–91 explains RFD's two retained states and parity-
dependent VMA' saves; §13.6.2 p124 distinguishes OUT(C),reg8/OUTI update phase;
§13.7.1.2 p126 defines the widened-last-line/R4-or-R9-cancellation route;
§19.5.3 pp209–212 defines parity changes. The French text was extracted with
pdf-inspector and the p124 chronogram rendered and inspected. These rules
support tracing the mechanism; no unsupported flag-clear patch is justified.
The single authorized Astra-high read-only diagnosis independently identifies
the same suppressed-save mechanism and finds no proven French-rule mismatch.
It ranks parity-enable lifetime/normalization, exact R0-zero acceptance/parity
evolution, and fixture entry history as unresolved causes. It also identifies
a logging trap: `c0 < oldc0` does not record consecutive R0=0 line origins;
those rows cannot reconstruct parity history. A minimal one-shot trigger,
followed by ordinary frames without retriggering, is the proposed original-
CRTC1 discriminator before choosing a permanent failing expectation.

A narrowly read independent implementation, CPCSyntaxError commit
`ba477b84496e7399ff7c001bbf86863e49cf3eca`, supplies a concrete competing policy.
`src/core/crtc_type_1.h` lines476–501 uses one `rfdActive` flag for both
any-row R12/R13 reload and parity-sensitive VMA' save, and retires it on a
successful save. This core has separate VMA-source and sticky parity-enable
flags. CPCSyntaxError uses frame parity rather than ParityC9 at that save and
arms its R0 cancellation route at the register rewrite rather than the actual
extended-line end. These differences make it a diagnostic lead, not an oracle
or a ready-made patch. Its PPI preserves mode on BSR writes and saves control
with bit7 set (`ppi.cpp`, `sna.cpp:271`); this does not establish AmSpirit's
actual PPI MODE. No files in the comparison repository were changed.

A's later uninterrupted AmSpirit C05D capture remains readable. Its software
counter C12D is232F versus startup25A8. C130 decrements it once per caller,
so the pre-decrement C05D state corresponds to caller634, provided the timer
has not reset; raw monitor-frame waiting alone did not establish that ordinal.
Keep this source-derived correspondence separate from elapsed-time alignment.

## Reproduction and scope

Ignored working artifacts: `.roster-scratch/v3-resume/` contains fresh/late
inputs, paired-results.json and input assumptions; `v3-sdram/` contains the
controller adapter and caller512 trace; `v3-full-entry/` contains full
initialization, table comparison and passive RFD/save observations. Their
READMEs record exact commands, formats and limits. All copyrighted media,
snapshots, RAM/disassembly and images remain ignored. Production RTL and
tracked simulation sources are unchanged; exploratory builds/runs are not
behavior gates. The next requirement is a source-grounded first divergence
and fail-first vector before any timing/state repair.
