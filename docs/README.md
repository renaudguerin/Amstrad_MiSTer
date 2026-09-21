# Amstrad CPC & Plus MiSTer Core — Documentation Map

Welcome to the documentation for the cycle-accurate Amstrad CPC and Plus / GX4000 FPGA core for the MiSTer platform (targeting Cyclone V / DE10-Nano).

---

## 1. Authority Ranking & Verification Precedence

When investigating behavior, writing testbenches, or implementing RTL rules:

1. **Real Hardware & Logon System Photographs** (`shaker.logonsystem.eu`): Real silicon behavior outranks all written documentation.
2. **French CRTC Compendium (ACCC) v1.11**: Definitive documentary oracle for CRTC timing, internal counters, and per-type edge cases. (English edition serves as translation/reading aid).
3. **Hardware Platform Specifications** ([`docs/specs/`](specs/)): Specifications for the Gate Array, ASIC, MMU banking, PPI, PSG, and container formats.
4. **Internal Digests & Audit Findings** ([`docs/classic/audit-findings.md`](classic/audit-findings.md)): Internal findings F1–F12 and verified timing rules.

---

## 2. Core Architecture & Two Development Streams

Development is split into two strictly separated behavioral streams:
- **Classic Accuracy Stream** ([`docs/classic/`](classic/)): Hardens the cycle-accurate classic CRTC core (`rtl/CRTC.v`, `rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`) and gate-level netlist Gate Array (`rtl/GA40010/`).
- **Amstrad Plus / ASIC Stream** ([`docs/plus/`](plus/)): Implements the behavioral Amstrad Plus ASIC (`rtl/plus/asic_video.v`, `asic_dma.v`, `asic_regs.v`) for 464+, 6128+, and GX4000 features (sprites, 4096-colour palette, split screens, PRI, DMA audio, cartridge banking).

---

## 3. Project Management & Workflow

- **Status & Roadmap**:
  - [`current-status.md`](current-status.md): Current handoff state per stream, open hardware residuals, latest artifact.
  - [`implementation-roadmap.md`](implementation-roadmap.md): Strategic dependency order and acceptance gates.
  - [`backlog.md`](backlog.md): Prioritized architectural, observability, and structural debt items (B1–B20).
  - [`review-debt.md`](review-debt.md): Open cross-provider review debt; cleared rows are in [`archive/`](archive/).
- **Build & Infrastructure**:
  - [`building.md`](building.md): Verilator simulation, local compilation, and Quartus setup.
  - [`ci-testing-policy.md`](ci-testing-policy.md): GitHub Actions simulation matrix, runner tiers, and synthesis policy.
  - [`task-workflow.md`](task-workflow.md): Multi-agent orchestration, worktree lifecycle, and reference provisioning.

---

## 4. Documentation Hierarchy

```text
docs/
├── specs/                        # Canonical hardware & platform reference specifications
│   ├── README.md                 # Inventory & precedence ranking
│   ├── common/                   # Shared platform specs (PPI, PSG, MMU/PAL, FDC, I/O map)
│   ├── classic/                  # Classic Gate Array & CRTC controller datasheets
│   ├── plus/                     # Amstrad Plus ASIC ("Arnold V"), CPR cartridge spec, IM0
│   ├── formats/                  # Container formats (SNA v1-v3, DSK, EDSK, CDT)
│   └── datasheets/               # Industry datasheets (OKI 82C55, Zilog 8536)
│
├── classic/                      # Classic CRTC accuracy stream (symlinked from docs/accuracy/)
│   ├── audit-findings.md         # Numbered findings F1–F20
│   ├── compendium-01-counters.md # ACCC counters analysis
│   ├── compendium-02-sync.md     # ACCC sync analysis
│   ├── compendium-03-display-regs.md # ACCC display register analysis
│   ├── shaker/                   # Shaker test descriptions & cross-references
│   ├── accc-update-procedure.md  # Runbook for adopting a new ACCC edition
│   ├── extract/                  # ACCC v1.11 extraction manifest, scripts & page renders
│   └── archive/                  # Settled reviews and completed ACCC diff/audit ledgers
│
├── plus/                         # Amstrad Plus / ASIC stream
│   ├── architecture.md           # FPGA core RTL implementation architecture & phasing
│   ├── asic-documentation-gap-map.md # Documentation gap analysis vs real hardware
│   ├── references/asic-reference.md # Condensed ASIC register and timing reference
│   ├── hardware-test-checklist.md # Title retest matrix and report fields
│   └── archive/                  # Settled reviews and completed implementation reports
│
├── defects/                      # Self-contained defect reproduction directories
│   ├── copter271/                # Copter 271 raster/split defect (ticket, captures, script)
│   └── arn5diag/                 # Arnold 5 diagnostic defect notes & snapshot
│
├── investigations/               # Self-contained topic & architectural investigation folders
│   ├── sonic/                    # Sonic the Hedgehog GX4000 reverse-engineering & timing
│   ├── video-boundary/           # B6 video boundary decisions & synthesis audits
│   ├── write-timing/             # B8 CPU write timing & SDRAM video coherence
│   ├── fdc-timing/               # FDC pre-edge triage & diagnostics
│   ├── ssm-csl/                  # Shaker CSL & SSM protocol design and capture ABI
│   ├── hardware-runs/            # Real-hardware capture logs & diagnostic runs
│   ├── session-logs/             # Stream-orchestration revisit note (cited by task-workflow.md)
│   └── archive/                  # Settled investigation reviews, handoffs & evidence logs
│
├── plans/                        # Dated execution plans; several hold locked decisions still cited
│
├── archive/                      # Superseded top-level ledgers: status history, cleared review debt
│
└── issues/                       # Structured markdown issue tickets (linked to backlog)
```
