#!/usr/bin/env python3
"""Real wrapper + CTest/JUnit + mutant campaign decision; configure/build replaced by an explicit fixture writer."""
import argparse
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import sys


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--result', type=Path, required=True)
    args = p.parse_args()
    args.work.mkdir(parents=True, exist_ok=False)
    source = args.source.resolve()
    runner_path = source / 'morsehgp3D_v11/tests/mutants/run_mutants.py'
    spec = importlib.util.spec_from_file_location('audit_mutant_reader', runner_path)
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    fixture = args.work / 'fixture'
    (fixture / 'cmake').mkdir(parents=True)
    (fixture / 'tests').mkdir()
    shutil.copy2(source / 'morsehgp3D_v11/cmake/run_expect.cmake', fixture / 'cmake/run_expect.cmake')
    (fixture / 'CMakeLists.txt').write_text('# fixture never configured or built\n')
    script = fixture / 'tests/program'
    script.write_text('#!' + sys.executable + '\nraise SystemExit(0)\n')
    script.chmod(0o755)
    manifest = {'module': 'essai', 'plancher': 1, 'mutants': [{
        'id': 'interpreteur_absent', 'fichier': 'tests/program', 'cherche': '#!' + sys.executable,
        'remplace': '#!' + str(args.work / 'NO_SUCH_INTERPRETER'), 'porte': 'mhgp11_target'}]}
    manifest_path = args.work / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    cmake, ctest = shutil.which('cmake'), shutil.which('ctest')
    setup_calls = []

    def fixture_writer(self, folder, options, parallel):
        # Produces only a CTest fixture description. No compiler or CMake configure/build is called.
        folder = Path(folder)
        build = folder / 'build'
        build.mkdir()
        words = [cmake, '-DCMD=' + str(folder / 'src/tests/program'), '-DEXPECTED=0', '-DNARGS=0',
                 '-P', str(folder / 'src/cmake/run_expect.cmake')]
        text = 'add_test(mhgp11_target ' + ' '.join(json.dumps(w) for w in words) + ')\n'
        text += 'set_tests_properties(mhgp11_target PROPERTIES TIMEOUT 5)\n'
        (build / 'CTestTestfile.cmake').write_text(text)
        setup_calls.append({'folder': str(folder), 'scope': 'explicit no-build fixture writer',
                            'ctest_file': text, 'parallel_argument': parallel})
        return None, 'audit fixture writer: no configure/build\n'

    runner.Builder.configure_and_build = fixture_writer
    report = args.work / 'report.json'
    argv = ['--source', str(fixture), '--manifest', str(manifest_path), '--work', str(args.work / 'campaign'),
            '--jobs', '1', '--build-jobs', '1', '--ctest', ctest, '--report', str(report), '--keep']
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        code = runner.run(argv)
    written = json.loads(report.read_text())
    logs = {}
    for name in ['temoin_0', 'interpreteur_absent']:
        logs[name] = (args.work / 'campaign' / name / 'build/Testing/Temporary/LastTest.log').read_text()
    ok = (code == 3 and written['temoin'] == 'vert' and written['mutants'][0]['verdict'] == 'INVALIDE' and
          'lancement_impossible' in written['mutants'][0]['detail'] and
          'mutants_ok' not in output.getvalue() and 'Test Passed.' in logs['temoin_0'] and
          'run_expect_verdict lancement_impossible' in logs['interpreteur_absent'])
    payload = {'scope': __doc__, 'optimized': sys.flags.optimize, 'source': str(source), 'argv': argv,
               'run_returncode': code, 'output': output.getvalue(), 'report': written,
               'setup_double_calls': setup_calls, 'ctest_logs': logs, 'check_passed': ok,
               'process_cleanup': 'run_ctest_gate uses subprocess.run; CTest, CMake and the witness interpreter are waited synchronously. The bad interpreter never launches.'}
    args.result.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'check_passed': ok, 'campaign_code': code, 'optimized': sys.flags.optimize}))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
