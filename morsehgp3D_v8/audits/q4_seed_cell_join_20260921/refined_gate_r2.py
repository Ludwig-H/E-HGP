#!/usr/bin/env python3
"""Exercise a refined atlas with exact oracles, without editing the frozen probe.

The only C++ delta is the explicitly archived local atlas options.
All product headers come from the original product32 snapshot and both
compilers verify their actual -MM dependency sets before and after testing. The runner
uses the existing compiler flags, command receipts and
Cartesian Fraction oracle. Inputs, generated source and failures are retained
inside this capture; no existing qualification or LiDAR receipt is modified.
"""
import json
from pathlib import Path
import struct
import traceback

import campaign as C

BASE, ROOT, ORACLE = C.BASE, C.ROOT, C.ORACLE
FOLDER = BASE/'receipts/refined_r2'
BUILD = BASE/'.build/refined_r2'
SOURCE = FOLDER/'source.cpp'
RUNNER = Path(__file__).resolve()
INCLUDE = BASE/'snapshot/include'
ORIGINAL_MANIFEST = BASE/'receipts/qualification_r1/MANIFEST.json'
ORIGINAL_COMPLETION = BASE/'receipts/qualification_r1/COMPLETION.json'
FROZEN_PROBE_SHA256 = '6bad3a6ecd680eb1048bf9d4c50f065d144b4fb09344804c4a8eca117f1a15df'
OLD = 'const Q4LocalOptions options{};'
NEW = 'const Q4LocalOptions options{Q4CenterDomainMode::Positive,4,341,16,2,true};'
REQUIRED = ('cell_splits', 'family_cache_hits', 'zero_bound_products', 'emitted')
NAVIGATION = {'seed_queries', 'seed_owner_tests', 'seed_owner_rejections',
              'query_visits', 'line_tests', 'line_skips', 'peak_buffer_bytes'}


def compile_flags():
    # The complete historical include tree MUST precede the evolving product.
    return ['-I', str(INCLUDE), *C.flags(), '-I', str(BASE)]


def dependency_scan(compiler):
    return C.execute([compiler, *compile_flags(), '-MM', str(SOURCE)])


def dependency_paths(record):
    C.require(record['returncode'] == 0 and record['stderr'] == '', 'Dependency scan failed')
    text = record['stdout'].replace('\\\n', ' ')
    paths = {Path(token).resolve() for token in text.split(':', 1)[1].split()}
    C.require(paths and SOURCE in paths, 'Generated source absent from dependency scan')
    C.require(not any(path.is_relative_to(ROOT/'morsehgp3D_v8/src') for path in paths),
              'Compilation still reaches the evolving product include tree')
    original = C.load(ORIGINAL_MANIFEST)
    expected = {INCLUDE/Path(name).relative_to('morsehgp3D_v8/src'): expected
                for name, expected in original['sources'].items()
                if name.startswith('morsehgp3D_v8/src/')}
    actual = {path for path in paths if path.is_relative_to(INCLUDE)}
    C.require(actual == set(expected), 'Compiled snapshot header set differs from product32')
    for path, expected_sha in expected.items():
        C.require(C.sha(path) == expected_sha, 'Historical compiled header changed: '+str(path))
    return paths


def pinned_sources(compiled):
    original = C.load(ORIGINAL_MANIFEST)
    C.require(C.load(ORIGINAL_COMPLETION)['manifest_sha256'] == C.sha(ORIGINAL_MANIFEST),
              'Original product32 manifest changed')
    paths = set(compiled)
    paths.update(C.libraries())
    paths.update(BASE/name for name in ('campaign.py', 'math_checks.py', 'probe.cpp', 'read.py'))
    paths.update(BASE.parent/name for name in ('q34_global_contract_20260921/oracle.py',
                 'q4_center_blocks_20260920/oracle_gate.py', 'q4_center_blocks_20260920/fixtures.py'))
    paths.update((RUNNER, ORIGINAL_MANIFEST, ORIGINAL_COMPLETION))
    result = {str(path.relative_to(ROOT)): C.sha(path) for path in sorted(paths)}
    for library in C.libraries():
        name = str(library.relative_to(ROOT))
        C.require(result[name] == original['sources'][name], 'Pinned product32 archive changed')
    snapshot = str((BASE/'snapshot/q4_local.cpp').relative_to(ROOT))
    C.require(result[snapshot] == original['sources'][snapshot], 'Pinned product32 C++ snapshot changed')
    C.require(C.sha(BASE/'probe.cpp') == FROZEN_PROBE_SHA256, 'Frozen probe changed')
    C.require(SOURCE.read_text() == (BASE/'probe.cpp').read_text().replace(OLD, NEW),
              'Generated source is not exactly the declared single replacement')
    return result


def fixtures():
    original = dict(ORACLE.fixtures())
    regular = original['regular_q4_cover4']
    yield 'refined_regular4', regular[:4]
    yield 'refined_regular6', regular
    yield 'refined_isolated8', original['isolated_shallow_vertex']
    yield 'refined_shell30', original['shell30']
    yield 'refined_shell30_inside', original['shell30']+[(20, 20, 20)]
    yield 'refined_coincident_seed_lines', [(15,20,20),(24,23,20),(20,15,20),(23,16,20)]
    for trial in range(3):
        yield f'refined_random_u16_{trial}', original[f'random_nonaxial_{trial}']


def input_file(name, points):
    path = BUILD/'inputs'/(name+'.u16le')
    C.require(not path.exists(), 'Refined input overwrite refused')
    path.write_bytes(b''.join(struct.pack('<HHH', *point) for point in points))
    return path


def check_report(data, record, expected):
    C.require(data['status'] == 'completed', 'Incomplete probe output')
    C.require((data['n'], data['kmax'], data['edge'], data['grain']) ==
              (record['n'], record['kmax'], record['edge'], record['grain']),
              'Probe/command identity mismatch')
    normalized = ORACLE.normalize(data['records'])
    C.require(normalized == expected, 'Exact refined oracle differs: '+record['case'])
    C.require(len(set(normalized)) == len(normalized), 'Duplicate exact support emission')
    for mode in ('baseline', 'alive', 'join'):
        report = data[mode]
        C.require(report['matches_baseline'] is True and report['output_count'] == len(expected),
                  'Reference/alive/join mismatch')
        for category in ('generator', 'sweep', 'extra'):
            C.require(all(type(value) is int and value >= 0 for value in report[category].values()),
                      'Invalid work counter')
    for field, value in data['baseline']['sweep'].items():
        if field not in NAVIGATION:
            C.require(value == data['alive']['sweep'][field] == data['join']['sweep'][field],
                      'Refined downstream work changed: '+field)
    extra = data['join']['extra']
    C.require(extra['block_max_sites'] <= record['grain'], 'Block/cache grain exceeded')
    C.require(extra['family_preparations'] <= extra['seed_cache_misses'],
              'More families than distinct lazy seed preparations')


def qualify():
    C.require(not FOLDER.exists() and not BUILD.exists(), 'Refined capture/build already exists')
    C.require(C.sha(BASE/'probe.cpp') == FROZEN_PROBE_SHA256, 'Unexpected original probe')
    original = (BASE/'probe.cpp').read_text()
    C.require(original.count(OLD) == 1, 'Options replacement must be unique')
    FOLDER.mkdir(parents=True)
    BUILD.mkdir(parents=True)
    (BUILD/'inputs').mkdir()
    SOURCE.write_text(original.replace(OLD, NEW))
    manifest = dict(schema='mhgp8_audit_q4_seed_cell_join_v1', status='started',
        public_status='not_claimed', scope='Refined small exact oracle; atlas options only are changed',
        sources={}, records=[], binaries={}, cpu_affinity=sorted(C.os.sched_getaffinity(0)),
        include_policy='All 16 product32 headers from git d1b4dbc6, before live src; g++ and clang++ -MM verified',
        timing_scope='Functional refined gate, concurrent workspace load; no timing claim',
        source_delta=dict(source=str(SOURCE.relative_to(ROOT)), replacement_count=1,
                          original=OLD, replacement=NEW, original_sha256=FROZEN_PROBE_SHA256),
        required_positive_coverage=list(REQUIRED))
    C.write(FOLDER/'MANIFEST.json', manifest)
    phase = 'dependencies'
    try:
        compiled = None
        for compiler in ('g++', 'clang++'):
            scan = dependency_scan(compiler)
            C.append(FOLDER, manifest, scan)
            paths = dependency_paths(scan)
            C.require(compiled is None or paths == compiled, 'Compiler dependency sets differ')
            compiled = paths
        before = pinned_sources(compiled)
        manifest['sources'] = before
        C.write(FOLDER/'MANIFEST.json', manifest)
        phase = 'compilers'
        common = compile_flags()
        commands = [
            ['g++', '--version'], ['clang++', '--version'],
            ['g++', *common, '-O2', str(SOURCE), str(C.libraries()[0]),
             '-pthread', '-o', str(BUILD/'release')],
            ['clang++', *common, '-O1', '-g', '-fsanitize=address,undefined',
             '-fno-omit-frame-pointer', str(SOURCE), str(C.libraries()[1]),
             '-pthread', '-o', str(BUILD/'sanitize')]]
        for command in commands:
            C.append(FOLDER, manifest, C.execute(command))
            print('PASS refined command', len(manifest['records']), flush=True)
        manifest['binaries'] = {str((BUILD/name).relative_to(ROOT)): C.sha(BUILD/name)
                                for name in ('release', 'sanitize')}
        C.write(FOLDER/'MANIFEST.json', manifest)
        phase = 'exact_oracles'
        counts = dict(calls=0, outputs=0, max_shell=0)
        coverage = {binary: {field: 0 for field in REQUIRED} for binary in ('release', 'sanitize')}
        witnesses = {binary: {} for binary in coverage}
        for name, points in fixtures():
            path = input_file(name, points)
            all_expected = ORACLE.expected(points, 10, 4, counts)
            edges = sorted({ORACLE.owner(points, tuple(r['support'])) for r in all_expected})[:2]
            if (0, 1) not in edges:
                edges.append((0, 1))
            for a, b in edges:
                for k in (3, 5, 10):
                    expected = ORACLE.normalize([r for r in all_expected if r['depth'] < k-2
                        and ORACLE.owner(points, tuple(r['support'])) == (a, b)])
                    for grain in (8, 64):
                        for binary in ('release', 'sanitize'):
                            command = [str(BUILD/binary), str(path), str(k), str(a), str(b), str(grain)]
                            record = dict(case=name, n=len(points), kmax=k, edge=[a,b], grain=grain,
                                          input_sha256=C.sha(path), **C.execute(command))
                            C.append(FOLDER, manifest, record)
                            data = json.loads(record['stdout'])
                            check_report(data, record, expected)
                            counts['calls'] += 1
                            counts['outputs'] += len(expected)
                            counts['max_shell'] = max([counts['max_shell']]+[len(r[-1]) for r in expected])
                            observed = {field: data['join']['extra'][field] for field in REQUIRED if field != 'emitted'}
                            observed['emitted'] = data['join']['sweep']['emitted']
                            for field, value in observed.items():
                                coverage[binary][field] += value
                                if value and field not in witnesses[binary]:
                                    witnesses[binary][field] = dict(record=manifest['records'][-1]['path'],
                                        case=name, kmax=k, edge=[a,b], grain=grain, value=value,
                                        oracle_exact=True)
            print('PASS refined oracle', name, flush=True)
        manifest.update(counts=counts, positive_coverage=coverage, coverage_witnesses=witnesses)
        phase = 'nonvacuity'
        missing = {binary: [field for field in REQUIRED if values[field] <= 0]
                   for binary, values in coverage.items()}
        missing = {binary: fields for binary, fields in missing.items() if fields}
        C.require(not missing, 'Missing refined fixture exercise: '+json.dumps(missing, sort_keys=True))
        C.require(counts['max_shell'] == 30, 'Full shell30 payload not exercised')
        phase = 'closure'
        for compiler in ('g++', 'clang++'):
            scan = dependency_scan(compiler)
            C.append(FOLDER, manifest, scan)
            C.require(dependency_paths(scan) == compiled, 'Compiled dependency set changed during gate')
        C.require(before == pinned_sources(compiled), 'Source/library/runner changed during refined gate')
        for name, expected in manifest['binaries'].items():
            C.require(C.sha(ROOT/name) == expected, 'Refined executable changed')
        manifest['status'] = 'completed'
        C.write(FOLDER/'MANIFEST.json', manifest)
        C.write(FOLDER/'COMPLETION.json', dict(status='completed', records=len(manifest['records']),
                                            manifest_sha256=C.sha(FOLDER/'MANIFEST.json')))
        print(json.dumps(dict(status='completed', counts=counts, positive_coverage=coverage), sort_keys=True),
              flush=True)
    except BaseException as error:
        manifest.update(status='failed', failed_phase=phase)
        C.write(FOLDER/'MANIFEST.json', manifest)
        C.write(FOLDER/'FAILURE.json', dict(status='failed', phase=phase, error=repr(error),
            traceback=traceback.format_exc(), records=len(manifest['records']),
            manifest_sha256=C.sha(FOLDER/'MANIFEST.json')))
        (FOLDER/'failed_refined_gate_r2.py').write_bytes(RUNNER.read_bytes())
        raise


if __name__ == '__main__':
    qualify()
