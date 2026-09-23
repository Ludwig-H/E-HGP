#!/usr/bin/env python3
"""Recheck a RAW physical-sector/density receipt and finite-size slopes."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

if not __debug__:
    raise SystemExit('This receipt reader requires Python assertions; do not run with -O.')

SECTORS = ('full', 'half_x_neg', 'half_x_nonneg',
           'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
           'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
CASE_NAMES = {f'full_{s}' for s in SECTORS} | {'quarter_full', 'half_full'}
LEVER_NAMES = ('atlas_saturate_deep', 'q3_leaf_census', 'q34_dead_lanes',
               'q34_witness_cache', 'q34_dead_core', 'tower_meb_proposal')
EXPECTED_BINARY_SHA256 = 'e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80'


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def ids(blob):
    assert len(blob) % 4 == 0
    return [row[0] for row in struct.iter_unpack('<I', blob)]


def slope(a, b, key):
    return math.log(b[key] / a[key]) / math.log(b['sites'] / a['sites'])


def metrics(row):
    probe = row['probe']
    return {
        'sites': row['sites'],
        'external_wall_s': row['external_wall_s'],
        'chain_wall_s': probe['times_ms']['chain_total'] / 1000,
        'chain_cpu_s': probe['chain_cpu_s'],
        'q34_wall_s': probe['times_ms']['q34'] / 1000,
        'catalogue_balls': probe['catalogue']['balls'],
        'expanded_pairs': probe['ledger']['expanded_pairs'],
        'dead_core_loads': probe['ledger']['dead_core_loads'],
        'dead_core_form_sites': probe['ledger']['dead_core_form_sites'],
        'dead_core_forms_per_load': (probe['ledger']['dead_core_form_sites'] /
                                     probe['ledger']['dead_core_loads']),
        'dead_core_uniform_tests': probe['ledger']['dead_core_uniform_tests'],
        'core_sites': probe['ledger']['core_sites'],
        'peak_rss_kb': probe['peak_rss_kb'],
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--out', type=Path, default=Path('/tmp/mhgp9-raw-physical-density-20260923'))
    p.add_argument('--repo', type=Path, default=Path.cwd())
    a = p.parse_args()
    manifest = json.loads((a.out / 'MANIFEST.json').read_text())
    assert manifest['schema'] == 'mhgp9_raw_physical_sector_density_inputs_v1'
    assert manifest['raw_to_grid_bijective'] and manifest['raw_to_float32_bijective']
    assert len(manifest['grid_side_moved_original_ids']) == 3
    for profile in ('grid', 'float32'):
        source = a.repo / manifest[f'source_{profile}_manifest']
        assert sha(source.read_bytes()) == manifest[f'source_{profile}_manifest_sha256']
    grid_dir = (a.repo / manifest['source_grid_manifest']).parent
    full_grid = (grid_dir / 'full.u32le').read_bytes()
    assert sha(full_grid) == manifest['source_grid_full_points_sha256']
    grid_map = ids((grid_dir / 'raw_to_full.u32le').read_bytes())
    assert sha((grid_dir / 'raw_to_full.u32le').read_bytes()) == manifest['source_grid_raw_to_full_sha256']
    for density in ('quarter', 'half', 'full'):
        d = manifest['datasets'][density]
        sets = {}
        for sector, entry in d.items():
            point = (a.out / entry['points_file']).read_bytes()
            rid_raw = (a.out / entry['raw_return_ids_file']).read_bytes()
            gid_raw = (a.out / entry['grid_full_site_ids_file']).read_bytes()
            assert sha(point) == entry['points_sha256']
            assert sha(rid_raw) == entry['raw_return_ids_sha256']
            assert sha(gid_raw) == entry['grid_full_site_ids_sha256']
            raw_ids, grid_ids = ids(rid_raw), ids(gid_raw)
            assert len(raw_ids) == len(grid_ids) == entry['sites'] == len(point) // 12
            assert grid_ids == sorted(set(grid_ids))
            assert len(set(raw_ids)) == len(raw_ids)
            assert all(grid_map[rid] == gid for rid, gid in zip(raw_ids, grid_ids))
            assert point == b''.join(full_grid[12 * gid:12 * gid + 12] for gid in grid_ids)
            sets[sector] = set(raw_ids)
        assert sets['half_x_neg'].isdisjoint(sets['half_x_nonneg'])
        assert sets['half_x_neg'] | sets['half_x_nonneg'] == sets['full']
        assert sum(len(sets[s]) for s in SECTORS[3:]) == len(sets['full'])
        assert set.union(*(sets[s] for s in SECTORS[3:])) == sets['full']
        assert sets['quarter_x_neg_y_neg'] | sets['quarter_x_neg_y_nonneg'] == sets['half_x_neg']
        assert sets['quarter_x_nonneg_y_neg'] | sets['quarter_x_nonneg_y_nonneg'] == sets['half_x_nonneg']
    for sector in SECTORS:
        sets = [set(ids((a.out / manifest['datasets'][d][sector]['raw_return_ids_file']).read_bytes()))
                for d in ('quarter', 'half', 'full')]
        assert sets[0] < sets[1] < sets[2]
    lines = [json.loads(line) for line in (a.out / 'CASES.jsonl').read_text().splitlines() if line]
    assert len(lines) == len(CASE_NAMES) and {line['name'] for line in lines} == CASE_NAMES
    assert {line['binary_sha256'] for line in lines} == {EXPECTED_BINARY_SHA256}
    by_name = {}
    for line in lines:
        name = line['name']
        density, sector = line['density'], line['sector']
        assert name == f'{density}_{sector}' and line['validated']
        assert line['exit_code'] == 0 and not line['timed_out']
        entry = manifest['datasets'][density][sector]
        assert line['sites'] == entry['sites'] and line['input_sha256'] == entry['points_sha256']
        argv = line['argv']
        assert len(argv) == 10 and argv[:3] == ['nice', '-n', '19']
        assert Path(argv[3]).name == 'mhgp9_tower_probe'
        assert Path(argv[4]).name == entry['points_file']
        assert argv[5:] == ['5', '8', '--s=8', '--static=8', '--grid=1mm']
        stdout = (a.out / f'{name}.stdout').read_bytes()
        stderr = (a.out / f'{name}.stderr').read_bytes()
        assert sha(stdout) == line['stdout_sha256'] and sha(stderr) == line['stderr_sha256']
        probe = json.loads(stdout)
        assert probe == line['probe']
        assert probe['schema'] == 'mhgp9_tower_probe_v12'
        assert probe['status'] == 'complete_relative'
        assert probe['input'] == {'format': 'u32le', 'grid': '1mm',
                                  'sites': entry['sites'], 'hash': line['input_fnv']}
        opt = probe['options']
        assert opt['K'] == opt['K_effective'] == 5
        assert opt['s'] == opt['workers'] == opt['tower_static_threads'] == 8
        assert opt['run_tower'] and opt['levers'] == {k: True for k in LEVER_NAMES}
        assert [o['K'] for o in probe['orders']] == [1, 2, 3, 4, 5]
        cat = probe['catalogue']
        assert cat['balls'] == cat['unique_keys'] == sum(cat['by_qmin'])
        assert cat['shell_over_12'] == 0
        assert probe['times_ms']['chain_total'] > 0 and probe['chain_cpu_s'] > 0
        by_name[name] = metrics(line)
    density = []
    for pa, ch in (('quarter_full', 'half_full'), ('half_full', 'full_full')):
        a0, b0 = by_name[pa], by_name[ch]
        density.append({'from': pa, 'to': ch, 'n_ratio': b0['sites'] / a0['sites'],
                        'p': {key: slope(a0, b0, key) for key in (
                            'external_wall_s', 'chain_wall_s', 'chain_cpu_s',
                            'q34_wall_s', 'catalogue_balls', 'expanded_pairs',
                            'dead_core_loads', 'dead_core_form_sites',
                            'dead_core_forms_per_load', 'dead_core_uniform_tests',
                            'core_sites')}})
    full = by_name['full_full']
    spatial = {}
    for part, names in (
        ('halves', ('full_half_x_neg', 'full_half_x_nonneg')),
        ('quarters', tuple('full_' + s for s in SECTORS[3:])),
    ):
        spatial[part] = {
            'quadratic_reference': sum((by_name[name]['sites'] / full['sites']) ** 2 for name in names),
            'sum_piece_over_full': {
                key: sum(by_name[name][key] for name in names) / full[key]
                for key in ('external_wall_s', 'chain_wall_s', 'chain_cpu_s',
                            'q34_wall_s', 'catalogue_balls', 'expanded_pairs',
                            'dead_core_loads', 'dead_core_form_sites',
                            'dead_core_uniform_tests', 'core_sites')},
        }
    result = {'schema': 'mhgp9_raw_physical_sector_density_summary_v1',
              'binary_sha256': lines[0]['binary_sha256'],
              'n_valid_cases': len(lines), 'metrics': by_name,
              'density_slopes': density, 'spatial_sums': spatial}
    (a.out / 'SUMMARY.json').write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'n_valid_cases': len(lines), 'density_slopes': density,
                      'spatial_sums': spatial}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
