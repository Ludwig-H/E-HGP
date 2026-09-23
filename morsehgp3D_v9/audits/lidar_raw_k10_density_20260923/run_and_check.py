#!/usr/bin/env python3
"""Reproducible three-density RAW 08/000000 K10 CPU probe and strict reader."""

import argparse
import datetime as dt
import hashlib
import json
import math
import resource
import struct
import subprocess
import time
from pathlib import Path

if not __debug__:
    raise SystemExit('Assertions are required; Python -O is unsupported.')

EXPECTED_BINARY = 'e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80'
PROBE = Path('/workspaces/E-HGP/build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe')
OUT = Path('/tmp/mhgp9-raw-k10-density-20260923')
HERE = Path(__file__).resolve().parent
LEVELS = ('quarter', 'half', 'full')
FNV_PRIME = 1099511628211
MASK = (1 << 64) - 1
LEVERS = ('atlas_saturate_deep', 'q3_leaf_census', 'q34_dead_lanes',
          'q34_witness_cache', 'q34_dead_core', 'tower_meb_proposal')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fnv(raw):
    h = 14695981039346656037
    for b in (len(raw) // 12).to_bytes(8, 'little'):
        h = ((h ^ b) * FNV_PRIME) & MASK
    for (value,) in struct.iter_unpack('<I', raw):
        assert value < (1 << 18)
        for b in value.to_bytes(8, 'little'):
            h = ((h ^ b) * FNV_PRIME) & MASK
    return f'{h:016x}'


def load_manifest():
    manifest = json.loads((OUT / 'MANIFEST.json').read_text())
    assert manifest['schema'] == 'mhgp9_raw_physical_sector_density_inputs_v1'
    # Exact match to the independently published K5 matrix inputs.
    assert sha((OUT / 'MANIFEST.json').read_bytes()) == sha(
        (HERE.parent / 'lidar_raw_physical_scaling_20260923/MANIFEST.json').read_bytes())
    return manifest


def entry_for(manifest, level):
    entry = manifest['datasets'][level]['full']
    raw = (OUT / entry['points_file']).read_bytes()
    ids_raw = (OUT / entry['raw_return_ids_file']).read_bytes()
    assert sha(raw) == entry['points_sha256']
    assert sha(ids_raw) == entry['raw_return_ids_sha256']
    assert len(raw) == 12 * entry['sites']
    assert len(ids_raw) == 4 * entry['sites']
    ids = {x[0] for x in struct.iter_unpack('<I', ids_raw)}
    assert len(ids) == entry['sites']
    return entry, raw, ids


def check_probe(result, entry, fnv_hash):
    assert result['schema'] == 'mhgp9_tower_probe_v12'
    assert result['status'] == 'complete_relative'
    assert result['reason'] == 'complete_relative_to_cross_checked_catalogue'
    assert result['input'] == {'format': 'u32le', 'grid': '1mm',
                               'sites': entry['sites'], 'hash': fnv_hash}
    opt = result['options']
    assert opt['K'] == opt['K_effective'] == 10
    assert opt['s'] == opt['workers'] == opt['tower_static_threads'] == 8
    assert opt['run_tower']
    assert opt['levers'] == {name: True for name in LEVERS}
    assert len(result['orders']) == 10
    assert [x['K'] for x in result['orders']] == list(range(1, 11))
    assert all(x['nodes'] == x['births'] + x['merges'] for x in result['orders'])
    digest = result['tower_digest']
    assert isinstance(digest, str) and len(digest) == 16
    assert all(c in '0123456789abcdef' for c in digest)
    assert result['catalogue']['balls'] == result['catalogue']['unique_keys']
    assert result['catalogue']['balls'] == sum(result['catalogue']['by_qmin'])
    assert result['catalogue']['shell_over_12'] == 0
    assert result['chain_cpu_s'] > 0
    assert result['times_ms']['chain_total'] > 0


def run(manifest, timeout):
    assert sha(PROBE.read_bytes()) == EXPECTED_BINARY
    receipt = HERE / 'CASES.jsonl'
    prior = [json.loads(x) for x in receipt.read_text().splitlines() if x] if receipt.exists() else []
    done = set()
    for row in prior:
        if not row.get('validated', row['exit_code'] == 0):
            continue
        level = row['level']
        entry, raw, _ = entry_for(manifest, level)
        stdout_file = row.get('stdout_file', f'{level}.stdout')
        stderr_file = row.get('stderr_file', f'{level}.stderr')
        stdout, stderr = (HERE / stdout_file).read_bytes(), (HERE / stderr_file).read_bytes()
        assert sha(stdout) == row['stdout_sha256'] and sha(stderr) == row['stderr_sha256']
        assert row['input_sha256'] == sha(raw) and row['input_fnv'] == fnv(raw)
        assert row['binary_sha256'] == EXPECTED_BINARY
        assert row['exit_code'] == 0 and not row['timed_out']
        result = json.loads(stdout)
        assert result == row['probe']
        check_probe(result, entry, fnv(raw))
        done.add(level)
    for level in LEVELS:
        if level in done:
            continue
        attempt = sum(row['level'] == level for row in prior)
        tag = level if attempt == 0 else f'{level}.retry{attempt}'
        stdout_file, stderr_file = f'{tag}.stdout', f'{tag}.stderr'
        entry, raw, _ = entry_for(manifest, level)
        file = OUT / entry['points_file']
        cmd = ['nice', '-n', '19', str(PROBE), str(file), '10', '8',
               '--s=8', '--static=8', '--grid=1mm']
        start = dt.datetime.now(dt.timezone.utc).isoformat()
        before = resource.getrusage(resource.RUSAGE_CHILDREN)
        tick = time.monotonic()
        try:
            p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
            stdout, stderr, code, timed_out = p.stdout, p.stderr, p.returncode, False
        except subprocess.TimeoutExpired as ex:
            stdout = ex.stdout or b''
            stderr = ex.stderr or b''
            if isinstance(stdout, bytes): stdout = stdout.decode(errors='replace')
            if isinstance(stderr, bytes): stderr = stderr.decode(errors='replace')
            code, timed_out = None, True
        elapsed = time.monotonic() - tick
        after = resource.getrusage(resource.RUSAGE_CHILDREN)
        end = dt.datetime.now(dt.timezone.utc).isoformat()
        (HERE / stdout_file).write_text(stdout)
        (HERE / stderr_file).write_text(stderr)
        try:
            result = json.loads(stdout) if code == 0 and not timed_out else None
        except json.JSONDecodeError:
            result = None
        validation_error = None
        try:
            assert result is not None
            check_probe(result, entry, fnv(raw))
        except (AssertionError, KeyError, TypeError, ValueError) as ex:
            validation_error = f'{type(ex).__name__}: {ex}'
        row = {'schema': 'mhgp9_raw_k10_density_case_v1', 'level': level,
               'sites': entry['sites'], 'input_sha256': sha(raw), 'input_fnv': fnv(raw),
               'binary_sha256': EXPECTED_BINARY, 'argv': cmd,
               'started_utc': start, 'ended_utc': end,
               'external_wall_s': elapsed,
               'external_cpu_user_s': after.ru_utime - before.ru_utime,
               'external_cpu_system_s': after.ru_stime - before.ru_stime,
               'timed_out': timed_out, 'exit_code': code,
               'validated': validation_error is None,
               'validation_error': validation_error,
               'attempt': attempt, 'stdout_file': stdout_file, 'stderr_file': stderr_file,
               'stdout_sha256': sha(stdout.encode()), 'stderr_sha256': sha(stderr.encode()),
               'probe': result}
        with receipt.open('a') as f:
            f.write(json.dumps(row, sort_keys=True) + '\n')
        print(json.dumps({'level': level, 'sites': entry['sites'], 'wall_s': elapsed,
                          'cpu_s': result.get('chain_cpu_s') if isinstance(result, dict) else None,
                          'forms': result.get('ledger', {}).get('dead_core_form_sites')
                          if isinstance(result, dict) else None}),
              flush=True)
        if code != 0 or timed_out or validation_error is not None:
            raise RuntimeError(f'case {level} failed; partial receipt retained')


def verify(manifest):
    records = [json.loads(x) for x in (HERE / 'CASES.jsonl').read_text().splitlines() if x]
    assert len(records) >= len(LEVELS)
    assert {x['level'] for x in records} == set(LEVELS)
    # Preserve and hash-check failed attempts; use exactly one later success per level.
    successful = {}
    for row in records:
        level = row['level']
        stdout_file = row.get('stdout_file', f'{level}.stdout')
        stderr_file = row.get('stderr_file', f'{level}.stderr')
        stdout, stderr = (HERE / stdout_file).read_bytes(), (HERE / stderr_file).read_bytes()
        assert sha(stdout) == row['stdout_sha256'] and sha(stderr) == row['stderr_sha256']
        if row.get('validated', row['exit_code'] == 0):
            assert level not in successful
            successful[level] = row
    assert set(successful) == set(LEVELS)
    k5_dir = HERE.parent / 'lidar_raw_physical_scaling_20260923'
    k5_hashes = dict(line.split('  ', 1) for line in
                     (k5_dir / 'SHA256SUMS').read_text().splitlines())
    # The SHA file is digest-first; invert to look up each published K5 output.
    k5_hashes = {name: digest for digest, name in k5_hashes.items()}
    id_sets = []
    by_level = {}
    for level in LEVELS:
        row = successful[level]
        entry, raw, ids = entry_for(manifest, level)
        id_sets.append(ids)
        assert row['schema'] == 'mhgp9_raw_k10_density_case_v1'
        assert row['sites'] == entry['sites'] and row['input_sha256'] == sha(raw)
        assert row['input_fnv'] == fnv(raw) and row['binary_sha256'] == EXPECTED_BINARY
        assert row['argv'] == ['nice', '-n', '19', str(PROBE),
                               str(OUT / entry['points_file']), '10', '8',
                               '--s=8', '--static=8', '--grid=1mm']
        assert row['exit_code'] == 0 and not row['timed_out']
        assert row.get('validated', True)
        stdout_file = row.get('stdout_file', f'{level}.stdout')
        stderr_file = row.get('stderr_file', f'{level}.stderr')
        stdout, stderr = (HERE / stdout_file).read_bytes(), (HERE / stderr_file).read_bytes()
        assert sha(stdout) == row['stdout_sha256'] and sha(stderr) == row['stderr_sha256']
        result = json.loads(stdout)
        assert result == row['probe']
        check_probe(result, entry, fnv(raw))
        k5_file = k5_dir / f'{level}_full.stdout'
        assert sha(k5_file.read_bytes()) == k5_hashes[k5_file.name]
        k5_result = json.loads(k5_file.read_text())
        assert k5_result['input'] == result['input']
        assert k5_result['orders'] == result['orders'][:5]
        by_level[level] = {
            'sites': entry['sites'], 'chain_cpu_s': result['chain_cpu_s'],
            'chain_wall_s': result['times_ms']['chain_total'] / 1000,
            'q34_wall_s': result['times_ms']['q34'] / 1000,
            'dead_core_loads': result['ledger']['dead_core_loads'],
            'dead_core_form_sites': result['ledger']['dead_core_form_sites'],
            'expanded_pairs': result['ledger']['expanded_pairs'],
            'catalogue_balls': result['catalogue']['balls'],
            'peak_rss_kb': result['peak_rss_kb'],
            'tower_digest': result['tower_digest'],
            'orders': result['orders'],
            'first_five_orders_identical_to_published_K5': True,
        }
    assert id_sets[0] < id_sets[1] < id_sets[2]
    slopes = {}
    for a, b in zip(LEVELS, LEVELS[1:]):
        left, right = by_level[a], by_level[b]
        slopes[f'{a}_to_{b}'] = {
            key: math.log(right[key] / left[key]) / math.log(right['sites'] / left['sites'])
            for key in ('chain_cpu_s', 'chain_wall_s', 'q34_wall_s', 'dead_core_loads',
                        'dead_core_form_sites', 'expanded_pairs', 'catalogue_balls')}
    summary = {'schema': 'mhgp9_raw_k10_density_summary_v1',
               'binary_sha256': EXPECTED_BINARY, 'cases': by_level,
               'source_manifest_sha256': sha((OUT / 'MANIFEST.json').read_bytes()),
               'attempts': len(records), 'successful_cases': len(successful),
               'slopes': slopes}
    (HERE / 'SUMMARY.json').write_text(json.dumps(summary, indent=2, sort_keys=True) + '\n')
    print(json.dumps(summary, indent=2, sort_keys=True))


def main():
    p = argparse.ArgumentParser()
    p.add_argument('mode', choices=('run', 'verify'))
    p.add_argument('--timeout', type=int, default=600)
    args = p.parse_args()
    manifest = load_manifest()
    if args.mode == 'run':
        run(manifest, args.timeout)
    else:
        verify(manifest)


if __name__ == '__main__':
    main()
