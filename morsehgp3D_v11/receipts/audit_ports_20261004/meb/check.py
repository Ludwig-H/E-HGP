#!/usr/bin/env python3
"""Garde math ciblée de q3 différé : Gram/Gauss indépendant des formules C++.
Aucun oracle/moteur produit importé. Pas de qualification native ni chrono.
"""
from fractions import Fraction as F
from itertools import combinations,permutations
import json
CHECKS=0

def need(v,message):
 global CHECKS
 CHECKS+=1
 if not v:raise ValueError(message)

def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def gauss(rows,rhs):
 n=len(rows);a=[[F(v) for v in row]+[F(x)] for row,x in zip(rows,rhs)]
 for k in range(n):
  pivot=next((j for j in range(k,n) if a[j][k]),None)
  if pivot is None:return None
  a[k],a[pivot]=a[pivot],a[k];v=a[k][k];a[k]=[x/v for x in a[k]]
  for j in range(n):
   if j!=k:
    v=a[j][k];a[j]=[x-v*y for x,y in zip(a[j],a[k])]
 return [row[-1] for row in a]
def sphere_gram(points):
 a=points[0];u=[sub(p,a) for p in points[1:]]
 if not u:return tuple(map(F,a)),F(0),[F(1)]
 b=gauss([[dot(x,y) for y in u] for x in u],[F(dot(x,x),2) for x in u])
 if b is None:return None
 c=tuple(F(a[j])+sum(v*u[i][j] for i,v in enumerate(b)) for j in range(3));d=sub(c,a)
 return c,dot(d,d),[1-sum(b)]+b

def raw_q3(points):
 a,b,c=points;u,v=sub(b,a),sub(c,a);w=cross(u,v);g=dot(w,w)
 if not g:return None
 uu,vv=dot(u,u),dot(v,v);t=tuple(uu*v[j]-vv*u[j] for j in range(3));N=cross(t,w);D=2*g
 num=uu*vv*dot(sub(c,b),sub(c,b));den=4*g
 return a,N,D,num,den

def acute(points):
 a,b,c=points
 return dot(sub(b,a),sub(c,a))>0 and dot(sub(a,b),sub(c,b))>0 and dot(sub(a,c),sub(b,c))>0

def main():
 base=[[(0,0,0),(4,0,0),(2,3,0)],[(0,0,0),(4,0,0),(0,4,0)],[(0,0,0),(4,0,0),(1,1,0)],[(0,0,0),(2,0,0),(4,0,0)],[(1,2,3),(4,0,2),(0,5,1)]]
 rows=[]
 for bits in (18,21,24):
  for scale in (1,((1<<bits)-1)//16):
   for fi,pts0 in enumerate(base):
    pts=[tuple(scale*v for v in p) for p in pts0];oracle=sphere_gram(pts)
    for pp in permutations(pts):
     raw=raw_q3(pp);need((raw is None)==(oracle is None),'affine degeneracy')
     if raw is None:continue
     a,N,D,num,den=raw; c,r2,weights=sphere_gram(pp)
     need(tuple(F(a[j])+F(N[j],D) for j in range(3))==c,'independent Gram center')
     need(F(num,den)==r2,'degree6 raw radius vs Gram')
     need(acute(pp)==all(x>0 for x in weights),'strict positive support iff acute')
     for z in pts+[(0,0,0),(16*scale,16*scale,16*scale)]:
      d=sub(z,a);power=D*dot(d,d)-2*dot(N,d);diff=dot(sub(z,c),sub(z,c))-r2
      need(F(power,D)==diff,'candidate power equivalent before materialization')
    rows.append(dict(bits=bits,scale=scale,fixture=fi,strict=acute(pts),degenerate=oracle is None))
 # Exact non-vacuity: regular tetra0/2, same tuple and point-test order as explicit fixture.
 tetra=[(0,0,0),(2,2,0),(2,0,2),(0,2,2)]
 diameter=sphere_gram(tetra[:2]);point_tests=0;forms=1
 for z in tetra:
  point_tests+=1
  if dot(sub(z,diameter[0]),sub(z,diameter[0]))>diameter[1]:break
 rejected=0
 for t in combinations(range(4),3):
  forms+=1;pp=[tetra[i] for i in t];need(acute(pp),'four strict q3 prefixes')
  c,r2,_=sphere_gram(pp)
  for z in tetra:
   point_tests+=1
   if dot(sub(z,c),sub(z,c))>r2:rejected+=1;break
 c,r2,w=sphere_gram(tetra);forms+=1
 need(all(v>0 for v in w),'strict q4')
 for z in tetra:
  point_tests+=1;need(dot(sub(z,c),sub(z,c))<=r2,'q4 contains all')
 need(c==(1,1,1) and r2==3,'canonical MEB tetra')
 need(forms==6 and point_tests==17 and rejected==4,'exact logical ledger and four avoided q3 Levels')
 # Actual winning q3 raw presentation unchanged: (0,0),(4,0),(2,3).
 raw=raw_q3(base[0]);need(raw[3:]==(2704,576),'non-reduced winning q3 level')
 print(json.dumps(dict(status='PASS',checks=CHECKS,profiles=rows,tetra=dict(presentations=forms,point_tests=point_tests,rejected_q3_levels=rejected,winning_q=4),winning_q3_raw_level=[2704,576],scope='autonomous exact model plus static port reading; no native, fit, GCP, oracle-product imports'),sort_keys=True))
if __name__=='__main__':main()
