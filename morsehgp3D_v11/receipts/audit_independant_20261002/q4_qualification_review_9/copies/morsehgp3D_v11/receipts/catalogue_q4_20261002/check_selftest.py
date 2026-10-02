"""Pure synthetic reader tests: no native executable, no cloud, no historical evidence modification."""
import copy
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch
import xml.etree.ElementTree as ET

import check


def require(value, message):
    if not value:
        raise ValueError(message)


def fixtures_module(filename, alias, module):
    path = check.HERE.parent / 'catalogue_profiles_20261002' / filename
    with patch.dict(sys.modules, {alias: module}):
        return check.load_module('q4_fixture_' + alias, path)


previous = fixtures_module('check_selftest.py', 'check', check.profiles)
asan_fixture = fixtures_module('check_asan18_selftest.py', 'check_asan18', check.asan)


def encode(value):
    return json.dumps(value, sort_keys=True).encode()


def fixture():
    report, arguments = previous.fixture()
    baseline = copy.deepcopy(report)
    baseline['runs'] = [r for r in baseline['runs'] if r['kmax'] == 5 and r['case'] != 'uniform_u18_n32000']
    report.update(schema=check.SCHEMA, work_schema=check.WORK_SCHEMA, supplement_sha256='d' * 64)
    for row in report['runs']:
        row['events'][1]['work'] = dict(q4_candidates=9, q4_levels=1)
        row['qmin_counts'] = {'2': 6, '3': 4, '4': 1}
    refresh(report)
    return report, (*arguments, 'd' * 64), baseline


def refresh(report):
    previous.refresh(report)
    report['q4_comparisons'] = check.q4_comparisons(report['runs'])
    report['conforming'] = report['conforming'] and all(c['status'] == 'equal' for c in report['q4_comparisons'])


def supplement_fixture(failed=False):
    data = asan_fixture.fixture(failed)
    prefix = check.asan.PREFIX
    root = ET.fromstring(data[prefix + 'junit.xml'])
    name = 'mhgp11_num_unit_candidate'
    case = ET.SubElement(root, 'testcase', name=name, status='run')
    ET.SubElement(case, 'system-out').text = ('test candidate controles=831 echecs=0 plancher=800\n'
                                            'mhgp11_test_ok tests=1 controles=831\nrun_expect_verdict conforme\n')
    root.set('tests', '5')
    for case in root:
        if case.get('name') in check.FRACTION_GATES:
            node = case.find('system-out')
            node.text = node.text.replace('7526', '11838').replace('504', '528')
    data[prefix + 'junit.xml'] = ET.tostring(root)
    selected = check.js(data[prefix + 'tests.json'])
    selected.append(dict(name=name, disabled=False, labels=['unit', 'fast']))
    data[prefix + 'tests.json'] = encode(selected)
    summary = check.js(data[check.BASE + 'summary.json'])
    config = summary['configurations'][0]
    for key in ('selected', 'passed', 'ctest_total'):
        config['tests'][key] += 1
    for label in ('unit', 'fast'):
        config['passed_labels'][label] += 1
    data[prefix + 'result.json'] = encode(config)
    data[check.BASE + 'summary.json'] = encode(summary)
    return data


def failed_matrix():
    data, configs = {}, []
    for name in sorted(check.old.foundation.NAMES):
        config = dict(name=name, status='build_failed' if name == 'gcc_release' else 'not_run_deadline',
                      conforming=False)
        configs.append(config)
        data[check.BASE + name + '/result.json'] = encode(config)
    data[check.BASE + 'summary.json'] = encode(dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True,
        requested=[c['name'] for c in configs], configurations=configs,
        statuses={c['name']: c['status'] for c in configs}, signals=[], exit_code=1, conforming=False))
    return data


def capture(report, args, baseline, matrix_code=0, mutate=None):
    # Only the archive transport and main-matrix fixture are mocked. The supplement, v2 report, command and
    # session validators execute normally; the frozen transport readers have their own receipt tests.
    report = copy.deepcopy(report)
    supplement = supplement_fixture()
    data = {check.SUPPLEMENT + key[len(check.BASE):]: value for key, value in supplement.items()}
    data[check.BASE + 'summary.json'] = b'{}'
    report['qualification_sha256'] = check.sha(b'{}')
    report['supplement_sha256'] = check.sha(supplement[check.BASE + 'summary.json'])
    for record in report['builds']:
        name = record['configuration']
        payload = encode(dict(configuration=name))
        data[check.BASE + name + '/build_provenance.json'] = payload
        record['provenance_sha256'] = check.sha(payload)
    if matrix_code == 0:
        data[check.BENCH + 'files/profiles.json'] = encode(report)
    codes = (matrix_code, 0, 0 if report['conforming'] else 1) if matrix_code == 0 else (matrix_code, 0, 2)
    for name, code in zip(check.COMMANDS, codes):
        data['results/cmd/' + name + '/meta.txt'] = ('status=%s\nexit_code=%d\ngroup_closed=1\n' %
                                                    ('ok' if code == 0 else 'failed', code)).encode()
    good = sum(code == 0 for code in codes)
    receipt = dict(commit=check.SOURCE_COMMIT, private_key_deleted=True, oslogin_key_removed=True, reserve_released=True,
                   status='completed' if good == 3 else 'failed_remote', worker_exit_code=0 if good == 3 else 1)
    worker = dict(commands_total='3', commands_ok=str(good), status='completed' if good == 3 else 'failed')
    if mutate:
        mutate(receipt, worker, data)
    with tempfile.TemporaryDirectory(prefix='ehgp_q4_reader_') as tmp:
        folder = Path(tmp)
        (folder / 'matrix.json').write_bytes(data[check.BASE + 'summary.json'])
        (folder / 'asan18.json').write_bytes(data[check.SUPPLEMENT + 'summary.json'])
        if check.BENCH + 'files/profiles.json' in data:
            (folder / 'profiles.json').write_bytes(data[check.BENCH + 'files/profiles.json'])
        with patch.object(check.old, 'read_capture', return_value=(receipt, worker, data)), \
                patch.object(check.old, 'inputs', return_value=args[:2]), \
                patch.object(check, 'judge_matrix', return_value=({}, args[3], matrix_code)), \
                contextlib.redirect_stdout(io.StringIO()):
            return check.check(folder, baseline)


def main():
    report, args, baseline = fixture()
    require(check.judge_report(report, *args)['conforming'], 'complete q4 witness')
    require(check.compare_baseline(report, baseline)['complete'], 'fifteen paired successes')
    failed = copy.deepcopy(report)
    previous.failure(failed)
    failed['runs'][0].pop('qmin_counts')
    refresh(failed)
    require(not check.judge_report(failed, *args)['conforming'], 'timeout preserved')
    require(len(check.compare_baseline(failed, baseline)['missing']) == 1, 'missing comparison visible')
    different = copy.deepcopy(report)
    different['runs'][0]['events'][1]['work']['q4_candidates'] += 1
    refresh(different)
    require(check.judge_report(different, *args)['q4_different'] == 1, 'q4 divergence remains non-green')
    partial = copy.deepcopy(report)
    partial.update(complete=False, runs=partial['runs'][:1], not_run=[])
    partial['runs'][0].update(status='pending_semantic')
    previous.remove_artifact(partial['runs'][0])
    partial['runs'][0].pop('qmin_counts')
    refresh(partial)
    require(not check.judge_report(partial, *args)['conforming'], 'pending decode preserved')
    artifact = copy.deepcopy(report)
    artifact['runs'][-1].update(status='artifact_error',
                               errors=[dict(stage='artifact', type='ValueError', message='q4 mismatch')])
    artifact['runs'][-1].pop('semantic')
    refresh(artifact)
    require(not check.judge_report(artifact, *args)['conforming'], 'qmin-only partial artifact preserved')
    supplement = supplement_fixture()
    require(check.judge_supplement(supplement) == ((5, 5, 0, 0), 4, 0), 'new num supplement')
    require(check.judge_supplement(supplement_fixture(True)) == ((5, 4, 1, 0), 4, 1), 'failed num supplement')
    require(check.judge_matrix(failed_matrix())[2] == 1, 'matrix build failures retained')
    require(not capture(report, args, baseline), 'assembled three-command success')
    require(capture(report, args, baseline, matrix_code=1), 'assembled matrix failure without benchmark')
    require(capture(failed, args, baseline), 'assembled failed benchmark')
    check.command(dict(status='failed', group_closed='1', exit_code='1'), 1)
    check.command(dict(status='skipped_deadline'))
    receipt = dict(status='failed_remote', worker_exit_code=1)
    worker = dict(commands_total='3', commands_ok='1', status='failed')
    metas = [dict(status='failed'), dict(status='ok'), dict(status='failed')]
    require(not check.session(receipt, worker, metas), 'failed matrix session preserved')
    cases = []

    def refuse(name, function):
        try:
            function()
        except (check.old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError):
            cases.append(name)
        else:
            raise ValueError(name + ' accepted')

    def corrupted(name, operation, source=report):
        value = copy.deepcopy(source)
        operation(value)
        refuse(name, lambda: check.judge_report(value, *args))

    def native_change(value, field, new):
        value['runs'][0]['events'][1]['work'][field] = new
        value['runs'][0]['stdout'] = '\n'.join(json.dumps(e) for e in value['runs'][0]['events'])

    corrupted('schema', lambda r: r.update(schema='ehgp.v11.catalogue_profiles.v1'))
    corrupted('work_schema', lambda r: r.update(work_schema='other'))
    corrupted('supplement_hash', lambda r: r.update(supplement_sha256='0' * 64))
    corrupted('qmin_missing', lambda r: r['runs'][0].pop('qmin_counts'))
    corrupted('qmin_bool', lambda r: r['runs'][0]['qmin_counts'].update({'4': True}))
    corrupted('qmin_total', lambda r: r['runs'][0]['qmin_counts'].update({'2': 7}))
    corrupted('levels_not_emitted', lambda r: native_change(r, 'q4_levels', 0))
    corrupted('candidates_below_levels', lambda r: native_change(r, 'q4_candidates', 0))
    corrupted('negative_candidates', lambda r: native_change(r, 'q4_candidates', -1))
    corrupted('overflow_candidates', lambda r: native_change(r, 'q4_candidates', 2**64))
    corrupted('extra_work_field', lambda r: native_change(r, 'invented', 0))
    corrupted('hidden_q4_difference', lambda r: r.update(q4_comparisons=report['q4_comparisons']), different)
    corrupted('q4_difference_green', lambda r: r.update(conforming=True), different)
    corrupted('pending_qmin', lambda r: r['runs'][0].update(qmin_counts={'2': 6, '3': 4, '4': 1}), partial)
    corrupted('pending_complete', lambda r: r.update(complete=True), partial)
    corrupted('duplicate_run', lambda r: r['runs'].append(copy.deepcopy(r['runs'][0])))
    corrupted('wrong_build_hash', lambda r: r['builds'][0].update(sha256='0' * 64))
    corrupted('cross_profile_omission', lambda r: r['not_run'][0].update(coord_bits=21), failed)
    for field in ('canonical_sha256', 'canonical_bytes', 'semantic'):
        value = copy.deepcopy(report)
        row = value['runs'][0]
        row[field] = '0' * 64 if field == 'canonical_sha256' else 99 if field == 'canonical_bytes' else {}
        refuse('baseline_' + field, lambda value=value: check.compare_baseline(value, baseline))
    for field in ('balls', 'levels', 'incidences', 'peak_reserved_bytes', 'reserved_after_bytes'):
        value = copy.deepcopy(report)
        value['runs'][0]['events'][1][field] += 1
        refuse('baseline_' + field, lambda value=value: check.compare_baseline(value, baseline))
    value = copy.deepcopy(report)
    value['runs'][0]['events'][1]['logical']['nodes'] += 1
    refuse('baseline_work', lambda: check.compare_baseline(value, baseline))
    value = copy.deepcopy(report)
    value['manifest_sha256'] = '0' * 64
    refuse('baseline_inputs', lambda: check.compare_baseline(value, baseline))
    for gate in ('mhgp11_num_unit_candidate', 'mhgp11_num_unit_power_paths'):
        value = copy.deepcopy(supplement)
        root = ET.fromstring(value[check.asan.PREFIX + 'junit.xml'])
        case = next(c for c in root if c.get('name') == gate)
        case.find('system-out').text = 'run_expect_verdict conforme\n'
        value[check.asan.PREFIX + 'junit.xml'] = ET.tostring(root)
        refuse('missing_' + gate + '_controls', lambda value=value: check.judge_supplement(value))
    for old, new in (('11838', '7526'), ('528', '504'), ('"bits": 18', '"bits": 24')):
        value = copy.deepcopy(supplement)
        value[check.asan.PREFIX + 'junit.xml'] = value[check.asan.PREFIX + 'junit.xml'].replace(old.encode(), new.encode())
        refuse('supplement_' + old, lambda value=value: check.judge_supplement(value))
    value = copy.deepcopy(supplement)
    summary = check.js(value[check.BASE + 'summary.json'])
    summary.update(exit_code=False)
    value[check.BASE + 'summary.json'] = encode(summary)
    refuse('supplement_bool_exit', lambda: check.judge_supplement(value))
    for meta in (dict(status='ok', exit_code='0'), dict(status='ok', group_closed='0', exit_code='0')):
        refuse('command_closure_' + str(meta.get('group_closed')), lambda meta=meta: check.command(meta, 0))
    refuse('session_failed_green', lambda: check.session(dict(status='completed', worker_exit_code=0), worker, metas))
    refuse('capture_wrong_source', lambda: capture(report, args, baseline,
        mutate=lambda receipt, worker, data: receipt.update(commit='0' * 40)))
    refuse('capture_extra_command', lambda: capture(report, args, baseline,
        mutate=lambda receipt, worker, data: data.update({'results/cmd/003_extra/meta.txt': b'status=ok'})))
    refuse('capture_supplement_unclosed', lambda: capture(report, args, baseline,
        mutate=lambda receipt, worker, data: data.update({'results/cmd/001_asan18/meta.txt': b'status=ok\nexit_code=0'})))
    refuse('capture_benchmark_after_matrix_failure', lambda: capture(report, args, baseline, matrix_code=1,
        mutate=lambda receipt, worker, data: data.update({check.BENCH + 'files/profiles.json': encode(report)})))
    require(len(cases) == 41, 'mutation floor: %d' % len(cases))
    print(json.dumps(dict(synthetic_positive_fixtures=12, corruptions_refused=len(cases), cases=cases,
                         native_calls=0), sort_keys=True))


if __name__ == '__main__':
    main()
