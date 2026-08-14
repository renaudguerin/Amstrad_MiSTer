# Amstrad MiSTer accuracy and Plus implementation roadmap

This is the execution plan for improving the existing CPC core and, separately, adding
Amstrad Plus/GX4000 support. The detailed evidence remains in `accuracy/` and
`plus/references/`; this document records dependency order, integration boundaries, and
acceptance gates for a fresh implementation session.

Do not combine the two work streams merely because both concern video. Classic accuracy
changes refine `rtl/UM6845R.v` while retaining the netlist Gate Array. Plus support adds a
parallel ASIC path with its own type-3 CRTC, memory mapping, palette, sprites, and DMA. They
can share simulation infrastructure, but neither stream should wait for the other or be
merged into the same behavioral PR.

## 1. Current baseline

- `master` is the upstream baseline.
- The current development branch contains the accuracy/reference documents, the F1-F3 and
  main F5 corrections, the Verilator CRTC/Plus gates, and GitHub Actions synthesis.
- GitHub Actions has completed simulation, Quartus 17.0.2 compilation, fitter, TimeQuest,
  RBF packaging, and artifact upload for the final hardware-test milestone (`ba5b629`, run
  `31637450102`). New top-level/file-list commits still require their own run.
- `sim/` currently reports 46 required CRTC passes with no expected failures. The Plus leaf
  and SDRAM integration suites are green.
  Do not start another timing-sensitive finding until its focused failing vector exists.
- P-2 model plumbing and the P-1 cartridge memory/SDRAM contract are implemented but
  behaviorally tied off. No Plus/GX4000 firmware or cartridge boots yet.
- ACCC v1.10 is the primary Compendium baseline. Prefer the checked-in digests, audit
  prompts, and `accuracy/accc-1.10-differences.md`; consult `docs/ACCC1.10-EN.pdf` only for
  rules explicitly marked for re-extraction. Retain v1.9 only for historical comparison.
- The v1.10 documentation rebaseline and deterministic F12/t16 counter milestone are
  complete. The next classic checkpoint is F4's test-only `t07`/`t08` foundation.

The current branch is a useful staging branch, not a requirement to publish one large PR.
The commits may be rearranged into the small sequences below before publication.
See `current-status.md` for the exact handoff and real-hardware checklist.

## 2. Integration rules

1. Keep classic CPC mode bit-identical when implementing Plus plumbing. With the new Plus
   selector off, existing CPC model bits, CRTC selection, ROM loading, video, memory, disk,
   tape, and snapshot behavior must retain their old paths.
2. Keep Plus code out of `UM6845R.v`. The Plus ASIC's type-3 CRTC belongs in the new
   behavioral Plus path described in `plus/architecture.md`.
3. Give every behavioral change one focused commit and one deterministic regression test.
   A later commit may refactor only after the behavior commit is independently green.
4. Add every new synthesizable Verilog/SystemVerilog source to `files.qip` in the same
   commit that first instantiates it. Also add it to the relevant simulation file list.
   A simulation-only pass is not evidence that Quartus can see a module.
5. Preserve neutral bus behavior. Existing CPU read responders return `8'hFF` when not
   selected and are combined by bitwise AND. New Plus responders must follow that contract;
   open-bus behavior is an explicit selected response, not an accidental undriven signal.
6. Do not mix a new memory client, a new arbiter policy, and a file parser in one commit.
   Establish and test the memory interface first, then connect the CPR stream.
7. No acceptance test may silently depend on an unavailable ROM, cartridge, disk, or
   hardware setup. Unit tests generate their own stimuli. Manual assets and hardware are
   named separately in the checkpoint record.

## 3. Verification ladder

Every checkpoint advances through the cheapest applicable gates in order.

### Gate A: deterministic local checks

- `make -C sim` must run the full CRTC suite non-interactively and return nonzero on an
  unexpected failure. Existing known divergences may start as named `xfail(Fn)` cases;
  implementing Fn changes only that case to a required pass.
- Plus modules get similarly deterministic, self-contained benches. Tests should assert
  exact cycle, decoded address, selected page, returned byte, interrupt edge, or pixel/pen
  result. A screenshot is not a unit-test oracle.
- Run HDL lint where available and `git diff --check` on every commit.
- A failure reports the test/vector, time, expected value, and actual value and produces a
  bounded VCD trace when timing is relevant.

### Gate B: synthesis and integration

- The GitHub Actions Quartus 17.0 build must produce the RBF, fitter summary, and TimeQuest
  report. First run this once on the unmodified integration baseline; otherwise a toolchain
  failure can be mistaken for an RTL regression.
- Synthesize every checkpoint that changes top-level wiring, clocks, memory arbitration,
  RGB width, or `files.qip`. Small CRTC-only commits may share one synthesis run only after
  their individual simulation gates pass, but keep their commits independently bisectable.
- Record fitter utilization and the worst timing result at each Plus milestone. A sudden
  change is a regression signal even if Quartus returns success.
- The local UTM/Quartus setup in `docs/building.md` is a second route to the same gate, not
  a different acceptance standard.

### Gate C: real software and hardware

- Run SHAKER or the named diagnostic for the changed behavior before calling a timing fix
  complete.
- On MiSTer, first verify classic boot, a known-good disk `cat`, video, keyboard/joystick,
  and reset. Then run the finding- or milestone-specific titles.
- Capture the RBF commit, MiSTer version, selected model/CRTC, test image hash or filename,
  and observed result. For visual comparisons, retain a real-hardware or trusted-emulator
  reference and describe the expected raster feature.
- A hardware-only success never replaces Gate A. It promotes a deterministic implementation
  after simulation and synthesis have passed.

## 4. Classic CPC accuracy checkpoints

Classic work is intentionally serial because most findings touch the same state machine in
`UM6845R.v`. The required order below supersedes the older priority table in
`accuracy/audit-findings.md` where dependencies differ.

| Checkpoint | Work, in order | Deterministic exit | Hardware/software exit |
|---|---|---|---|
| **C0: establish the harness — deterministic complete; hardware pending** | Implement the Verilator harness; run the complete register table; verify the already-present F1 fix rather than rewriting it | `t01` passes for types 0 and 1, baseline protection tests pass, and all unimplemented findings are explicit named xfails | One baseline RBF builds remotely; classic CPC boot/disk smoke test passes |
| **C1: status readback — deterministic complete; hardware pending** | F2 only | `t06` proves bit 5 changes only at the required C0=R0 sample and excludes the dynamic R6=0 border case; `t01` remains green | SHAKER/type-detection status test |
| **C2: VSYNC write timing — deterministic complete; hardware pending** | F3 only | `t02` covers type-0 blocked writes at C0=0/1, type-0 extended duration, and unchanged type-1 partial-line duration; `t03` protects re-entrancy | SHAKER VSYNC tests plus Onescreen Colonies and PHX regression |
| **C3: R0 stall — deterministic complete; hardware pending** | F5 only | `t09` proves type-0 freeze, the single deferred C4 increment, R2-dependent HSYNC, clean resume, and unchanged type-1 one-character lines | SHAKER R0 tests; monitor sync and GA interrupt behavior remain stable |
| **C4: border decision** | F6 only if its approximation is explicitly accepted | `t10` distinguishes type 0 and type 1 and proves skew placement | Visual R1>R0 discriminator and affected demos |
| **C5A: v1.10 type-0 adjustment arbitration — deterministic complete; hardware pending** | F12, test first | `t16a`-`t16s` prove C0=0 same-edge comparison, C0=1/R5=0 entry including exact R0=1 rollover consumption, R5 acceptance/rejection around C0=2, R4/R9 live-write windows including exact-R0 at both bus phases, the exact-R0 R9-to-R5 split, R0=0/1 default adjustment, active-adjustment R0=0 freeze, completion reset, and retained-state lifecycle | Focused SHAKER or hardware traces verify uncertain sub-character MA/DE/VSYNC timing without changing the fixed counter expectations |
| **C5B: equality/overflow foundation** | F4 only, after F12 establishes the corrected state seam | `t07` and `t08` pass, including the tightened RLAL regression vectors; no shortcut term is retained to hide a latch bug | SHAKER overflow/rupture tests and Batman Forever, The Demo, and Yao demo sweep |
| **C6: type-1 adjustment** | F8 only, after F4 | `t11` proves independent C5 counting, continuing C4/C9, RA sequence, and the R5=0 mid-adjustment behavior | SHAKER adjustment vectors |
| **C7: type-0 R9 race** | Revised F9 only, after F12/F4/F8 have stabilized the counter structure | `t12` reproduces both documented exact-cycle results using the v1.10 comparison target; it must not preserve the v1.9 rationale as an oracle | Contrived timing test; hardware trace if simulation and SHAKER disagree |
| **C8: type-1 RFD** | F7 only, after F4/F8/F9 | `t13` proves trigger timing, frame parity, VMA reload, and no behavioral change when never armed | SHAKER RFD tests and a CRTC-1 RFD demo sweep |
| **C9: interlace** | F10 last, split into fixtures, type-0 machinery, then type-1 machinery | New parity and entry/exit fixtures derived from the cited SHAKER tables; all t01-t15 regressions stay green | SHAKER interlace suite and hardware comparison for both CRTC types |

### F6 decision gate

F6 documents a half-character border byte, while the current CRTC-to-Gate-Array contract is
character-granular. Default action is to defer F6. If the project accepts the documented
one-character approximation, implement it in its own optional commit/PR at C4 so it can be
reverted without disturbing counter work. Do not describe that approximation as exact. If
accuracy rather than compatibility is the goal, first design a half-character-capable seam
and reassess its impact on the netlist Gate Array.

### F10 scope gate

F10 is not a normal one-commit finding. Before RTL work, turn the SHAKER 22C/3 tables into
reviewed fixtures and re-check only the PDF pages flagged by the audit. Keep type-0 and
type-1 implementations separate, with a passing fixture-only commit before each behavioral
commit. F10 is allowed to remain a later project after C8 ships.

## 5. Plus/GX4000 checkpoints

Plus development has its own internal stack. It may begin once the shared build and
simulation conventions are stable; it does not depend on completing classic C1-C9.

### P-2: model selection before Plus behavior

**Status:** deterministic implementation complete; hardware remains deliberately
unselected.

Add a separate OSD `Plus model` field with `Off`, `GX4000`, `6128+`, and `464+` values. Do
not extend or reinterpret the existing classic CPC `Model` field. Decode the new value once
into `plus_mode` and static model capabilities (RAM size, FDC, and tape), then plumb those
signals without changing behavior while `Plus model = Off`. Cartridge reset-page selection
is deliberately not a static capability: GX4000 fixes the high window to page 1, whereas the
464+ and 6128+ select page 1 or page 3 from the external `/EXP` state. P0 must define and
test that dynamic input before implementing reset mapping.

Exit requires an exhaustive decode test, reset/default-off test, and a classic-mode
integration trace showing unchanged selected paths. Reserve non-overlapping status bits and
document them beside the existing status map.

### P-1: cartridge SDRAM contract before CPR parsing

**Status:** deterministic implementation and Quartus integration complete as a tied-off
foundation. P0 owns the first production connection of the cartridge service.

Before accepting `.cpr`, write and test one cartridge-memory ownership design. It must fix:

- the 512 KiB SDRAM region and collision check against every existing client;
- one canonical conversion from `{physical_page[4:0], offset[13:0]}` to SDRAM address;
- ownership of request/acknowledge signals and arbitration between ioctl loader writes and
  CPU cartridge reads;
- priorities during download, reset behavior, bounds rejection, which layer owns short-page
  zero fill, and behavior if the CPU requests a cartridge byte while a load is active; and
- where address arbitration joins the existing RAM/tape/disk SDRAM schedule, including the
  synthesis-visible module/file-list boundary.

Use a small memory service or a clearly owned extension of `sdram.v`; do not duplicate page
address arithmetic in the parser, MMU, and motherboard. Unit tests must interleave loader
writes and CPU reads over page 0, page 31, the last byte, invalid page/address values, and
reset. Only after this interface passes may P0 connect RIFF/CPR parsing.

### P0-P8 functional milestones

The corrected functional phases and exact exits live in `plus/architecture.md` §4. In
summary: cartridge boot is followed by the CRTC3 counter/timing and basic pixel foundation;
only then come the ASIC page/palette, PRI, and sprites. Exact readback and I/O traps may land
later because they do not provide timing signals consumed by PRI/sprites. Split/scroll, DMA,
and platform quirks remain later milestones.

At the ASIC-page milestone, use an exhaustive decode table and explicit read-response
contract:

- selected readable sources participate in the core's `8'hFF`-neutral wired-AND CPU bus;
- unmapped and write-only reads return the modeled Plus open-bus byte;
- sprite pixel writes retain only the low nibble, X/Y high bytes apply their documented
  masks/sign extension on read, mirrored registers follow the chosen documented rule, and
  DCSR read/write ranges differ as specified;
- ASIC-page writes do not write through to underlying main RAM; and
- any deliberately emulated external-expansion dual-write bug is isolated behind a named
  condition and test, not produced accidentally by incomplete decoding.

Each milestone ends with a classic-mode regression, a Quartus build, and the phase-specific
diagnostic/title named in the architecture. A title reaching a screen is useful smoke
evidence, but it does not replace the decoder, counter, interrupt, or compositor assertions.

## 6. Commit and PR structure

No PR needs to be created merely to follow this plan. Keep local commits in the same shape
so they can later be pushed as small stacked PRs.

### Classic stack

Recommended stack: `C0 harness` -> `C1 F2` -> `C2 F3` -> `C3 F5` -> optional `C4 F6` ->
`C5A F12` -> `C5B F4` -> `C6 F8` -> `C7 F9` -> `C8 F7`. F10 is a separate stack. The
existing F1 commit can remain distinct; C0 adds its deterministic verification.

- One finding per commit and normally one finding per PR.
- A small PR is roughly one behavior change, its focused vectors, and a short documentation
  update. Avoid a line-count rule; semantic separability matters more.
- Harness scaffolding may be one medium PR, but its initial test-data commits should remain
  reviewable and must not change DUT behavior.
- Merge the classic stack in order. Rebase later branches after each merge so a regression
  can be attributed to one finding without resolving repeated `UM6845R.v` conflicts.

### Plus stack

Recommended stack: `P-2 model field` -> `P-1 cartridge memory contract/service` -> `P0 CPR +
boot` -> `P1 CRTC3 foundation` -> `P2 ASIC page/palette` -> `P3 PRI` -> `P4 sprites` -> `P5
CRTC3 bus quirks` -> `P6 split/scroll` -> `P7 DMA` -> `P8 polish`.

- P-2 is independently mergeable because default-off behavior is invariant.
- P-1 may be independently mergeable if the cartridge service is unselected in classic
  mode. P0 stacks on it; do not combine the parser with initial arbitration.
- P1-P8 stack internally because they share the Plus ASIC interfaces. They should not stack
  on unfinished classic findings unless they need a shared harness commit already destined
  for merge.
- A Plus PR may contain several commits only when they form one vertical milestone: module
  logic, unit tests, integration wiring plus `files.qip`, and documentation. Keep each commit
  buildable where practical and the final PR deterministically testable.
- RGB widening, SDRAM arbitration, clock/WAIT changes, and PPI changes deserve their own
  focused integration review even when they are part of a milestone.

## 7. Bisect and rollback rules

1. Every commit must compile its local simulation targets. Never leave a red intermediate
   commit in a stack that will be used for `git bisect`.
2. Behavior changes and broad refactors do not share a commit. Land a behavior-preserving
   rename/extraction first or after the verified change.
3. Do not squash different findings or Plus milestones together. A fix discovered during a
   later milestone gets a `fixup!` commit until review, then is folded only into that same
   milestone's owning commit.
4. Optional approximations, hardware bugs, and compatibility quirks use named parameters or
   isolated signals and focused tests. Their commits must be safely revertible.
5. Keep `plus_mode = 0` as a stable bisection oracle. For every Plus commit, save a short
   classic regression result; if classic behavior changes, treat it as an integration bug.
6. Tag or record the last commit passing each C/P checkpoint. Retain failing VCDs and Quartus
   reports outside Git or as CI artifacts, identified by commit.
7. When a hardware regression is found, first bisect with the same RBF deployment procedure
   and configuration. After identifying the commit, add a deterministic regression vector
   before repairing it.

## 8. Immediate execution queue

1. Test the synthesized current milestone on real MiSTer hardware using
   `current-status.md`; record classic CPC and F2/F3/F5 results.
2. Classic stream: add and review the F4 `t07`/`t08` vectors as a test-only
   checkpoint, then implement equality/overflow without weakening the type-0 RLAL guards.
3. Plus stream: implement P0 MMU, bounded CPR parsing, `/EXP`, and boot integration against
   the accepted P-1 service/SDRAM contract. Do not combine this with Plus video work.
4. Keep F6 deferred unless a half-character-capable interface is designed or the project
   explicitly accepts the reversible one-character approximation.
5. Continue classic work F4 -> F8 -> F9 -> F7. Start F10 only after its fixtures and PDF
   re-check gate are ready.
