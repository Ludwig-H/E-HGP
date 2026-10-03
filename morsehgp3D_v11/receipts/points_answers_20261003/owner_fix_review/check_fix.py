#!/usr/bin/env python3
"""Actual frozen oracle AST versus independent rational radical intervals; no native/fit/GCP."""
import ast
import decimal
from fractions import Fraction as F
import hashlib
from itertools import product
import json
from math import isqrt
from pathlib import Path
import random
import sys

BASE=Path(__file__).resolve().parent
sys.dont_write_bytecode=True
sys.path.insert(0,str(BASE/'reference'))
from hgp11_ref import Definition  # noqa: E402
checks=0


def need(ok,label):
    global checks
    checks+=1
    if not ok:raise RuntimeError(label)


source=BASE/'source/points_reference.py'
pin=next(e['sha256'] for e in json.loads((BASE/'before.json').read_text())['entries'] if e['snapshot']=='source/points_reference.py')
need(hashlib.sha256(source.read_bytes()).hexdigest()==pin,'oracle SHA pin')
names={'parents_of','popcount','Tree','two_roots_sign','reference_radius_rules','reference_owner_signature'}
tree=ast.parse(source.read_bytes(),filename=str(source))
selected=[n for n in tree.body if isinstance(n,(ast.FunctionDef,ast.ClassDef)) and n.name in names]
need({n.name for n in selected}==names,'exact AST requested definitions')
pr={'Fraction':F}
exec(compile(ast.fix_missing_locations(ast.Module(body=selected,type_ignores=[])),str(source),'exec'),pr)


def radical_sign(terms):
    """Independent algorithm: rational square classes, then rigorous isqrt intervals.

    No import/call of either two_roots_sign or the bench comparator.
    """
    groups=[]
    for q,c in terms:
        q,c=F(q),F(c)
        if not q or not c:continue
        need(q>0,'nonnegative radicands')
        for g in groups:
            ratio=q/g[0];a,b=isqrt(ratio.numerator),isqrt(ratio.denominator)
            if a*a==ratio.numerator and b*b==ratio.denominator:
                g[1]+=c*F(a,b);break
        else:groups.append([q,c])
    groups=[(q,c) for q,c in groups if c]
    if not groups:return 0
    for bits in [192,384,768,1536,3072]:
        lo=hi=F(0);unit=1<<bits
        for q,c in groups:
            a=isqrt((q.numerator<<(2*bits))//q.denominator)
            left=F(a,unit);right=left if a*a*q.denominator==q.numerator<<(2*bits) else F(a+1,unit)
            lo+=c*(left if c>0 else right);hi+=c*(right if c>0 else left)
        if lo>0:return 1
        if hi<0:return -1
    raise RuntimeError('independent comparison refusal')


def two_roots_expected(a,b,c,d):
    return radical_sign([(a,1),(b,1),(c,-1),(d,-1)])


comparison_count=equalities=0
for raw in product(range(8),repeat=4):
    args=tuple(F(x) for x in raw)
    got=pr['two_roots_sign'](*args);expected=two_roots_expected(*args)
    need(got==expected,'two_roots integer exhaustive oracle')
    comparison_count+=1;equalities+=expected==0

rng=random.Random(20261003)
for _ in range(250):
    args=tuple(F(rng.randrange(100),rng.randrange(1,20)) for _ in range(4))
    need(pr['two_roots_sign'](*args)==two_roots_expected(*args),'two_roots rational oracle')
    comparison_count+=1
near=[]
for shift in [-1,0,1]:
    target=F(8)+F(shift,1<<180)
    args=(F(2),F(18),F(8),target)
    got=pr['two_roots_sign'](*args);expected=two_roots_expected(*args)
    need(got==expected==-shift,'exact plateau versus strict adjacent scalar')
    near.append({'target':str(target),'sign':got,'max_fraction_bits':max(max(a.numerator.bit_length(),a.denominator.bit_length()) for a in args)})
    comparison_count+=1


def independent_rules(res,n,m):
    parent=[-1]*len(res.nodes)
    for v,node in enumerate(res.nodes):
        for c in node.children:parent[c]=v
    def lca(a,b):
        seen=set()
        while a>=0:seen.add(a);a=parent[a]
        while b not in seen:b=parent[b]
        return b
    profiles=[[] for _ in range(n)]
    for cut in res.cuts:
        for v,coverage,_ in cut.closed:
            if coverage.bit_count()>=m:
                for i in range(n):
                    if coverage>>i&1:profiles[i].append((cut.level,v))
    out=[]
    for profile in profiles:
        t=min(c for c,v in profile);start=next(v for c,v in profile if c==t)
        arg=(F(0),F(0))
        for q,v in profile:
            a=lca(start,v)
            meet=max(t,q) if a in [start,v] else max(t,q,res.nodes[a].level)
            if radical_sign([(meet,1),(arg[1],1),(q,-1),(arg[0],-1)])>0:arg=(meet,q)
        owner=start
        while parent[owner]>=0 and radical_sign([(t,1),(arg[0],1),(arg[1],-1),(res.nodes[parent[owner]].level,-1)])>=0:
            owner=parent[owner]
        out.append((t,arg[0],arg[1],owner))
    return out


rows=[]
base=[(0,0,0),(2,2,0),(-4,-4,0),(4,4,0)]
for shift,scale,reverse in [(0,1,False),(4,1,False),(4,1,True),(10,3,False)]:
    permutation=list(reversed(range(4))) if reverse else list(range(4))
    points=[tuple(scale*c+shift for c in base[i]) for i in permutation]
    res=Definition(points).order(2)
    for m in [1,3,4]:
        expected=independent_rules(res,4,m)
        oracles=[]
        for precision in [8,120,200]:
            got,_=pr['reference_radius_rules'](res,4,m,precision=precision)
            for i,(e,owner,triple) in enumerate(got['margin_r']):
                ti,mi,qi,oi=expected[i]
                need(owner==oi,'exact owner independent of Decimal precision')
                need(radical_sign([(triple[0],1),(triple[1],1),(triple[2],-1),(ti,-1),(mi,-1),(qi,1)])==0,'exact maximal rival date')
                # The winner chosen by the real oracle must dominate every competing profile point.
            oracles.append({'precision':precision,'owners':[o for _,o,_ in got['margin_r']]})
        if m==1:
            site=permutation.index(0)
            t,meet,q,owner=expected[site]
            need((t,meet,q)==(F(2*scale*scale),F(18*scale*scale),F(8*scale*scale)),'plateau selected triple')
            signature=pr['reference_owner_signature'](res,4,owner)
            native_labels=sorted(permutation[i] for i in signature[1])
            need(signature[0]==8*scale*scale and native_labels==[0,1,3],'closed owner parent signature')
            need(two_roots_expected(t,meet,q,F(8*scale*scale))==0,'symbolic date equals parent birth')
        rows.append({'shift':shift,'scale':scale,'reversed':reverse,'m':m,'oracles':oracles,
                     'exact_triplets_and_owners':[[str(t),str(meet),str(q),o] for t,meet,q,o in expected]})

print(json.dumps({'status':'PASS','checks':checks,'oracle_sha256':pin,'two_root_comparisons':comparison_count,
                 'integer_equalities':equalities,'near_plateau_192bit_scalars':near,'closed_plateau_profiles':rows,
                 'scope':'actual frozen Python oracle AST versus independent Fraction/radical classes+isqrt; no native/build/fit/GCP',
                 'conclusion':'The selected rival and closed owner are now exact and independent of Decimal precision; prior raw m1 counterexample is closed.'},indent=2,sort_keys=True))
