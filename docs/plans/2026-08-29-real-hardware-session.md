# Real-hardware session — 2026-08-29

This is the durable evidence record for the 2026-08-29 MiSTer session. It keeps
observations separate from repository deductions and from fixes that have not
yet returned to hardware.

## Provenance

- Platform: real MiSTer hardware.
- Core build: described by the tester as the latest build available on
  2026-08-29. Exact RBF filename, Git SHA, MiSTer version, and file hashes were
  not recorded; do not assign these results to a specific commit until that is
  recovered.
- Classic CRTC: CRTC 1 was explicitly used for at least the classic comparison.
- Plus model, FDC menu setting, mounted DSK, expansion-ROM inventory, and saved
  status state were not recorded for every title.
- Comparison baseline: upstream core for the broad classic observation. Named
  SHAKER entries and Logon System reference photographs were not recorded.

## Plus results

| Title or operation | Hardware observation | Status after source investigation |
|---|---|---|
| Start in classic mode | AmstradDiag upper and lower ROM content appeared to load automatically. The OSD did not make the active expansion-ROM inventory or an unload action clear. | Confirmed UI/state visibility gap. `rom_map` is set-only during the session and the OSD has no explicit expansion-ROM inventory/unload action. |
| Switch classic → Plus; load Burnin' Rubber CPR | Crash; part of the AmstradDiag failsafe RAM test appeared in the border area. | Strongly explained by a confirmed MRER/RMR2 ownership defect: unlocked RMR2 is also consumed as MRER by the GA paths, allowing the fixed classic lower-ROM decode to answer `&0000` after relocation. Burnin' Rubber contains the matching relocation/copy/jump sequence. Hardware re-test required after the fix. |
| BASIC/System CPR boot | Copyright information, then immediate `read fail` or `disc missing`. | Normal high-window page 3 is Plus AMSDOS, not leaked classic AMSDOS. Real FDC transaction coverage is absent. Record Plus model, FDC setting, and mounted DSK on re-test. |
| Dick Tracy | Working pretty well. | Retain as a positive regression title. |
| Tintin | Working pretty well. | Retain as a positive regression title. |
| Mystical | Working pretty well. | Retain as a positive regression title. |
| Barbarian II | Working pretty well. | Retain as a positive regression title. |
| RoboCop | Sprites remain garbled; the P10 remediation produced no visible improvement. | Confirmed P10f in-flight fetch protection regression: a delayed stale fetch can overwrite CPU write-through data. Title causality remains to be re-tested. |
| Navy Seals | Nearly works; vertical hardware scrolling is very choppy; bullets leave garbled trails over the traversed background. | Confirmed SSCR vertical row-base/wrapped-RA defect matches the choppiness. Bullet trails need a title trace to distinguish sprite state, stale sprite fetch, and background fetch. |
| World of Sports, bike event | Same choppy vertical hardware scrolling. | Same SSCR wrap defect is a strong common cause. |
| Pang | Grey screen with blue border. | RMR2 writes are affected by the GA seam. Independent confirmed gaps also remain in PRI monitor-HSYNC timing and SSCR raw-bit semantics. No first-divergence trace yet. |
| Panza Kick Boxing | Grey screen with blue border. | Not fully explained by the ROM relocation path. PRI timing and first-adjustment-line split behavior remain candidates; no first-divergence trace yet. |
| CRTC3 demo | Crashes or flickers in a loop after Flowlib messages and emulator detection. | CRTC3 R8 interlace/IVM is stored but not implemented, providing a direct emulator discriminator. PRI timing is also inaccurate. Exact detection branch not yet traced. |
| Other CPRs | Many crash during boot. | Multi-causal. Re-test after the confirmed MRER/RMR2 seam fix before attributing residual failures. |

## Classic results

- On CRTC 1, the broad SHAKER Module A comparison showed few or no meaningful
  differences from upstream.
- This does not refute the recent accuracy fixes: most target Module B/C/D/E,
  rare adjustment/interlace states, type 0 only, or readback. The Module A
  entries actually run were not named.
- The current core still has no CRTC-1 R2.JIT sub-character implementation.
  DSC4 is therefore a high-value discriminator, not an expected pass today.
- Longshot's guidance, retained verbatim for the next accuracy session:

  > les chapitres 14.7.1 et 9.3.4.1 abordent les spécificités de la technique
  > R2.JIT (La mise à jour de R2 exactement sur la position C0==R2 retarde le
  > début de la zone noire de la Hsync) Sur un CRTC 1, il faut que la technique
  > RFD soit gérée correctement pour que ça fonctionne. (Autrement dit, R5>0
  > modifié sur C0==R0 et gestion correcte de la parité interlace en IVM avec R8)

  The checked ACCC v1.11 material places R2.JIT in section 14.6.1, not 14.7.1;
  section 9.3.4.1 remains relevant. Preserve Longshot's wording but verify the
  edition/section mismatch before implementing.
- `The Demo` still reports `disc missing`. Treat this as an FDC integration
  result, not CRTC evidence. A confirmed regression currently applies the Plus
  internal-FDC alias decoder to classic mode and must be corrected separately.

## Required re-test matrix

For every row, record RBF filename/SHA, Plus model or classic model/CRTC, FDC
setting, mounted DSK/CDT/CPR filenames and hashes, expansion-ROM inventory, and
whether settings were restored from saved status.

1. Burnin' Rubber, Navy Seals, Pang, Panza, RoboCop, and CRTC3 demo after the
   confirmed fixes, with Dick Tracy as the positive control.
2. BASIC/System CPR under each Plus model. For 6128+, repeat with a known-good
   mounted DSK and FDC enabled; for GX4000/464+, explicitly expect no FDC.
3. Classic `The Demo` with a trace of the first motor/status/data port.
4. Named SHAKER entries, not only Module A as a whole: confirm SHAKER's detected
   CRTC, then use the target map in `docs/accuracy/shaker-module-a-map.md`.
5. DSC4 on CRTC 1 after an R2.JIT fixture exists; keep RFD+IVM as a separate
   compound discriminator.
