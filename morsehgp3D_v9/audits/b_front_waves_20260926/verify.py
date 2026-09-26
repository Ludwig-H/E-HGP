#!/usr/bin/env python3
"""Portable archive reader; never runs a benchmark or depends on old binaries."""
import argparse
import hashlib
import json
import math
from pathlib import Path

def need(ok, why):
    if not ok:
        raise ValueError(why)

def read(folder):
    for line in (folder/'SHA256SUMS').read_text().splitlines():
        digest,name=line.split('  ',1)
        need(Path(name).name==name,'unsafe member')
        need(hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,'hash '+name)
    provenance=json.loads((folder/'PROVENANCE.json').read_text())
    summary=json.loads((folder/'SUMMARY.json').read_text())
    commands=json.loads((folder/'COMMANDS.json').read_text())
    need(summary['sources_stable'] is True and not summary['GCP_used'],'scope')
    need(summary['commands']==len(commands),'commands')
    rows=[]
    for i,c in enumerate(commands):
        need(c['returncode']==c['expected'],'returncode')
        out=(folder/f'{i:02d}.stdout').read_text()
        err=(folder/f'{i:02d}.stderr').read_text()
        if c['expected']:
            need(c['expected']==1 and not out and err.startswith('cause=waves.'),'mutant')
        else:
            need(not err,'stderr')
            current=[json.loads(line) for line in out.splitlines()]
            need(current,'missing rows')
            rows+=current
    need(rows==summary['rows'],'raw vs summary')
    for r in rows:
        need(r['equal_all_work'] is True and r['equal_rectangles'] is True,'equality')
        need(r['tasks']==r['children']+1,'task conservation')
        need(r['waves']==r['max_depth']+1,'wave/depth')
        need(r['max_width']<=r['tasks'] and r['rectangles']<=r['tasks'],'mass')
        t=r['raw_front_tile32']
        need(t['scope']=='before_S2_rectangle_filter','tiles scope')
        need(t['full_tiles']==t['pairs']//32,'tile floor')
        for prefix in ('dfs','waves'):
            need(t[prefix+'_contained_tiles']+t[prefix+'_crossing_tiles']==t['full_tiles'],'tile partition')
        need(sum(t['rectangles_by_mass'])==r['rectangles'],'rectangle histogram')
    if provenance['mode']=='fixtures':
        need(len(rows)==504 and summary['mutants']==4,'fixtures coverage')
        need({r['s'] for r in rows}=={8,10,12} and {r['K'] for r in rows}=={2,5,10},'fixtures K/s')
    elif provenance['mode']=='scaling':
        need(len(rows)==9,'scaling coverage')
        for kind in ('uniform','terrain','rows'):
            selected=sorted((r for r in rows if r['case']==kind),key=lambda r:r['n'])
            need([r['n'] for r in selected]==[8000,16000,32000],'sizes')
            for a,b in zip(selected,selected[1:]):
                exponents={key:math.log2(b[key]/a[key]) for key in
                    ('tasks','rectangles','h_bound_tests','witness_descent_steps') if a[key] and b[key]}
                print(json.dumps(dict(regime=kind,n1=a['n'],n2=b['n'],exponents=exponents)))
    elif provenance['mode']=='frame':
        need(len(rows)==3 and {r['s'] for r in rows}=={8,10,12},'frame s')
        need(all(r['n']*12==provenance['frame_bytes'] for r in rows),'full input')
    elif provenance['mode']=='scale32':
        need(len(rows)==1 and rows[0]['n']==32000 and rows[0]['case']=='uniform','scale32')
    if provenance.get('packed',False):
        need(all(r['packed_tasks'] is True and r['task_bytes']==16 for r in rows),'packed task layout')
    print('PASS',folder.name,len(rows),'rows',summary['mutants'],'mutants')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('folders',type=Path,nargs='+')
    for folder in p.parse_args().folders:
        read(folder)
