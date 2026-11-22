#!/usr/bin/env python3
"""Exercise production-T80 routing against actual Git event histories."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

POLICY = Path(__file__).with_name('production-t80-policy.py').resolve()


class PolicyTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name)
        self.git('init', '-q')
        self.git('config', 'user.name', 'CI test')
        self.git('config', 'user.email', 'ci@example.invalid')
        (self.repo / 'seed').write_text('seed')
        self.git('add', '.')
        self.git('commit', '-qm', 'base')
        self.base = self.git('rev-parse', 'HEAD')
        self.revision = 0
        self.fake = tempfile.TemporaryDirectory()
        self.addCleanup(self.fake.cleanup)
        gh = Path(self.fake.name) / 'gh'
        gh.write_text("""#!/usr/bin/env python3
import json, os, sys
if os.environ.get('FAKE_GH_MODE') == 'error':
    sys.exit(1)
if os.environ.get('FAKE_GH_MODE') == 'invalid':
    print('not json')
    sys.exit(0)
args = sys.argv[1:]
assert args[:3] == ['api', '-X', 'GET']
assert args[3] == 'repos/example/project/actions/workflows/build.yml/runs'
assert 'head_sha=' + os.environ['FAKE_BASE'] in args
assert 'status=success' in args
mode = os.environ.get('FAKE_GH_MODE', 'success')
sha = '0' * 40 if mode == 'wrong-sha' else os.environ['FAKE_BASE']
conclusion = 'failure' if mode == 'failure' else 'success'
status = 'in_progress' if mode == 'in-progress' else 'completed'
print(json.dumps({'workflow_runs': [] if mode == 'empty' else [{'head_sha': sha, 'status': status, 'conclusion': conclusion}]}))
""")
        gh.chmod(0o755)

    def git(self, *args):
        return subprocess.check_output(['git', *args], cwd=self.repo, text=True).strip()

    def change(self, *paths):
        self.revision += 1
        for path in paths:
            p = self.repo / path
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(str(self.revision))
        self.git('add', '.')
        self.git('commit', '-qm', 'change')
        return self.git('rev-parse', 'HEAD')

    def scope(self, head, **overrides):
        env = os.environ | {'EVENT_NAME': 'push', 'GITHUB_REF': 'refs/heads/feature',
                            'GITHUB_SHA': head, 'BEFORE_SHA': self.base,
                            'PR_BASE_SHA': '', 'PR_HEAD_SHA': '',
                            'GITHUB_REPOSITORY': 'example/project', 'GH_TOKEN': 'fake',
                            'PATH': self.fake.name + os.pathsep + os.environ['PATH']} | overrides
        env['FAKE_BASE'] = env['PR_BASE_SHA'] if env['EVENT_NAME'] == 'pull_request' else env['BEFORE_SHA']
        return subprocess.check_output(['python3', str(POLICY), 'scope'], cwd=self.repo, env=env, text=True).strip()

    def test_base_workflow_success_is_required(self):
        head = self.change('scripts/hardware-loop/sna_pull.py')
        self.assertEqual(self.scope(head), 'snapshot')
        for mode in ('error', 'invalid', 'empty', 'wrong-sha', 'failure', 'in-progress'):
            with self.subTest(mode=mode):
                self.assertEqual(self.scope(head, FAKE_GH_MODE=mode), 'full')
        self.assertEqual(self.scope(head, GH_TOKEN=''), 'full')

    def test_host_dependencies(self):
        for path in ('sna_pull.py', 'driver.py', 'ssm_ring.py'):
            with self.subTest(path=path):
                head = self.change('scripts/hardware-loop/' + path)
                self.assertEqual(self.scope(head), 'snapshot')
                self.base = head

    def test_host_only_and_optional_prose(self):
        head = self.change('scripts/hardware-loop/test_sna_pull.py', 'scripts/hardware-loop/ssm_dt_trace.py', 'docs/notes.md')
        self.assertEqual(self.scope(head), 'none')

    def test_mixed_and_unknown_fail_closed(self):
        for path in ('scripts/diagnostics/a.py', 'scripts/hardware-loop/new.py',
                     'rtl/a.v', 'sim/a.py', 'sim/TESTS.md', 'unknown.md', '.github/workflows/build.yml', 'toolchain.txt'):
            with self.subTest(path=path):
                head = self.change('scripts/hardware-loop/test_driver.py', path)
                self.assertEqual(self.scope(head), 'full')
                self.base = head

    def test_rename_and_unusual_path_fail_closed(self):
        head = self.change('scripts/hardware-loop/test_driver.py')
        self.base = head
        self.git('mv', 'scripts/hardware-loop/test_driver.py', 'scripts/hardware-loop/new.py')
        self.git('commit', '-qm', 'rename')
        self.assertEqual(self.scope(self.git('rev-parse', 'HEAD')), 'full')
        self.base = self.git('rev-parse', 'HEAD')
        head = self.change('scripts/hardware-loop/test_driver.py', ' scripts/hardware-loop/test_driver.py')
        self.assertEqual(self.scope(head), 'full')

    def test_pr_uses_verified_merge_tree(self):
        branch = self.git('branch', '--show-current')
        self.git('checkout', '-qb', 'feature')
        head = self.change('scripts/hardware-loop/test_sna_pull.py')
        self.git('checkout', '-q', branch)
        base = self.change('base-file')
        self.git('merge', '--no-ff', '-qm', 'merge PR', head)
        merge = self.git('rev-parse', 'HEAD')
        args = dict(EVENT_NAME='pull_request', PR_BASE_SHA=base, PR_HEAD_SHA=head)
        self.assertEqual(self.scope(merge, **args), 'none')
        self.assertEqual(self.scope(merge, **(args | {'FAKE_GH_MODE': 'failure'})), 'full')
        self.assertEqual(self.scope(head, **args), 'full')
        self.assertEqual(self.scope(merge, **(args | {'PR_BASE_SHA': self.base})), 'full')
        self.assertEqual(self.scope(merge, **(args | {'PR_HEAD_SHA': self.base})), 'full')
        self.assertEqual(self.scope('0' * 40, **args), 'full')
        # A merge resolution can add content absent from the head branch.
        (self.repo / 'sim').mkdir()
        (self.repo / 'sim' / 'changed.v').write_text('merge resolution')
        self.git('add', '.')
        self.git('commit', '--amend', '-qm', 'merge with resolution')
        self.assertEqual(self.scope(self.git('rev-parse', 'HEAD'), **args), 'full')

    def test_untrusted_events_and_bases(self):
        head = self.change('scripts/hardware-loop/test_driver.py')
        for overrides in ({'EVENT_NAME': 'workflow_dispatch'}, {'GITHUB_REF': 'refs/tags/v1'},
                          {'BEFORE_SHA': '0' * 40}, {'BEFORE_SHA': 'bad'},
                          {'EVENT_NAME': 'pull_request', 'PR_BASE_SHA': 'bad', 'PR_HEAD_SHA': head}):
            with self.subTest(overrides=overrides):
                self.assertEqual(self.scope(head, **overrides), 'full')
        self.assertEqual(self.scope(self.base, BEFORE_SHA=head), 'full')
        self.assertEqual(self.scope(self.base, BEFORE_SHA=self.base), 'full')


    def test_workflow_gate_exact_result(self):
        workflow = (POLICY.parents[2] / '.github/workflows/build.yml').read_text()
        gate = workflow.split('# Production-T80 gate:', 1)[1].split('# End production-T80 gate.', 1)[0]
        gate = gate.split('\n', 1)[1]
        for scope in ('full', 'snapshot', 'none', '', 'invalid'):
            for result in ('success', 'skipped', 'failure', 'cancelled'):
                for policy in ('success', 'failure'):
                    expected = policy == 'success' and ((scope in ('full', 'snapshot') and result == 'success') or (scope == 'none' and result == 'skipped'))
                    env = os.environ | {'PRODUCTION_T80_POLICY_RESULT': policy,
                                        'PRODUCTION_T80_SCOPE': scope,
                                        'PRODUCTION_T80_RESULT': result}
                    p = subprocess.run(['bash', '-c', gate], env=env, capture_output=True)
                    self.assertEqual(p.returncode == 0, expected, (policy, scope, result))


if __name__ == '__main__':
    unittest.main()
