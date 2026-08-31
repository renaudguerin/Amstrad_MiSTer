# Hardware session 2026-09-01: sync filter modes and Plus defects

User-run session on real hardware against RBF `e5dc56a`, the first build carrying the OSD sync
filter toggle. Screenshots were captured by the user; they are the primary evidence and this
file records what was read from them.

## Sync filter Off: not a usable path

**SHAKER Module A, entry (T) R2 UPDATE DURING HSYNC, CRTC 1.** The test's header text and the
upper hex label rows render correctly. Below that the frame degrades progressively: colour
bands smear, then the lower third becomes dense noise. The failure is not uniform corruption,
it starts partway down the frame, which is where the test begins its per-line R2 writes.

**DSC4.** With the filter on, the title screen renders correctly (LOGON PRESENTS DSC 4, the
four scrolling name texts). With the filter off, the frame collapses into a tall narrow strip
of noise: the scaler has lost horizontal lock entirely and is deriving a wrong line length.

**Reading.** The raw path fails exactly when line geometry varies, which is the condition this
software creates deliberately. Ordinary software displays correctly with the filter off. This
is what motivated splitting `crt_filter`'s two jobs; see `backlog.md` B1 and the live-blanking
mode added in `74882c7`.

**What it does not tell us.** Nothing about whether the R2.JIT black-zone position is correct,
because the display never stabilised enough to judge it. That question is still open and is
what the live-blanking mode exists to answer.

## Turning the filter off fixed none of the Plus defects

Consistent with B1's stated scope limit: the filter sits after the CRTC in the video path and
does not touch VRAM addressing, DE-gated data, border and ink, or the sprite and palette path.
The Plus symptoms have a different cause.

## Burnin' Rubber: a sprite renders where it should be hidden

**Observation.** On the title screen, a reddish circular arc is drawn at the extreme right edge
of the display, partially clipped by the screen edge. It should not be visible at all at this
point: the object is meant to scroll in from the right later. The rest of the screen, including
the logo and text, is correct.

**Reading.** This is a sprite horizontal position or visibility fault, not a rendering fault:
the sprite's pixels are correct, its placement is not. The specific shape, an object that
should be off-screen to the right appearing clipped at the right edge, is the signature of a
comparison that folds an off-screen coordinate back into the visible range rather than
discarding it. Candidates, in order of likelihood: the sprite X comparison truncating the
coordinate to fewer bits than the ASIC uses, a signed versus unsigned mismatch, or a clamp
where hardware discards.

This is the most diagnosable of the reported Plus symptoms and the best first target for any
frame-diff harness, because the expected result is unambiguous: the sprite is absent.

## On reference screenshots

Several of these are useful without a reference image, because the defect is a categorical
statement rather than a pixel comparison: "this sprite should not be visible", "this screen
should not be black", "this frame should not be noise". A reference matters only when the
question is *how much* something is displaced. Both the sync-filter captures and the Burnin'
Rubber capture are in the first category and are directly actionable as they stand.
