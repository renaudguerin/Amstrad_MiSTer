---
name: cpcse-test
description: Run SHAKER or CPC caller experiments in CPCSyntaxError and inspect its matching source to explain hardware-backed differences from MiSTer.
---

# CPCSyntaxError experiments

Read [the toolbelt guide](../../../docs/cpcsyntaxerror-testing.md) for installed
paths, verified headless commands and evidence limits. Use the bundled CLI and
client before writing another emulator-control layer.

Prefer headless CSL/SSM for reproducible batches. Pin the executable and matching
source revision, disk, script/history, machine/CRTC revision and firmware. Use a
fresh output directory and record default CSL errata versus `--no-errata`.
Canonical author scripts and all firmware/media remain private and untracked.

The useful sequence is: original-hardware result → fresh CPCSyntaxError match
where MiSTer fails → relevant source/state trace → French-ACCC check → focused
failing RTL proof. Do not treat execution completion or a source comment as a
hardware verdict. Older portal screenshots do not establish current-version
behavior. For cases CPCSyntaxError fails or cannot resolve, use AmSpirit as the
additional reference; original hardware remains authoritative.

Distinguish raw counters/status/pixels from monitor response and acquisition.
CPCSyntaxError's character-granularity I/O scheduler cannot settle every physical
bus edge, and its SHAKER-trained monitor renderer is not an independent monitor
oracle. Do not transplant a broad rule merely because one page matches.

The GUI command API is opt-in TCP newline JSON, not AmSpirit HTTP. Use a distinct
port such as 7000 if both run. Headless CSL needs no API listener. Avoid sharing
the app's settings files between live GUI and isolated batches.

Use original boot/pre-initialization state for authentic replay. Verify snapshot
version before reading counters; restarting an initializer from a later snapshot
can change RAM and invalidate the comparison. Record the actual current state
before presenting a live experimental screen as a reference.
