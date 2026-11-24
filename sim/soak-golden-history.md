# CRTC soak golden hash

The fixed seed randomized CRTC soak uses this current expected hash:

| Hash | Set on | Scope |
|---|---|---|
| `0x1f55e98534921b92` | 2026-11-24 | CRTC0 dual C9/R9 and C9/R5 comparator in ordinary additional management: R4 restored to C4 re-enables the C9==R9 reset and C4 increment, R5 endpoint keeps priority; bounded B26 candidate for SHAKER E6 0409 cases 27/28 (French ACCC 11.2.2 pp82-83, 13.2.4 p108) |
| `0x6bb49f9262b5c886` | 2026-11-13 | CRTC1 active VSYNC width held at R0=0; bounded B26 candidate for original SHAKER AI/008D high samples, with C4/C9 still advancing (French ACCC 13.3 p115) |
| `0x974e5c048eb7f877` | 2026-11-06 | Current type 0 and type 1 behaviour |

Run `make -C sim soak SOAK_EXPECT=0x1f55e98534921b92` to compare a change. If a behaviour preserving refactor changes the hash, investigate the difference. For an intentional behaviour change, add a new top row with the source or hardware basis and affected behaviour.
