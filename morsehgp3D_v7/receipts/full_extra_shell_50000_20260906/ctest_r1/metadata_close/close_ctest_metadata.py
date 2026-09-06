#!/usr/bin/env python3
"""Read-only rejudgment after the -MD depfile reader error; no engine rerun."""
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import xml.etree.ElementTree as ET

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
CAPTURE = BASE / 'ctest_r1'
BUILD = BASE / 'cmake_r1'
TARGET = 'mhgp7_full_extra_shell_diagnostic_gate'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    original = json.loads((CAPTURE / 'receipt.json').read_text())
    need(original['status'] == 'failed' and '/usr/include/stdc-predef.h' in original['error'], 'specific postprocessing failure')
    need(len(original['commands']) == 7 and all(row['exit_code'] == 0 for row in original['commands']), 'all commands closed successfully')
    for name, pin in original['artifacts'].items():
        need(sha(CAPTURE / name) == pin, 'original capture unchanged')
    tests = {'mhgp7_full_extra_shell_diagnostic': (0, '--selftest'),
             'mhgp7_full_extra_shell_diagnostic_bad_argument': (2, '--unknown')}
    inventory = json.loads((CAPTURE / 'inventory.stdout').read_text())
    need(len(inventory['tests']) == 2 and {t['name'] for t in inventory['tests']} == set(tests), 'exact inventory')
    for test in inventory['tests']:
        code, arg = tests[test['name']]
        need('-DEXPECTED=' + str(code) in test['command'] and '-DARGS=' + arg in test['command'] and
             all(p['name'] != 'TIMEOUT' for p in test['properties']), 'codes/arguments/no added timeout')
    cases = ET.parse(CAPTURE / 'junit.xml').getroot().findall('.//testcase')
    need(len(cases) == 2 and {c.attrib['name'] for c in cases} == set(tests) and
         all(c.find('failure') is None and c.find('skipped') is None for c in cases), 'two tests passed')
    need('"checks":33,"status":"passed","FULL_built":false' in (CAPTURE / 'ctest.stdout').read_text(), '33 checks')
    depfile = 'CMakeFiles/' + TARGET + '.dir/tests/full_extra_shell_diagnostic_gate.cpp.o.d'
    deps = [Path(name).resolve() for name in shlex.split((BUILD / depfile).read_text().replace('\\\n', ' ').split(':', 1)[1])]
    before = json.loads((CAPTURE / 'source_candidates_before.json').read_text())
    local = {str(path.relative_to(ROOT)): sha(path) for path in deps if path.is_relative_to(ROOT)}
    for name in ('CMakeLists.txt', 'cmake/run_expect.cmake', 'bench/full_extra_shell_diagnostic.hpp', 'tests/full_extra_shell_diagnostic_gate.cpp'):
        local['morsehgp3D_v7/' + name] = sha(ROOT / 'morsehgp3D_v7' / name)
    local_before = {name: before[name] for name in local}
    need(local == local_before, 'compiled/control project sources unchanged')
    external_after = {str(path): sha(path) for path in deps if not path.is_relative_to(ROOT)}
    builds = json.loads((BUILD / 'compile_commands.json').read_text())
    selected = [row for row in builds if row['file'] == str(ROOT / 'morsehgp3D_v7/tests/full_extra_shell_diagnostic_gate.cpp')]
    need(len(selected) == 1 and 'MHGP7_TESTING' not in selected[0]['command'], 'nominal target')
    out = CAPTURE / 'metadata_close'
    out.mkdir(exist_ok=False)
    for name in ('CMakeCache.txt', 'CTestTestfile.cmake', 'compile_commands.json', 'Testing/Temporary/LastTest.log',
                 'CMakeFiles/' + TARGET + '.dir/flags.make', depfile):
        dest = out / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(BUILD / name, dest)
    shutil.copyfile(__file__, out / 'close_ctest_metadata.py')
    save(out / 'compiled_sources_before.json', local_before)
    save(out / 'compiled_sources_after.json', local)
    save(out / 'external_dependencies_observed_after.json', external_after)
    report = dict(status='completed_rejudgment', tests_passed=2, diagnostic_checks=33,
        original_capture_status='failed_postprocessing_preserved', original_receipt_sha256=sha(CAPTURE / 'receipt.json'),
        expected_program_codes={k: v[0] for k, v in tests.items()}, project_sources_stable=True,
        external_dependency_scope='hashes observed after compilation only, not pre/post stability',
        binary_sha256=sha(BUILD / TARGET), compiled_project_sources=len(local), external_dependencies=len(external_after),
        engine_reruns=0, extra_compilations=0, explicit_TIMEOUT_added=False, public_status='not_claimed')
    report['artifacts'] = {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
    save(out / 'receipt.json', report)
    print(json.dumps(dict(status=report['status'], tests=2, checks=33, receipt_sha256=sha(out / 'receipt.json')), sort_keys=True))


if __name__ == '__main__':
    main()
