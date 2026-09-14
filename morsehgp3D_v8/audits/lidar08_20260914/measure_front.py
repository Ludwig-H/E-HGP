#!/usr/bin/env python3
"""Bounded local campaigns over pinned real LiDAR inputs; front only."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import subprocess
import sys

BASE = Path(__file__).resolve().parent
TIMES = ('load_ms', 'cloud_ms', 'index_ms', 'front_callback_ms',
         'validation_ms', 'destruction_ms', 'total_ms')
PLANS = {
    'pilot': (('single_000000', 'overlap_5'), (8000,), (8,), 1),
    'growth': (('single_000000', 'overlap_5'), (16000, 32000), (8,), 1),
    'check50k': (('single_000000', 'single_000100', 'single_000200', 'overlap_5'), (50000,), (8,), 2),
    'separation': (('single_000000', 'overlap_5'), (8000,), (10, 12), 1),
}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def plan(name):
    datasets, sizes, separations, repeats = PLANS[name]
    return [(dataset, n, s, repeat, mode)
            for dataset in datasets for n in sizes for s in separations
            for repeat in range(repeats)
            for mode in (('pure', 'samples') if repeat % 2 == 0 else ('samples', 'pure'))]


def check_result(row):
    require(row['returncode'] == 0 and 'result' in row, 'failed invocation')
    result = row['result']
    dataset, n, s, repeat, mode = row['key']
    require(row['returncode'] == 0 and row['stderr'] == '', 'failed invocation')
    require(result == json.loads(row['stdout']), 'raw result mismatch')
    require(result['status'] == 'completed' and result['public_status'] == 'not_claimed'
            and result['n'] == n and result['kmax'] == 10 and result['s'] == s
            and result['front_mode'] == mode and result['input_fnv64'] == row['input_fnv64'],
            'command/input/result mismatch')
    require(result['scope'] == 'draft_front_only_ledger_not_geometry_oracle'
            and result['separation_convention'] == 'box_gap_diameter_v1'
            and result['gcp_used'] is False and result['threads'] == 1, 'scope mismatch')
    require(all(isinstance(result[x], (int, float)) and not isinstance(result[x], bool)
                and math.isfinite(result[x]) and result[x] >= 0 for x in TIMES), 'invalid times')
    total = n * (n - 1) // 2
    require(result['total_unordered_pairs'] == total and result['active_lane_mask'] == 7, 'pair count')
    for q in range(3):
        require(result['rejected_pair_mass'][q] + result['residual_pair_mass'][q] == total, 'lane ledger')
        require(mode != 'pure' or result['rejected_pair_mass'][q] == 0, 'pure rejection')
    require(result['residual_pair_mass'] == result['callback_lane_mass']
            and result['emitted_rectangles'] == result['callback_rectangles'], 'callback ledger')


def validate(directory):
    manifest = json.loads((directory / 'MANIFEST.json').read_text())
    completion = json.loads((directory / 'COMPLETION.json').read_text())
    rows = [json.loads(line) for line in (directory / 'MEASURES.jsonl').read_text().splitlines()]
    require(completion['status'] == 'completed', 'campaign did not complete')
    require([row['key'] for row in rows] == [list(key) for key in plan(manifest['plan'])], 'campaign matrix')
    require(manifest['runner_sha256'] == digest(Path(__file__)), 'runner changed')
    for name, expected in manifest['closure_inputs'].items():
        require(digest(BASE / name) == expected, 'changed input: ' + name)
    require(completion['manifest_sha256'] == digest(directory / 'MANIFEST.json')
            and completion['measures_sha256'] == digest(directory / 'MEASURES.jsonl'), 'closure hashes')
    discrete = {}
    for row in rows:
        check_result(row)
        ds, n, s, repeat, mode = row['key']
        expected = [str(BASE / manifest['probe']), str(BASE / 'prepared' / ds / f'n{n}.u16le'), '10', str(s), mode]
        require(row['command'] == expected, 'recorded command mismatch')
        require(row['input_sha256'] == manifest['closure_inputs'][f'prepared/{ds}/n{n}.u16le'], 'row input pin')
        key = (ds, n, s, mode)
        value = {k: v for k, v in row['result'].items() if k not in TIMES}
        require(key not in discrete or discrete[key] == value, 'non-deterministic repeated work')
        discrete[key] = value
    return {'status': 'passed', 'rows': len(rows), 'plan': manifest['plan']}


def measure(name):
    directory = BASE / ('campaign_' + name)
    require(not directory.exists(), 'refuse to overwrite an existing campaign')
    binary = BASE / '.build/front_input_probe'
    build = json.loads((BASE / 'BUILD.json').read_text())
    require(build['status'] == 'passed', 'source snapshot gate did not pass')
    require(digest(binary) == build['runs'][0]['binary_sha256'], 'probe differs from built snapshot')
    entries = plan(name)
    inputs = sorted({f'prepared/{ds}/n{n}.u16le' for ds, n, _, _, _ in entries})
    closure = {path: digest(BASE / path) for path in
               ['BUILD.json', 'source_snapshot.zip', 'FETCH_MANIFEST.json', 'prepared/MANIFEST.json',
                'prepare_inputs.py', '.build/front_input_probe', *inputs]}
    checksums = {}
    for path in inputs:
        h = 14695981039346656037
        for byte in (BASE / path).read_bytes():
            h = ((h ^ byte) * 1099511628211) & ((1 << 64) - 1)
        checksums[path] = format(h, 'x')
    allowed = sorted(os.sched_getaffinity(0))
    cpu = allowed[-1]
    manifest = {'schema': 'mhgp8_real_lidar_front_campaign_v1', 'plan': name,
                'scope': 'local_front_only_on_unique_quantized_real_returns', 'public_status': 'not_claimed',
                'phase': 'exploration_v8_hors_registre', 'backend': 'cpu_reference',
                'profile': 'quantized_u16_input_only', 'mode': 'audit_independant_math_and_architecture',
                'started_utc': stamp(), 'runner_sha256': digest(Path(__file__)), 'closure_inputs': closure,
                'probe': '.build/front_input_probe', 'command': sys.argv, 'python': sys.version,
                'platform': platform.platform(), 'allowed_cpus': allowed, 'selected_cpu': cpu,
                'execution_order': 'pure,samples then samples,pure for repeat two',
                'timeout_seconds': 180, 'timeout_policy': 'failed_campaign_no_truncated_result',
                'matrix': [list(key) for key in entries]}
    directory.mkdir(); write(directory / 'MANIFEST.json', manifest)
    status = 'failed'
    try:
        with (directory / 'MEASURES.jsonl').open('x') as stream:
            for key in entries:
                ds, n, s, repeat, mode = key
                path = f'prepared/{ds}/n{n}.u16le'
                command = [str(binary), str(BASE / path), '10', str(s), mode]
                row = {'key': list(key), 'command': command, 'started_utc': stamp(),
                       'loadavg_before': os.getloadavg(), 'input_sha256': closure[path],
                       'input_fnv64': checksums[path]}
                try:
                    p = subprocess.run(command, text=True, capture_output=True, timeout=180,
                                       preexec_fn=lambda: os.sched_setaffinity(0, {cpu}))
                    row.update(returncode=p.returncode, stdout=p.stdout, stderr=p.stderr)
                    if p.returncode == 0:
                        row['result'] = json.loads(p.stdout)
                except subprocess.TimeoutExpired as error:
                    row.update(returncode=124, stdout=(error.stdout or b'').decode(),
                               stderr=(error.stderr or b'').decode(), timeout=True)
                row['finished_utc'] = stamp()
                stream.write(json.dumps(row, allow_nan=False) + '\n'); stream.flush()
                check_result(row)
                print(json.dumps({'key': key, 'front_ms': row['result']['front_callback_ms'],
                                  'rectangles': row['result']['emitted_rectangles'],
                                  'residual': row['result']['residual_pair_mass']}), flush=True)
        require(all(digest(BASE / path) == expected for path, expected in closure.items()), 'input changed during run')
        status = 'completed'
    finally:
        write(directory / 'COMPLETION.json', {'status': status, 'finished_utc': stamp(),
              'manifest_sha256': digest(directory / 'MANIFEST.json'),
              'measures_sha256': digest(directory / 'MEASURES.jsonl')})
    print(json.dumps(validate(directory)), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', choices=PLANS)
    parser.add_argument('--validate', type=Path)
    args = parser.parse_args()
    require((args.plan is None) != (args.validate is None), 'choose --plan or --validate')
    if args.validate:
        print(json.dumps(validate(args.validate.resolve())))
    else:
        measure(args.plan)
