#!/usr/bin/env python3
"""Temoins exacts bornes Q1/Q2 : boules minimales et nerf des intersections."""
import argparse,hashlib,itertools,json,subprocess
from fractions import Fraction as Q
from functools import lru_cache
from pathlib import Path
HERE=Path(__file__).resolve().parent

def need(ok,why):
 if not ok:raise ValueError(why)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def solve(a,b):
 a=[list(map(Q,row))+[Q(v)]for row,v in zip(a,b)];n=len(a)
 for i in range(n):
  at=next((j for j in range(i,n)if a[j][i]),None)
  if at is None:return None
  a[i],a[at]=a[at],a[i];v=a[i][i];a[i]=[x/v for x in a[i]]
  for j in range(n):
   if j!=i:
    v=a[j][i];a[j]=[x-v*y for x,y in zip(a[j],a[i])]
 return tuple(row[-1]for row in a)

class Geometry:
 def __init__(self,points):self.points=tuple(tuple(map(Q,p))for p in points)
 @lru_cache(None)
 def sphere(self,support):
  p=self.points[support[0]]
  if len(support)==1:return p,Q(0)
  v=[sub(self.points[i],p)for i in support[1:]];alpha=solve([[dot(a,b)for b in v]for a in v],[dot(a,a)/2 for a in v])
  if alpha is None:return None
  c=tuple(p[j]+sum(a*u[j]for a,u in zip(alpha,v))for j in range(3));return c,dot(sub(c,p),sub(c,p))
 @lru_cache(None)
 def meb(self,part):
  need(bool(part),'empty MEB');best=None
  for q in range(1,min(4,len(part))+1):
   for s in itertools.combinations(part,q):
    made=self.sphere(s)
    if made is None:continue
    c,r=made
    if (best is None or r<best[1])and all(dot(sub(self.points[i],c),sub(self.points[i],c))<=r for i in part):best=(c,r)
  need(best is not None,'MEB exists');return best
 def level(self,part):return self.meb(tuple(sorted(part)))[1]

def connected(vertices,edge):
 parent=list(range(len(vertices)))
 def root(i):
  while parent[i]!=i:i=parent[i]
  return i
 for i,j in itertools.combinations(range(len(vertices)),2):
  if edge(vertices[i],vertices[j]):
   a,b=root(i),root(j);parent[max(a,b)]=min(a,b)
 groups={}
 for i,v in enumerate(vertices):groups.setdefault(root(i),[]).append(v)
 return list(groups.values())

def coverage(g,k,level,strict=True):
 inside=(lambda r:r<level)if strict else(lambda r:r<=level)
 parts=[f for f in itertools.combinations(range(len(g.points)),k)if inside(g.level(f))]
 groups=connected(parts,lambda a,b:inside(g.level(tuple(set(a)|set(b)))))
 return groups,{f:j for j,group in enumerate(groups)for f in group}

def local(g,inner,shell,k,level):
 t=k-len(inner);As=[a for a in itertools.combinations(shell,t)if g.level(a)<level]if t>0 else[]
 traces={a:tuple(sorted(inner+a))for a in As}
 need(all(g.level(f)<level for f in traces.values()),'inner preserves strictness')
 groups=connected(As,lambda a,b:g.level(tuple(set(a)|set(b)))<level)
 adjacent=connected(As,lambda a,b:len(set(a)|set(b))==t+1 and g.level(tuple(set(a)|set(b)))<level)
 need(groups==adjacent,'adjacent moves span each local component')
 global_groups,where=coverage(g,k,level)
 for group in groups:need(len({where[traces[a]]for a in group})==1,'local implies global equivalence')
 original={where[f]for f in traces.values()};compressed={where[traces[group[0]]]for group in groups}
 need(original==compressed,'same open branches')
 return {'traces':[list(traces[a])for a in As],'local_components':[[list(traces[a])for a in group]for group in groups],'representatives':[list(traces[group[0]])for group in groups],'open_global_components':len(global_groups),'open_branch_set':sorted(original)}

def run(repo):
 cap=json.loads((HERE/'capture.json').read_text())
 for p,h in cap['sources']:
  data=subprocess.check_output(['git','show',cap['base_git']+':'+p],cwd=repo);need(hashlib.sha256(data).hexdigest()==h,'source pin '+p)
 triangle=Geometry([(0,0,0),(4,0,0),(2,4,0)]);tl=triangle.level((0,1,2));need(tl==Q(25,4),'triangle critical level')
 tr=local(triangle,(),(0,1,2),2,tl);need(len(tr['local_components'])==3 and tr['open_global_components']==3,'common vertex does not identify branches')
 need(len(coverage(triangle,2,tl,False)[0])==1,'triangle threeway merge')
 tetra=Geometry([(0,0,0),(0,2,2),(2,0,2),(2,2,0)]);need(tetra.level((0,1,2,3))==3,'tetra critical level');need(all(tetra.level(f)==Q(8,3)for f in itertools.combinations(range(4),3)),'tetra faces')
 high,hw=coverage(tetra,3,Q(17,6));low,lw=coverage(tetra,2,Q(17,6));need(len(high)==4 and len(low)==1,'vertical not injective')
 need(all(len({lw[e]for e in itertools.combinations(f,2)})==1 for group in high for f in group),'every subpart in same lower component')
 tet=local(tetra,(),tuple(range(4)),3,Q(3));need(len(tet['local_components'])==4,'common edge does not identify branches')
 points=[(10,5,0),(9,8,0),(8,9,0),(5,10,0),(1,2,0)]
 ext=Geometry(points);e=local(ext,(),tuple(range(5)),2,Q(25));need(sorted(map(len,e['local_components']))==[1,8],'extended 9 to 2')
 ext_i=Geometry(points+[(5,5,0)]);ei=local(ext_i,(5,),tuple(range(5)),3,Q(25));need(sorted(map(len,ei['local_components']))==[1,8],'extended with interior 9 to 2')
 # All critical sub-shells of this five-site sphere, with and without its interior centre.
 bounded=0
 for m in range(2,6):
  for chosen in itertools.combinations(range(5),m):
   if ext.meb(chosen)!=((Q(5),Q(5),Q(0)),Q(25)):continue
   for with_i in (False,True):
    g=Geometry([points[i]for i in chosen]+([(5,5,0)]if with_i else[]));inner=(m,)if with_i else();shell=tuple(range(m))
    for t in range(1,m+1):local(g,inner,shell,t+len(inner),Q(25));bounded+=1
 # Same minimal sphere but different order-local cell: junction at k=3, birth at k=4.
 shared=Geometry([(0,0,0),(4,0,0),(2,1,0),(2,2,0)]);a=(0,1,2);b=(0,1,2,3)
 need(shared.meb(a)==shared.meb(b)==((Q(2),Q(0),Q(0)),Q(4)),'shared MEB certificate')
 j=local(shared,(2,),(0,1,3),3,Q(4));birth=local(shared,(2,),(0,1,3),4,Q(4));need(len(j['local_components'])==2 and not birth['traces'],'same sphere does not imply same order target')
 # Saturation at k does not imply saturation at k+1, even for identical certified sphere.
 sat=Geometry([(0,5,0),(10,5,0),(4,5,0),(6,5,0)]);need(sat.meb((0,1))==sat.meb((0,1,2))==((Q(5),Q(5),Q(0)),Q(25)),'saturation fixture sphere')
 c,r=sat.meb((0,1));p=sum(dot(sub(z,c),sub(z,c))<r for z in sat.points);need(p==2 and p>=2 and not p>=3,'threshold obstruction')
 # Reusing only a support without testing the added point is false.
 outside=Geometry([(0,0,0),(4,0,0),(2,1,0),(2,3,0)]);need(outside.level((0,1,2,3))>outside.level((0,1,2)),'enclosure guard necessary')
 return {'triangle':{'points':[[int(x)for x in p]for p in triangle.points],'critical_level':'25/4','open':tr,'closed_components':1},'tetrahedron':{'points':[[int(x)for x in p]for p in tetra.points],'cut':'17/6','face_level':'8/3','critical_level':'3','components_k2':1,'components_k3':4,'open':tet},'extended':{'points':points,'critical_level':'25','p':0,'k':2,'qmin':2,**e},'extended_with_interior':{'points':points+[(5,5,0)],'critical_level':'25','p':1,'k':3,'qmin':2,**ei},'bounded_local_cases':bounded,'shared_sphere':{'points':[[int(x)for x in p]for p in shared.points],'level':'4','k3_local_components':2,'k4_is_birth':True},'saturation':{'points':[[int(x)for x in z]for z in sat.points],'level':'25','p':2,'saturated_k2':True,'saturated_k3':False},'guards_refuted':['common_k_minus_1_subset','same_lower_component_implies_same_upper_component','same_sphere_implies_same_order_target','saturation_k_implies_saturation_k_plus_1','reuse_support_without_enclosure']}
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--repo',type=Path,required=True);a.add_argument('--write',action='store_true');x=a.parse_args();r=json.loads(json.dumps(run(x.repo)))
 if x.write:(HERE/'results.json').write_text(json.dumps(r,separators=(',',':'))+'\n')
 else:need(r==json.loads((HERE/'results.json').read_text()),'results changed')
 print(json.dumps({'status':'ok','bounded_local_cases':r['bounded_local_cases'],'extended_traces':len(r['extended']['traces']),'extended_resolutions':len(r['extended']['representatives']),'tetra_k2':r['tetrahedron']['components_k2'],'tetra_k3':r['tetrahedron']['components_k3'],'guard_obstructions':len(r['guards_refuted'])}))
