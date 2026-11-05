# CRTC1 DE/03B6: text-row residual no longer reproduced

Accepted production e24cb50 now displays the photographed A/A/B text rows on
03B6, both after normal-boot direct E and after original D9→E. All six E PNGs
are pixel-identical between those entry paths. The old5fba168 full-walk image
had only one A row; that content difference is not reproduced on these two
current paths. No new RTL or causal bisection is claimed.

Direct E completes22commands/6PNGs/zero failures/13SSM records. D9→E completes
42commands/18PNGs/zero failures/35records. Both use unchanged original
subsection commands from the updated package, normal SHAKE27D disk boot,
French keyboard, CRTC1, Plus off, Full filtering and explicit task-owned SSH
reuse/50ms SSM polling. Root viewed all six direct images, all18 history
images in a contact sheet, full03B6 and its physical reference
`ref/cpc_CPC/CRTC1/DE_03B6_E.webp`. All six E pages show the expected bounded
text/result families, including OK at03AE/03AF/03B4 and the offset message at03B5.
D9 flashing phases are not promoted to photograph matches from one frame.

RBF SHA256 is c65d7e60ac4ff95263e2bf425da248c31b6c838bfadb90f2e653df533605cc46;
disk SHA256 is6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61.
Private evidence:`local/overnight-20261103/resumed-20261104/c1-03b6-current/`
contains both extracted CSLs, native manifests/PNGs and
`DIRECT-D9-COMPARISON.json` with all six RGB equality checks.

The printed FF versus the physical photograph's5F remains the separately
known PPI machine-input difference. It does not invalidate the restored text
rows or prove all PPI inputs correct. Neither short entry is qualified as the
entire canonical D history; this is a bounded known-residual recheck, not a
new baseline or a refresh of its historical comparison denominator.
