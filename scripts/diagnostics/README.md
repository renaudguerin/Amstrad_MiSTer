# Diagnostic fixtures

These small source fixtures exercise timing boundaries used by the public simulation benches. The classic B25 and Plus PA7 probes check CPU, Gate Array and ASIC interactions; the Plus hardware probes provide reproducible inputs for associated tests.

Run the relevant target through `python3 sim/select_tests.py --run` or consult [sim/TESTS.md](../../sim/TESTS.md) for a specific bench. A passing fixture verifies its stated boundary, not overall software compatibility.
