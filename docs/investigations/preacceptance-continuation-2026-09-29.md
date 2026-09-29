# Pre-acceptance continuation, 2026-09-29

This batch continues the delivered `9ddbf74` closeout. The acceptance bitstream
remains `output_files/Amstrad_20260929_cec641c.rbf`, SHA-256
`40f7d66bfcfda37ce9f5053ff3e3af833713bdb4065fe4bcde9c311a3cb4dd2c`.
The changes below add evidence and diagnostics, not production RTL. The user's
separate clean title run on `4a44394` is not extended to another bitstream.

## Integrated evidence

- **MiSTer format 2 and B25:** source `d2eabc5`, integration `d188607`.
  Coherent SSM records establish steady requested/applied modes 0/0, 1/1 and
  2/2. B25 AMSDOS boots on CRTC0/1 produce the complete identical 72-record
  tables matching the production fixture. Full/Raw-pixels screenshots match;
  Raw-CRT changes captured frame geometry. Original CFG was restored and
  verified. [Device evidence](mister-format2-b25-2026-09-29.md). The local
  archive `output_files/mister-format2-b25-2026-09-29.tar.gz` has SHA-256
  `00e41bcf057e14092e729e56e3cd34fc14becf76b08246b59346fae9c21e5795`.
- **Production-T80 SSM provider:** refreshed source `d6525cd`, integration
  `882a200`. Five deterministic cases exercise the actual motherboard fetch
  and resolved-data exports, natural GA WAIT, held-fetch final data, PPI bus
  consumption and interrupt exclusion. The old isolated 0038/0039 mismatch
  was a HALT-address observation difference; completed fetches, ACK and stack
  provide the execution oracle. Fresh Opus review and focused/selected gates
  pass. Exact integration CI `36522821654` passes production-T80, 42 selected
  benches, lint and the required gate; synthesis correctly skips.
  [Provider evidence and limits](ssm-csl/b4-production-fetch-2026-09-29.md).
  Logs are retained in `output_files/b4-production-fetch/`.
- **Real SSH recovery:** refreshed source `a025bb5`, integration `0ed54d9`.
  An acknowledged CFG write was independently observed before its owned SSH
  client was killed. The unmodified runner reported failure and restored
  byte-identical original data through fresh connections, without emergency
  fallback. The reviewed experiment changes no production code.
  [Evidence](ssm-csl/csl-real-ssh-recovery-2026-09-29.md). Its one-shot harness
  and raw run log are retained in `output_files/csl-real-ssh-recovery/`.
- **B9 MID FRAME replay:** refreshed source `fcde0ec`, integration `5f8f8ae`.
  All four authentic page-B MID entries execute with the source-derived entry
  stack. R8=3 is accepted at C0=1A, and both update stages complete mid-line.
  Counts 049D/049F format 4E40/9C60. This entry contract does not encounter the
  known C0=3F collision. Opus review's final-source rerun condition is satisfied
  by the selected slow diagnostic gate and retained trace. Exact integration
  CI `36523293909` passes simulation/lint, production-T80 and the required
  gate; synthesis correctly skips. The private-payload diagnostic itself is
  established by the local selected slow gate, not by hosted CI.
  [Investigation and trace](shaker-b9-mid/README.md); gate/review logs are in
  `output_files/b9-mid/`.

Documentation-only integrations require no simulation; the two diagnostic
integrations retain their independent review and focused evidence. Their path
classification requires no new synthesis. No private SHAKER payload or ACCC
PDF is distributed with the diagnostics.

## Added full SHAKER task — module A complete, B–E deferred

The user added a fifth task: update current suite documentation for the private
`local/test_media/shaker/CSL_27` files and run the full supported matrix. The
old fixed-delay files moved to `CSL_26`. The new scripts use `wait_ssm`; the
reported approximately 30% speedup is an author expectation, not a measurement.
The author warns of untested scripts and possible typos, especially module D.

Run each module/type explicitly, avoiding automatic chains into unsupported
CRTC types. Use the 2.7 script and disk first; record any execution issue and
retain its partial evidence, then run the corresponding 2.6 script and disk.
A successful fallback is not a successful 2.7 script. Preserve author originals,
keep numeric runs in Full, and distinguish runner completion from correctness
of the displayed hardware results. Types 2/4 must not be mapped to 0/1.

The first classic A0 attempt retained 107 captures before SSH hostname
resolution failed. Later cells, including its 2.6 fallback, did not execute
usefully. This is an infrastructure interruption, not a proven CSL typo.
The runner initially could not verify CFG restoration. After connectivity was
restored and MiSTer rebooted, the original CFG was restored and hash-verified
before fresh execution. Module A then completed on CRTC0/1/3 with 122/127/123
captures and no fallback required. At the user’s quota cutoff, B–E were deferred.
Final original-CFG restoration, MENU and temporary-file cleanup are verified;
the device slot is released. The full A–E matrix remains incomplete.

Host support from source `9659157`, integrated as `dcd0218`, implements true Plus/CRTC3:
explicit Plus model policy, system CPR before DSK, and distinct result labels.
The user selected `6128_FR.cpr` with French keyboard layout for these runs;
it boots directly into BASIC without the original system cartridge's F1 step.
The extension passed fail-first regression checks, fresh Opus review and the
205-test host suite (one skip); the selected gate requires no simulation.
Exact integration CI `36526429367` passed simulation/lint, production-T80,
synthesis policy and the required gate; synthesis correctly skipped. Logs are
retained in `output_files/shaker27-integration/`.
The interrupted classic run used the previous runner. The resumed A3 run demonstrates the French cartridge/disk boot path and
CRTC3 script execution; it does not establish every displayed result as correct. The [suite report](shaker27-full-suite-2026-09-29.md)
records partial A0 screen discrepancies without claiming original-hardware
correctness. Its retained local archive is
`output_files/shaker27-suite-2026-09-29.tar.gz`, SHA-256
`1247f98b779e9479981af20aee81b1ebdd08eb22359373334aa7b20a72d2e13f`.
The completed module A evidence archive is
`output_files/shaker27-suite-2026-09-29-module-a.tar.gz`, SHA-256
`ff5239149d379469ef4d7c94bfc4ec31d0319745ecd314ea894eed0c147dca2d`.
Resume B–E with fresh output directories and a new independent CFG snapshot.
The 30% speedup remains unmeasured; image/reference comparison is separate
from successful script completion.

## Remaining physical acceptance

Original-CPC B25 counts and the PA3 original-Plus follow-up remain open.
The original type-1 MID glyph (4E40/4F40) still needs direct evidence. CTM and
connector timing, pathological filter acquisition, live mode transitions,
real cartridge/SDRAM provider coverage and persistent SSH-outage recovery are
not established by these bounded checks. No speculative hardware repair follows
from simulator agreement or an unclassified screenshot.
