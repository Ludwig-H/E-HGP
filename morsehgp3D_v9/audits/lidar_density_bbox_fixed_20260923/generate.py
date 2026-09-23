#!/usr/bin/env python3
import hashlib
import json
import struct
from pathlib import Path

OUT = Path('/tmp/mhgp9-extrema-ablation-20260923')
ROOT = Path('/tmp/mhgp9-density-audit-20260923')
SOURCE = Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_02_grid')
SECTOR = 'quarter_x_nonneg_y_neg'
SEED = 0xD1DA73A520260923
MASK = (1 << 64) - 1

def sha(data):
    return hashlib.sha256(data).hexdigest()

def mix(z):
    z = (z + 0x9e3779b97f4a7c15) & MASK
    z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & MASK
    z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & MASK
    return z ^ (z >> 31)

def rows(prefix):
    point_bytes = Path(str(prefix) + '.u32le').read_bytes()
    sid_bytes = Path(str(prefix) + '.site_ids.u32le').read_bytes()
    oid_bytes = Path(str(prefix) + '.original_site_ids.u32le').read_bytes()
    xyz = list(struct.iter_unpack('<III', point_bytes))
    sid = [v[0] for v in struct.iter_unpack('<I', sid_bytes)]
    oid = [v[0] for v in struct.iter_unpack('<I', oid_bytes)]
    assert len(xyz) == len(sid) == len(oid)
    assert len(set(oid)) == len(oid)
    return xyz, sid, oid, (point_bytes, sid_bytes, oid_bytes)

def bbox(xyz):
    return [[min(p[a] for p in xyz), max(p[a] for p in xyz)] for a in range(3)]

def main():
    srcman = json.loads((SOURCE / 'MANIFEST.json').read_text())
    auditman = json.loads((ROOT / 'MANIFEST.json').read_text())
    assert auditman['selection']['seed_hex'] == f'{SEED:016x}'
    full = rows(SOURCE / SECTOR)
    for key, blob in zip(('points_sha256','site_ids_sha256','original_site_ids_sha256'),full[3]):
        assert sha(blob) == srcman['datasets'][SECTOR][key]
    originals = {}
    for density in ('quarter','half'):
        original = rows(ROOT / f's02_{density}_{SECTOR}')
        for key, blob in zip(('points_sha256','site_ids_sha256','original_site_ids_sha256'),original[3]):
            assert sha(blob) == auditman['datasets'][density][SECTOR][key]
        originals[density] = original
    xyz, sid, oid, _ = full
    n = len(oid)
    assert n == 14829
    rank = sorted(range(n), key=lambda i:(mix(oid[i] ^ SEED), oid[i]))
    extremal = set()
    for axis in range(3):
        extremal.add(min(range(n), key=lambda i:(xyz[i][axis],oid[i])))
        extremal.add(min(range(n), key=lambda i:(-xyz[i][axis],oid[i])))
    extremal_ids = sorted(oid[i] for i in extremal)
    assert len(extremal) <= 6
    result = {
        'schema':'mhgp9_density_extrema_ablation_inputs_v1',
        'source_scene':'08/000200',
        'sector':SECTOR,
        'seed_hex':f'{SEED:016x}',
        'selection':'sector-local hash rank (equivalent to original global-hash intersection at original cardinality), force x/y/z extrema in every density; fill remaining slots with the same hash rank',
        'source_full_points_sha256':sha(full[3][0]),
        'source_manifest_sha256':sha((SOURCE / 'MANIFEST.json').read_bytes()),
        'source_density_manifest_sha256':sha((ROOT / 'MANIFEST.json').read_bytes()),
        'encoded_sensor_origin':srcman['partition']['encoded_sensor_origin'],
        'full_bbox':bbox(xyz),
        'extremal_original_ids':extremal_ids,
        'datasets':{},
    }
    previous = None
    for density in ('quarter','half'):
        old = originals[density]
        old_ids = set(old[2])
        target = len(old[2])
        assert old_ids == {oid[i] for i in rank[:target]}
        selected = set(extremal)
        selected.update(i for i in rank if i not in extremal and len(selected) < target)
        assert len(selected) == target
        assert previous is None or previous < selected
        previous = selected
        take = [i for i in range(n) if i in selected]
        assert len(take) == target
        selected_xyz = [xyz[i] for i in take]
        assert bbox(selected_xyz) == bbox(xyz)
        point_bytes = b''.join(struct.pack('<III',*xyz[i]) for i in take)
        sid_bytes = b''.join(struct.pack('<I',sid[i]) for i in take)
        oid_bytes = b''.join(struct.pack('<I',oid[i]) for i in take)
        prefix = OUT / f's02_{density}_{SECTOR}_extrema'
        Path(str(prefix) + '.u32le').write_bytes(point_bytes)
        Path(str(prefix) + '.site_ids.u32le').write_bytes(sid_bytes)
        Path(str(prefix) + '.original_site_ids.u32le').write_bytes(oid_bytes)
        new_ids = {oid[i] for i in selected}
        result['datasets'][density] = {
            'sites':target,
            'bbox':bbox(selected_xyz),
            'points_file':str(prefix) + '.u32le',
            'points_sha256':sha(point_bytes),
            'site_ids_sha256':sha(sid_bytes),
            'original_site_ids_sha256':sha(oid_bytes),
            'added_original_ids':sorted(new_ids - old_ids),
            'evicted_original_ids':sorted(old_ids - new_ids),
            'original_points_sha256':sha(old[3][0]),
            'original_original_site_ids_sha256':sha(old[3][2]),
        }
    assert previous < set(range(n))
    assert 122516 in extremal_ids
    assert 122516 in set(struct.unpack('<' + 'I'*(len(Path(result['datasets']['quarter']['points_file'].replace('.u32le','.original_site_ids.u32le')).read_bytes())//4), Path(result['datasets']['quarter']['points_file'].replace('.u32le','.original_site_ids.u32le')).read_bytes()))
    (OUT / 'INPUT_MANIFEST.json').write_text(json.dumps(result,indent=2,sort_keys=True) + '\n')
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__ == '__main__':
    main()
