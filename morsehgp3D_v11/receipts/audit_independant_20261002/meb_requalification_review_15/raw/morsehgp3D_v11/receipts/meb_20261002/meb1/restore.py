"""Reconstitute prior benchmark payloads exactly; preserve the historical manifest unchanged."""
import hashlib
import json
from pathlib import Path
import random
import struct

ROOT = Path('/workspaces/E-HGP/build/v11-development-20261002')
OUT = ROOT.parent / 'v11-meb-data-20261002'
OLD = ROOT / 'morsehgp3D_v11/receipts/index_20261002/index1/inputs.json'
checks = []

def verified(path, expected):
    raw = path.read_bytes()
    actual = hashlib.sha256(raw).hexdigest()
    if actual != expected:
        raise ValueError('hash mismatch ' + str(path))
    checks.append(dict(path=str(path), bytes=len(raw), sha256=actual))
    return raw

def words(raw):
    return [v[0] for v in struct.iter_unpack('<I', raw)]

manifest_raw = verified(OLD, 'acddfeb0306be40160884dbbf6531c30ecb530565fb4da89f9d3497bdca4807e')
manifest = json.loads(manifest_raw)
for case in manifest['cases']:
    p = case['provenance']
    if p['kind'] == 'lidar':
        verified(Path(p['source_manifest']), p['source_manifest_sha256'])
        base = Path(p['source_manifest']).parent
        raw = verified(Path(p['raw_path']), p['raw_sha256'])
        mask = verified(Path(p['mask_path']), p['mask_sha256'])
        mapping = {name: words(verified(base / name, digest))
                   for name, digest in p['source_mapping_sha256'].items()}
        raw_map = mapping['raw_to_original.u32le']
        if len(raw) != 16 * len(raw_map) or len(raw_map) != len(mask) or len(set(raw_map)) != len(raw_map):
            raise ValueError('raw mapping domain')
        inverse = {v: i for i, v in enumerate(raw_map)}
        ids = [inverse[v] for v in mapping['full.original_site_ids.u32le']]
        if set(ids) != {i for i, v in enumerate(mask) if v != 1}:
            raise ValueError('ground mask coverage')
        coordinates = verified(base / 'full.u32le', case['sha256'])
    else:
        rng = random.Random(p['seed'])
        points, seen = [], set()
        while len(points) < case['count']:
            xyz = tuple(rng.getrandbits(18) for _ in range(3))
            if xyz not in seen:
                points.append(xyz)
                seen.add(xyz)
        coordinates = b''.join(struct.pack('<III', *xyz) for xyz in points)
        ids = list(range(case['count']))
    id_bytes = b''.join(struct.pack('<I', v) for v in ids)
    for data, filename, digest, width in [(coordinates, case['coordinates'], case['sha256'], 12),
                                        (id_bytes, case['point_ids'], case['ids_sha256'], 4)]:
        if len(data) != width * case['count'] or hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('reconstruction mismatch ' + filename)
        (OUT / filename).write_bytes(data)
        verified(OUT / filename, digest)
(OUT / 'manifest.json').write_bytes(manifest_raw)
report = dict(schema='mhgp11.input_restoration.v1', checks=checks, outputs=12,
              manifest_unchanged=True, original_generator='historical hash retained, not rerun',
              restore_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(OUT / 'restoration.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
print('restoration_ok payloads=12 hash_checks=' + str(len(checks)))
