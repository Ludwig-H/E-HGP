#!/usr/bin/env python3
"""Pure parser/collector controls; all native subprocesses are replaced by fixtures."""
import argparse
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from unittest.mock import patch

import index_g4 as driver
import index_semantic as sem

need = sem.need


def fixture(bits=18):
    events = [dict(phase='cloud', sites=4, points=4, read_ns=10, cloud_ns=20, peak_reserved_bytes=256,
                   reserved_after_bytes=120),
              dict(phase='index', status='ok', reason='none', coord_bits=bits, leaf_size=8, wall_ns=30,
                   nodes=1, max_depth=1, node_bytes=40, peak_reserved_bytes=160, reserved_after_bytes=160)]
    words = [bits, 4, 64]
    for ordinal in range(64):
        q, threshold, support = ordinal % 4 + 1, (5, 10, 13)[ordinal % 3], sem.support(ordinal, 4)
        shell = sorted(support[:q])
        words += [q, threshold] + support + [0, 0, 0, q] + shell
        events.append(dict(phase='query', ordinal=ordinal, arity=q, threshold=threshold, status='ok', reason='none',
                           factory_ns=1, wall_ns=3, reference_ns=5, reference_ok=True, kind='complete', interior=0,
                           shell=q, peak_reserved_bytes=160 + 4 * q, reserved_after_bytes=160 + 4 * q,
                           logical=dict(nodes=2, bounds=0, point_tests=8, inside_blocks=0, outside_blocks=0, passes=2)))
    events += [dict(phase='summary', queries=64, complete=64, saturated=0, degenerate=0, query_ns=192,
                    reference_ns=320, reserved_after_bytes=160, arities=[16] * 4),
               dict(phase='exit', status='ok', reason='none')]
    return sem.MAGIC + struct.pack('<%dQ' % len(words), *words), events


def decode_controls(root):
    hashes = []
    raw_hashes = []
    path = root / 'proof.bin'
    for bits in driver.PROFILES:
        payload, events = fixture(bits)
        path.write_bytes(payload)
        value = sem.inspect(path, bits, 4, events)
        hashes.append(value['sha256'])
        raw_hashes.append(value['raw_sha256'])
        need(value['raw_sha256'] == hashlib.sha256(payload).hexdigest() and value['bytes'] == len(payload),
             'known raw hash')
    need(len(set(hashes)) == 1 and len(set(raw_hashes)) == 3, 'profile-independent exact output')
    payload, events = fixture()
    mutations = ('truncated', 'extra', 'profile', 'support', 'duplicate_site', 'events_missing', 'events_duplicate',
                 'reference', 'boolean', 'passes', 'summary', 'memory', 'query_memory', 'index_memory',
                 'threshold', 'kind', 'floor')
    for mode in mutations:
        data, values = payload, copy.deepcopy(events)
        if mode == 'truncated':
            data = data[:-1]
        elif mode == 'extra':
            data += b'x'
        elif mode == 'profile':
            data = data[:10] + struct.pack('<Q', 24) + data[18:]
        elif mode == 'support':
            data = data[:50] + struct.pack('<Q', 3) + data[58:]
        elif mode == 'duplicate_site':
            data = data[:-8] + data[-16:-8]
        elif mode == 'events_missing':
            values.pop(2)
        elif mode == 'events_duplicate':
            values[3] = copy.deepcopy(values[2])
        elif mode == 'reference':
            values[2]['reference_ok'] = False
        elif mode == 'boolean':
            values[2]['logical']['nodes'] = True
        elif mode == 'passes':
            values[2]['logical']['passes'] = 1
        elif mode == 'summary':
            values[-2]['query_ns'] += 1
        elif mode == 'memory':
            values[-2]['reserved_after_bytes'] += 1
        elif mode == 'query_memory':
            values[2]['reserved_after_bytes'] += 1
            values[2]['peak_reserved_bytes'] += 1
        elif mode == 'index_memory':
            values[1]['peak_reserved_bytes'] += 1
        elif mode == 'threshold':
            values[2]['threshold'] = 0
        elif mode == 'kind':
            values[2]['kind'] = 'saturated'
        else:
            for event in values[2:-2]:
                if event['arity'] == 4:
                    event['status'] = 'degenerate'
            values[-2].update(queries=48, complete=48, degenerate=16, query_ns=144, reference_ns=240,
                              arities=[16, 16, 16, 0])
        path.write_bytes(data)
        try:
            sem.inspect(path, 18, 4, values)
        except ValueError:
            pass
        else:
            raise ValueError('corruption accepted: ' + mode)
    return len(mutations)


def attempts(root):
    args = argparse.Namespace(work=root, data=root)
    case = dict(name='tiny', count=4, coordinates='xyz', point_ids='ids')
    # Quatre positions distinctes : arbre radix a une feuille de 8 (noeuds 1, profondeur 1), comme la fixture.
    (root / 'xyz').write_bytes(struct.pack('<12I', 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1))
    modes = ('ok', 'bad_json', 'bad_events', 'bad_binary', 'missing_binary', 'stderr', 'refused', 'failed', 'signal',
             'timeout', 'launch')
    for mode in modes:
        checkpoints = []

        def child(argv, **kwargs):
            need(kwargs['timeout'] == 30 and kwargs['check'] is False and argv[-1] == str(sem.BUDGET), 'process bounds')
            if mode == 'timeout':
                raise subprocess.TimeoutExpired(argv, 30, output=b'{', stderr=b'partial\xff')
            if mode == 'launch':
                raise OSError('unavailable')
            data, events = fixture()
            if mode == 'bad_events':
                events[2]['reference_ok'] = False
            code = dict(refused=2, failed=3, signal=-15).get(mode, 0)
            if mode not in ('missing_binary', 'refused', 'failed', 'signal'):
                Path(argv[3]).write_bytes(data[:-1] if mode == 'bad_binary' else data)
            output = '\n'.join(json.dumps(e) for e in events).encode()
            if mode == 'bad_json':
                output += b'\n{"x":1,"x":2}'
            return subprocess.CompletedProcess(argv, code, output, b'diagnostic' if mode == 'stderr' else b'')

        with patch.object(driver.subprocess, 'run', side_effect=child):
            row = driver.measure(Path('fake'), case, 18, args, lambda row: checkpoints.append(copy.deepcopy(row)))
        expected = {'ok': 'ok', 'bad_json': 'invalid_output', 'bad_events': 'invalid_output',
                    'bad_binary': 'artifact_error', 'missing_binary': 'artifact_error', 'refused': 'refused',
                    'failed': 'failed', 'signal': 'failed', 'timeout': 'timeout', 'launch': 'launch_error',
                    'stderr': 'invalid_output'}[mode]
        need(row['status'] == expected and checkpoints[0]['status'] == 'running' and checkpoints[-1] == row,
             mode + ' retained verdict/checkpoint')
        need(not (root / 'tiny_b18.bin').exists(), 'output cleanup')
        if mode == 'ok':
            need(checkpoints[1]['status'] == 'pending_semantic' and row['semantic']['queries'] == 64, 'decode checkpoint')
    return len(modes)


def builds(root):
    args = argparse.Namespace(builds=root / 'builds', qualification=root / 'matrix/summary.json',
                              supplement=root / 'supplement/summary.json')
    for path, names in ((args.qualification, driver.QUALIFIED), (args.supplement, {'gcc_asan_ubsan18'})):
        path.parent.mkdir(parents=True)
        configs = [dict(name=n, status='ok') for n in sorted(names)]
        driver.base.save(path, dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True, conforming=True,
                                    exit_code=0, signals=[], requested=sorted(names), configurations=configs,
                                    statuses={n: 'ok' for n in names}))
    paths = []
    for bits, name in list(driver.PROFILES.items()) + [(18, 'gcc_asan_ubsan18')]:
        extra = name == 'gcc_asan_ubsan18'
        path = (args.supplement if extra else args.qualification).parent / name / 'build_provenance.json'
        path.parent.mkdir()
        exe = args.builds / name / 'build/mhgp11_index_bench'
        exe.parent.mkdir(parents=True)
        exe.write_bytes(('fake index B%d' % bits).encode())
        cache = ('MHGP11_COORD_BITS:STRING=%d\nMHGP11_SANITIZE:BOOL=%s\nMHGP11_TSAN:BOOL=OFF\n'
                 'MHGP11_POISON:BOOL=OFF\nMHGP11_MODULES:STRING=num;index\n' % (bits, 'ON' if extra else 'OFF'))
        files = [dict(path='CMakeCache.txt', size=len(cache), sha256=hashlib.sha256(cache.encode()).hexdigest(), text=cache),
                 dict(path=exe.name, size=exe.stat().st_size, sha256=driver.base.digest(exe))]
        if extra:
            files += [dict(path=n, size=4, sha256='a' * 64) for n in ('mhgp11_index_probe', 'libmhgp11.a')]
        driver.base.save(path, dict(schema='ehgp.v11.build_provenance.v1', complete=True, errors=[], files=files))
        paths.append(path)
    need(set(driver.checked_builds(args)[0]) == set(driver.PROFILES), 'qualified positive')
    count = 0
    for path in (args.qualification, args.supplement):
        original = load = driver.load(path)
        for mode in ('false_success', 'bool_exit', 'duplicate', 'signals'):
            value = copy.deepcopy(load)
            if mode == 'false_success':
                value['conforming'] = False
            elif mode == 'bool_exit':
                value['exit_code'] = False
            elif mode == 'duplicate':
                value['configurations'].append(value['configurations'][0])
            else:
                value['signals'] = [15]
            driver.base.save(path, value)
            try:
                driver.checked_builds(args)
            except ValueError:
                count += 1
            else:
                raise ValueError('bad qualification accepted')
        driver.base.save(path, original)
    for path in paths:
        original = driver.load(path)
        for mode in ('cache', 'duplicate', 'incomplete'):
            value = copy.deepcopy(original)
            if mode == 'cache':
                value['files'][0]['text'] += 'corruption'
            elif mode == 'duplicate':
                value['files'].append(value['files'][0])
            else:
                value['complete'] = False
            driver.base.save(path, value)
            try:
                driver.checked_builds(args)
            except ValueError:
                count += 1
            else:
                raise ValueError('bad provenance accepted')
        driver.base.save(path, original)
    return count


def schedules(root):
    manifest = {'cases': [dict(name=n, count=4, coordinates='xyz', point_ids='ids') for n in driver.previous.COUNTS]}
    (root / 'xyz').write_bytes(struct.pack('<12I', 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1))  # forme radix (1, 1)
    builds = {bits: {'path': 'fake%d' % bits} for bits in driver.PROFILES}
    for mode in ('ok', 'failed', 'different', 'incomplete_different', 'work_different', 'interrupted'):
        args = argparse.Namespace(out=root / mode, work=root / (mode + '_work'), data=root,
                                  qualification=root / 'unused', supplement=root / 'unused2')

        def child(argv, **_kwargs):
            bits = int(argv[0][4:])
            data, events = fixture(bits)
            if mode in ('failed', 'incomplete_different') and bits == 24:
                raise subprocess.TimeoutExpired(argv, 30, output=b'', stderr=b'')
            Path(argv[3]).write_bytes(data)
            return subprocess.CompletedProcess(argv, 0, '\n'.join(json.dumps(e) for e in events).encode(), b'')

        original = sem.inspect

        def decode(path, bits, count, events, shape=None):
            if mode == 'interrupted':
                raise KeyboardInterrupt()
            value = original(path, bits, count, events, shape)
            if bits == 21 and mode in ('different', 'incomplete_different'):
                value['sha256'] = 'b' * 64
            if bits == 21 and mode == 'work_different':
                events[2]['logical']['point_tests'] += 1
            return value

        with patch.object(driver, 'checked_builds', return_value=(builds, 'c' * 64)), \
                patch.object(driver.previous, 'inputs', return_value=(manifest, 'd' * 64)), \
                patch.object(driver.base, 'digest', return_value='e' * 64), \
                patch.object(driver.subprocess, 'run', side_effect=child), \
                patch.object(sem, 'inspect', side_effect=decode), contextlib.redirect_stdout(io.StringIO()):
            try:
                code = driver.run(args)
            except KeyboardInterrupt:
                need(mode == 'interrupted', 'unexpected interruption')
                code = None
        report = driver.load(args.out / 'index.json')
        if mode == 'interrupted':
            need(code is None and report['complete'] is False and len(report['runs']) == 1 and
                 report['runs'][0]['status'] == 'pending_semantic' and len(report['not_run']) == 17,
                 'process result lost during decoding')
        else:
            need(code == (0 if mode == 'ok' else 1) and len(report['runs']) == 18 and not report['not_run'],
                 'schedule verdict/inventory')
            if mode in ('different', 'incomplete_different', 'work_different'):
                need(all(c['status'] == 'different' for c in report['comparisons']), 'difference hidden')
    return 6


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11-index-reader-') as temp:
        root = Path(temp)
        corruption = decode_controls(root)
        attempts_count = attempts(root)
        provenance = builds(root)
        schedule_count = schedules(root)
    need((corruption, attempts_count, provenance, schedule_count) == (17, 11, 20, 6), 'coverage floor')
    print('index_collector_verdict conforme attempts11 corruptions17 provenance20 schedules6 native0')


if __name__ == '__main__':
    main()
