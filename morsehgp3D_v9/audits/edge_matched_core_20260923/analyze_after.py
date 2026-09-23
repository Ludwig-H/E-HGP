#!/usr/bin/env python3
"""Attribute the exact post-dead-core lane mask on the full raw LiDAR frame."""
import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path

AUDIT = Path(__file__).resolve().parent
REPO = AUDIT.parents[2]
MANIFEST = REPO / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json'
QUARTERS = ('quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
            'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
REC = struct.Struct('<IIII')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def records(folder, summary, after):
    result = {}
    total = 0
    for part in summary['trace']['parts']:
        data = folder.joinpath('trace', part['file']).read_bytes()
        if sha(data) != part['sha256'] or len(data) % REC.size or len(data) // REC.size != part['records']:
            raise ValueError('trace part mismatch')
        for a, b, n, word in REC.iter_unpack(data):
            key = (a << 32) | b
            if key in result:
                raise ValueError('duplicate edge')
            before = word & 0xff if after else word
            post = (word >> 8) & 0xff if after else None
            if a >= b or before not in (2, 4, 6) or \
               (after and (word >> 16 or post not in (0, 2, 4, 6) or post & ~before)):
                raise ValueError('invalid core lane mask')
            result[key] = (n, before, post)
            total += n
    if len(result) != summary['trace']['records'] or total != summary['trace']['sum_core_sites']:
        raise ValueError('trace summary mismatch')
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--before-run', type=Path, required=True)
    ap.add_argument('--after-run', type=Path, required=True)
    ap.add_argument('--after-patch-manifest', type=Path, required=True)
    ap.add_argument('--after-binary', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit(f'refusing existing output: {args.out}')
    manifest_data = MANIFEST.read_bytes()
    if args.inputs.joinpath('MANIFEST.json').read_bytes() != manifest_data:
        raise ValueError('input manifest mismatch')
    meta = json.loads(manifest_data)['datasets']['full']
    sector = {}
    full_ids_data = args.inputs.joinpath(meta['full']['raw_return_ids_file']).read_bytes()
    full_points_data = args.inputs.joinpath(meta['full']['points_file']).read_bytes()
    if sha(full_ids_data) != meta['full']['raw_return_ids_sha256'] or \
       sha(full_points_data) != meta['full']['points_sha256']:
        raise ValueError('full raw-ID/point map mismatch')
    full_ids_ordered = [row[0] for row in struct.iter_unpack('<I', full_ids_data)]
    full_points_ordered = list(struct.iter_unpack('<III', full_points_data))
    if len(full_ids_ordered) != len(full_points_ordered) or \
       len(set(full_ids_ordered)) != len(full_ids_ordered):
        raise ValueError('full IDs/points not unique/aligned')
    point_by_raw = dict(zip(full_ids_ordered, full_points_ordered, strict=True))
    full_ids = set(full_ids_ordered)
    for q in QUARTERS:
        data = args.inputs.joinpath(meta[q]['raw_return_ids_file']).read_bytes()
        if sha(data) != meta[q]['raw_return_ids_sha256']:
            raise ValueError('quarter raw-ID map mismatch')
        for (raw_id,) in struct.iter_unpack('<I', data):
            if raw_id in sector:
                raise ValueError('overlapping quarters')
            sector[raw_id] = q
    if set(sector) != full_ids:
        raise ValueError('quarters do not partition full raw IDs')
    before_summary = json.loads(args.before_run.joinpath('SUMMARY.json').read_text())
    after_summary = json.loads(args.after_run.joinpath('SUMMARY.json').read_text())
    if before_summary['case'] != 'full' or after_summary['case'] != 'full':
        raise ValueError('expected full/full cases')
    if before_summary['trace'].get('trace_format', 'before') != 'before' or \
       after_summary['trace']['trace_format'] != 'before_after':
        raise ValueError('trace modes differ')
    if before_summary['input_sha256'] != after_summary['input_sha256'] or \
       before_summary['raw_ids_sha256'] != after_summary['raw_ids_sha256'] or \
       before_summary['logical_result'] != after_summary['logical_result'] or \
       before_summary['core_ledger']['core_sites'] != after_summary['core_ledger']['core_sites']:
        raise ValueError('input/result/core differs across trace builds')
    patch = json.loads(args.after_patch_manifest.read_text())
    if patch['trace_mode'] != 'before_after_core' or \
       patch['source_commit'] != 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435' or \
       sha(Path(patch['source_dir']).joinpath(patch['source_file']).read_bytes()) != patch['injected_sha256'] or \
       sha(Path(patch['source_dir']).joinpath('src/gen/pipeline/edge_trace_audit.hpp').read_bytes()) != patch['trace_header_sha256'] or \
       sha(AUDIT.joinpath('edge_trace_after_audit.hpp').read_bytes()) != patch['trace_header_sha256']:
        raise ValueError('after-core source snapshot differs')
    if sha(args.after_binary.read_bytes()) != after_summary['batch']['binary_sha256'] or \
       after_summary['batch']['binary_sha256'] != after_summary['engine']['binary_sha256']:
        raise ValueError('after-core binary differs')
    cache = args.after_binary.parent.joinpath('CMakeCache.txt').read_text()
    if 'CMAKE_BUILD_TYPE:STRING=Release' not in cache or 'MHGP9_ENABLE_CUDA:BOOL=OFF' not in cache:
        raise ValueError('after-core build is not Release CPU')
    # run_case.py already hashed stdout/stderr; repeat the essential comparison.
    for mode in ('engine', 'batch'):
        stdout = args.after_run.joinpath(f'{mode}.stdout').read_bytes()
        stderr = args.after_run.joinpath(f'{mode}.stderr').read_bytes()
        if sha(stdout) != after_summary[mode]['stdout_sha256'] or \
           sha(stderr) != after_summary[mode]['stderr_sha256'] or stderr:
            raise ValueError(f'after-core {mode} stdout/stderr differs')
    before = records(args.before_run, before_summary, False)
    after = records(args.after_run, after_summary, True)
    if before.keys() != after.keys():
        raise ValueError('trace build changed the set of core edges')
    totals = {side: Counter() for side in ('intra', 'cross')}
    planes = {side: Counter() for side in ('x_cross', 'y_only_cross')}
    length_selectors = {metres: Counter() for metres in (2, 4, 8)}
    for key, (n, pre, post) in after.items():
        if before[key] != (n, pre, None):
            raise ValueError('trace build changed core size/pre-mask')
        a, b = key >> 32, key & 0xffffffff
        side = 'intra' if sector[a] == sector[b] else 'cross'
        pa, pb = point_by_raw[a], point_by_raw[b]
        squared_mm = sum((pa[i] - pb[i]) ** 2 for i in range(3))
        for metres, row in length_selectors.items():
            if squared_mm >= (1000 * metres) ** 2:
                row['loads'] += 1
                row['forms'] += n
                if post == 0:
                    row['closed_loads'] += 1
                    row['closed_forms'] += n
                else:
                    row['open_loads'] += 1
                    row['open_forms'] += n
        subset = [] if side == 'intra' else [planes['x_cross' if
                  sector[a].startswith('quarter_x_neg_') != sector[b].startswith('quarter_x_neg_')
                  else 'y_only_cross']]
        for row in [totals[side], *subset]:
            row['loads'] += 1
            row['forms'] += n
            row[f'pre_{pre}_loads'] += 1
            row[f'after_{post}_loads'] += 1
            row[f'after_{post}_forms'] += n
            if post == 0:
                row['closed_loads'] += 1
                row['closed_forms'] += n
            else:
                row['open_loads'] += 1
                row['open_forms'] += n
    all_closed = sum(totals[s]['closed_loads'] for s in totals)
    if all_closed != after_summary['core_ledger']['core_closed_edges'] or \
       sum(totals[s]['loads'] for s in totals) != after_summary['trace']['records'] or \
       sum(totals[s]['forms'] for s in totals) != after_summary['trace']['sum_core_sites'] or \
       sum(planes[s]['loads'] for s in planes) != totals['cross']['loads'] or \
       sum(planes[s]['forms'] for s in planes) != totals['cross']['forms'] or \
       sum(planes[s]['closed_forms'] for s in planes) != totals['cross']['closed_forms'] or \
       not (length_selectors[2]['forms'] >= length_selectors[4]['forms'] >= length_selectors[8]['forms']):
        raise ValueError('post-core lane mask accounting mismatch')
    result = {'schema': 'mhgp9_edge_matched_core_post_proof_v1',
              'source_commit': patch['source_commit'],
              'input_manifest_sha256': sha(manifest_data),
              'after_patch_metadata_sha256': sha(json.dumps({k: v for k, v in patch.items()
                                                             if k != 'source_dir'},
                                                            sort_keys=True, separators=(',', ':')).encode()),
              'before_binary_sha256': before_summary['batch']['binary_sha256'],
              'after_binary_sha256': after_summary['batch']['binary_sha256'],
              'edge_sets_equal': True, 'core_sizes_and_pre_masks_equal': True,
              'logical_results_equal': True, 'core_closed_edges': all_closed,
              'by_sector_relation': {k: dict(v) for k, v in totals.items()},
              'by_physical_plane': {k: dict(v) for k, v in planes.items()},
              'intrinsic_length_selector_mm': {str(metres * 1000): dict(row)
                                               for metres, row in length_selectors.items()},
              'checks': {'sha_and_source': True, 'release_cpu_build': True,
                         'stdout_replayed': True, 'every_edge_matched': True,
                         'post_subset_pre': True, 'closed_ledger_equal': True}}
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps(result['by_sector_relation'], sort_keys=True))


if __name__ == '__main__':
    main()
