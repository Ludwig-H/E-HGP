#!/usr/bin/env python3
"""Five-site exact Gamma2 / actual Pi2c / weighted-MMt countercheck. No engine."""
import ast
import hashlib
import itertools
import json
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).absolute().parent
PINS = {
    'pipeline_snapshot.py': '8fb7eba774b2547bb34aafecd1405964d1d4a56687f308562a435f258be01734',
    'mmt_pond_snapshot.py': 'abbeac86b171e851a4e782a5d573496e2e4a9e118ceddf4c820db21a6c66b2af',
    'qsqrt_snapshot.py': 'a1d1ed969fa5afc7b9a7e1746e07d770da45c5f80e9c2c80e034ad358394984d',
}

def require(c, message):
    if not c:
        raise RuntimeError(message)

def snapshots():
    out = {}
    for name, wanted in PINS.items():
        data = (ROOT / name).read_bytes()
        require(hashlib.sha256(data).hexdigest() == wanted, 'snapshot pin ' + name)
        out[name] = data
    return out

def ast_items(source, wanted):
    nodes = [n for n in ast.parse(source).body if isinstance(n, (ast.ClassDef, ast.FunctionDef)) and n.name in wanted]
    require({n.name for n in nodes} == set(wanted), 'AST inventory')
    return ast.Module(body=nodes, type_ignores=[])

def load_actual(src):
    qg = {'__name__': 'qsqrt_snapshot'}
    exec(compile(src['qsqrt_snapshot.py'], 'qsqrt_snapshot.py', 'exec'), qg)
    QS = qg['QS']
    # Only the two operations used by mmt_point_pond. Neither Scene nor native imports.
    class RayonAdapter:
        def __init__(self, b2=None, qs=None):
            self.qs = QS.sqrt(b2) if b2 is not None else qs
        @staticmethod
        def somme(q):
            return RayonAdapter(qs=q)
        def cmp(self, other):
            return self.qs.cmp(other.qs)
    pg = {'Fraction': F, 'exiger': require}
    exec(compile(ast_items(src['pipeline_snapshot.py'], ['Admissibilite']), 'pipeline_snapshot.py', 'exec'), pg)
    mg = {'Fraction': F, 'exiger': require, 'QS': QS, 'Rayon': RayonAdapter}
    exec(compile(ast_items(src['mmt_pond_snapshot.py'], ['_anc', 'mmt_point_pond']), 'mmt_pond_snapshot.py', 'exec'), mg)
    return pg['Admissibilite'], mg['mmt_point_pond'], QS

def d2(a, b):
    return sum((x-y)**2 for x,y in zip(a,b))

def meb(points):
    # Exhaust all supports of two or three sites, check containment. No heuristic.
    balls = []
    for a,b in itertools.combinations(points,2):
        c = tuple((u+v)/2 for u,v in zip(a,b)); r = d2(a,b)/4
        if all(d2(c,z) <= r for z in points):
            balls.append(r)
    if len(points) == 3:
        a,b,c = points
        u = tuple(z-w for z,w in zip(b,a)); v = tuple(z-w for z,w in zip(c,a))
        G = d2(u,(0,0)); H = sum(x*y for x,y in zip(u,v)); J = d2(v,(0,0)); D = G*J-H*H
        if D:
            s = (G*J-H*J)/(2*D); t = (G*J-H*G)/(2*D)
            center = tuple(a[i]+s*u[i]+t*v[i] for i in range(2))
            r = d2(center,a)
            require(all(d2(center,z) == r for z in points), 'circumcenter')
            balls.append(r)
    require(balls, 'no MEB support')
    return min(balls)

NAMES = ('x','a','b','z','y')

def geometry(e):
    return ((F(0),F(0)), (F(6),F(0)), (F(0),F(8)), (F(-1),F(8)), (F(6),F(8)-e))

def gamma_tree(P, mutation=None):
    pairs = list(itertools.combinations(range(5),2))
    triples = list(itertools.combinations(range(5),3))
    beta = {f:meb([P[i] for i in f]) for f in pairs+triples}
    if mutation == 'premature_xby':
        beta[(0,2,4)] = beta[(0,1,4)]
    elif mutation == 'omit_xay':
        triples.remove((0,1,4))
    birth=[]; parent=[]; children=[]; cover=[{} for _ in P]; active={}; cuts=[]
    for q in sorted(set(beta.values())):
        live=[p for p in pairs if beta[p] <= q]; union={p:p for p in live}
        def find(p):
            while union[p] != p:
                p=union[p]
            return p
        for t in triples:
            if beta[t] <= q:
                edges=list(itertools.combinations(t,2)); root=find(edges[0])
                for p in edges[1:]:
                    union[find(p)] = root
        comps={}
        for p in live:
            comps.setdefault(find(p),set()).add(p)
        nxt={}
        for C in sorted(comps.values(), key=lambda s:tuple(sorted(s))):
            old=sorted({v for oldC,v in active.items() if set(oldC) <= C})
            if len(old) == 1:
                v=old[0]  # continuation, retain delayed point activations
            else:
                v=len(birth); birth.append(q); parent.append(-1); children.append(old)
                for u in old:
                    parent[u]=v
            for i in set(itertools.chain.from_iterable(C)):
                cover[i].setdefault(v,q)
            nxt[frozenset(C)] = v
        active=nxt
        cuts.append((q, [sorted(C) for C in comps.values()]))
    death=[None if p<0 else birth[p] for p in parent]
    T=SimpleNamespace(birth=birth,parent=parent,children=children,death=death)
    require(sum(p<0 for p in parent)==1, 'one root')
    require(all(birth[v] <= c and (death[v] is None or c < death[v]) for cv in cover for v,c in cv.items()), 'half-open cover')
    for cv in cover:
        require(all(p<0 or p in cv for v in cv for p in [parent[v]]), 'hereditary cover')
    return T,cover,beta,cuts

def ancestor(T,v,s):
    while T.parent[v] >= 0 and T.birth[T.parent[v]] <= s:
        v=T.parent[v]
    return v

def lca(T,u,v):
    seen=set()
    while u>=0:
        seen.add(u); u=T.parent[u]
    while v not in seen:
        v=T.parent[v]
    return v

def text(q):
    return q.text()

def one(e, actual, mutation=None):
    Ad,MMt,QS=actual
    T,cover,beta,cuts=gamma_tree(geometry(e),mutation)
    ad=Ad(T,cover,3,mode='continu')
    omega={v:ad.omega(v,cover[0]) for v in cover[0]}
    xa=next(v for v,c in cover[0].items() if c==9)
    t1=F(25)-4*e+e*e/4
    t2=F(16)+(F(3)-2*e/3+e*e/12)**2
    require(beta[(0,1,4)]==t1 and beta[(0,2,4)]==t2, 'MEB t1/t2')
    expected=(t2-t1)/(t2-9) if e else F(0)
    require(omega[xa]==expected,'actual Pi2c omega')
    positive=[cover[0][v] for v,w in omega.items() if w>0]
    require(min(positive)==(9 if e else 16), 'actual positive anchor')
    require(T.death[xa]==(t1 if e else 25), 'no earlier fusion of xa')
    eta=F(2,3); kappa=F(4)
    rx=MMt(T,ad.ponderer_bande(cover[0],eta),eta,kappa)
    ra=MMt(T,ad.ponderer_bande(cover[1],eta),eta,kappa)
    expected_half=F(12) if e else F(6145,288)
    require(rx['T_half']==expected_half, 'real weighted-MMt T_half')
    require(rx['date'].cmp(QS.sqrt(expected_half))==0, 'real weighted-MMt date')
    require(ra['date'].cmp(QS.sqrt(F(12)))==0,'a date')
    hxa=lca(T,rx['owner'],ra['owner'])
    height=rx['date']
    for candidate in (ra['date'], QS.sqrt(T.birth[hxa])):
        if candidate.cmp(height)>0:
            height=candidate
    require(all(height.cmp(q)>=0 for q in (rx['date'],ra['date'],QS.sqrt(T.birth[hxa]))),'height exact max')
    require(height.cmp(QS.sqrt(F(12)) if e else QS.rat(5))==0,'real reunion x,a')
    if e:
        require(rx['N']==1 and rx['A']==9 and rx['W']==6*expected,'single early mass')
    else:
        require(rx['A']==16 and rx['W']==F(1535,144),'limit masses')
    control=Ad(T,cover,2,mode='continu')
    require(all(control.omega(v,cv)==1 for cv in cover for v in cv),'mcs<=K unit weights')
    rc=MMt(T,control.ponderer_bande(cover[0],eta),eta,kappa)
    require(rc['A']==9 and rc['date'].cmp(QS.sqrt(F(12)))==0,'mcs2 control anchor/date')
    return {
        'epsilon':str(e), 'omega_xa':str(omega[xa]), 'A':str(rx['A']),
        't1':str(t1), 't2':str(t2), 'W':str(rx['W']), 'T_half':str(rx['T_half']),
        'date_x':text(rx['date']), 'date_x_approx':float(rx['date']),
        'date_a':text(ra['date']), 'height_xa':text(height), 'height_xa_approx':float(height),
        'argmax_x':[str(v) for v in rx['argmax']],
        'triple_MEBs':[[','.join(NAMES[i] for i in t),str(beta[t])] for t in itertools.combinations(range(5),3)],
        'point_x_profile':[[v,str(cover[0][v]),None if T.death[v] is None else str(T.death[v]),str(omega[v])] for v in cover[0]],
        'tree':{'birth':list(map(str,T.birth)), 'parent':T.parent, 'children':T.children},
        'nodes':len(T.birth), 'cut_count':len(cuts), 'mcs2_control':'all omega=1, A=9, date_x=sqrt(12)',
    }

def run():
    src=snapshots(); actual=load_actual(src)
    cases=[one(e,actual) for e in (F(0),F(1,8),F(1,32),F(1,128),F(1,1024),F(1,4096))]
    killed=[]
    for name in ('premature_xby','omit_xay'):
        try:
            one(F(1,8),actual,name)
        except RuntimeError as exc:
            killed.append({'mutant':name,'diagnostic':str(exc)})
        else:
            raise RuntimeError('causal mutant survived '+name)
    require(len(cases)==6 and len(killed)==2,'nonempty inventory')
    require(snapshots()==src,'snapshots changed during replay')
    return {'scope':'Fraction Gamma2 exhaustive, actual AST Pi2c and actual AST mmt_point_pond; no native Scene/pipeline call',
            'K':2,'mcs':3,'eta':'2/3','kappa':'4','cases':cases,'mutants':killed,
            'snapshot_pins_before':PINS,'snapshot_pins_after':PINS,'native_calls':0,'GCP':'unused'}

if __name__=='__main__':
    print(json.dumps(run(),sort_keys=True,indent=1))
