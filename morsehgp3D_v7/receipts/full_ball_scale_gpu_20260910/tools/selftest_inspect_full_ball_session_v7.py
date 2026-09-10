#!/usr/bin/env python3
"""Pure inspector gates: never call GCP, SSH, NVCC or any subprocess."""
import copy
from contextlib import redirect_stdout
import importlib.util
import io
import json
from pathlib import Path
import shlex
import sys
from types import SimpleNamespace
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('inspect_full_ball_under_test', HERE / 'inspect_full_ball_session_v7.py')
INSPECT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(INSPECT)

checks = 0
rejects = 0


def need(ok, reason):
    global checks
    checks += 1
    if not ok:
        raise ValueError(reason)


def reject(call, reason):
    global rejects
    try:
        call()
    except ValueError:
        rejects += 1
    else:
        raise ValueError('mutant survived: ' + reason)


def forbidden(*args, **kwargs):
    raise RuntimeError('subprocess forbidden in pure test')


def fixture():
    session = Path('/private/session')
    generation = '2026-09-10T12:00:00.123456Z'
    handoff = dict(INSPECT.TARGET, schema='e-hgp.start-handoff.v3', status='targeted_running',
                   last_start_timestamp=generation)
    remote = '/tmp/ehgp-full-v7-0123456789abcdef.AbCdEf1234'
    worker = ['python3', remote + '/worker.py', '--source-root', remote + '/source',
              '--source-manifest', remote + '/source_manifest.json', '--source-manifest-sha256', 'a' * 64,
              '--guard-mark', remote + '/double_guard_verified', '--guard-mark-sha256', 'b' * 64,
              '--generation', generation, '--session-deadline-epoch', '1789043400.0',
              '--closing-margin-seconds', '300', '--output', remote + '/output']
    for key, value in INSPECT.TARGET.items():
        worker += ['--' + key, value]
    intent = dict(argv=[INSPECT.GCLOUD, 'compute', 'ssh', INSPECT.TARGET['instance'],
        '--project=' + INSPECT.TARGET['project'], '--zone=' + INSPECT.TARGET['zone'], '--quiet',
        '--ssh-key-file=' + str(session / 'session_key'), '--ssh-key-expiration=2026-09-10T13:00:00.000000Z',
        '--ssh-flag=-n', '--ssh-flag=-o BatchMode=yes', '--ssh-flag=-o ConnectTimeout=15',
        '--command=exec ' + shlex.join(worker)])
    return session, handoff, intent, worker


def read_program_gate():
    """Run the remote read program against memory-only fake files."""
    class File:
        def __init__(self, name, raw, symlink=False):
            self.name, self.raw, self.symlink = name, raw, symlink

        def __lt__(self, other):
            return self.name < other.name

        def is_file(self):
            return True

        def is_symlink(self):
            return self.symlink

        def stat(self):
            return SimpleNamespace(st_size=len(self.raw))

        def open(self, mode):
            need(mode == 'rb', 'remote read mode')
            return io.BytesIO(self.raw)

        def read_text(self):
            return self.raw.decode()

    files = [File('probe.stderr', b'noise\nstage_start=generate\nstage_complete=generate seconds=1\n'),
             File('complete.summary.json', b'{"status":"completed"}'),
             File('partial.summary.json', b'{'), File('skip.summary.json', b'{}', symlink=True)]
    directory = SimpleNamespace(is_symlink=lambda: False, is_dir=lambda: True, iterdir=lambda: files)
    output = io.StringIO()
    with patch('pathlib.Path', lambda value: directory), patch.object(sys, 'argv', ['reader', '/fixture/output']), redirect_stdout(output):
        exec(compile(INSPECT.READ, '<remote-read-pure-fixture>', 'exec'), {})
    result = json.loads(output.getvalue())
    need(result['summaries'] == {'complete.summary.json': {'status': 'completed'}} and
         result['summary_read_errors'] == {'partial.summary.json': 'JSONDecodeError'}, 'partial summary preserved as error')
    need(result['stages']['probe.stderr'][-1] == 'stage_complete=generate seconds=1' and
         'skip.summary.json' not in result['files'], 'stage read and symlink exclusion')


def main():
    if len(sys.argv) != 1:
        return 2
    INSPECT.subprocess.run = forbidden
    session, handoff, intent, worker = fixture()
    validate = lambda h=handoff, i=intent, p=session, mode=0o40700: INSPECT.validate_controller_records(p, mode, h, i)
    plan = validate()
    need(plan['generation'] == handoff['last_start_timestamp'] and plan['argv'] == intent['argv'] and
         plan['argv'] is not intent['argv'], 'nominal immutable plan')
    bootstrap = copy.deepcopy(intent); bootstrap['argv'][-1] += ' --bootstrap'
    need(validate(i=bootstrap)['output'] == plan['output'], 'recorded optional bootstrap is never executed')
    for field, value in (('project', 'other'), ('zone', 'other'), ('instance', 'other'), ('schema', 'old'),
                         ('status', 'targeted_stopped'), ('last_start_timestamp', None),
                         ('last_start_timestamp', 'yesterday'), ('last_start_timestamp', '2026-02-30T12:00:00Z')):
        reject(lambda field=field, value=value: validate(h=dict(handoff, **{field: value})), field)
    for bad in ({}, [], None):
        reject(lambda bad=bad: validate(h=bad), 'handoff shape')
    for bad in ({}, [], None, {'argv': []}, {'argv': intent['argv'][:7]}, {'argv': intent['argv'] + ['--extra']}):
        reject(lambda bad=bad: validate(i=bad), 'intent shape')
    for mode in (0o40755, 0o40777, 0o100700, 0o700, True):
        reject(lambda mode=mode: validate(mode=mode), 'session mode')
    for path in (Path('relative/session'), Path('/private/../session'), Path('/')):
        reject(lambda path=path: validate(p=path), 'session path')
    for index, value in ((0, '/tmp/gcloud'), (2, 'scp'), (3, 'other'), (4, '--project=other'),
                         (5, '--zone=other'), (6, '--no-quiet'), (7, '--ssh-key-file=/private/other'),
                         (8, '--ssh-key-expiration='), (8, '--ssh-key-expiration=2026-09-09T12:00:00Z'),
                         (9, '--ssh-flag=-t'), (10, '--ssh-flag=-o BatchMode=no'), (11, '--ssh-flag=-o ProxyCommand=x'),
                         (12, '--command=other'), (12, '--command=exec "unterminated'), (7, None)):
        changed = copy.deepcopy(intent); changed['argv'][index] = value
        reject(lambda changed=changed: validate(i=changed), 'SSH identity')
    for option, value in (('--output', '/tmp/ehgp-full-wrong/output'),
                          ('--output', '/tmp/ehgp-full-v7-0123456789abcdef.AbCdEf1234/subdir/output'),
                          ('--output', '/tmp/ehgp-full-v7-0123456789abcdef.AbCdEf1234/../output'),
                          ('--output', 'relative/output'), ('--source-root', '/tmp/other/source'),
                          ('--source-manifest', '/tmp/other/source_manifest.json'),
                          ('--guard-mark', '/tmp/other/double_guard_verified'), ('--generation', '2026-09-10T12:01:00Z'),
                          ('--project', 'other'), ('--zone', 'other'), ('--instance', 'other'),
                          ('--source-manifest-sha256', 'bad'), ('--guard-mark-sha256', 'G' * 64),
                          ('--session-deadline-epoch', 'NaN'), ('--session-deadline-epoch', '1'),
                          ('--closing-margin-seconds', '0')):
        remote = worker.copy(); remote[remote.index(option) + 1] = value
        changed = copy.deepcopy(intent); changed['argv'][-1] = '--command=exec ' + shlex.join(remote)
        reject(lambda changed=changed: validate(i=changed), 'worker binding')
    for remote in (worker[:-1], worker + ['--output', plan['output']], ['python3', '/tmp/other/worker.py', *worker[2:]],
                   [*worker[:2], '--output', *worker[3:]], ['sh', *worker[1:]]):
        changed = copy.deepcopy(intent); changed['argv'][-1] = '--command=exec ' + shlex.join(remote)
        reject(lambda changed=changed: validate(i=changed), 'worker options')
    state = dict(status='RUNNING', labels={'project': 'e-hgp'}, lastStartTimestamp=plan['generation'])
    INSPECT.validate_running_generation(state, plan['generation']); need(True, 'same running generation')
    for changed in (None, {}, dict(state, status='TERMINATED'), dict(state, labels={}), dict(state, labels=[]),
                    dict(state, lastStartTimestamp='2026-09-10T12:00:01Z')):
        reject(lambda changed=changed: INSPECT.validate_running_generation(changed, plan['generation']), 'observed generation')
    progress = dict(files={str(i): 1000000 for i in range(100)}, summaries={str(i): {'large': 'x' * 10000} for i in range(100)},
                    stages={str(i): ['stage_start=one', 'stage_complete=' + 'x' * 10000] for i in range(100)},
                    summary_read_errors={'partial.summary.json': 'JSONDecodeError'})
    receipt = dict(progress=progress, generation=plan['generation'])
    before = copy.deepcopy(receipt)
    lines = INSPECT.summary_lines(receipt, Path('/private/progress.json'))
    need(len(lines) == 6 and sum(map(len, lines)) < 1500 and receipt == before, 'bounded summary preserves receipt')
    need('summary_read_errors=1' in lines[1] and lines[-1].endswith('/private/progress.json'), 'summary points to full receipt')
    read_program_gate()
    need(rejects >= 60 and checks >= 5, 'nonvacuity')
    print(f'inspect_full_ball_selftest=passed checks={checks} rejects={rejects} subprocess_calls=0 GCP=not_used')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
