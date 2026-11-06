# ASIC rendered-RGB assurance — 6 November 2026

The exact `fdf348c` milestone bitstream has a qualified enabled-sprite
RED→GREEN proof and fresh Opus 5.5 review. Opus gives **GO**, with no P0–P2
findings; no RTL change or new RBF is required. The current public fixture
remains unchanged. This closes source-review debt, not every hardware gate.

## Independent sprite proof

One settled x1 sprite uses RGB `123` and `AB6` with transparent holes. A
private composition holds the existing external ASIC unlock/page inputs active;
production T80/READY performs 281 real CPU writes per case. There are no page
reads, internal register/RAM forcing or snapshot setup. The same guest timing
and sources are used for enabled/disabled controls and ordinary widths 4/5/6.
This excludes MMU/unlock acceptance.

| Independent check | Before, c0939d8 | Current, fdf348c |
|---|---:|---:|
| Enabled 4/5 LEFT8 mismatches | 681 / 1104 | 0 / 1104 |
| Mixed-colour sprite dots wrong | 20 / 24 | 0 / 24 |
| Four border-edge displacements | 0 clocks | 32 clocks |

The expectation remains French ACCC §14.4 pp134–135's accepted ordinary 4/5
LEFT8 relation. Expected colours and holes come from the frozen input pattern,
not a history tap. The observer proves actual registered compositor contribution.
All 55 nonpublic fields match across nine 8192-clock captures; Full5/6 and
raw-mode CSVs are byte-identical across versions. Full return matches all 8192
source/public RGB samples and earns fresh acquisition/READY receipts.

There is one executed cohort per source. An initial pre-DUT compile failure
from unused output-pin warnings is preserved; explicitly opening only those
outputs fixes the adapter without changing inputs or expectations. Exact commands
in the private evidence directory are:

```sh
bash build.sh old > old/build.log 2>&1
./old/obj/sprite_probe old/run inputs > old/run/output.log 2>&1
bash build.sh current > current/build.log 2>&1
./current/obj/sprite_probe current/run inputs > current/run/output.log 2>&1
python3 compare.py > comparison.log
```

Final current result: `asic-r3-sprite: PASS completeControls=6 mixed12bitSpriteRows=6 rawDirectComparisons=16384 FullReturnExact=8192 FrenchLEFT8=PASS`.
The parent reads the complete C++/top diffs and report and verifies 119 artifact
hashes and 30 compiled source pairs without repeating the DUT or global gates.
Evidence: `local/overnight-20261103/resumed-20261104/asic-r3-sprite-coverage-20261106/`.

## Review and retained limits

Opus 5.5/high run `20261106T034350Z-22195-5069` reads the production change,
fixture and prepared sprite proof. It checks exact eight-dot timing, classic
packing, post-compositor ownership, mode/reset/SNA clearing and producer/consumer
connections. It accepts RGB-history isolation across a live owner change by
inspection; this does not certify whole-machine live model switching. It runs
no tests/builds/devices and takes CI, timing and native captures from the
parent's verified receipts. Review and limits are preserved in
`asic-r3-opus-review-20261106/` under the same private evidence root.

Keep the public cross-module R3 fixture. The sprite probe remains a recorded
one-shot check under the test policy, as the reviewer recommends. Other sprite
magnifications/overlaps, moved-R2/JIT correction boundaries, full uninterrupted
demos and physical HDMI/CRT response remain outside this proof.

The Full correction is specifically qualified for ordinary 4/5 positioning;
5/6 are preserved anchors. It does not add the missing shorter-width-3
correction. Opus identifies that broader presentation gap by source inspection,
not a new native measurement or failing vector; no numeric error size is adopted
here. Document the limitation rather than altering CRTC/GA timing to hide it.
[Existing synthesis and native acceptance](asic-r3-rendered-position-2026-11-05.md).
