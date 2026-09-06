#!/usr/bin/env python3
"""Offline transport/status checks; no engine, geometry oracle or cloud calls."""
import hashlib
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

PINS = {
    'run_r1': 'd835d6c2c8bbc31a73a0028f6d1611bde352999bfe70f93290d4f6dfef045092',
    'run_r2': '505c39961c2d3beca5b2a7a08d6e9575ad2df0baa3ee4d5674f7b62b851b3f70',
    'run_r3': 'f24c397a98ce3ff09e55c9fad01fbecf763878d483dcd59a4dea973518f0e298',
    'ctest_r1': '5dee16d27d55a3bbfc5be65ead64dd6ca75fa0ae583f0cc3058e8d9284deeee7',
    'ctest_r1/metadata_close': 'bcc575c6e7c0f4d11b0554f337a96158ba12400ddede1ba8823e56e91255f59a',
}
NAMES = {
    'run_r1': ['compile_O2'],
    'run_r2': ['compile_O2', 'selftest_O2', 'square_O2', 'compile_SAN', 'selftest_SAN'],
    'run_r3': ['compile_O2', 'selftest_O2', 'square_O2', 'compile_SAN', 'selftest_SAN',
               'square_SAN', 'unknown_gate', 'compile_probe', 'micro_plain',
               'micro_diagnostic', 'duplicate_flag', 'n50000_k10'],
    'ctest_r1': ['cmake_version', 'compiler_version', 'ctest_version', 'configure',
                 'build', 'inventory', 'ctest'],
}
CODES = {'run_r1': [1], 'run_r2': [0, 0, 0, 0, 1],
         'run_r3': [0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 2], 'ctest_r1': [0] * 7}
SCHEMA = 'mhgp7-full-gabriel-probe-v6'
DIAGNOSTIC = 'mhgp7-extra-shell-diagnostic-v1'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pairs(items):
    result = {}
    for key, value in items:
        need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def parse(text):
    return json.loads(text, object_pairs_hook=pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))


def load(path):
    return parse(path.read_text())


def equal(left, right):
    return json.dumps(left, sort_keys=True) == json.dumps(right, sort_keys=True)


def main(root):
    root = root.resolve()
    pub = load(root / 'publication.json')
    need(pub['public_status'] == 'not_claimed' and pub['actual_GCP_used'] is False,
         'publication scope')
    for name, pin in pub['payload_artifacts'].items():
        path = root / name
        need(path.is_file() and not path.is_symlink() and sha(path) == pin, 'payload hash: ' + name)
    for name, item in pub['source_locations'].items():
        path = (root / item['relative_path']).resolve()
        need(path.is_relative_to(root.parent) and sha(path) == item['sha256'], 'source: ' + name)
    manifest = root / 'BASE_SHA256SUMS'
    if manifest.exists():
        for line in manifest.read_text().splitlines():
            pin, name = line.split('  ', 1)
            need(not name.startswith(('/', './', '../', 'reader/')) and
                 (root / name).resolve().is_relative_to(root) and sha(root / name) == pin,
                 'base manifest: ' + name)
    receipts = {}
    for name, pin in PINS.items():
        path = root / name / 'receipt.json'
        need(sha(path) == pin, 'closed receipt pin: ' + name)
        receipts[name] = load(path)
        for relative, expected in receipts[name].get('artifacts', {}).items():
            virtual = name + '/' + relative
            path = root / virtual
            if path.exists():
                need(sha(path) == expected, 'receipt artifact: ' + virtual)
            else:
                omission = pub['omitted_artifacts'][virtual]
                need(omission['sha256'] == expected, 'omission hash: ' + virtual)
                if 'replacement' in omission:
                    need(sha(root / omission['replacement']) == expected, 'replacement bytes')
                else:
                    need(omission['reason'] == 'redundant_intent_command_retained', 'unexpected omission')
    for name, names in NAMES.items():
        receipt = receipts[name]
        need(len(receipt['commands']) == len(names), 'command count')
        for index, command_name in enumerate(names):
            prefix = root / name / command_name
            row = load(prefix.with_suffix('.command.json'))
            need(equal(row, receipt['commands'][index]), 'command/receipt binding')
            need(type(row['exit_code']) is int and row['exit_code'] == CODES[name][index], 'exit code')
            need(type(row['pid']) is int and row['pid'] > 0, 'PID')
            for stream in ('stdout', 'stderr'):
                need(sha(prefix.with_suffix('.' + stream)) == row[stream + '_sha256'], 'stream hash')
            if name != 'ctest_r1':
                expected = 2 if command_name in ('unknown_gate', 'duplicate_flag', 'n50000_k10') else 0
                need(row['expected_exit'] == expected and row['timeout'] is None, 'expected code/no quota')
            else:
                need(row['ended_ns'] >= row['started_ns'] and row['imposed_timeout'] is None, 'CTest closure')
        if name.startswith('run_'):
            need(receipt['sources_stable'] is True and receipt['actual_GCP_used'] is False and
                 receipt['performance_contract_certified'] is False and receipt['public_status'] == 'not_claimed',
                 'run scope')
            need(equal(load(root / name / 'edited_sources_before.json'),
                       load(root / name / 'edited_sources_after.json')), 'edited source stability')
    need(receipts['run_r1']['status'] == receipts['run_r2']['status'] == 'failed', 'failures preserved')
    need('Werror=ignored-attributes' in (root / 'run_r1/compile_O2.stderr').read_text(), 'r1 Werror')
    need('LeakSanitizer does not work under ptrace' in (root / 'run_r2/selftest_SAN.stderr').read_text(), 'r2 LSan')
    need(pub['historical_r1_recorder_source_available'] is False and
         pub['historical_r1_gate_source_available'] is False, 'historical source gap')
    need(receipts['run_r3']['status'] == 'completed', 'r3 capture closed')
    for mode in ('O2', 'SAN'):
        need(equal(load(root / ('run_r3/selftest_' + mode + '.stdout')),
                   {'schema': 'mhgp7-extra-shell-diagnostic-gate-v1', 'checks': 33,
                    'status': 'passed', 'FULL_built': False}), '33 gate checks')
        need((root / ('run_r3/selftest_' + mode + '.stderr')).stat().st_size == 0, 'gate stderr')
    need((root / 'run_r3/square_O2.stdout').read_bytes() ==
         (root / 'run_r3/square_SAN.stdout').read_bytes(), 'square parity')
    deps = load(root / 'run_r3/dependencies.json')
    need(len(deps) == 43, '43 real probe dependencies')
    for name, pin in deps.items():
        need(pub['source_locations'][name]['sha256'] == pin, 'dependency source binding')
    for snapshot in ('run_r3/edited_sources_before.json', 'ctest_r1/metadata_close/compiled_sources_before.json'):
        for name, pin in load(root / snapshot).items():
            need(pub['source_locations'][name]['sha256'] == pin, 'source snapshot binding')
    arms = []
    for name, diagnosed in [('micro_plain', False), ('micro_diagnostic', True)]:
        rows = [parse(line) for line in (root / ('run_r3/' + name + '.stdout')).read_text().splitlines()]
        need(len(rows) == 10 and [r['type'] for r in rows] == ['configuration'] + ['order'] * 8 + ['terminal'], 'eight micro orders')
        need(all(r['schema'] == SCHEMA for r in rows), 'original v6 schema preserved')
        need(all(('extra_shell_diagnostics' in r) == diagnosed for r in rows), 'opt-in metadata only')
        if diagnosed:
            need(all(r['extra_shell_diagnostics'] == DIAGNOSTIC for r in rows) and
                 rows[-1]['extra_shell_diagnostic_records'] == 0, 'diagnostic schema')
        else:
            need('extra_shell_diagnostic_records' not in rows[-1], 'plain output unchanged')
        need(rows[-1]['outcome'] == 'complete_relative' and rows[-1]['exit_code'] == 0 and
             rows[-1]['completed_orders_diagnostic'] == 8 and
             (root / ('run_r3/' + name + '.stderr')).stat().st_size == 0, 'successful micro')
        arms.append(rows)
    need([r.get('certificate_digest') for r in arms[0]] ==
         [r.get('certificate_digest') for r in arms[1]], 'micro semantic digest parity')
    rows = [parse(line) for line in (root / 'run_r3/n50000_k10.stdout').read_text().splitlines()]
    need([r['type'] for r in rows] == ['configuration', 'terminal'], 'no 50k FULL orders')
    terminal = rows[-1]
    need(all(r['schema'] == SCHEMA and r['extra_shell_diagnostics'] == DIAGNOSTIC for r in rows), '50k schemas')
    need(terminal['reason'] == 'probe_rank_relevant_extra_shell' and terminal['exit_code'] == 2 and
         terminal['outcome'] == 'unsupported_degeneracy' and terminal['completed_orders_diagnostic'] == 0 and
         terminal['certificate_digest'] == '' and terminal['complete_requested_horizontal_orders'] is False and
         terminal['last_stage'] == 'regularity' and terminal['rank_relevant_extra_shell'] == 4 and
         terminal['extra_shell_diagnostic_records'] == 4 and terminal['public_status'] == 'not_claimed', '50k refusal unchanged')
    traces = [parse(line) for line in (root / 'run_r3/n50000_k10.stderr').read_text().splitlines() if line.startswith('{')]
    need(len(traces) == 4 and len({r['ball_index'] for r in traces}) == 4, 'four BallData records')
    need(all(r['schema'] == DIAGNOSTIC and r['type'] == 'extra_shell' and r['diagnostic_only'] is True and
             r['input_digest'] == terminal['input_digest'] and r['n'] == 50000 and r['smax'] == 11 for r in traces), 'trace metadata binding')
    duplicate = load(root / 'run_r3/duplicate_flag.stdout')
    need(duplicate['exit_code'] == 2 and duplicate['reason'] == 'probe_arguments', 'duplicate option refusal')
    closed = receipts['ctest_r1/metadata_close']
    need(receipts['ctest_r1']['status'] == 'failed' and closed['status'] == 'completed_rejudgment' and
         closed['original_receipt_sha256'] == PINS['ctest_r1'] and closed['tests_passed'] == 2 and
         closed['diagnostic_checks'] == 33 and closed['engine_reruns'] == closed['extra_compilations'] == 0 and
         closed['project_sources_stable'] is True, 'CTest metadata-only closure')
    need(equal(load(root / 'ctest_r1/metadata_close/compiled_sources_before.json'),
               load(root / 'ctest_r1/metadata_close/compiled_sources_after.json')), 'CTest sources stable')
    tests = {'mhgp7_full_extra_shell_diagnostic': (0, '--selftest'),
             'mhgp7_full_extra_shell_diagnostic_bad_argument': (2, '--unknown')}
    inventory = load(root / 'ctest_r1/inventory.stdout')['tests']
    need(len(inventory) == 2 and {t['name'] for t in inventory} == set(tests), 'CTest inventory')
    for test in inventory:
        code, arg = tests[test['name']]
        need('-DEXPECTED=' + str(code) in test['command'] and '-DARGS=' + arg in test['command'] and
             all(p['name'] != 'TIMEOUT' for p in test['properties']), 'CTest expected return/no timeout')
    junit = ET.parse(root / 'ctest_r1/junit.xml').getroot()
    cases = junit.findall('testcase')
    need(len(cases) == 2 and {c.attrib['name'] for c in cases} == set(tests) and
         all(c.attrib['status'] == 'run' and c.find('failure') is None and c.find('skipped') is None for c in cases), '2/2 CTests')
    print(json.dumps({'status': 'verified_offline', 'public_status': 'not_claimed', 'GCP_used': False,
                      'new_engine_runs': 0, 'historical_commands': 25, 'diagnostic_checks': 33,
                      'probe_dependencies': 43, 'micro_orders_per_arm': 8, 'BallData_traces': 4,
                      'n50000_exit': 2, 'n50000_FULL_orders': 0, 'CTest_passed': 2,
                      'performance_contract_certified': False}, sort_keys=True))


if __name__ == '__main__':
    try:
        need(len(sys.argv) <= 2, 'usage: verify.py [receipt-root]')
        main(Path(sys.argv[1]) if len(sys.argv) == 2 else Path(__file__).resolve().parent)
    except (ValueError, KeyError, TypeError, OSError, ET.ParseError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(2)
