#!/usr/bin/env python3
"""Exact independent guard: a local strict tetrahedron has global q_min=3.
No implementation/reference import; rational affine circumcentres and exhaustive subsets.
"""
from fractions import Fraction as F
from itertools import combinations, permutations
import json


def require(ok, message):
    if not ok:
        raise ValueError(message)


def solve(a, b):
    n = len(b)
    rows = [[F(x) for x in a[i]] + [F(b[i])] for i in range(n)]
    for col in range(n):
        pivot = next((i for i in range(col, n) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        d = rows[col][col]
        rows[col] = [x/d for x in rows[col]]
        for i in range(n):
            if i != col:
                t = rows[i][col]
                rows[i] = [x-t*y for x,y in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def dot(a,b): return sum(x*y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def d2(a,b): return dot(sub(a,b), sub(a,b))


def sphere(points):
    if len(points) == 1:
        return tuple(map(F,points[0])), F(0), (F(1),)
    origin = points[0]
    vectors = [sub(p, origin) for p in points[1:]]
    coeff = solve([[dot(a,b) for b in vectors] for a in vectors],
                  [dot(a,a)/F(2) for a in vectors])
    if coeff is None:
        return None
    centre = tuple(F(origin[j])+sum(t*v[j] for t,v in zip(coeff,vectors)) for j in range(3))
    weights = (1-sum(coeff),)+coeff
    return centre, d2(centre, origin), weights


def morton(p):
    return sum(((p[j] >> bit)&1) << (3*bit+j) for bit in range(24) for j in range(3))


def valid_candidates(points, containing):
    out=[]
    for q in range(1,5):
        for ids in combinations(range(len(points)),q):
            s=sphere([points[i] for i in ids])
            if s is not None and all(d2(s[0],p) <= s[1] for p in containing):
                out.append((ids,s))
    return out


records=((5,5,0),(2,1,5),(10,5,5),(2,9,5),(5,9,8))
points=tuple(sorted(records,key=morton))
require(points==records, 'Morton ordering differs')
centre=(F(5),F(5),F(5))
require(all(d2(centre,p)==25 for p in points), 'not common shell')
local_ids=(0,1,2,4)
local=tuple(points[i] for i in local_ids)
mins=valid_candidates(local,local)
beta=min(s[1] for _,s in mins)
winners=[(ids,s) for ids,s in mins if s[1]==beta]
require(beta==25, 'wrong local MEB level')
require(len(winners)==1 and len(winners[0][0])==4, 'local support not strict unique q4')
require(winners[0][1][0]==centre and all(w>0 for w in winners[0][1][2]), 'wrong local weights')
positive=[]
for q in range(1,5):
    for ids in combinations(range(len(points)),q):
        s=sphere([points[i] for i in ids])
        if s is not None and s[0]==centre and s[1]==25 and all(w>0 for w in s[2]):
            positive.append((ids,s[2]))
qmin=min(len(ids) for ids,_ in positive)
canonical=min(ids for ids,_ in positive if len(ids)==qmin)
require(qmin==3 and canonical==(1,2,3), 'global canonical triangle differs')
require(all(0 not in ids for ids,_ in positive if len(ids)==3), 'minimum shell site occurs in q3')
# Every input order must give the same geometric/global identity after Morton normalization.
orders=0
for perm in permutations(records):
    require(tuple(sorted(perm,key=morton))==points, 'order sensitivity')
    orders+=1
# Present unit fixture: MEB(A) classifies, but MEB(I union A) is required for descent.
line=(tuple(map(F,(0,0,0))), tuple(map(F,(4,0,0))), tuple(map(F,(5,0,0))), tuple(map(F,(11,0,0))))
fulltrace_mebs=[]
for a in (line[0],line[3]):
    trace=(a,line[1],line[2])
    cands=valid_candidates(trace,trace)
    r=min(s[1] for _,s in cands)
    require(r > 0 and r < F(121,4), 'trace does not descend strictly')
    fulltrace_mebs.append(str(r))
require(fulltrace_mebs==['25/4','49/4'], 'I+A levels differ')
print(json.dumps({'sites_morton':points,'morton_keys':[morton(p) for p in points],
 'local_part':local_ids,'local_arity':4,'local_weights':[str(w) for w in winners[0][1][2]],
 'centre':[str(x) for x in centre],'beta':'25','global_p':0,'global_m':5,
 'global_qmin':qmin,'global_canonical':canonical,
 'global_positive_q3':[ids for ids,_ in positive if len(ids)==3],
 'local_k':4,'catalogue_K':4,'lookup_local_miss':True,
 'complete_census_bytes':20,'permutations':orders,
 'line_classification_beta':'0','line_full_trace_betas':fulltrace_mebs,
 'scope':'standalone exact math, no native or product/reference import'},sort_keys=True))
