#!/usr/bin/env python3
"""Adversarial bounded checks of our e02 mathematical assertions.

No native/build/fit/GCP. Own Gram and explicit Gamma are reused via selected
AST definitions from the already closed independent giant check, copied here;
its main and imports are never run. Exact Rad is an AST target arithmetic tool.
Fresh checks attack L6 -> B, nonregular birth with population > k, core dates
past root birth, insertion, and the reciprocal/cube identities with a separate
small multiquadratic-ring implementation. They do not qualify an engine or
prove statistical optimality, nor extrapolate finite H to PPP.
"""
import ast
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import hashlib
import json
import math
import sys
import types

ROOT=Path(__file__).resolve().parent
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'git_source/morsehgp3D_v11/reference'))
from hgp11_ref import Definition
CHECKS=0


def need(ok,label):
    global CHECKS
    CHECKS+=1
    if not ok:raise RuntimeError(label)


def source_primitives():
    path=ROOT/'previous_proofs/giant_math_20261004/check.py'
    wanted={'solve','circumsphere','Own','exact_private_primitives','BAdapter','check_B_meeting'}
    tree=ast.parse(path.read_text());nodes=[n for n in tree.body if getattr(n,'name',None) in wanted]
    # explicit initialization from the copied target arithmetic helper only
    nodes.extend(ast.parse("EXACT=exact_private_primitives()\nRad,rcmp,rmax=(EXACT[k] for k in ('Rad','rcmp','rmax'))").body)
    env=dict(ROOT=ROOT,Q=Q,ast=ast,math=math,types=types,combinations=combinations,need=need)
    exec(compile(ast.Module(body=nodes,type_ignores=[]),str(path),'exec'),env)
    return env


ENV=source_primitives()
Own,BAdapter,Rad,rcmp=(ENV[k] for k in ('Own','BAdapter','Rad','rcmp'))


def encoded(v):
    return str(v)


def point_checks():
    cases=[
        ('cross_U4_birth_k3',[(6,5,5),(4,5,5),(5,6,5),(5,4,5)]),
        ('root_birth_before_core',[(13,14,10),(15,10,10),(13,6,10),(5,10,10)]),
        ('R1_protected_variant',[(25,25,25),(35,25,25),(45,25,25),(55,25,25),(68,25,25),(78,25,25),(5,35,25),(5,15,25)]),
        ('rectangle_boundary',[(6,2,0),(0,0,0),(0,4,0),(12,0,0),(12,4,0)]),
        ('equilateral_3d',[(1,1,2),(1,2,1),(2,2,2),(3,3,2),(4,4,2),(4,3,3)]),
    ]
    rows=[]
    for name,X in cases:
        A=Definition(X)
        orders=[3] if name in ('cross_U4_birth_k3','root_birth_before_core') else [2,3]
        for k in orders:
            res=A.order(k);b=BAdapter(X,res,k+1);own=Own(X,k)
            for i in range(len(X)):
                predicted=b.predicted_B(i)
                need(rcmp(predicted,b.sB[i])==0,'direct B/core scan vs meeting')
                qualified=[(c.level,v) for c in res.cuts for v,cov,_core in c.closed if cov.bit_count()>=k+1 and cov>>i&1]
                t=min(h for h,_ in qualified);p=next(v for h,v in qualified if h==t)
                d=res.core[i].level
                limit=Rad.sqrt(t)+Rad.sqrt(d).scale(Q(1,2))
                bridge=b.level_meet(p,t,res.core[i].nodes,d)
                need(rcmp(Rad.sqrt(bridge),limit)<=0,'L6 primary -> core direct bridge')
                need(rcmp(b.e[i],limit)<=0,'L6 H entry bound')
                need(rcmp(b.sB[i],limit)<=0,'B same upper bound as H')
                D=b.e[i]-Rad.sqrt(t)
                need(rcmp(b.sB[i]-b.e[i],Rad.sqrt(d).scale(Q(1,2))-D)<=0,'B extra-delay remaining slack only')
                need(d<=4*t,'cover radius >= core distance / 2')
            if name=='cross_U4_birth_k3':
                c=(Q(5),Q(5),Q(5));ball=own.critical[c,Q(1)]
                need(ball['q']==2 and len(ball['shell'])==4 and not ball['inner'],'qmin2 full shell4')
                need(all(own.meb(part)[0]==1 for part in combinations(range(4),3)),'no strict trace among triple vertices')
                need(len(res.nodes)==1 and res.nodes[0].level==1 and not res.nodes[0].children,'nonregular population4 birth k3')
                need(res.cuts[-1].closed[0][1].bit_count()==4,'birth population exceeds k')
            if name=='root_birth_before_core':
                need(res.nodes[0].level==16 and len(res.nodes)==1,'FULL root birth 16')
                need(all(c.level==25 for c in res.cover),'first cover can exceed root birth')
                need(max(c.level for c in res.core)>25,'core can exceed all cover and root birth')
            rows.append(dict(name=name,k=k,n=len(X),B_dates=[encoded(v) for v in b.sB],
                             H_entries=[encoded(v) for v in b.e],root_birth=str(res.nodes[-1].level)))
    # Old insertion obstruction recaptured at integer coordinates without int(Fraction) coercion.
    X=[(v,0,0) for v in (0,2000,4000)]
    Y=[(v,0,0) for v in (0,1,2000,4000)]
    bx=BAdapter(X,Definition(X).order(2),3);by=BAdapter(Y,Definition(Y).order(2),3)
    need(rcmp(bx.e[0],Rad.rat(2000))==0,'old qualified entry 2000')
    need(rcmp(by.e[0],Rad.rat(1000))==0,'insertion qualified entry 1000')
    need(rcmp(bx.sB[0],Rad.rat(2000))==0 and rcmp(by.sB[0],Rad.rat(1000))==0,'B also not insertion stable')
    # Scalar stability alone does not transport owners, hence does not prove B3.
    abstract={'FULL_births':[1,1,100],'HX_entry':1,'HY_entry':1,'core_entry':1,
              'HX_owner':'A','HY_owner':'B','core_owner':'A','BX':1,'BY':100}
    need(abstract['HX_entry']==abstract['HY_entry'] and abstract['BX']!=abstract['BY'],'strong alignment indispensable, not scalar H dates alone')
    return rows,abstract


# Separate exact multiquadratic arithmetic. Only rational-square ratio tests;
# no factorization and no private Rad implementation in reciprocal checks.
def normalize(terms):
    classes=[]
    for c,a in terms:
        c,a=Q(c),Q(a)
        if not c or not a:continue
        for j,(coef,rep) in enumerate(classes):
            ratio=a/rep;n,d=ratio.numerator,ratio.denominator;sn,sd=math.isqrt(n),math.isqrt(d)
            if sn*sn==n and sd*sd==d:
                classes[j]=(coef+c*Q(sn,sd),rep);break
        else:classes.append((c,a))
    return [(c,a) for c,a in classes if c]


def mul(a,b):
    return normalize([(c*d,x*y) for c,x in a for d,y in b])


def equal(a,b):
    return not normalize(a+[(-c,x) for c,x in b])


def positive(t,M,q):
    if q<t+M:return True
    if q==t+M:return t*M>0
    return (q-t-M)**2<4*t*M


def inv(e):
    t,M,q=e;d=t+M-q;delta=d*d-4*t*M
    if delta:
        return normalize([((t-M-q)/delta,t),((M-t-q)/delta,M),(d/delta,q),(-2/delta,t*M*q)])
    # positivity eliminates the root-sum branch and requires min > 0
    small=min(t,M)
    need(small>0,'singular positive date has min positive')
    return [(Q(1,2)/small,small)]


def reciprocal_checks():
    candidates=[(Q(t,7),Q(M,11),Q(q,13)) for t in range(5) for M in range(5) for q in range(7)]
    candidates +=[(Q(a*a,49),Q(b*b,49),Q((a-b)**2,49)) for a in range(1,5) for b in range(1,5)]
    candidates +=[(Q(2),Q(162),Q(50)),(Q(8),Q(98),Q(32)),(Q(1,4),Q(14000000**2),Q(13999999**2))]
    tested=singular=zero_rejected=0
    for t,M,q in candidates:
        if not positive(t,M,q):
            zero_rejected+=1;continue
        e=normalize([(1,t),(1,M),(-1,q)]);rec=inv((t,M,q));cube=mul(mul(rec,rec),rec)
        need(equal(mul(e,rec),[(1,1)]),'reciprocal identity incl coincident square classes')
        need(equal(mul(mul(mul(e,e),e),cube),[(1,1)]),'cube identity for z3')
        need(len(rec)<=4 and len(cube)<=4,'at most four square classes per inverse/cube')
        singular+=(t+M-q)**2==4*t*M;tested+=1
    # Certified equality of dates must yield equal reciprocal and cube values.
    a=inv((Q(2),Q(162),Q(50)));b=inv((Q(8),Q(98),Q(32)))
    need(equal(a,b),'equal radical dates -> equal inverse')
    need(equal(mul(mul(a,a),a),mul(mul(b,b),b)),'equal radical dates -> equal cube')
    return dict(tested=tested,singular=singular,nonpositive_skipped=zero_rejected)


def main():
    rows,abstract=point_checks();reciprocal=reciprocal_checks()
    meta=json.loads((ROOT/'SOURCES.json').read_text())
    for key,folder in [('git_sources','git_source'),('private_before','private_before'),('previous_proofs','previous_proofs')]:
        for obj in meta[key]:need(hashlib.sha256((ROOT/folder/obj['path']).read_bytes()).hexdigest()==obj['sha256'],'snapshot dependency hash')
    print(json.dumps(dict(pin=meta['pin'],guards=CHECKS,points=rows,abstract_owner_guard=abstract,
                         reciprocals=reciprocal,result='PASS; no native or statistical qualification'),indent=2,sort_keys=True))


if __name__=='__main__':main()
