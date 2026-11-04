# SHAKER E short checks — 4 November 2026

These two short runs completed before the user clarified work order: finish
known SHAKER and DSC4/Amazing defects before launching a broad fresh baseline.
E3 and the proposed full A/B/C/D matrix were not launched. Keep these captures
as targeted evidence; do not label their successful execution as hardware passes.

Both use `Amstrad_20261104_7ac11a2.rbf`, SHA256
`4dc1e7628248a30092719ff026c63997bae6d95810c49bc7ad981c10b7a501ec`,
and the installed author disk SHA256
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
The source and full build are pinned in the [R0 milestone](asic-r0-terminal-latch-2026-11-04.md).
Canonical E0/E1 CSL, French keyboard, SSM and requested Full are used with
`--no-follow-loads --max-wait 120 --tolerate-capture-timeout`. All recorded
SSM observations report requested/applied mode0 (Full); general manifest
settings alone remain request-only metadata.

| CRTC | Commands | Captures | Capture failures | SSM records | Result |
|---|---:|---:|---:|---:|---|
| 0 | 34 | 11 | 0 | 27 | Complete, including0407..040A, beyond the older package's0407 stall |
| 1 | 25 | 7 | 0 | 16 | Complete |

Parent inspected all18 PNGs in five contact sheets. E0's0407/0408/0409 show
several numerical `WRONG` results in the R9/R5 comparator-switch family;
0403/0404 have truncated/repeated display geometry. These are observed program
outputs, not original-machine adjudications. The cached original-CPC index
contains472/465/413 images for CRTC0/1/3 and no module-E photograph. Do not
invent a photo pass/fail denominator for these new E pages.

E1 retains the warning text on0401, and0402 has cropped warning/header text
instead of the earlier blank capture. That change alone does not identify a
counter, VMA or acquisition cause. The new captures do not close the existing
E/RFD questions. A targeted matching-version CPCSyntaxError comparison can
provide a source lead; it is not a replacement hardware oracle.

A fresh pinned CPCSyntaxError v0.12.0 E1 run also completes seven captures and
shows the warning on0401. Its0402 has the full header where the native image
is cropped. Parent inspected all seven software captures. Several03FC numeric
rows differ from MiSTer, but the software warning itself is not a hardware
success that can settle the VMA rule. Inputs are the same canonical script and
disk, `--model cpc6128 --crtc 1 --no-chain --no-errata`, with the supplied
firmware directory. Software evidence is in sibling `cpcse-e1-v012/`.

Private inputs, complete manifests, hashes and images are under
`local/overnight-20261103/resumed-20261104/device/e{0,1}-7ac11a2/`, with the
contact sheets beside them. No production code or new RBF was introduced.

## Direct E6 control and author-feedback lead

The direct key6/CRTC0 check on7ac11a2 completed16commands/fourcaptures, zero
failures and nineSSM records. All four PNGs0407..040A are byte-identical to the
earlier full E0 walk. The comparator case21 still gives003A against0066;
[the source-derived fix](crtc0-r9-r5-comparator-2026-11-04.md) has separate
review/build acceptance. This direct entry preserves the observed page state
without repeating the preceding module tests.

A Terra high source map covers all28numeric cases and exact polling addresses
in ignored `resumed-20261104/e6-case-map/CASE-MAP.md`. Cases27/28 label the
update R9=2, but their original instructions are `LD BC,BD03` at65AD/6624
followed by `OUT(C),C`, hence writeR9=3. Parent checked both bytes in the pinned
payload1a747864…bca1. Treat this as a **label/source discrepancy for author
feedback**, not proof that either the recipe or expected results should change.
The current disk and scripts remain unchanged. Native expectation text alone
does not substitute for a photograph or source-derived rule.
