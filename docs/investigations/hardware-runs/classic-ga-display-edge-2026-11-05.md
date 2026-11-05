# Classic GA display-edge correction

The retained-byte experimental serializer moved fixed-mode border/ink transitions one colour refresh early. At LOAD, it paired the incoming byte's DISPEN with the outgoing byte's last pixel. This is G1 from the fresh Opus5.5 source review. The one-line correction selects the existing delayed DISPEN (`u1005`) at colour refresh. Mode acceptance, byte cursor, replay/epoch logic, palette sampling and blanking are unchanged.

## Failing-first proof

Eight deterministic public-input cases cover entry and exit in all four settled modes. Each changes DISPEN two master clocks after a real LOAD, then identifies the first upstream prefetch and the first actual subsequent LOAD. The test asserts that this first LOAD has the incoming display tag and valid byte; it cannot skip an inconvenient LOAD to find a matching tag. Seventeen distinct nonblack palette colours prevent border/pen aliasing.

The complete-byte rule comes from French ACCC v1.11 §9.1 p47 and §17.1 p177, with Mode3 interpretation from §9.3.4.3.4 p63. Expected publication uses the existing fixed-byte oracle: LOAD+8 master clocks in Mode2, LOAD+12 in the other modes. Neither LOAD nor that oracle changes in the correction. The uncorrected serializer publishes at LOAD+4: all eight cases fail, with 4/192 wrong samples in Mode2 and 8/192 otherwise. After the correction all eight have zero wrong samples; prefetch131637 and LOAD131645 are unchanged. No phase search or RGB-derived expectation is used.

`make -C rtl/GA40010 sna-lifecycle-test` passes80 cases: the existing8 settled-mode and64 freshness cases plus8 display-edge cases. `make -C sim classic-mode-boundary-test` passes all64 production-T80 controls (40 fixed,24 mixed/JIT). Final `python3 sim/select_tests.py --run` reports **PASS46 benches** after the last code edit. Root verified the three final source hashes and accepted the worker's recorded gate without repeating it.

## Review and acceptance bounds

Claude Opus5.5 high follow-up `20261105T062119Z-89082-5616` clears G1 at source level and accepts the corrected serializer for bounded experimental RBF use. The previous strong review checked queue/anchor/replay, reset/SNA lifecycle and16-bit identity lifetime without finding a counterexample. Optional identity simplification is deferred. The reviewer's sandbox prevented shell/hash commands; root independently checked the final source manifest and actual diff.

The fixed-mode DE proof does not cover DE coinciding with FORCE_BLANK, mode acceptance or palette writes. The existing retained-byte and64 production-T80 checks cover pixel ownership separately. No unique silicon pipeline, all-phase closure or full-demo correctness is claimed. Fresh full synthesis, timing and targeted MiSTer checks remain required. The earlier90f3b18 RBF remains held and must not be promoted.

Private immutable proof, baseline/candidate logs, final manifests and provider logs are in `local/overnight-20261103/resumed-20261104/ga-display-edge-proof/`. Final video SHA256: `306e258eaa8ad7f803f1535b615f7f986eb5e1ca60b4efb815b779be3fe05f27`.
