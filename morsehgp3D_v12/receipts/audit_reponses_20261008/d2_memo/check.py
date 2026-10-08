#!/usr/bin/env python3
"""Preuves Git + petits calculs Fraction ; aucun moteur natif."""
import argparse
from fractions import Fraction as F
import hashlib
import itertools
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def orient(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def dist(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))


def circles(points):
    out = {}
    for q in (2, 3):
        for pts in itertools.combinations(points, q):
            a, b = pts[:2]
            if q == 2:
                c = tuple(F(x+y, 2) for x, y in zip(a, b))
            else:
                z = pts[2]
                u, v = (b[0]-a[0], b[1]-a[1]), (z[0]-a[0], z[1]-a[1])
                den = u[0]*v[1]-u[1]*v[0]
                if not den:
                    continue
                h = F(dist(b, (0, 0))-dist(a, (0, 0)), 2)
                j = F(dist(z, (0, 0))-dist(a, (0, 0)), 2)
                c = ((h*v[1]-j*u[1])/den, (u[0]*j-v[0]*h)/den)
                det = orient(a, b, z)
                if not all(x > 0 for x in (orient(c,b,z)/det, orient(a,c,z)/det, orient(a,b,c)/det)):
                    continue
            radius = dist(c, a)
            key = c, radius
            out[key] = min(q, out.get(key, q))
    return [(c,r,q,sum(dist(c,p)<r for p in points)) for (c,r),q in out.items()]


def replay(repo, scratch):
    pins = json.loads((HERE/'pins.json').read_text())
    blobs = {}
    for item in pins['git_inputs']:
        raw = subprocess.check_output(['git','show',item['pin']+':'+item['path']],cwd=repo)
        need(hashlib.sha256(raw).hexdigest()==item['sha256'], 'pin differs')
        blobs[item['name']] = raw
    host = json.loads(blobs['session_receipt'])
    manifest = {i['path']:i['sha256'] for i in host['source']['manifest']}
    for item in pins['git_inputs']:
        if item.get('session_source'):
            need(manifest[item['path']]==item['sha256'], 'session source differs')
    for name in ['unit', 'support', 'resolve']:
        item=next(i for i in pins['git_inputs'] if i['name']==name)
        raw=subprocess.check_output(['git','show',pins['delivery_commit']+':'+item['path']],cwd=repo)
        need(raw==blobs[name], 'delivered source changed')
    log = blobs['ctest'].decode()
    gates = {}
    for name in ['witness_d2','witness_memo']:
        rows = [s for s in log.splitlines() if re.search(r'Test #\d+: mhgp12_tower_unit_'+name+r'\s',s)]
        need(len(rows)==1 and 'Passed' in rows[0] and 'Skipped' not in rows[0], 'native gate not passed')
        gates[name] = rows[0].strip()
    unit = blobs['unit'].decode()
    need('MHGP12_TEST(witness_d2, 15)' in unit and 'MHGP12_TEST(witness_memo, 12)' in unit, 'gate bodies changed')
    mutant_base = json.loads(blobs['mutant_base'])
    need(len(mutant_base['mutants'])==7 and not any(m['porte'].endswith(('witness_d2','witness_memo'))
                                                   for m in mutant_base['mutants']), 'base mutant scope differs')
    if scratch:
        for item in pins['external_reviewed']:
            raw=(scratch/item['path']).read_bytes()
            need(hashlib.sha256(raw).hexdigest()==item['sha256'],'external artifact differs')
            data=json.loads(raw)
            cohort=data['mutants']
            need(len(cohort)==item['count'],'external count differs')
            field='porte' if item['kind']=='manifest' else 'juge'
            need(not any(m[field].endswith(('witness_d2','witness_memo')) for m in cohort),'external date gate changed')
            if item['kind']=='report':
                need(data['code']==0 and data['temoin']=='vert' and all(m['verdict']=='TUE' for m in cohort), 'campaign differs')
    points=[(2,10),(18,10),(10,20),(9,3),(11,3)]
    allballs=circles(points)
    catalogue=[row for row in allballs if row[2]+row[3]<=3]
    j=next(r for center,r,_,_ in catalogue if center==(F(10),F(59,5)))
    need(j==F(1681,25), 'ABC level')
    previous=max(r for _,r,_,_ in catalogue if r<j)
    ab=next(row for row in allballs if row[0]==(10,10) and row[1]==64)
    zw=next(row for row in catalogue if row[0]==(10,3) and row[1]==1)
    need(previous==41 and ab[2:]==(2,2) and zw[2:]==(2,0),'D2 premise')
    need(previous < 64 < j and not (64 <= previous), 'D2 invalid predecessor guard')
    initial, terminal, calling = (F((b-a)**2,4) for a,b in [(0,6),(2,4),(0,4)])
    need(not initial < calling and terminal < calling,'memo early read')
    return dict(native_gates=gates, floors=dict(witness_d2=15,witness_memo=12),
                d2=dict(initial='64',terminal='1',junction=str(j),catalogue_predecessor=str(previous),
                        AB_p=2,AB_qmin=2,AB_in_Cat2=False,legitimate_initial_date=True,predecessor_guard=False),
                memo=dict(initial=str(initial),terminal=str(terminal),calling=str(calling),
                          initial_date_admits=False,terminal_date_would_admit=True),
                native_date_mutant_proven=False,native_run_by_auditor=False,
                closure_scope='native_G_D2_MEMO_gates_only; no_future_TMVR_transfer')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--repo-root',type=Path,default=HERE.parents[3])
    p.add_argument('--scratch',type=Path);p.add_argument('--check',action='store_true');args=p.parse_args()
    out=replay(args.repo_root,args.scratch)
    if args.check:
        need(out==json.loads((HERE/'results.json').read_text()),'results differ')
        print('d2_memo_ok: sources et deux portes archivees, dates Fraction ; aucun natif')
    else:print(json.dumps(out,indent=1,sort_keys=True))
