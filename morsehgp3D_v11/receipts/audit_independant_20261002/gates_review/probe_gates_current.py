#!/usr/bin/env python3
"""Small causal gate controls, no C++/GPU/cloud; run against the recorded source copy."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--result', type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=False)
    source = args.source.resolve()
    records = []
    checks = []

    def check(name, condition, details):
        checks.append({'name': name, 'ok': bool(condition), 'details': details})

    def call(name, command, timeout=30):
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
        done = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              text=True, env=env, timeout=timeout)
        records.append({'name': name, 'argv': list(map(str, command)), 'code': done.returncode,
                        'stdout': done.stdout, 'stderr': done.stderr})
        return done

    cmake = shutil.which('cmake')
    ctest = shutil.which('ctest')
    fixture = args.work / 'fixture'
    fixture.mkdir()
    shutil.copytree(source / 'cmake', fixture / 'cmake')
    (fixture / 'tests').mkdir()
    (fixture / 'tests' / 'style_probe.py').write_text('print("style_probe_ok")\n')
    program_line = 'set(PROGRAM_GOOD "' + sys.executable + '")'
    cmake_text = ('cmake_minimum_required(VERSION 3.20)\n'
                  'project(gate_probe LANGUAGES NONE)\n'
                  'enable_testing()\n'
                  'set(Python3_EXECUTABLE "' + sys.executable + '")\n' + program_line + '\n'
                  'include(cmake/gates.cmake)\n'
                  'mhgp11_expect_code(mhgp11_target 0 "${PROGRAM_GOOD}" -c "pass" LABELS fast)\n'
                  'mhgp11_python_gate(mhgp11_style 0 tests/style_probe.py LABELS fast)\n'
                  'mhgp11_check_gate_registry()\n')
    (fixture / 'CMakeLists.txt').write_text(cmake_text)
    manifest = {'module': 'essai', 'plancher': 1, 'mutants': [{
        'id': 'commande_absente', 'fichier': 'CMakeLists.txt', 'cherche': program_line,
        'remplace': 'set(PROGRAM_GOOD "' + str(args.work / 'NO_SUCH_PROGRAM') + '")',
        'porte': 'mhgp11_target'}]}
    manifest_path = args.work / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    manifest_sha = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
    source_sha = hashlib.sha256((fixture / 'CMakeLists.txt').read_bytes()).hexdigest()
    campaign = call('campaign_missing_command', [sys.executable, str(source / 'tests/mutants/run_mutants.py'),
                    '--source', str(fixture), '--manifest', str(manifest_path),
                    '--work', str(args.work / 'campaign'), '--jobs', '1', '--build-jobs', '1', '--keep'])
    check('campaign_false_green_on_missing_command', campaign.returncode == 0 and
          'TUE' in campaign.stdout and 'dont_signal=1' in campaign.stdout and
          'mutants_ok' in campaign.stdout, {'code': campaign.returncode, 'stdout': campaign.stdout})
    for name in ('temoin_0', 'commande_absente'):
        log = args.work / 'campaign' / name / 'build' / 'Testing' / 'Temporary' / 'LastTest.log'
        records.append({'name': name + '_LastTest', 'text': log.read_text()})
    check('fixture_original_unchanged', source_sha == hashlib.sha256(
        (fixture / 'CMakeLists.txt').read_bytes()).hexdigest(), source_sha)

    build = args.work / 'inventory_build'
    configured = call('fixture_configure_no_languages', [cmake, '-S', str(fixture), '-B', str(build)])
    check('fixture_configured', configured.returncode == 0, configured.stdout)
    full = call('inventory_all', [ctest, '--test-dir', str(build), '--show-only=json-v1'])
    config = json.loads((source / 'tools/g4_matrix.json').read_text())
    style = next(item for item in config['configurations'] if item['name'] == 'style')
    selected = call('inventory_style_matrix', [ctest, '--test-dir', str(build),
                   '--show-only=json-v1'] + style['ctest_args'])
    full_names = [item['name'] for item in json.loads(full.stdout)['tests']]
    selected_names = [item['name'] for item in json.loads(selected.stdout)['tests']]
    check('matrix_style_includes_opt', 'mhgp11_style_opt' in full_names and
          selected_names == ['mhgp11_style', 'mhgp11_style_opt'], {'all': full_names, 'selected': selected_names,
                                               'style_config': style})
    selected_run = call('style_matrix_selected_run', [ctest, '--test-dir', str(build),
                        '--output-on-failure', '--no-tests=error'] + style['ctest_args'])
    check('style_matrix_subset_is_green', selected_run.returncode == 0, selected_run.stdout)

    wrapper = source / 'cmake/run_expect.cmake'
    for expected in range(5):
        result = call('expected_code_' + str(expected), [cmake, '-DCMD=' + sys.executable,
                      '-DEXPECTED=' + str(expected), '-DNARGS=2', '-DARG0=-c',
                      '-DARG1=import sys; sys.exit(' + str(expected) + ')', '-P', str(wrapper)])
        check('accept_exact_' + str(expected), result.returncode == 0, result.stdout + result.stderr)
    wrong = call('wrong_code', [cmake, '-DCMD=' + sys.executable, '-DEXPECTED=0', '-DNARGS=2',
                 '-DARG0=-c', '-DARG1=import sys; sys.exit(2)', '-P', str(wrapper)])
    check('reject_wrong_numeric', wrong.returncode != 0 and 'run_expect_verdict code' in wrong.stdout,
          wrong.stdout + wrong.stderr)
    signal = call('real_sigterm', [cmake, '-DCMD=' + sys.executable, '-DEXPECTED=0', '-DNARGS=2',
                  '-DARG0=-c', '-DARG1=import os, signal; os.kill(os.getpid(), signal.SIGTERM)',
                  '-P', str(wrapper)])
    check('wrapper_rejects_real_signal', signal.returncode != 0 and
          'run_expect_verdict arret_anormal' in signal.stdout, signal.stdout + signal.stderr)
    gate = module(source / 'tests/support/mhgp11_gate.py', 'gate_review_support')
    completed = gate.run([sys.executable, '-c', 'import time; time.sleep(1)'], timeout=0.03)
    check('python_timeout_is_not_numeric', completed.code is None and completed.timed_out,
          {'code': completed.code, 'signal': completed.signal, 'timed_out': completed.timed_out})
    # These are classification controls, not evidence that any product mutant is non-causal.
    mutants = module(source / 'tests/mutants/run_mutants.py', 'gate_review_mutants')
    check('unknown_failure_has_no_cause', mutants.kill_cause('lancement impossible : outil absent') == 'autre',
          mutants.kill_cause('lancement impossible : outil absent'))
    result = {'schema': 'mhgp11.audit.gates.v1', 'optimized': sys.flags.optimize,
              'source': str(source), 'fixture_manifest_sha256': manifest_sha,
              'fixture_source_sha256': source_sha, 'checks': checks, 'commands': records,
              'note': 'Expected failing child gates are retained. Success of this audit means the stated classifications were reproduced, not that the product is qualified.'}
    args.result.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    failed = [item['name'] for item in checks if not item['ok']]
    print(json.dumps({'checks': len(checks), 'failed': failed, 'optimized': sys.flags.optimize}))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
