#!/usr/bin/env python3
"""Join the five exact core traces by original return ID and decompose work."""
import argparse
import hashlib
import json
import random
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


def read_case(input_dir, name, meta):
    points_data = (input_dir / meta['points_file']).read_bytes()
    ids_data = (input_dir / meta['raw_return_ids_file']).read_bytes()
    if sha(points_data) != meta['points_sha256'] or sha(ids_data) != meta['raw_return_ids_sha256']:
        raise ValueError(f'{name}: input hash mismatch')
    points = list(struct.iter_unpack('<III', points_data))
    ids = [row[0] for row in struct.iter_unpack('<I', ids_data)]
    if len(points) != len(ids) or len(set(ids)) != len(ids):
        raise ValueError(f'{name}: duplicate/unaligned raw IDs')
    return dict(zip(ids, points, strict=True)), set(ids)


def projection(row):
    return {key: row[key] for key in ('input', 'generator', 'catalogue', 'orders', 'tower_digest')}


def read_trace(run_root, case, density):
    folder = run_root / case
    summary = json.loads((folder / 'SUMMARY.json').read_text())
    if summary['case'] != case or summary.get('density', 'full') != density or \
       summary['trace']['duplicate_records'] != 0:
        raise ValueError(f'{case}: invalid summary or duplicate edges')
    if summary['engine']['binary_sha256'] != summary['batch']['binary_sha256']:
        raise ValueError(f'{case}: engine and batch used different binaries')
    outputs = {}
    for mode in ('engine', 'batch'):
        stdout = (folder / f'{mode}.stdout').read_bytes()
        stderr = (folder / f'{mode}.stderr').read_bytes()
        if sha(stdout) != summary[mode]['stdout_sha256'] or \
           sha(stderr) != summary[mode]['stderr_sha256'] or stderr:
            raise ValueError(f'{case}: {mode} stdout/stderr hash mismatch')
        outputs[mode] = json.loads(stdout)
        if outputs[mode]['status'] != 'complete_relative':
            raise ValueError(f'{case}: {mode} not complete_relative')
    if projection(outputs['engine']) != projection(outputs['batch']) or \
       projection(outputs['batch']) != summary['logical_result']:
        raise ValueError(f'{case}: stdout projections differ')
    for key in ('core_builds', 'core_sites', 'dead_core_loads', 'dead_core_form_sites'):
        if outputs['engine']['ledger'][key] != outputs['batch']['ledger'][key] or \
           outputs['batch']['ledger'][key] != summary['core_ledger'][key]:
            raise ValueError(f'{case}: core ledger differs')
    edges = {}
    count = forms = 0
    for part in summary['trace']['parts']:
        data = (folder / 'trace' / part['file']).read_bytes()
        if sha(data) != part['sha256'] or len(data) // REC.size != part['records'] or len(data) % REC.size:
            raise ValueError(f'{case}: trace part mismatch')
        for a, b, n, mask in REC.iter_unpack(data):
            key = (a << 32) | b
            if key in edges or a >= b or mask not in (2, 4, 6):
                raise ValueError(f'{case}: duplicate/bad edge')
            edges[key] = (n, mask)
            count += 1
            forms += n
    trace = summary['trace']
    if count != trace['records'] or forms != trace['sum_core_sites'] or count != trace['unique_edges']:
        raise ValueError(f'{case}: trace summary mismatch')
    if count != summary['core_ledger']['dead_core_loads'] or \
       count != summary['core_ledger']['core_builds'] or \
       forms != summary['core_ledger']['core_sites'] or \
       forms != summary['core_ledger']['dead_core_form_sites'] + 2 * count:
        raise ValueError(f'{case}: core accounting mismatch')
    return edges, summary


def ids_by_geometry(edge, points):
    a, b = edge
    pa, pb = points[a], points[b]
    center = tuple(pa[i] + pb[i] for i in range(3))
    radius = sum((pa[i] - pb[i]) ** 2 for i in range(3))
    return {raw_id for raw_id, p in points.items()
            if sum((2 * p[i] - center[i]) ** 2 for i in range(3)) <= radius}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--run-root', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--density', choices=('quarter', 'half', 'full'), default='full')
    ap.add_argument('--patch-manifest', type=Path, required=True)
    ap.add_argument('--binary', type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit(f'refusing existing output: {args.out}')
    manifest_data = MANIFEST.read_bytes()
    if args.inputs.joinpath('MANIFEST.json').read_bytes() != manifest_data:
        raise SystemExit('input manifest differs from versioned copy')
    manifest = json.loads(manifest_data)['datasets'][args.density]
    point_maps = {}
    id_sets = {}
    for case in ('full',) + QUARTERS:
        point_maps[case], id_sets[case] = read_case(args.inputs, case, manifest[case])
    if sum(len(id_sets[q]) for q in QUARTERS) != len(id_sets['full']) or \
       set.union(*(id_sets[q] for q in QUARTERS)) != id_sets['full']:
        raise ValueError('quarters do not partition the full input')
    sector = {}
    for q in QUARTERS:
        for raw_id, xyz in point_maps[q].items():
            if raw_id in sector or point_maps['full'][raw_id] != xyz:
                raise ValueError('raw ID belongs to multiple quarters or changed u18 coordinates')
            sector[raw_id] = q

    quarter_edges = {}
    cases = {}
    for q in QUARTERS:
        quarter_edges[q], cases[q] = read_trace(args.run_root, q, args.density)
        if cases[q]['input_sha256'] != manifest[q]['points_sha256'] or \
           cases[q]['raw_ids_sha256'] != manifest[q]['raw_return_ids_sha256']:
            raise ValueError(f'{q}: run input differs')
        for key in quarter_edges[q]:
            if key >> 32 not in id_sets[q] or key & 0xffffffff not in id_sets[q]:
                raise ValueError(f'{q}: traced edge escapes quarter input')
    full_edges, cases['full'] = read_trace(args.run_root, 'full', args.density)
    if cases['full']['input_sha256'] != manifest['full']['points_sha256'] or \
       cases['full']['raw_ids_sha256'] != manifest['full']['raw_return_ids_sha256']:
        raise ValueError('full run input differs')
    binaries = {cases[c][mode]['binary_sha256'] for c in cases for mode in ('engine', 'batch')}
    if len(binaries) != 1 or sha(args.binary.read_bytes()) != next(iter(binaries)):
        raise ValueError('cases used different binaries')
    patch = json.loads(args.patch_manifest.read_text())
    if patch['source_commit'] != 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435':
        raise ValueError('source commit differs')
    source_dir = Path(patch['source_dir'])
    if sha((source_dir / patch['source_file']).read_bytes()) != patch['injected_sha256'] or \
       sha((source_dir / 'src/gen/pipeline/edge_trace_audit.hpp').read_bytes()) != patch['trace_header_sha256'] or \
       sha(AUDIT.joinpath('edge_trace_audit.hpp').read_bytes()) != patch['trace_header_sha256']:
        raise ValueError('patched source differs')
    cache = args.binary.parent.joinpath('CMakeCache.txt').read_text()
    if 'CMAKE_BUILD_TYPE:STRING=Release' not in cache or 'MHGP9_ENABLE_CUDA:BOOL=OFF' not in cache:
        raise ValueError('expected Release CPU-only build not found')

    # Category totals are disjoint; mask is the q3/q4 lane set BEFORE core.
    stats = {key: Counter() for key in ('common', 'full_only_intra', 'cross', 'quarter_only')}
    cross_planes = {key: Counter() for key in ('x_cross', 'y_only_cross')}
    samples = {'common': [], 'full_only_intra': [], 'cross': [], 'quarter_only': []}
    size_hist = {key: {'loads': Counter(), 'forms': Counter()} for key in ('intra', 'cross')}
    cross_sizes = []
    random_gen = random.Random(20260923)
    for key, (full_n, full_mask) in full_edges.items():
        a, b = key >> 32, key & 0xffffffff
        qa, qb = sector[a], sector[b]
        side = 'cross' if qa != qb else 'intra'
        bin_power = full_n.bit_length() - 1
        size_hist[side]['loads'][bin_power] += 1
        size_hist[side]['forms'][bin_power] += full_n
        if qa != qb:
            kind = 'cross'
            cross_sizes.append(full_n)
            plane = 'x_cross' if qa.startswith('quarter_x_neg_') != qb.startswith('quarter_x_neg_') \
                    else 'y_only_cross'
            cross_planes[plane]['loads'] += 1
            cross_planes[plane]['forms'] += full_n
            cross_planes[plane][f'mask_{full_mask}'] += 1
            stats[kind]['loads'] += 1
            stats[kind]['full_forms'] += full_n
            stats[kind][f'mask_{full_mask}'] += 1
            if len(samples[kind]) < 8:
                samples[kind].append((a, b, full_n, None, qa))
            continue
        local = quarter_edges[qa].pop(key, None)
        if local is None:
            kind = 'full_only_intra'
            stats[kind]['loads'] += 1
            stats[kind]['full_forms'] += full_n
            stats[kind][f'mask_{full_mask}'] += 1
            if len(samples[kind]) < 8:
                samples[kind].append((a, b, full_n, None, qa))
        else:
            kind = 'common'
            local_n, local_mask = local
            if local_n > full_n:
                raise ValueError(f'common edge {(a, b)} has larger quarter core')
            stats[kind]['loads'] += 1
            stats[kind]['full_forms'] += full_n
            stats[kind]['quarter_forms'] += local_n
            stats[kind]['delta_forms'] += full_n - local_n
            stats[kind][f'full_mask_{full_mask}'] += 1
            stats[kind][f'quarter_mask_{local_mask}'] += 1
            if len(samples[kind]) < 8:
                samples[kind].append((a, b, full_n, local_n, qa))
            elif random_gen.randrange(stats[kind]['loads']) < 8:
                samples[kind][random_gen.randrange(8)] = (a, b, full_n, local_n, qa)
    for q in QUARTERS:
        for key, (n, mask) in quarter_edges[q].items():
            a, b = key >> 32, key & 0xffffffff
            if sector[a] != q or sector[b] != q:
                raise ValueError('quarter edge escapes its input')
            stats['quarter_only']['loads'] += 1
            stats['quarter_only']['quarter_forms'] += n
            stats['quarter_only'][f'mask_{mask}'] += 1
            if len(samples['quarter_only']) < 8:
                samples['quarter_only'].append((a, b, None, n, q))
    full_loads = cases['full']['trace']['records']
    quarter_loads = sum(cases[q]['trace']['records'] for q in QUARTERS)
    full_forms = cases['full']['trace']['sum_core_sites']
    quarter_forms = sum(cases[q]['trace']['sum_core_sites'] for q in QUARTERS)
    if full_loads != sum(stats[k]['loads'] for k in ('common', 'full_only_intra', 'cross')) or \
       quarter_loads != stats['common']['loads'] + stats['quarter_only']['loads']:
        raise ValueError('load category accounting failed')
    if full_forms != sum(stats[k]['full_forms'] for k in ('common', 'full_only_intra', 'cross')) or \
       quarter_forms != stats['common']['quarter_forms'] + stats['quarter_only']['quarter_forms']:
        raise ValueError('form category accounting failed')
    identity_rhs = (stats['common']['delta_forms'] + stats['full_only_intra']['full_forms'] +
                    stats['cross']['full_forms'] - stats['quarter_only']['quarter_forms'])
    if full_forms - quarter_forms != identity_rhs:
        raise ValueError('form difference identity failed')
    if sum(cross_planes[p]['loads'] for p in cross_planes) != stats['cross']['loads'] or \
       sum(cross_planes[p]['forms'] for p in cross_planes) != stats['cross']['full_forms']:
        raise ValueError('cross-plane partition failed')
    if sum(size_hist[s]['loads'].total() for s in ('intra', 'cross')) != full_loads or \
       sum(size_hist[s]['forms'].total() for s in ('intra', 'cross')) != full_forms:
        raise ValueError('core-size histogram does not partition full work')
    cross_sizes.sort(reverse=True)
    top_cross = {}
    for label, numerator, denominator in (('top_1pct', 1, 100), ('top_10pct', 1, 10)):
        count = (len(cross_sizes) * numerator + denominator - 1) // denominator
        subtotal = sum(cross_sizes[:count])
        top_cross[label] = {'edges': count, 'forms': subtotal,
                            'fraction_cross_forms': subtotal / stats['cross']['full_forms'],
                            'fraction_all_full_forms': subtotal / full_forms}

    geometry_samples = []
    for kind, rows in samples.items():
        for a, b, full_n, local_n, q in rows:
            actual_full = ids_by_geometry((a, b), point_maps['full']) if full_n is not None else None
            actual_local = ids_by_geometry((a, b), point_maps[q]) if local_n is not None else None
            if (actual_full is not None and len(actual_full) != full_n) or \
               (actual_local is not None and len(actual_local) != local_n):
                raise ValueError(f'geometry sample mismatch: {kind} {(a, b)}')
            if kind == 'common' and actual_full & id_sets[q] != actual_local:
                raise ValueError(f'geometry sample intersection mismatch: {(a, b)}')
            geometry_samples.append({'category': kind, 'raw_edge': [a, b],
                                     'full_core': full_n, 'quarter_core': local_n,
                                     'sector': q})

    result = {'schema': 'mhgp9_edge_matched_core_decomposition_v1',
              'density': args.density,
              'source_commit': 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435',
              'binary_sha256': next(iter(binaries)), 'input_manifest_sha256': sha(manifest_data),
              'patch_metadata_sha256': sha(json.dumps({k: v for k, v in patch.items()
                                                       if k != 'source_dir'},
                                                      sort_keys=True, separators=(',', ':')).encode()),
              'cases': {c: {'sites': cases[c]['input_sites'],
                            'input_sha256': cases[c]['input_sha256'],
                            'raw_ids_sha256': cases[c]['raw_ids_sha256'],
                            'loads': cases[c]['trace']['records'],
                            'forms': cases[c]['trace']['sum_core_sites'],
                            'tower_digest': cases[c]['logical_result']['tower_digest']}
                        for c in cases},
              'full_loads': full_loads, 'quarter_loads': quarter_loads,
              'full_forms': full_forms, 'quarter_forms': quarter_forms,
              'form_difference': full_forms - quarter_forms, 'identity_rhs': identity_rhs,
              'categories': {key: dict(value) for key, value in stats.items()},
              'cross_planes': {key: dict(value) for key, value in cross_planes.items()},
              'full_core_size_histogram_pow2': {
                  side: {'loads': dict(sorted(part['loads'].items())),
                         'forms': dict(sorted(part['forms'].items()))}
                  for side, part in size_hist.items()},
              'cross_form_concentration': top_cross,
              'geometry_samples': geometry_samples,
              'checks': {'input_id_partition': True, 'same_xyz_for_same_raw_id': True,
                         'same_binary': True, 'stdout_replay_equal': True,
                         'release_cpu_build': True, 'unique_edges_each_case': True,
                         'loads_conserved': True, 'forms_conserved': True,
                         'common_core_monotone_all': True, 'sampled_core_geometry': True,
                         'sampled_core_membership_intersection': True}}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'loads': [full_loads, quarter_loads], 'forms': [full_forms, quarter_forms],
                      'categories': {k: {'loads': v['loads'], 'full_forms': v['full_forms'],
                                         'quarter_forms': v['quarter_forms']}
                                     for k, v in stats.items()}}, sort_keys=True))


if __name__ == '__main__':
    main()
