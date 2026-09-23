#!/usr/bin/env python3
"""Rebuild physically cut, nested-density u18 inputs from versioned v8 data.

The f32 sector IDs define sides of x=0/y=0 in the original sensor frame.
Every HGP coordinate is taken by original return ID from the *whole-scene*
1 mm u18 grid. No sector is retranslated or requantized.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

if not __debug__:
    raise SystemExit('This receipt generator requires Python assertions; do not run with -O.')

SOURCE = Path('morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i')
SECTORS = ('full', 'half_x_neg', 'half_x_nonneg',
           'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
           'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
SEED = 0xD1DA73A520260923
MASK = (1 << 64) - 1


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def ids(blob):
    assert len(blob) % 4 == 0
    return [row[0] for row in struct.iter_unpack('<I', blob)]


def splitmix64(value):
    value = (value + 0x9e3779b97f4a7c15) & MASK
    value = ((value ^ (value >> 30)) * 0xbf58476d1ce4e5b9) & MASK
    value = ((value ^ (value >> 27)) * 0x94d049bb133111eb) & MASK
    return value ^ (value >> 31)


def check_source(scene, manifest, name):
    row = manifest['datasets'][name]
    point_bytes = (scene / row['points_file']).read_bytes()
    id_bytes = (scene / row['site_ids_file']).read_bytes()
    assert sha(point_bytes) == row['points_sha256']
    assert sha(id_bytes) == row['site_ids_sha256']
    assert len(point_bytes) == 12 * row['sites']
    assert len(id_bytes) == 4 * row['sites']
    return point_bytes, ids(id_bytes)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--repo', type=Path, default=Path.cwd())
    p.add_argument('--out', type=Path,
                   default=Path('/tmp/mhgp9-raw-physical-density-20260923'))
    a = p.parse_args()
    grid_dir = a.repo / SOURCE / 'scene_00_000000_grid'
    f32_dir = a.repo / SOURCE / 'scene_00_000000_float32'
    a.out.mkdir(parents=True, exist_ok=True)
    gm_bytes = (grid_dir / 'MANIFEST.json').read_bytes()
    fm_bytes = (f32_dir / 'MANIFEST.json').read_bytes()
    gm, fm = json.loads(gm_bytes), json.loads(fm_bytes)
    xyz, grid_ids = check_source(grid_dir, gm, 'full')
    raw_xyz, f32_ids = check_source(f32_dir, fm, 'full')
    assert grid_ids == f32_ids == list(range(len(grid_ids)))
    assert len(xyz) == len(raw_xyz)
    n = len(grid_ids)
    # Full IDs are separately assigned after sorting each representation.
    # The two raw_to_full maps, not equality of those IDs, give the join.
    grid_map_raw = (grid_dir / gm['raw_to_full']['file']).read_bytes()
    f32_map_raw = (f32_dir / fm['raw_to_full']['file']).read_bytes()
    assert sha(grid_map_raw) == gm['raw_to_full']['sha256']
    assert sha(f32_map_raw) == fm['raw_to_full']['sha256']
    grid_map, f32_map = ids(grid_map_raw), ids(f32_map_raw)
    assert len(grid_map) == len(f32_map) == n
    assert sorted(grid_map) == sorted(f32_map) == list(range(n))
    grid_to_return = [0] * n
    f32_to_return = [0] * n
    for rid, gid in enumerate(grid_map):
        grid_to_return[gid] = rid
    for rid, fid in enumerate(f32_map):
        f32_to_return[fid] = rid
    raw_ids = list(range(n))
    ranks = sorted(raw_ids, key=lambda rid: (splitmix64(rid ^ SEED), rid))
    selected = {'quarter': set(ranks[:n // 4]),
                'half': set(ranks[:n // 2]), 'full': set(raw_ids)}
    assert selected['quarter'] < selected['half'] < selected['full']
    physical = {}
    for sector in SECTORS:
        _, sector_ids = check_source(f32_dir, fm, sector)
        assert sector_ids == sorted(set(sector_ids))
        physical[sector] = {f32_to_return[fid] for fid in sector_ids}
    assert physical['full'] == set(raw_ids)
    assert physical['half_x_neg'].isdisjoint(physical['half_x_nonneg'])
    assert physical['half_x_neg'] | physical['half_x_nonneg'] == physical['full']
    assert len(set.union(*(physical[s] for s in SECTORS[3:]))) == n
    assert sum(len(physical[s]) for s in SECTORS[3:]) == n
    assert physical['quarter_x_neg_y_neg'] | physical['quarter_x_neg_y_nonneg'] == physical['half_x_neg']
    assert physical['quarter_x_nonneg_y_neg'] | physical['quarter_x_nonneg_y_nonneg'] == physical['half_x_nonneg']
    raw_sector = {}
    for fid, rid in enumerate(f32_to_return):
        x, y, _ = struct.unpack_from('<fff', raw_xyz, 12 * fid)
        hx = 'neg' if x < 0 else 'nonneg'
        qy = 'neg' if y < 0 else 'nonneg'
        raw_sector[rid] = (f'half_x_{hx}', f'quarter_x_{hx}_y_{qy}')
    for sector in SECTORS[1:]:
        assert physical[sector] == {i for i in raw_ids if sector in raw_sector[i]}
    ox, oy, _ = gm['partition']['encoded_sensor_origin']
    moved = []
    for gid, rid in enumerate(grid_to_return):
        x, y, _ = struct.unpack_from('<III', xyz, 12 * gid)
        gh = 'neg' if x < ox else 'nonneg'
        gq = 'neg' if y < oy else 'nonneg'
        if raw_sector[rid] != (f'half_x_{gh}', f'quarter_x_{gh}_y_{gq}'):
            moved.append(rid)
    assert len(moved) == gm['boundaries']['raw_to_represented_quadrant_changes']
    manifest = {
        'schema': 'mhgp9_raw_physical_sector_density_inputs_v1',
        'source_grid_manifest': str(SOURCE / 'scene_00_000000_grid/MANIFEST.json'),
        'source_grid_manifest_sha256': sha(gm_bytes),
        'source_float32_manifest': str(SOURCE / 'scene_00_000000_float32/MANIFEST.json'),
        'source_float32_manifest_sha256': sha(fm_bytes),
        'source_grid_full_points_sha256': sha(xyz),
        'source_float32_full_points_sha256': sha(raw_xyz),
        'source_full_ids_sha256': sha((grid_dir / 'full.site_ids.u32le').read_bytes()),
        'source_grid_raw_to_full_sha256': sha(grid_map_raw),
        'source_float32_raw_to_full_sha256': sha(f32_map_raw),
        'raw_returns': n,
        'grid_unique_sites': n,
        'float32_unique_sites': n,
        'raw_to_grid_bijective': True,
        'raw_to_float32_bijective': True,
        'sector_policy': 'original float32 sensor x<0/x>=0 then y<0/y>=0; no poses',
        'geometry_policy': 'joint original return ID to whole-scene u18/1mm coordinates; no per-sector translation',
        'encoded_sensor_origin': [ox, oy, gm['partition']['encoded_sensor_origin'][2]],
        'grid_side_moved_original_ids': moved,
        'density_policy': 'global splitmix64(original return ID XOR seed), first floor(n/4), first floor(n/2), all; then sector intersection',
        'seed_hex': f'{SEED:016x}',
        'global_sites': n,
        'global_density_sizes': {'quarter': n // 4, 'half': n // 2, 'full': n},
        'datasets': {},
    }
    for density in ('quarter', 'half', 'full'):
        manifest['datasets'][density] = {}
        for sector in SECTORS:
            keep = selected[density] & physical[sector]
            chosen_grid = [gid for gid, rid in enumerate(grid_to_return) if rid in keep]
            point_bytes = b''.join(xyz[12 * gid:12 * gid + 12] for gid in chosen_grid)
            id_bytes = b''.join(struct.pack('<I', grid_to_return[gid]) for gid in chosen_grid)
            grid_id_bytes = b''.join(struct.pack('<I', gid) for gid in chosen_grid)
            prefix = f's00_{density}_{sector}'
            (a.out / f'{prefix}.u32le').write_bytes(point_bytes)
            (a.out / f'{prefix}.raw_return_ids.u32le').write_bytes(id_bytes)
            (a.out / f'{prefix}.grid_full_site_ids.u32le').write_bytes(grid_id_bytes)
            manifest['datasets'][density][sector] = {
                'sites': len(chosen_grid),
                'points_file': f'{prefix}.u32le',
                'points_sha256': sha(point_bytes),
                'raw_return_ids_file': f'{prefix}.raw_return_ids.u32le',
                'raw_return_ids_sha256': sha(id_bytes),
                'grid_full_site_ids_file': f'{prefix}.grid_full_site_ids.u32le',
                'grid_full_site_ids_sha256': sha(grid_id_bytes),
            }
        d = manifest['datasets'][density]
        assert d['half_x_neg']['sites'] + d['half_x_nonneg']['sites'] == d['full']['sites']
        assert sum(d[s]['sites'] for s in SECTORS[3:]) == d['full']['sites']
        assert d['quarter_x_neg_y_neg']['sites'] + d['quarter_x_neg_y_nonneg']['sites'] == d['half_x_neg']['sites']
        assert d['quarter_x_nonneg_y_neg']['sites'] + d['quarter_x_nonneg_y_nonneg']['sites'] == d['half_x_nonneg']['sites']
    for sector in SECTORS:
        assert set(ids((a.out / manifest['datasets']['quarter'][sector]['raw_return_ids_file']).read_bytes())) \
            < set(ids((a.out / manifest['datasets']['half'][sector]['raw_return_ids_file']).read_bytes())) \
            < set(ids((a.out / manifest['datasets']['full'][sector]['raw_return_ids_file']).read_bytes()))
    (a.out / 'MANIFEST.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'sites': {density: {s: d['sites'] for s, d in sectors.items()}
                                for density, sectors in manifest['datasets'].items()},
                      'grid_side_moved_original_ids': moved}, sort_keys=True))


if __name__ == '__main__':
    main()
