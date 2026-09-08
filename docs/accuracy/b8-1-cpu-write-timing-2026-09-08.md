# B8-1: production CPU write phases

Accuracy task based on `d46609d066aafb6b182fd6fa504a91719500cd91`.
The type engines now retain qualified R5/R0 write events across the interval
between a production CPU write and the CRTC character decision. Register storage
still updates on every system clock. This repairs the real-GA/scripted-bus
regressions described below; it is not a DSC4 or SHAKER hardware fix verdict.

## Capture and consumption contract

The production divider in `Amstrad.sv` supplies one `ce_16` pulse per four system
clocks. The real `ga40010` supplies CPU positive enables at `S=00/0f/ff/f0`,
CRTC `CLKEN` at `S=03`, and `nCLKEN` at `S=e0`. Production `T80pa.vhd` changes
I/O write outputs after its positive CPU enable edge. CRTC register storage
therefore first captures that write on the following system edge, without
`ce_16` or `CLKEN`; at the next character decision the register already contains
the new value. The dedicated fixture instantiates real GA and CRTC modules,
uses the production divider, and launches bus outputs with nonblocking
assignments on those CPU enables. `-UVERILATOR` excludes the GA's extra legacy
comparison-only simulation circuit.

Old register values and DI are compared on the **first system edge that stores
the new value**, using C0 and the route's qualification at that edge. A retained
bit represents the qualified event, not a second register file. A held write
cannot repeatedly qualify because storage already contains its new value.

| Route | Captured qualification | Consumption |
|---|---|---|
| Type 1 R5 RFD | Old R5=0, DI nonzero, C0=old R0 | Next CLKEN arms the existing RFD flags and MA-source decision; C9=R9 still disables the VMA-source flag while arming parity. Stored R5 controls ordinary adjustment; a direct-CLKEN write uses DI for that same decision. |
| Type 1 R0 widening | DI>old R0, C0=old R0, C4=R4, C9=R9, R5=0, outside adjustment | Next CLKEN opens the existing extended-line window. R4/R9 cancellation is evaluated at the actual extended line end; no cancellation means no RFD. |
| Type 0 IA-6 | Old R0=1, DI>1, C0=1, existing true-last-line capture, R5=R8=0, outside adjustment | An early write supplies the vertical action at the next CLKEN while C0 advances 1→2 normally. Reusing the artificial direct-CLKEN hold would duplicate C0=1. The direct-edge path and its subsequent vertical-action latch remain distinct. |

Reset, snapshot load, and a sampled switch away from the owning type discard
unconsumed events. Character decisions consume them once. Register writes,
R2.JIT/R3 restart handling, R6 half-character handling, and R8 transitions retain
their existing cadence. No shared engine interface or CPU/FDC/P10 fixture was
redesigned.

## Source derivation and R6 boundary

Read both user-owned ACCC v1.11 PDFs through pdf-inspector 1.17.0. Both reports
are `native_partial`; the relevant prose is in the native text layer. Rendered
French pp.124 and 127 were checked for R0 timing/counting. The PDFs and generated
outputs remain ignored.

- French §11.6 pp.89–92 (English pp.87–90): R5 0→nonzero at C0=R0 arms RFD;
  neither nonzero→nonzero nor nonzero→0 does. §11.6.1 p.90 (EN p.88) retains
  the terminal-C9 VMA-source exception. With R1=8, start MA=1234, and row 1
  line 0, ordinary saved MA is 123c; RFD must instead reload 1234.
- French §13.6.2 p.124 and §13.7.1.2 p.126 (EN pp.122,124): true-last-line
  R0 widening plus an R9/R4 cancellation arms RFD at the extended end.
  The fixture widens 15→63, then changes R9 3→5 via separate bus address/data
  writes. Expected C4/C9 at the extended end are 3/4, with MA=1234; without
  cancellation the normal frame origin is 0/0 and no flags arm.
- French §13.7.2 pp.126–128, especially rendered p.127: unsafe R0=1 widening
  keeps C9 while C4 overflows, and C0 continues without a repeated character.
  The true-last-line recipe remains in adjustment even with R5=0. Widening
  at C0=0 remains the safe control. The non-last-line overflow recipe and
  instruction-form distinctions remain outside this patch's acceptance.
- French §18.2.3 p.190 / English p.189 agree that ordinary C4=R6 border is
  sticky until frame origin. The old `r6w_b` predicate must not be blindly
  carried into the new event contract: retaining old R6 would enable ordinary
  nonzero-row reopening against that rule. No R6 RTL changed. Separate
  production-phase controls pin sticky equality and reversible R6=0 border
  outside row 0. The last-character/frame-origin exception needs its own
  precisely phased regression; it is not certified here. The known French/
  English type-0 §18.3.2 difference (BL-036/IA-3) is a different issue.

## Verification and retained evidence

The final dedicated fixture has 45 cases: four CPU phases across R5/R0 routes,
terminal-C9 RFD, R6 controls, a direct-CLKEN positive control, and eight lifecycle
checks. On unchanged production RTL from the exact base, **20 cases fail** and
25 controls pass; after repair all 45 pass. Earlier diagnostic exit zero meant
successful reproduction of a defect, not acceptance. Those raw diagnostics are
preserved unchanged.

The first candidate also failed four tightened assertions against inserting an
extra C0=1 character; this was corrected before acceptance. Ignored evidence
under `docs/references/b8-audit-2026-09-08/` includes the original diagnostic,
acceptance failure logs, and final validation logs. Tests were not weakened.

- `make -C sim`: PASS, 192 classic cases, dedicated 45-case phase fixture,
  all Plus/GA/u765 suites; the existing TV80/FDC payload-polling XFAIL remains.
- `make -C sim soak SOAK_EXPECT=0x6e8258198d6e6137`: PASS, 2,845,088 characters
  and CLKEN samples, seed `0xaccc5eed20260822`. The previous expected hash
  `0x2263c9fc44af4ee7` fails because R5/R0 events now execute. Seed, stimulus,
  and sampled field set/order are unchanged. No unrelated assertion changed.
- `make -C sim lint` with installed Verilator 5.052: exit 2, the same five
  fatal SIMILARNAME warnings reproduced from the exact-base RTL/sim archive
  (CRTC field/FIELD and de/DE; asic_crtc hcc/HCC; crt_filter shift/SHIFT and
  hsync_i/HSYNC_I). No warning policy or unrelated source was changed.
  Aggregate lint with CI-pinned Verilator 5.050 is pending toolchain availability.
- `git diff --check`: PASS.

Review status: **REVIEW PENDING**. The coordinator sequences the fresh guarded
cross-provider review. The complete source/diff and French rules above must be
reviewed, especially early-versus-direct type-0 timing, event lifetime, live
cancellation of the type-1 extended line, and preservation of sub-character
paths. A partial review is not clearance. No integration is authorized here.

## Evidence limits

This is French-source and real-GA/scripted-bus simulation evidence. No installed
GHDL, NVC, or Yosys toolchain was found in the bounded production-T80 feasibility
probe. The available boot fixture uses a TV80 surrogate and its existing payload
polling XFAIL remains separate. No production T80 instruction sequence was
executed, and neither T80 nor TV80 was modified. The fixture tests legal launch
phases; it does not certify the different OUT(C),r and OUTI instruction recipes.
No Quartus synthesis, RBF, hardware, DSC4, SHAKER, or title verdict is claimed.
