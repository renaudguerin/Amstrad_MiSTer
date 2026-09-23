# B16 SNA model selection and Main readback

SNA v3 headers 5 (464+) and 6 (GX4000), plus a header-4 control (6128+),
select the expected model on the MiSTer from saved Plus Off. After each load,
Main's **System > Save settings** writes the expected live model bits into
`Amstrad.CFG`. This observes the normal model publication/echo path; it does
not visually verify the OSD label or establish a functional snapshot restore.

## Build and input identity

- Task base: `57a90bc86b1ff05d2eacf18cd303957152d7bee4`.
- Actual RBF: `Amstrad_20260923_cb60ff8.rbf`, SHA-256
  `29f69fc4ab58c7072f922661da1b1bec9c1df80bc1e245d0ab73d5b22d2d7310`.
  The exact-master build was still synthesizing during this run. The command
  `git log --oneline cb60ff8..57a90bc -- rtl Amstrad.sv files.qip` returned no
  changes; this remains a test of the named ancestor artifact, not a test of
  newly synthesized master bytes.
- On-disk Main SHA-256:
  `9f6e5a237c36be6404ab4823d804821491db4bf125827f84aca2a1ca31f0a8a6`.
  This identifies the stored binary, not independently the running process.
- Original media:
  `local/test_media/defects/arn5diag/snapshot_20260913_171245_OS_PLUS_FR.sna`,
  1,661,850 bytes, v3/header 4, SHA-256
  `01b385147e481ae7926d8ff327aa3213a531350ba262f4b0c97f17a3eb3ff487`.
  Header-4 control is byte-identical. Copies for headers 5/6 differ only at
  byte `0x6D` (decimal 109), with identical lengths. Original media was not edited.

## Procedure and observations

Exclusive device ownership was confirmed with the coordinator after the Eerie
session released it. Each case started with the original 16-byte CFG and Plus
bits `[34:33]` cleared to Off; all other settings were retained. An MGL loaded
the pinned RBF and then the SNA through F6, with no cartridge or model input.
After an eight-second wait, the bounded `input_replay.py` schedule sent F12,
Right, Up five times, Enter, F12 (500 ms spacing, 120 ms holds), selecting
System > Save settings from a freshly loaded core. The CFG was then downloaded.

Main's [Save settings handler](https://github.com/MiSTer-devel/Main_MiSTer/blob/aa271e41ebbf616903f9e0216b0900aead5bfce1/menu.cpp)
calls `user_io_status_save`; the
[status implementation](https://github.com/MiSTer-devel/Main_MiSTer/blob/aa271e41ebbf616903f9e0216b0900aead5bfce1/user_io.cpp)
saves `cur_status`, populated by `UIO_GET_STATUS` and echoed through
`UIO_SET_STATUS2`. This source explains the readback method; the distinct saved
bytes below are the device observation. Unlike inspecting an unchanged CFG,
explicitly saving after the load samples Main's current selection.

| Header | Expected / saved `[34:33]` | Saved CFG (hex) | SNA SHA-256 |
| --- | --- | --- | --- |
| 4 | 2 / 2: 6128+ | `00004000040000000000000000000000` | `01b385147e481ae7926d8ff327aa3213a531350ba262f4b0c97f17a3eb3ff487` |
| 5 | 3 / 3: 464+ | `00004000060000000000000000000000` | `d105ac09cbcef23a1954897f6603dbc65a34716fa2572a2a314b55c4a0bf6eed` |
| 6 | 1 / 1: GX4000 | `00004000020000000000000000000000` | `78f047fb69b7b7da18b8a0898838dc3a40de5401f4253d7adb5d6740f3fd15a8` |

One native capture per case was inspected. All show the same vertical coloured
bars, 768 × 273, SHA-256
`35e6edfdd5c01d71c078b51dd92b49a847bd4b1ec1172d516d11b94ef464d52d`.
The cartridge required by the snapshot was deliberately omitted: these images
are not usable-restore evidence and cannot distinguish the models. The CFG
readback, rather than pixel equality, establishes the selected values.

## Acceptance boundary and restoration

Accepted by this probe: headers 4/5/6 from Off select the expected model and
reach Main's live status without manual model selection or a second core load.
This is one normal round trip per header, not a stress test of delayed echoes.

Still unverified here: visually rendered OSD labels, functional cartridge-backed
restores for headers 5/6, manual OSD file loading, and preservation of explicit
Plus selections during CPR loading. Earlier CPR-from-Off boot evidence remains
valid, but this SNA probe does not extend its live-model readback coverage.
No AmSpirit comparison was needed because no emulated-behavior claim is made.
No RTL, simulation model, tests, or build inputs changed.

The original CFG was restored in `finally`, downloaded, and compared byte for
byte. A final device read after returning to MENU again reported SHA-256
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
All task-created device SNA/MGL/script files were removed; named screenshots
remain. The device reported `MENU` and was released to the coordinator.

Private evidence is retained in this task checkout under the gitignored
`docs/screenshots/b16-model-2026-09-23/`: probe script, replay schedule, SNA
copies, original/applied/saved/restored CFGs, MGLs, captures, manifest and command
log. The directory was moved there after the run; command logs retain its
original `docs/specs/b16-model-2026-09-23/` location. Preserve this evidence before
worktree cleanup. Only this report and the B16 status updates are committed.
