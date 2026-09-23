#!/usr/bin/env python3
"""Join shadow-prefix records to the earlier exact core-edge trace."""
import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path

HERE = INPUTS = BASE = None
REC = struct.Struct('<IIIIIIII')
BASE_REC = struct.Struct('<IIII')
QUARTERS = ('quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
            'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
THRESHOLD_MM2 = 4_000 ** 2


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def normalized_patch_sha(path):
    patch = json.loads(path.read_text())
    patch.pop('source_dir', None)
    return sha((json.dumps(patch, indent=2, sort_keys=True) + '\n').encode())


def files(density):
    folder = BASE / 'full' if density == 'full' else BASE / density / 'full'
    return folder / 'SUMMARY.json', folder / 'trace'


def read_u32(path):
    data = path.read_bytes()
    if len(data) % 4:
        raise ValueError(f'{path}: bad u32 length')
    return [row[0] for row in struct.iter_unpack('<I', data)]


def inputs(density):
    manifest = json.loads((INPUTS / 'MANIFEST.json').read_text())['datasets'][density]
    full = manifest['full']
    ids = read_u32(INPUTS / full['raw_return_ids_file'])
    point_data = (INPUTS / full['points_file']).read_bytes()
    if sha(point_data) != full['points_sha256'] or \
       sha((INPUTS / full['raw_return_ids_file']).read_bytes()) != full['raw_return_ids_sha256']:
        raise ValueError('input SHA differs')
    points = list(struct.iter_unpack('<III', point_data))
    if len(ids) != len(points) or len(set(ids)) != len(ids):
        raise ValueError('full raw-ID/XYZ map bad')
    xyz = dict(zip(ids, points, strict=True))
    quadrant = {}
    for name in QUARTERS:
        meta = manifest[name]
        part = INPUTS / meta['raw_return_ids_file']
        if sha(part.read_bytes()) != meta['raw_return_ids_sha256']:
            raise ValueError(f'{name}: raw-ID SHA differs')
        for raw_id in read_u32(part):
            if raw_id in quadrant:
                raise ValueError('quadrant IDs overlap')
            quadrant[raw_id] = name
    if set(quadrant) != set(ids):
        raise ValueError('quadrants do not partition full raw IDs')
    return xyz, quadrant


def baseline_edges(density):
    path, trace_dir = files(density)
    meta = json.loads(path.read_text())
    edges = {}
    count = forms = 0
    for part in meta['trace']['parts']:
        data = (trace_dir / part['file']).read_bytes()
        if sha(data) != part['sha256'] or len(data) != BASE_REC.size * part['records']:
            raise ValueError('baseline trace part mismatch')
        for a, b, n, pre in BASE_REC.iter_unpack(data):
            key = (a << 32) | b
            if key in edges or a >= b or n < 2 or pre not in (2, 4, 6):
                raise ValueError('duplicate/bad baseline edge')
            edges[key] = (n, pre)
            count += 1
            forms += n
    if count != meta['trace']['records'] or forms != meta['trace']['sum_core_sites']:
        raise ValueError('baseline totals mismatch')
    return edges, meta


def summarize(counter):
    n = counter['loads']
    forms = counter['forms']
    saved = counter['potential_unmaterialized_forms']
    return {'loads': n, 'forms': forms, 'visited_prefix_forms': forms - saved,
            'potential_unmaterialized_forms': saved,
            'potential_fraction_forms': saved / forms if forms else None,
            'loads_full_prefix': counter['loads_full_prefix'],
            'loads_short_prefix': n - counter['loads_full_prefix'],
            'post_closed_loads': counter['post_closed_loads'],
            'post_closed_forms': counter['post_closed_forms'],
            'post_closed_potential_unmaterialized_forms':
            counter['post_closed_potential_unmaterialized_forms'],
            'depth2_scanned_cells': counter['depth2_scanned_cells'],
            'depth2_full_scans': counter['depth2_full_scans'],
            'depth2_descended_cells': counter['depth2_descended_cells']}


def main():
    global HERE, INPUTS, BASE
    ap = argparse.ArgumentParser()
    ap.add_argument('density', choices=('quarter', 'half', 'full'))
    ap.add_argument('--run-root', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--baseline-root', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--post-receipt', type=Path,
                    default=Path(__file__).resolve().parents[1] /
                    'edge_matched_core_20260923/POST_CORE_FULL.json')
    args = ap.parse_args()
    HERE, INPUTS, BASE = args.run_root, args.inputs, args.baseline_root
    density = args.density
    folder = HERE / 'runs' / f'{density}_k5'
    run = json.loads((folder / 'RUN.json').read_text())
    probe_bytes = (folder / 'stdout.json').read_bytes()
    if run['stdout_sha256'] != sha(probe_bytes):
        raise ValueError('shadow stdout hash mismatch')
    probe = json.loads(probe_bytes)
    edges, base_meta = baseline_edges(density)
    xyz, quadrant = inputs(density)
    if run['file_sha256']['input'] != base_meta['input_sha256'] or \
       run['file_sha256']['ids'] != base_meta['raw_ids_sha256'] or \
       probe['tower_digest'] != base_meta['logical_result']['tower_digest']:
        raise ValueError('source input or digest differs')
    bins = {name: Counter() for name in ('all', 'intra', 'cross_x', 'cross_y_only',
                                         'cross_x_long4m', 'cross_x_short4m')}
    intersections = {}
    masks = Counter()
    count = forms = post_closed = 0
    for name, expected_sha in run['trace_part_sha256'].items():
        data = (folder / 'trace' / name).read_bytes()
        if sha(data) != expected_sha or len(data) % REC.size:
            raise ValueError('shadow trace hash/length mismatch')
        for a, b, n, h, packed, depth2_scanned, depth2_full, depth2_descended in REC.iter_unpack(data):
            key = (a << 32) | b
            if edges.pop(key, None) != (n, packed & 0xff):
                raise ValueError(f'missing/mismatched original edge {(a, b)}')
            pre, post = packed & 0xff, (packed >> 8) & 0xff
            if a >= b or a not in xyz or b not in xyz or n < 2 or h > n or \
               packed >> 16 or pre not in (2, 4, 6) or post not in (0, 2, 4, 6) or post & ~pre or \
               depth2_scanned > 16 or depth2_full > depth2_scanned or \
               depth2_descended > depth2_full or \
               ((depth2_full or depth2_descended) and h != n):
                raise ValueError(f'invalid prefix counters for edge {(a, b)}')
            q_a, q_b = quadrant[a], quadrant[b]
            x_cross = q_a.startswith('quarter_x_neg_') != q_b.startswith('quarter_x_neg_')
            y_cross_only = q_a != q_b and not x_cross
            pa, pb = xyz[a], xyz[b]
            long4m = sum((pa[i] - pb[i]) ** 2 for i in range(3)) >= THRESHOLD_MM2
            groups = ['all', 'cross_x' if x_cross else ('cross_y_only' if y_cross_only else 'intra')]
            if x_cross:
                groups.append('cross_x_long4m' if long4m else 'cross_x_short4m')
            for group in groups:
                c = bins[group]
                c['loads'] += 1
                c['forms'] += n
                c['potential_unmaterialized_forms'] += n-h
                c['loads_full_prefix'] += h == n
                c['post_closed_loads'] += post == 0
                c['post_closed_forms'] += n if post == 0 else 0
                c['post_closed_potential_unmaterialized_forms'] += n-h if post == 0 else 0
                c[f'pre_{pre}_loads'] += 1
                c[f'after_{post}_loads'] += 1
                c[f'after_{post}_forms'] += n
                c['depth2_scanned_cells'] += depth2_scanned
                c['depth2_full_scans'] += depth2_full
                c['depth2_descended_cells'] += depth2_descended
            combo = ('x_cross' if x_cross else 'not_x_cross',
                     'long4m' if long4m else 'short4m',
                     'post_closed' if post == 0 else 'post_open')
            c = intersections.setdefault(combo, Counter())
            c['loads'] += 1
            c['forms'] += n
            c['potential_unmaterialized_forms'] += n-h
            masks[(pre, post)] += 1
            count += 1
            forms += n
            post_closed += post == 0
    if edges:
        raise ValueError(f'{len(edges)} baseline edges have no shadow mate')
    ledger = probe['ledger']
    if count != ledger['dead_core_loads'] or count != ledger['core_builds'] or \
       forms != ledger['core_sites'] or \
       forms - 2*count != ledger['dead_core_form_sites'] or \
       post_closed != ledger['core_closed_edges']:
        raise ValueError('shadow trace does not reproduce core ledger')
    post_receipt_equal = None
    if density == 'full':
        post_receipt = json.loads(args.post_receipt.read_text())
        for name, receipt in (('cross_x', post_receipt['by_physical_plane']['x_cross']),
                              ('cross_y_only', post_receipt['by_physical_plane']['y_only_cross']),
                              ('intra', post_receipt['by_sector_relation']['intra'])):
            c = bins[name]
            for key in ('loads', 'forms', 'closed_loads', 'closed_forms'):
                mine = c['post_closed_loads'] if key == 'closed_loads' else \
                       c['post_closed_forms'] if key == 'closed_forms' else c[key]
                if mine != receipt[key]:
                    raise ValueError(f'post-core receipt differs for {name}/{key}')
            for pre in (2,4,6):
                if c[f'pre_{pre}_loads'] != receipt[f'pre_{pre}_loads']:
                    raise ValueError(f'post-core receipt differs for {name}/pre_{pre}')
            for post in (0,2,4,6):
                for unit in ('loads', 'forms'):
                    if c[f'after_{post}_{unit}'] != receipt[f'after_{post}_{unit}']:
                        raise ValueError(f'post-core receipt differs for {name}/after_{post}_{unit}')
        post_receipt_equal = True
    summary = {'schema': 'mhgp9_lazy_prefix_shadow_analysis_v1', 'density': density,
               'source_commit': run['source_commit'], 'patch_provenance_sha256':
               normalized_patch_sha(HERE / 'PATCH_MANIFEST.json'),
               'input_sha256': run['file_sha256']['input'],
               'raw_ids_sha256': run['file_sha256']['ids'],
               'binary_sha256': run['file_sha256']['binary'],
               'matched_original_edges': count, 'baseline_discrete_equal':
               run['baseline_discrete_equal'],
               'checks': {'edge_join_exact': True, 'ledgers_equal': True,
                          'post_core_closure_count_equal': True,
                          'all_counter_invariants': True,
                          'quadrants_partition_full_scene': True,
                          'independent_post_core_receipt_equal': post_receipt_equal},
               'groups': {k: summarize(v) for k, v in bins.items()},
               'intersections': {'/'.join(k): summarize(v) for k,v in intersections.items()},
               'pre_post_masks': {f'{a}->{b}': n for (a,b),n in sorted(masks.items())},
               'ledger': {k: ledger[k] for k in ('dead_core_loads', 'dead_core_form_sites',
                                                'core_sites', 'core_closed_edges',
                                                'dead_core_uniform_tests')},
               'tower_digest': probe['tower_digest']}
    target = args.out
    target.write_text(json.dumps(summary, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'density': density, 'loads': count, 'forms': forms,
                      'potential_unmaterialized_forms': bins['all']['potential_unmaterialized_forms'],
                      'cross_x_forms': bins['cross_x']['forms'],
                      'cross_x_potential_unmaterialized_forms':
                      bins['cross_x']['potential_unmaterialized_forms'],
                      'output': str(target)}))


if __name__ == '__main__':
    main()
