# Project Issues & Tickets

This directory contains structured Markdown-based tickets for defects, hardware bugs, and discrete implementation tasks.

---

## Ticket Lifecycle & Format

Tickets are stored as standalone Markdown files: `ISSUE-<number>-<slug>.md` (e.g. `ISSUE-021-raster-fire-latch.md`).

For complex defects requiring local test media, reproduction scripts, and screenshots, prefer creating a dedicated self-contained directory under [`docs/defects/<slug>/`](../defects/) with a `README.md` following this structure.

### Ticket Template

```markdown
# ISSUE-XXX: [Descriptive Title]

- **Stream**: classic | plus | general
- **Status**: open | in_progress | resolved | closed
- **Backlog Item**: [B1-B20] (if applicable)
- **Branch**: `<stream>/<topic>`
- **Assigned Worker / Agent**: [Name / Model]

---

## 1. Summary & Symptom
Describe the observed defect or missing hardware feature. Quote observed versus expected behavior.

## 2. Reproduction & Artifacts
- **Test Media**: Path to test cartridge, DSK, or SNA in `local/test_media/`
- **Screenshots**: Embedded crop or comparison image
- **Harness**: Command to run simulation test or device capture (e.g. `$mister-capture`)

## 3. Root Cause Analysis
Trace the defect to specific RTL signals, clock phase, or missing specification rule. Cite relevant sections of the French ACCC or `docs/specs/`.

## 4. Implementation Plan & Gate
- Failing deterministic test vector (must fail before RTL fix!)
- Proposed RTL edits
- Acceptance criteria (e.g. `make -C sim`, golden soak hash)

## 5. Resolution & Verification Evidence
Record the closing commit SHA, simulation output, and hardware confirmation photograph.
```
