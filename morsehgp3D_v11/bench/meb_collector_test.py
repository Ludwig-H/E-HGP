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

import meb_g4 as driver
import meb_semantic as sem

need = sem.need


def fixture(bits=18, signed=False):
    from math import comb
    events = [dict(phase='cloud', sites=12, points=12, read_ns=10, cloud_ns=20, peak_reserved_bytes=1000,
                   reserved_after_bytes=344),
              dict(phase='index', status='ok', reason='none', coord_bits=bits, leaf_size=8, wall_ns=30,
                   nodes=3, max_depth=2, node_bytes=40, peak_reserved_bytes=464, reserved_after_bytes=464)]
    words = [bits, 12, 48]

    def exact(value, budget):
        limbs = 2 if budget <= 127 else (budget + 63) // 64
        words.extend([int(value < 0), limbs] + [(abs(value) >> (64 * i)) & (2**64 - 1) for i in range(limbs)])

    for ordinal in range(48):
        size, threshold = ordinal % 12 + 1, (5, 10, 13)[ordinal % 3]
        selection = sem.part(ordinal, 12)
        q = min(size, 4)
        support = selection[:q]
        words += [size, threshold] + selection + [sem.NONE] * (12 - size) + [q] + support + [sem.NONE] * (4 - q)
        words += [2 if signed and q > 1 else 1, 1, 1]
        for coordinate in ((0, 0, 0) if q == 1 else (-1 if signed else 1, 0, 0)):
            exact(coordinate, 5 * bits + 5)
        exact(1 if q == 1 else 2, 4 * bits + 5)
        exact(0 if q == 1 else 1, 8 * bits + 12)
        exact(1 if q == 1 else 4, 6 * bits + 8)
        words += [0, 0, size] + selection
        presentations = 1 + sum(comb(size, r) for r in range(1, q))
        events.append(dict(phase='query', ordinal=ordinal, size=size, threshold=threshold, status='ok', reason='none',
                           meb_search='first_strict_containing_v1',
                           support_size=q, meb_ns=2, census_ns=3, wrapper_ns=6, reference_ns=5, reference_ok=True,
                           kind='complete', interior=0, shell=size, meb_peak_bytes=464, meb_after_bytes=464,
                           census_peak_bytes=464 + 4 * size, census_after_bytes=464 + 4 * size,
                           wrapper_peak_bytes=464 + 8 * size, wrapper_after_bytes=464 + 8 * size,
                           meb_logical=dict(presentations=presentations, nondegenerate=presentations,
                                            positive=presentations, containing=1, comparisons=0,
                                            point_tests=presentations * size),
                           census_logical=dict(nodes=2, bounds=0, point_tests=24, inside_blocks=0,
                                               outside_blocks=0, passes=2)))
    events += [dict(phase='summary', queries=48, complete=48, saturated=0, meb_ns=96, census_ns=144,
                    wrapper_ns=288, reference_ns=240, reserved_after_bytes=464, support_sizes=[4, 4, 4, 36]),
               dict(phase='exit', status='ok', reason='none')]
    return sem.MAGIC + struct.pack('<%dQ' % len(words), *words), events


def decode_controls(root):
    hashes, raw_hashes = [], []
    path = root / 'proof.bin'
    for bits in driver.PROFILES:
        payload, events = fixture(bits)
        path.write_bytes(payload)
        value = sem.inspect(path, bits, 12, events)
        hashes.append(value['sha256']); raw_hashes.append(value['raw_sha256'])
        need(value['raw_sha256'] == hashlib.sha256(payload).hexdigest(), 'known raw hash')
        negative_payload, negative_events = fixture(bits, signed=True)
        path.write_bytes(negative_payload)
        negative = sem.inspect(path, bits, 12, negative_events)
        need(negative['sha256'] == value['sha256'] and negative['raw_sha256'] != value['raw_sha256'],
             'signed N=-1 with anchor=2 equals N=1 with anchor=1, D=2')
    need(len(set(hashes)) == 1 and len(set(raw_hashes)) == 3, 'exact profile identity')
    payload, events = fixture()
    path.write_bytes(payload)
    expected = sem.inspect(path, 18, 12, events)
    # Same second-query center/radius, a different unreduced N/D presentation.
    equivalent = bytearray(payload)
    struct.pack_into('<Q', equivalent, 634, 2)
    struct.pack_into('<Q', equivalent, 730, 4)
    path.write_bytes(equivalent)
    changed = sem.inspect(path, 18, 12, events)
    need(changed['sha256'] == expected['sha256'] and changed['raw_sha256'] != expected['raw_sha256'],
         'rational presentation normalization')
    modes = ('truncated', 'extra', 'profile', 'part', 'duplicate_site', 'events_missing', 'events_duplicate',
             'reference', 'boolean', 'passes', 'summary', 'memory', 'query_memory', 'wrapper_memory',
             'threshold', 'kind', 'presentations', 'comparisons', 'support_size', 'point_tests',
             'integer_padding', 'negative_zero', 'denominator', 'radius', 'support_wrong', 'anchor_domain',
             'center_offset', 'search_missing', 'search_exhaustive', 'containing_many', 'rank', 'rank_positive')
    for mode in modes:
        data, values = payload, copy.deepcopy(events)
        if mode == 'truncated': data = data[:-1]
        elif mode == 'extra': data += b'x'
        elif mode == 'profile': data = data[:10] + struct.pack('<Q', 24) + data[18:]
        elif mode == 'part': data = data[:50] + struct.pack('<Q', 11) + data[58:]
        elif mode == 'duplicate_site': data = data[:-8] + data[-16:-8]
        elif mode == 'events_missing': values.pop(2)
        elif mode == 'events_duplicate': values[3] = copy.deepcopy(values[2])
        elif mode == 'reference': values[2]['reference_ok'] = False
        elif mode == 'boolean': values[2]['meb_logical']['containing'] = True
        elif mode == 'passes': values[2]['census_logical']['passes'] = 1
        elif mode == 'summary': values[-2]['meb_ns'] += 1
        elif mode == 'memory': values[-2]['reserved_after_bytes'] += 1
        elif mode == 'query_memory': values[2]['census_after_bytes'] += 1
        elif mode == 'wrapper_memory': values[2]['wrapper_peak_bytes'] += 1
        elif mode == 'threshold': values[2]['threshold'] = 0
        elif mode == 'kind': values[2]['kind'] = 'saturated'
        elif mode == 'presentations': values[2]['meb_logical']['presentations'] += 1
        elif mode == 'comparisons': values[2]['meb_logical']['comparisons'] += 1
        elif mode == 'support_size': values[2]['support_size'] = 2
        elif mode == 'point_tests': values[2]['meb_logical']['point_tests'] = 0
        elif mode == 'search_missing': values[2].pop('meb_search')
        elif mode == 'search_exhaustive': values[2]['meb_search'] = 'exhaustive_v1'
        elif mode == 'containing_many': values[6]['meb_logical']['containing'] = 2
        elif mode == 'rank': values[6]['meb_logical']['presentations'] += 1
        elif mode == 'rank_positive': values[6]['meb_logical']['positive'] = 15
        else:
            offset, replacement = dict(integer_padding=(218, 3), negative_zero=(210, 1), denominator=(322, 0),
                                       radius=(354, 1), support_wrong=(154, 11), anchor_domain=(186, 2**18),
                                       center_offset=(226, 1))[mode]
            data = data[:offset] + struct.pack('<Q', replacement) + data[offset + 8:]
        path.write_bytes(data)
        try:
            sem.inspect(path, 18, 12, values)
        except ValueError:
            pass
        else:
            raise ValueError('corruption accepted: ' + mode)
    return len(modes)


def attempts(root):
    args = argparse.Namespace(work=root, data=root)
    case = dict(name='tiny', count=12, coordinates='xyz', point_ids='ids')
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
            need(checkpoints[1]['status'] == 'pending_semantic' and row['semantic']['queries'] == 48, 'decode checkpoint')
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
        exe = args.builds / name / 'build/mhgp11_meb_bench'
        exe.parent.mkdir(parents=True)
        exe.write_bytes(('fake index B%d' % bits).encode())
        cache = ('MHGP11_COORD_BITS:STRING=%d\nMHGP11_SANITIZE:BOOL=%s\nMHGP11_TSAN:BOOL=OFF\n'
                 'MHGP11_POISON:BOOL=OFF\nMHGP11_MODULES:STRING=num;index;tower\n' % (bits, 'ON' if extra else 'OFF'))
        files = [dict(path='CMakeCache.txt', size=len(cache), sha256=hashlib.sha256(cache.encode()).hexdigest(), text=cache),
                 dict(path=exe.name, size=exe.stat().st_size, sha256=driver.base.digest(exe))]
        if extra:
            files += [dict(path=n, size=4, sha256='a' * 64) for n in ('mhgp11_tower_probe', 'libmhgp11.a')]
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
    manifest = {'cases': [dict(name=n, count=12, coordinates='xyz', point_ids='ids') for n in driver.previous.COUNTS]}
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

        def decode(path, bits, count, events):
            if mode == 'interrupted':
                raise KeyboardInterrupt()
            value = original(path, bits, count, events)
            if bits == 21 and mode in ('different', 'incomplete_different'):
                value['sha256'] = 'b' * 64
            if bits == 21 and mode == 'work_different':
                events[2]['census_logical']['point_tests'] += 1
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
        report = driver.load(args.out / 'meb.json')
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
    need((corruption, attempts_count, provenance, schedule_count) == (32, 11, 20, 6), 'coverage floor')
    print('meb_collector_verdict conforme attempts11 corruptions32 provenance20 schedules6 native0')


if __name__ == '__main__':
    main()
