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
