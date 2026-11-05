# CRTC0 IVM exit: correction of the persistent-comparison reading

The latest French ACCC v1.11 §19.8.1 pp.221,224–225 contradicts the persistent
post-exit frozen comparator in F16. Root reads the rendered pages directly;
a focused independent Astra-medium reading confirms p225. This corrects a
misreading in the current digest, Q19(b), Q21 and the old exit-test expectations.
It is not new author correspondence.

The source history explains the stale expectations: archived
[Round1 feedback §2.8](../../classic/archive/accc-author-feedback-round1-2026-08-27.md)
already records the corrupted1/7/7 and2/7/7 exit cells as corrected to2/0/0
in v1.11. The [edition differences](../../classic/archive/accc-1.11-differences.md)
record the same correction. The current French file matches the known reissue
SHA25628f25c73c1797578522f34ce9ff558210386972c9257b5b8081927483ee02c3b.
Those corrections had not reached the persistent F16 rule and its vectors;
the later claim to have visually reconfirmed a C9=7 continuation was also wrong.

All four odd-frame panels on French p225 end with
(C4,C9,C9-VMA)=(1,6,6), then(2,0,0). None contains a C9=7 row. In the
bottom-right panel the exit line is(1,3,7), followed by(1,4,4),(1,5,5),
(1,6,6),(2,0,0). French p224's nonmatching even-frame exits show the same
ordinary continuation; its previously alleged bottom-left anomaly is absent.
The matching even-frame exit at C9=3/VMA6 instead immediately advances to row2.

French p221 describes using the old IVM-derived C9-VMA against plain R9 on
the exit/write line, with the mode state taking effect at the next C0=0.
Its recovery example changes R9 to the exit-line VMA **before that line ends**.
The displayed tables do not expose physical comparator topology or every
possible bus/seam phase, but they decisively reject an unchanged frozen
nonmatch persisting until raw C9 wraps31→0. Q21's premise is withdrawn;
exactly coincident writes and unshown cases remain separate limits.

Fresh hardware-photo-backed C5 evidence prompted this recheck. Native42 prints
all odd; fresh CPCSyntaxError0.12.0 and AmSpirit reproduce6E/6O/OEOEOE.
The unchanged original production-T80 even caller reproduces equal PPI bytes
with source write coordinates matching the peer; passive observation sees the
current frozen flag over31 boundaries. The separately captured odd SNA loses
its hidden parity on the public loader, so the two replays are not independent
parity cases. This is useful original-caller evidence, without an inferred
complete-frame delay or physical-aperture claim.

The Sol-high correction removes only the persistent comparator/state and its
live/seam mux legs. Seven source-corrected existing exit owners fail239/7 on
unchanged005, then the full246-owner suite passes with the correction. The
p221 recovery tests now write R9 on the exit line, preserving every recovered
row assertion. Existing B1 frame-resumption and entry/lifecycle controls pass.
Fresh Opus reviewGO confirms the source reading and transport scope.

The untouched original even C5 replay changes F4/F4(O) to F4/F5(E). All five
accepted writes,7106PPI read-prefix bursts and4616line records match through
the affected seam. The first counter difference is old31/7→31/8 versus new32/0;
the new rawVS rise falls between the two original stable read apertures.
This is a causal scalar proof without a fitted CPU timing rule.

The tracked transport passes final `python3 sim/select_tests.py --run`:
46benches, including246CRTC owners. Fixed-seed soak intentionally changes
d8edb00f54abdb5e→ed57ffffdbf0b452 with unchanged stimulus/projection. Only two
review-requested stale comments differ from the frozen proposal. Full synthesis
and native C5/B1 acceptance remain pending. A separate untouched pre-freeze
initializer replay is rebuilding the original parity-selection context; no
complete18-call result is claimed until it finishes.

Evidence: `local/overnight-20261103/fr-p{221,224,225}.png`;
`local/overnight-20261103/resumed-20261104/c0-c5-029e-recheck/` contains
`FRENCH-EXIT-TABLE-CHECK.md`, accepted peer/source assessments and
the qualified original-caller `proof/`. Root verifies69 proof artifacts and29
compiled source bindings to005ea84. Dated earlier reports retain their original
claims for audit history; current F16, the digest and author-question list are
corrected instead of silently treating those reports as authority.

Technical information sourced from the Amstrad CPC CRTC Compendium by Longshot
(CC BY-NC-ND).
