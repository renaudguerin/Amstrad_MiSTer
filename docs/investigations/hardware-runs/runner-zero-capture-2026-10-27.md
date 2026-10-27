# CSL runner: zero-capture runs no longer report success (2026-10-27)

SHAKER C0-26 completed every command with zero captures, and `csl_runner.py`
recorded it as `status: success` and exit 0 (see
[the 2.7 B–E session report](shaker27-modules-b-e-2026-10-15.md) §3). A walk that requests captures
and verifies none now ends with `status: "no_captures"` and exit 2. Exit 2 is the code `main()`
already uses for a completed walk that failed on the device. Every non-dry manifest now
carries `completed_captures`.

The runner counts requests in `runner.capture_count`, which covers CSL `screenshot`, `--screenshot-at` and SSM
screenshot markers. A script that requests nothing still reports success, and dry runs are unchanged.

Limits: a run whose SSM detector never fires requests no capture and still reports success.
In that case `ssm_records` and `ssm_wait_unconsumed` are the evidence. A `--stop-at` placed before the first capture also stays a success.
`scripts/shaker-ref/compare_runs.py` prefers `success` manifests when choosing attempts, so
`no_captures` directories now rank lower, which is intended.

Proof: the new test `test_a_run_with_no_completed_capture_is_not_a_success` failed before the change
(`AssertionError: 'success' != 'no_captures'`). After it:
`python3 -m unittest discover -s scripts/hardware-loop -p 'test_csl_runner.py'` gives
`Ran 91 tests … OK`. Two tolerate-timeout tests now expect `no_captures`, because both complete
with zero verified captures. `test_ssm_ring.py`'s
`test_hard_reset_discards_pending_wait_events_from_the_previous_machine` fails identically
on unmodified `HEAD`; that failure predates this change and is unrelated.
