#!/usr/bin/env python3
"""Borned S6 scalar/Gram proof: no native or 2**24 masks; source pins explicit."""
import ast
from fractions import Fraction
from itertools import combinations
from math import comb,isqrt
from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parent
CHECKS=0
PINS={
'wip/src/supports/counts.cpp':'23e01a8784e54d0dbe8d60ce3a2ec7ad9f4cae53f8668f39a5f2d4e728e8989a',
'wip/src/supports/counts.hpp':'c3e91e1de8bc7f72f5dc7b7067e1d72d6e9e82f924dd713cfc5287231092883e',
'wip/src/supports/supports.hpp':'cd0c4dd4aee1868b2eb266e9bcba7340247dc3e2fc36d8706c44c14de358993b',
'wip/src/supports/enumerate.cpp':'3a8fcd0d7af148e4d7748234c8c02bec5f75158bfef41c5e7c5299177dbe7c86',
'published/reference/hgp11_ref/supports.py':'826f3f94ce9eeb66e6ed04edce8266dd70b52498fd929c2ec7f7dbc01538a541'}
def need(ok,why):
 global CHECKS
 CHECKS+=1
 if not ok:raise RuntimeError(why)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def det(a,b,c):return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])
def determinant_positive(ps):
 if len(ps)==2:return all(x+y==0 for x,y in zip(*ps))
 if len(ps)==3:
  a,b,c=ps
  return det(a,b,c)==0 and all(dot(sub(b,a),sub(c,a))>0 for a,b,c in ((a,b,c),(b,a,c),(c,a,b)))
 a,b,c,d=ps;D=det(sub(b,a),sub(c,a),sub(d,a))
 w=(det(b,c,d),-det(a,c,d),det(a,b,d),-det(a,b,c))
 return D!=0 and all(x*D>0 for x in w)
def points_on(r2):
 r=isqrt(r2)
 return tuple((x,y,z) for x in range(-r,r+1) for y in range(-r,r+1) for z in range(-r,r+1) if x*x+y*y+z*z==r2)
def universe_supports(pts,gram,verify_gram):
 found=[];by={2:0,3:0,4:0}
 for a in (2,3,4):
  for ids in combinations(range(len(pts)),a):
   ps=[pts[i] for i in ids];yes=determinant_positive(ps)
   if verify_gram:
    w=gram(ps,(0,0,0));independent_yes=w is not None and all(x>0 for x in w)
    need(yes==independent_yes,'determinant vs exact Gram')
   if yes:found.append(ids);by[a]+=yes
 return tuple(found),by

def bounded_closure(pts,Qs):
 minimal={sum(1<<i for i in Q) for Q in Qs}
 counts={2:0,3:0,4:0};examples={}
 for a in (2,3,4):
  for ids in combinations(range(len(pts)),a):
   mask=sum(1<<i for i in ids)
   closed=any(sum(1<<i for i in subpart) in minimal for b in range(2,a+1) for subpart in combinations(ids,b))
   counts[a]+=closed
   if closed and mask not in minimal:examples.setdefault(a,ids)
 return counts,examples

def scalar_factory(p,m,q,k):
 if m>24:return 'support_shell_capacity'
 return 'supports_invariant' if k<1 or k>12 or q<2 or q>4 or m<q or p>11 or p+q>k+1 else 'ok'
def choose(x,y):return 0 if y<0 or y>x else comb(x,y)
def main():
 for name,h in PINS.items():need(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==h,'source pin '+name)
 cpp=(ROOT/'wip/src/supports/counts.cpp').read_text();hpp=(ROOT/'wip/src/supports/counts.hpp').read_text();enum=(ROOT/'wip/src/supports/enumerate.cpp').read_text();public=(ROOT/'wip/src/supports/supports.hpp').read_text()
 need('p > kMaxInterior || p + q > u32{k} + 1' in cpp,'p bound evaluated before sum')
 need('kMaxInterior = kMaxOrder - 1' in hpp and 'kMaxOrder = 12' in hpp,'pmax11 pinned')
 need(hpp.count('if (arity < 2 || arity > 4) return 0;')==2,'arity helpers total guard')
 need('y < 0 || y > x || x > static_cast<i32>(kBinomialTop)' in hpp,'table upper guard')
 need('counts.strict_traces = counts.compressed_parts - closure.parts(static_cast<u32>(t));' in cpp,'strict traces source formula')
 need('closure.parts(shape.qmin()) == 0' in cpp and '(j < q && parts != 0)' in cpp,'closure foreign-shape guards')
 need(enum.index('out[count++] = found;') < enum.index('void zeta_or'),'materialized minimal list source')
 need(enum.index('MHGP11_TRY(enumerate(sphere.value()')<enum.index('zeta_or(marks, m);'),'zeta after Q enumeration')
 need('num::strictly_inside(sphere, u[i], u[j], u[k], u[l])' in enum,'q4 direct predicate, no presentation flag')
 need('if (ledger != nullptr) ledger->add(work);' in enum,'success-only ledger commit')
 need('registre par fil' in public and 'apres la jointure' in public,'private ledgers contract')

 bad_rejected=0;valid=0
 for k in (1,2,12):
  for q in (2,3,4):
   for p in range((1<<32)-4,1<<32):
    need(scalar_factory(p,q,q,k)=='supports_invariant','large p refused');bad_rejected+=1
 for k in range(1,13):
  for p in range(k):
   for q in (2,3,4):
    for m in (q,24):
     actual=scalar_factory(p,m,q,k);expected='ok' if p+q<=k+1 else 'supports_invariant'
     need(actual==expected,'bounded Shape domain unchanged');valid+=actual=='ok'
 need(scalar_factory(0xffffffff,25,2,1)=='support_shell_capacity','shell refusal precedes shape refusal')
 need(scalar_factory(11,24,2,12)=='ok','max valid boundary')

 parsed=ast.parse((ROOT/'published/reference/hgp11_ref/supports.py').read_text());names={'_gauss','barycentric'}
 funcs=[n for n in parsed.body if isinstance(n,ast.FunctionDef) and n.name in names]
 need({n.name for n in funcs}==names,'standalone S1 Gram helpers found')
 ns={'Fraction':Fraction};exec(compile(ast.Module(body=funcs,type_ignores=[]),'pinned_S1_Gram','exec'),ns)
 gram=ns['barycentric'];pts=points_on(5)
 need(len(pts)==24 and len(set(pts))==24,'sphere5 exact24 distinct')
 Qs,arity=universe_supports(pts,gram,True)
 closure,nonminimal=bounded_closure(pts,Qs)
 need(set(arity)=={2,3,4} and all(arity.values()),'three arities present')
 need(closure[2]==arity[2] and closure[3]>arity[3] and closure[4]>arity[4],'minimal not zeta output')
 triples=[(2,1,0),(-2,1,0),(1,-2,0)];tetra=[(2,1,0),(-2,1,0),(0,-1,2),(0,-1,-2)]
 need(gram(triples,(0,0,0))==(Fraction(1,4),Fraction(5,12),Fraction(1,3)),'given exact q3 weights')
 need(gram(tetra,(0,0,0))==(Fraction(1,4),)*4,'given exact q4 weights')
 need(det(sub(tetra[1],tetra[0]),sub(tetra[2],tetra[0]),sub(tetra[3],tetra[0]))==-32,'q4 affine rank')
 nonminimal_ps=[pts[i] for i in nonminimal[3]]
 w=gram(nonminimal_ps,(0,0,0));need(not(w is not None and all(x>0 for x in w)),'zeta nonminimal triple is not Q')
 base_sets={tuple(sorted(pts[i] for i in Q)) for Q in Qs}
 for order in (tuple(reversed(range(24))),tuple(range(1,24))+ (0,)):
  perm=tuple(pts[i] for i in order);newQ,newarity=universe_supports(perm,gram,False);newN,_=bounded_closure(perm,newQ)
  need({tuple(sorted(perm[i] for i in Q)) for Q in newQ}==base_sets,'permutation Q exact')
  need(newarity==arity and newN==closure,'permutation N exact')
 for bits in (18,21,24):
  scale=((1<<bits)-1)//4;native=[tuple((x+2)*scale for x in p) for p in pts];center=(2*scale,)*3
  need(all(0<=c<(1<<bits) for p in native for c in p),'profile coordinates admissible')
  need(all(dot(sub(p,center),sub(p,center))==5*scale*scale for p in native),'exact translated homothety shell')
  need(gram([native[pts.index(p)] for p in triples],center)==(Fraction(1,4),Fraction(5,12),Fraction(1,3)),'q3 similarity exact')
  need(gram([native[pts.index(p)] for p in tetra],center)==(Fraction(1,4),)*4,'q4 similarity exact')

 low_counts=[]
 for k in (1,2,3):
  counts={'k':k,'kparties_reliees':comb(24,k),'compressed_parts':comb(24,k),'strict_traces':comb(24,k)-closure.get(k,0),'cofaces':closure[k+1],'gabriel_cofaces':closure[k+1],'per_support_cofaces':{str(a):choose(24-a,k+1-a) for a in (2,3,4)}}
  low_counts.append(counts)
  need(counts['strict_traces']>=0,'no strict count underflow')
 need(low_counts[0]['per_support_cofaces']['4']==0 and arity[4]>0,'K1 q4 no coface retained')
 need(low_counts[1]['cofaces']!=arity[3] and low_counts[2]['cofaces']!=arity[4],'omitted zeta mutation detected')

 # p0/K<=3 uses at most 4-site combinations, not the 2**24 mask space.
 other=[]
 for r2 in (6,10,11,13):
  pp=points_on(r2);qq,aa=universe_supports(pp,gram,False);nn,_=bounded_closure(pp,qq)
  need(len(pp)==24,'other proposed24-site sphere')
  other.append({'squared_radius':r2,'sites':len(pp),'Q_by_arity':aa,'N2_N3_N4':nn,'proof':'exact determinants and containment of minimal masks; no independent Gram cross-check on this row'})
 pp=points_on(9);need(len(pp)==30,'sphere9 has30 sites')
 axes={(3,0,0),(-3,0,0),(0,3,0),(0,-3,0),(0,0,3),(0,0,-3)}
 selected=tuple(sorted(axes))+tuple(p for p in pp if p not in axes)[:19]
 need(len(selected)==25 and len(set(selected))==25,'specified sphere9 subset25')
 need((3,0,0) in selected and (-3,0,0) in selected,'opposite pair guarantees same central MEB')
 need(all(dot(p,p)==9 for p in selected),'same complete25 shell in selected cloud')
 need(scalar_factory(0,25,2,1)=='support_shell_capacity','primitive refuses25 before masks')

 # Counts and public packed fields, analytic upper bounds only: no sizeof or RSS measurement.
 max_ball_parts=max(comb(p+m,k) for k in range(1,13) for p in range(k) for m in range(2,25))
 max_ball_cofaces=max(comb(p+m,k+1) for k in range(1,13) for p in range(k) for m in range(2,25))
 B=(1<<32)-1;maxS=comb(24,2)+comb(24,3)+comb(24,4);Z=4*B*maxS
 need(maxS==12926 and maxS<(1<<16),'support_count u16 safe')
 need(max_ball_parts==834451800 and max_ball_cofaces==1476337800,'local count maxima')
 need(max_ball_parts*B<(1<<64) and max_ball_cofaces*B<(1<<64),'manifest per-ball sums u64 safe')
 need(B*maxS<(1<<64) and Z<(1<<64),'global supports and arities use u64 safe')
 need(48*((1<<(24-6))*8)==100663296,'conditional closure workspace48 is96MiB')
 result={'status':'PASS','checks':CHECKS,'native_executed':False,'source_sha256':PINS,'fixed_shape':{'large_p_refusals':bad_rejected,'valid_cases_retained':valid},'sphere5':{'points':pts,'squared_radius':5,'sites':24,'presentations_up_to4':sum(comb(24,a) for a in (2,3,4)),'Q_by_arity':arity,'Q_total':len(Qs),'N2_N3_N4':closure,'counts_K1_K2_K3':low_counts,'q3_weights':['1/4','5/12','1/3'],'q4_weights':['1/4']*4,'q4_determinant':-32,'nonminimal_zeta_example3':nonminimal_ps,'mutation_guards':['replace minimal Q by closure','omit zeta when counting N','remove q4 at K1 because cofaces0']},'other_spheres':other,'sphere9_refusal_subset25':{'squared_radius':9,'full_shell_size':30,'selected_points':selected,'qmin':2,'k':1,'expected_refusal':'support_shell_capacity','scope':'mathematical cloud and scalar public factory, not native rejection/FULL execution'},'capacity':{'max_supports_per_ball':maxS,'global_S_at_B_u32max':B*maxS,'global_Z_upper_at_B_u32max':Z,'max_kparties_per_ball':max_ball_parts,'max_cofaces_per_ball':max_ball_cofaces,'max_kparties_sum_at_B_u32max':max_ball_parts*B,'max_cofaces_sum_at_B_u32max':max_ball_cofaces*B,'closure_scratch_bytes_m24':(1<<(24-6))*8,'closure_scratch_bytes_W48_m24':100663296,'future_admission_formula':'resident domain + resident tree + count metadata + W_active * (8*closure_words(max_selected_m) + support_capacity(max_selected_m)*sizeof(Support)) + exact admitted output arrays','sizeof_Support_measured':False},'scope':['Source S6 WIP at basef98, published S1 helpers Git5ad; new documentation929 separately pinned.','Sphere5 cross-check uses all12926 presentations versus independent Gram; other4 rows only determinant/containment computation.','No enumeration of2**24 subsets, native execution, memory allocation stress, G4 or wall-time claim.','S6a primitive takes caller buffers; global count/fill assembly/admission not delivered in this snapshot.']}
 print(json.dumps(result,sort_keys=True,indent=2))
if __name__=='__main__':main()
