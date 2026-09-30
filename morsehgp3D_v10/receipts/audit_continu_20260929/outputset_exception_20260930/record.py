"""Bounded helper-only execution; preserve commands, streams and source pins."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
ORIGIN = Path('/tmp/mhgp10-audit-geant/verif_raccord_r2/repo')
SOURCES = ['probe.cpp', 'record.py'] + [str(p.relative_to(ROOT)) for p in sorted((ROOT / 'source').rglob('*')) if p.is_file()]

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def run(name, argv):
    start = datetime.datetime.now(datetime.timezone.utc).isoformat()
    p = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=30)
    (ROOT / (name + '.stdout')).write_bytes(p.stdout)
    (ROOT / (name + '.stderr')).write_bytes(p.stderr)
    return {'name': name, 'argv': argv, 'cwd': str(ROOT), 'started_utc': start,
            'finished_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'rc': p.returncode,
            'stdout_sha256': sha(ROOT / (name + '.stdout')), 'stderr_sha256': sha(ROOT / (name + '.stderr'))}

before = {p: sha(ROOT / p) for p in SOURCES}
origin_before = {str(p.relative_to(ROOT / 'source')): sha(ORIGIN / 'morsehgp3D_v10/src' / p.relative_to(ROOT / 'source'))
                 for p in (ROOT / 'source').rglob('*') if p.is_file()}
commands = [run('compiler', ['g++', '--version'])]
for mode, flags in [('normal', ['-O2']), ('ubsan', ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=undefined'])]:
    commands.append(run('compile_' + mode, ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror'] + flags +
                        ['-Isource', 'probe.cpp', '-o', str(ROOT / ('probe_' + mode))]))
    if commands[-1]['rc'] == 0:
        (ROOT / mode).mkdir()
        commands.append(run('run_' + mode, [str(ROOT / ('probe_' + mode)), str(ROOT / mode)]))
after = {p: sha(ROOT / p) for p in SOURCES}
origin_after = {p: sha(ORIGIN / 'morsehgp3D_v10/src' / p) for p in origin_before}
binary_sha256 = {p.name: sha(p) for p in ROOT.glob('probe_*') if p.is_file()}
receipt = {'kind': 'isolated_outputset_exception_audit', 'origin': str(ORIGIN),
           'origin_head': subprocess.run(['git', '-C', str(ORIGIN), 'rev-parse', 'HEAD'], capture_output=True, text=True, check=True).stdout.strip(),
           'source_before': before, 'source_after': after, 'origin_before': origin_before, 'origin_after': origin_after,
           'binary_sha256': binary_sha256, 'commands': commands,
           'scope': 'Frozen helper only; no engine/GCP; gates require observed faults, not a repaired implementation.'}
(ROOT / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
if before != after or origin_before != origin_after or len(commands) != 5 or any(c['rc'] != 0 for c in commands):
    sys.exit(1)
print('outputset_exception_audit_observed')
