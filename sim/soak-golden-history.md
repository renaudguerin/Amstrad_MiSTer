# CRTC soak golden hash

The fixed seed randomized CRTC soak uses this current expected hash:

| Hash | Set on | Scope |
|---|---|---|
| `0xcdadadb1e6b70a2e` | 2026-11-26 | CRTC0 paired odd-R9/IVM entry (SHAKER B1 R9E57). At sample 1,598,046 the first raster after an even entry compares raw C9=1, advancing C4 and resetting C9 where the former IVM comparison continued. The projection also samples the three pending-comparison latches. |
| `0xfc229b889c5013ce` | 2026-11-26 | CRTC0 frozen-origin R6 border hold (SHAKER BU `021B`). The earlier pin and state projection matches `0x0bd50d6b801e1607`. The new hash includes the ownership state; the directed pin test covers both restore orders. |
| `0x0bd50d6b801e1607` | 2026-11-26 | CRTC0 selects the incoming progressive phase at the committed origin (French ACCC §19.7.2 p.219, SHAKER B1). At sample 125,462 the departing field flag remains high while `R8=0`. The corrected path asserts raw VSYNC at the origin and latches the line phase. The sampled counters and input mode agree throughout. |
| `0x8155f94fb1bf9cd7` | 2026-11-26 | CRTC0 resumes a newly entered R0=0 adjustment with the ordinary C9+1 versus R5 comparison (French ACCC §13.2.6 p.110, SHAKER A7). The fixed-seed soak first changes the obsolete short-line latch at sample 2 while all sampled pins remain identical. |
| `0x1238e17cd6ec9d3a` | 2026-11-26 | Type1 carries the outgoing C4=R6=0 equality through an actual frame origin (French ACCC §18.2.3 p.190, SHAKER 039C–039F). The first state change is the open sticky gate with DE still low. The first sampled pin change is DE reopening when R6 is rewritten unequal. |
| `0x6e49dbfaaa09912a` | 2026-11-26 | Type0 compares elapsed VSYNC lines against live R3h (SHAKER 02EA, French ACCC §14.2 p.133). The first difference is a pulse at C3h=15 wrapping with live R3h=5 where the previous loaded width expired. |
| `0xc16b325bacbe0cab` | 2026-11-26 | Combined CRTC0 03FA and 02DD fixes with CRTC1 03FC. The soak also samples the pending first adjustment row in CRTC1. |
| `0xb344ce3e9c337d99` | 2026-11-26 | Standalone CRTC1 deferred first adjustment row (SHAKER 03FC, French ACCC v1.11 §11.2.4 p.85) with the earlier soak projection. |
| `0xdbb46a42dd22fd14` | 2026-11-26 | Combined CRTC0 odd-R9 IVM entry parity (SHAKER 02DD) and ordinary R5 adjustment exit (SHAKER 03FA). |
| `0x7248cd9cba31b96b` | 2026-11-26 | CRTC0 ordinary adjustment entry ends when the R9 rollover produces C9=0 and R5 has been cleared (SHAKER 03FA). |
| `0x4ee3a069c36fb412` | 2026-11-26 | CRTC0 odd-R9 IVM entry seeds C9 parity from the current C4 and frame parity on ordinary rows (SHAKER 02DD, French ACCC §19.5.2 pp.206,208). |
| `0x8860eabba62aca89` | 2026-11-26 | Combined classic CRTC changes with the provisional nonterminal R7 write aperture at a raster carry. |
| `0x9e40e4b32dd115e2` | 2026-11-26 | Provisional CRTC1 R7 onset at a nonterminal raster carry (B26, AmSpirit B6C1 fifth-slot control) |
| `0xcd4b8f6d197f3dbd` | 2026-11-26 | Combined classic write aperture, CRTC1 S recovery and CRTC0 E6 held R4 rematch and scoped added-line handling. |
| `0xe9ea082db65fd953` | 2026-11-26 | Sample CRTC0's held R4 equality through adjustment rematch. The held comparator choice follows the AmSpirit E6C0 result pending an original CRTC0 probe. |
| `0xe95c30878c236016` | 2026-11-24 | CRTC0 dual C9/R9 and C9/R5 comparator in ordinary additional management: R4 restored to C4 re-enables the C9==R9 reset and C4 increment, R5 endpoint keeps priority; excluded on exact-R0 R9 owner lines and IVM/R8-toggle lines; a rematch landing on R6 captures ParityR6; bounded B26 candidate for SHAKER E6 0409 cases 27/28 (French ACCC 11.2.2 pp82-83, 13.2.4 p108) |
| `0xa3cb8ff67dfa5a42` | 2026-11-24 | CRTC0 R7=C4 write at C0=R0 of a row's last line blocked when the next row's C4 differs (French ACCC 16.4 and 16.4.1.1 p170; SHAKER 019D) |
| `0x6bb49f9262b5c886` | 2026-11-13 | CRTC1 active VSYNC width held at R0=0; bounded B26 candidate for original SHAKER AI/008D high samples, with C4/C9 still advancing (French ACCC 13.3 p115) |
| `0x974e5c048eb7f877` | 2026-11-06 | Current type 0 and type 1 behaviour |

Run `make -C sim soak SOAK_EXPECT=0xfc229b889c5013ce` to compare a change. If a behaviour preserving refactor changes the hash, investigate the difference. For an intentional behaviour change, add a new top row with the source or hardware basis and affected behaviour.
