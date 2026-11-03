# SHAKER 2.7 CRTC 1 sweep after the RFD fixes — 2026-11-03

## Position

The CRTC 1 walks of modules A, B and C now **complete**, after two script
workarounds and no RTL change beyond the [RFD fixes](rfd-frame-lifetime-2026-11-03.md).
Captures were compared against the Logon System real-CPC photographs. No
regression from the RFD work was found. Pages that remain different are listed
below with their classification. Module D still stops on input timing, and
module E has only seven pages.

## Setup

MiSTer `root@mister`, classic 6128, Plus off, CRTC 1, Full sync, French ROM.
RBF `Amstrad_20261103_07bac19.rbf` (same RTL as master `5fba168`) for the
first sweep, and `Amstrad_20261103_5fba168.rbf` for the later B-1, C-1 and
probe runs. SSM-driven `csl_runner.py` walks with `--no-follow-loads`.
Real-hardware references: `scrape_logon.py --arch cpc_CPC --crtc 1`, which
fetched 465 photographs. Private evidence: `local/investigations/shaker-sweep-2026-11-03/`
in the main clone (walks, `csl/` script copies, `ref/`, `sheets*/`, Gemini
triage reports `triage-gemini*.md`, bisect folders).

| Walk | Result |
|---|---|
| A-1 | complete, 127 captures |
| B-1 | stalls at `00E5` as before; with the `Z`→`W` copy, **complete, 139 captures** |
| C-1 | stalls at `02F1` as before; with the marker-typo copy, **complete, 80 captures** |
| D-1 | stalls at `0305` (known); a copy skipping test T stalls after `0311` |
| E-1 | complete, 7 captures |

Both script problems are described in the
[driver guide](mister-hardware-loop-driver.md#the-csl-runner): SHAKER reads
menu letters by physical key position, so the French swap loses `Z`; and C-1
waits for `02F1`/`02F2` where the module emits `03F1`/`03F2`. The D-1 stop
after `0311` leaves test 1's screen up while the next space and `2` go
unanswered. That matches the documented `0305` input-timing stop, where slower
input gets through. It is not a display defect.

## Photo comparison

Gemini 3.8 Flash classified 229 photo/capture pairs from the first sweep:
202 match, 10 phase, 17 different. The parent then checked the differences:

| Pages | Classification |
|---|---|
| `0037`, `008D`–`0091`, `023C`, `02CD` | **Known, not CRTC.** Printed PPI port B reads `5E/5F` on the photographed machine and `FE/FF` here. Bits 7 (cassette) and 5 (/EXP) differ; the VSYNC bit agrees (finding D7, 2026-09-10). |
| `00D5`, `00D6`, `00DD`, `00DE` (B1 interlace, C9=2/6) | **Pre-existing acquisition.** Near-uniform yellow captures on every RBF back to `925b5bb` (bisected) and in the October walk. |
| `02C9`, `02CB` (C, R5/R8 on last line) | **Pre-existing.** Black in every walk since September; the photographs show result text. Open. |
| `02D3` (C, R5 additional line in interlace) | **History-dependent.** A cold isolated run prints the correct `#4E40` on `46f8ab8`, `918237f` and `5fba168`. After the module's earlier tests, two walks printed `#5060` and `#0840` WRONG. Replay (below) traces it to state left by test E. |
| `02DA` (C, interlace VSYNC nightmare F) | **Pre-existing.** `#0050` WRONG, expected `#0060`. The capture is byte-identical to October's. |
| `009A` (A, R1 stories I) | Open: the photo shows a full RAM-snow screen, the capture is black with displaced red text. |

The second batch covered the 121 newly reachable B-1 and C-1 pages: 98
match, 20 phase, 3 different. The parent checked all three:

| Page | Classification |
|---|---|
| `019D` (B6, OUTI on R0) | **Known residual.** `#1F` WRONG versus the photo's `#1E`; the production-T80 XFAIL and its open write-visibility question are unchanged. |
| `01E7` (BO, 1-A/1-B identifier) | **Expected.** The photographed machine is a CRTC 1-B; the core models 1-A and deliberately omits RFD#10 (§11.6.2). |
| `0216` (BS, BE00 check) | **Documented variant.** R31 reads `#FF` here and `#7F` on the photographed machine. French ACCC §21.2.2 p.245 reports "127 ou 255" for this read; both are real-CPC values. All other values on the page match. |

## `02D3` and the blank E/P pages: replay diagnosis

An authentic-code replay of SHAKE27C on production T80, GA and CRTC reproduces
both walk values. Test E (`02C9`) ends in the §11.3.2 R5=0 deadlock (French
pp.86-87, author answer Q20): its last attempt writes R5=0 on the C0=R0 edge
where C5+1=R5 is decided, the model reads that as no release, and the type-1
engine stays in vertical adjustment. P and the menu do not clear it. S's R5=8
then ends adjustment wherever C5 has reached; the stuck frames are 312 lines,
so the result cycles with menu idle time (`#0840`, `#5060`, `#0440`, `#0640`).
S alone gives `#4E40`. The pre-today and current engines give identical
results, so the RFD changes are not involved.

E and P display blank because `vde` re-arms only on `frame_new`, which never
fires while stuck. Their photographs show text, so hardware re-opens the
display at the stuck C4 reset. A scratch `rtl/CRTC.v` change re-arming `vde`
there restores E/P display in the replay and passes crtc-test 238/238. It
needs a fail-first vector and a hardware RBF before integration. Whether the
R5 write on the C0=R0 decision edge should release the deadlock (which would
also fix `02D3` in the walk) needs the `02CA` photograph or an author answer.
A separate model mismatch is noted: E's "CPU TO NEW FRAME" line reads `#0060`
in the model and `#0080` on the photo.

Replay harness, extracted payload, disassembly and logs:
`local/investigations/shaker-sweep-2026-11-03/c02d3-replay/` (`build2.sh`,
`c_test`; `eng_old.v`, `eng_new.v`, `CRTC_vdefix.v`).
