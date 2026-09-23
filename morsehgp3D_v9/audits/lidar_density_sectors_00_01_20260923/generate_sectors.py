#!/usr/bin/env python3
"""Deterministic nested density subsets intersected with encoded sensor sectors."""
import hashlib
import json
import struct
from pathlib import Path

ROOT=Path('/tmp/mhgp9-density-audit-20260923/sectors_00_01')
CROSS=ROOT.parent/'crossscene'
BASE=Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6')
SEED=0xD1DA73A520260923
MASK=(1<<64)-1
SECTORS=('half_x_neg','half_x_nonneg','quarter_x_neg_y_neg',
         'quarter_x_neg_y_nonneg','quarter_x_nonneg_y_neg','quarter_x_nonneg_y_nonneg')

def mix(z):
    z=(z+0x9e3779b97f4a7c15)&MASK
    z=((z^(z>>30))*0xbf58476d1ce4e5b9)&MASK
    z=((z^(z>>27))*0x94d049bb133111eb)&MASK
    return z^(z>>31)

def sha(b):return hashlib.sha256(b).hexdigest()

def words(b):
    assert len(b)%4==0
    return struct.unpack(f'<{len(b)//4}I',b)

def main():
    ROOT.mkdir(exist_ok=True)
    cross=json.loads((CROSS/'MANIFEST.json').read_text())
    assert cross['selection']['seed_hex']==f'{SEED:016x}'
    man={'schema':'ephemeral_mhgp9_lidar_sensor_density_sectors_v1',
         'selection':{'seed_hex':f'{SEED:016x}',
                      'score':'splitmix64(original_site_id XOR seed)',
                      'rank':'global retained sites after full-frame ground mask',
                      'sizes':'floor(n/4), floor(n/2)',
                      'order':'original full retained-site order',
                      'boundary':'encoded sensor-plane boundaries x=origin_x, y=origin_y belong nonnegative side'},
         'crossscene_manifest_sha256':sha((CROSS/'MANIFEST.json').read_bytes()),
         'source_scenes':{}}
    for scene in ('00','01'):
        src=BASE/f'scene_{scene}_grid'
        srcman=json.loads((src/'MANIFEST.json').read_text())
        xyz=(src/'full.u32le').read_bytes()
        sid=words((src/'full.site_ids.u32le').read_bytes())
        oid=words((src/'full.original_site_ids.u32le').read_bytes())
        n=len(sid)
        assert len(xyz)==12*n and len(oid)==n and len(set(oid))==n
        assert sha(xyz)==srcman['datasets']['full']['points_sha256']
        assert sha((src/'full.site_ids.u32le').read_bytes())==srcman['datasets']['full']['site_ids_sha256']
        assert sha((src/'full.original_site_ids.u32le').read_bytes())==srcman['datasets']['full']['original_site_ids_sha256']
        ox,oy,oz=srcman['partition']['encoded_sensor_origin']
        points=[struct.unpack_from('<III',xyz,12*i) for i in range(n)]
        sides=[]
        for x,y,z in points:
            hx='neg' if x<ox else 'nonneg'
            qy='neg' if y<oy else 'nonneg'
            sides.append((f'half_x_{hx}',f'quarter_x_{hx}_y_{qy}'))
        ranks=sorted(range(n),key=lambda i:(mix(oid[i]^SEED),oid[i]))
        selected={'quarter':set(ranks[:n//4]),'half':set(ranks[:n//2]),'full':set(range(n))}
        assert selected['quarter']<selected['half']<selected['full']
        for sector in SECTORS:
            take=[i for i in range(n) if sector in sides[i]]
            coords=b''.join(xyz[12*i:12*i+12] for i in take)
            ids=b''.join(struct.pack('<I',sid[i]) for i in take)
            assert coords==(src/f'{sector}.u32le').read_bytes(),sector
            assert ids==(src/f'{sector}.site_ids.u32le').read_bytes(),sector
        ent={'source_manifest_sha256':sha((src/'MANIFEST.json').read_bytes()),
             'source_full_points_sha256':sha(xyz),
             'source_full_site_ids_sha256':sha((src/'full.site_ids.u32le').read_bytes()),
             'source_full_original_site_ids_sha256':sha((src/'full.original_site_ids.u32le').read_bytes()),
             'source_sites':n,'encoded_sensor_origin':[ox,oy,oz],
             'density':{}}
        for density in ('quarter','half'):
            ent['density'][density]={}
            full_take=[i for i in range(n) if i in selected[density]]
            full_bytes=b''.join(xyz[12*i:12*i+12] for i in full_take)
            assert full_bytes==(CROSS/f's{scene}_{density}_full.u32le').read_bytes()
            assert sha(full_bytes)==cross['scenes'][scene]['datasets'][density]['points_sha256']
            full_sid=b''.join(struct.pack('<I',sid[i]) for i in full_take)
            full_oid=b''.join(struct.pack('<I',oid[i]) for i in full_take)
            assert full_sid==(CROSS/f's{scene}_{density}_full.site_ids.u32le').read_bytes()
            assert full_oid==(CROSS/f's{scene}_{density}_full.original_site_ids.u32le').read_bytes()
            for sector in SECTORS:
                take=[i for i in range(n) if i in selected[density] and sector in sides[i]]
                coords=b''.join(xyz[12*i:12*i+12] for i in take)
                ids=b''.join(struct.pack('<I',sid[i]) for i in take)
                original=b''.join(struct.pack('<I',oid[i]) for i in take)
                prefix=f's{scene}_{density}_{sector}'
                (ROOT/f'{prefix}.u32le').write_bytes(coords)
                (ROOT/f'{prefix}.site_ids.u32le').write_bytes(ids)
                (ROOT/f'{prefix}.original_site_ids.u32le').write_bytes(original)
                ent['density'][density][sector]={'sites':len(take),'points_file':f'{prefix}.u32le',
                    'points_sha256':sha(coords),'site_ids_sha256':sha(ids),'original_site_ids_sha256':sha(original)}
            ds=ent['density'][density]
            assert ds['half_x_neg']['sites']+ds['half_x_nonneg']['sites']==len(full_take)
            assert sum(ds[q]['sites'] for q in SECTORS[2:])==len(full_take)
            assert ds['quarter_x_neg_y_neg']['sites']+ds['quarter_x_neg_y_nonneg']['sites']==ds['half_x_neg']['sites']
            assert ds['quarter_x_nonneg_y_neg']['sites']+ds['quarter_x_nonneg_y_nonneg']['sites']==ds['half_x_nonneg']['sites']
        man['source_scenes'][scene]=ent
    (ROOT/'MANIFEST.json').write_text(json.dumps(man,indent=2,sort_keys=True)+'\n')
    print(json.dumps({s:{d:{q:v['sites'] for q,v in dd.items()} for d,dd in e['density'].items()} for s,e in man['source_scenes'].items()},indent=2))

if __name__=='__main__':main()
