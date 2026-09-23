#!/usr/bin/env python3
"""Run only missing 1/4 and 1/2 density sensor sectors, W8 K10 then K5."""
import datetime
import hashlib
import json
import subprocess
import time
from pathlib import Path

ROOT=Path('/tmp/mhgp9-density-audit-20260923/sectors_00_01')
BIN=Path('/workspaces/E-HGP/build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe')
SECTORS=('half_x_neg','half_x_nonneg','quarter_x_neg_y_neg',
         'quarter_x_neg_y_nonneg','quarter_x_nonneg_y_neg','quarter_x_nonneg_y_nonneg')
BINARY_SHA='e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    assert sha(BIN)==BINARY_SHA
    man=json.loads((ROOT/'MANIFEST.json').read_text())
    for k in (10,5):
        for scene in ('00','01'):
            for density in ('quarter','half'):
                for sector in SECTORS:
                    name=f's{scene}_{density}_{sector}_k{k}'
                    out=ROOT/f'{name}.json'
                    if out.exists():
                        print('SKIP',name,flush=True)
                        continue
                    src=ROOT/f's{scene}_{density}_{sector}.u32le'
                    assert sha(src)==man['source_scenes'][scene]['density'][density][sector]['points_sha256']
                    args=['nice','-n','19',str(BIN),str(src),str(k),'8','--s=8','--static=8','--grid=1mm']
                    started=datetime.datetime.now(datetime.timezone.utc).isoformat()
                    t0=time.monotonic()
                    proc=subprocess.run(args,text=True,capture_output=True)
                    wall=time.monotonic()-t0
                    ended=datetime.datetime.now(datetime.timezone.utc).isoformat()
                    probe=json.loads(proc.stdout) if proc.returncode==0 else None
                    result={'schema':'ephemeral_mhgp9_lidar_sector_density_case_v1',
                            'name':name,'started_utc':started,'ended_utc':ended,'argv':args,
                            'binary_sha256':BINARY_SHA,'input_sha256':sha(src),
                            'exit_code':proc.returncode,'external_wall_s':wall,
                            'stderr':proc.stderr,'probe':probe}
                    out.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
                    if proc.returncode: raise RuntimeError(f'{name}: exit={proc.returncode}: {proc.stderr}')
                    print('DONE',name,'n=',probe['input']['sites'],'wall=',round(wall,3),
                          'pairs=',probe['ledger']['expanded_pairs'],
                          'form=',probe['ledger']['dead_core_form_sites'],
                          'status=',probe['status'],flush=True)

if __name__=='__main__':main()
