# CSL runner failure safety

Scope: host-only B4 phase 0 runner, based on `cc8e57814950caa0c7e41f1be75f5c2c9b413cfd`.
No RTL changes or physical-device calls are part of this verification.

## Required behavior

A CFG write may modify the remote file even when SSH reports failure or times out.
Restoration therefore becomes necessary before dispatching the mutating command,
not after hash verification. The original bytes must remain in a separate local
backup while upload staging is reused. Failed restoration must fail an otherwise
successful run and its persisted manifest; if the script already failed, that
primary error remains the exception presented to the caller, with cleanup failure
recorded alongside it.

A live `crtc_select` is a no-op only when the runner knows the loaded type and
it matches. Unknown initial type cannot be relabelled by metadata. Live
`disk_insert` is rejected because this runner has no supported live mounting
transport. Configuration/media in the existing reset fold still apply at core
load. The CTRL mapping remains RIGHTCTRL; only the incorrect claim that
LEFTCTRL is unmapped is corrected against `rtl/hid.sv`.

## Acceptance boundary

Offline fault injection establishes host recovery and reporting behavior. It
does not prove recovery through a real disconnected SSH session, active OSD
state, or unobserved keyboard mappings. The original B4 hardware-only key-map
residual remains open. Existing Phase 1 device acceptance is not extended by
these tests.

## Offline verification

`python3 scripts/hardware-loop/test_csl_runner.py` reproduced seven failures
before the fixes (67 tests; six corpus tests skipped because the user-owned
SHAKER bundle was absent). The same suite passed after the fixes: 67 tests,
six skips. Failure injection changes actual emulated CFG bytes, then checks
byte-identical recovery after a partial write, an SSH timeout, or a hash
mismatch. Separate checks cover failed restoration, the CLI exit status, and
preservation of a primary script error when restoration also fails. Existing
power-on fold and same-type CRTC no-op checks remain passing.

`python3 sim/select_tests.py --run` exited 0 with the final line
`select_tests: no simulation needed`. The selection ran once after the final
code edit. No RTL or synthesis gate is implicated.

Raw local logs are retained under the ignored checkout directory
`.fdc-scratch/csl-runner-safety-20260929/`: `fail-first-expanded.log`,
`focused-pass-final.log`, and `select-tests.log`. The committed tests are the
reproducible source; logs are not a hardware observation.

## Independent review

Opus 5.5 medium returned **CLEAR** in run `20260929T004043Z-66217-cccf`,
reviewing the final source and saved logs without rerunning the gate or contacting
a device. It closed all four findings, checked the reset fold and same-type
no-op, confirmed primary-error precedence and the separate original backup,
and verified both Ctrl mappings against `rtl/hid.sv`. The full review output is
retained beside the other raw logs as `opus-review.log`.

The review noted that when both script execution and restoration fail, the CLI
prints the primary script error; operators must consult `manifest.json` for
`cleanup.cfg_restore` and its local backup path. This preserves the requested
primary-error precedence. Real transport interruption and the source-derived
key-map entries remain hardware acceptance limits.

## Real SSH follow-up

The [controlled transport-interruption experiment](csl-real-ssh-recovery-2026-09-29.md)
now verifies recovery after a completed CFG write on the real MiSTer: the owned
SSH client was killed, the production runner reported failure, and fresh
connections restored the exact original bytes without emergency fallback.
Persistent network loss, partial writes on hardware, active OSD state and
key-map acceptance remain outside that result.
