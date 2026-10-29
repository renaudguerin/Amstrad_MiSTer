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

A's later uninterrupted AmSpirit C05D capture remains readable. Its software
counter C12D is232F versus startup25A8. C130 decrements it once per caller,
so the pre-decrement C05D state corresponds to caller634, provided the timer
has not reset; raw monitor-frame waiting alone did not establish that ordinal.
Keep this source-derived correspondence separate from elapsed-time alignment.

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

A corrected passive observer records actual CRTC register-file acceptance and
CLKEN-qualified line/frame events, including repeated R0=0 origins. Its C012
128K RAM and all 11299 named CPU fetch events through the phenotype match the
earlier replay; first dense caller217 remains tick397868927/MA0C34. From caller170
through the short caller218 endpoint it records 29639 line origins, including
14655 with R0=0, with no hidden-state continuity gaps. PF and PC9 are equal
at every recorded pre- and post-edge state. Selecting one instead of the
other cannot change the predicate anywhere in this recorded interval.

The observer also locates the first arming transaction: R0 changes 0→63 at
tick345202567, C0/C4/C9=0/0/2 with R9=2; R9 changes 2→3 at345203399;
the actual extended-line end qualifies the R0 route at345206654 and the
post-edge hidden tuple is F. Accepted writes are system-clock edges, including
repeated acceptance while the CPU holds the bus, not invented first-IORQ
transactions. The one-tick distinction from the earlier state observations
is the documented pre-edge versus subsequent sampled-state convention.

Immediately before the lower return, caller216's R9=0 tail has 15 frame origins
after the final R9=3 origin; caller217 has 14. Each toggles both parity states.
The last R9=3 origin is 65521 versus 61425 ticks before C05D, a 4096-tick
difference. This accounts for C05D hidden E versus 2 and the opposite parity
after the lower origin. Assigning a uniquely missing member among identical
periodic origins would depend on alignment; the count difference is exact.
This closes a model-history explanation, not the original-hardware oracle.

Of the 42 source-flag clear transitions in that interval, 21 have the qualified
successful-save context and 21 have R1>R0 with bare C9=R9. The last clear before
dense217 is tick397606527: R0=0, R1=48, C9=R9=3, hidden 3→2. CLKEN is 0 on
that system-clocked source-disarm edge. Thus the source-scope control below
exercises a disarm route present in the actual replay history, rather than
only a hypothetical register arrangement.

French ACCC v1.11 §11.6 pp89–91 explains RFD's two retained states and parity-
dependent VMA' saves; §13.6.2 p124 distinguishes OUT(C),reg8/OUTI update phase;
§13.7.1.2 p126 defines the widened-last-line/R4-or-R9-cancellation route;
§19.5.3 pp209–212 defines parity changes. The French text was extracted with
pdf-inspector and the p124 chronogram rendered and inspected. These rules
support tracing the mechanism; they do not yet settle the save gate's scope
after source retirement. The qualified observer above replaces the earlier
`c0 < oldc0` line log, which cannot reconstruct consecutive R0=0 origins.
Neither a blanket flag clear nor a polarity change is justified by a matching
picture alone.

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

## RFD rule decision remains open

A second, distinct French-rule review separates four competing decisions. At
the pinned comparison revision, their implementations are:

| Decision | Current RTL | CPCSyntaxError |
| --- | --- | --- |
| RFD save parity selector | ParityC9 | ParityFrame |
| Value suppressing the save | odd / 1 | even / 0 |
| Successful save retires | VMA-source state | combined source/parity gate |
| Even-R9 ParityC9 row toggle outside IVM | enabled | disabled |

French §11.6 p89 explicitly distinguishes two states. Section 11.6.2 p91
specifies the successful comparison's clear for the VMA-source state and
constrains parity-enable changes within the frame; it does not explicitly
define automatic parity-enable clearing at the following origin. The existing
[author response](../../classic/accc-author-response-round2-2026-08-31.md)
confirms source-state clearing only. The unanswered
[author question23](../../classic/accc-author-questions.md) already identifies
the same lifetime ambiguity from SHAKER D history.

There is also a polarity tension within the French text. Section 11.6.1 p90
explicitly attributes the failed save to odd parity. Sections 11.6.2–11.6.3
pp91–92 associate IVM ON/OFF before RFD, which normalizes parity to even,
with that failing case; the after-RFD recipe associates the following odd
frame with the other case. This is an apparent tension whose operating
conditions have not yet been matched, not an established contradiction.
A permanent expected value must not silently choose between these readings.
The RTL comment at
`rtl/crtc_type1_engine.v:505` also overstates p90 by naming ParityC9 where
the source names frame parity. V3's parity-selector decision must resolve
that claim before treating its interpretation as verified.

CPCSyntaxError explicitly cites the Compendium's frame-parity wording for
its PF selector. No measured hardware exception or author clarification for
its save-gating scope or the opposite polarity was found in the scoped
source/history inspection. A small CRTC-only executable
comparison confirms its implemented policy: with odd R9 and IVM off, an
even-PF arm suppresses saves until an odd-PF save retires `rfdActive`; an
odd-PF arm retires at the first eligible save. An even-R9 IVM arm also
suppresses with PF=0/PC9=1 and retires with PF=1/PC9=1. These are synthetic
emulator observations, not a title replay or original-CPC truth table.

A separate production-CRTC pin observer uses the same small register geometry
and one ordinary-bus R5 pulse, choosing each PF phase by waiting rather than
writing hidden state. It observes the opposite policy: PF=PC9=0 saves, while
the initial PF=PC9=1 matches suppress. It retains three origins after the first
successful save and confirms parity-enable remains set after the source flag
clears. Both standalone probes compiled and completed their protocol checks;
they are not cycle-equivalent replays or production behavior gates.

State retention and combinational scope must also be separated. The French
two-state wording does not explicitly require parity to gate every later
VMA' save after R12/R13 sourcing ends. CPCSyntaxError can retain parity
eligibility and its within-frame RFD#10 lock while consulting parity only
when its source state is active. Its combined effective gate is therefore
not proof that it incorrectly clears the second state. Conversely, the
current RTL's global gate after source retirement is an interpretation,
not a proven consequence of keeping two state bits.

A stronger scope discriminator uses odd R9 and a known inhibiting parity,
arms RFD, then temporarily sets R1>R0 across C9=R9. French §11.6 p89 explicitly
disarms R12/R13 sourcing on that bare match without a successful save.
Restore R1<R0 before a later eligible save in the same frame, with no R8
write, origin or new RFD. A source-active control should still suppress;
the source-disarmed arm distinguishes a global retained gate (suppresses)
from a source-dependent gate (saves). An R12/R13 sentinel at C4>0 checks
source retirement independently of row advancement. Resumed saves could
also represent hidden normalization at that disarm; the observation would
not validate every CPCSyntaxError policy.

The private RTL scope experiment executes that control: source/parity=1/1
initially inhibits; R1>R0 clears source without a save; after restoration,
source/parity=0/1 still yields save-base/save=1/0 in the same frame. The
next-row sentinel distinguishes MA3008 from the active-source control's
MA3040. CPCSyntaxError cannot execute the equivalent cleared-source control:
it retains `rfdActive` across the R1>R0 bare match and restoration. Its
ordinary API produces MA3040 in both arms. That is an unsupported control
in the comparison model, not an original-CPC verdict. Both bounded programs
compiled and completed their declared protocol checks.

The recommended original-CRTC probe uses two parity-preparation pages and
interprets the page whose source-active controls actually inhibit. Its scope
measurement then stays within one frame with no R8 change or retrigger. This
avoids assigning an absolute parity truth table before observing hardware.
Broader polarity work should use odd R9 first, then separate an IVM PF≠PC9
case from a non-IVM even-R9 evolution case.
For lifetime, observe a successful post-RFD save, enter the known inhibiting
parity without retriggering, and compare with a fresh-arm control. This
distinguishes retained eligibility from retirement/clearing; distinguishing
successful-save retirement from origin clearing needs a further controlled
within-frame parity transition. At the first Amazing dense lower save,
PF=PC9=1, so selector choice alone cannot distinguish that immediate event.
No production repair or permanent failing expectation follows from the
comparison alone.

## CPU-executed scope protocol

An ignored prototype now executes the scope experiment through production
T80/GA/CRTC, using the original BCOPY drift-sync and delay routines. The
eight completed cases are two parity-preparation pages × startup delays of
0, 7, 41 and 113 ordinary NOPs. DI executes on the CPU and the real GA IRQ
remains connected. PPI F5 input is projected as the VSYNC bit; this is not
a full PPI, ROM, floppy or original-CPC test.

Actual CPU writes place the terminal, main and rearm R5 pulses at
C4:C9:C0 = 0:7:63, 1:2:63 and 5:0:63, restoring R5 at C0=3 on each next
line. P0 normalizes at row0/C9=2; P1 normalizes in preceding blanking and
crosses one origin. R1=80 spans the row2 last line; R1=32 is restored on
row3. No R8 change, origin, retrigger or successful save intervenes between
the main P1 pulse and its source-disarmed save observation.

The source-false A01, source-active B00 and following C00 address witnesses
all hold. After source clear, the B sentinel is ignored. In the current
model, P1 then repeats A01 across rows3→4 and repeats B00 after rearming;
P0 advances C01→C02 and B00→B01. These labels map the observed MA values
to synthetic marker pages. The fixture also checks the mapped RAM byte;
it does not model production video DMA/rendering. The exact build/run and
`python3 .roster-scratch/v3-scope-portable/check-scope.py` reports are in
that directory's `CPU-SCOPE.md` and logs. This is a demonstrated CPU-driven
discriminator of current behavior, not a permanent assertion of hardware
truth or a completed portable disk.

Initialization order matters: programming R0 first while reset R4/R9 are
zero accidentally opens the widened-last-line route, then later geometry
writes arm RFD. Programming safe R4/R9 before R0 avoids that precondition;
the accepted traces reject any source/parity state before synchronization.
The failed startup traces remain private rather than being relabeled as
valid marker results.

## Authentic COPY comparison limits

The authentic SHAKE27B COPY group provides higher-authority visual targets:
[A](https://shaker.logonsystem.eu/images/cpc/CPC/BCOPY_CRTC1_A.webp) has mostly
bitmap-like lower content, while
[B](https://shaker.logonsystem.eu/images/cpc/CPC/BCOPY_CRTC1_B.webp) and
[C1](https://shaker.logonsystem.eu/images/cpc/CPC/BCOPY_CRTC1_C1.webp) repeat
test-label rows. Its A–D loop temporarily writes R9=6 around the R5 pulse
and restores R9=7; A places R8 ON/OFF after that sequence, B before, C neither
and D both. Both optional calls execute with R9=7 and have the same nominal
21µs duration. Thus direct even-R9 normalization during R8 is excluded, but
an intervening even-R9 row boundary can still change counter/parity history.
The exact C9 phase and pictured iteration remain necessary before deriving
parity constants from the photographs. No A–D loop implements the R1>R0
scope control. The supplied disk/body provenance and private static mapping
are retained under `v3-bcopy-source/`; no copyrighted bytes are tracked.

A later genuine AmSpirit A checkpoint at PC6754 supplies complete RAM and
PPI82 without the Amazing PPI substitution. A named diagnostic starts at
6751 so the original CALL9F52 resynchronizes before the loop; counters and
hidden state still start reset. A second diagnostic changes only the two
call targets to the authentic B-body configuration, retaining A's other
RAM/history. It is not an authentic B checkpoint.

Across three model loops, both pulse R5 at C4=5/C9=0/C0=63. The R9=6 window
contains only C9 0→1 with no C4 increment. B's early R8 pair executes at
C4=4/C9=0; A's late pair at10/0. B normalizes before arm and permits the
row5 save; A suppresses rows5–9 until its later normalization, then saves
on row10. Thus an even-R9 C4 transition does not explain this model ordering
effect. These are resynchronized counter predictions, not hardware parity
measurements or matched reproductions of the photographs. Inputs, explicit
diagnostic changes and accepted-edge traces are in `v3-bcopy-replay/`.

## Reproduction and scope

Ignored working artifacts: `.roster-scratch/v3-resume/` contains fresh/late
inputs, paired-results.json and input assumptions; `v3-sdram/` contains the
controller adapter and caller512 trace; `v3-cpcse-policy/` and
`v3-rtl-policy/` contain the CRTC-only comparisons and protocols;
`v3-full-entry/` contains full initialization, table comparison and passive
RFD/save observations; `v3-scope-portable/` holds the CPU-driven prototype,
and `v3-bcopy-source/` / `v3-bcopy-replay/` hold the COPY comparison. Their
READMEs record exact commands, formats and limits. All copyrighted media,
snapshots, RAM/disassembly and images remain ignored. Production RTL and
tracked simulation sources are unchanged; exploratory builds/runs are not
behavior gates. The next requirement is a source-grounded first divergence
and fail-first vector before any timing/state repair.
