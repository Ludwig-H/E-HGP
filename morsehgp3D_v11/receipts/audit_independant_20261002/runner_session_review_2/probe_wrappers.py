#!/usr/bin/env python3
"""Controls of copied wrappers only: no GCP call, compiler, CMake configure/build, CTest or product binary."""
import argparse
import ctypes
import importlib.util
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time


def load(path):
    spec = importlib.util.spec_from_file_location('audit_g4_matrix', path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--result', type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=False)
    source = args.source.resolve()
    records, checks = [], []

    def check(name, ok, value):
        checks.append({'name': name, 'ok': bool(ok), 'value': value})

    def run(name, argv):
        done = subprocess.run(argv, capture_output=True, text=True, timeout=10)
        records.append({'name': name, 'argv': list(map(str, argv)), 'code': done.returncode,
                        'stdout': done.stdout, 'stderr': done.stderr})
        return done

    wrapper = source / 'morsehgp3D_v11/cmake/run_expect.cmake'
    program = args.work / 'bad_interpreter'
    program.write_text('#!' + str(args.work / 'NO_SUCH_INTERPRETER') + '\nexit 0\n')
    program.chmod(0o755)
    cmake = shutil.which('cmake')
    issue = run('existing_executable_missing_interpreter', [cmake, '-DCMD=' + str(program),
                '-DEXPECTED=0', '-DNARGS=0', '-P', str(wrapper)])
    check('missing_interpreter_is_now_launch_error', issue.returncode != 0 and
          'run_expect_verdict lancement_impossible' in issue.stdout and
          'run_expect_verdict arret_anormal' not in issue.stdout, issue.stdout + issue.stderr)
    ordinary = run('ordinary_code_0', [cmake, '-DCMD=' + sys.executable, '-DEXPECTED=0', '-DNARGS=2',
                   '-DARG0=-c', '-DARG1=pass', '-P', str(wrapper)])
    check('ordinary_code_preserved', ordinary.returncode == 0, ordinary.stdout + ordinary.stderr)
    real_signal = run('genuine_signal', [cmake, '-DCMD=' + sys.executable, '-DEXPECTED=0', '-DNARGS=2',
                      '-DARG0=-c', '-DARG1=import os,signal; os.kill(os.getpid(),signal.SIGTERM)',
                      '-P', str(wrapper)])
    check('genuine_signal_stays_distinct', real_signal.returncode != 0 and
          'run_expect_verdict arret_anormal' in real_signal.stdout, real_signal.stdout + real_signal.stderr)

    matrix = load(source / 'morsehgp3D_v11/tools/g4_matrix.py')
    config = matrix.validate_configuration({'name': 'one', 'build': False, 'require_labels': ['unit']})
    selected = [{'name': 'mhgp11_a', 'labels': ['unit'], 'disabled': False}]
    step = {'status': 'ok', 'exit_code': 0}
    for name, ports, stdout, expected in [
        ('empty', [], '100% tests passed, 0 tests failed out of 0\n', 'vacuous'),
        ('skipped', selected, '1/1 Test #1: mhgp11_a ...........***Skipped 0.00 sec\n'
                            '100% tests passed, 0 tests failed out of 1\n', 'vacuous'),
        ('passed', selected, '1/1 Test #1: mhgp11_a ........... Passed 0.01 sec\n'
                           '100% tests passed, 0 tests failed out of 1\n', 'ok')]:
        verdict = matrix.judge_tests(ports, step, stdout, None, '', matrix.LIMITS, config, False)
        check('judge_' + name, verdict['status'] == expected, verdict)

    # Real Steps.run, harmless Python parent and child; make this audit a Linux subreaper so cleanup reaps its child.
    libc = ctypes.CDLL(None)
    if libc.prctl(36, 1, 0, 0, 0) != 0:
        raise RuntimeError('cannot establish audit-owned child cleanup')
    log = args.work / 'background_parent.log'
    code = 'import subprocess,sys; p=subprocess.Popen([sys.executable,"-c","import time; time.sleep(5)"]); print(p.pid,flush=True)'
    steps = matrix.Steps(time.monotonic() + 10)
    row = steps.run('audit_background_parent', [sys.executable, '-c', code], args.work, dict(os.environ), log, 3)
    child = int(log.read_text().strip())
    try:
        state = Path('/proc/' + str(child) + '/stat').read_text().rsplit(')', 1)[1].split()[0]
        check('successful_step_leaves_live_descendant', row['status'] == 'ok' and state not in ('Z', 'X'),
              {'step': row, 'child_pid': child, 'child_state_after_return': state})
    finally:
        os.kill(child, signal.SIGTERM)
        os.waitpid(child, 0)
    records.append({'name': 'Steps.run_background_control', 'step': row, 'log': log.read_text(),
                    'audit_cleanup': 'SIGTERM + waitpid of audit-owned child, no other process touched'})

    # Exercise the actual final decision of main with configuration execution replaced by a declared test double.
    # No product command runs. The measuring probe is interrupted by an actual SIGTERM delivered to this audit.
    synthetic = args.work / 'synthetic'
    (synthetic / 'morsehgp3D_v11').mkdir(parents=True)
    (synthetic / 'morsehgp3D_v11/CMakeLists.txt').write_text('# never configured or built\n')
    matrix_file = args.work / 'matrix.json'
    matrix_file.write_text(json.dumps({'schema': matrix.MATRIX_SCHEMA, 'source_dir': 'morsehgp3D_v11',
        'budget_seconds': 60, 'configurations': [{'name': 'one', 'build': False,
        'probes': [{'name': 'measure', 'executable': 'mhgp11_measure'}]}]}))
    matrix.first_line = lambda argv: 'audit test double, no tool executed'
    matrix.ctest_version = lambda: (3, 22)

    def passed_configuration(config, context, threads):
        (context.out / config['name']).mkdir()
        return {'name': config['name'], 'status': 'ok', 'conforming': True, 'optional': False,
                'threads': threads, 'steps': [{'name': 'synthetic_success'}], 'seconds': 0,
                'tests': {'passed': 1, 'selected': 1}, 'reason': ''}

    def interrupted_probe(config, result, context, threads):
        os.kill(os.getpid(), signal.SIGTERM)
        return [{'name': 'measure', 'status': 'interrupted', 'exit_code': -signal.SIGTERM}]

    matrix.run_configuration = passed_configuration
    matrix.run_probes = interrupted_probe
    original_handlers = {s: signal.getsignal(s) for s in [signal.SIGTERM, signal.SIGINT, signal.SIGHUP]}
    try:
        returned = matrix.main(['--src', str(synthetic), '--out', str(args.work / 'matrix_output'),
                    '--work', str(args.work / 'matrix_work'), '--matrix', str(matrix_file), '--threads', '1'])
    finally:
        for s, handler in original_handlers.items():
            signal.signal(s, handler)
    summary = json.loads((args.work / 'matrix_output/matrix/summary.json').read_text())
    check('signal_during_probe_still_green', returned == 0 and summary['signals'] == [signal.SIGTERM] and
          summary['conforming'] and summary['complete'], {'main_return': returned, 'summary': summary})
    records.append({'name': 'main_signal_control', 'scope': 'configuration and measuring probe are declared test doubles; actual main, scheduler, signal handler and final summary/exit rule are executed', 'main_return': returned, 'summary': summary})
    payload = {'schema': 'mhgp11.audit.runner_session_controls.v1', 'optimized': sys.flags.optimize,
               'source': str(source), 'scope': __doc__, 'checks': checks, 'commands': records}
    args.result.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + '\n')
    bad = [c['name'] for c in checks if not c['ok']]
    print(json.dumps({'checks': len(checks), 'failed': bad, 'optimized': sys.flags.optimize}))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
