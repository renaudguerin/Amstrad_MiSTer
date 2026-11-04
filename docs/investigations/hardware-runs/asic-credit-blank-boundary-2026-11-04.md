# ASIC credit clipping: byte/blank boundary — 4 November 2026

The ASIC credit candidate shifts only terminal horizontal RGB blanking. A
genuine DSC4 replay attributes the missing pixels to that boundary, and four
independent normal-boot AmSpirit controls measure both edges. The retained
regression fails before the change and passes afterward. Independent Gemini
4 Argon high review accepts the implementation. Full synthesis and targeted
MiSTer checks now pass; credits are clear, with bounded SHAKER photo support.
The earlier private probes below are evidence history, not the final contract.

## Controlled integration result

Copied production motherboard/ASIC RTL runs with the real GHDL-translated T80
and READY timing. Six distinct bytes `B6,6D,E3,9C,5B,A5` provide independently
decoded Mode1/Mode2 controls over 192 master clocks. Both controls match every
RGB and pen sample, with the same unique first-byte RGB origin at tick 259.
The comparison has no byte transposition or per-mode bit/cadence discrepancy.

Only preprogrammed R2 differs:46 for the unblanked control,22 for the blanked
case; R3 low=2. No JIT write or CPU release-phase search is involved.

| Observation | Master ticks |
|---|---|
| Raw HSYNC | 255 through 382; falls 383 |
| RGB blank | 259 through 386; releases 387 |
| Both pulse widths | 128 |
| First suppressed correct pixel | 259: byteB6, pen 1, active DE/held DE, palette redF00, final RGB 000 |

All 192 data, pen, DE and palette observations remain identical to the
unblanked case. Four complete marker bytes are suppressed;5B is the first
visible marker after the pulse. The first RGB difference is therefore blank
precedence, not a corrupt fetched byte. This body-window result does not
establish absolute physical MA/DE alignment.

The motherboard assembles the even/odd word from actual VRAM reads;
`asic_video` latches its halves at CLKEN and pixel7, then registers RGB at
PIXEN. The measured first byte follows physical MA3105 while current MA has
advanced to3106/HCC22. `blank=HSYNC` wins at RGB registration while the data
pipeline continues underneath it. This is an ASIC pixel-path observation,
upstream of `crt_filter`.

## Authority and initialization limits

Rendered French ACCC v1.11 §14.7.2 p144 distinguishes type 4's19th Mode 2 pixel
from a proposed type 3 17th pixel. It explicitly calls the type 3 timing an
assumption pending a future diagram. Relative to the unique byte origin,
measured blank onset is index 0; adopting that assumption would require
index 16/tick 323,64 master clocks later. That is a measured16-dot gap against
a working assumption, not a hardware-golden failure. It does not choose
whether to alter data alignment or blank timing.

The private SNA substrate explicitly seeds lab counters and IRQ state at zero;
those values are not measured hardware state. The CPU resumes through the
existing restoredFFF0 trampoline to HALT8000, without host-counter release.
Observation begins after two complete frame origins, row6/raster0/C018.
All four cases reach the same observation after 2753663 clocks. Hidden VMA
uses production loader initialization. No arbitrary title-checkpoint fidelity
is claimed by the primed byte control.

The fresh FR Plus DSC4 credits snapshot is a separate input, SHA256
`c73625e4ce42dc3711389178f57e62ef6133d34d981e476d2f4d5e93b9c4c936`.
Its normal-boot reference has complete labels and names; no post-initializer
restart or PC/RAM/counter patch was used.

## Genuine title attribution

One unchanged-input replay runs the real CPU naturally from PC2085 through
23552 fetches and 2842507 master clocks, without ROM dependencies. Two complete
natural frame wraps precede observation. Standard header/RAM and CPC+ state
pass through production decoders; the emulator-private SPRT chunk is skipped.
There is no trampoline, host-counter release, NOP fitting or state patch.

At fixed palette calibration and translation, 14878 colored pixels match the
device image, with 76 differences retained. The reference alignment has 208
differences. Parent inspected both label crops. All 139 reference-colored label
samples selected by post-edge HSYNC have the correct source pen/palette and
active DE. Of those, 132 are black at RGB; seven onset-edge controls retain
their preceding RGB and are already visible. The frozen fail-first oracle
distinguishes the 132 missing samples from those seven controls.

The first missing sample is reference(387,129), replay(341,89), tick 1930463:
pen 1/palette 666/DE1/HSYNC1 produces RGB 000. The first GFX colon uses original
RAM bytes DB99=12,E399=31,FB99=12,C3E9=31. Those bytes are identical in final
RAM and reach the captured serializer halves; decoded pens remain present
under blank. This attributes the clipping upstream of the sync filter.

Full-frame comparisons retain all differences; their translation is not a
physical beam-origin measurement. Hidden VMA/divider/serializer state is not
serialized. The fixture omits the external cartridge/Plus-MMU path, inactive
for this input; it would not qualify a title using those mappings. These
limits permit the bounded blank-plane attribution, not exact checkpoint or
whole-frame equivalence. No absolute hardware delay is inferred from it.

Title evidence is frozen separately in `asic/credit-title-attribution/` under
the same ignored root: input/source hashes, normal-loader adapter, first-fetch
trace, full frame/colon/suppressed-pixel ledgers, fixed comparisons and handoff.
Commands there are `bash build.sh`, `./obj/title_replay > run.log 2>&1`, and
`python3 compare.py`.

## One delay hypothesis: informative, not accepted

A separate private copy tests exactly one horizontal-blank delay: 16 native
16MHz samples, or 64 master clocks. Raw HSYNC, mode/IRQ events, vertical blank,
register writes and the complete data path remain unchanged. The qualified
type 3 diagram and the independently attributed missing labels motivate this
experiment; neither is treated as proof of that silicon delay.

The frozen 139-sample oracle goes from 7/139 visible to 139/139. All 132 missing
target samples recover. At the fixed reference translation, whole-frame
mismatches fall from 208 to 92, comprising 76 prior differences and 16 new
trailing pixels. Those new nonblack pixels are at reference x398 on colon
stroke rows where both reference and baseline are black. Names/background
outside the label region do not worsen, but the full image is not exact.

Both controlled modes preserve raw HSYNC 255..383 and every upstream
data/pen/palette sample; candidate RGB blank is 323..451 instead of 259..387.
All 319488 title samples match upstream, as do RAM, first-fetch and run logs.
No timing sweep or second candidate was run. The hypothesis is frozen under
`asic/credit-blank-delay16/` and is **not promoted** to production, CI or an
RBF. Resolving that trailing edge requires an independent edge oracle, not
selecting another delay to erase the 16 pixels.

The first new pixel is reference(398,131), source(352,91), tick1938699.
The preceding PIXEN sample retains byte12/pen1/palette666. At that tick the
input byte is00 and the palette address is0, but the registered pen/palette
still carry the final old grey sample. The delayed blank rises after the
edge; RGB consumes its prior low value and remains grey. At the next PIXEN,
tick1938703, pen/palette and RGB are black. The trace directly records
PIXEN-post state; its pre-edge blank is inferred from the preceding sample
and the only-PIXEN register assignments. This pins the final old-sample
interaction without proving which physical latch edge should suppress it.

The private history also clears to zero on reset/SNA. Releasing into an
already-high HSYNC can expose pixels until it fills, and pre-snapshot pending
history is not serialized. No reset/SNA boundary acceptance was attempted.
Those startup limits are another reason the experiment is not production-ready.

Frozen evidence is under ignored
`local/overnight-20261103/asic/credit-blank-boundary/`: source hashes, adapter
diff, input, executable, build/run logs, full CSVs, `ledger.json`, French render
and `HANDOFF.txt/json`. Commands from that directory are `bash build.sh`,
`./obj/byte_probe > run.log 2>&1`, and `python3 analyze.py > analysis.log`.
Parent verified the run log, executable and ledger hashes against the frozen
handoff. No timing variant, production test, new RBF or device change follows
from this controlled unit alone.

## Normal-boot reference controls

Four independent normal-boot cartridges now measure the missing software edge
contract, without reusing the title's expected last pixel. Guest Z80 code copies
itself into RAM, programs the palette/CRTC, clears VRAM, writes the same six
marker bytes on 200 rows, disables interrupts and halts. The paired inputs use
Mode1/2 and R2=46/22, with R3=2 and otherwise standard63/40/38/25/30/7 timing.
There is no snapshot input, host PC/RAM/counter write or beam-conditioned CPU
release. The code reaches HALT8A44, SPBFF0, IFF1=IFF2=0 naturally.

Parent verifies the exact registers and palette, then the **entire16KiB VRAM**
against the independently constructed 200-row marker pattern. After two further
natural frames, it captures a paused uncropped1024-pixel raster. Render is Off;
bright RGB245 and dark5 separate unambiguously at threshold180. No horizontal
resizing or title-registration search is involved. Parent inspected all four
images through their contact sheet.

| Control | Mode1 | Mode2 |
|---|---:|---:|
| Complete independently decoded marker | 48 Mode2-width samples | 48 Mode2-width samples |
| Normal DE width / outer border | 640 /384 | 640 /384 |
| Marker start relative to DE | 336 | 336 |
| First black sample relative to marker | **15** | **15** |
| Last black sample | **46** | **46** |
| First reactivated sample | **47** | **47** |
| Interior black width | 32 | 32 |

The offline reader first proves the full unique color pattern and border
geometry, then locates black runs without an expected edge. R2's change moves
DE origin by384 samples, exactly24 characters; each case's independently
identified DE origin anchors its marker. Both modes give15..46. This differs
from the earlier16..47 private hypothesis and motivates testing a15-sample
terminal-RGB alignment. It is an **AmSpirit1.17.1/core2491682 measurement**,
not proof that the physical ASIC contains15 stages. French §14.7.2 p144's
qualified type3 working assumption remains qualified.

Frozen source, cartridge hashes, all request/response metadata, readiness
checks, screenshots, snapshots captured for observation only, optical ledger
and `capture_references.py` are under ignored
`local/overnight-20261103/resumed-20261104/asic-credit-edge/`. Readiness uses
`view=raw` atC000 under the guest's C0 mapping; `view=fw` is a separate ROM
view and is not used as CPU RAM. The reader command is
`python3 read_edges.py reference` from that directory. All four cases complete.
The next production regression must first fail on current RTL, retain full
data/pen controls and both edges, and protect reset/SNA history normalization.

## Production candidate and acceptance

`rtl/plus/asic_video.v` samples raw HSYNC into a 15-PIXEN history and uses its
oldest sample only at the final RGB mask. Reset clears it; snapshot apply fills
it from incoming `SNA_HS`, replacing pending transitions from the old machine
state. This is deterministic normalization: the hidden preceding pixel history
is not serialized, so arbitrary mid-pulse snapshot fidelity is not claimed.
Raw sync, counters, mode events, register admission and IRQ consumers retain
their timing. Fifteen is the measured software-reference alignment, not a
claim about a physical ASIC register chain.

The retained `make -C sim asic-blank-alignment-test` uses production T80/READY,
normal motherboard snapshot owners and an original six-byte VRAM marker. On
pre-fix RTL both full 192-clock unblanked controls pass, then each mode reports
120 RGB mismatches at the intended boundary. After the fix both modes pass:
unique byte origin260, raw HSYNC `[256,384)`, RGB black `[320,448)`; data, pen,
DE and VSYNC controls are preserved. A separate public-pin leaf covers 151
reset/reset-free snapshot lifecycle checks. Default-disabled fixture video RAM
preserves existing B18 callers. The target is indexed and runs in production
T80 CI.

Focused verification:

- `make -C sim asic-blank-alignment-test b18-snapshot-test`: PASS, both complete
  mode windows, lifecycle, six classic capture/restore and six host-publication
  checks.
- `make -C sim/plus run/asic_video_tests run/asic_mode_latch_tests`: PASS, 75 video
  cases and mode latch. Three existing raw-HS-relative sample points now observe
  the delayed plateau; their R3=1, colors and sprite/border assertions are kept.
- `python3 sim/select_tests.py --run`: **PASS 44 benches**, once after the final
  code edit. The slow new target has the focused pass above. No classic soak
  change is claimed for this ASIC-only behavior change.

The final production source is semantically identical to the frozen private
15-pixel title candidate. Existing DSC4 replay evidence therefore applies:
139/139 reference label samples pass, including132 newly recovered pixels and
seven previously visible controls; fixed-frame differences fall208→76 with
zero new pixels. CPU, RAM and319488 upstream samples are unchanged. The76
remaining differences are unassigned, not declared hardware errors.

Cross-provider review: guarded `ask-agy`, `gemini-4-argon-high`, run
`20261104T134111Z-80111-6ccd`, **ACCEPT**, no blocking findings, no tracked edits
or repeated gate. Review checked nonblocking-edge arithmetic, both mask edges,
snapshot/reset priority, fixture defaults and test integration. One cosmetic
assertion-label spacing nit is left unchanged to preserve the frozen gate.

The parent verified all eight source/test hashes against the final handoff.
Frozen red/green logs, complete diff, hashes and final gate line are in
`local/overnight-20261103/resumed-20261104/asic-credit-production/`; the review
is in sibling `asic-credit-review/REVIEW.md`. The next acceptance is a full
Quartus artifact with timing closure, then targeted DSC4 credits/main effects,
Amazing and SHAKER A4/BI8/AEAR7 controls. No broad fresh baseline follows yet.

## Full artifact and targeted device acceptance

Source275af8a90c407422d1ff830129652e9659557814, full hosted
[run41843571428](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41843571428),
passes simulation/lint, production T80, synthesis and the required gate. Quartus
17.0.2.602 reports setup0.091ns/hold0.241ns across seven clocks, zero TNS/errors/
critical or implicit-net warnings. Artifact13331597231,
`Amstrad-build-350-1-full`, contains `Amstrad_20261104_275af8a.rbf`,3112820bytes,
SHA2566382ded9dd830310096a769393c1e16f68db9cd876f8b21d9938cc23f6153672.
The MiSTer copy matches. The artifact retains the compiler log but no standalone
STA summary; complete workflow output preserves the successful timing checker.

Same Full/French6128+ setup and unchanged disks/cartridge as7ac11a2. Five
normal-boot targeted runs total85commands/21captures/zero failures/30SSM:

| Target | Execution | Result |
|---|---|---|
| DSC4 |16commands/3captures | All four credit colons and handwritten names are clear. Main otter and later rotating/fire-band image remain present. |
| Amazing |13commands/2captures | Logo and lower text/effect region remain present in both sampled frames. |
| BI |25commands/8captures/17SSM | Widths224/16/32/48 repeat unchanged for OUT(C)/OUTI. Both black edges shift exactly15native pixels at row160. White1µs text now clips LOGON→GON as on the original Plus photograph; the2µs partial first word also has the photographed mask placement. |
| AE/AR |21commands/7captures/12SSM | AE008C is byte-identical to7ac11a2, readable marker09. Six AR images change only between black and their original nonblack RGB values; **zero changed nonblack-to-nonblack RGB samples**. Fine-stripe/color families remain consistent with the six available original photos. |
| A4 |10commands/1capture/1SSM |0037 is byte-identical to7ac11a2. Top3C/3D and OUTI deadlines stay accepted; lower PPI words remain the separate known discrepancy. |

Parent inspected every native image and all eight original BI photographs,
including the full1/2µs photographs, plus the six AE/AR references. These support
specific widths, clipping and pattern families, not pixel-identical CRT geometry
or complete ASIC compliance. The unchanged A4 RAM-observation question remains.
Each SHAKER SSM record carries requested/applied Full mode0; title captures pin
CFG application but do not claim screenshot-time scaler lock from absent markers.

Frozen artifact/provenance is in ignored `resumed-20261104/asic-credit-build/`;
all five device runs are under sibling `device/` with275af8a suffix. BI's exact
edge ledger and physical-reference contact sheet, AE/AR's pixel-difference
ledger, and A4's PNG identity comparison retain the bounds above. Original
photo URLs/hashes remain in `asic/b3-r3jit-real-plus-ref/index.json` and
`cpcse/ae-ar3-real-plus-ref/index.json`. No broad fresh baseline was launched.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
