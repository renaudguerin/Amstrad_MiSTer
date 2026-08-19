# Current implementation status

This is the handoff for the next development and hardware-test session. It describes the
state of `codex/exploratory-gx4000-plus-plan` on 2026-08-19. The detailed behavioral rules
remain in `accuracy/`; the long-term ordering remains in `implementation-roadmap.md`.

## Hardware-test milestone

`1a1233f` is the newest hardware-test milestone. GitHub Actions run `31661330994` passed the
complete Verilator gate, Quartus 17.0.2 compilation, fitter, TimeQuest, RBF packaging, and
artifact upload. It adds the independently reviewed C0>=2 F12 arbitration slice to the
earlier Dandanator/SDRAM milestone. The follow-on work after `1a1233f` completes the
C0=0/C0=1/short-R0 counter paths in simulation and is not part of this RBF.

Three bisectable CI builds have also been downloaded locally under the ignored
`output_files/hardware-milestones/` directory:

| Milestone | Commit | RBF SHA-256 |
|---|---|---|
| F2 status readback | `9c16729` | `40992b8e41ead9a9441734aedd171bd0e0942412fbadb089d62e26e4d3e8ba0c` |
| F3 complete, before F5 | `9956d83` | `9634938072dec7a9c82676a3a5b7192ef6927df50e77d164b84963d0d6c554d2` |
| F5 plus tied-off SDRAM foundation | `4ffa853` | `67efe7c7d07f49b31edb6dfca0c19ccf99237bd62df5eefdb0c780a86c95f0a9` |
| Final milestone with Dandanator isolation | `ba5b629` | `7ede21c7449868764f576c114f1697ffd5e6ce4a9b98a38679861d2d52dd3249` |
| F12 C0>=2 arbitration | `1a1233f` | `fd9705732ae20cb45f1807d4c980b893e974392c3b8f48bdb69ff57794f93319` |

The `1a1233f` fitter used 14,947 / 41,910 ALMs (36%), 685,217 block-memory bits (12%),
and 3 / 6 PLLs. Worst setup and hold slacks were positive at +0.541 ns and +0.192 ns
respectively.

Hardware testing reported on 2026-08-19 found no regression against the stock core, but
also no CRTC-0 compatibility improvement in the SHAKER Module A tests that were run. This
result applies to `1a1233f`, not the later deterministic-complete F12 commit `da79915`.
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
- The current local gate reports 46 required CRTC passes, no expected failures, and no
  failures. The Plus leaf and SDRAM integration suites are also green.

The next classic checkpoint is F4. Add `t07` equality/overflow and `t08` CRTC-ID boundary
vectors, including the tightened type-0 RLAL guards, before removing the zero-value
shortcuts. F4 then unblocks F8, the remaining F9 worked-example coverage, and F7 in that
order. F6
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
- Dandanator uploads are bounded below the Plus cartridge reservation: bank 3
  `0x000000..0x07ffff` remains Dandanator, while `0x080000..0x0fffff` is reserved for Plus.
- The CPR parser, Plus MMU/reset mapping, `/EXP` sampling, firmware/cartridge boot path,
  ASIC register page, CRTC3/video, palette, interrupts, sprites, split/scroll, and DMA are
  not implemented.

The next Plus milestone is P0 cartridge boot. Keep it stacked on the accepted P-1 contract:
connect the existing memory service, add a bounded RIFF/CPR parser, define `/EXP` for 464+
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
- The complete deterministic F12/t16 counter-arbitration milestone is present in RTL and
  the executable harness. Its C0>=2 predecessor has a synthesized CI build; this extended
  C0=0/C0=1/short-R0 slice is not yet synthesized or hardware-tested.

## Next-session order

1. Synthesize the current `da79915` F12 state and rerun the same SHAKER Module A selection,
   recording individual subtests so the result is directly comparable with `1a1233f`.
2. Map each persistent SHAKER difference to an implemented finding or a named gap; add a
   deterministic regression before repairing any newly understood behavior.
3. Classic: add the F4 `t07`/`t08` vector-only checkpoint, then implement equality/overflow
   without weakening the F12 arbitration or the tightened type-0 RLAL guards.
4. Plus: in a separate stack, implement P0 MMU/parser/boot integration against the existing
   memory-service and SDRAM contracts.
5. Update this file when either stream reaches its next hardware-testable checkpoint.
