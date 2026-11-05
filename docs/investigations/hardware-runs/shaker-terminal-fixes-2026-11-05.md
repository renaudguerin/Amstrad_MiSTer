# SHAKER terminal protection and adjustment fixes

Source005ea842ccfc1613f20abf380ae4b91fbc7d6a34 on the experimental branch
contains Type0 R7 protection338d111, Type1 terminal adjustment entry6e3f14f
and first-C4=1 address reload005ea84. All are Sol-high authored, proven
failing-first and independently reviewed by Opus5.5. The final selection passes
46benches, including246CRTC owners; soak is d8edb00f54abdb5e.

[Full run42083118790](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42083118790)
passes all required jobs on that exact source: productionT80, simulation/lint,
synthesis and required gate. Artifact13444240458, `Amstrad-build-395-1-full`,
contains `Amstrad_20261105_005ea84.rbf` (3,088,616bytes), SHA256
cb059f2e291bd384792cb4f9b1353d20b7c58e909ef72489ceab049857250be7.
Clean_full Quartus17.0.2Build602 gives setup0.324ns/hold0.243ns, seven clocks
each and zero TNS. Top-level warning IDs/counts equal42befa3; no actual critical
warning/error, inferred latch or implicit net appears. Device copy is hash-identical.

Targeted native checks use the unchanged author disk6b9ff106…0eb61, FR keyboard
and Full filter. Root views all completed captures:

| Caller | Current native result | Bound |
|---|---|---|
| C0 A4/AI |28commands/eight PNGs/zero runner failures;0091 lengths6B/6B/38 and corrected post-toggle VS bits. The six preceding captures are RGB-identical to42. |AI0091 repaired; separate A4 OUTI fields remain open, no whole0037 pass.|
| C1 ModuleEOption2 |23commands/six PNGs/zero runner failures;0401/0402 show only success, both0400 images remain RGB-identical. Later0401 is273rows versus prior250. |Original Option2 behavior matches inspected AmSpirit reference; no new original-CPC photo denominator or C5 count closure.|

All15cohorts complete300commands/72PNGs with no runner failure; root views all.
O's six images and E/P/S's four are RGB-identical to accepted5261414.
Static C0/Plus eight images and all20D7images are RGB-identical to42.
C5's three remain RGB-identical/allO, as expected before its separate fix.
All six DSC4/Amazing groups retain the sampled scenes. DSC4 credits and first
main captures are exact; animated bottom/later phases differ. Amazing's first
static text window y134..210 is RGB-identical on C0/C1/Plus; animation phases
are not normalized, so whole-image equality or full-demo acceptance is unproved.
This build does not contain the separate private F16/C5 comparator correction.
No broad milestone baseline, master integration or release is implied.

Build receipt, retained reports, scripts, native manifests/captures, image
comparisons and parent dispositions are under
`local/overnight-20261103/resumed-20261104/shaker-005ea84-build/`.
Source-derived proofs and review details remain in the
[AI0091](shaker-crtc0-0091-protection-2026-11-05.md) and
[ModuleE](shaker-module-e-vma-2026-11-05.md) investigations.
