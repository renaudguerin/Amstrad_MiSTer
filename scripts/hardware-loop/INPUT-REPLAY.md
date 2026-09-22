# B17 timed keyboard replay

`input_replay.py` runs on the MiSTer with Python 3 and `/dev/uinput`. It submits a
bounded JSON schedule through one virtual keyboard (`B17 replay keyboard`, USB
identity `0000:b017`), including releases through that same device on completion,
SIGINT, SIGHUP or SIGTERM. No MBC installation is needed. Existing CSL 1.5
`wait_ssm`, CSL 2.6 cases, and the hardware-loop driver are unchanged.

```sh
scp scripts/hardware-loop/input_replay.py root@mister:/tmp/
scp scripts/hardware-loop/cases/sonic-start.json root@mister:/tmp/
ssh root@mister 'python3 /tmp/input_replay.py /tmp/sonic-start.json'
```

Do the Sonic setup below first. General keyboard schedules need no joystick map.
The helper does not load a core, select a model, install a map, or take screenshots.
Use the existing hardware-loop/MGL/CSL tools for those operations. Device ownership
must stay with one operator through setup, replay, capture and restoration.

## Schedule

```json
{
  "version": 1,
  "duration": 2,
  "events": [[0, 106, 1], [0.5, 106, 0]]
}
```

Each event is `[seconds, Linux keyboard code, value]`, with value 1 for press and
0 for release. Times are nondecreasing offsets from replay start, after the
uinput discovery delay (`--settle`, default 1 second). Duration includes any
trailing quiet period. Keys still held at duration are released automatically.
Duplicate presses and unmatched releases are rejected before opening uinput.
Only keyboard codes 1–255 are supported; no controller axes or joystick buttons.
Limits are 600 seconds and 100,000 transitions.

Scheduling uses absolute monotonic deadlines so processing each event does not
add cumulative delay. Late events run immediately; this is **wall-clock navigation,
not frame-locked gameplay**. Discovery and host scheduling can vary. A schedule
assumes the intended screen and OSD state; check the captured checkpoint instead
of treating successful delivery as proof of reaching it.

Normal catchable interruption releases the helper's keys and destroys its virtual
device. SIGKILL, power loss, or kernel write failures cannot guarantee delivery of
releases to Main. Reset/reload the core before trusting the next capture in those
cases. A core reset during replay does not stop the process: interrupt the replay
first. The helper cannot release keys held by physical devices or another process.

## Sonic GX: use Main's keyboard joystick mode

The dedicated map routes arrows to joystick directions, left Ctrl to fire 1,
left Alt to fire 2, and Space to fire 3. Main F18 (Linux 188) explicitly selects
keyboard joystick 1; F20 (190) restores normal keyboard mode. These are explicit
mode selections, not blind Scroll Lock toggles. The supplied schedule holds fire 1
for 200 ms, then restores normal mode. This bypasses the CPC keyboard/joystick matrix
separation through Main's existing mapping; no virtual controller framework is needed.

For the tested setup, install this **temporary** map on the MiSTer before creating
the replay keyboard. Refuse an existing file; preserve any user configuration.

```sh
ssh root@mister "python3 - <<'PY'
import struct
path = '/media/fat/config/inputs/Amstrad_input_0000_b017_v3.map'
with open(path, 'xb') as out:
    out.write(struct.pack('<32I', 106,105,108,103,29,56,57, *([0]*25)))
PY"
```

Use a suitable hardware-tested Plus core, load the Sonic CPR through MGL F8, select
6128+ before core loading, and close the OSD. Preserve the original `Amstrad.CFG`
when changing model settings. The tested core predates load-time automatic Plus
selection. See the [device record](../../docs/investigations/hardware-runs/b17-input-replay-2026-09-22.md)
for exact identities, boot timing observations and limitations.

Afterward remove only this temporary map and restore the original configuration.
A normal replay sends F20; an interrupted schedule may leave Main in joystick
mode even though all owned keys are released. Send a standalone F20 press/release
schedule or reload MENU before yielding the device. Custom global keyboard maps,
unique-controller mapping settings or a different Main build may change the route;
validate on that setup before reusing this recipe.

## Recording remains deferred

The original B17 proposal assumed passive evdev reads could observe play without
`EVIOCGRAB`. The current Main itself grabs devices. A real-device test injected
press/release events into the helper keyboard while a second, non-grabbing reader
watched its evdev node: that reader received zero events. Main still consumed
replay input. Shipping that recorder would silently produce empty sessions, so
this slice intentionally contains **no recorder** and captures no host or physical
keyboard activity. B17 recording and physical-controller replay remain open.

Relevant Main source: [input.cpp](https://github.com/MiSTer-devel/Main_MiSTer/blob/master/input.cpp)
(`grabbed`, `EVIOCGRAB`, `get_map_name`, keyboard joystick routing) and
[user_io.cpp](https://github.com/MiSTer-devel/Main_MiSTer/blob/master/user_io.cpp)
(`EMU_SWITCH_1`, `set_emu_mode`). The device record pins the tested Main binary;
these upstream links are navigation references, not its exact source provenance.

Focused tests: `python3 -m unittest discover -s scripts/hardware-loop -p test_input_replay.py`.
They cover write-latency drift, interruption after a potentially delivered press,
setup resource cleanup, continuing releases after one failure, and invalid schedules.
