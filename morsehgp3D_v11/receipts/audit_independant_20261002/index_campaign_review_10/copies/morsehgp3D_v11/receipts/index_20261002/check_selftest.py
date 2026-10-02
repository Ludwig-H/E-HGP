"""Pure index-receipt tests. No native process, cloud call or historical receipt modification."""
import contextlib
import copy
from functools import lru_cache
import io
import json
from pathlib import Path
import tempfile
from unittest.mock import patch
import xml.etree.ElementTree as ET

import check


def require(value, message):
    if not value:
        raise ValueError(message)


def encode(value):
    return json.dumps(value, sort_keys=True).encode()


@lru_cache(maxsize=None)
def shape(n):
    if n <= 8:
        return 1, 1
    left, right = shape(n // 2), shape(n - n // 2)
    return 1 + left[0] + right[0], 1 + max(left[1], right[1])


def refresh(report):
    report['comparisons'] = check.comparisons(report['runs'])
    complete = report['complete'] and len(report['runs']) == 18 and all(r['status'] == 'ok' for r in report['runs'])
    report['full_schedule_completed'] = complete
    report['conforming'] = complete and all(r['status'] == 'equal' for r in report['comparisons'])
    for row in report['runs']:
        row['stdout'] = '\n'.join(json.dumps(e) for e in row['events'])


def fixture():
    cases = [dict(name=name, count=n, coordinates=name + '.xyz', point_ids=name + '.ids',
                  profile='quantized_u18_input_only', unit_site_weights=True, duplicate_sites=0)
             for name, n in check.old.CASE_COUNTS.items()]
    manifest, builds, provenance, records = dict(cases=cases), {}, {}, []
    for bits, name in check.profiles.PROFILES.items():
        cache = ('MHGP11_COORD_BITS:STRING=%d\nMHGP11_SANITIZE:BOOL=OFF\nMHGP11_TSAN:BOOL=OFF\n'
                 'MHGP11_POISON:BOOL=OFF\n' % bits)
        pin = dict(path='/synthetic/%s/build/mhgp11_index_bench' % name, coord_bits=bits, configuration=name,
                   sha256=check.sha(name.encode()), bytes=123, provenance_sha256=check.sha((name + 'provenance').encode()))
        records.append(pin)
        builds[name] = {'mhgp11_index_bench': dict(sha256=pin['sha256'], size=123),
                        'CMakeCache.txt': dict(text=cache, size=len(cache), sha256=check.sha(cache.encode()))}
        provenance[name] = pin['provenance_sha256']
    report = dict(schema=check.SCHEMA, complete=True, conforming=True, requested_runs=18, repetitions_requested=1,
                  leaf_size=8, queries_per_run=64, thresholds=[5, 10, 13], timeout_seconds=30,
                  native_schedule_bound_seconds=540, manifest=manifest, manifest_sha256='a' * 64,
                  qualification_sha256='b' * 64, supplement_sha256='c' * 64,
                  supplement_provenance_sha256='d' * 64, builds=records, runs=[], not_run=[], comparisons=[],
                  full_schedule_completed=True)
    for case in sorted(cases, key=lambda c: (c['name'].startswith('lidar'), c['count'])):
        n, name = case['count'], case['name']
        nodes, depth = shape(n)
        cloud_bytes, index_bytes = 28 * n + 8, 28 * n + 8 + 40 * nodes
        for pin in records:
            bits = pin['coord_bits']
            events = [dict(phase='cloud', sites=n, points=n, read_ns=10, cloud_ns=20,
                           peak_reserved_bytes=40 * n + 8, reserved_after_bytes=cloud_bytes),
                      dict(phase='index', status='ok', reason='none', coord_bits=bits, leaf_size=8, wall_ns=30,
                           nodes=nodes, max_depth=depth, node_bytes=40, peak_reserved_bytes=index_bytes,
                           reserved_after_bytes=index_bytes)]
            for i in range(64):
                q, threshold = 1 + i % 4, (5, 10, 13)[i % 3]
                p, u = (0, 1) if q == 1 else (threshold, 0)
                events.append(dict(phase='query', ordinal=i, arity=q, threshold=threshold, factory_ns=1,
                    status='ok', reason='none', reference_ok=True, wall_ns=3, reference_ns=5,
                    peak_reserved_bytes=index_bytes + 4 * (p + u), reserved_after_bytes=index_bytes + 4 * (p + u),
                    interior=p, shell=u, kind='complete' if q == 1 else 'saturated',
                    logical=dict(nodes=2, bounds=2, point_tests=0, inside_blocks=2, outside_blocks=0, passes=2)))
            totals = dict(queries=64, complete=16, saturated=48, degenerate=0, query_ns=192,
                          reference_ns=320, reserved_after_bytes=index_bytes)
            events += [dict(phase='summary', arities=[16] * 4, **totals), dict(phase='exit', status='ok', reason='none')]
            raw = check.sha(('%s-%d' % (name, bits)).encode())
            row = dict(case=name, coord_bits=bits, repetition=0, count=n, whole_input=True, status='ok', exit_code=0,
                       timeout_seconds=30, argv=[pin['path'], case['coordinates'], case['point_ids'],
                       '%s_b%d.bin' % (name, bits), str(check.BUDGET)], events=events, errors=[], stdout='', stderr='',
                       process_wall_seconds=0.01, semantic_wall_seconds=0.001, canonical_sha256=raw, canonical_bytes=5000,
                       semantic=dict(sha256=check.sha(name.encode()), raw_sha256=raw, bytes=5000, **totals))
            report['runs'].append(row)
    refresh(report)
    arguments = (manifest, 'a' * 64, 'b' * 64, builds, provenance, 'c' * 64, 'd' * 64)
    return report, arguments


def remove_artifacts(row):
    for key in ('semantic', 'canonical_sha256', 'canonical_bytes', 'semantic_wall_seconds'):
        row.pop(key, None)


def fail(report, state='timeout', index=0):
    row = report['runs'][index]
    remove_artifacts(row)
    row.update(status=state, exit_code=None, stdout='', stderr='', events=[], errors=[])
    stage = dict(timeout='process', launch_error='launch', invalid_output='success', artifact_error='artifact').get(state)
    if stage:
        row['errors'] = [dict(stage=stage, type='SyntheticError', message='fixture failure')]
    if state in ('invalid_output', 'artifact_error'):
        row['exit_code'] = 0
    elif state == 'refused':
        row['exit_code'] = 2
    elif state == 'failed':
        row['exit_code'] = 3
    refresh(report)


def partial(report, state):
    all_rows = report['runs']
    report.update(complete=False, runs=all_rows[:1], not_run=[dict(case=r['case'], coord_bits=r['coord_bits'],
                  repetition=0, reason='pending') for r in all_rows[1:]])
    row = report['runs'][0]
    remove_artifacts(row)
    row['status'] = state
    if state == 'running':
        row.update(exit_code=None, events=[], stdout='', stderr='', errors=[])
        row.pop('process_wall_seconds')
    refresh(report)


def fraction_value(name, bits=18):
    value = dict(bits=bits, input_sha256='e' * 64)
    if name.startswith('mhgp11_num_bounds'):
        value.update(cases=391, checks=3400, valid=354, degenerate=34, refused=3, contacts=87,
                     inside=30, outside=137, loose=128, wide={18: 0, 21: 7, 24: 35}[bits])
    elif name.startswith('mhgp11_num_'):
        value.update(checks=11838, geometry=528, degeneracies=50, integers=160)
    else:
        value.update(requests=1010, checks=36020, permutations=47, kinds=dict(complete=302, saturated=702, refused=6))
    return value


def supplement_fixture(failed=False):
    names = sorted(check.REQUIRED) + ['synthetic_%d' % i for i in range(36 - len(check.REQUIRED))]
    tests = [dict(name=n, disabled=False, labels=['unit', 'fast']) for n in names]
    root = ET.Element('testsuite', tests='36', failures=str(int(failed)), disabled='0', skipped='0')
    for name in names:
        bad = failed and name == 'synthetic_0'
        case = ET.SubElement(root, 'testcase', name=name, status='fail' if bad else 'run')
        if bad:
            ET.SubElement(case, 'failure').text = 'synthetic failure'
        if name in check.UNIT_GATES:
            group, count, floor = check.UNIT_GATES[name]
            output = 'test %s controles=%d echecs=0 plancher=%d\nmhgp11_test_ok tests=1 controles=%d\n' % (
                group, count, floor, count)
        elif name in check.BENCH_GATES:
            output = check.BENCH_GATES[name] + '\n'
        elif name in check.REQUIRED:
            output = json.dumps(fraction_value(name)) + '\n'
        else:
            output = ''
        ET.SubElement(case, 'system-out').text = output + 'run_expect_verdict conforme\n'
    config = dict(name=check.asan.NAME, status='failed' if failed else 'ok', conforming=not failed,
                  tests=dict(selected=36, passed=36 - int(failed), failed=int(failed), not_run=0,
                             ctest_total=36, ctest_failed=int(failed)),
                  passed_labels=dict(unit=36 - int(failed), fast=36 - int(failed)), steps=[])
    for name in ('configure', 'build', 'list', 'test'):
        bad = failed and name == 'test'
        config['steps'].append(dict(name=name, status='failed' if bad else 'ok', exit_code=int(bad)))
    summary = dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True, requested=[check.asan.NAME],
                   configurations=[config], statuses={check.asan.NAME: config['status']}, signals=[],
                   exit_code=int(failed), conforming=not failed)
    files = []
    for name in sorted(check.asan.BINARIES | check.asan.BUILD_FILES):
        if name == 'CMakeCache.txt':
            text = ''.join('%s:STRING=%s\n' % (k, v) for k, v in check.asan.CACHE.items())
        elif name.endswith('flags.make'):
            text = 'CXX_FLAGS = -fsanitize=address,undefined\n'
        elif name.endswith('link.txt'):
            text = 'synthetic link\n'
        else:
            text = None
        payload = (text or 'synthetic binary').encode()
        row = dict(path=name, sha256=check.sha(payload), size=len(payload))
        if text is not None:
            row['text'] = text
        files.append(row)
    prefix = check.asan.PREFIX
    provenance = dict(schema='ehgp.v11.build_provenance.v1', complete=True, errors=[], files=files)
    return {check.BASE + 'summary.json': encode(summary), prefix + 'result.json': encode(config),
            prefix + 'tests.json': encode(tests), prefix + 'junit.xml': ET.tostring(root),
            prefix + 'build_provenance.json': encode(provenance)}


def capture(report, args, matrix_code=0, mutate=None):
    # Transport and the main nine-configuration fixture alone are mocked. Actual supplement, report, metadata
    # and session checks execute; historical transport readers retain their own synthetic/live gates.
    report = copy.deepcopy(report)
    supplement = supplement_fixture()
    data = {check.SUPPLEMENT + k[len(check.BASE):]: v for k, v in supplement.items()}
    data[check.BASE + 'summary.json'] = b'{}'
    report['qualification_sha256'] = check.sha(b'{}')
    report['supplement_sha256'] = check.sha(data[check.SUPPLEMENT + 'summary.json'])
    report['supplement_provenance_sha256'] = check.sha(data[check.SUPPLEMENT + check.asan.NAME + '/build_provenance.json'])
    for pin in report['builds']:
        name = pin['configuration']
        payload = encode(dict(configuration=name))
        data[check.BASE + name + '/build_provenance.json'] = payload
        pin['provenance_sha256'] = check.sha(payload)
    if matrix_code == 0:
        data[check.BENCH + 'files/index.json'] = encode(report)
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
    with tempfile.TemporaryDirectory(prefix='ehgp_index_reader_') as tmp:
        folder = Path(tmp)
        (folder / 'matrix.json').write_bytes(data[check.BASE + 'summary.json'])
        (folder / 'asan18.json').write_bytes(data[check.SUPPLEMENT + 'summary.json'])
        if check.BENCH + 'files/index.json' in data:
            (folder / 'index.json').write_bytes(data[check.BENCH + 'files/index.json'])
        with patch.object(check.old, 'read_capture', return_value=(receipt, worker, data)), \
                patch.object(check.old, 'inputs', return_value=args[:2]), \
                patch.object(check, 'judge_matrix', return_value=({}, args[3], matrix_code)), \
                contextlib.redirect_stdout(io.StringIO()):
            return check.check(folder)


def main():
    report, args = fixture()
    positive = 0

    def accepted(value, conforming=False):
        nonlocal positive
        require(check.judge_report(value, *args)['conforming'] is conforming, 'verdict witness')
        positive += 1

    accepted(report, True)
    for state in ('timeout', 'failed', 'refused', 'launch_error', 'invalid_output', 'artifact_error'):
        value = copy.deepcopy(report)
        fail(value, state)
        accepted(value)
    for state in ('running', 'pending_semantic'):
        value = copy.deepcopy(report)
        partial(value, state)
        accepted(value)
    artifact = copy.deepcopy(report)
    artifact['runs'][0].update(status='artifact_error', errors=[dict(stage='cleanup', type='OSError', message='kept')])
    refresh(artifact)
    accepted(artifact)
    divergent = copy.deepcopy(report)
    divergent['runs'][0]['semantic']['sha256'] = 'f' * 64
    refresh(divergent)
    accepted(divergent)
    require(check.judge_supplement(supplement_fixture()) == ((36, 36, 0, 0), len(check.REQUIRED), 0), 'supplement')
    require(check.judge_supplement(supplement_fixture(True))[2] == 1, 'supplement failure retained')
    require(not capture(report, args), 'assembled success')
    require(capture(report, args, 1), 'failed matrix excludes benchmark')
    require(capture(divergent, args), 'assembled semantic disagreement')
    positive += 5
    mutations = []

    def refuse(name, function):
        try:
            function()
        except (check.old.foundation.Refusal, check.asan.old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError):
            mutations.append(name)
        else:
            raise ValueError(name + ' accepted')

    def corrupted(name, action, source=report):
        value = copy.deepcopy(source)
        action(value)
        refuse(name, lambda: check.judge_report(value, *args))

    def native_change(value, index, key, new):
        value['runs'][0]['events'][index][key] = new
        value['runs'][0]['stdout'] = '\n'.join(json.dumps(e) for e in value['runs'][0]['events'])

    for key in ('manifest_sha256', 'qualification_sha256', 'supplement_sha256', 'supplement_provenance_sha256'):
        corrupted(key, lambda r, key=key: r.update({key: '0' * 64}))
    corrupted('schema', lambda r: r.update(schema='other'))
    corrupted('leaf', lambda r: r.update(leaf_size=16))
    corrupted('query_count', lambda r: r.update(queries_per_run=63))
    corrupted('duplicate_profile', lambda r: r['builds'][1].update(coord_bits=18))
    corrupted('binary_hash', lambda r: r['builds'][0].update(sha256='0' * 64))
    corrupted('provenance_hash', lambda r: r['builds'][0].update(provenance_sha256='0' * 64))
    corrupted('wrong_command', lambda r: r['runs'][0]['argv'].__setitem__(0, '/other'))
    corrupted('missing_attempt', lambda r: r['runs'].pop(0))
    corrupted('duplicate_attempt', lambda r: r['runs'].append(copy.deepcopy(r['runs'][0])))
    corrupted('false_complete', lambda r: r.update(conforming=True), divergent)
    corrupted('hidden_difference', lambda r: r.update(comparisons=report['comparisons']), divergent)
    corrupted('profile_native', lambda r: native_change(r, 1, 'coord_bits', 21))
    corrupted('negative_duration', lambda r: native_change(r, 2, 'wall_ns', -1))
    corrupted('cloud_memory', lambda r: native_change(r, 0, 'reserved_after_bytes', 1))
    corrupted('index_memory', lambda r: native_change(r, 1, 'peak_reserved_bytes', 1))
    corrupted('node_size', lambda r: native_change(r, 1, 'node_bytes', 32))
    corrupted('query_memory', lambda r: native_change(r, 2, 'peak_reserved_bytes', 1))
    corrupted('scan_failure', lambda r: native_change(r, 2, 'reference_ok', False))
    corrupted('query_identity', lambda r: native_change(r, 2, 'ordinal', 1))
    corrupted('wrong_threshold', lambda r: native_change(r, 2, 'threshold', 7))
    corrupted('summary_clock', lambda r: native_change(r, -2, 'query_ns', 193))
    corrupted('semantic_clock', lambda r: r['runs'][0]['semantic'].update(query_ns=193))
    corrupted('raw_hash', lambda r: r['runs'][0].update(canonical_sha256='0' * 64))
    corrupted('stderr_success', lambda r: r['runs'][0].update(stderr='UBSan fixture'))
    corrupted('bool_exit', lambda r: r['runs'][0].update(exit_code=False))
    corrupted('nonfinite_time', lambda r: r['runs'][0].update(process_wall_seconds=float('nan')))
    pending = copy.deepcopy(report)
    partial(pending, 'pending_semantic')
    corrupted('pending_green', lambda r: r.update(conforming=True), pending)
    corrupted('pending_complete', lambda r: r.update(complete=True), pending)
    corrupted('pending_hash', lambda r: r['runs'][0].update(canonical_sha256='a' * 64), pending)
    corrupted('pending_queue_lost', lambda r: r['not_run'].pop(), pending)
    corrupted('pending_queue_reordered', lambda r: r['not_run'].reverse(), pending)
    corrupted('pending_omission_reason', lambda r: r['not_run'][0].update(reason='same_profile_failed'), pending)
    for key in ('MHGP11_COORD_BITS', 'MHGP11_MODULES', 'MHGP11_SANITIZE', 'MHGP11_TSAN', 'MHGP11_POISON'):
        data = supplement_fixture()
        path = check.asan.PREFIX + 'build_provenance.json'
        provenance = check.js(data[path])
        row = next(r for r in provenance['files'] if r['path'] == 'CMakeCache.txt')
        row['text'] = row['text'].replace(key + ':STRING=' + check.asan.CACHE[key], key + ':STRING=wrong')
        row.update(size=len(row['text']), sha256=check.sha(row['text'].encode()))
        data[path] = encode(provenance)
        refuse('supplement_' + key, lambda data=data: check.judge_supplement(data))
    for gate in ('mhgp11_num_unit_bounds', 'mhgp11_num_bounds_fraction', 'mhgp11_index_unit_concurrency',
                 'mhgp11_index_fault_starvation', 'mhgp11_index_fraction', 'mhgp11_index_bench_io'):
        data = supplement_fixture()
        root = ET.fromstring(data[check.asan.PREFIX + 'junit.xml'])
        next(c for c in root if c.get('name') == gate).find('system-out').text = 'run_expect_verdict conforme\n'
        data[check.asan.PREFIX + 'junit.xml'] = ET.tostring(root)
        refuse('supplement_gate_' + gate, lambda data=data: check.judge_supplement(data))
    for target in ('mhgp11_index_probe', 'mhgp11_num_bounds_probe', 'mhgp11_index_fault'):
        data = supplement_fixture()
        path = check.asan.PREFIX + 'build_provenance.json'
        value = check.js(data[path])
        value['files'] = [r for r in value['files'] if r['path'] != target]
        data[path] = encode(value)
        refuse('supplement_binary_' + target, lambda data=data: check.judge_supplement(data))
    refuse('source_pin', lambda: capture(report, args, mutate=lambda r, w, d: r.update(commit='0' * 40)))
    refuse('session_verdict', lambda: capture(report, args, mutate=lambda r, w, d: w.update(commands_ok='2')))
    refuse('open_benchmark', lambda: capture(report, args, mutate=lambda r, w, d:
           d.update({check.BENCH + 'meta.txt': b'status=ok\nexit_code=0\ngroup_closed=0\n'})))
    root = ET.Element('testsuite')
    for module, identities in check.MUTANTS.items():
        case = ET.SubElement(root, 'testcase', name='mhgp11_mutants_' + module, status='run')
        n = len(identities)
        ET.SubElement(case, 'system-out').text = ('\n'.join(i + ' TUE code' for i in identities) + '\n' +
            'mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=0 plancher=%d\n' %
            (module, n, n, n) + 'run_expect_verdict conforme\n')
    full = ''
    for i, case in enumerate(root):
        name, output = case.get('name'), case.findtext('system-out')
        full += ('%d/2 Testing: %s\n%d/2 Test: %s\nCommand: synthetic\nOutput:\n' % (i + 1, name, i + 1, name) +
                 '----------------------------------------------------------\n' + output +
                 '<end of output>\nTest time = 0 sec\nTest Passed.\n')
    data = {check.BASE + 'mutants/junit.xml': ET.tostring(root),
            check.BASE + 'mutants/LastTest.log': full.encode()}
    check.causal_mutants(dict(name='mutants', status='ok', tests={}), data)
    positive += 1
    first = root[0].find('system-out')
    original = first.text
    first.text = original[:100] + '...\nThe rest of the test output was removed since it exceeds the threshold of 1024 bytes.\n'
    data[check.BASE + 'mutants/junit.xml'] = ET.tostring(root)
    check.causal_mutants(dict(name='mutants', status='ok', tests={}), data)
    positive += 1
    path = check.BASE + 'mutants/LastTest.log'
    for label, bad in [('LastTest_duplicate', full + full), ('LastTest_wrong_name', full.replace('Testing: mhgp11_mutants_num',
                        'Testing: another_test')), ('LastTest_failed', full.replace('Test Passed.', 'Test Failed.', 1)),
                       ('LastTest_prefix', full.replace(' TUE code', ' SURVIT code', 1))]:
        corrupt = dict(data)
        corrupt[path] = bad.encode()
        refuse(label, lambda corrupt=corrupt: check.causal_mutants(dict(name='mutants', status='ok', tests={}), corrupt))
    first.text = original
    first = root[0].find('system-out')
    first.text = first.text.replace(' TUE code', ' TUE signal', 1)
    data[check.BASE + 'mutants/junit.xml'] = ET.tostring(root)
    refuse('mutant_signal_claimed_causal', lambda: check.causal_mutants(dict(name='mutants', status='ok', tests={}), data))
    print(json.dumps(dict(positive=positive, corruptions=len(mutations), native_calls=0, cases=mutations), sort_keys=True))


if __name__ == '__main__':
    main()
