#!/usr/bin/env python3
"""Run one pinned CPU S2 engine/batch pair and verify its core trace."""
import argparse
import hashlib
import json
import os
import struct
import subprocess
import time
from collections import Counter
from pathlib import Path

AUDIT = Path(__file__).resolve().parent
REPO = AUDIT.parents[2]
INPUT_MANIFEST = REPO / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json'
EXPECTED_INPUT_MANIFEST_SHA = '6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095'
QUARTERS = ('quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
            'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
CASES = ('full',) + QUARTERS
REC = struct.Struct('<IIII')
FNV_PRIME = 1099511628211
FNV_MASK = (1 << 64) - 1


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_input(data, raw_ids):
    if len(data) % 12 or len(raw_ids) * 3 != len(data) or len(raw_ids) % 4:
        raise ValueError('point/raw-ID lengths differ')
    ids = [row[0] for row in struct.iter_unpack('<I', raw_ids)]
    if len(set(ids)) != len(ids) or max(ids, default=0) >= 123389:
        raise ValueError('raw IDs are not unique/in domain')
    h = 14695981039346656037
    for word in (len(ids), *(v for xyz in struct.iter_unpack('<III', data) for v in xyz)):
        for byte in word.to_bytes(8, 'little'):
            h = ((h ^ byte) * FNV_PRIME) & FNV_MASK
    return ids, f'{h:016x}'


def projection(row):
    return {key: row[key] for key in ('input', 'generator', 'catalogue', 'orders', 'tower_digest')}


def run_probe(binary, point_file, raw_id_file, trace_dir, mode, cwd):
    command = [str(binary), str(point_file), '5', '8', '--s=8', '--static=8', '--grid=1mm',
               f'--lever=q34_batch_filter={int(mode == "batch")}', '--lever=q34_gpu_filter=0']
    env = os.environ.copy()
    env.pop('MHGP9_AUDIT_TRACE_DIR', None)
    env.pop('MHGP9_AUDIT_RAW_IDS', None)
    if mode == 'batch':
        trace_dir.mkdir()
        env['MHGP9_AUDIT_TRACE_DIR'] = str(trace_dir)
        env['MHGP9_AUDIT_RAW_IDS'] = str(raw_id_file)
    out = cwd / f'{mode}.stdout'
    err = cwd / f'{mode}.stderr'
    before = (sha(point_file.read_bytes()), sha(raw_id_file.read_bytes()), sha(binary.read_bytes()))
    start = time.monotonic()
    with out.open('wb') as stdout, err.open('wb') as stderr:
        completed = subprocess.run(command, env=env, stdout=stdout, stderr=stderr,
                                   check=False, timeout=1800)
    elapsed = time.monotonic() - start
    after = (sha(point_file.read_bytes()), sha(raw_id_file.read_bytes()), sha(binary.read_bytes()))
    if before != after or completed.returncode != 0:
        raise RuntimeError(f'{mode} failed: return={completed.returncode}, immutable={before == after}, '
                           f'stderr={err.read_text()[:500]}')
    row = json.loads(out.read_text())
    if row['status'] != 'complete_relative':
        raise RuntimeError(f'{mode} status is {row["status"]}')
    return row, {'argv': command, 'returncode': completed.returncode, 'elapsed_s': elapsed,
                 'stdout_sha256': sha(out.read_bytes()), 'stderr_sha256': sha(err.read_bytes()),
                 'input_sha256': before[0], 'raw_ids_sha256': before[1], 'binary_sha256': before[2]}


def trace_summary(trace_dir, trace_format):
    count = total = duplicates = 0
    masks = Counter()
    after_masks = Counter()
    seen = set()
    parts = []
    for path in sorted(trace_dir.glob('part_*.bin')):
        blob = path.read_bytes()
        if len(blob) % REC.size:
            raise ValueError(f'partial record in {path}')
        for a, b, size, packed in REC.iter_unpack(blob):
            mask = packed if trace_format == 'before' else packed & 0xff
            after = None if trace_format == 'before' else (packed >> 8) & 0xff
            if a >= b or a >= 123389 or b >= 123389 or size < 2 or mask not in (2, 4, 6) or \
               (trace_format == 'before_after' and
                (packed >> 16 != 0 or after not in (0, 2, 4, 6) or after & ~mask)):
                raise ValueError(f'invalid record in {path}')
            key = (a << 32) | b
            if key in seen:
                duplicates += 1
            else:
                seen.add(key)
            count += 1
            total += size
            masks[mask] += 1
            if after is not None:
                after_masks[after] += 1
        parts.append({'file': path.name, 'records': len(blob) // REC.size, 'sha256': sha(blob)})
    if not parts:
        raise ValueError('empty trace directory')
    result = {'records': count, 'unique_edges': len(seen), 'duplicate_records': duplicates,
              'sum_core_sites': total, 'mask_before_core': dict(sorted(masks.items())), 'parts': parts,
              'trace_format': trace_format}
    if trace_format == 'before_after':
        result['mask_after_core'] = dict(sorted(after_masks.items()))
        result['core_closed_edges'] = after_masks[0]
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--binary', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--case', choices=('preflight',) + CASES, required=True)
    ap.add_argument('--density', choices=('quarter', 'half', 'full'), default='full')
    ap.add_argument('--trace-format', choices=('before', 'before_after'), default='before')
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit(f'refusing existing case output: {args.out}')
    args.out.mkdir(parents=True)
    input_manifest_bytes = INPUT_MANIFEST.read_bytes()
    if sha(input_manifest_bytes) != EXPECTED_INPUT_MANIFEST_SHA:
        raise SystemExit('versioned input manifest changed')
    if args.inputs.joinpath('MANIFEST.json').read_bytes() != input_manifest_bytes:
        raise SystemExit('regenerated input manifest differs')
    manifest = json.loads(input_manifest_bytes)
    source_case = 'full' if args.case == 'preflight' else args.case
    meta = manifest['datasets'][args.density][source_case]
    original_points = args.inputs / meta['points_file']
    original_ids = args.inputs / meta['raw_return_ids_file']
    data, id_data = original_points.read_bytes(), original_ids.read_bytes()
    if sha(data) != meta['points_sha256'] or sha(id_data) != meta['raw_return_ids_sha256']:
        raise SystemExit('input SHA mismatch')
    if args.case == 'preflight':
        data, id_data = data[:1500 * 12], id_data[:1500 * 4]
        point_file = args.out / 'preflight.u32le'
        raw_id_file = args.out / 'preflight.raw_return_ids.u32le'
        point_file.write_bytes(data)
        raw_id_file.write_bytes(id_data)
    else:
        point_file, raw_id_file = original_points, original_ids
    ids, input_fnv = read_input(data, id_data)
    engine, engine_meta = run_probe(args.binary, point_file, raw_id_file,
                                    args.out / 'engine_trace_unused', 'engine', args.out)
    batch, batch_meta = run_probe(args.binary, point_file, raw_id_file,
                                  args.out / 'trace', 'batch', args.out)
    if engine['input']['hash'] != input_fnv or batch['input']['hash'] != input_fnv:
        raise RuntimeError('independent input FNV mismatch')
    if engine['input']['sites'] != len(ids) or batch['input']['sites'] != len(ids):
        raise RuntimeError('input site count mismatch')
    if projection(engine) != projection(batch):
        raise RuntimeError('engine and batch logical outputs differ')
    for key in ('core_builds', 'core_sites', 'dead_core_loads', 'dead_core_form_sites',
                'core_closed_edges'):
        if engine['ledger'][key] != batch['ledger'][key]:
            raise RuntimeError(f'engine/batch {key} differs')
    trace = trace_summary(args.out / 'trace', args.trace_format)
    if trace['duplicate_records'] != 0:
        raise RuntimeError('duplicate core edge: set-based matching would be invalid')
    ledger = batch['ledger']
    if trace['records'] != ledger['dead_core_loads'] or trace['records'] != ledger['core_builds']:
        raise RuntimeError('trace/core load count mismatch')
    if trace['sum_core_sites'] != ledger['core_sites'] or \
       trace['sum_core_sites'] != ledger['dead_core_form_sites'] + 2 * ledger['dead_core_loads']:
        raise RuntimeError('trace/core form count mismatch')
    if args.trace_format == 'before_after' and trace['core_closed_edges'] != ledger['core_closed_edges']:
        raise RuntimeError('post-core mask does not reproduce closed edge count')
    summary = {'schema': 'mhgp9_edge_matched_core_case_v1', 'case': args.case,
               'density': args.density,
               'source_case': source_case, 'input_manifest_sha256': EXPECTED_INPUT_MANIFEST_SHA,
               'input_sites': len(ids), 'input_fnv': input_fnv,
               'input_sha256': sha(data), 'raw_ids_sha256': sha(id_data),
               'engine': engine_meta, 'batch': batch_meta,
               'logical_result': projection(batch),
               'core_ledger': {k: ledger[k] for k in ('core_builds', 'core_sites', 'dead_core_loads',
                                                       'dead_core_form_sites', 'core_closed_edges')},
               'trace': trace}
    (args.out / 'SUMMARY.json').write_text(json.dumps(summary, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'case': args.case, 'density': args.density, 'sites': len(ids), 'loads': trace['records'],
                      'forms': trace['sum_core_sites'], 'duplicates': trace['duplicate_records'],
                      'digest': batch['tower_digest']}, sort_keys=True))


if __name__ == '__main__':
    main()
