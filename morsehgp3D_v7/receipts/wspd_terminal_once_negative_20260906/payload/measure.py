#!/usr/bin/env python3
"""Separate explicit build/run for a single before/after front-only timing."""
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
from datetime import datetime, timezone

BASE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('terminal_once_common', BASE / 'capture_common.py')
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)
c.need(c.sha(BASE / 'capture_common.py') == '003e3e6c972d880e2848fd2fb2371e3e650e5397e273970665308903a8cf9ea3', 'common_pin')
c.BASE, c.ROOT, c.LOG = BASE, BASE.parents[1], BASE / 'measure_logs'


def build():
    c.LOG.mkdir()
    for arm in ('reference', 'candidate'):
        binary, dep = BASE / 'bin' / ('measure_' + arm), c.LOG / (arm + '.d')
        c.run(arm + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
              '-pthread', '-O2', '-I' + str(BASE / arm), '-MMD', '-MF', str(dep),
              str(BASE / 'front_measure.cpp'), '-o', str(binary)], compile_step=True)
        files = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
        c.need(str(BASE / arm / 'src/pipeline/generate.hpp') in files, 'generate_dependency')
        c.need(all(Path(p).is_relative_to(BASE / arm) or Path(p) == BASE / 'front_measure.cpp' for p in files), 'snapshot_escape')
        c.save(c.LOG / (arm + '.build.json'), dict(binary_sha256=c.sha(binary),
               dependencies={p: c.sha(p) for p in files}))


def run():
    c.CPU = 6
    captures = []
    for arm in ('reference', 'candidate'):
        binary = BASE / 'bin' / ('measure_' + arm)
        binding = json.loads((c.LOG / (arm + '.build.json')).read_text())
        c.need(c.sha(binary) == binding['binary_sha256'], 'binary_pin')
        c.need(all(c.sha(p) == v for p, v in binding['dependencies'].items()), 'dependency_pin')
        label = arm + '_measure'
        row = dict(argv=['taskset', '-c', '6', str(binary), '--measure'], cwd=str(c.ROOT),
                   cpu=6, started=datetime.now(timezone.utc).isoformat(),
                   timeout=None, imposed_rlimits=None, pid=None, exit_code=None, closed=False)
        c.save(c.LOG / (label + '.intent.json'), row)
        process = None
        try:
            with (c.LOG / (label + '.stdout')).open('xb') as out, (c.LOG / (label + '.stderr')).open('xb') as err:
                process = subprocess.Popen(row['argv'], cwd=c.ROOT, stdout=out, stderr=err,
                                           start_new_session=True)
                row['pid'] = process.pid
                print(label, 'PID=' + str(process.pid), row['started'], flush=True)
                try:
                    row['exit_code'] = process.wait()
                except BaseException:
                    row['interrupted'] = True
                    os.killpg(process.pid, signal.SIGKILL)
                    row['exit_code'] = process.wait()
                    raise
        finally:
            if process is not None:
                try:
                    os.killpg(process.pid, 0)
                    os.killpg(process.pid, signal.SIGKILL)
                    row['residual_group_killed'] = True
                except ProcessLookupError:
                    row['closed'] = True
            row['ended'] = datetime.now(timezone.utc).isoformat()
            row['streams'] = {suffix: dict(sha256=c.sha(c.LOG / (label + '.' + suffix)),
                                           bytes=(c.LOG / (label + '.' + suffix)).stat().st_size)
                              for suffix in ('stdout', 'stderr') if (c.LOG / (label + '.' + suffix)).exists()}
            c.save(c.LOG / (label + '.command.json'), row)
        print(label, 'exit=' + str(row['exit_code']), 'closed=' + str(row['closed']), flush=True)
        c.need(row['exit_code'] == 0 and row['closed'], 'measurement_process_failed')
        c.need(c.sha(binary) == binding['binary_sha256'] and
               all(c.sha(p) == v for p, v in binding['dependencies'].items()), 'compiled_bytes_changed')
        captures.append(json.loads((c.LOG / (arm + '_measure.stdout')).read_text()))
    a, b = captures
    clocks = [v.pop('front_ms') for v in captures]
    work = [v.pop('work') for v in captures]
    equality = json.dumps(a, sort_keys=True, separators=(',', ':')) == json.dumps(b, sort_keys=True, separators=(',', ':'))
    result = dict(status='completed' if equality else 'failed', public_status='not_claimed',
                  scope='single_before_after_front_component_only', cpu=6, literal_semantic_equality=equality,
                  reference_ms=clocks[0], candidate_ms=clocks[1], speedup=clocks[0] / clocks[1],
                  work=work, rectangle_count=len(a['semantic']['rectangles']),
                  semantic_sha256=__import__('hashlib').sha256(json.dumps(a, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                  stdout_sha256={arm: c.sha(c.LOG / (arm + '_measure.stdout')) for arm in ('reference', 'candidate')})
    c.save(c.LOG / 'result.json', result)
    print(json.dumps(result, sort_keys=True))
    c.need(equality, 'literal_front_changed')


if __name__ == '__main__':
    if sys.argv[1:] == ['--build']:
        build()
    elif sys.argv[1:] == ['--run']:
        run()
    else:
        print('inert: --build or separately authorized --run')
        sys.exit(2)
