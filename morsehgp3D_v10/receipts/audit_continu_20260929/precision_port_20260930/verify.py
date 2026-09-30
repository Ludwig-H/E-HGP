"""Read-only verification of this closed audit, not an engine validator."""
from __future__ import annotations

from contextlib import redirect_stdout
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys


sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def read_json(path):
    return json.loads(path.read_text(), object_pairs_hook=unique_object,
                      parse_constant=lambda value: (_ for _ in ()).throw(
                          ValueError('nonfinite JSON: ' + value)))


def manifest(folder):
    entries = set()
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        require(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest),
                'invalid digest')
        path = folder / name
        require(not Path(name).is_absolute() and '..' not in Path(name).parts,
                'invalid manifest path')
        require(name not in entries and path.is_file() and not path.is_symlink(),
                'missing, duplicate or linked path: ' + name)
        require(hashlib.sha256(path.read_bytes()).hexdigest() == digest,
                'SHA mismatch: ' + str(path))
        entries.add(name)
    actual = {str(path.relative_to(folder)) for path in folder.rglob('*')
              if path.is_file() and path != folder / 'SHA256SUMS'}
    require(entries == actual, 'incomplete manifest: ' + str(folder))
    return len(entries)


def validate_native_fields(actual, mode, bits):
    require(actual['mode'] == mode and actual['bits'] == bits,
            'native fixture identity')
    require(type(actual['bits']) is int and type(actual['support']) is list,
            'native fixture schema')
    if mode == 'orient':
        require(type(actual['orientations']) is list and len(actual['orientations']) == 3,
                'missing orientations')
        return
    require(type(actual['constructed']) is bool and
            set(actual['center']) == {'N', 'D'} and len(actual['center']['N']) == 3,
            'missing center')
    if mode in ('level2', 'level4'):
        require(set(actual['level']) == {'num', 'den'}, 'missing level')
    if mode == 'box3':
        require(type(actual['ownership']) is bool and set(actual['box']) == {'lo', 'hi'},
                'missing owner')
    if mode in ('side2', 'side3'):
        require(type(actual['queries']) is list and
                len(actual['queries']) == (3 if mode == 'side2' else 1), 'missing queries')
        for query in actual['queries']:
            require(set(query) == {'xyz', 'side', 'key'}, 'query schema')


def geometry():
    root = ROOT / 'geometry'
    rec = read_json(root / 'receipt.json')
    require(len(rec['native']) == 24 and len(rec['judges']) == 40, 'geometry panel')
    require(all(command['exit_code'] == 0 for command in rec['compile']), 'compile failure')
    spec = importlib.util.spec_from_file_location('closed_geometry_oracle', root / 'oracle.py')
    oracle = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(oracle)
    observations = {}
    for call in rec['native']:
        key = (call['kind'], call['mode'], call['bits'])
        require(key not in observations, 'duplicate native call')
        if call['exit_code'] == 0:
            require(not call['ub_reported'], 'successful call with UB diagnostic')
            actual = read_json(root / call['log'])
            validate_native_fields(actual, call['mode'], call['bits'])
            output = io.StringIO()
            with redirect_stdout(output):
                rc = oracle.judge(actual)
            judged = json.loads(output.getvalue())
            observations[key] = (rc, judged)
        else:
            require(call['kind'] == 'ubsan' and call['exit_code'] == 1 and
                    call['ub_reported'] and 'runtime error: signed integer overflow' in
                    (root / call['log']).read_text(), 'unexpected failed call')
    require(len(observations) == 20, 'successful native output count')
    seen = set()
    for call in rec['judges']:
        key = (call['kind'], call['mode'], call['bits'])
        pair_key = (call['python'], *key)
        require(pair_key not in seen, 'duplicate judgment')
        seen.add(pair_key)
        rc, judged = observations[key]
        require(call['exit_code'] == rc and call['result'] == judged['status'] and
                read_json(root / call['log']) == judged, 'archived judgment mismatch')
    require(seen == {(python, *key) for python in ('normal', 'optimized')
                     for key in observations}, 'missing normal/optimized pair')
    matches = sum(rc == 0 for rc, _ in observations.values())
    require(matches == 12 and len(observations) - matches == 8, 'geometry findings changed')
    for kind in ('normal', 'ubsan'):
        _, outcome = observations[kind, 'level4', 24]
        require(outcome['actual']['level']['num'] == '0' and
                outcome['expected']['radius2'] == '844424829468675/4', 'silent q4 case')
    # These schema refusals are mandatory even though the old diagnostic
    # oracle does not reject omission of every optional-looking field.
    schema_refusals = 0
    for mode, field in [('level4', 'level'), ('box3', 'ownership'), ('side3', 'queries')]:
        actual = dict(observations['normal', mode, 18][1]['actual'])
        del actual[field]
        try:
            validate_native_fields(actual, mode, 18)
        except (RuntimeError, KeyError):
            schema_refusals += 1
    require(schema_refusals == 3, 'missing-field mutations survived')
    return dict(native_calls=24, successful_outputs=20, ubsan_stops=4,
                fraction_matching=12, fraction_mismatching=8,
                archived_judgments=40, schema_omission_refusals=schema_refusals)


def scalar_filters():
    root = ROOT / 'filters_scalar'
    normal = read_json(root / 'normal.json')
    require(normal == read_json(root / 'optimized.json') and
            normal['status'] == 'PASS' and normal['checks'] == 614 and not normal['failures'],
            'scalar panel changed')
    require(read_json(root / 'preflight_reduced_expectation.normal.json') ==
            read_json(root / 'preflight_reduced_expectation.optimized.json') and
            read_json(root / 'preflight_reduced_expectation.normal.json')['status'] == 'FAIL',
            'rejected preflight lost')
    return dict(checks_per_pass=614, rejected_preflight_preserved=True,
                native_compilation=False)


def interfaces():
    root = ROOT / 'interfaces'
    rec = read_json(root / 'report.json')
    require(rec['source_hash_count'] == 28 and rec['hashes_stable'] is True and
            (root / 'hashes_before.sha256').read_bytes() ==
            (root / 'hashes_after.sha256').read_bytes(), 'interface closure')
    return dict(source_hashes_observed=28, engine_executions=0)


def native_filters():
    root = ROOT / 'filters_native'
    rec = read_json(root / 'receipt.json')
    require(rec['schema'] == 'mhgp10_site_filter_native_receipt_v1' and
            rec['gcp_used'] is False and rec['repository_files_changed'] is False,
            'native filter scope')
    execution = rec['execution']
    require(execution['native_program_invocations'] == 3 and
            execution['actual_closed_ball_queries'] == 9 and
            execution['wrapper_preparation_failure']['native_executed'] is False,
            'native chronology')
    require(rec['provenance']['private_sources_before'] ==
            rec['provenance']['private_sources_after'], 'native source closure')
    for source in rec['provenance']['dependencies']:
        require(source['sha256_before'] == source['sha256_after'] == source['copied_sha256'] ==
                hashlib.sha256((root / 'source' / source['path']).read_bytes()).hexdigest(),
                'native copied body mismatch')
    actual = read_json(root / 'normal.stdout.json')
    require(actual == read_json(root / 'ubsan.stdout.json') ==
            read_json(root / 'normal_initial.combined.txt'), 'native outputs differ')
    require(actual['checks'] == 59 and actual['gate_failures'] == 0 and
            actual['key_controls'] == 9 and actual['expected_failures_confirmed'] == 3,
            'native classification floors')
    for mode, capture in [('normal', 'captured_normal_repeat'), ('ubsan', 'captured_ubsan')]:
        require(execution['compile_' + mode]['completion']['exit_code'] == 0 and
                execution[capture]['exit_code'] == 0 and execution[capture]['stderr'] == '' and
                json.loads(execution[capture]['stdout']) == actual and
                (root / (mode + '.stderr.txt')).read_text() == '', 'native final pair')
    spec = importlib.util.spec_from_file_location('closed_native_filter_judge', root / 'judge.py')
    judge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(judge)
    output = io.StringIO()
    with redirect_stdout(output):
        rc = judge.judge(actual)
    judged = json.loads(output.getvalue())
    require(rc == 0 and judged['checks'] == 72 and
            judged['status'] == 'EXPECTED_FAILURES_CONFIRMED', 'native Fraction judgment')
    seen = set()
    for call in execution['judges']:
        key = (call['mode'], call['optimized'])
        require(key not in seen and call['exit_code'] == 0 and
                json.loads(call['output']) == judged, 'native archived judgment')
        seen.add(key)
        suffix = 'optimized' if call['optimized'] else 'normal'
        require(read_json(root / ('judge_' + call['mode'] + '_' + suffix + '.stdout.json')) ==
                judged, 'native judge output file')
    require(seen == {(mode, optimized) for mode in ('normal', 'ubsan')
                     for optimized in (False, True)}, 'native judge pairing')
    return dict(native_program_invocations=3, closed_ball_queries=9,
                fixtures=3, native_checks_per_invocation=59, judge_checks_per_pass=72,
                archived_judge_passes=4, conclusion='expected errors confirmed outside u18')


def metric():
    root = ROOT / 'grid_metric'
    execution = read_json(root / 'execution.json')
    require(execution['status'] == 'PASS' and execution['GCP_used'] is False and
            execution['source_before'] == execution['source_after'] and
            all(command['returncode'] == 0 for command in execution['commands']), 'metric capture')
    normal = read_json(root / 'normal.stdout')
    optimized = read_json(root / 'optimized.stdout')
    require(normal.pop('optimize_flag') == 0 and optimized.pop('optimize_flag') == 1 and
            normal == optimized and normal['status'] == 'PASS', 'metric differential')
    counts = dict(coordinate_roundings=117, raw_mapping_entries=39, radius_pairs=183,
                  fused_profiles=2, shifted_cut_profiles=2, origin_outside_u32_profiles=6)
    require(normal['counts'] == counts, 'metric panel')
    require(all(row['fixed_raw_ID_universe_only'] is True and
                row['deduplicated_site_KNN_equivalence_claimed'] is False
                for row in normal['observations']), 'metric scope')
    return counts


def metric_review():
    root = ROOT / 'grid_metric_review'
    rec = read_json(root / 'receipt.json')
    require(rec['GCP_used'] is False and rec['native_engine_executed'] is False and
            rec['original_packet_modified'] is False and len(rec['commands']) == 2 and
            all(call['rc'] == 0 for call in rec['commands']), 'metric review scope')
    for name, digest in rec['original_sha256'].items():
        require(hashlib.sha256((ROOT / 'grid_metric' / name).read_bytes()).hexdigest() == digest,
                'metric review provenance')
    for name in ('normal', 'optimized'):
        require(read_json(root / (name + '.output')) ==
                read_json(ROOT / 'grid_metric' / (name + '.stdout')), 'metric rerun differs')
    require(read_json(root / 'findings.json')['status'] ==
            'NO_PROOF_DEFECT_FOUND_WITH_STATED_SCOPE', 'metric review findings')
    return dict(independent_python_reruns=2, same_bounded_panel=True,
                native_engine_executions=0)


def main():
    inner_only = sys.argv[1:] == ['--inner-only']
    require(inner_only or not sys.argv[1:], 'unsupported arguments')
    components = ('geometry', 'filters_scalar', 'filters_native', 'interfaces',
                  'grid_metric', 'grid_metric_review')
    manifests = {name: manifest(ROOT / name) for name in components}
    if not inner_only:
        manifest(ROOT)
    result = dict(status='PASS', scope='closed audit reader; not an engine qualification',
                  inner_manifest_entries=manifests, geometry=geometry(),
                  scalar_filters=scalar_filters(), native_filters=native_filters(),
                  interfaces=interfaces(), grid_metric=metric(),
                  metric_review=metric_review(), GCP_used=False)
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
