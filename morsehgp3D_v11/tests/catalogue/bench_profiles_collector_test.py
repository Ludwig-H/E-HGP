#!/usr/bin/env python3
"""Profile collector controls with mocked subprocesses; no native execution or cloud access."""
import argparse
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from bench_semantic_test import fixture, need
import catalogue_profiles as driver


def events(bits=18):
    return [{'phase': 'cloud', 'points': 4, 'sites': 4, 'cloud_ns': 20, 'read_ns': 10},
            {'phase': 'catalogue', 'status': 'ok', 'balls': 11, 'levels': 4, 'incidences': 28,
             'wall_ns': 30, 'coord_bits': bits, 'kmax': 5, 'generation_passes': 2,
             'peak_reserved_bytes': 100, 'reserved_after_bytes': 50,
             'logical': dict.fromkeys(sorted(driver.LOGICAL), 7)},
            {'phase': 'exit', 'status': 'ok'}]


def row(name, bits, kmax=5, status='ok'):
    return {'case': name, 'coord_bits': bits, 'kmax': kmax, 'repetition': 0, 'status': status,
            'semantic': {'sha256': 'a' * 64}, 'events': events(bits)}


def attempts(root):
    args = argparse.Namespace(work=root, data=root)
    case = {'name': 'test', 'coordinates': 'xyz', 'point_ids': 'ids', 'count': 4}
    calls = 0
    modes = ('ok', 'wrong_bits', 'wrong_K', 'wrong_work', 'bad_json', 'bad_canonical',
             'missing_output', 'refused', 'failed', 'signal', 'timeout', 'launch')
    for mode in modes:
        def child(argv, **kwargs):
            nonlocal calls
            calls += 1
            need(kwargs['timeout'] == 30 and kwargs['check'] is False, 'native process bounds')
            if mode == 'timeout':
                raise subprocess.TimeoutExpired(argv, 30, output=b'{"phase":', stderr=b'partial')
            if mode == 'launch':
                raise OSError('launch unavailable')
            values = events(21 if mode == 'wrong_bits' else 18)
            if mode == 'wrong_K':
                values[1]['kmax'] = 10
            if mode == 'wrong_work':
                values[1]['logical']['nodes'] = -1
            payload = '\n'.join(json.dumps(value) for value in values).encode()
            if mode == 'bad_json':
                payload += b'\n{"duplicate":1,"duplicate":2}'
            code = {'refused': 2, 'failed': 3, 'signal': -15}.get(mode, 0)
            if mode not in ('missing_output', 'refused', 'failed', 'signal'):
                data, _ = fixture(18)
                Path(argv[3]).write_bytes(data[:-1] if mode == 'bad_canonical' else data)
            return subprocess.CompletedProcess(argv, code, payload, b'')

        with patch.object(driver.subprocess, 'run', side_effect=child):
            result = driver.measure(root / 'fake', case, 18, 5, args)
        wanted = {'ok': 'ok', 'bad_json': 'invalid_output', 'refused': 'refused', 'failed': 'failed',
                  'signal': 'failed', 'timeout': 'timeout', 'launch': 'launch_error'}.get(mode, 'artifact_error')
        need(result['status'] == wanted, mode + ': process/collection verdict')
        need(result['coord_bits'] == 18 and result['repetition'] == 0, 'attempt identity')
        need(not (root / 'test_b18_k5.bin').exists(), 'canonical cleanup')
        if wanted == 'ok':
            need(result['semantic']['balls'] == 11 and 'canonical_sha256' in result, 'both digests required')
        if wanted in ('artifact_error', 'invalid_output', 'timeout', 'launch_error'):
            need(result['errors'], 'structured error preserved')
    need(calls == len(modes), 'attempt inventory')
    return calls


def builds(root):
    args = argparse.Namespace(builds=root / 'builds', qualification=root / 'matrix' / 'summary.json')
    args.qualification.parent.mkdir()
    summary = {'conforming': True, 'exit_code': 0, 'complete': True,
               'configurations': [{'name': name, 'status': 'ok'} for name in driver.PROFILES.values()]}
    args.qualification.write_text(json.dumps(summary))
    for bits, name in driver.PROFILES.items():
        executable = args.builds / name / 'build' / 'mhgp11_catalogue_bench'
        executable.parent.mkdir(parents=True)
        executable.write_bytes(('fake binary B%d' % bits).encode())
        cache = ('MHGP11_COORD_BITS:STRING=%d\n' % bits).encode()
        files = [{'path': executable.name, 'size': executable.stat().st_size,
                  'sha256': driver.base.digest(executable)},
                 {'path': 'CMakeCache.txt', 'size': len(cache), 'sha256': hashlib.sha256(cache).hexdigest(),
                  'text': cache.decode()}]
        provenance = args.qualification.parent / name / 'build_provenance.json'
        provenance.parent.mkdir()
        provenance.write_text(json.dumps({'schema': 'ehgp.v11.build_provenance.v1', 'complete': True,
                                         'errors': [], 'files': files}))
    need(set(driver.checked_builds(args)) == set(driver.PROFILES), 'positive qualified binaries')
    corruptions = 0
    for name in driver.PROFILES.values():
        path = args.qualification.parent / name / 'build_provenance.json'
        original = path.read_text()
        for mode in ('hash', 'cache', 'duplicate', 'incomplete'):
            value = json.loads(original)
            if mode == 'hash':
                value['files'][0]['sha256'] = '0' * 64
            elif mode == 'cache':
                cache = b'MHGP11_COORD_BITS:STRING=17\n'
                value['files'][1].update(text=cache.decode(), size=len(cache), sha256=hashlib.sha256(cache).hexdigest())
            elif mode == 'duplicate':
                value['files'].append(value['files'][0])
            else:
                value['complete'] = False
            path.write_text(json.dumps(value))
            try:
                driver.checked_builds(args)
            except ValueError:
                corruptions += 1
            else:
                raise ValueError(mode + ': invalid provenance accepted')
        path.write_text(original)
    need(corruptions == 12, 'provenance corruption floor')
    return corruptions


def schedules(root):
    manifest = {'cases': [{'name': name, 'count': count} for name, count in driver.COUNTS.items()]}
    builds = {bits: {'path': 'fake%d' % bits} for bits in driver.PROFILES}
    for mode in ('ok', 'one_failed', 'different', 'work_different', 'incomplete_different'):
        args = argparse.Namespace(out=root / mode, work=root / (mode + '_work'), data=root,
                                  qualification=root / 'unused')

        def measure(_exe, case, bits, kmax, _args, checkpoint):
            failed = mode in ('one_failed', 'incomplete_different') and bits == 18 and kmax == 5
            result = row(case['name'], bits, kmax, 'timeout' if failed else 'ok')
            if mode in ('different', 'incomplete_different') and bits == 24:
                result['semantic']['sha256'] = 'b' * 64
            if mode == 'work_different' and bits == 24:
                result['events'][1]['logical']['nodes'] += 1
            checkpoint(result)
            return result

        with patch.object(driver, 'checked_builds', return_value=builds), \
                patch.object(driver, 'inputs', return_value=(manifest, 'a' * 64)), \
                patch.object(driver.base, 'digest', return_value='b' * 64), \
                patch.object(driver, 'measure', side_effect=measure), contextlib.redirect_stdout(io.StringIO()):
            code = driver.run(args)
        report = json.loads((args.out / 'profiles.json').read_text())
        need(code == (0 if mode == 'ok' else 1), 'schedule conformity')
        need(len(report['runs']) + len(report['not_run']) == 36, 'exact requested inventory')
        if mode in ('one_failed', 'incomplete_different'):
            need(len(report['runs']) == 30 and len(report['not_run']) == 6, 'only same-profile causal omission')
            need(all(r['coord_bits'] == 18 and r['kmax'] == 10 for r in report['not_run']), 'cross-profile suppression')
        if mode in ('different', 'work_different', 'incomplete_different'):
            need(all(c['status'] == 'different' for c in report['comparisons']), 'disagreement hidden by incompleteness')
    return 5


def interrupted_decoder(root):
    args = argparse.Namespace(out=root / 'interrupted', work=root / 'interrupted_work', data=root,
                              qualification=root / 'unused')
    manifest = {'cases': [{'name': name, 'count': 4} for name in driver.COUNTS]}
    builds = {bits: {'path': 'fake%d' % bits} for bits in driver.PROFILES}

    def child(argv, **_kwargs):
        payload, _ = fixture(18)
        Path(argv[3]).write_bytes(payload)
        return subprocess.CompletedProcess(argv, 0, '\n'.join(json.dumps(e) for e in events()).encode(), b'')

    # The real collector has captured a native success, then is interrupted during semantic decoding.
    for case in manifest['cases']:
        case.update(coordinates='xyz', point_ids='ids')
    with patch.object(driver, 'checked_builds', return_value=builds), \
            patch.object(driver, 'inputs', return_value=(manifest, 'a' * 64)), \
            patch.object(driver.base, 'digest', return_value='b' * 64), \
            patch.object(driver.subprocess, 'run', side_effect=child), \
            patch.object(driver.semantic, 'inspect', side_effect=KeyboardInterrupt):
        try:
            driver.run(args)
        except KeyboardInterrupt:
            pass
        else:
            raise ValueError('decoder interruption swallowed')
    report = json.loads((args.out / 'profiles.json').read_text())
    need(report['complete'] is False and report['full_schedule_completed'] is False, 'interruption falsely complete')
    need(len(report['runs']) == 1, 'native attempt absent/duplicated')
    attempt = report['runs'][0]
    need(attempt['status'] == 'pending_semantic' and attempt['exit_code'] == 0 and attempt['events'] == events(),
         'native result missing from checkpoint')
    need('semantic' not in attempt and 'canonical_sha256' not in attempt, 'unfinished artifact promoted')
    return 1


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11_profiles_collector_') as folder:
        root = Path(folder)
        count = attempts(root)
        corruption_count = builds(root)
        schedule_count = schedules(root)
        interruption_count = interrupted_decoder(root)
    need((count, corruption_count, schedule_count, interruption_count) == (12, 12, 5, 1), 'collector non-vacuity')
    print('catalogue_profiles_collector_verdict conforme attempts12 corruptions12 schedules5 interrupted1 native0')


if __name__ == '__main__':
    main()
