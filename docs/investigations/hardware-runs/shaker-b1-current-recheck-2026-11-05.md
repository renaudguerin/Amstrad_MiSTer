# SHAKER B1 current recheck — 5 November 2026

The four previously uniform-yellow captures `00D5/00D6/00DD/00DE` now show
structured six-colour images on production `e24cb50` and diagnostic `c90ec8a`.
The corresponding RGB images are identical between the two builds. This is a
bounded recheck of the known pages, not a new B1 baseline or a causal fix
identification.

The current `00D5` and `00DD` captures cannot establish their requested intro
states: their respective successors `00D6` and `00DE` were already published
in the same poll batch before capture began. The `00D6` and `00DE` patterns
are corroborated by the corresponding original-machine photographs at the
pattern-family level. Geometry and filter fidelity have not been judged; do
not count the superseded intro captures as passes.

The older run did record four flat-yellow images. Its marker gap alone does
not prove that the requested guest state persisted until capture: state can
change before the next marker. Accordingly, the earlier provider's definitive
scaler-fault and non-epoch-ambiguity conclusions are not supported. Uniform RGB
alone does not assign the fault to the scaler or core. No filter or output-chain
changes were found in the scoped `5fba168..c90ec8a` interval, and no bisection
selected a causal fix.

Evidence is retained locally: [current capture qualification](../../../local/overnight-20261103/resumed-20261104/acquisition-epoch-audit/CURRENT-CHECK.json),
[production RGB comparison](../../../local/overnight-20261103/resumed-20261104/acquisition-epoch-audit/PRODUCTION-CONTROL.json),
[older acquisition audit](../../../local/overnight-20261103/resumed-20261104/acquisition-epoch-audit/audit-notes.json),
and [assessment of its limits](../../../local/overnight-20261103/resumed-20261104/acquisition-epoch-audit/PARENT-ASSESSMENT.md).
The current captures and manifests are in the adjacent `device-b1-e24cb50` and
`device-b1-c90ec8a` directories. A state-qualified intro capture and a separate
geometry/filter comparison remain needed.

## Faster transport follow-up

A separate original-prefix e24 run with task-owned SSH multiplexing and50ms
SSM polling completes23commands/15PNGs/zero failures/21records. All15PNGs
were inspected, with four full target images and physical photos. D5/DD are
no longer flagged as already superseded in their poll batch, but their PNGs
still show patterns rather than the photographs' intro text. Thus absence of
the superseded flag does not establish guest-state alignment. D6/DE retain the
corroborating pattern families; geometry remains unjudged. No intro pass is
claimed and no further blind transport-timing retry is selected.

Private`ssh-multiplex-tooling/device-b1-e24cb50/` retains the complete manifest,
raw capture hashes and measured requested/applied Full mode0 at the target
markers. `B1-INTRO-RESULT.json` records the limits. Exact opcode-frame capture
and the caller's text-to-pattern transition remain separate from host latency.
