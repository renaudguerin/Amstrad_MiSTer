# Remaining Plus title defects

## Hardware position

User report on 2026-09-27, following delivery of `cf62d5f`:

- Copter 271, World of Sports BMX and all listed CRTC3 checks are fixed.
- Prehistorik II is almost fixed, with one corrupt line immediately above the
  bottom HUD. That line is absent on `edaa15b`.
- Eerie Forest's left-edge defect remains.
- Fire & Forget 2 has sky-gradient flicker and music slowdown, also present on
  `edaa15b`. Symptoms occur in no-input attract mode. The user subsequently confirms gameplay on `84e6969` has a stable sky and
  normal music; its title-screen issues are outside this bisect.

Hardware and visual testing remain with the user. This investigation adds no
hardware capture, emulator replay or RTL change. Similar slowdown symptoms do
not establish the same cause as the repaired BMX status-classification bug.

## Separate regression ranges

For Prehistorik II, the production changes from `edaa15b` to `cf62d5f` are:
`bd65578` shaped ACK, `1eec469` cross-line PRI, `8c657cc` live sprite Y
eligibility, `bbd7348` sprite frame-restart cache, `7596a7c` DMA operation-bit
handling, and `0c2cebb` ACK provenance repair. A line at the playfield/HUD seam
makes raster split timing worth investigating, but its location alone does
not distinguish a raster event from sprite/background fetch behavior. Avoid
retesting the entire game against builds already known to have gross ACK
regressions; a targeted diagnostic or isolated candidate is preferable once
its register sequence is established.

For Fire & Forget 2, `edaa15b` is a confirmed bad endpoint, not a good baseline.
The confirmed gameplay interval is now `84e6969` good to `edaa15b` bad. Keep cartridge, model and sync settings
constant. Record sky stability and music cadence separately at the same scene.

## Source lead and limits

CPCEC at `/Users/renaudg/code/cpcec`, source commit
`c025aab961a796b918cc99bc3e16216ea65bb5d1`, explicitly names Fire & Forget 2 in
its changed-PRI-write handling (`cpcec.c:2110`). Our production GA latches
programmed interrupts from its raster event or deferred event; it has no
explicit PRI-write event. This is a concrete implementation difference, not
proof that adding CPCEC's entire rule is correct on hardware.

Keep three questions separate: request assertion on PRI writes, clearing a
pending request on PRI writes, and the phase of ordinary raster interrupts.
The latter two already have conflicting references or accepted behavior to
protect. See [the PRI source comparison](references/amspirit-pri-phase-2026-09-26.md)
and [the accepted ACK repair](ack-provenance-regression-2026-09-26.md).

Before implementation, establish the game's relevant write/IRQ sequence and
an independently grounded failing deterministic case. In particular, compare
writes during raw HSYNC before and after our monitor-HSYNC event, same-value
writes, PRI zero, pending classic requests, and ACK overlap. Do not retime all
ordinary PRI events or import CPCEC's shared pending-request clear merely to
make one title agree.

### Focused CPCEC comparison result

Read-only delegated analysis found one title-specific comment, at
`cpcec.c:2110`; the CPU and AY headers contain no FF2-specific sound handling.
`asic_regs.v:505–508` stores PRI, while `asic_ga_timing.v:621–625` compares it
only at the existing raster events. If a changed current-line PRI write occurs
after those events while raw HSYNC remains active, CPCEC asserts a request and
our core can miss it. This is the highest-value discriminator. Whether FF2
actually executes that sequence, and whether its music is serviced by that
handler, remain unverified. Do not turn a plausible common explanation for
palette flicker and music cadence into an established cause.

CPCEC's changed-value guard also matters: its comment does not imply every
write, including a same-value write, must retrigger. Its ordinary raw-HSYNC
assertion event (`cpcec.c:874–877`) and clear-on-other-nonzero-PRI-write branch
(`2112–2113`) are separate differences requiring separate evidence.

## Confirmed good endpoint and next human check

The user confirms FF2 gameplay on full RBF `84e6969` has stable sky gradients
and normal music. Title screens have separate issues; they are not the pass/fail
criterion for this bisect. Earlier reports locate the bad symptom in no-input
attract mode; compare the same visible gameplay scene across builds.

Good RBF:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/Amstrad-local-build-12-1-full/Amstrad_20260901_84e6969.rbf`
SHA256 `5787f6b8ed05ee8b9ad56506aa38b63be7c0e5b33de0c7a6c1860758d52f5cbf`.

Next candidate: `88262b9fcffdd40df2a5d28d1cb4278f863b4caa`, an ancestor of
`edaa15b` and descendant of `84e6969`. It divides the interval into 33 earlier
and 30 later RTL-changing commits (`git rev-list --count <range> -- rtl`).
[CI run 34789878189](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34789878189)
passed simulation, local full synthesis and required gate. Downloaded artifact
`Amstrad-local-build-221-1-full`, ID `10328352517`, reports `build_mode=clean_full`.
Setup minimum +0.387ns, hold minimum +0.241ns, zero TNS in the timing summary.

Delivered RBF:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/Amstrad_20260914_88262b9.rbf`
SHA256 `bad7d36995c5f480c9328aae1b7f0174881214998786b5610b18bcd11e7b1076`.
Task copy and reports: `output_files/ff2-bisect-88262b9/`.

Keep 6128Plus / Full sync and the cartridge unchanged. Judge stable sky and
normal music in the affected gameplay scene, not unrelated title defects.
If good, narrow to `88262b9..edaa15b`; if bad, narrow to
`84e6969..88262b9`. Inability to reach the scene is inconclusive.

### Implication for the CPCEC lead

Reading the good `84e6969` GA shows it also lacks a PRI-write trigger and fires
on monitor-HSYNC fall. Therefore missing CPCEC set-on-write behavior alone
cannot explain this regression. A later state/timing change might expose that
limitation, but it is no longer sufficient grounds for the first repair.
The midpoint includes the September 13 PRI line-alias, DCSR acknowledge and
coincident-raster/ACK changes; its result separates these and earlier changes
from later snapshot, DMA, pending-classic and sprite repairs.


Cartridge found: `local/test_media/cartridges/01_PlusGames/Fire And Forget II.cpr`,
SHA256 `ce72fcf911b4b403a5012f8dedabd567c80a8af2e43e1b0646fd55468e48e794`.
Use the user's existing cartridge if different and record its identity before
comparing outcomes.

The scout found retained GitHub full artifact `Amstrad-build-256-1-full`,
run `35824567406`, artifact `10735122629`, for
`41a1f277b5ed71c629f5935f0e688a98a3e6680b` (expiry 2026-10-07 at lookup).
It is a possible closer endpoint, not a confirmed good FF2 build. Its recorded
RBF SHA256 is `6c36309368659edbbfe1044e09a804639f6b7ec9c02b68526ff7d488e122b331`.
Download only as needed after the first result. Skip local `566e0c7` as a bisect
point: its RTL is identical to `edaa15b`.


## Second bisect result and next candidate

The user confirms `88262b9` has both sky flicker and slow music. The remaining
interval is `84e6969` good → `88262b9` bad. The exact 16/17-commit midpoint
`91b7d41` and nearby September 8–12 runs no longer expose downloadable
artifacts at lookup; no rebuild was started.

Next candidate is retained full build `bee92a6a371bda7201a36281d37ce555affb7bf1`,
immediately before the September 13 interrupt changes. Only four RTL commits
separate it from the bad `88262b9`:

- `b5c3014`: PRI line compare requires bit 8 clear.
- `9faa140`: DCSR bit 7 sampled at acknowledge rather than raster fire.
- `a0778b6`: PSG R7 reset value.
- `f455f79`: preserve raster fire coincident with an in-flight acknowledge.

A good result isolates those four; a bad result moves the endpoint back to
`bee92a6`, requiring an earlier retained local artifact or a historical rebuild.
Use the same gameplay sky/music criterion and ignore unrelated title defects.

[CI run 34746071976](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34746071976)
passed simulation, full synthesis and required gate. Artifact
`Amstrad-build-209-1-full`, ID `10314411698`, downloaded to
`output_files/ff2-bisect-bee92a6/`; reports identify a clean full build.
Delivered file:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/Amstrad_20260913_bee92a6.rbf`.
SHA256 `8f8ca8020535d5fb62fabec5e0dfc516ed4da9d2f12e0688f0448a6d2edd21fc`.


## Third bisect result: four-change interval confirmed

The user reports `bee92a6` working against the same sky/music criterion.
Current interval: `bee92a6` good → `88262b9` bad. Next test is
`05cb9fded0651b49298326a5cf4e136ab2b1a718`, which includes `b5c3014` (PRI line
compare) and `9faa140` (DCSR acknowledgement sampling), but excludes `a0778b6`
(PSG R7 reset) and `f455f79` (coincident raster/ACK latch).

If bad, test between the first pair; if good, test between the latter pair.
No RTL or simulation changes are needed for this historical hardware bisect.

[CI run 34767160549](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34767160549)
passed simulation, full synthesis and required gate. Downloaded artifact
`Amstrad-build-215-1-full`, ID `10320944161`, reports `build_mode=clean_full`.
Task copy and reports: `output_files/ff2-bisect-05cb9fd/`.
Delivered RBF:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/Amstrad_20260913_05cb9fd.rbf`.
SHA256 `a81d89cc4382ed65da91a96498b822b49ade2331e4f2fd2365e1e0f9009659af`.


## Fourth bisect result: two-change interval

The user reports `05cb9fd` bad. Current interval is `bee92a6` good →
`05cb9fd` bad. Only `b5c3014` (PRI line compare requires bit 8 clear) and
`9faa140` (DCSR bit 7 sampled at acknowledge rather than raster fire) change
RTL in that interval. The later PSG reset and coincident-raster latch changes
are not needed to trigger the symptom.

Next check: `c595031b06b58cb7b59693fe1567904dde505635`, which contains only the
PRI comparison change relative to the good endpoint. If bad, the introducing
RTL change is `b5c3014`; if good, it is `9faa140`. This identifies a triggering
change, not automatically the correct repair: an accuracy correction may expose
another missing behavior, so do not revert it without examining the interaction.

[CI run 34757536661](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34757536661)
passed simulation, full synthesis and required gate. Downloaded artifact
`Amstrad-build-210-1-full`, ID `10317899695`, reports `build_mode=clean_full`.
Task copy and reports: `output_files/ff2-bisect-c595031/`.
Delivered RBF:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/Amstrad_20260913_c595031.rbf`.
SHA256 `b6b5aa253dd8a696a4473d6554b086a0c8b89c143d1bd7d1cbde0a399f23e5c7`.


## Completed hardware bisect

The user reports `c595031` bad. Adjacent relevant endpoints are `bee92a6` good
and `c595031` bad; their only production RTL difference is `b5c3014`:

```verilog
// Before: ignores the ninth counter bit
({crtc_line[8], pri} == crtc_line)
// After: excludes counter values 256 and above
({1'b0, pri} == crtc_line)
```

This identifies the regression trigger. It does not prove the old comparison
is the physical ASIC rule: the same change repaired Copter 271's logo palette
on device, and the documented primary-source conflict remains unresolved.
The user owns visual checks; no further historical build is needed to locate
the introducing RTL change.

A read-only Astra-high source comparison finds CPCEC constructs the same
nine-bit line value and uses full equality, while also implementing the
FF2-specific changed-PRI-write event. That is a concrete compatibility model
where FF2 does not require n+256 aliasing. The old alias event may have masked
a missing current-line write event, but this needs an actual game write/IRQ
sequence or independently grounded probe before an RTL repair. An independent
Opus5.5-high assessment `20260927T004637Z-50401-f7ec` completed successfully.
It independently confirms the one-line production delta and favors a missing
PRI-write event masked by aliasing, conditional on a game trace. Its CPCEC read
was denied outside its allowed directories, so that part relied on repository
notes; the native Astra pass did inspect CPCEC directly. No tests or edits were
performed by either reviewer. Private Opus output is preserved in
`docs/specs/crtc3-2026-09-25/ff2/opus-opinion/`.


## Resolving the source conflict on original hardware

The user has a real Plus and GX4000 and offers to run a CPR. This allows a
direct test of the disputed rule instead of inferring hardware behavior from
game compatibility. A purpose-built diagnostic is being prepared; no RTL
change is justified solely by the completed bisect.

The original [Quasar ASIC article](https://quasar.cpcscene.net/doku.php?id=assem%3Aasic)
explicitly qualifies the PRI=10 / line266 example by sufficient displayed
height, mentioning R6. The shorter CPCWiki statement omits that condition.
This is a hypothesis to test, not enough evidence for a new R6 gate. Copter's
saved snapshot has R4=38,R5=0,R6=25,R9=7 (312 total,200 displayed), but its
handler deliberately uses PRI255 outside the displayed region, so a general
vertical-display gate would conflict with that program too.

CPCEC computes `crtc_line=(C9&7)+8*(C4&63)` at `cpcec.c:271`, updates it at
1056, and uses full equality at 874 and 2106. Our motherboard's line input
`{plus_vc[5:0],plus_rc[2:0]}` is equivalent. No hidden eight-bit truncation or
R6 gate was found in CPCEC. Its changed-write trigger is still a distinct
possible missing behavior, independent of whether ordinary IRQs alias.

AmSpirit lite1.15.1 is reachable. Its live API documentation confirms CPU-PC
breakpoints and CRTC registers/rasterline/VSYNC, but no direct ASIC PRI state,
raw HSYNC or C4/C9. A snapshot's CPC+ chunk plus breakpoints at identified PRI
writers can recover program intent; an exact sub-line event trace requires
additional instrumentation or a controlled program. Another emulator source
is not needed before the original-hardware alias probe.

Initial diagnostic design: count interrupts over 32 complete VSYNC periods,
with constant nonzero PRI and DMA disabled. Compare PRI10 with R6=25 versus
R6=34; include PRI100 as a normal control and PRI255/R6=25 as an off-display
lower-half control. Keep R0=63,R4=38,R5=0,R7=30,R9=7 throughout. Restore R6=25
for the final readable results screen. This distinguishes no alias, unconditional
alias and possible display-height-dependent alias without depending on FF2 or
on a PRI write during the counted interval. Changed-PRI timing is a separate
follow-up once this basic comparator question has a real-hardware answer.


## AmSpirit alias probe result

Generated diagnostic source: `scripts/diagnostics/pri_alias_probe.py`; protocol
and RAM map: `scripts/diagnostics/README.md`. CPR SHA256
`c41a332ba467c0830f894a689f7f525a0794b275b7f83e116218bdb6b368a0b9`.
Generation/structure checks pass and an independent rebuild is byte-identical.
`python3 sim/select_tests.py --run` reports `select_tests: no simulation needed`.
No production RTL changed.

At the user's request, ran the CPR in AmSpirit lite1.15.1/core2491682 with
6128Plus model4, CRTC3. It reached status80 at B002, and all four result words
at B010/B012/B014/B016 contain32 (`0020` hex):

| PRI | R6 | IRQ count over32 frames |
| --- | --- | --- |
|100|25|32|
|10|25|32|
|10|34|32|
|255|25|32|

The API RAM read and photographed emulator screen agree. The preceding
AmSpirit machine snapshot, configuration, rendering and pause state were
restored. Private evidence (results JSON, screenshot, original snapshot and
restore metadata): `docs/specs/crtc3-2026-09-25/ff2/alias-probe-amspirit/`.

This directly establishes AmSpirit's no-alias behavior at both tested display
heights, plus a lower-half interrupt outside display. Along with CPCEC's source,
it supports retaining the current comparison while investigating PRI-write or
phase behavior. It is still emulator evidence; the same CPR is ready for
original-Plus/GX4000 confirmation if needed. The user prefers the emulator
check first and trusts its implementation. No original-hardware run is claimed.


Opus5.5-medium review `20260927T005455Z-54824-c22a` found no blockers in the
probe. Its two recommended hardware-initialization improvements were applied:
write sprite magnifications directly instead of copying through register
readback, and acknowledge any pending PRI before zeroing the measurement count.
The parent inspected those exact changes; the final generator gate and
AmSpirit run were repeated afterward, with the same four32-count result.
Review hashing was permission-blocked; the parent independently hashed the
final CPR and will verify the delivered copy. Review output is preserved under
`docs/specs/crtc3-2026-09-25/ff2/alias-probe-review/`.

Delivered diagnostic:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/pri-alias-probe.cpr`.
AmSpirit results screenshot:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/pri-alias-probe-amspirit.png`.
