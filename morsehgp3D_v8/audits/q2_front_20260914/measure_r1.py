#!/usr/bin/env python3
"""Closed, bounded q2 integration measurements on existing real LiDAR inputs."""
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
ROOT = BASE.parents[2]
DATA = BASE.parent / 'lidar08_20260914/prepared'
PLANS = {
    'pilot': [('single_000000', 8000, 8, 'samples', 'shared'),
              ('single_000000', 8000, 8, 'samples', 'pairwise'),
              ('single_000000', 8000, 8, 'pure', 'shared'),
              ('single_000000', 8000, 8, 'pure', 'pairwise')],
    'growth': [('single_000000', n, 8, 'samples', 'shared') for n in (16000, 32000)],
    'other_scans': [(ds, 8000, 8, 'samples', 'shared') for ds in ('single_000100', 'single_000200')],
    'separation': [('single_000000', 8000, s, 'samples', 'shared') for s in (10, 12)],
}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fnv(path):
    h = 14695981039346656037
    for byte in path.read_bytes():
        h = ((h ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return format(h, 'x')


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def check(row):
    require(row['returncode'] == 0 and row['stderr'] == '', 'failed invocation')
    result = row['result']
    dataset, n, s, front, census = row['key']
    require(result == json.loads(row['stdout']), 'raw JSON mismatch')
    require(result['status'] == 'completed' and result['public_status'] == 'not_claimed'
            and result['scope'] == 'q2_support_stream_not_full_checksums_not_oracle'
            and result['schema'] == 'mhgp8_lidar_q2_audit_v1', 'wrong scope')
    require((result['n'], result['s'], result['front_mode'], result['census_mode']) ==
            (n, s, front, census) and result['kmax'] == 10
            and result['active_lane_mask'] == 1 and result['input_fnv64'] == row['input_fnv64'],
            'command/input/result mismatch')
    require(result['threads'] == 1 and result['gcp_used'] is False, 'execution scope mismatch')
    for key, value in result.items():
        if key.endswith('_ms'):
            require(isinstance(value, (int, float)) and not isinstance(value, bool)
                    and math.isfinite(value) and value >= 0, 'invalid duration')
    require(result['total_unordered_pairs'] == n * (n - 1) // 2, 'wrong all-pairs mass')
    require(result['candidate_pairs'] == result['accepted_pairs'] + result['rejected_pairs']
            and result['candidate_pairs'] == result['front_work']['residual_pair_mass'][0]
            and result['candidate_pairs'] + result['front_work']['rejected_pair_mass'][0]
            == result['total_unordered_pairs'], 'lost q2 pair mass')
    require(result['accepted_pairs'] == result['digest']['supports']
            and result['digest']['supports'] == result['census_work']['payload_supports'],
            'physical output ledger mismatch')


def validate(directory):
    manifest = json.loads((directory / 'MANIFEST.json').read_text())
    done = json.loads((directory / 'COMPLETION.json').read_text())
    require(done['status'] == 'completed', 'incomplete or failed campaign')
    require(manifest['runner_sha256'] == sha(Path(__file__)), 'changed runner')
    require(done['manifest_sha256'] == sha(directory / 'MANIFEST.json')
            and done['measures_sha256'] == sha(directory / 'MEASURES.jsonl'), 'broken closure')
    for path, expected in manifest['input_sha256'].items():
        require(sha(ROOT / path) == expected, 'changed input ' + path)
    rows = [json.loads(line) for line in (directory / 'MEASURES.jsonl').read_text().splitlines()]
    require([r['key'] for r in rows] == [list(key) for key in PLANS[manifest['plan']]], 'wrong matrix')
    outputs = {}
    for row in rows:
        check(row)
        ds, n, s, front, census = row['key']
        path = DATA / ds / f'n{n}.u16le'
        require(row['command'] == [str(ROOT / manifest['binary']), str(path), '10', str(s), front, census],
                'command does not reproduce the row')
        require(row['input_sha256'] == manifest['input_sha256'][str(path.relative_to(ROOT))], 'wrong input pin')
        key = ds, n
        require(key not in outputs or outputs[key] == row['result']['digest'], 'paired support digests differ')
        outputs[key] = row['result']['digest']
    return dict(status='passed', plan=manifest['plan'], rows=len(rows), output_groups=len(outputs))


def measure(plan, build):
    directory = BASE / ('campaign_' + plan)
    require(not directory.exists(), 'refuse to overwrite a campaign')
    build_receipt = BASE / (build + '_BUILD.json')
    build_value = json.loads(build_receipt.read_text())
    binary = BASE / '.build' / build / 'lidar_q2_probe'
    require(build_value['status'] == 'passed' and sha(binary) == build_value['runs'][-1]['binary_sha256'],
            'build is not closed')
    paths = {DATA / ds / f'n{n}.u16le' for ds, n, _, _, _ in PLANS[plan]}
    paths.update((binary, build_receipt, BASE / (build + '_sources.zip'), DATA / 'MANIFEST.json'))
    closure = {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}
    cpus = sorted(os.sched_getaffinity(0))
    cpu = cpus[-1]
    manifest = dict(schema='mhgp8_q2_lidar_campaign_v1', plan=plan, matrix=PLANS[plan],
                    public_status='not_claimed', scope='q2_integration_not_full_tower',
                    binary=str(binary.relative_to(ROOT)), input_sha256=closure,
                    runner_sha256=sha(Path(__file__)), command=sys.argv, platform=platform.platform(),
                    python=sys.version, allowed_cpus=cpus, selected_cpu=cpu,
                    timeout_seconds=180, timeout_policy='failed_run_no_truncated_result',
                    started_utc=stamp(), input_preparation='lidar08_20260914/INPUTS.json')
    directory.mkdir()
    write(directory / 'MANIFEST.json', manifest)
    status = 'failed'
    try:
        with (directory / 'MEASURES.jsonl').open('x') as stream:
            for key in PLANS[plan]:
                ds, n, s, front, census = key
                path = DATA / ds / f'n{n}.u16le'
                command = [str(binary), str(path), '10', str(s), front, census]
                row = dict(key=key, command=command, started_utc=stamp(), loadavg_before=os.getloadavg(),
                           input_sha256=sha(path), input_fnv64=fnv(path))
                try:
                    result = subprocess.run(command, capture_output=True, text=True, timeout=180,
                                            preexec_fn=lambda: os.sched_setaffinity(0, {cpu}))
                    row.update(returncode=result.returncode, stdout=result.stdout, stderr=result.stderr)
                    if result.returncode == 0:
                        try:
                            row['result'] = json.loads(result.stdout)
                        except json.JSONDecodeError as error:
                            row['parse_error'] = str(error)
                except subprocess.TimeoutExpired as error:
                    row.update(returncode=124, stdout=(error.stdout or b'').decode(),
                               stderr=(error.stderr or b'').decode(), timeout=True)
                row['finished_utc'] = stamp()
                stream.write(json.dumps(row, allow_nan=False) + '\n')
                stream.flush()
                check(row)
                print(json.dumps({'key': key, 'pipeline_ms': row['result']['pipeline_total_ms'],
                                  'supports': row['result']['accepted_pairs'],
                                  'count_nodes': row['result']['census_work']['count_node_visits']}), flush=True)
        require(all(sha(ROOT / path) == expected for path, expected in closure.items()), 'input changed during run')
        status = 'completed'
    finally:
        write(directory / 'COMPLETION.json', dict(status=status, finished_utc=stamp(),
              manifest_sha256=sha(directory / 'MANIFEST.json'), measures_sha256=sha(directory / 'MEASURES.jsonl')))
    print(json.dumps(validate(directory)), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', choices=PLANS)
    parser.add_argument('--build', default='r1')
    parser.add_argument('--validate', type=Path)
    args = parser.parse_args()
    require((args.plan is None) != (args.validate is None), 'choose plan or validate')
    if args.validate:
        print(json.dumps(validate(args.validate.resolve())))
    else:
        measure(args.plan, args.build)
