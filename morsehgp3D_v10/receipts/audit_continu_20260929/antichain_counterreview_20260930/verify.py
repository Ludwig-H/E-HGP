"""Portable Python proof replay. No native, compiler, or GPU invocation."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
RECEIPTS = ROOT.parents[1]
PACKET = RECEIPTS / 'audit_independant_20260930' / 'fixed_k_antichain'
REFERENCE = ROOT.parent / 'point_condensation_cover_r2_20260930' / 'reference' / 'hgp10_ref.py'
WANTED = {'README.md', 'verify.py', 'rejudge.py', 'record.py', 'receipt.json',
          'normal.stdout', 'normal.stderr', 'optimized.stdout', 'optimized.stderr',
          'geometry_default.py', 'stream_extrema.py', 'record_additional.py',
          'additional_receipt.json'}
WANTED |= {f'{stem}.{mode}.{stream}' for stem in ['geometry_default', 'stream_extrema']
           for mode in ['normal', 'optimized'] for stream in ['stdout', 'stderr']}


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


pins = {}
for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    require(name in WANTED and name not in pins, 'unexpected manifest member')
    require(sha(ROOT / name) == digest, 'hash mismatch: ' + name)
    pins[name] = digest
require(set(pins) == WANTED, 'incomplete inventory')
require(sha(PACKET / 'SHA256SUMS') ==
        '9ab1e991f360331a9bcfe91a84920bb1708d8370c44ac6a603fd90d161f1806e', 'wrong external archive')
dependencies = {PACKET / 'SHA256SUMS'}
for line in (PACKET / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    require(sha(PACKET / name) == digest, 'external dependency mismatch')
    dependencies.add(PACKET / name)
require(len(dependencies) == 38, 'external inventory')
for file, local, reference, scripts in [
        ('receipt.json', ['record.py', 'rejudge.py'], False, ['rejudge.py']),
        ('additional_receipt.json', ['record_additional.py', 'geometry_default.py',
                                     'stream_extrema.py'], True,
         ['geometry_default.py', 'stream_extrema.py'])]:
    r = json.loads((ROOT / file).read_text())
    paths = dependencies | {ROOT / n for n in local}
    if reference:
        paths.add(REFERENCE)
    expected = {str(p.relative_to(RECEIPTS)): sha(p) for p in paths}
    require(r['sources_before'] == r['sources_after'] == expected, 'capture input pins')
    require(r['native_invocations'] == r['sklearn_invocations'] == 0
            and r['GCP_used'] is False, 'capture scope')
    rows = r['commands']
    require(len(rows) == 2 * len(scripts), 'wrong command count')
    for row, (script, mode) in zip(rows, [(s, m) for s in scripts
                                          for m in ['normal', 'optimized']]):
        argv = ['python3', '-B', *(['-O'] if mode == 'optimized' else []), script]
        stem = mode if file == 'receipt.json' else script[:-3] + '.' + mode
        require(row['argv'] == argv and row['returncode'] == 0, 'wrong command/result')
        require(row['stdout'] == stem + '.stdout' and row['stderr'] == stem + '.stderr',
                'wrong captured streams')
        require(row['started_unix_ns'] > 0 and
                row['ended_unix_ns'] >= row['started_unix_ns'], 'capture times')
        require((ROOT / row['stderr']).read_bytes() == b'', 'nonempty captured stderr')
mode = 'optimized' if sys.flags.optimize else 'normal'
results = {}
for script in ['rejudge.py', 'geometry_default.py', 'stream_extrema.py']:
    argv = ['python3', '-B', *(['-O'] if mode == 'optimized' else []), script]
    r = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=30)
    stem = mode if script == 'rejudge.py' else script[:-3] + '.' + mode
    require(r.returncode == 0 and r.stderr == b'', 'proof replay failed')
    require(r.stdout == (ROOT / (stem + '.stdout')).read_bytes(), 'proof replay differs')
    results[script] = json.loads(r.stdout)
require(results['rejudge.py']['pair_height_checks'] == 434 and
        results['rejudge.py']['strict_pair_height_advances'] == 0, 'six-export scope')
require(results['geometry_default.py']['eta'] == '1/8' and
        results['geometry_default.py']['original_pair_height'] == '25' and
        results['geometry_default.py']['reduced_pair_height'] == '200/9', 'geometric witness')
require(results['stream_extrema.py']['counts'] == dict(contexts=482, bands=2892,
        arbitrary_cases=6227, different_roots=4086), 'structural panel')
require(all(sha(ROOT / n) == digest for n, digest in pins.items()), 'archive changed')
print(json.dumps(dict(status='ARCHIVE_PASS', manifest_entries=len(pins),
                      proof_script_invocations_now=3, native_invocations=0,
                      sklearn_invocations=0, GCP_used=False), sort_keys=True))
