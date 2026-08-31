# Credits

This core is a fork of the MiSTer Amstrad CPC core, adding classic CRTC accuracy work for
types 0 (HD6845S) and 1 (UM6845R) and Amstrad Plus / GX4000 ASIC support.

This file is the durable home for attribution. Individual source files carry their own
headers, but headers move and get rewritten during refactoring; this page is meant to survive
that. Anyone redistributing this core, in source or as a built RBF, should carry it forward.

## Amstrad CPC CRTC Compendium

The classic CRTC accuracy work in this project is derived from the **Amstrad CPC CRTC
Compendium** by **Serge Querne (Longshot / Logon System)**, <https://shaker.logonsystem.eu>,
licensed CC BY-NC-ND 4.0.

    // Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
    // (CC BY-NC-ND).

The Compendium's attribution directive asks for that mention in the source headers of CRTC
emulation modules and in the visible credits of any product built from them. This project
honours both. The credit line is carried in `rtl/CRTC.v`, `rtl/crtc_type0_engine.v`,
`rtl/crtc_type1_engine.v` and `sim/sim_main.cpp`, individual rules cite their Compendium
section at the point of implementation, and any new module implementing Compendium behaviour
must carry it too.

**If you redistribute or repackage this core, keep this section.** The Compendium represents
decades of original hardware research, without which the accuracy work here would not exist,
and its author's central concern is that credit is preserved rather than eroded as code passes
through later hands.

The Compendium PDFs themselves are **not** distributed with this repository. They are the
author's work, obtainable from the link above.

## Reference documentation

- **Amstrad CPC CRTC Compendium**, Serge Querne (Longshot / Logon System) — the authority for
  classic CRTC behaviour in this project.
- **SHAKER** test suite and its real-hardware reference photographs, also by Logon System,
  <https://shaker.logonsystem.eu> — the accuracy oracle: results are judged by comparison
  against those photographs, not against other emulators.
- **CPCWiki**, <https://www.cpcwiki.eu> — Gate Array, ASIC, disk format and connector
  references.
- Amstrad's own **Arnold V15** internal documentation, for the Plus ASIC.

## Upstream core

- **MiSTer Amstrad CPC core**, <https://github.com/MiSTer-devel/Amstrad_MiSTer> — the base this
  project forks.
- **Sorgelig** (Alexey Melnikov), original author of the MiSTer and MiST Amstrad cores and
  creator of the MiSTer project itself.
- **CoreAmstrad / FPGAmstrad**, Renaud Hélias — ancestor of parts of the video path;
  `rtl/crt_filter.v` descends from that lineage by way of Sorgelig's 2018 rework, which in turn
  references the JEMU Gate Array implementation.

## Components

| Component | Attribution |
|---|---|
| `rtl/GA40010/` Gate Array | Gyorgy Szombathelyi, based on `40010-simplified_V03.pdf` by Gerald. Netlist-derived from a decapped chip; see <https://github.com/codedchip/AMSGateArray> for that lineage. |
| `rtl/u765/` FDC | Gyorgy Szombathelyi |
| `rtl/tzxplayer.vhd` | Gyorgy Szombathelyi, structure based on the c1530 tap player by darfpga |
| `rtl/T80/` Z80 | The T80 core, with undocumented-feature work by TobiFlex and Sean Riddle among others, and later changes by Sorgelig |
| `rtl/YM2149.sv` PSG | MikeJ (2005), with later changes by Sorgelig |
| `rtl/sdram.v` | Sorgelig, based on the SDRAM module by Till Harbaum |
| `rtl/playcity/` | PlayCity expansion by TotO, VHDL implementation by Slingshot, Verilog version by Sorgelig. <https://www.cpcwiki.eu/index.php/PlayCity> |

Individual file headers remain authoritative where they are more specific than this table.

## Licensing notes

The core is distributed under the GNU General Public License. Preserving the copyright and
attribution notices in the source is a requirement of that licence, not merely a courtesy.

`rtl/YM2149.sv` carries a BSD-style clause requiring that its copyright notice be reproduced
**in synthesized form** as well as in source. Anyone distributing a built RBF should treat that
as an obligation attaching to the binary.
