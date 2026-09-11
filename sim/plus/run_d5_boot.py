"""Require both original private firmware images to emit Ready, never disc missing."""
import hashlib
from pathlib import Path
import subprocess
import sys

binary, english, french = sys.argv[1:]
images = [
    (english, '3ce35dfccf79ee6bf8f990124aa4e0af1ce9753cbca03af8545abef21cf081ae', []),
    (french, 'ab241580c9b6a9fa9aeae94ca6847ea70dca386d305e38fc2ac03fce603360cf', ['--classic-rom']),
]
# Validate every input before starting simulation; never patch cartridge bytes.
for name, expected, _ in images:
    if hashlib.sha256(Path(name).read_bytes()).hexdigest() != expected:
        sys.exit(f'FAIL: unexpected cartridge SHA256: {name}')
failed = subprocess.run([binary, '--controls']).returncode != 0
for name, _, options in images:
    print(f'Boot regression: {name}', flush=True)
    for model_options in ([], ['--464']):
        failed |= subprocess.run([binary, name, *options, *model_options]).returncode != 0
sys.exit(int(failed))
