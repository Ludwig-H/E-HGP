#!/usr/bin/env python3
"""Bounded stdlib model only: no C++/product import, no native execution, no timing claim."""
from fractions import Fraction as Q
from itertools import combinations, product
from pathlib import Path
import hashlib
import json

checks = 0

def check(value, message):
    global checks
    checks += 1
    if not value:
        raise ValueError(message)

def sub(a, b): return tuple(x-y for x,y in zip(a,b))
def dot(a, b): return sum(x*y for x,y in zip(a,b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def sq(a): return dot(a,a)

def solve(a, b):
    rows = [[Q(x) for x in row]+[Q(y)] for row,y in zip(a,b)]
    for j in range(len(rows)):
        pivot = next((i for i in range(j,len(rows)) if rows[i][j]),None)
        if pivot is None: return None
        rows[j],rows[pivot] = rows[pivot],rows[j]
        scale = rows[j][j]
        rows[j] = [x/scale for x in rows[j]]
        for i in range(len(rows)):
            if i != j:
                value=rows[i][j]
                rows[i]=[x-value*y for x,y in zip(rows[i],rows[j])]
    return tuple(row[-1] for row in rows)

def gram_sphere(points, ids):
    a=points[ids[0]]
    if len(ids)==1: return tuple(map(Q,a)), Q(0), (Q(1),)
    v=[sub(points[i],a) for i in ids[1:]]
    alpha=solve([[dot(u,w) for w in v] for u in v],[Q(sq(u),2) for u in v])
    if alpha is None: return None
    off=tuple(sum(t*u[j] for t,u in zip(alpha,v)) for j in range(3))
    return tuple(Q(a[j])+off[j] for j in range(3)),sq(off),(1-sum(alpha),)+alpha

def form(points, ids):
    a=points[ids[0]];q=len(ids)
    if q==1: n=(0,0,0);d=1;raw=(0,1)
    else:
        u=sub(points[ids[1]],a);uu=sq(u)
        if q==2: n=u;d=2;raw=(uu,4)
        else:
            v=sub(points[ids[2]],a);vv=sq(v);w=cross(u,v)
            if q==3:
                g=sq(w)
                if not g:return None
                t=tuple(uu*v[j]-vv*u[j] for j in range(3))
                n=cross(t,w);d=2*g
                raw=(uu*vv*sq(sub(points[ids[2]],points[ids[1]])),4*g)
            else:
                s=sub(points[ids[3]],a);ss=sq(s)
                vs,su,uv=cross(v,s),cross(s,u),cross(u,v)
                det=dot(u,vs)
                if not det:return None
                n=tuple(uu*vs[j]+vv*su[j]+ss*uv[j] for j in range(3));d=2*det
                if d<0:n=tuple(-x for x in n);d=-d
                raw=(sq(n),d*d)
    c=tuple(Q(a[j])+Q(n[j],d) for j in range(3));r2=Q(*raw)
    oracle=gram_sphere(points,ids)
    check(oracle is not None and (c,r2)==oracle[:2],'form differs from independent Gram sphere')
    check(all(sq(sub(points[i],c))==r2 for i in ids),'support not on shell')
    return {'support':ids,'N':n,'D':d,'raw_level':raw,'center':c,'radius':r2,'weights':oracle[2],'anchor':a}

def power(ball,p):
    v=sub(p,ball['anchor'])
    value=ball['D']*sq(v)-2*dot(ball['N'],v)
    check(Q(value,ball['D'])==sq(sub(p,ball['center']))-ball['radius'],'power differs from independent distance')
    return value

def bounded(points,lazy):
    n=len(points);ledger={k:0 for k in ['presentations','nondegenerate','positive','containing','comparisons','point_tests','diameter_pairs']}
    materializations={'q1':0,'q2':0,'q3':0,'q4':0};rejected_q3=[]
    first=(0,) if n==1 else max(combinations(range(n),2),key=lambda t:sq(sub(points[t[1]],points[t[0]])))
    ledger['diameter_pairs']=0 if n==1 else n*(n-1)//2
    tuples=[first]+[t for q in (3,4) for t in combinations(range(n),q)]
    for ids in tuples:
        q=len(ids);ledger['presentations']+=1
        if q==3:
            a,b,c=(points[i]for i in ids)
            if not sq(cross(sub(b,a),sub(c,a))):continue
            ledger['nondegenerate']+=1
            if min(dot(sub(b,a),sub(c,a)),dot(sub(a,b),sub(c,b)),dot(sub(a,c),sub(b,c)))<=0:continue
        ball=form(points,ids)
        if ball is None:continue
        if q!=3:ledger['nondegenerate']+=1
        if q==4 and min(ball['weights'])<=0:continue
        ledger['positive']+=1
        if q<3 or (q==3 and not lazy):materializations['q'+str(q)]+=1
        outside=None
        for i,p in enumerate(points):
            ledger['point_tests']+=1
            if power(ball,p)>0:outside=i;break
        if outside is not None:
            if q==3:rejected_q3.append({'support':ids,'raw_level':ball['raw_level'],'outside':outside,'power':power(ball,points[outside])})
            continue
        if q==4 or (q==3 and lazy):materializations['q'+str(q)]+=1
        ledger['containing']+=1
        return {'support':ids,'N':ball['N'],'D':ball['D'],'raw_level':ball['raw_level'],'ledger':ledger,'materializations':materializations,'rejected_q3':rejected_q3}
    raise ValueError('no containing MEB')

def canonical_oracle(points):
    choices=[]
    for q in range(1,min(4,len(points))+1):
        for ids in combinations(range(len(points)),q):
            sphere=gram_sphere(points,ids)
            if sphere is None:continue
            c,r2,weights=sphere
            if min(weights)<0 or any(sq(sub(p,c))>r2 for p in points):continue
            choices.append((r2,q,ids))
    return min(choices)

def compare(points):
    eager,lazy=bounded(points,False),bounded(points,True)
    for field in ['support','N','D','raw_level','ledger','rejected_q3']:
        check(eager[field]==lazy[field],'changed '+field)
    r2,q,ids=canonical_oracle(points)
    check((Q(*eager['raw_level']),len(eager['support']),eager['support'])==(r2,q,ids),'canonical MEB mismatch')
    check(lazy['materializations']['q3']<=eager['materializations']['q3'],'extra q3 materialization')
    return eager,lazy

def main():
    base=Path(__file__).resolve().parent
    manifest=json.loads((base/'SOURCE_BEFORE.json').read_text())
    for row in manifest['files']:
        data=(base/'source'/row['path']).read_bytes()
        check(len(data)==row['bytes'],'source size')
        check(hashlib.sha256(data).hexdigest()==row['sha256'],'source hash')
    tetra=((0,0,0),(2,2,0),(2,0,2),(0,2,2))
    eager,lazy=compare(tetra)
    check(eager['materializations']['q3']==4 and lazy['materializations']['q3']==0,'non-vacuous reject benefit')
    check(eager['support']==(0,1,2,3),'tetra canonical q4')
    check(eager['ledger']['presentations']==6 and eager['ledger']['point_tests']==17,'tetra exact model work')
    check(eager['rejected_q3'][0]=={'support':(0,1,2),'raw_level':(512,192),'outside':3,'power':256},'exact raw witness')
    check(eager['raw_level']==(3072,1024),'accepted raw q4 level')
    witness=form(tetra,(0,1,2))
    check(witness['N']==(128,64,64) and witness['D']==96,'q3 raw centre')
    cube=sorted(product((0,1),repeat=3),key=lambda p:p[0]+2*p[1]+4*p[2])
    cases=[tuple(cube[i] for i in t)for t in combinations(range(8),4)]
    cases += [((0,0,0),),((0,0,0),(2,0,0)),((0,0,0),(2,0,0),(1,0,0)),((0,0,0),(2,0,0),(0,2,0)),((0,0,0),(4,0,0),(2,3,0)),((0,0,0),(2,0,0),(2,2,0),(0,2,0))]
    for points in cases:compare(points)
    profile=[]
    for bits in (18,21,24):
        scale=(1<<(bits-1))-1
        pts=tuple(tuple(1+scale*x for x in p)for p in tetra)
        compare(pts)
        q3=form(pts,(0,1,2));value=power(q3,pts[3])
        check(q3['raw_level']==(512*scale**6,192*scale**4),'degree six/four, raw preserved')
        check(q3['N']==(128*scale**5,64*scale**5,64*scale**5) and q3['D']==96*scale**4,'raw centre degree five/four')
        check(value==256*scale**6,'power degree six')
        check(value.bit_length()<=6*bits+8,'SideInt budget')
        cert=q3['D']<(1<<(123-2*bits)) and all(abs(n)<(1<<(124-bits))for n in q3['N'])
        native=(6*bits+8<=127 or cert)
        if bits>18:check(not native and value>=(1<<127),'q3-to-q4 retag would lose required fallback')
        profile.append({'bits':bits,'scale':scale,'q3_power_bits':value.bit_length(),'side_budget_bits':6*bits+8,'global_q3_i128_certificate':cert,'native_power_by_current_guard':native})
    summary={'scope':'stdlib exact model; no C++ execution or chrono','source_pin':manifest['pin'],'checks':checks,'comparison_cases':1+len(cases)+len(profile),'witness_points':tetra,'eager':eager,'lazy_q3':lazy,'profile_extreme_guards':profile,'same_candidate_scope':'One private q3 candidate shared with catalogue; preserve q3 tag/certificates and non-reduced degree-six Level.'}
    print(json.dumps(summary,sort_keys=True,separators=(',',':')))

if __name__=='__main__':main()
