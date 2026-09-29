"""R1 interrupted during cube_max with leaf_size=2; not a completed qualification.
Independent small catalogue audit: exact levels, canonical Morton support, weights, split invariance.
No source under the repository is modified. No timing/scaling qualification.
"""
import hashlib
import itertools
import json
import pathlib
import random
import subprocess
import sys
from fractions import Fraction

BASE=pathlib.Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10')
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(BASE/'reference'))
import hgp10_ref as R

def morton(p):
    return sum(((p[a]>>b)&1)<<(3*b+a) for a in range(3) for b in range(18))

def expected(points,weights,K):
    # Exhaustive presentations, exact Gauss from the separate Python reference.
    balls={}
    for q in (2,3,4):
        for indices in itertools.combinations(range(len(points)),q):
            cc=R.circumcenter([points[i] for i in indices])
            if cc is None: continue
            ctr,lam=cc
            if not all(v>0 for v in lam): continue
            radius=R.d2(ctr,points[indices[0]])
            key=(tuple(ctr),radius)
            support=tuple(sorted((points[i] for i in indices),key=morton))
            if key in balls:
                old=balls[key]
                if (q,tuple(map(morton,support))) < (old['q'],tuple(map(morton,old['sup']))):
                    old['q']=q; old['sup']=support
                continue
            inner=tuple(p for p in points if R.d2(ctr,p)<radius)
            shell=tuple(p for p in points if R.d2(ctr,p)==radius)
            balls[key]={'level':radius,'q':q,'sup':support,'I':frozenset(inner),'U':frozenset(shell),
                        'p':sum(weights[points.index(p)] for p in inner),
                        'u':sum(weights[points.index(p)] for p in shell),
                        'weighted':any(weights[points.index(p)]>1 for p in shell)}
    return {b['sup']:b for b in balls.values() if (b['p'] < K if b['weighted'] else b['p']+b['q']<=K+1)}

def parse(text):
    out=[]
    for line in text.splitlines():
        h,s,i,u=line.split('|')
        rank,q,p,w,flags,num,den=map(int,h.split())
        conv=lambda text: tuple(tuple(map(int,v.split(','))) for v in text.split())
        out.append(dict(rank=rank,q=q,p=p,u=w,flags=flags,level=Fraction(num,den),sup=conv(s),I=frozenset(conv(i)),U=frozenset(conv(u))))
    return out

def need(cond,msg):
    if not cond: raise RuntimeError(msg)

def main():
    rng=random.Random(2026092905)
    cases=[
      ('pair_boundary',[(0,0,0),(262143,262143,262143)],[1,1]),
      ('pair_weighted',[(12,12,12),(14,12,12)],[3,1]),
      ('square',[(0,0,262143),(2,0,262143),(0,2,262143),(2,2,262143)],[1]*4),
      ('octa_weighted',[(10+d if a==0 else 10,10+d if a==1 else 10,10+d if a==2 else 10) for a in range(3) for d in (-2,2)]+[(10,10,10)],[3,1,2,1,4,1,2]),
      ('cube_max',list(itertools.product((0,262143),repeat=3)),[1]*8),
      ('right_triangle',[(0,0,0),(3,0,0),(0,4,0)],[1]*3),
    ]
    for i in range(18):
        domain=(0,1,262141,262142,262143) if i%3==0 else range(9)
        pts=set()
        while len(pts)<9:
            p=tuple(rng.choice(domain) for _ in range(3))
            if i%3==2: p=(p[0],p[1],262143)
            pts.add(p)
        points=sorted(pts)
        weights=[rng.choice((1,1,2,3,5)) for p in points] if i%2 else [1]*len(points)
        cases.append((f'random_{i}',points,weights))
    checks=balls=weighted=extended=0
    for ci,(name,points,weights) in enumerate(cases):
        src=HERE/f'{name}.txt'
        src.write_text(''.join('%d %d %d\n'%p for p,w in zip(points,weights) for _ in range(w)))
        for K in (1,2,3,5,10,12):
            want=expected(points,weights,K)
            ranks={v:i for i,v in enumerate(sorted({b['level'] for b in want.values()}))}
            for leaf,threads in ((0,1),(2,4),(256,1)):
                cmd=[str(HERE/'probe'),str(src),str(K),str(leaf),str(threads)]
                run=subprocess.run(cmd,capture_output=True,text=True,check=False)
                need(run.returncode==0,f'{name}, K={K}, leaf={leaf}: rc={run.returncode} {run.stderr}')
                got=parse(run.stdout)
                need(len(got)==len(want),f'{name}, K={K}: count {len(got)} vs {len(want)}')
                seen=set()
                for b in got:
                    need(b['sup'] not in seen and b['sup'] in want,f'{name}, K={K}: bad/duplicate canonical {b}')
                    seen.add(b['sup']); ref=want[b['sup']]
                    for key in ('q','p','u','level','I','U'):
                        need(b[key]==ref[key],f'{name}, K={K}: {key} got {b[key]} ref {ref[key]}')
                    need(b['rank']==ranks[b['level']],f'{name}: bad dense rank')
                    need(bool(b['flags']&2)==ref['weighted'],f'{name}: weighted flag')
                    need(bool(b['flags']&1)==(len(b['U'])>b['q']),f'{name}: extended flag')
                    weighted+=ref['weighted'];extended+=len(b['U'])>b['q']
                checks+=1;balls+=len(got)
    paths=[HERE/'probe',HERE/'probe.cpp',HERE/'check.py']+list(BASE.glob('src/catalogue/*'))+list(BASE.glob('src/arith/*'))
    receipt={'schema':'mhgp10_catalogue_targeted_audit_v1','status':'completed','cases':len(cases),'calls':checks,
             'ball_records_checked':balls,'weighted_records_checked':weighted,'extended_records_checked':extended,
             'checks':['exact numerical level against Fraction','Morton-minimal canonical support','full I/U/weighted counts',
                       'dense exact ranks','extended/weighted flags','leaf-size 0/2/256; workers 1/4'],
             'hashes':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k!='hashes'}))

if __name__=='__main__': main()
