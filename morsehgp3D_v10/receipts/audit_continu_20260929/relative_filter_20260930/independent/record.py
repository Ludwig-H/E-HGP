"""Capture native pilot and independent exact judgments in this fresh build."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
import inputs

root = Path(__file__).resolve().parent
pilot = Path('/tmp/mhgp10-relative-sphere-filter-0930.gBKSarn2')
bins = {name: pilot / executable for name, executable in
        [('normal', 'normal_v2'), ('ubsan', 'ubsan_v2'),
         ('drop_rest', 'drop_rest'), ('fixed_margin', 'fixed_margin')]}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def store(name, content):
    with (root / name).open('x') as stream:
        stream.write(content)


def run(name, argv, stdin=None, env=None):
    completed = subprocess.run(argv, input=stdin, capture_output=True, text=True,
                               check=False, timeout=30, env=env)
    store(name+'.stdout', completed.stdout)
    store(name+'.stderr', completed.stderr)
    return dict(name=name, argv=list(map(str, argv)), exit_code=completed.returncode)


paths = [root/'inputs.py', root/'inputs_v1.py', root/'judge.py', root/'record.py',
         pilot/'relative_filter.cpp', *bins.values()]
before = {str(path): digest(path) for path in paths}
requests = inputs.panel()
store('requests.json', json.dumps(requests, sort_keys=True)+'\n')
batch = '\n'.join(map(inputs.line, requests))+'\n'
store('requests.stdin', batch)
commands = []
for name, binary in bins.items():
    commands.append(run(name, [str(binary)], stdin=batch,
                        env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}))
    for mode, options in [('normal', []), ('optimized', ['-O'])]:
        commands.append(run('judge_'+name+'_'+mode,
                            [sys.executable, '-B', *options, str(root/'judge.py'),
                             str(root/'requests.json'), str(root/(name+'.stdout'))]))
after = {str(path): digest(path) for path in paths}
expected = {name: (1 if name.startswith(('judge_drop_rest_', 'judge_fixed_margin_')) else 0)
            for name in (command['name'] for command in commands)}
status = 'PASS' if before == after and all(
    c['exit_code'] == expected[c['name']] for c in commands) else 'FAIL'
store('execution.json', json.dumps(dict(status=status, before=before, after=after,
      requests=len(requests), commands=commands, expected_exit_codes=expected,
      scope='isolated numerical filter; errors of two mutants expected', GCP_used=False),
      sort_keys=True, indent=2)+'\n')
print(json.dumps(dict(status=status, requests=len(requests), commands=commands), sort_keys=True))
raise SystemExit(0 if status == 'PASS' else 1)
