"""Portable hash-first reader; two small Python oracle replays, no native calls."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
OLD = '/tmp/mhgp10-weighted-majority-proof-20260930.IEHqmmpr'
PYTHON = '/home/codespace/.python/current/bin/python3'
SOURCE = {
    'check.py': '4fa840d7cfd005506c8e3582d1e27c520814f7be43ada8d2fa31560344407d50',
    'PROTOCOL.txt': 'd77c8cc16876e2dd4843bd627ed9ea11a9a020cda47ed70412a07135956dad01',
    'record.py': '486d0e3227767f599e02ded2ccacce9669f1fd612dc3f4d147cd9c48545101ac',
}
FILES = set(SOURCE) | {'README.md', 'receipt.json', 'verify.py'} | {
    'logs/'+name+'.'+stream for name in ('normal', 'optimized') for stream in ('stdout', 'stderr')}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def inventory():
    pins = {}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        m = re.fullmatch(r'([0-9a-f]{64})  ([A-Za-z0-9_./-]+)', line)
        require(m is not None, 'manifest syntax')
        value, name = m.groups()
        require(name in FILES and name not in pins, 'manifest inventory')
        pins[name] = value
    require(set(pins) == FILES, 'manifest incomplete')
    actual = {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}
    require(actual == FILES | {'SHA256SUMS'}, 'actual inventory')
    for name, value in pins.items():
        require(sha(ROOT/name) == value, 'hash '+name)
    return pins


def main():
    pins = inventory()
    r = json.loads((ROOT/'receipt.json').read_text())
    require(r['schema'] == 'mhgp10.weighted_euler_majority.v1' and r['status'] == 'CAPTURED', 'schema/status')
    require(type(r['native_calls']) is int and r['native_calls'] == 0 and r['GCP_used'] is False, 'scope')
    require(r['sources_before'] == r['sources_after'] == SOURCE, 'capture sources')
    for name, value in SOURCE.items():
        require(sha(ROOT/name) == value, 'frozen source '+name)
    require(r['earlier_exploratory_runs'] == [dict(mode='normal', cases=390, final_source=False),
                                           dict(mode='optimized', cases=392, final_source=True)], 'exploratory scope')
    require([c['name'] for c in r['commands']] == ['normal', 'optimized'], 'command inventory')
    previous = -1
    outputs = []
    for c in r['commands']:
        flags = ['-B'] if c['name'] == 'normal' else ['-B', '-O']
        require(c['argv'] == [PYTHON, *flags, OLD+'/check.py'] and c['cwd'] == OLD, 'historical argv')
        require(type(c['code']) is int and c['code'] == 0 and c['stderr'] == '', 'exit/diagnostic')
        require(type(c['start_ns']) is int and type(c['end_ns']) is int
                and previous <= c['start_ns'] <= c['end_ns'], 'monotonic stamps')
        previous = c['end_ns']
        a, b = dt.datetime.fromisoformat(c['start_utc']), dt.datetime.fromisoformat(c['end_utc'])
        require(a.utcoffset() == b.utcoffset() == dt.timedelta(0) and a <= b, 'UTC stamps')
        for stream in ('stdout', 'stderr'):
            require((ROOT/('logs/'+c['name']+'.'+stream)).read_text() == c[stream], 'capture linkage')
        report = json.loads(c['stdout'])
        require(report['status'] == 'WEIGHTED_EULER_MAJORITY_PASS' and report['cases'] == 392
                and report['atoms'] == 7742 and report['permutation_checks'] == 1176
                and report['full_cohort_checks'] == 392 and report['lca_calls'] == 23226
                and report['rejected_inputs'] == 8 and report['native_calls'] == 0
                and report['GCP_used'] is False, 'result scope')
        require([m['mutant'] for m in report['mutants']] ==
                ['lower_median', 'arbitrary_pivot', 'ignore_activation', 'drop_ancestor_atoms'], 'mutant inventory')
        require([q['atoms'] for q in report['selection_only_growth']] == [8000, 16000, 32000], 'selection scope')
        replay = subprocess.run([sys.executable, *flags, str(ROOT/'check.py')], cwd=ROOT,
                                capture_output=True, text=True, check=False, timeout=20)
        require(replay.returncode == 0 and replay.stderr == '' and replay.stdout == c['stdout'], 'oracle replay')
        outputs.append(c['stdout'])
    require(outputs[0] == outputs[1], 'normal/optimized difference')
    require(inventory() == pins, 'archive changed')
    print(json.dumps(dict(status='WEIGHTED_MAJORITY_ARCHIVE_PASS', files=len(FILES),
                         python_oracle_replays=2, native_calls=0, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
