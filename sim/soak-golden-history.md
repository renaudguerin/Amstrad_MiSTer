# CRTC soak golden hash

The fixed seed randomized CRTC soak uses this current expected hash:

| Hash | Set on | Scope |
|---|---|---|
| `0x7248cd9cba31b96b` | 2026-11-26 | CRTC0 ordinary adjustment entry ends when the R9 rollover produces C9=0 and R5 has been cleared (SHAKER 03FA). |
| `0x8860eabba62aca89` | 2026-11-26 | Combined classic CRTC changes with the provisional nonterminal R7 write aperture at a raster carry. |
| `0x9e40e4b32dd115e2` | 2026-11-26 | Provisional CRTC1 R7 onset at a nonterminal raster carry (B26, AmSpirit B6C1 fifth-slot control) |
| `0xcd4b8f6d197f3dbd` | 2026-11-26 | Combined classic write aperture, CRTC1 S recovery and CRTC0 E6 held R4 rematch and scoped added-line handling. |
| `0xe9ea082db65fd953` | 2026-11-26 | Sample CRTC0's held R4 equality through adjustment rematch. The held comparator choice follows the AmSpirit E6C0 result pending an original CRTC0 probe. |
| `0xe95c30878c236016` | 2026-11-24 | CRTC0 dual C9/R9 and C9/R5 comparator in ordinary additional management: R4 restored to C4 re-enables the C9==R9 reset and C4 increment, R5 endpoint keeps priority; excluded on exact-R0 R9 owner lines and IVM/R8-toggle lines; a rematch landing on R6 captures ParityR6; bounded B26 candidate for SHAKER E6 0409 cases 27/28 (French ACCC 11.2.2 pp82-83, 13.2.4 p108) |
| `0xa3cb8ff67dfa5a42` | 2026-11-24 | CRTC0 R7=C4 write at C0=R0 of a row's last line blocked when the next row's C4 differs (French ACCC 16.4 and 16.4.1.1 p170; SHAKER 019D) |
| `0x6bb49f9262b5c886` | 2026-11-13 | CRTC1 active VSYNC width held at R0=0; bounded B26 candidate for original SHAKER AI/008D high samples, with C4/C9 still advancing (French ACCC 13.3 p115) |
| `0x974e5c048eb7f877` | 2026-11-06 | Current type 0 and type 1 behaviour |

Run `make -C sim soak SOAK_EXPECT=0x7248cd9cba31b96b` to compare a change. If a behaviour preserving refactor changes the hash, investigate the difference. For an intentional behaviour change, add a new top row with the source or hardware basis and affected behaviour.
