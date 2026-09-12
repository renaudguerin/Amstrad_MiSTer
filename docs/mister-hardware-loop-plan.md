# Unattended MiSTer hardware test loop

Plan researched 2026-09-08 for backlog B2/B4. No device was accessed or changed.
The first deliverable is a repeatable capture of one stable SHAKER screen through
SSH, using existing Linux facilities. Exact SSM capture is a later, separate gate.

Host preparation is implemented at `7e39204`: see the
[driver guide](mister-hardware-loop-driver.md) and
[ARM cross-build recipe](mister-mbc-cross-build.md). Offline validation and
review pass. The [September 12 device run](b2-device-capture-2026-09-12.md)
now demonstrates DSK boot, French input and repeated numeric SHAKER captures;
active OSD mode and exact event capture remain distinct limits.
The CSL/SSM follow-on is planned in
[csl-ssm-implementation-plan.md](csl-ssm-implementation-plan.md).

## Start with the tools already available

Current Main_MiSTer source at
[`f8dc68e`](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/input.cpp#L6229)
accepts `load_core` and `screenshot` through `/dev/MiSTer_cmd`. Installed Main must
be checked: a feature in current source is not proof that the device has it.
Send one command at a time; this interface is not a general newline command queue.

```sh
printf '%s\n' 'load_core /media/fat/_Computer/Amstrad_test.rbf' > /dev/MiSTer_cmd
printf '%s\n' 'screenshot capture_001.png' > /dev/MiSTer_cmd
```

The second example assumes `screenshot_image_format=png`. Prefer the native-sized
capture for comparisons. The optional `scaled` argument performs software scaling
from the captured buffer; it is not an HDMI screenshot.

[MiSTer Batch Control](https://github.com/pocomane/MiSTer_Batch_Control/blob/3873450d413c30e6b0339e6b3dbf2373e0e5a74a/Readme.md)
is the smallest candidate for missing glue: a public-domain C command-line tool,
using Linux `uinput`, with `raw_seq` press/release injection and MGL support.
Prefer an already installed input service if it supplies the same functions.
[mrext Remote](https://github.com/wizzomafizzo/mrext/blob/f10a879626d04ef55f1b20d181502b8b0d5552a6/docs/remote-api.md)
offers launch, keyboard, and screenshot retrieval over HTTP/WebSocket, but an
SSH-only loop does not need another permanent service.

Use the [official MGL format](https://mister-devel.github.io/MkDocs_MiSTer/advanced/mgl/)
for core/media loading where supported. The actual fork's `Amstrad.sv` CONF_STR
at `65364ee` supplies these slots; do not use MBC's built-in Amstrad disk mapping
without checking it:

| Media | Core entry | MGL operation to validate |
|---|---|---|
| DSK drive A/B | `S0` / `S1` | mount, `type="s"`, index 0/1 |
| Plus cartridge | `F8` | memory load, `type="f"`, index 8 |
| Snapshot | `F6` | memory load, `type="f"`, index 6 |
| Tape | `F4` | memory load, `type="f"`, index 4 |

Mounting a disk does not type BASIC's RUN command. Loading a CPR resets the
machine and executes the cartridge. Configure model/CRTC and media separately,
then verify the SHAKER footer: successful file transfer alone does not establish
the selected emulated machine. Prefer DSK/CPR for the initial loop; the B8 review
found incomplete Plus snapshot restoration, so SNA is not an assumed shortcut.

## First implementation slice

Owner: the B2 implementation task, once SSH access is supplied.

1. Inventory installed Main version, current core, input tools, screenshot
   configuration and output mode. Record the selected RBF, media hashes, model,
   CRTC, Full/Live/Off setting, scaler/CRT settings and reset/load sequence.
   Reuse working device tools; build MBC only if needed.
2. Load a named test RBF and DSK through verified Main/MGL commands. Inject the
   minimum key sequence to reach one stable SHAKER test, then release all keys.
   Keep an input log and bounded timeouts. Do not begin with the entire suite.
3. Request a unique native screenshot. Main accepts only one pending capture and
   drops additional requests while busy. Wait for a completed, decodable file,
   copy it through SCP, and check its dimensions and identity locally.
4. Repeat the same case three times. Inspect the images for tearing, unexpected
   frame alternation and different test states before calling the loop repeatable.
5. Add the named DSC4 and unresolved Plus cases only after this round trip works.
   Preserve the September 2 Burnin' Rubber right-edge pass and tentative Amazing
   Demo pass as regressions. A changed image is not automatically an improvement.

This should be one small host driver plus case descriptions. A database, web UI,
custom daemon and new on-FPGA command processor are not prerequisites.

## What screenshots can establish

Main's [capture code](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/scaler.cpp#L539)
copies the scaler buffer beginning at `0x20000000`, using its header dimensions
and stride, then saves asynchronously. The
[frame callback](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/frame_timer.cpp#L124)
uses a polled frame counter or timer fallback. This gives useful displayed-image
evidence, not an exact FPGA event timestamp or raw RGB/sync trace.

The current copy has no visible buffer ownership/freeze handshake. Triple
buffering and interlaced half-frame updates make a coherent event-matched image a
separate problem; compare the device's Main and this fork's vendored ASCAL before
altering either. Repeated stable-screen capture is the inexpensive first gate.
Do not promise that a screenshot diagnoses which raw sync edge caused a failure.

## CSL: compatibility before exact timing

Use the [current SHAKER bundle](https://shaker.logonsystem.eu/Shaker_CSL.zip),
linked by the [CSL/SSM page](https://shaker.logonsystem.eu/ssmcsl). At research time
it contains CSL v1.4 (5 July 2026), SSM v1.1 (July 2026), SHAKER scripts for
modules A–E and CRTC 0–4, and `shaker26.dsk` / `shaker27.dsk`. Older indexed PDFs
are not the current specification. The page's CSL PDF link is inconsistent with
the bundle filename; the bundle is the verified source for that document.

Implement a bounded subset: `reset`, `crtc_select`, `disk_insert`, `key_output`,
`key_delay`, ordinary `wait`, and named screenshot. Preserve script ordering.
CSL `wait` uses emulated microseconds; Linux delays are an approximation and must
be reported as such. `key_delay` has press, between-key and optional carriage-
return durations. Bundled scripts can declare `csl_version 1.0` while using the
three-argument form: test against the supplied corpus, not only its declarations.
`keyboard_write` uses ten active-low keyboard-matrix bytes and needs a deliberate
mapping or core-side matrix interface, not a blind host-key translation.

Reject unsupported commands with an explicit reason. Do not silently approximate
`wait_ssm0000`, motor transitions, exact `wait_vsyncoffon`, `screenshot vsync`, or
snapshot export and then claim full CSL compliance.

## SSM: an event detector does not preserve the requested image

[SSM v1.1](https://shaker.logonsystem.eu/Shaker_CSL/SSM-STANDARD-EN.pdf) specifies
two consecutive executed ED-prefixed instructions, `ED LL ED HH`; the marker is
`HH*256+LL`, recognized after the HH byte is read. Do not scan arbitrary RAM data.

| Marker | Current meaning |
|---|---|
| `0000` | releases `wait_ssm0000` |
| `FFFE` (`ED FE ED FF`) | named screenshot |
| `FFFF` (`ED FF ED FF`) | snapshot |
| `FFFD` / `FFFC` | Sikoview logging start/break |

Older standalone `ED FE` / `ED FF` descriptions must not drive an implementation.
Use explicit reserved cases: the current document's ordinary allowed-byte list
and reserved table need separate handling.

Stock HPS software cannot observe the FPGA CPU instruction stream. A later B4
slice needs a passive recognizer using actual instruction fetch/retirement
semantics, then an event record with marker, sequence, core clock, frame/field and
raster position. A bounded queue must report overflow rather than silently lose
events. It can establish that a marker executed; HPS polling can retrieve it later.

Exact capture additionally requires retaining the image associated with that
event. Specify whether the target is the framebuffer accumulated at HH, the
preceding complete frame, or the following complete frame; only the first matches
the literal immediate SSM request. Determine how to retain that representation
and transfer buffer ownership before claiming conformance. Pausing the CPU alone
does not freeze the video engine or guarantee the correct buffer. Exact CSL waits
similarly need core-clock scheduling or observed core events.

## Reference images and comparison

The portal's [test API](https://shaker.logonsystem.eu/api/tests) supplies test names,
CRTC applicability and `subtests[].hex` identifiers; its
[architecture API](https://shaker.logonsystem.eu/api/archs) identifies emulator
versions. At research time Amspirit is listed as 2.0. The site's own
[image-routing code](https://shaker.logonsystem.eu/js/results.js) supplies the
filename mapping. Use it instead of guessing associations from filenames.

Cache only the selected cases initially under ignored `docs/references/`, keeping
original images and a local URL/version/test/CRTC manifest. Do not commit the
corpus; establish image redistribution terms separately if publication is later
requested. Keep source attribution and a direct Compendium link.

Compare our repeated captures first, then align crop, scale, field and colour
representation with Amspirit. Use side-by-side crops and image overlays to locate
differences; automated geometry/palette metrics can triage them. Avoid treating
raw pixel inequality as failure or relying on a visual model to grade hundreds of
nearly identical cases. Amspirit is a useful comparison target; hardware remains
the final authority. Logon photographs corroborate physical behavior but are not
pixel-exact screenshots.

## Completion gates

- **B2 first slice:** three repeatable, named, retrievable captures with verified
  model/media/build identity; no unsupported timing claim.
- **B4 CSL subset:** supplied script fragment reaches the correct test and records
  each approximation or unsupported command.
- **B4 event slice:** executed markers are identified once, in order, with explicit
  overflow behavior, independently of HPS polling latency.
- **B4 exact capture:** event-to-image association is demonstrated under buffering
  and interlace, not inferred from a screenshot filename or successful command.
