#!/usr/bin/env python3
"""Time the v12 CPU FULL chain on one physical-sector/density matrix.

The generated inputs are immutable during the run. Preparation is excluded
from all timings. A failed or timed-out case is kept, then the campaign stops.
"""
import argparse
import datetime as dt
import hashlib
import json
import resource
import struct
import subprocess
import time
from pathlib import Path

if not __debug__:
    raise SystemExit('This receipt runner requires Python assertions; do not run with -O.')

FNV_PRIME = 1099511628211
FNV_MASK = (1 << 64) - 1
DEFAULT_PROBE = Path('/workspaces/E-HGP/build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe')
SECTORS = ('quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
           'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg',
           'half_x_neg', 'half_x_nonneg')
CASES = [('full', s) for s in SECTORS] + [('quarter', 'full'), ('half', 'full'), ('full', 'full')]


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def input_fnv(raw):
    h = 14695981039346656037
    for byte in (len(raw) // 12).to_bytes(8, 'little'):
        h = ((h ^ byte) * FNV_PRIME) & FNV_MASK
    for (value,) in struct.iter_unpack('<I', raw):
        assert value < 1 << 18
        for byte in value.to_bytes(8, 'little'):
            h = ((h ^ byte) * FNV_PRIME) & FNV_MASK
    return f'{h:016x}'


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--out', type=Path, default=Path('/tmp/mhgp9-raw-physical-density-20260923'))
    p.add_argument('--probe', type=Path, default=DEFAULT_PROBE)
    p.add_argument('--timeout', type=int, default=300)
    p.add_argument('--only', nargs='*', help='case names, e.g. full_half_x_neg')
    a = p.parse_args()
    manifest = json.loads((a.out / 'MANIFEST.json').read_text())
    assert manifest['schema'] == 'mhgp9_raw_physical_sector_density_inputs_v1'
    probe_sha = sha(a.probe.read_bytes())
    completed = set()
    receipt = a.out / 'CASES.jsonl'
    if receipt.exists():
        for line in receipt.read_text().splitlines():
            if line:
                completed.add(json.loads(line)['name'])
    for density, sector in CASES:
        name = f'{density}_{sector}'
        if a.only is not None and name not in a.only:
            continue
        if name in completed:
            continue
        entry = manifest['datasets'][density][sector]
        file = a.out / entry['points_file']
        raw = file.read_bytes()
        assert sha(raw) == entry['points_sha256']
        assert len(raw) == 12 * entry['sites']
        fnv = input_fnv(raw)
        cmd = ['nice', '-n', '19', str(a.probe), str(file), '5', '8',
               '--s=8', '--static=8', '--grid=1mm']
        before = resource.getrusage(resource.RUSAGE_CHILDREN)
        started_utc = dt.datetime.now(dt.timezone.utc).isoformat()
        began = time.monotonic()
        try:
            run = subprocess.run(cmd, capture_output=True, text=True, timeout=a.timeout)
            stdout, stderr, exit_code, timed_out = run.stdout, run.stderr, run.returncode, False
        except subprocess.TimeoutExpired as exc:
            stdout = exc.stdout or b''
            stderr = exc.stderr or b''
            if isinstance(stdout, bytes):
                stdout = stdout.decode(errors='replace')
            if isinstance(stderr, bytes):
                stderr = stderr.decode(errors='replace')
            exit_code, timed_out = None, True
        elapsed = time.monotonic() - began
        ended_utc = dt.datetime.now(dt.timezone.utc).isoformat()
        after = resource.getrusage(resource.RUSAGE_CHILDREN)
        (a.out / f'{name}.stdout').write_text(stdout)
        (a.out / f'{name}.stderr').write_text(stderr)
        try:
            result = json.loads(stdout)
        except (ValueError, TypeError):
            result = None
        valid = bool(
            not timed_out and exit_code == 0 and isinstance(result, dict) and
            result.get('schema') == 'mhgp9_tower_probe_v12' and
            result.get('status') == 'complete_relative' and
            result.get('input') == {'format': 'u32le', 'grid': '1mm',
                                    'sites': entry['sites'], 'hash': fnv} and
            result.get('options', {}).get('K') == 5 and
            result.get('options', {}).get('workers') == 8 and
            result.get('options', {}).get('tower_static_threads') == 8 and
            result.get('options', {}).get('run_tower') is True and
            len(result.get('orders', [])) == 5
        )
        row = {
            'schema': 'mhgp9_raw_physical_sector_density_case_v1',
            'name': name, 'density': density, 'sector': sector,
            'sites': entry['sites'], 'input_sha256': entry['points_sha256'],
            'input_fnv': fnv, 'binary_sha256': probe_sha, 'argv': cmd,
            'started_utc': started_utc, 'ended_utc': ended_utc,
            'external_wall_s': elapsed,
            'external_cpu_user_s': after.ru_utime - before.ru_utime,
            'external_cpu_system_s': after.ru_stime - before.ru_stime,
            'timed_out': timed_out, 'exit_code': exit_code, 'validated': valid,
            'stdout_sha256': sha(stdout.encode()), 'stderr_sha256': sha(stderr.encode()),
        }
        if isinstance(result, dict):
            row['probe'] = result
        with receipt.open('a') as f:
            f.write(json.dumps(row, sort_keys=True) + '\n')
        print(json.dumps({k: row[k] for k in ('name', 'sites', 'external_wall_s',
                                            'external_cpu_user_s', 'validated')}
                         | {'chain_cpu_s': result.get('chain_cpu_s') if isinstance(result, dict) else None,
                            'dead_core_form_sites': result.get('ledger', {}).get('dead_core_form_sites')
                            if isinstance(result, dict) else None}), flush=True)
        if not valid:
            raise RuntimeError(f'case {name} did not validate; receipt preserved')


if __name__ == '__main__':
    main()
