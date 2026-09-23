#!/usr/bin/env python3
"""Read and, when sources exist, rebuild the scene 02 physical-cut ablation."""

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
SOURCE = Path('morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6')
SECTORS = ('quarter_x_neg_y_neg', 'quarter_x_nonneg_y_neg')


def require(test, message):
    if not test:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def words(data):
    require(len(data) % 4 == 0, 'unaligned u32 payload')
    return [row[0] for row in struct.iter_unpack('<I', data)]


def source_file(folder, manifest, dataset, kind):
    entry = manifest['datasets'][dataset]
    field = {'points': 'points', 'original': 'original_site_ids',
             'site': 'site_ids'}[kind]
    data = (folder / entry[field + '_file']).read_bytes()
    require(sha(data) == entry[field + '_sha256'],
            f'source SHA mismatch: {folder.name}/{dataset}/{kind}')
    return data


def fnv_u32_points(data):
    require(len(data) % 12 == 0, 'bad point payload width')
    h = 14695981039346656037
    mask = (1 << 64) - 1
    for value in (len(data) // 12, *words(data)):
        for shift in range(0, 64, 8):
            h ^= (value >> shift) & 255
            h = (h * 1099511628211) & mask
    return f'{h:016x}'


def rebuild(manifest, output=None):
    grid = REPO / SOURCE / 'scene_02_grid'
    f32 = REPO / SOURCE / 'scene_02_float32'
    gm_raw, fm_raw = (grid / 'MANIFEST.json').read_bytes(), (f32 / 'MANIFEST.json').read_bytes()
    require(sha(gm_raw) == manifest['source_grid_manifest_sha256'], 'grid manifest SHA')
    require(sha(fm_raw) == manifest['source_float32_manifest_sha256'], 'float32 manifest SHA')
    gm, fm = json.loads(gm_raw), json.loads(fm_raw)
    grid_points = source_file(grid, gm, 'full', 'points')
    f32_points = source_file(f32, fm, 'full', 'points')
    grid_original = source_file(grid, gm, 'full', 'original')
    f32_original = source_file(f32, fm, 'full', 'original')
    require(sha(grid_points) == manifest['source_full_grid_points_sha256'], 'full grid SHA')
    require(sha(f32_points) == manifest['source_full_float32_points_sha256'], 'full float32 SHA')
    require(sha(grid_original) == manifest['source_full_grid_original_ids_sha256'], 'grid ID SHA')
    require(sha(f32_original) == manifest['source_full_float32_original_ids_sha256'], 'f32 ID SHA')
    grid_ids, f32_ids = words(grid_original), words(f32_original)
    n = 45845
    require(len(grid_ids) == len(f32_ids) == n and
            len(grid_points) == len(f32_points) == 12 * n, 'full size')
    require(words(source_file(grid, gm, 'full', 'site')) == list(range(n)), 'grid site ranks')
    require(words(source_file(f32, fm, 'full', 'site')) == list(range(n)), 'f32 site ranks')
    grid_raw = (grid / 'raw_to_original.u32le').read_bytes()
    f32_raw = (f32 / 'raw_to_original.u32le').read_bytes()
    require(sha(grid_raw) == manifest['source_grid_raw_to_original_sha256'], 'grid map SHA')
    require(sha(f32_raw) == manifest['source_float32_raw_to_original_sha256'], 'f32 map SHA')
    gmap, fmap = words(grid_raw), words(f32_raw)
    require(len(gmap) == len(fmap) == 125526 and
            len(set(gmap)) == len(set(fmap)) == len(gmap), 'raw maps not bijective')
    ginv = {original: raw for raw, original in enumerate(gmap)}
    finv = {original: raw for raw, original in enumerate(fmap)}
    greturn = [ginv[original] for original in grid_ids]
    freturn = [finv[original] for original in f32_ids]
    require(len(set(greturn)) == len(set(freturn)) == n and
            set(greturn) == set(freturn), 'retained raw returns disagree')
    moved_raw = ginv[61939]
    require(moved_raw == 14826 and fmap[moved_raw] == 61942, 'moved return identity')
    gi = grid_ids.index(61939)
    fi = f32_ids.index(61942)
    require(greturn[gi] == freturn[fi] == moved_raw, 'moved return join')
    gx, gy, _ = struct.unpack_from('<III', grid_points, 12 * gi)
    fx, fy, _ = struct.unpack_from('<fff', f32_points, 12 * fi)
    ox, oy, _ = gm['partition']['encoded_sensor_origin']
    require(gx == ox and gy < oy and fx < 0 and fy < 0, 'moved side geometry')

    sets = {sector: set() for sector in SECTORS}
    for i, raw in enumerate(freturn):
        x, y, _ = struct.unpack_from('<fff', f32_points, 12 * i)
        if y < 0:
            sets[SECTORS[0] if x < 0 else SECTORS[1]].add(raw)
    require(sets[SECTORS[0]].isdisjoint(sets[SECTORS[1]]), 'physical overlap')
    for sector in SECTORS:
        f32_sector = {finv[x] for x in words(source_file(f32, fm, sector, 'original'))}
        require(sets[sector] == f32_sector, f'f32 sector signs: {sector}')
        baseline_original = words(source_file(grid, gm, sector, 'original'))
        baseline_raw = {ginv[x] for x in baseline_original}
        baseline_points = source_file(grid, gm, sector, 'points')
        require(sha(baseline_points) == manifest['cases'][sector]['baseline_points_sha256'],
                f'baseline input SHA: {sector}')
        require(sets[sector] ^ baseline_raw == {moved_raw}, f'not one-site movement: {sector}')
        chosen = [i for i, raw in enumerate(greturn) if raw in sets[sector]]
        generated = {
            'points': b''.join(grid_points[12*i:12*i+12] for i in chosen),
            'original_site_ids': b''.join(struct.pack('<I', grid_ids[i]) for i in chosen),
            'site_ids': b''.join(struct.pack('<I', i) for i in chosen),
        }
        entry = manifest['cases'][sector]
        require(len(chosen) == entry['sites'] and len(baseline_raw) == entry['baseline_sites'],
                f'new/baseline size: {sector}')
        for kind, data in generated.items():
            require(sha(data) == entry[kind + '_sha256'], f'generated SHA: {sector}/{kind}')
            if output is not None:
                target = output / entry[kind + '_file']
                require(not target.exists() or target.read_bytes() == data,
                        f'output exists with different bytes: {target}')
                target.write_bytes(data)
        yield sector, generated['points']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--offline', action='store_true', help='check archived receipt without v8 data')
    parser.add_argument('--out', type=Path, help='regenerate the two u18 inputs here')
    parser.add_argument('--binary', type=Path, help='also check the live v12 binary SHA')
    args = parser.parse_args()
    require(not args.offline or args.out is None, '--out requires the v8 sources')
    manifest = json.loads((HERE / 'MANIFEST.json').read_text())
    results = json.loads((HERE / 'RESULTS.json').read_text())
    require(manifest['schema'] == 'mhgp9_scene02_physical_quarter_61939_ablation_v1', 'manifest schema')
    require(results['schema'] == 'mhgp9_scene02_physical_boundary_k10_ablation_v1', 'results schema')
    require(results['binary_sha256'] == manifest['binary_sha256_expected'], 'binary pin')
    source_summary = HERE.parent / 'lidar_density_scene02_20260923/SUMMARY.json'
    require(sha(source_summary.read_bytes()) == results['source_summary_sha256'], 'old summary SHA')
    old = json.loads(source_summary.read_text())['cases']['10']
    attempts = [json.loads(line) for line in (HERE / 'ATTEMPTS.jsonl').read_text().splitlines()]
    require(len(attempts) == 2 and {row['name'] for row in attempts} == set(SECTORS), 'attempt matrix')
    attempts = {row['name']: row for row in attempts}
    if args.binary:
        require(sha(args.binary.read_bytes()) == results['binary_sha256'], 'live binary SHA')
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)
    live_payloads = {} if args.offline else dict(rebuild(manifest, args.out))

    for sector in SECTORS:
        row = attempts[sector]
        ent = manifest['cases'][sector]
        probe_bytes = (HERE / (sector + '.stdout')).read_bytes()
        require(sha(probe_bytes) == row['stdout_sha256'], f'stdout SHA: {sector}')
        require(row['exit_code'] == 0 and not row['timed_out'] and row['external_wall_s'] > 0,
                f'attempt failed: {sector}')
        require(row['input_sha256_before'] == row['input_sha256_after'] == ent['points_sha256'],
                f'input mutation: {sector}')
        require(row['binary_sha256_after'] == results['binary_sha256'], f'binary mutation: {sector}')
        require(row['stderr_sha256'] == sha((HERE / (sector + '.stderr')).read_bytes()),
                f'stderr SHA: {sector}')
        require(row['argv'] == ['nice', '-n', '19', 'mhgp9_tower_probe', ent['points_file'],
                                '10', '8', '--s=8'], f'command: {sector}')
        probe = json.loads(probe_bytes)
        require(probe['schema'] == 'mhgp9_tower_probe_v12' and probe['status'] == 'complete_relative',
                f'probe status: {sector}')
        options = probe['options']
        require(options['K'] == options['K_effective'] == 10 and
                options['workers'] == options['s'] == options['tower_static_threads'] == 8 and
                options['run_tower'] and all(options['levers'].values()), f'options: {sector}')
        require(probe['input']['format'] == 'u32le' and probe['input']['sites'] == ent['sites'],
                f'probe input: {sector}')
        require([x['K'] for x in probe['orders']] == list(range(1, 11)), f'orders: {sector}')
        require(probe['catalogue']['balls'] == probe['catalogue']['unique_keys'] ==
                sum(probe['catalogue']['by_qmin']), f'catalogue: {sector}')
        ledger = probe['ledger']
        require(ledger['core_sites'] == ledger['dead_core_form_sites'] +
                2 * ledger['dead_core_loads'], f'core forms: {sector}')
        old_case = old[sector]['full']
        half = old[sector]['half']
        result = results['cases'][sector]
        base = result['baseline']
        physical = result['physical']
        require(base == {k: old_case[v] for k, v in (
            ('sites', 'n'), ('core_sites', 'core_sites'),
            ('dead_core_form_sites', 'dead_core_form_sites'),
            ('expanded_pairs', 'expanded_pairs'), ('catalogue_balls', 'catalogue_balls'),
            ('chain_cpu_s', 'chain_cpu_s'), ('chain_total_ms', 'chain_total_ms'),
            ('tower_digest', 'tower_digest'), ('input_fnv64', 'input_fnv64'))},
            f'old case mismatch: {sector}')
        expected = {
            'sites': probe['input']['sites'], 'core_sites': ledger['core_sites'],
            'dead_core_form_sites': ledger['dead_core_form_sites'],
            'dead_core_loads': ledger['dead_core_loads'],
            'expanded_pairs': ledger['expanded_pairs'],
            'catalogue_balls': probe['catalogue']['balls'],
            'tower_nodes': sum(order['nodes'] for order in probe['orders']),
            'chain_cpu_s': probe['chain_cpu_s'],
            'chain_total_ms': probe['times_ms']['chain_total'],
            'tower_digest': probe['tower_digest'], 'input_fnv64': probe['input']['hash'],
        }
        require(physical == expected, f'new metrics mismatch: {sector}')
        require(result['delta'] == {k: physical[k] - base[k] for k in (
            'sites', 'core_sites', 'dead_core_form_sites', 'expanded_pairs',
            'catalogue_balls')}, f'deltas: {sector}')
        require(result['half_density'] == {'sites': half['n'], 'core_sites': half['core_sites']},
                f'half density: {sector}')
        for label, case in (('baseline', base), ('physical', physical)):
            p = math.log(case['core_sites'] / half['core_sites']) / math.log(case['sites'] / half['n'])
            require(abs(p - result['p_core_sites_' + label]) < 1e-12,
                    f'finite slope: {sector}/{label}')
        if live_payloads:
            require(probe['input']['hash'] == fnv_u32_points(live_payloads[sector]),
                    f'FNV input hash: {sector}')
    require(results['cases']['quarter_x_nonneg_y_neg']['p_core_sites_physical'] > 2,
            'hot-quarter threshold did not persist')
    if args.offline:
        print('PASS: archived K10 outputs, SHA, counters and slopes (sources not reconstructed)')
    else:
        print('PASS: physical-cut source join, one raw-return move, K10 outputs and slopes')


if __name__ == '__main__':
    main()
