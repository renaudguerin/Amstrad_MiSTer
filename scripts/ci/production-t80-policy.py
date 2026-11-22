#!/usr/bin/env python3
"""Select the production-T80 suite only for audited host-only changes."""
import json
import os
import re
import subprocess
import sys

HOST = {f'scripts/hardware-loop/{name}'.encode() for name in (
    'driver.py', 'sna_pull.py', 'ssm_ring.py', 'ssm_dt_trace.py',
    'shaker_ssm_inventory.py', 'test_driver.py',
    'test_shaker_ssm_inventory.py', 'test_sna_pull.py',
)}
# B18 imports sna_pull, which imports both driver and ssm_ring.
SNAPSHOT = {f'scripts/hardware-loop/{name}'.encode() for name in
            ('sna_pull.py', 'driver.py', 'ssm_ring.py')}


def git(*args):
    return subprocess.check_output(['git', *args], stderr=subprocess.DEVNULL)


def commit(value):
    # Reject option-like/ref expressions and all-zero/missing event SHAs.
    if not re.fullmatch(r'[0-9a-fA-F]{40}', value) or not int(value, 16):
        raise ValueError('invalid commit SHA')
    return git('rev-parse', '--verify', value + '^{commit}').strip().decode()


def prose(path):
    return (path in (b'README.md', b'CREDITS.md') or
            (path.startswith(b'docs/') and path.endswith(b'.md')))


def verified_base(base):
    """Require a successful Build core run on this exact comparison commit."""
    repo = os.environ.get('GITHUB_REPOSITORY', '')
    if not os.environ.get('GH_TOKEN') or not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo):
        return False
    try:
        response = subprocess.check_output(
            ['gh', 'api', '-X', 'GET', f'repos/{repo}/actions/workflows/build.yml/runs',
             '-f', f'head_sha={base}', '-f', 'status=success', '-f', 'per_page=100'],
            stderr=subprocess.DEVNULL, timeout=15)
        runs = json.loads(response)['workflow_runs']
        return isinstance(runs, list) and any(
            isinstance(run, dict) and run.get('head_sha') == base and
            run.get('status') == 'completed' and run.get('conclusion') == 'success'
            for run in runs)
    except (OSError, subprocess.SubprocessError, ValueError, KeyError, TypeError):
        return False


def scope():
    env = os.environ
    event = env.get('EVENT_NAME', '')
    if event not in ('push', 'pull_request') or env.get('GITHUB_REF', '').startswith('refs/tags/'):
        return 'full'
    try:
        if event == 'push':
            head = commit(env.get('GITHUB_SHA', ''))
            base = commit(env.get('BEFORE_SHA', ''))
            subprocess.run(['git', 'merge-base', '--is-ancestor', base, head],
                           check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            head = commit(env.get('GITHUB_SHA', ''))
            base = commit(env.get('PR_BASE_SHA', ''))
            pr_head = commit(env.get('PR_HEAD_SHA', ''))
            parents = git('rev-list', '--parents', '-n', '1', head).decode().split()
            if parents != [head, base, pr_head]:
                return 'full'
        paths = git('diff', '--name-only', '--no-renames', '-z', base, head).split(b'\0')
        paths = [p for p in paths if p]
        host = set(paths) & HOST
        if not host or any(p not in HOST and not prose(p) for p in paths):
            return 'full'
        if not verified_base(base):
            return 'full'
        return 'snapshot' if host & SNAPSHOT else 'none'
    except (ValueError, subprocess.CalledProcessError, OSError):
        return 'full'


def main():
    if sys.argv[1:] == ['scope']:
        print(scope())
        return 0
    return 1


if __name__ == '__main__':
    sys.exit(main())
