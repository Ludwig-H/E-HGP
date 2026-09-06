#!/usr/bin/env python3
"""Only the two extra-shell diagnostic CTests; no benchmark or cloud action."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
SOURCE = ROOT / 'morsehgp3D_v7'
BUILD = BASE / 'cmake_r1'
OUT = BASE / 'ctest_r1'
TARGET = 'mhgp7_full_extra_shell_diagnostic_gate'
TESTS = {'mhgp7_full_extra_shell_diagnostic': (0, '--selftest'),
         'mhgp7_full_extra_shell_diagnostic_bad_argument': (2, '--unknown')}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    need(sys.argv[1:] == ['--execute'], 'inert without --execute')
    need(not BUILD.exists() and not OUT.exists(), 'fresh directories required')
    OUT.mkdir()
    controls = ['CMakeLists.txt', 'cmake/run_expect.cmake', 'bench/full_extra_shell_diagnostic.hpp',
                'tests/full_extra_shell_diagnostic_gate.cpp']
    paths = set((SOURCE / 'src').rglob('*.hpp')) | set((SOURCE / 'bench').glob('*.hpp'))
    paths.update(SOURCE / name for name in controls)
    before = {str(path.relative_to(ROOT)): sha(path) for path in sorted(paths)}
    save(OUT / 'source_candidates_before.json', before)
    for name in controls:
        dest = OUT / 'source_controls' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(SOURCE / name, dest)
    shutil.copyfile(__file__, OUT / 'ctest_capture.py')
    receipt = dict(status='failed', public_status='not_claimed', commands=[], expected_program_codes={k: v[0] for k, v in TESTS.items()},
                   explicit_TIMEOUT_added=False, benchmark=False, GCP_used=False)

    def command(name, argv):
        row = dict(argv=list(map(str, argv)), cwd=str(ROOT), started_ns=time.time_ns(), imposed_timeout=None)
        save(OUT / (name + '.intent.json'), row)
        with (OUT / (name + '.stdout')).open('xb') as out, (OUT / (name + '.stderr')).open('xb') as err:
            process = subprocess.Popen(row['argv'], cwd=ROOT, stdin=subprocess.DEVNULL,
                                       stdout=out, stderr=err, start_new_session=True)
            row['pid'] = process.pid
            save(OUT / (name + '.spawn.json'), row)
            try:
                row['exit_code'] = process.wait()
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGKILL)
                row['exit_code'] = process.wait()
                row['ended_ns'] = time.time_ns()
                row['stdout_sha256'], row['stderr_sha256'] = sha(OUT / (name + '.stdout')), sha(OUT / (name + '.stderr'))
                receipt['commands'].append(row)
                save(OUT / (name + '.command.json'), row)
        print(name, row['exit_code'], flush=True)
        need(row['exit_code'] == 0, name + ' failed')

    try:
        for name, program in (('cmake_version', 'cmake'), ('compiler_version', 'c++'), ('ctest_version', 'ctest')):
            command(name, [program, '--version'])
        command('configure', ['cmake', '-S', SOURCE, '-B', BUILD, '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=' + str(ROOT / 'build/v7_boost_gate/extracted/usr/include')])
        command('build', ['cmake', '--build', BUILD, '--target', TARGET, '--parallel', '1'])
        regex = '^mhgp7_full_extra_shell_diagnostic(_bad_argument)?$'
        command('inventory', ['ctest', '--test-dir', BUILD, '-R', regex, '--show-only=json-v1'])
        inventory = json.loads((OUT / 'inventory.stdout').read_text())
        need(len(inventory['tests']) == 2 and {t['name'] for t in inventory['tests']} == set(TESTS), 'exact two CTests')
        for test in inventory['tests']:
            code, argument = TESTS[test['name']]
            need('-DEXPECTED=' + str(code) in test['command'] and '-DARGS=' + argument in test['command'], 'expected code and argv')
            need(all(prop['name'] != 'TIMEOUT' for prop in test['properties']), 'no explicit timeout')
        command('ctest', ['ctest', '--test-dir', BUILD, '-R', regex, '-V', '--output-on-failure', '--output-junit', OUT / 'junit.xml'])
        cases = ET.parse(OUT / 'junit.xml').getroot().findall('.//testcase')
        need(len(cases) == 2 and {c.attrib['name'] for c in cases} == set(TESTS) and
             all(c.find('failure') is None and c.find('skipped') is None for c in cases), 'two tests passed')
        raw = (OUT / 'ctest.stdout').read_text()
        need('"checks":33,"status":"passed","FULL_built":false' in raw, '33 diagnostic checks')
        relative_dep = 'CMakeFiles/' + TARGET + '.dir/tests/full_extra_shell_diagnostic_gate.cpp.o.d'
        deptext = (BUILD / relative_dep).read_text().replace('\\\n', ' ').split(':', 1)[1]
        dependencies = {str(Path(name).resolve().relative_to(ROOT)) for name in shlex.split(deptext)}
        dependencies.update('morsehgp3D_v7/' + name for name in controls)
        compiled_before = {name: before[name] for name in sorted(dependencies)}
        compiled_after = {name: sha(ROOT / name) for name in sorted(dependencies)}
        save(OUT / 'compiled_sources_before.json', compiled_before)
        save(OUT / 'compiled_sources_after.json', compiled_after)
        need(compiled_before == compiled_after, 'compiled/control source drift')
        for name in ('CMakeCache.txt', 'CTestTestfile.cmake', 'compile_commands.json', 'Testing/Temporary/LastTest.log',
                     'CMakeFiles/' + TARGET + '.dir/flags.make', relative_dep):
            dest = OUT / 'cmake_evidence' / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(BUILD / name, dest)
        builds = json.loads((BUILD / 'compile_commands.json').read_text())
        selected = [r for r in builds if r['file'] == str(SOURCE / 'tests/full_extra_shell_diagnostic_gate.cpp')]
        need(len(selected) == 1 and 'MHGP7_TESTING' not in selected[0]['command'], 'nominal binary without testing macro')
        receipt.update(status='completed', tests_passed=2, diagnostic_checks=33, sources_stable=True,
                       binary_sha256=sha(BUILD / TARGET), compiled_sources_sha256=sha(OUT / 'compiled_sources_before.json'),
                       CMakeLists_sha256=before['morsehgp3D_v7/CMakeLists.txt'])
    except BaseException as error:
        receipt['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        receipt['artifacts'] = {str(path.relative_to(OUT)): sha(path) for path in sorted(OUT.rglob('*')) if path.is_file()}
        save(OUT / 'receipt.json', receipt)
    print(json.dumps(dict(status=receipt['status'], tests=2, checks=33, receipt_sha256=sha(OUT / 'receipt.json')), sort_keys=True))


if __name__ == '__main__':
    main()
