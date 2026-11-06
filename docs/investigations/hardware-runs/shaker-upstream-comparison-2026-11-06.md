# Selected official-core SHAKER comparison

Host-driven SHAKER controls work on the official core without CSL/SSM support
in the FPGA. Four serial runs compare the official 3 June 2026 RBF against
`fdf348c`, using the same current 2.7 disk, existing French firmware setup,
CRTC selection and private fixed-wait keyboard scripts on one MiSTer.

| Control | Official core | fdf348c | Conclusion |
|---|---|---|---|
| CRTC0 C5/029E | Even-only rows; top/bottom wrap | Clear 6E/6O/OEOEOE | The scoped parity/presentation correction is visible |
| CRTC1 B6/019D | 13/14/26/26/1F/1F; fifth R7 check wrong | Byte-identical complete PNG | This residual is shared with upstream |
| CRTC1 B9/01AD | Cropped/duplicated rows | Cropped upper page | Fixed-wait capture cannot establish complete scalar acceptance |
| CRTC1 B9/01AE | Scrambled multiple epochs, 546 rows | Clear 273-row page; three 3F rows 2740, MID 4E40 | Candidate presentation improves; the unreadable upstream page is not a numeric oracle |

Official release source is commit
`6c2c39b6607fc870f5b60ca893c40fb9c873c81f` in
[MiSTer-devel/Amstrad_MiSTer](https://github.com/MiSTer-devel/Amstrad_MiSTer/commit/6c2c39b6607fc870f5b60ca893c40fb9c873c81f).
The downloaded 2,818,228-byte RBF has SHA-256
`04080cb7ad8ca94400871ae5b2b13e2f36a664130687a3cf1e83285480e09db3`;
its separate device copy was hash-verified before loading. This identifies a
release artifact, without independent synthesis attestation. Candidate RBF
identity matches the [milestone archive](shaker-milestone-archive-2026-11-06.md).

All four manifests succeed, all eight PNG hashes/dimensions match, CFG
restoration succeeds after each run, and root viewed every image at full
resolution. CRTC/model settings are evidenced by applied CFG, not the omitted
OSD. The unchanged firmware setup was shared; no independent ROM dump was
collected. Upstream has no applied-filter SSM observation. Fixed waits and
asynchronous scaler grabs cannot guarantee an identical opcode or raster
epoch. Original-CPC photographs remain the accuracy authority.

This covers selected CRTC0/1 controls only, with no Plus/CRTC3 or full-module
claim. The candidate was loaded last and remains active. Private scripts,
source/artifact identities, manifests, original images and parent acceptance
are under `local/overnight-20261103/resumed-20261104/`
`milestone-release-20261106/upstream/`.
