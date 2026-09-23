#!/usr/bin/env python3
"""Independent full-frame density subsets for SemanticKITTI 08/000000 and /000100."""
import hashlib
import json
import struct
from pathlib import Path
from generate import mix, SEED

BASE=Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6')
OUT=Path('/tmp/mhgp9-density-audit-20260923/crossscene')

def sha(b): return hashlib.sha256(b).hexdigest()

def words(b):
    assert len(b)%4==0
    return struct.unpack(f'<{len(b)//4}I',b)

def main():
    OUT.mkdir(exist_ok=True)
    out={'schema':'ephemeral_mhgp9_lidar_crossscene_inputs_v1',
         'selection':{'score':'splitmix64(original_site_id XOR seed)',
                      'seed_hex':f'{SEED:016x}',
                      'global_rank_over':'retained_sites_after_full_frame_ground_mask',
                      'subset_sizes':'floor(n/4), floor(n/2)',
                      'output_order':'original_full_order'},'scenes':{}}
    for scene in ('00','01'):
        src=BASE/f'scene_{scene}_grid'
        man=json.loads((src/'MANIFEST.json').read_text())
        xyz=(src/'full.u32le').read_bytes()
        sid=words((src/'full.site_ids.u32le').read_bytes())
        oid=words((src/'full.original_site_ids.u32le').read_bytes())
        n=len(sid)
        assert len(xyz)==12*n and len(oid)==n and len(set(oid))==n
        assert sha(xyz)==man['datasets']['full']['points_sha256']
        assert sha((src/'full.site_ids.u32le').read_bytes())==man['datasets']['full']['site_ids_sha256']
        assert sha((src/'full.original_site_ids.u32le').read_bytes())==man['datasets']['full']['original_site_ids_sha256']
        ranked=sorted(range(n),key=lambda i:(mix(oid[i]^SEED),oid[i]))
        ent={'source_manifest_sha256':sha((src/'MANIFEST.json').read_bytes()),
             'source_points_sha256':sha(xyz),
             'source_site_ids_sha256':sha((src/'full.site_ids.u32le').read_bytes()),
             'source_original_site_ids_sha256':sha((src/'full.original_site_ids.u32le').read_bytes()),
             'source_sites':n,'datasets':{}}
        for density,count in (('quarter',n//4),('half',n//2)):
            selected=set(ranked[:count]); indices=[i for i in range(n) if i in selected]
            assert len(indices)==count
            coords=b''.join(xyz[12*i:12*i+12] for i in indices)
            ids=b''.join(struct.pack('<I',sid[i]) for i in indices)
            original=b''.join(struct.pack('<I',oid[i]) for i in indices)
            prefix=f's{scene}_{density}_full'
            (OUT/f'{prefix}.u32le').write_bytes(coords)
            (OUT/f'{prefix}.site_ids.u32le').write_bytes(ids)
            (OUT/f'{prefix}.original_site_ids.u32le').write_bytes(original)
            ent['datasets'][density]={'sites':count,'points_file':f'{prefix}.u32le',
                                      'points_sha256':sha(coords),'site_ids_sha256':sha(ids),
                                      'original_site_ids_sha256':sha(original)}
        q=set(ranked[:n//4]); h=set(ranked[:n//2]); assert q<h
        out['scenes'][scene]=ent
    (OUT/'MANIFEST.json').write_text(json.dumps(out,indent=2,sort_keys=True)+'\n')
    print(json.dumps({s:{d:v['sites'] for d,v in e['datasets'].items()} for s,e in out['scenes'].items()},indent=2))

if __name__=='__main__': main()
