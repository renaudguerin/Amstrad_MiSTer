# CRTC soak golden hash

The fixed seed randomized CRTC soak uses this current expected hash:

| Hash | Set on | Scope |
|---|---|---|
| `0x974e5c048eb7f877` | 2026-11-06 | Current type 0 and type 1 behaviour |

Run `make -C sim soak SOAK_EXPECT=0x974e5c048eb7f877` to compare a change. If a behaviour preserving refactor changes the hash, investigate the difference. For an intentional behaviour change, add a new top row with the source or hardware basis and affected behaviour.
