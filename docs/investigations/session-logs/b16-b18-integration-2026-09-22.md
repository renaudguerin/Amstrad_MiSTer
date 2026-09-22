# B16 and B18 integration — 2026-09-22

## Source and review

The documentation/queue reconciliation is `07414d1`. B16 source `5b55705` was
rebased without patch changes to `1b4d45e` and merged as `b8a6181`. B18 source
`88204c8` was rebased without patch changes to `f337f2b` and merged as `37ccfc3`.
The two behavior streams remain separate commits. The combined implementation tip is
`37ccfc3ef8d2bb516391167015d34da38adffe1b`.

Both merges were clean. Integration changed shared documentation only; reviewed
RTL, tests and build files were unchanged. Existing source gates were verified in their
logs, and exact-SHA CI supplied the merged gate without a duplicate local suite.

- B16: Opus follow-up `20260922T001601Z-65220-f2d2` approved the repaired delta.
  Invalid CPR selection, stale status echoes and action-bit replay have failing-before-fix
  regressions. See [the implementation contract](../../plus/b16-load-model-2026-09-22.md).
- B18: Opus follow-up `20260922T002141Z-71132-650f` returned CLEAR after the PSG-width
  correction. Six model/CRTC cases and six host publication cases are covered. See
  [the fixture and limits](../../b18-sna-save.md#classic-capture-and-round-trip-fixture-2026-09-22).
- Existing review-debt rows remain open; the new fixture is not retrospective review
  clearance for earlier save-path edits.

## CI and delivered artifact

[Build run 35673603710](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35673603710)
ran on the combined implementation SHA above. Successful jobs: `synthesis-policy`,
`simulation` (40 selected benches plus lint, index 47 rows), `production-t80`, `route`,
`synthesis (full)` and `required-gate`. `synthesis-local` was skipped as intended by
hosted routing. The production-T80 log confirms all six B18 capture/restore cases and
all six host torn-read/retry cases. No duplicate synthesis dispatch was made.

Artifact `Amstrad-build-231-1-full` (ID `10671959686`), Quartus 17.0.2,
`build_mode=clean_full`:

- RBF: `output_files/Amstrad_20260922_37ccfc3.rbf` in the integration checkout.
- SHA-256: `a3c9fb8eba80a64df2928bc55d2cca7d68a9a302121b6678dad07ded04a3780c`;
  downloaded and copied bytes match.
- Reports: `output_files/reports-37ccfc3/` (ignored).
- Logic: 23,675 / 41,910 ALMs (56%); 28,380 registers; 102 RAM blocks;
  701,596 block-memory bits; 35 DSP blocks.
- Worst setup +0.553 ns; worst hold +0.241 ns; all reported TNS zero.

The later documentation-only handoff commit reuses this artifact; it is not a new build.
Neither the RBF nor reports are committed.

## Remaining acceptance

No device run was performed with this RBF. B16 still needs Main/OSD echo and real
CPR/SNA boot/restore checks; classic-header-to-Off policy is unchanged. B18's fixture
uses external byte memory and fixed FDC/video data, covers uncompressed classic restores,
and does not restore SNA-unrepresented state. Physical 464/664 saving, Plus save and
user-facing SD transport remain separate work. The classic soak hash remains
`0xb1cb70da95c2e44f`; neither branch changes classic CRTC behavior.

Read-only preflight found MiSTer at MENU with a K400 keyboard and virtual input, but
no physical controller enumerated. AmSpirit lite 1.15.1 responded locally. This establishes
B17 keyboard-development prerequisites, not recording/replay acceptance. Sonic work awaits
the user's new ASIC sources. CSL 1.5 / SSM 1.2 work proceeds on its own branch and is not
included in this artifact.
