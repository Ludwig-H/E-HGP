"""Fresh tiny builds in /tmp; writes only this new receipt."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

BASE = Path(__file__).resolve().parent
REPO = BASE.parents[4]
before = json.loads((BASE / 'SOURCE_BEFORE.json').read_text())
build = Path(tempfile.mkdtemp(prefix='mhgp10-wide-order-followup-', dir='/tmp'))
commands = []


def call(name, argv, env=None):
    result = subprocess.run(argv, cwd=BASE, capture_output=True, env=env)
    (BASE / (name+'.stdout')).write_bytes(result.stdout)
    (BASE / (name+'.stderr')).write_bytes(result.stderr)
    commands.append(dict(name=name, argv=argv, shell_display=shlex.join(argv), cwd=str(BASE),
                         env_overrides=({'UBSAN_OPTIONS': env['UBSAN_OPTIONS']} if env else {}),
                         exit_code=result.returncode))
    (BASE / 'COMMANDS.json').write_text(json.dumps(commands, indent=2)+'\n')
    if result.returncode:
        raise RuntimeError(name+' failed; stderr retained')
    return result.stdout


call('compiler', ['g++', '--version'])
common = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wconversion', '-Wshadow',
          '-I', str(BASE/'source/src'), str(BASE/'check.cpp')]
normal = build/'normal'
ubsan = build/'ubsan'
call('compile_normal', common+['-O2', '-o', str(normal)])
call('compile_ubsan', common+['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
                            '-o', str(ubsan)])
out = call('normal', [str(normal)])
sanenv = os.environ.copy()
sanenv['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
sanout = call('ubsan', [str(ubsan)], sanenv)
if out != sanout:
    raise RuntimeError('normal/UBSan output mismatch')
for name in ('normal', 'ubsan'):
    call('judge_'+name, ['python3', '-B', str(BASE/'judge.py'), str(BASE/(name+'.stdout'))])
    call('judge_'+name+'_O', ['python3', '-B', '-O', str(BASE/'judge.py'), str(BASE/(name+'.stdout'))])
after = dict(commit=before['commit'], method=before['method'], sources=[])
for row in before['sources']:
    actual = (BASE/row['receipt_path']).read_bytes()
    gitblob = subprocess.check_output(['git', 'show', before['commit']+':'+row['git_path']], cwd=REPO)
    digest = hashlib.sha256(actual).hexdigest()
    if actual != gitblob or digest != row['sha256']:
        raise RuntimeError('source closure mismatch: '+row['git_path'])
    after['sources'].append(dict(row, stable=True))
after['head_at_closure'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip()
(BASE/'SOURCE_AFTER.json').write_text(json.dumps(after, indent=2)+'\n')
summary = dict(status='PASS', source_commit=before['commit'], source_hashes_stable=True,
               build_directory=str(build), native_invocations=2, geometric_pairs_per_run=2,
               threshold_bridges_per_run=1, normal_ubsan_identical=True, python_judgments=4,
               full_build=False, native_wide_level_constructors=False, FULL=False, GCP=False,
               binary_hashes={name:hashlib.sha256(path.read_bytes()).hexdigest()
                              for name, path in [('normal',normal),('ubsan',ubsan)]})
(BASE/'RESULT.json').write_text(json.dumps(summary, indent=2)+'\n')
print(json.dumps(summary, sort_keys=True))
