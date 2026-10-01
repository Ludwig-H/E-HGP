#!/usr/bin/env python3
"""One immutable capture, private temporary binaries, TEXT-only closed archive."""
import json
import os
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.dont_write_bytecode = True
import read as judge

ROOT = Path(__file__).resolve().parent

def sources():
    return {judge.ORIGIN+'/'+name: judge.sha(judge.ORIGIN+'/'+name) for name in judge.PINS}

def seeds():
    return {name: judge.sha(ROOT/name) for name in judge.SEED_FILES}

def write_new(name, value):
    with (ROOT/name).open('x', encoding='utf-8') as output:
        output.write(json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+'\n')

def run(argv, timeout, overrides):
    start = time.perf_counter()
    env = dict(os.environ, **overrides)
    code, out, err, timed_out = None, '', '', False
    try:
        process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True, env=env, start_new_session=True)
        try:
            out, err = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(process.pid, signal.SIGKILL)
            out, err = process.communicate()
        code = process.returncode
    except OSError as exc:
        err = str(exc)
    return {'argv': argv, 'timeout_s': timeout, 'env_overrides': overrides,
            'returncode': code, 'timed_out': timed_out, 'stdout': out, 'stderr': err,
            'elapsed_s': time.perf_counter()-start}

def main():
    judge.need(len(sys.argv) == 1, 'record.py takes no arguments')
    judge.need(not (ROOT/'manifest.json').exists(), 'already closed: manifest exists')
    expected = set(judge.SEED_FILES)
    actual = set()
    for path in ROOT.rglob('*'):
        judge.need(not path.is_symlink(), 'archive symlink')
        if path.is_file():
            actual.add(path.relative_to(ROOT).as_posix())
        else:
            judge.need(path.is_dir() and path.relative_to(ROOT).as_posix() in judge.DIRS, 'archive directory/special file')
    judge.need(actual == expected, 'fresh closed seed inventory required')
    p = judge.decode((ROOT/'protocol.json').read_text()); judge.protocol(p)
    before, archive_before = sources(), seeds()
    refs = {judge.ORIGIN+'/'+x: y for x, y in judge.PINS.items()}
    judge.need(before == refs, 'actor sources changed before capture')
    for name, digest in judge.PINS.items():
        if name != 'tower/tower.cpp':
            judge.need(archive_before['baseline/'+name] == digest, 'actor snapshot mismatch')
    version = run([p['compiler'], '--version'], 10, {'LC_ALL': 'C'})
    capture = {'schema': judge.SCHEMA, 'status': 'closed', 'archive_path': str(ROOT),
               'compiler': {'version': version, 'sha256': judge.sha(p['compiler'])}, 'cases': []}
    with tempfile.TemporaryDirectory(prefix='mhgp10-orient-runtime-', dir='/tmp') as runtime:
        capture['runtime_path'] = runtime
        for case in p['cases']:
            exe = runtime+'/'+case['name']
            inc = str(ROOT/'mutant' if case['mutant'] else ROOT/'baseline')
            argv = [p['compiler']]+p['common_flags']+(judge.UBFLAGS if case['ubsan'] else [])
            argv += ['-DORIENT_MUTANT='+str(case['mutant']), '-I'+inc, '-I'+str(ROOT/'baseline'),
                     str(ROOT/'probe.cpp'), '-o', exe]
            compile_result = run(argv, p['compile_timeout_s'], {'LC_ALL': 'C'})
            item = {'name': case['name'], 'compile': compile_result, 'binary_sha256': None, 'run': None}
            if compile_result['returncode'] == 0 and not compile_result['timed_out']:
                item['binary_sha256'] = judge.sha(exe)
                overrides = {'LC_ALL': 'C'}
                if case['ubsan']:
                    overrides['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
                item['run'] = run([exe], p['run_timeout_s'], overrides)
            capture['cases'].append(item)
    after, archive_after = sources(), seeds()
    write_new('captures.json', capture)
    write_new('source_close.json', {'schema': judge.SCHEMA, 'status': 'closed', 'before': before,
              'after': after, 'archive_before': archive_before, 'archive_after': archive_after})
    judge.need(archive_before == archive_after, 'archive seeds changed during capture; no manifest')
    manifest = {'schema': judge.SCHEMA, 'status': 'closed',
                'files': {name: judge.sha(ROOT/name) for name in judge.FILES}}
    write_new('manifest.json', manifest)
    digest = judge.sha(ROOT/'manifest.json')
    print('manifest_sha256 '+digest, flush=True)
    judge.check(ROOT, digest)
    print('capture_ok: baseline, mutant, baseline_ubsan; binaries omitted; not_claimed')
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, TypeError, KeyError) as exc:
        print('REFUS: '+str(exc), file=sys.stderr)
        sys.exit(1)
