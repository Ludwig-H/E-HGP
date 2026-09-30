#!/usr/bin/env python3
"""Strict portable R2 reader; writes nothing, only stdlib and abstract proof."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
ORIGINAL = '/tmp/mhgp10-pkappa-stream-proof-20260930.EqYu7zNG'
BASE = {'README.md', 'PROTOCOL.txt', 'check.py', 'SOURCE_PINS.json',
        'normal.output.txt', 'optimized.output.txt', 'receipt.json', 'verify.py'}
PAIR = {'PROTOCOL.txt', 'check.py'}
OWN = {'CLOSURE_R2.md', 'verify_r2.py', 'receipt.json'}
SCOPE = 'abstract_monotone_tree_only_rational_radii'


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def h(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def manifest(base, filename, allowed):
    got = {}
    for line in (base / filename).read_text().splitlines():
        sha, name = line.split('  ', 1)
        require(name in allowed and name not in got, 'manifest inventory: ' + filename)
        require(len(sha) == 64 and all(c in '0123456789abcdef' for c in sha), 'digest syntax')
        require(h(base / name) == sha, 'manifest digest: ' + name)
        got[name] = sha
    require(set(got) == allowed, 'manifest complete: ' + filename)
    return got


def original():
    pins_all = manifest(ROOT, 'SHA256SUMS', BASE)
    require({p.name for p in ROOT.iterdir() if p.is_file()} == BASE | {'SHA256SUMS'},
            'parent file inventory')
    pins = json.loads((ROOT / 'SOURCE_PINS.json').read_text())
    rec = json.loads((ROOT / 'receipt.json').read_text())
    require(set(pins['before_runs']) == PAIR and set(rec['source_after_runs']) == PAIR,
            'source pin exact inventory')
    require(pins['prepared_before_execution'] is True, 'initial pre-execution pin')
    pair = {name: pins_all[name] for name in PAIR}
    require(pins['before_runs'] == pair == rec['source_after_runs'], 'source pin values')
    exp = (ROOT / 'normal.output.txt').read_bytes()
    require(exp == (ROOT / 'optimized.output.txt').read_bytes(), 'original captures unequal')
    data = json.loads(exp)
    require(data['schema'] == 'mhgp10.pkappa_stream.abstract.v1' and data['status'] == 'PASS',
            'proof capture schema/status')
    require(data['scope'] == SCOPE and data['engine_calls'] == 0 and data['gcp'] is False,
            'proof scope')
    argv = [['python3', '-B', ORIGINAL + '/check.py'],
            ['python3', '-B', '-O', ORIGINAL + '/check.py']]
    require(rec['schema'] == 'mhgp10.pkappa_stream.receipt.v1' and rec['scope'] == SCOPE,
            'initial receipt schema/scope')
    require(rec['commands'] == argv and rec['commands_exit_codes'] == [0, 0], 'initial argv/code')
    require(rec['capture_stream'] == 'exec_command combined output; each is one JSON line',
            'initial capture stream scope')
    require(rec['counts'] == data['counts'] and rec['record_sha256'] == data['record_sha256']
            and rec['mutants'] == data['mutants'], 'initial receipt/capture linkage')
    require(rec['outputs_byte_identical'] is True and rec['performance_timing_claim'] is False,
            'initial receipt claims')
    require(rec['engine_calls'] == 0 and rec['gcp'] is False and rec['failures'] == [],
            'initial receipt execution scope')
    return {'manifest': h(ROOT / 'SHA256SUMS'), 'all': pins_all, 'source_pair': pair}, exp, data


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def execute(expected):
    captures = []
    for flags in (['-B'], ['-B', '-O']):
        argv = [sys.executable, *flags, str(ROOT / 'check.py')]
        start, start_ns = utc(), time.monotonic_ns()
        run = subprocess.run(argv, capture_output=True, check=False, timeout=60)
        end_ns, end = time.monotonic_ns(), utc()
        require(run.returncode == 0 and run.stderr == b'' and run.stdout == expected,
                'new proof run code/stdout/stderr')
        captures.append({'argv': argv, 'code': run.returncode, 'start_utc': start, 'end_utc': end,
                         'start_monotonic_ns': start_ns, 'end_monotonic_ns': end_ns,
                         'stdout': run.stdout.decode(), 'stderr': run.stderr.decode(),
                         'stdout_sha256': hashlib.sha256(run.stdout).hexdigest(),
                         'stderr_sha256': hashlib.sha256(run.stderr).hexdigest()})
    return captures


def capture():
    before, expected, data = original()
    reader = {name: h(HERE / name) for name in {'verify_r2.py', 'CLOSURE_R2.md'}}
    captures = execute(expected)
    after, expected_after, _data = original()
    reader_after = {name: h(HERE / name) for name in reader}
    require(before == after and expected_after == expected and reader == reader_after, 'capture pins changed')
    return {'schema': 'mhgp10.pkappa_stream.reader_r2.v1', 'scope': SCOPE,
            'source_root_at_capture': str(ROOT), 'python_executable': sys.executable,
            'original_before': before, 'original_after': after,
            'reader_before': reader, 'reader_after': reader_after,
            'captures': captures, 'counts': data['counts'], 'record_sha256': data['record_sha256'],
            'mutants': data['mutants'], 'engine_calls': 0, 'gcp': False}


def check_capture(rec, pins, expected, data):
    require(rec['schema'] == 'mhgp10.pkappa_stream.reader_r2.v1' and rec['scope'] == SCOPE,
            'R2 capture schema/scope')
    require(rec['original_before'] == pins == rec['original_after'], 'R2 original pins')
    reader_files = {'verify_r2.py', 'CLOSURE_R2.md'}
    require(set(rec['reader_before']) == reader_files == set(rec['reader_after']), 'R2 reader inventory')
    require(rec['reader_before'] == rec['reader_after']
            == {name: h(HERE / name) for name in reader_files}, 'R2 reader pins')
    require(rec['counts'] == data['counts'] and rec['record_sha256'] == data['record_sha256']
            and rec['mutants'] == data['mutants'], 'R2 capture/proof linkage')
    require(rec['engine_calls'] == 0 and rec['gcp'] is False, 'R2 execution scope')
    source = Path(rec['source_root_at_capture'])
    require(source.is_absolute() and source.name == ROOT.name, 'R2 declared historical root')
    executable = rec['python_executable']
    require(Path(executable).is_absolute(), 'R2 declared executable')
    require(len(rec['captures']) == 2, 'R2 command inventory')
    previous_ns, previous_utc = None, None
    for cap, flags in zip(rec['captures'], (['-B'], ['-B', '-O'])):
        require(cap['argv'] == [executable, *flags, str(source / 'check.py')] and cap['code'] == 0,
                'R2 argv/code')
        start, end = dt.datetime.fromisoformat(cap['start_utc']), dt.datetime.fromisoformat(cap['end_utc'])
        require(start.utcoffset() == end.utcoffset() == dt.timedelta(0) and start <= end, 'R2 UTC timestamps')
        s, e = cap['start_monotonic_ns'], cap['end_monotonic_ns']
        require(type(s) is int and type(e) is int and 0 <= s <= e, 'R2 monotonic timestamps')
        require(previous_ns is None or (previous_ns <= s and previous_utc <= start), 'R2 run ordering')
        previous_ns, previous_utc = e, end
        require(cap['stdout'].encode() == expected and cap['stderr'] == '', 'R2 stdout/stderr')
        require(cap['stdout_sha256'] == hashlib.sha256(expected).hexdigest()
                and cap['stderr_sha256'] == hashlib.sha256(b'').hexdigest(), 'R2 capture hashes')


def main():
    if sys.argv[1:] == ['--capture']:
        print(json.dumps(capture(), sort_keys=True, indent=2))
        return
    require(sys.argv[1:] == [], 'reader arguments')
    allowed = OWN | {'../SHA256SUMS'}
    before_r2 = manifest(HERE, 'SHA256SUMS_R2', allowed)
    require({p.name for p in HERE.iterdir()} == OWN | {'SHA256SUMS_R2'}, 'R2 exact inventory')
    before, expected, data = original()
    rec = json.loads((HERE / 'receipt.json').read_text())
    check_capture(rec, before, expected, data)
    execute(expected)
    after, _expected, _data = original()
    require(before == after and before_r2 == manifest(HERE, 'SHA256SUMS_R2', allowed), 'reader closure changed')
    print(json.dumps({'status': 'ARCHIVE_R2_PASS', 'original_manifest_entries': len(BASE),
                      'r2_manifest_entries': len(allowed), 'proof_invocations_now': 2,
                      'counts': data['counts'], 'engine_calls_now': 0, 'gcp': False}, sort_keys=True))


if __name__ == '__main__':
    main()
