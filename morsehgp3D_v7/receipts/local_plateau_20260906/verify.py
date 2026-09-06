#!/usr/bin/env python3
"""Offline receipt verification only; no subprocess or geometric replay."""
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def read(path):
    return json.loads(path.read_text(), object_pairs_hook=unique)


def main():
    root = Path(__file__).resolve().parent
    publication = read(root / 'publication.json')
    need(publication['GCP_used'] is False and publication['FULL_integrated'] is False and
         publication['public_status'] == 'not_claimed', 'local scope')
    for name, pin in publication['files'].items():
        need(sha(root / name) == pin, 'file: ' + name)
    for ref in [*publication['sources'].values(), publication['real_trace'], publication['real_judge']]:
        need(sha(root / ref['relative_path']) == ref['sha256'], 'source/reference')
    manifest = root / 'SHA256SUMS'
    if manifest.exists():
        for line in manifest.read_text().splitlines():
            pin, name = line.split('  ', 1)
            need(not name.startswith('/') and '..' not in Path(name).parts and sha(root / name) == pin, 'manifest')
    q = root / 'qualification'
    prepare, san = read(q / 'prepare.receipt.json'), read(q / 'san.receipt.json')
    need(prepare['status'] == san['status'] == 'completed' and prepare['cpp_closed'] and san['cpp_closed'] and
         prepare['commands'] == 9 and san['commands'] == 2 and prepare['binaries'] == san['binaries'] and
         san['prepare_receipt_sha256'] == sha(q / 'prepare.receipt.json'), 'two-phase closure')
    for name, expected in (
        ('O2_dependencies', 0), ('O2_compile', 0), ('O2_selftest', 0), ('O2_unknown', 2),
        ('san_dependencies', 0), ('san_compile', 0), ('mutant_dependencies', 0),
        ('mutant_compile', 0), ('mutant_selftest', 1), ('san_selftest', 0), ('san_unknown', 2),
    ):
        row = read(q / (name + '.command.json'))
        need(row['closed'] is True and row['exit_code'] == row['expected_exit'] == expected and
             row['added_quotas'] == 'none' and row['pid'] is not None, 'command ' + name)
        need(row['environment']['ASAN_OPTIONS'] == 'detect_leaks=1:halt_on_error=1' and
             row['environment']['UBSAN_OPTIONS'] == 'halt_on_error=1:print_stacktrace=1', 'sanitizer policy')
        for stream in ('stdout', 'stderr'):
            need(sha(q / (name + '.' + stream)) == row['streams'][stream], 'command bytes')
    need((q / 'O2_selftest.stdout').read_bytes() == (q / 'san_selftest.stdout').read_bytes(), 'O2/SAN parity')
    gate = read(q / 'O2_selftest.stdout')
    need(gate['schema'] == 'mhgp7-local-plateau-v1' and gate['status'] == 'passed' and
         gate['tables'] == 18 and gate['ranks'] == 96 and gate['components'] == gate['representatives'] == 96 and
         gate['real_tables'] == 4 and gate['real_ranks'] == 40 and gate['large_p'] == 5000 and
         gate['supports_q2_q3_q4'] == [20, 14, 4] and gate['external_global_parent_counts'] == [2, 1], 'nonvacuity')
    need((q / 'san_selftest.stderr').read_bytes() == b'' and
         (q / 'mutant_selftest.stderr').read_text() == 'local plateau rejected: quotient.strict_component_count\n', 'causal mutation')
    nominal = (root / 'source_snapshot/morsehgp3D_v7/src/forest/local_plateau.hpp').read_text()
    mutated = (root / 'mutant/local_plateau.hpp').read_text()
    need(nominal.count('if (a != b) parent[') == 1 and
         nominal.replace('if (a != b) parent[', 'if (a != b && mask == 0) parent[') == mutated, 'single mutant edit')
    for mode in ('O2', 'san'):
        for absolute, pin in read(q / (mode + '.sources_before.json')).items():
            marker = '/morsehgp3D_v7/'
            if marker in absolute:
                name = 'morsehgp3D_v7/' + absolute.split(marker, 1)[1]
                need(publication['sources'][name]['sha256'] == pin, 'compiled project source')
    for name, pin in read(root / 'ctest/project_sources.json').items():
        need(publication['sources'][name]['sha256'] == pin, 'CMake project source')
    ctest = read(root / 'ctest/receipt.json')
    need(ctest['status'] == 'completed' and ctest['GCP_used'] is False and len(ctest['commands']) == 4 and
         ctest['recorder_sha256'] == sha(root / 'ctest/capture.py'), 'CTest closure')
    for name, row in zip(('configure', 'build', 'inventory', 'ctest'), ctest['commands']):
        need(row == read(root / ('ctest/' + name + '.command.json')) and
             row['exit_code'] == 0 and row['imposed_timeout'] is None, 'CMake command')
        for stream in ('stdout', 'stderr'):
            need(sha(root / ('ctest/' + name + '.' + stream)) == row[stream + '_sha256'], 'CMake bytes')
    tests = read(root / 'ctest/inventory.stdout')['tests']
    expected_tests = {'mhgp7_local_plateau': (0, '--selftest'), 'mhgp7_local_plateau_bad_argument': (2, '--unknown')}
    need(len(tests) == 2 and {t['name'] for t in tests} == set(expected_tests), 'two CTests')
    for test in tests:
        code, argument = expected_tests[test['name']]
        need('-DEXPECTED=' + str(code) in test['command'] and '-DARGS=' + argument in test['command'] and
             all(p['name'] != 'TIMEOUT' for p in test['properties']), 'CTest exact return/no new quota')
    cases = ET.parse(root / 'ctest/junit.xml').getroot().findall('testcase')
    need(len(cases) == 2 and all(case.attrib['status'] == 'run' and case.find('failure') is None and
         case.find('skipped') is None for case in cases), 'CTest all passed')
    need('MHGP7_TESTING' not in (root / 'ctest/CMakeFiles/mhgp7_local_plateau_gate.dir/flags.make').read_text(), 'nominal product headers')
    need(read(root / 'history/r1/receipt.json')['status'] != 'completed' and
         read(root / 'history/r2/receipt.json')['status'] != 'completed' and
         'LeakSanitizer does not work under ptrace' in (root / 'history/r2/logs/san_selftest.stderr').read_text(), 'historical failures preserved')
    print(json.dumps(dict(status='verified_offline', public_status='not_claimed', O2_SAN_equal=True,
        oracle_tables=18, oracle_ranks=96, real_ranks=40, causal_mutant_rejected=True,
        CTests_passed=2, commands_closed=15, FULL_integrated=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
