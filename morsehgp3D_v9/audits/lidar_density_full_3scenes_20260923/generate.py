#!/usr/bin/env python3
"""Ephemeral, reproducible global density and sensor-sector subsets."""
import hashlib
import json
import struct
from pathlib import Path

SRC = Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_02_grid')
OUT = Path('/tmp/mhgp9-density-audit-20260923')
SEED = 0xD1DA73A520260923
MASK = (1 << 64) - 1
SECTORS = ('full', 'half_x_neg', 'half_x_nonneg',
           'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
           'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')


def mix(z):
    z = (z + 0x9e3779b97f4a7c15) & MASK
    z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & MASK
    z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & MASK
    return z ^ (z >> 31)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load_words(p):
    b = p.read_bytes()
    assert len(b) % 4 == 0
    return struct.unpack(f'<{len(b)//4}I', b)


def main():
    OUT.mkdir(exist_ok=True)
    source = json.loads((SRC / 'MANIFEST.json').read_text())
    xyz = (SRC / 'full.u32le').read_bytes()
    sid = load_words(SRC / 'full.site_ids.u32le')
    oid = load_words(SRC / 'full.original_site_ids.u32le')
    n = len(sid)
    assert len(xyz) == 12*n and len(oid) == n
    for label, path in [('points_sha256', 'full.u32le'),
                        ('site_ids_sha256', 'full.site_ids.u32le'),
                        ('original_site_ids_sha256', 'full.original_site_ids.u32le')]:
        assert sha((SRC/path).read_bytes()) == source['datasets']['full'][label]
    assert n == source['datasets']['full']['sites']
    assert len(set(oid)) == n and len(set(sid)) == n
    ox, oy, _ = source['partition']['encoded_sensor_origin']
    rows = [struct.unpack_from('<III', xyz, 12*i) for i in range(n)]
    names = []
    for x,y,z in rows:
        hx = 'neg' if x < ox else 'nonneg'
        qy = 'neg' if y < oy else 'nonneg'
        names.append((f'half_x_{hx}', f'quarter_x_{hx}_y_{qy}'))
    for sector in SECTORS:
        chosen = [i for i in range(n) if sector == 'full' or sector in names[i]]
        raw = b''.join(xyz[12*i:12*i+12] for i in chosen)
        ids = b''.join(struct.pack('<I', sid[i]) for i in chosen)
        assert raw == (SRC/f'{sector}.u32le').read_bytes(), sector
        assert ids == (SRC/f'{sector}.site_ids.u32le').read_bytes(), sector
    ranks = sorted(range(n), key=lambda i: (mix(oid[i] ^ SEED), oid[i]))
    selected = {
        'quarter': set(ranks[:n//4]),
        'half': set(ranks[:n//2]),
        'full': set(range(n)),
    }
    assert selected['quarter'] < selected['half'] < selected['full']
    manifest = {
        'schema': 'ephemeral_mhgp9_lidar_density_v1',
        'source_manifest': str(SRC/'MANIFEST.json'),
        'source_manifest_sha256': sha((SRC/'MANIFEST.json').read_bytes()),
        'source_full_points_sha256': sha(xyz),
        'source_full_site_ids_sha256': sha((SRC/'full.site_ids.u32le').read_bytes()),
        'source_full_original_site_ids_sha256': sha((SRC/'full.original_site_ids.u32le').read_bytes()),
        'encoded_sensor_origin': [ox,oy,source['partition']['encoded_sensor_origin'][2]],
        'selection': {'score': 'splitmix64(original_site_id XOR seed)',
                      'seed_hex': f'{SEED:016x}',
                      'ranked_global_retained_sites': n,
                      'counts': {'quarter': n//4, 'half': n//2, 'full': n},
                      'tie_breaker': 'original_site_id',
                      'output_order': 'original_full_order',
                      'sector_policy': 'intersect global density set with encoded x/y sensor planes; boundary nonnegative'},
        'datasets': {},
    }
    for density in ('quarter','half'):
        manifest['datasets'][density] = {}
        for sector in SECTORS:
            chosen = [i for i in range(n) if i in selected[density] and
                      (sector == 'full' or sector in names[i])]
            point_data = b''.join(xyz[12*i:12*i+12] for i in chosen)
            sid_data = b''.join(struct.pack('<I',sid[i]) for i in chosen)
            oid_data = b''.join(struct.pack('<I',oid[i]) for i in chosen)
            prefix = f's02_{density}_{sector}'
            (OUT/f'{prefix}.u32le').write_bytes(point_data)
            (OUT/f'{prefix}.site_ids.u32le').write_bytes(sid_data)
            (OUT/f'{prefix}.original_site_ids.u32le').write_bytes(oid_data)
            manifest['datasets'][density][sector] = {
                'sites': len(chosen),
                'points_file': f'{prefix}.u32le',
                'points_sha256': sha(point_data),
                'site_ids_sha256': sha(sid_data),
                'original_site_ids_sha256': sha(oid_data),
            }
    for density in ('quarter','half'):
        d = manifest['datasets'][density]
        assert d['half_x_neg']['sites']+d['half_x_nonneg']['sites']==d['full']['sites']
        assert sum(d[q]['sites'] for q in SECTORS[3:])==d['full']['sites']
        assert d['quarter_x_neg_y_neg']['sites']+d['quarter_x_neg_y_nonneg']['sites']==d['half_x_neg']['sites']
        assert d['quarter_x_nonneg_y_neg']['sites']+d['quarter_x_nonneg_y_nonneg']['sites']==d['half_x_nonneg']['sites']
    (OUT/'MANIFEST.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'source_n':n,'source_sha256':manifest['source_full_points_sha256'],
                      'counts':{k:{s:v['sites'] for s,v in d.items()} for k,d in manifest['datasets'].items()}},indent=2))

if __name__=='__main__':main()
