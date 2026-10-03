"""Reconstitue les six entrees du banc v11 (empreintes du manifeste reuse1) depuis les reçus v8 du depot."""
import hashlib, json, random, struct, sys
from pathlib import Path

REPO = Path('/home/user/E-HGP')
OUT = Path(sys.argv[1])
MANIFEST = REPO / 'morsehgp3D_v11/receipts/full_regular_vertical_20261003/reuse1/inputs.json'

def verified(path, expected):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise SystemExit('hash mismatch ' + str(path))
    return raw

def words(raw):
    return [v[0] for v in struct.iter_unpack('<I', raw)]

manifest_raw = verified(MANIFEST, 'acddfeb0306be40160884dbbf6531c30ecb530565fb4da89f9d3497bdca4807e')
manifest = json.loads(manifest_raw)
OUT.mkdir(parents=True, exist_ok=True)
for case in manifest['cases']:
    p = case['provenance']
    if p['kind'] == 'lidar':
        base = REPO / Path(p['source_manifest']).relative_to('/workspaces/E-HGP').parent
        verified(base / 'MANIFEST.json', p['source_manifest_sha256'])
        mask = verified(REPO / Path(p['mask_path']).relative_to('/workspaces/E-HGP'), p['mask_sha256'])
        mapping = {name: words(verified(base / name, digest)) for name, digest in p['source_mapping_sha256'].items()}
        raw_map = mapping['raw_to_original.u32le']
        if len(raw_map) != len(mask) or len(set(raw_map)) != len(raw_map):
            raise SystemExit('raw mapping domain')
        inverse = {v: i for i, v in enumerate(raw_map)}
        ids = [inverse[v] for v in mapping['full.original_site_ids.u32le']]
        if set(ids) != {i for i, v in enumerate(mask) if v != 1}:
            raise SystemExit('ground mask coverage')
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
            raise SystemExit('reconstruction mismatch ' + filename)
        (OUT / filename).write_bytes(data)
(OUT / 'manifest.json').write_bytes(manifest_raw)
print('restoration_ok cases=%d' % len(manifest['cases']))
