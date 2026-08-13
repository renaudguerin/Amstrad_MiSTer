# Current implementation status

This is the handoff for the next development and hardware-test session. It describes the
state of `codex/exploratory-gx4000-plus-plan` on 2026-08-13. The detailed behavioral rules
remain in `accuracy/`; the long-term ordering remains in `implementation-roadmap.md`.

## Hardware-test milestone

`ba5b629` is the hardware-test milestone. GitHub Actions run `31637450102` passed the
complete Verilator gate, Quartus 17.0.2 compilation, fitter, TimeQuest, RBF packaging, and
artifact upload. It includes the independently reviewed 512 KiB Dandanator loader bound on
top of the synthesized P-1 SDRAM checkpoint.

Three bisectable CI builds have also been downloaded locally under the ignored
`output_files/hardware-milestones/` directory:

| Milestone | Commit | RBF SHA-256 |
|---|---|---|
| F2 status readback | `9c16729` | `40992b8e41ead9a9441734aedd171bd0e0942412fbadb089d62e26e4d3e8ba0c` |
| F3 complete, before F5 | `9956d83` | `9634938072dec7a9c82676a3a5b7192ef6927df50e77d164b84963d0d6c554d2` |
| F5 plus tied-off SDRAM foundation | `4ffa853` | `67efe7c7d07f49b31edb6dfca0c19ccf99237bd62df5eefdb0c780a86c95f0a9` |
| Final milestone with Dandanator isolation | `ba5b629` | `7ede21c7449868764f576c114f1697ffd5e6ce4a9b98a38679861d2d52dd3249` |

The final fitter used 14,891 / 41,910 ALMs (36%), 685,217 block-memory bits (12%),
and 3 / 6 PLLs. Worst setup and hold slacks were positive at +0.439 ns and +0.151 ns
respectively.

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
5. Load a normal 512 KiB Dandanator image. The final build rejects bytes at `0x080000` and
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
- F5 implements the main type-0 R0=0 freeze, R2-dependent HSYNC behavior, live entry and
  recovery, MA/RA behavior, type switching, and odd-field VSYNC freeze. One narrower F5
  subcase remains an explicit XFAIL: when C9 already equals R9 at freeze entry, real type 0
  increments C4 once before freezing.
- The first ACCC v1.10 F12 slice implements type-0 R5 arbitration at C0=2, the R4/R9
  last-line write windows, the exact-C0=R0 R9-to-R5 split, bus-phase capture, and adjustment
  completion. Twelve `t16a`-`t16l` vectors protect the C4/C9 results, the exact-R0 R4
  boundary at both bus phases, and arbitration-state clearing on snapshot load and live type
  changes. The R5=0 C0=1 entry and
  `R0<2` default-adjustment route remain deliberately outside this slice pending focused
  traces; the existing F5 XFAIL remains named.
- The current local gate reports 38 required CRTC passes, one named expected failure, no
  unexpected passes, and no failures.

The next classic checkpoint is the remaining F12 entry state, still test first. Add focused
vectors for the `R5=0` C0=1 equality-break route and correlate `R0<2` with the named F5
XFAIL before changing that shared state. Then add `t07` equality/overflow and `t08` CRTC-ID
boundary vectors, including the tightened type-0 RLAL guards, before implementing F4. F12
and F4 then unblock F8, the remaining F9 worked-example coverage, and F7 in that order. F6
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
- `ansible/` provisions the Debian 13 arm64 UTM guest at `192.168.64.3`, restores the
  `renaud` user's supplementary groups, mounts Rosetta, registers amd64 binfmt, and validates
  a real amd64 binary. Quartus itself still requires the manual Intel installer/EULA step;
  then run `ansible/post-install.yml` and `ansible/validate.yml`.
- ACCC v1.10 is now the primary documentation baseline. The checked-in digests and
  `accuracy/accc-1.10-differences.md` capture its rules and the edition delta; consult the
  full PDF only when a page is specifically flagged for re-extraction.
- The untracked `docs/ACCC1.10-EN.pdf` and `docs/ACCC1.9-EN.pdf` files are user-owned source
  material and must remain outside commits. v1.9 is retained only to verify the edition
  delta and historical citations.
- The first F12/t16 implementation slice is now present in RTL and the executable harness.
  It is a deterministic simulation milestone, not yet a synthesized or hardware-tested RBF.

## Next-session order

1. Test the preserved `ba5b629` RBF on MiSTer and record the result.
2. Incorporate real MiSTer observations; add a deterministic regression before repairing
   any newly found behavior.
3. Classic: finish the F12 `t16` entry-state vectors for C0=1/R5=0 and `R0<2`, preserving
   unresolved pin timing as named XFAILs. Follow with the F4 `t07`/`t08` vector-only
   checkpoint and equality/overflow implementation.
4. Plus: in a separate stack, implement P0 MMU/parser/boot integration against the existing
   memory-service and SDRAM contracts.
5. Update this file when either stream reaches its next hardware-testable checkpoint.
