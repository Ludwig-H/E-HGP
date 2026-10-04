#!/usr/bin/env python3
"""Bounded, scalar S6/reference-enumerator contract checks; no native execution."""
import ast
from fractions import Fraction as F
from itertools import combinations, product
from math import comb
from pathlib import Path
import hashlib
import json

ROOT=Path(__file__).resolve().parent
CHECKS=0

def need(value, message):
    global CHECKS
    CHECKS+=1
    if not value:
        raise RuntimeError(message)

# Reuse just the closed independent barycentric oracle. No import or execution
# of developer modules, C++ source, native code or the old test main.
p=ROOT/'references/initial_qb.py.source'
need(hashlib.sha256(p.read_bytes()).hexdigest()=='5e191f548760783d28b9f5cda602355bab1717926fe468e9e62f0af8e9705e8c','barycentric source pin')
tree=ast.parse(p.read_text())
selected=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='barycentric']
need(len(selected)==1,'unique barycentric AST')
namespace={'F':F}
exec(compile(ast.Module(body=selected,type_ignores=[]),'pinned_barycentric','exec'),namespace)
barycentric=namespace['barycentric']

def sub(a,b):return tuple(F(x)-F(y) for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def sign(x):return (x>0)-(x<0)
def det(a,b,c,d):return dot(cross(sub(b,a),sub(c,a)),sub(d,a))
def acute(a,b,c):return dot(sub(b,a),sub(c,a))>0 and dot(sub(a,b),sub(c,b))>0 and dot(sub(a,c),sub(b,c))>0

def native_shaped_supports(shell,centre):
    found=[]
    for i,j in combinations(range(len(shell)),2):
        if all(F(shell[i][d])+F(shell[j][d])==2*centre[d] for d in range(3)):
            found.append((1<<i)|(1<<j))
    for i,j,k in combinations(range(len(shell)),3):
        a,b,c=shell[i],shell[j],shell[k]
        if acute(a,b,c) and det(a,b,c,centre)==0:
            found.append((1<<i)|(1<<j)|(1<<k))
    for indices in combinations(range(len(shell)),4):
        points=tuple(shell[i] for i in indices)
        inside=True
        for opposite in range(4):
            face=tuple(points[i] for i in range(4) if i!=opposite)
            vertex=sign(det(*face,points[opposite]))
            if vertex==0 or sign(det(*face,centre))!=vertex:
                inside=False;break
        if inside:found.append(sum(1<<i for i in indices))
    return found

def independent_supports(shell,centre):
    found=[]
    for q in (2,3,4):
        for indices in combinations(range(len(shell)),q):
            weights=barycentric(tuple(shell[i] for i in indices),centre)
            if weights is not None and all(w>0 for w in weights):
                found.append(sum(1<<i for i in indices))
    return found

def closure_native_shaped(minimal,m):
    # Literal bitwise algorithm and low masks from copied catalogue_euler.hpp.
    low=(0x5555555555555555,0x3333333333333333,0x0F0F0F0F0F0F0F0F,0x00FF00FF00FF00FF,0x0000FFFF0000FFFF,0x00000000FFFFFFFF)
    used=1<<(m-6) if m>6 else 1
    words=[0]*used
    for mask in minimal:words[mask>>6]|=1<<(mask&63)
    original=tuple(words)
    for i in range(min(m,6)):
        for w in range(used):words[w]|=(words[w]&low[i])<<(1<<i)
    for i in range(6,m):
        for w in range(used):
            if (w>>(i-6))&1:words[w]|=words[w^(1<<(i-6))]
    weight=[sum(1<<b for b in range(64) if b.bit_count()==r) for r in range(7)]
    counts=[0]*(m+1)
    for w in range(used):
        if words[w]:
            for r in range(7):
                total=w.bit_count()+r
                if total<=m:counts[total]+=(words[w]&weight[r]).bit_count()
                else:need((words[w]&weight[r])==0,'closure leaked beyond shell')
    closed={mask for mask in range(1<<m) if (words[mask>>6]>>(mask&63))&1}
    return closed,counts,original,tuple(words)

def verify_fixture(name,shell,centre):
    beta=dot(sub(shell[0],centre),sub(shell[0],centre))
    need(all(dot(sub(p,centre),sub(p,centre))==beta for p in shell),name+' shell contact')
    actual=native_shaped_supports(shell,centre)
    expected=independent_supports(shell,centre)
    need(actual==expected,name+' predicate composition differs from barycentrics')
    need(len(actual)==len(set(actual)),name+' duplicate support')
    need(actual==sorted(actual,key=lambda mask:(mask.bit_count(),tuple(i for i in range(len(shell)) if mask>>i&1))),name+' output ordering')
    closed,counts,original,after=closure_native_shaped(actual,len(shell))
    direct={mask for mask in range(1<<len(shell)) if any(mask&q==q for q in expected)}
    need(closed==direct,name+' zeta closure')
    independent_counts=[sum(mask.bit_count()==j for mask in direct) for j in range(len(shell)+1)]
    need(counts==independent_counts,name+' N_j counts')
    need(set(actual)<=closed,name+' minimal support lost')
    return {'name':name,'m':len(shell),'beta':str(beta),'Q_counts':{str(q):sum(mask.bit_count()==q for mask in actual) for q in (2,3,4)},'minimal_supports':len(actual),'closed_parts':len(closed),'closed_parts_arity2to4':sum(2<=m.bit_count()<=4 for m in closed),'closure_overwrites_minimal_bits':original!=after,'N_j':counts}

def choose(n,k):return 0 if k<0 or k>n else comb(n,k)

def main():
    source=(ROOT/'sources/bench/catalogue_euler.hpp').read_text()
    required=['kMaxShell = 24','for (u32 l = k + 1; l < m; ++l)','if (num::is_midpoint(sphere, u[i], u[j]))','if (!num::strictly_acute(u[i], u[j], u[k])) continue;','num::orientation(u[i], u[j], u[k], sphere)','num::strictly_inside(sphere, u[i], u[j], u[k], u[l])','words[w] |= (words[w] & kLow[i]) << (1u << i)','words[w] |= words[w ^ (u64{1} << (i - 6))]']
    for text in required:need(text in source,'source guard changed: '+text)
    between=source[source.index('inline Outcome mark_supports'):source.index('inline Result<num::Point> site_point')]
    need('qmin' not in between and 'kmax' not in between and 'q4_presentation_strictly_inside' not in between,'enumerator wrongly narrows Q_b')
    cube=tuple(product((0,2),repeat=3))
    mixed=tuple(tuple(v+5 for v in p) for p in ((5,0,0),(-3,4,0),(-3,-4,0),(3,0,4),(-3,0,4),(0,3,-4),(0,-3,-4),(0,0,5),(0,0,-5)))
    fixtures=[('cube_mixed2_4',cube,(F(1),)*3),('sphere_mixed2_3_4',mixed,(F(5),)*3),('right_triangle',((0,5,0),(5,10,0),(10,5,0)),(F(5),F(5),F(0))),('equilateral',((0,0,0),(2,2,0),(2,0,2)),(F(4,3),F(2,3),F(2,3)))]
    results=[verify_fixture(*x) for x in fixtures]
    need(results[0]['Q_counts']=={'2':4,'3':0,'4':2},'active cube six-support guard')
    need(results[0]['closed_parts_arity2to4']>6 and results[0]['closure_overwrites_minimal_bits'],'closed bits cannot be published as Q_b')
    need(all(results[1]['Q_counts'][str(q)]>0 for q in (2,3,4)),'mixed fixture lacks an arity')
    need(results[2]['Q_counts']=={'2':1,'3':0,'4':0},'right triangle must only give diameter')
    profiles=[]
    for bits in (18,21,24):
        scale=((1<<bits)-1)//10
        shell=tuple(tuple(scale*v for v in p) for p in mixed)
        centre=(F(5*scale),)*3
        need(all(0<=x<(1<<bits) for p in shell for x in p),'Point profile domain')
        scaled=verify_fixture('mixed_scaled_u'+str(bits),shell,centre)
        need(scaled['Q_counts']==results[1]['Q_counts'],'profile-scale geometry changed')
        profiles.append({'bits':bits,'scale':scale,**scaled})
    # Internal cube event at K1 has qmin2. Q_b includes q4 despite zero cofaces.
    q4_cofaces=choose(8-4,1+1-4)
    need(q4_cofaces==0 and results[0]['Q_counts']['4']==2,'zero cofaces should not suppress tetrahedra')
    # All quantities below are scalar bounds, not huge allocations.
    support_cap=sum(comb(24,q) for q in (2,3,4))
    need(support_cap==12926 and support_cap<1<<32,'support count bound')
    max_k_parts=max_cofaces=max_support_cofaces=0
    for k in range(1,13):
        for p in range(k):
            for m in range(2,25):
                if p+m<k:continue
                if not any(p+q-1<=k and q<=m for q in (2,3,4)):continue
                parts=choose(p+m,k);cofaces=choose(p+m,k+1)
                max_k_parts=max(max_k_parts,parts);max_cofaces=max(max_cofaces,cofaces)
                need(parts<1<<32 and cofaces<1<<32,'per-ball field overflow')
                for q in (2,3,4):
                    if q>m:continue
                    value=choose(p+m-q,k+1-q)
                    max_support_cofaces=max(max_support_cofaces,value)
                    need(value<1<<32,'per-support cofaces overflow')
    need(max_k_parts==834451800 and max_cofaces==1476337800 and max_support_cofaces==193536720,'expected u32 maxima')
    need((1<<(24-6))*8==2097152,'scratch words/bytes')
    need(((1<<24)-1)<1<<32,'mask domain')
    maximum_balls=(1<<32)-2
    need(maximum_balls*support_cap<1<<64,'global support CSR offsets')
    sum_incidence_loose=sum(comb(24,q)*choose(35-q,13-q) for q in (2,3,4))
    need(sum_incidence_loose>1<<32,'sum metric is a distinct wider quantity')
    result={'status':'PASS','checks':CHECKS,'source_pin':'57dd21be1fd9ce68935910a78c8fc7174a1a3a5f','native_executed':False,'implementation_scope':'Reference mark_supports/closure_counts plus planned S6; no src/supports implementation exists in captured worktrees.','fixtures':results,'profile_scaled_scalar_fixtures':profiles,'capacity_bounds':{'kmax':12,'p_max':11,'m_max':24,'max_Q_supports_per_shell':support_cap,'max_k_parts':max_k_parts,'max_cofaces_loose':max_cofaces,'max_cofaces_per_support':max_support_cofaces,'scratch_bytes_per_worker':2097152,'support_CSR_count_bound':maximum_balls*support_cap,'sum_cofaces_incidence_loose_per_ball':sum_incidence_loose,'note':'These are per-field/scalar combinatorial upper bounds; no giant allocation, native overflow test or timing claim.'},'new_adverse_guards':{'export_minimal_before_zeta':True,'do_not_filter_zero_cofaces_support':True},'source_hashes':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'sources').rglob('*')) if p.is_file()}}
    print(json.dumps(result,sort_keys=True,indent=2))

if __name__=='__main__':main()
