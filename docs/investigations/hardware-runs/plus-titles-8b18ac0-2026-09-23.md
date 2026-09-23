# Plus title retest on `8b18ac0` — 2026-09-23

User-run manual session on the integrated build. This records the user's
direct hardware observations; source review, simulation, and full
title/subsystem acceptance remain separate.

## Build and configuration

- Commit: `8b18ac0` ("Integrate general/tape-sdram-bank"), in `HEAD`
  (`5991b59`).
- RBF: `output_files/Amstrad_20260922_8b18ac0.rbf`, local SHA-256
  `26185f485ac72a6be7e3ee17f9fc2e57a2e09c595ba8061d53a9aca90c960da2`.
  CI run `35797819805` (full synthesis, success); minimum setup slack
  `+0.215 ns` as recorded in the
  [device record](device-acceptance-cdcb3c3-2026-09-22.md). The device
  copy was not independently hashed for this run.
- RTL context: `8b18ac0` contains the B20-7 DMA terminal-PAUSE rule
  (`64702ac`), the cartridge READY-rate fix (`03f4724`), and the tape
  image relocation to bank 3 (`859fd24`). No sprite-engine RTL change
  versus `64702ac`.
- Unrecorded: MiSTer version, Plus model selection, CPR/DSK/CDT hashes,
  reset/load order, captures. Comparison source is user observation only.

## Reported hardware results

| Scope / symptom | User observation on `8b18ac0` | Verdict |
|---|---|---|
| Sonic GX corruption | Confirmed fixed | Consistent with the B20-7 acceptance on `64702ac` ([record](../../sonic/b20-7-dma-pause-acceptance-2026-09-22.md)); now observed on the integrated build. Progression depth (title only vs attract/gameplay) was not stated — do not infer it. |
| Navy Seals / World of Sports left-edge sprite-line flicker | Appears fixed | User-reported improvement on `8b18ac0`. No assigned RTL cause in this build (no sprite change since `64702ac`); closure stays pending a repeatable capture. The old black-screen report stays unreproduced with no assigned cause. |
| Switchblade / Eerie Forest | Still not loading | Reproduced on `8b18ac0`; remains open and evidence-gated. Failure shape (black vs grey/blue, first divergence) was not stated. |

## Impact on open tracking

- Promotes Sonic title corruption from "fixed on experimental `64702ac`" to
  "confirmed on integrated `8b18ac0`" for the title symptom only. General
  PAUSE 0/1, PPR and REPEAT boundaries stay open.
- Navy Seals / World of Sports flicker moves from "remains" to "reported
  fixed, closure pending" — checklist boxes stay unchecked until a
  repeatable capture+configuration pass exists.
- Switchblade / Eerie Forest stay in the evidence-gated crash family:
  next step is one reproducible case with exact media hash, model/config,
  reset sequence, and first visible failure (or first trace divergence),
  not another RTL guess.
