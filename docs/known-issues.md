# Known issues

The core remains experimental. The [latest release](https://github.com/renaudguerin/Amstrad_MiSTer/releases) is the recommended starting point; unreleased commits can behave differently.

- Classic CPC: some CRTC timing tricks and SHAKER cases still differ from original hardware. Passing simulation is not a claim of complete hardware accuracy.
- Plus/GX4000: most tested commercial games are playable, but advanced demos and some timing-sensitive effects still have visual or behavioural faults.
- Snapshot saving from the OSD is a development aid for classic CPC models. It does not currently write a file to the MiSTer SD card.
- CDT tape support is basic and has no seek or rewind.

For a reproducible issue, report the exact release or commit, machine model, disk or cartridge, settings, expected and observed result, and a photograph or capture where useful.
