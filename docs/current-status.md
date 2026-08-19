# Current implementation status

This is the handoff for the next development and hardware-test session. It describes the
state of `codex/exploratory-gx4000-plus-plan` on 2026-08-19. The detailed behavioral rules
remain in `accuracy/`; the long-term ordering remains in `implementation-roadmap.md`.

## Hardware-test milestone

`1a1233f` is the newest hardware-tested milestone. GitHub Actions run `31661330994` passed the
complete Verilator gate, Quartus 17.0.2 compilation, fitter, TimeQuest, RBF packaging, and
artifact upload. It adds the independently reviewed C0>=2 F12 arbitration slice to the
earlier Dandanator/SDRAM milestone. The follow-on work after `1a1233f` completes the
C0=0/C0=1/short-R0 counter paths in simulation and is not part of this RBF.

The retained CI builds have been downloaded locally under the ignored
`output_files/hardware-milestones/` directory:

| Milestone | Commit | RBF SHA-256 |
|---|---|---|
| F2 status readback | `9c16729` | `40992b8e41ead9a9441734aedd171bd0e0942412fbadb089d62e26e4d3e8ba0c` |
| F3 complete, before F5 | `9956d83` | `9634938072dec7a9c82676a3a5b7192ef6927df50e77d164b84963d0d6c554d2` |
| F5 plus tied-off SDRAM foundation | `4ffa853` | `67efe7c7d07f49b31edb6dfca0c19ccf99237bd62df5eefdb0c780a86c95f0a9` |
| Final milestone with Dandanator isolation | `ba5b629` | `7ede21c7449868764f576c114f1697ffd5e6ce4a9b98a38679861d2d52dd3249` |
| F12 C0>=2 arbitration | `1a1233f` | `fd9705732ae20cb45f1807d4c980b893e974392c3b8f48bdb69ff57794f93319` |
| F12 complete | `365c132` | `f44e16cc8c815a5d34c4a807feadbb54a25d358175fb4d542dfbcfddbd20f231` |
| F4 complete plus CPR parser | `5ddddef` | `e1ba1728435f33fc4fe8e1886b0b7b4021f12a4dff861767440b9e2b60a65ff6` |

The `1a1233f` fitter used 14,947 / 41,910 ALMs (36%), 685,217 block-memory bits (12%),
and 3 / 6 PLLs. Worst setup and hold slacks were positive at +0.541 ns and +0.192 ns.
The later `365c132` F12-complete build also passed synthesis, with worst setup and hold
slacks of +0.472 ns and +0.253 ns respectively; it has not yet been hardware-tested.
GitHub Actions run `32251491936` synthesized the F4-complete/CPR-parser state at `5ddddef`.
Its fitter used 14,899 / 41,910 ALMs (36%), 685,217 block-memory bits (12%), and 3 / 6
PLLs. Worst setup and hold slacks were +0.606 ns and +0.254 ns. TimeQuest still reports
the repository's existing unconstrained external I/O paths, so these positive internal
slacks are not full timing closure. This RBF is retained under
`output_files/hardware-milestones/f4-plus-cpr-5ddddef/` and is not hardware-tested yet.

Hardware testing reported on 2026-08-19 found no regression against the stock core, but
also no CRTC-0 compatibility improvement in the SHAKER Module A tests that were run. This
result applies to `1a1233f`, not the later deterministic-complete F12/F4 implementation.
Record the individual Module A subtests on the next pass so each failure can be mapped to a
named finding or an explicit coverage gap; do not infer hardware accuracy from the green
counter-level simulation gate alone.

The build is suitable for classic CPC regression testing. It contains the F1, F2, F3, and
main F5 CRTC accuracy work. Plus support is not bootable yet: the `Plus model` menu and
tested leaf modules are foundations only, and the production cartridge client remains tied
off. Selecting a Plus model is therefore not a meaningful hardware test at this milestone.

For a first MiSTer pass:

1. Keep `Plus model = Off`. Boot each classic model that you normally use and test both
   supported CRTC selections.
2. Confirm BASIC boot, keyboard/joystick, reset, video stability, tape if available, and
   `cat` plus a program load from a known-good disk.
3. Exercise SHAKER's status/readback and dynamic VSYNC tests for F2/F3.
4. Exercise its type-0 R0=0/stall tests for F5. Watch sync stability and Gate Array
   interrupt behavior as well as the diagnostic result.
5. Load a normal 512 KiB Dandanator image. The build rejects bytes at `0x080000` and
   above so malformed uploads cannot overwrite the reserved Plus cartridge region.
6. Record the RBF commit, MiSTer version, classic model, CRTC type, diagnostic/image name,
   and observed result. If a regression appears, compare the F2, pre-F5, and final milestone
   RBFs retained from CI before narrowing it further.

## Completed classic accuracy work

- A deterministic Verilator harness and GitHub simulation gate now cover the CRTC at pin
  level. Unexpected passes of named divergences fail the suite.
- F1 register readback is protected across R0-R31 for both supported CRTC types.
- F2 implements type-1 status bit 5 sampling at the required C0=R0 point and excludes the
  dynamic R6=0 false case.
- F3 implements model-specific live R7/VSYNC timing, including blocked type-0 early writes,
  partial-line duration, interlace boundaries, live type changes, and snapshot loading.
- F5 implements the type-0 R0=0 freeze, R2-dependent HSYNC behavior, live entry and
  recovery, MA/RA behavior, type switching, odd-field VSYNC freeze, and the single C4
  increment when C9 already equals R9 at freeze entry.
- ACCC v1.10 F12 counter arbitration is deterministic-complete for the documented entry
  paths. `t16a`-`t16s` cover R5 arbitration at C0=2, the R4/R9 last-line write windows,
  same-edge C0=0 writes, the C0=1/R5=0 equality-break route, exact-C0=R0 switching,
  `R0=0` and `R0=1` default adjustment, active-adjustment freeze, exact short-line latch
  consumption, bus phases, completion, and retained-state lifecycle. Exact sub-character MA/DE/VSYNC behavior still requires hardware or SHAKER
  traces; the deterministic assertions use C4/C9/RA and adjustment state.
- F4 removes the non-equality C9/C4 zero-limit shortcuts for both CRTC types and preserves
  type-0's live-versus-latched Last Line/RLAL behavior. Live R9 writes now also feed the
  independent VMA capture comparison, while active type-0 adjustment reuses C9 against
  R5 rather than R9. The `t07` counter and `t08` identification vectors now pass except
  for the two explicitly deferred type-1 adjustment-identification cases.
- The current local gate reports 71 required CRTC passes, two expected F8 divergences, no
  unexpected passes, and no failures. The Plus leaf and SDRAM integration suites are also
  green.

The next classic checkpoint is F8, completing type-1 R7 identification during vertical
adjustment. Then take the remaining F9 worked-example coverage and F7 in that order. F6
remains deferred because the documented
half-character border byte cannot be represented exactly by the current character-granular
CRTC-to-Gate-Array interface. F10 remains the last, separately fixture-gated project.

## Completed Plus foundations

- A separate default-off `Plus model` selector decodes GX4000, 6128+, and 464+ capabilities
  without reinterpreting the classic model field or selecting Plus hardware.
- The ASIC lock/unlock state machine is implemented and exhaustively unit-tested as a leaf.
- An atomic 512 KiB cartridge memory service is implemented and tested for clear, load,
  commit, abort/detach/reset, invalid addresses, and CPU reads.
- `sdram.v` now has a held cartridge request/acknowledge client with tested byte lanes,
  addressing, arbitration, back-to-back transfers, classic main/tape writes, and refresh
  fairness. The top-level ties it inactive until P0 integration.
- A real service-to-real-SDRAM simulation proves exact clear/load transaction counts,
  publication, and CPU readback without duplicate held requests.
- A bounded, streaming RIFF/CPR parser now validates the `RIFF`/`AMS!` envelope, accepts
  ordered `cbNN` cartridge-bank chunks, handles RIFF padding, streams payload bytes into
  the atomic cartridge service, and fails closed on malformed or aborted downloads.
- Dandanator uploads are bounded below the Plus cartridge reservation: bank 3
  `0x000000..0x07ffff` remains Dandanator, while `0x080000..0x0fffff` is reserved for Plus.
- Top-level parser/service wiring, the Plus MMU/reset mapping, `/EXP` sampling,
  firmware/cartridge boot path, ASIC register page, CRTC3/video, palette, interrupts,
  sprites, split/scroll, and DMA are not implemented.

The next Plus milestone is P0 cartridge boot. Keep it stacked on the accepted P-1 contract:
connect the parser to the existing memory service and download path, define `/EXP` for 464+
and 6128+ reset-page selection, and implement the Plus MMU/boot mux. Do not start Plus video
by extending `ga40010`; the planned path is a parallel behavioral CRTC3/ASIC video module.

## Build and tooling state

- `.github/workflows/build.yml` runs local-style Verilator tests/lint before a pinned Quartus
  17.0.2 synthesis job and uploads the RBF plus fitter/timing reports.
- GitHub currently warns that `actions/checkout@v4` targets deprecated Node 20 and is being
  forced onto Node 24. It does not fail the build; update the action in a separate CI-only
  maintenance commit rather than mixing it into an RTL milestone.
- `ansible/` provisions the Debian 13 arm64 UTM guest (reached as `quartus-vm.local` over
  mDNS), restores the build user's supplementary groups, mounts Rosetta, registers amd64
  binfmt, and validates
  a real amd64 binary. It also creates a private installer staging directory and provides a
  read-only checksum preflight for the exact Altera 17.0.2 payloads. Quartus itself still
  requires a human download and interactive EULA step; then run `ansible/post-install.yml`
  and `ansible/validate.yml -e quartus_required=true`.
- ACCC v1.10 is now the primary documentation baseline. The checked-in digests and
  `accuracy/accc-1.10-differences.md` capture its rules and the edition delta; consult the
  full PDF only when a page is specifically flagged for re-extraction.
- The local `docs/ACCC1.10-EN.pdf` is user-owned source material and must remain outside
  commits. If v1.9 is retained locally for edition-delta or historical-citation checks, it
  must likewise stay untracked.
- The local example cartridge `docs/plus/references/cartridges/crtc3_v2fix.cpr` is likewise
  deliberately untracked. Use it as a real RIFF/CPR parsing fixture when P0 starts; do not
  make it a build dependency or redistribute it from this repository.
- The complete deterministic F12/F4 counter milestone and CPR parser have a synthesized CI
  build at `5ddddef`, but have not yet been hardware-tested.

## Next-session order

1. Quartus VM post-install is a future-session task and must only run after explicit user
   authorization. Until then, keep synthesis on GitHub Actions. When authorized, run
   the four-command post-install sequence in `ansible/README.md` from the `ansible/`
   directory: check, apply, repeat the check, then validate with
   `quartus_required=true`.
2. Hardware-test `Amstrad_20260819_5ddddef.rbf` with Plus model disabled, then rerun the same
   SHAKER Module A selection. Record individual subtests so the result is directly
   comparable with `1a1233f`.
3. Map each persistent SHAKER difference to an implemented finding or a named gap; add a
   deterministic regression before repairing any newly understood behavior.
4. Classic: implement F8 type-1 R7 identification during vertical adjustment, leaving the
   current equality/overflow and F12 arbitration guards intact.
5. Plus: in a separate stack, wire the CPR parser into P0 MMU/boot integration against the
   existing memory-service and SDRAM contracts.
6. Update this file when either stream reaches its next hardware-testable checkpoint.
