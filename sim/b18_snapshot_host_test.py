#!/usr/bin/env python3
"""Exercise the real host puller against publication samples from B18 RTL."""
import base64
from pathlib import Path
import sys
from types import SimpleNamespace
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts/hardware-loop'))
from sna_pull import pull_snapshot, TornReadError, NoSnapshotError

class ReplayTransport:
    def __init__(self, reads):
        self.reads = iter(reads)
    def run_cmd(self, command, timeout):
        # This transport only substitutes physical memory access. All parsing,
        # coherence/retry and payload validation execute in production code.
        return SimpleNamespace(exit_code=0, stdout=base64.b64encode(next(self.reads)).decode(), stderr='')

for model in range(3):
    for kind in range(2):
        stem = Path(f'obj_dir/b18_snapshot/model{model}-type{kind}')
        before, torn, final = [Path(str(stem) + '-' + s + '.bin').read_bytes()
                               for s in ('before', 'torn', 'final')]
        try:
            pull_snapshot(ReplayTransport([before[:16], torn, final[:16]]), max_retries=1)
            raise AssertionError('accepted read overlapping publication')
        except TornReadError:
            pass
        invalid = bytes([255])*8 + before[8:16]
        try:
            pull_snapshot(ReplayTransport([invalid]), max_retries=1)
            raise AssertionError('accepted in-progress publication')
        except NoSnapshotError:
            pass
        hdr, payload = pull_snapshot(ReplayTransport([
            before[:16], torn, final[:16], final[:16], final, final[:16]
        ]), max_retries=2, retry_delay=0)
        assert hdr.generation == 1
        assert payload == final[16:16+hdr.file_bytes]
print('B18 host publication: PASS 6 torn-read/retry cases')
