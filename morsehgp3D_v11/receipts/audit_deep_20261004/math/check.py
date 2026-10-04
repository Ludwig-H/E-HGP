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
from hgp11_ref import Definition, Reference, judge
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
            res=A.order(k);other=Reference(X,k).order(k)
            need(not judge.compare_orders(res,other),'adversarial nonregular/source A-B FULL fields agree')
            b=BAdapter(X,res,k+1);own=Own(X,k)
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
                need(res.cover[3].level==25 and min(c.level for c in res.cover)==16,'one first cover can exceed root birth')
                need(max(c.level for c in res.core)>25,'core can exceed all cover and root birth')
            rows.append(dict(name=name,k=k,n=len(X),B_dates=[encoded(v) for v in b.sB],
                             H_entries=[encoded(v) for v in b.e],root_birth=str(res.nodes[-1].level)))
    # Limit fibre: B can add almost d_k/2 without any rival.
    for N in (1,2,5,20):
        X=[(3,3,0),(3+2*N,4,0),(3+2*N,2,0)]
        res=Definition(X).order(2);b=BAdapter(X,res,3)
        t=Q(4*N*N+1,4*N);d=Q(4*N*N+1)
        need(all(rcmp(v,Rad.rat(t))==0 for v in b.e),'no rival: e=t in thin isosceles fibre')
        need(rcmp(b.sB[0],Rad.sqrt(d))==0,'B waits for core in thin isosceles fibre')
        need(rcmp(b.sB[0],Rad.rat(t)+Rad.sqrt(d).scale(Q(1,2)))<=0,'B fibre upper bound')
        if N==20:
            need(rcmp(b.sB[0]-b.e[0],Rad.sqrt(d).scale(Q(49,100)))>0,'B delay close to half core distance, no rival')
        rows.append(dict(name='thin_isosceles_limit',N=N,k=2,n=3,
                         t=str(t),B_x=encoded(b.sB[0]),no_rival=True))
    # Old qualified crossing, independently locate closed owners at r=7.
    X=[(v,0,0) for v in (0,3,6,18,19,20)]
    groups=[]
    for k in (2,3):
        b=BAdapter(X,Definition(X).order(k),k+1);at={}
        for i,e in enumerate(b.e):
            if rcmp(e,Rad.rat(7))<=0:
                at.setdefault(b.alive_at(b.owner[i],Rad.rat(7)),set()).add(i)
        groups.append(list(at.values()))
    need({0,1,2} in groups[0] and {2,3,4,5} in groups[1],'qualified P1 cross-k crossing at same closed r7')
    # The effective-density identity is special to m=k+1, not arbitrary m.
    X=[(v,0,0) for v in (0,2,4,6)]
    res=Definition(X).order(2);b=BAdapter(X,res,4)
    need(all(rcmp(e,Rad.rat(2))==0 for e in b.e),'k2 m4 first qualified at r2 by connected triples')
    meb=Own(X,4).meb(tuple(range(4)))[0]
    need(meb==9,'alpha4 is r3, not qualified k2 m4 r2')
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



def maturity_domain_guard():
    # Abstract radical data compatible with the scalar L6 bounds; no claim
    # that this quadruple is realized by a geometric cloud/its P1 profile.
    primes=(2,3,5,7)
    n=1<<len(primes)
    def product(a,b):
        out=[Q(0)]*n
        for i,x in enumerate(a):
            if not x:continue
            for j,y in enumerate(b):
                if not y:continue
                coeff=1
                for k,p in enumerate(primes):
                    if i&j&(1<<k):coeff*=p
                out[i^j]+=x*y*coeff
        return out
    e=[Q(0)]*n
    e[1],e[2],e[4],e[8]=Q(1,2),Q(-1,2),Q(1,2),Q(1,2)
    columns=[]
    for j in range(n):
        basis=[Q(0)]*n;basis[j]=1;columns.append(product(e,basis))
    matrix=[[columns[j][i] for j in range(n)] for i in range(n)]
    unit=[Q(1)]+[Q(0)]*(n-1)
    inverse=ENV['solve'](matrix,unit)
    need(inverse is not None,'four-root date has inverse in degree16 field')
    need(product(e,inverse)==unit,'four-root inverse is exact')
    need(sum(v!=0 for v in inverse)==8,'maturity inverse can require eight classes')
    need(all(not v or i.bit_count()%2 for i,v in enumerate(inverse)),'inverse has odd square classes')
    cube=product(product(inverse,inverse),inverse)
    need(sum(v!=0 for v in cube)==8,'maturity cube can require eight classes')
    need(product(product(product(e,e),e),cube)==unit,'maturity inverse cube exact')
    H=Rad.sqrt(Q(2))+Rad.sqrt(Q(5))-Rad.sqrt(Q(3))
    d=Rad.sqrt(Q(7));s=(H+d).scale(Q(1,2))
    need(rcmp(d,H)>0 and rcmp(s,H)>0,'maturity chooses four-root branch')
    need(rcmp(H-Rad.sqrt(Q(2)),d.scale(Q(1,2)))<=0,'compatible scalar H delay bound')
    need(rcmp(d,Rad.sqrt(Q(2)).scale(2))<=0,'compatible scalar cover/core lower bound')
    return dict(theta='1/2',H='sqrt(2)+sqrt(5)-sqrt(3)',core='sqrt(7)',
                inverse_classes=8,cube_classes=8,
                scope='abstract radical-domain guard, not a geometric realization or H3 counterexample')



def later_scoring_guard():
    path=ROOT/'later_8f/Zoltan/demos/tools/choisir_bouts.py'
    method=next(n for n in ast.parse(path.read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='outcome')
    env={};exec(compile(ast.Module(body=[method],type_ignores=[]),str(path),'exec'),env)
    # One B-only node nested in root6. Ground-truth A4/B2; mcs2 prohibits
    # the four A singleton nodes. Exact global antichains: {B}, {root}, empty.
    best=[Q(2,3),Q(1)]
    need(env['outcome']([Q(2,5),Q(1)],best)=='win','8f classifies independent best-node threshold as hierarchy win')
    candidates=[(Q(0),Q(1)),(Q(2,3),Q(0)),(Q(0),Q(0))]
    simultaneous=max((a+b)/2 for a,b in candidates)
    need(simultaneous==Q(1,2) and sum(best)/2==Q(5,6),'strict best-node success need not have simultaneous admissible antichain')
    return dict(best_mean='5/6',best_antichain_mean='1/2',mcs=2,
                scope='valid hierarchy diagnostic, not a new scoring bug or native case')



def native_best_blocks_guard():
    path=ROOT/'native_metadata/b00_001470_velos_43_61.json'
    raw=path.read_bytes();d=json.loads(raw);meta=json.loads((ROOT/'NATIVE_METADATA_SOURCE.json').read_text())
    need(hashlib.sha256(raw).hexdigest()==meta['sha256'],'captured native metadata hash')
    need(meta['tar_matches_copy'] and meta['targeted_shutdown_certified'] and meta['observed_status']=='TERMINATED','closed session metadata, no native replay')
    receipt=(ROOT/'native_metadata/receipt.json').read_bytes()
    need(hashlib.sha256(receipt).hexdigest()==meta['receipt_sha256'],'closed receipt hash')
    need(d['status']=='ok' and len(set(d['meta']['objects']))==2,'two distinct declared targets')
    rows=[]
    for k,o in sorted(d['orders'].items(),key=lambda kv:int(kv[0])):
        blocks=o['members']['margin_r'];ss=[set(b) for b in blocks]
        need(len(blocks)==2,'one emitted best block per target')
        for block,subset in zip(blocks,ss):
            need(len(block)==len(subset) and all(type(i) is int and 0<=i<d['sites'] for i in block),'distinct valid shared site IDs')
        a,b=ss;inter=len(a&b)
        pair={'i':0,'j':1,'intersection':inter,'a_subset_b':a<=b,'b_subset_a':b<=a,'disjoint':not inter}
        row={'k':k,'hgp_best':o['margin_r']['best'],'hdb_best':o['hdbscan']['best'],
             'sizes':[len(x) for x in ss],'all_hgp_strictly_half':min(o['margin_r']['best'])>0.5,
             'all_hdb_strictly_half':min(o['hdbscan']['best'])>0.5,'pairs':[pair],
             'best_blocks_antichain':not inter}
        rows.append(row)
    need(rows[2]['k']=='5' and rows[2]['all_hgp_strictly_half'] and not rows[2]['all_hdb_strictly_half'],'actual winning k5')
    need(rows[2]['sizes']==[133,83] and rows[2]['best_blocks_antichain'],'actual returned k5 best blocks disjoint')
    need(rows[3]['pairs'][0]['b_subset_a'] and rows[3]['pairs'][0]['intersection']==54,'actual losing k10 has nested best blocks')
    expected=json.loads((ROOT/'native_best_blocks.json').read_text())
    need(expected['rows']==rows,'metadata-derived pair relations reproducible')
    return dict(name=d['name'],winning_k5_sizes=[133,83],winning_k5_intersection=0,
                losing_k10_intersection=54,source_sha256=meta['sha256'],
                scope='one closed-session emitted JSON; no IoU recomputation, EOM selector or common-cut promise')


def main():
    rows,abstract=point_checks();reciprocal=reciprocal_checks();maturity=maturity_domain_guard();scoring=later_scoring_guard();native=native_best_blocks_guard()
    meta=json.loads((ROOT/'SOURCES.json').read_text())
    for key,folder in [('git_sources','git_source'),('private_before','private_before'),('previous_proofs','previous_proofs')]:
        for obj in meta[key]:need(hashlib.sha256((ROOT/folder/obj['path']).read_bytes()).hexdigest()==obj['sha256'],'snapshot dependency hash')
    late=json.loads((ROOT/'LATER_SOURCES.json').read_text())
    for obj in late['sources']:need(hashlib.sha256((ROOT/'later_8f'/obj['path']).read_bytes()).hexdigest()==obj['sha256'],'later snapshot hash')
    print(json.dumps(dict(pin=meta['pin'],guards=CHECKS,points=rows,abstract_owner_guard=abstract,
                         reciprocals=reciprocal,maturity_domain=maturity,later_scoring=scoring,native_best_blocks=native,result='PASS; no native or statistical qualification'),indent=2,sort_keys=True))


if __name__=='__main__':main()
