#!/usr/bin/env python3
"""Compare the input adapter's digest with an independent scalar q2 oracle."""
from __future__ import annotations

import hashlib
from itertools import combinations, product
import json
import os
from pathlib import Path
import struct
import subprocess

BASE = Path(__file__).resolve().parent
MASK = (1 << 64) - 1


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def oracle(points):
    total = xor_value = supports = interiors = shells = 0
    for a, b in combinations(range(len(points)), 2):
        center = [points[a][j] + points[b][j] for j in range(3)]
        diameter = sum((points[a][j] - points[b][j]) ** 2 for j in range(3))
        powers = [diameter - sum((2 * p[j] - center[j]) ** 2 for j in range(3)) for p in points]
        inside = [i for i, power in enumerate(powers) if power > 0]
        shell = [i for i, power in enumerate(powers) if power == 0]
        if len(inside) >= 10:
            continue
        h = 14695981039346656037
        for word in [2, a, b, *center, diameter, len(inside), *inside, len(shell), *shell]:
            for byte in struct.pack('<Q', word):
                h = ((h ^ byte) * 1099511628211) & MASK
        total = (total + h) & MASK
        xor_value ^= h
        supports += 1
        interiors += len(inside)
        shells += len(shell)
    require(supports > 0 and interiors > 0 and shells >= 2 * supports, 'oracle vacuous')
    return dict(encoding='canonical_q2_support_v2', supports=supports,
                interior_ids=interiors, shell_ids=shells, sum=format(total, 'x'), xor=format(xor_value, 'x'))


def main():
    receipt = BASE / 'PROBE_CHECKS.json'
    require(not receipt.exists(), 'refuse to overwrite a receipt')
    source = BASE.parent / 'lidar08_20260914/prepared/single_000000/n8000.u16le'
    binary = BASE / '.build/r1/lidar_q2_probe'
    fixture = BASE / '.build/probe_input32.u16le'
    payload = source.read_bytes()[:32 * 6]
    require(len(payload) == 32 * 6, 'incomplete fixture')
    fixture.write_bytes(payload)
    points = list(struct.iter_unpack('<HHH', payload))
    expected = oracle(points)
    report = dict(schema='mhgp8_lidar_q2_probe_checks_v1', status='running',
                  scope='bounded_digest_comparison_not_collision_free_geometry_certificate',
                  source_sha256=sha(Path(__file__)), binary_sha256=sha(binary),
                  input_source=str(source), input_source_sha256=sha(source),
                  fixture_rule='first 32 records of pinned source, same little-endian order',
                  fixture_sha256=sha(fixture), oracle_pairs=496, oracle_point_tests=15872,
                  expected_digest=expected, runs=[])
    def save():
        receipt.write_text(json.dumps(report, indent=2, sort_keys=True, allow_nan=False) + '\n')
    save()
    try:
        for front, census in product(('pure', 'samples'), ('pairwise', 'shared')):
            command = [str(binary), str(fixture), '10', '8', front, census]
            result = subprocess.run(command, capture_output=True, text=True, timeout=30,
                                    preexec_fn=lambda: os.sched_setaffinity(0, {min(os.sched_getaffinity(0))}))
            record = dict(command=command, returncode=result.returncode, stdout=result.stdout, stderr=result.stderr)
            report['runs'].append(record)
            save()
            require(result.returncode == 0 and result.stderr == '', 'probe failed')
            require(json.loads(result.stdout)['digest'] == expected, 'probe/oracle digest mismatch')
        require(sha(binary) == report['binary_sha256'] and sha(Path(__file__)) == report['source_sha256']
                and sha(source) == report['input_source_sha256'], 'input changed during checks')
        report['status'] = 'passed'
    except BaseException as error:
        report.update(status='failed', error=type(error).__name__ + ': ' + str(error))
        raise
    finally:
        save()
    print(json.dumps({'status': report['status'], 'runs': len(report['runs']), 'oracle_pairs': 496,
                      'oracle_point_tests': 15872, 'supports': expected['supports']}))


if __name__ == '__main__':
    main()
