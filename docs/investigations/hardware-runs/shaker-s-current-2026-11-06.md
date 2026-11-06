# SHAKER CRTC1 S: current numeric failure — 6 November 2026

The full canonical Module C walk on `fdf348c` prints **0840 WRONG
(EXP:4E40)** on S/page02D3. This is a computed result mismatch, independently
of the older photograph's unresolved4E30/program-revision binding. The earlier
[scoped direct/history passes](shaker-s-history-2026-11-04.md) remain historical
evidence; they do not establish current acceptance. A fresh direct-entry
control on the same RBF gives4E40. This establishes a history/entry-path
difference; one canonical prefix repeat is running before selecting the
responsible state or attributing it to a particular later RTL change.

Direct entry completes13commands, two captures, no failures and one SSM
record. The marker capture reads4E40; the later explicit grab is black and
is retained as acquisition evidence. Root viewed both; CFG restoration and
PNG hashes pass.

The native full walk completes80captures without capture failures and152SSM
records. The RBF is `Amstrad_20261106_fdf348c.rbf`, SHA256
`17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`;
the original2.7 disk SHA256 is
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
Actual SSM filter observations are Full0. Native screenshot capture omits
the OSD and physical display response.

Fresh CPCSyntaxError v0.12.0, matching source
`22de765ef44da46c55217e4dd7caa385d69a6045`, completes the original full
CRTC1 Module C script with `--no-chain --no-errata` and prints4E40.
It uses its isolated default UK firmware; MiSTer uses French firmware.
This is a positive peer control, not an independent silicon rule or a
phase-matched comparison. Root viewed both full-resolution target images.
The screenshot-triage worker's provisional0040 reading was incorrect;
the accepted native value is0840.

Private evidence is under
`local/overnight-20261103/resumed-20261104/`:
`milestone-release-20261106/captures/crtc1-C-001/` and
`c1-s-cpcse-history-20261106/` (pinned plan, command, exit receipt, PNG and
parent hash/assessment). Other CPCSE pages are not newly adjudicated.
The ledger now marks this one previously unknown page broken, without
claiming a newly introduced regression or changing its expected value.
