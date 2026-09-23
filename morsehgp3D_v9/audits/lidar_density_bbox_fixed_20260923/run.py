#!/usr/bin/env python3
import hashlib
import json
import re
import struct
import subprocess
import time
from pathlib import Path

OUT=Path('/tmp/mhgp9-extrema-ablation-20260923')
BIN=Path('/workspaces/E-HGP/build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe')
SOURCE=Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_02_grid')
ORIGINAL=Path('/tmp/mhgp9-density-audit-20260923')
SUMMARY=Path('/workspaces/E-HGP/morsehgp3D_v9/audits/lidar_density_scene02_20260923/SUMMARY.json')
BINARY_SHA='e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80'
SECTOR='quarter_x_nonneg_y_neg'
MASK=(1<<64)-1

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def fnv_input(raw):
    h=14695981039346656037
    vals=(len(raw)//12,*struct.unpack(f'<{len(raw)//4}I',raw))
    for value in vals:
        for c in struct.pack('<Q',value):
            h=((h^c)*1099511628211)&MASK
    return f'{h:016x}'

def main():
    assert sha(BIN)==BINARY_SHA
    manifest=json.loads((OUT/'INPUT_MANIFEST.json').read_text())
    summary=json.loads(SUMMARY.read_text())
    baseline=summary['cases']['10'][SECTOR]
    baseline_probe=json.loads(Path(baseline['quarter']['receipt_ref']).read_text())['probe']
    expected_options=baseline_probe['options']
    cases=[
      ('original_quarter',ORIGINAL/f's02_quarter_{SECTOR}.u32le',baseline['quarter']['input_sha256']),
      ('extrema_quarter',Path(manifest['datasets']['quarter']['points_file']),manifest['datasets']['quarter']['points_sha256']),
      ('original_half',ORIGINAL/f's02_half_{SECTOR}.u32le',baseline['half']['input_sha256']),
      ('extrema_half',Path(manifest['datasets']['half']['points_file']),manifest['datasets']['half']['points_sha256']),
      ('full',SOURCE/f'{SECTOR}.u32le',baseline['full']['input_sha256']),
    ]
    for name,inp,expected_sha in cases:
        assert sha(inp)==expected_sha
        raw=inp.read_bytes()
        args=['nice','-n','19',str(BIN),str(inp),'10','8','--s=8','--static=8','--grid=1mm']
        print('START',name,'n=',len(raw)//12,flush=True)
        t0=time.monotonic()
        proc=subprocess.run(args,text=True,capture_output=True)
        wall=time.monotonic()-t0
        result={'name':name,'argv':args,'input_sha256':sha(inp),'binary_sha256':sha(BIN),
                'external_wall_s':wall,'exit_code':proc.returncode,'stderr':proc.stderr,
                'probe':json.loads(proc.stdout) if proc.returncode==0 else None}
        (OUT/f'{name}_k10.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
        if proc.returncode!=0:
            print('FAILED',name,'exit=',proc.returncode,proc.stderr,flush=True)
            raise SystemExit(proc.returncode)
        q=result['probe']
        assert q['schema']=='mhgp9_tower_probe_v12'
        assert q['status']=='complete_relative'
        assert q['input']['format']=='u32le' and q['input']['grid']=='1mm'
        assert q['input']['sites']==len(raw)//12 and q['input']['hash']==fnv_input(raw)
        assert q['options']==expected_options
        assert [o['K'] for o in q['orders']]==list(range(1,11))
        assert re.fullmatch('[0-9a-f]{16}',q['tower_digest'])
        assert q['catalogue']['balls']==q['catalogue']['unique_keys']==sum(q['catalogue']['by_qmin'])
        assert q['generator']['q3_emitted']==q['catalogue']['q3_presentations']
        assert q['generator']['q4_emitted']==q['catalogue']['q4_presentations']
        if name.startswith('original_'):
            density=name.removeprefix('original_')
            assert q['tower_digest']==baseline[density]['tower_digest']
            assert q['ledger']['dead_core_form_sites']==baseline[density]['dead_core_form_sites']
            assert q['ledger']['expanded_pairs']==baseline[density]['expanded_pairs']
        if name=='full':
            assert q['tower_digest']==baseline['full']['tower_digest']
            assert q['ledger']['dead_core_form_sites']==baseline['full']['dead_core_form_sites']
            assert q['ledger']['expanded_pairs']==baseline['full']['expanded_pairs']
        print('DONE',name,'wall_s=',round(wall,3),'cpu_s=',q['chain_cpu_s'],
              'forms=',q['ledger']['dead_core_form_sites'],'digest=',q['tower_digest'],flush=True)
    print('ALL_DONE',flush=True)

if __name__=='__main__':
    main()
