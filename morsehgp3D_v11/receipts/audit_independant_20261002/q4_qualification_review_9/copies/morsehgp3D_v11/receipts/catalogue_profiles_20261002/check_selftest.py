"""Synthetic receipt-JSON controls only: no native executable, archive mutation or cloud access."""
import copy
import json
import math

import check


def require(value, message):
    if not value:
        raise ValueError(message)


def comparisons(rows):
    result = []
    for name in sorted(check.old.CASE_COUNTS):
        for kmax in (5, 10):
            matches = [r for r in rows if (r['case'], r['kmax'], r['status']) == (name, kmax, 'ok')]
            hashes = {r['semantic']['sha256'] for r in matches}
            counts = {json.dumps(r['events'][1]['logical'], sort_keys=True) for r in matches}
            agree, work = len(hashes) <= 1, len(counts) <= 1
            result.append(dict(case=name, kmax=kmax, successful_profiles=[r['coord_bits'] for r in matches],
                               semantic_equal=agree, geometric_work_equal=work,
                               status='different' if not agree or not work else 'equal' if len(matches) == 3
                               else 'incomplete'))
    return result


def fixture():
    cases = [dict(name=name, count=count, coordinates=name + '.u32le', point_ids=name + '.ids.u32le',
                  profile='quantized_u18_input_only', unit_site_weights=True, duplicate_sites=0)
             for name, count in check.old.CASE_COUNTS.items()]
    manifest = dict(cases=cases)
    builds = {name: {'mhgp11_catalogue_bench': {'sha256': str(i + 1) * 64, 'size': 123},
                     'CMakeCache.txt': {'sha256': 'e' * 64}} for i, name in enumerate(check.PROFILES.values())}
    provenance = dict.fromkeys(check.PROFILES.values(), 'f' * 64)
    pins = [dict(coord_bits=bits, configuration=name, sha256=builds[name]['mhgp11_catalogue_bench']['sha256'],
                 bytes=123, provenance_sha256=provenance[name], cache_sha256='e' * 64,
                 path='/tmp/matrix/' + name + '/build/mhgp11_catalogue_bench') for bits, name in check.PROFILES.items()]
    report = dict(schema='ehgp.v11.catalogue_profiles.v1', attempt_schema='ehgp.v11.catalogue_attempt.v2',
                  manifest=manifest, manifest_sha256='a' * 64, qualification_sha256='b' * 64,
                  requested_runs=36, repetitions_requested=1, leaf_size=16, timeout_seconds=30,
                  native_schedule_bound_seconds=1080, complete=True, full_schedule_completed=True,
                  conforming=True, all_attempted_ok=True, builds=pins, runs=[], not_run=[])
    for case in sorted(cases, key=lambda c: (c['name'].startswith('lidar'), c['count'])):
        for pin in pins:
            bits = pin['coord_bits']
            for kmax in (5, 10):
                events = [dict(phase='cloud', points=case['count'], sites=case['count'], read_ns=1, cloud_ns=2,
                               cloud_peak_bytes=100),
                          dict(phase='catalogue', status='ok', coord_bits=bits, kmax=kmax, balls=11, levels=4,
                               incidences=28, generation_passes=2, logical=dict.fromkeys(sorted(check.LOGICAL), 5),
                               wall_ns=300, cpu_seconds=0.001, peak_reserved_bytes=200, reserved_after_bytes=100),
                          dict(phase='exit', status='ok')]
                row = dict(case=case['name'], coord_bits=bits, kmax=kmax, repetition=0, count=case['count'],
                           whole_input=True, timeout_seconds=30, process_wall_seconds=0.002, exit_code=0,
                           status='ok', errors=[], stderr='', events=events, stdout='',
                           catalogue_ms=300 / 1e6, cloud_ms=2 / 1e6, read_ms=1 / 1e6, catalogue_within_100ms=True,
                           canonical_sha256=str(bits // 10) * 64, canonical_bytes=2048, semantic_wall_seconds=0.01,
                           semantic=dict(schema='ehgp.v11.catalogue_semantic.v1', sha256='c' * 64,
                                         coord_bits=bits, kmax=kmax, sites=case['count'], balls=11, levels=4, incidences=28),
                           argv=[pin['path'], '/data/' + case['coordinates'], '/data/' + case['point_ids'],
                                 '/work/%s_b%d_k%d.bin' % (case['name'], bits, kmax), str(kmax), '16', '256', '0',
                                 str(2**32 - 1), str(8 * 1024**3)])
                row['stdout'] = '\n'.join(json.dumps(e) for e in events)
                report['runs'].append(row)
    report['comparisons'] = comparisons(report['runs'])
    return report, (manifest, 'a' * 64, 'b' * 64, builds, provenance)


def refresh(report):
    for row in report['runs']:
        row['stdout'] = '\n'.join(json.dumps(e) for e in row['events'])
    report['comparisons'] = comparisons(report['runs'])
    report['all_attempted_ok'] = all(r['status'] == 'ok' for r in report['runs'])
    report['full_schedule_completed'] = len(report['runs']) == 36 and report['all_attempted_ok']
    report['conforming'] = report['full_schedule_completed'] and all(c['status'] == 'equal' for c in report['comparisons'])


def remove_artifact(row):
    for key in ('semantic', 'canonical_sha256', 'canonical_bytes', 'semantic_wall_seconds', 'catalogue_ms',
                'cloud_ms', 'read_ms', 'catalogue_within_100ms'):
        row.pop(key, None)


def failure(report):
    row = report['runs'][0]
    row.update(status='timeout', exit_code=None, errors=[dict(stage='process', type='TimeoutExpired', message='timeout')])
    remove_artifact(row)
    omitted = report['runs'].pop(1)
    report['not_run'].append({**{k: omitted[k] for k in ('case', 'coord_bits', 'kmax', 'repetition')},
                              'reason': 'same_profile_K5_failed'})
    refresh(report)


def main():
    original, arguments = fixture()
    require(check.judge_report(original, *arguments)['conforming'], 'positive complete witness')
    failed = copy.deepcopy(original)
    failure(failed)
    require(not check.judge_report(failed, *arguments)['conforming'], 'failed campaign preserved')
    different = copy.deepcopy(failed)
    different['runs'][3]['semantic']['sha256'] = 'd' * 64  # B24/K5 disagrees with B21, while B18 timed out.
    refresh(different)
    require(check.judge_report(different, *arguments)['different'] == 1, 'two surviving profiles compared')
    partial = copy.deepcopy(original)
    partial.update(complete=False, runs=partial['runs'][:1], not_run=[])
    pending = partial['runs'][0]
    pending['status'] = 'pending_semantic'
    remove_artifact(pending)
    refresh(partial)
    require(not check.judge_report(partial, *arguments)['conforming'], 'pending semantic checkpoint')
    artifact = copy.deepcopy(original)
    artifact['runs'][-1].update(status='artifact_error', errors=[dict(stage='cleanup', type='OSError', message='cleanup')])
    refresh(artifact)
    require(not check.judge_report(artifact, *arguments)['conforming'], 'cleanup error preserves failure')
    corruptions = []

    def corrupt(name, callback, source=original):
        value = copy.deepcopy(source)
        callback(value)
        corruptions.append((name, value))

    def native_value(report, key, value):
        report['runs'][0]['events'][1][key] = value
        report['runs'][0]['stdout'] = '\n'.join(json.dumps(e) for e in report['runs'][0]['events'])

    def hidden_semantics(report):
        match = next(c for c in report['comparisons'] if c['status'] == 'different')
        match.update(status='incomplete', semantic_equal=True)

    def hidden_work(report):
        logical = dict(report['runs'][0]['events'][1]['logical'], nodes=6)
        native_value(report, 'logical', logical)

    corrupt('duplicate_run', lambda r: r['runs'].append(copy.deepcopy(r['runs'][0])))
    corrupt('missing_run', lambda r: r['runs'].pop())
    corrupt('unknown_profile', lambda r: r['runs'][0].update(coord_bits=27))
    corrupt('wrong_executable', lambda r: r['runs'][0]['argv'].__setitem__(0, '/tmp/wrong'))
    corrupt('wrong_binary_hash', lambda r: r['builds'][0].update(sha256='0' * 64))
    corrupt('wrong_cache_hash', lambda r: r['builds'][0].update(cache_sha256='0' * 64))
    corrupt('wrong_provenance_hash', lambda r: r['builds'][0].update(provenance_sha256='0' * 64))
    corrupt('wrong_manifest_hash', lambda r: r.update(manifest_sha256='0' * 64))
    corrupt('wrong_qualification_hash', lambda r: r.update(qualification_sha256='0' * 64))
    corrupt('wrong_leaf', lambda r: r['runs'][0]['argv'].__setitem__(5, '32'))
    corrupt('wrong_native_bits', lambda r: native_value(r, 'coord_bits', 24))
    corrupt('wrong_semantic_bits', lambda r: r['runs'][0]['semantic'].update(coord_bits=24))
    corrupt('nonfinite_duration', lambda r: r['runs'][0].update(process_wall_seconds=math.inf))
    corrupt('cross_profile_omission', lambda r: r['not_run'][0].update(coord_bits=21), failed)
    corrupt('unjustified_omission', lambda r: r['not_run'][0].update(reason='other'), failed)
    corrupt('failed_green', lambda r: r.update(conforming=True, full_schedule_completed=True), failed)
    corrupt('semantic_disagreement_hidden', hidden_semantics, different)
    corrupt('work_disagreement_hidden', hidden_work)
    corrupt('partial_green', lambda r: r.update(conforming=True, full_schedule_completed=True), partial)
    corrupt('pending_claimed_complete', lambda r: r.update(complete=True), partial)
    corrupt('pending_with_hash', lambda r: r['runs'][0].update(canonical_sha256='c' * 64), partial)
    corrupt('partial_code_signal', lambda r: r['runs'][0].update(exit_code=-15), partial)
    for name, report in corruptions:
        try:
            check.judge_report(report, *arguments)
        except (check.old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError):
            pass
        else:
            raise ValueError(name + ' accepted')
    check.bench_group_closed({'group_closed': '1'}, True)
    closure_cases = []
    for name, meta in [('missing_group_closed', {}), ('group_not_closed', {'group_closed': '0'})]:
        try:
            check.bench_group_closed(meta, True)
        except check.old.foundation.Refusal:
            closure_cases.append(name)
        else:
            raise ValueError(name + ' accepted')
    require(len(corruptions) == 22 and len(closure_cases) == 2, 'corruption floor')
    print(json.dumps({'synthetic_positive_fixtures': 5, 'corruptions_refused': len(corruptions) + len(closure_cases),
                      'cases': [name for name, _ in corruptions] + closure_cases, 'native_calls': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
