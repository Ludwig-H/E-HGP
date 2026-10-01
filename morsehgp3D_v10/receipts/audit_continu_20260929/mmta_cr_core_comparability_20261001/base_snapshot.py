#!/usr/bin/env python3
"""Limited read-only MMtA audit: actual AST functions, exact Gamma2, no native Scene."""
import ast
import hashlib
import itertools
import json
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace

ROOT=Path(__file__).absolute().parent
PINS={
 'principe_snapshot.py':'c4ab293273d9b84d370852f0953986fac4be8c27a50e9c50459523a9087e58bb',
 'gamma_reference_snapshot.py':'c99080876e3b62d5aebc184e28f501acae281fb4db023c8a07378bbcd8c88f29',
 'qsqrt_snapshot.py':'a1d1ed969fa5afc7b9a7e1746e07d770da45c5f80e9c2c80e034ad358394984d',
}

def require(c,m):
 if not c: raise RuntimeError(m)

def source(name):
 b=(ROOT/name).read_bytes(); require(hashlib.sha256(b).hexdigest()==PINS[name],'source pin '+name); return b

def select(src,names):
 ns=[n for n in ast.parse(src).body if isinstance(n,(ast.ClassDef,ast.FunctionDef)) and n.name in names]
 require({n.name for n in ns}==set(names),'AST inventory'); return ast.Module(body=ns,type_ignores=[])

qg={'__name__':'qsqrt_snapshot'}
exec(compile(source('qsqrt_snapshot.py'),'qsqrt_snapshot.py','exec'),qg)
QS=qg['QS']

class RayonAdapter:
 def __init__(self,b2=None,qs=None): self.qs=QS.sqrt(b2) if b2 is not None else qs
 @staticmethod
 def somme(q): return RayonAdapter(qs=q)
 def cmp(self,other): return self.qs.cmp(other.qs)

pg={'Fraction':F,'exiger':require,'QS':QS,'Rayon':RayonAdapter}
FUNCTIONS=('Donnees','poids_point','_masse','_taux','bord','_date','mmt_pondere','mmt_pondere_lent','qmax')
exec(compile(select(source('principe_snapshot.py'),FUNCTIONS),'principe_snapshot.py','exec'),pg)
Donnees=pg['Donnees']

def d2(a,b): return sum((x-y)**2 for x,y in zip(a,b))

def meb(points):
 balls=[]
 for a,b in itertools.combinations(points,2):
  c=tuple((u+v)/2 for u,v in zip(a,b)); r=d2(a,b)/4
  if all(d2(c,z)<=r for z in points): balls.append(r)
 if len(points)==3:
  a,b,c=points; u=tuple(z-w for z,w in zip(b,a)); v=tuple(z-w for z,w in zip(c,a))
  G=sum(t*t for t in u); H=sum(x*y for x,y in zip(u,v)); J=sum(t*t for t in v); D=G*J-H*H
  if D:
   s=(G*J-H*J)/(2*D); t=(G*J-H*G)/(2*D)
   C=tuple(a[i]+s*u[i]+t*v[i] for i in range(len(a))); r=d2(C,a)
   require(all(d2(C,z)==r for z in points),'circumcenter'); balls.append(r)
 require(balls,'MEB'); return min(balls)

def tree(P):
 n=len(P); pairs=list(itertools.combinations(range(n),2)); triples=list(itertools.combinations(range(n),3))
 beta={f:meb([P[i] for i in f]) for f in pairs+triples}
 birth=[]; parent=[]; children=[]; cover=[{} for _ in P]; active={}
 for q in sorted(set(beta.values())):
  live=[p for p in pairs if beta[p]<=q]; un={p:p for p in live}
  def find(p):
   while un[p]!=p: p=un[p]
   return p
  for t in triples:
   if beta[t]<=q:
    ps=list(itertools.combinations(t,2)); r=find(ps[0])
    for p in ps[1:]: un[find(p)]=r
  comps={}
  for p in live: comps.setdefault(find(p),set()).add(p)
  nxt={}
  for C in sorted(comps.values(),key=lambda s:tuple(sorted(s))):
   old=sorted({v for oldC,v in active.items() if set(oldC)<=C})
   if len(old)==1: v=old[0]
   else:
    v=len(birth); birth.append(q); parent.append(-1); children.append(old)
    for u in old: parent[u]=v
   for i in set(itertools.chain.from_iterable(C)): cover[i].setdefault(v,q)
   nxt[frozenset(C)]=v
  active=nxt
 root=next(v for v,p in enumerate(parent) if p<0)
 require(sum(p<0 for p in parent)==1,'root')
 death=[None if p<0 else birth[p] for p in parent]
 depth=[None]*len(birth); topdown=[]; todo=[(root,0)]
 while todo:
  v,d=todo.pop(); topdown.append(v); depth[v]=d
  todo.extend((c,d+1) for c in children[v])
 T=SimpleNamespace(birth=birth,parent=parent,children=children,death=death,root=root,depth=depth,topdown=topdown)
 # Constructor adapter reproduces Donnees.__init__, including Euler/point IDs.
 dk2=[min(d2(P[x],P[y]) for y in range(n) if y!=x) for x in range(n)]
 core=[]
 for x in range(n):
  first=min(cover[x].values()); candidates=[]
  for v,c in cover[x].items():
   if c!=first: continue
   while parent[v]>=0 and birth[parent[v]]<=dk2[x]: v=parent[v]
   candidates.append(v)
  require(candidates and len(set(candidates))==1,'core unique nearest component'); core.append(candidates[0])
 ctx=SimpleNamespace(point_id=list(range(n)),n=n,dk2=lambda x:dk2[x],order=SimpleNamespace(core_node=core))
 sc=SimpleNamespace(n=n,noms=[str(x) for x in range(n)],ctx=ctx,arbre_masses=lambda:(T,cover,{}))
 D=Donnees(sc)
 require(D.cov==cover and D.T is T,'constructor coverage')
 return D,beta

def maximum(qs):
 out=qs[0]
 for q in qs[1:]:
  if q.cmp(out)>0: out=q
 return out

def evaluate(P,mcs=3):
 D,beta=tree(P); T=D.T; a,A=D.admissibilite(mcs)
 results=[]; checks=0
 for x in range(len(P)):
  port,Ax,Ah,E2=pg['poids_point'](D,x,mcs,F(1,2),F(1,2))
  S=Ah
  # S is exactly first coverage by an admissible component, with no weight-support filtering.
  direct=min(max(c,a[v]) for v,c in D.cov[x].items() if a[v] is not None)
  require(Ah==S==direct and E2==Ah+F(1,2)*Ax,'first admissible coverage / band'); checks+=1
  r=pg['mmt_pondere'](D,port,Ax,E2,F(3)); slow=pg['mmt_pondere_lent'](D,port,Ax,E2,F(3))
  require(r is not None and slow is not None,'unexpected W0')
  require(r['W']>=F(1,2)*Ax,'W lower bound'); checks+=1
  require(r['date'].cmp(slow['date'])==0,'fast/slow date')
  require(all(r[k]==slow[k] for k in ('O','T_half','W','E2')),'fast/slow state'); checks+=5
  date=r['date']; O=r['O']
  require(A[O] is not None,'owner admissibility')
  date=maximum([date,QS.sqrt(A[O])]); owner=D.anc_rayon(O,RayonAdapter.somme(date))
  require(owner in D.cov[x] and date.cmp(QS.sqrt(D.cov[x][owner]))>=0,'NP')
  require(A[owner] is not None and date.cmp(QS.sqrt(A[owner]))>=0,'admissibility final'); checks+=2
  results.append({'date_q':date,'owner':owner,'S':S,'Ax':Ax,'E2':E2,'W':r['W'],'T_half':r['T_half'],'omega':[(c,Av,om,T.birth[v],T.death[v]) for v,(c,Av,om) in port.items()], 'argmax':r['argmax']})
 heights={}
 for x,y in itertools.combinations(range(len(P)),2):
  C=D.lca(results[x]['owner'],results[y]['owner'])
  heights[(x,y)]=maximum([results[x]['date_q'],results[y]['date_q'],QS.sqrt(T.birth[C])])
 return results,heights,checks

def enc(results,heights):
 return {'points':[{'date':r['date_q'].text(),'date_approx':float(r['date_q']),'owner':r['owner'],
                    'Ahat':str(r['S']),'Ax':str(r['Ax']),'E2':str(r['E2']),'W':str(r['W']),'T_half':str(r['T_half']),
                    'omega':[[str(c),None if Av is None else str(Av),str(om),str(b),None if d is None else str(d)] for c,Av,om,b,d in r['omega']],
                    'argmax':list(map(str,r['argmax']))} for r in results],
         'heights':[[list(xy),q.text(),float(q)] for xy,q in heights.items()]}

def run():
 fixtures=[]
 for e in (F(0),F(1,8),F(1,32),F(1,128),F(1,1024)):
  fixtures.append(('five',e,((F(0),F(0)),(F(6),F(0)),(F(0),F(8)),(F(-1),F(8)),(F(6),F(8)-e)),3))
 for e in (F(-1,128),F(0),F(1,128)):
  fixtures.append(('collinear',e,((F(0),F(0)),(F(2)-e,F(0)),(F(4),F(0))),3))
  fixtures.append(('square',e,((F(-1),F(-1)),(F(1),F(-1)),(F(1)+e,F(1)),(F(-1),F(1))),3))
  fixtures.append(('contact',e,((F(0),F(0),F(0)),(F(10),F(0),F(0)),(F(9)-e,F(3),F(0)),(F(0),F(0),F(9))),3))
 out=[]; grouped={}; total=0
 for name,e,P,mcs in fixtures:
  rs,hs,count=evaluate(P,mcs); total+=count
  out.append({'fixture':name,'epsilon':str(e),'coordinates':[[str(v) for v in p] for p in P], 'mcs':mcs,**enc(rs,hs)})
  grouped.setdefault(name,{})[e]=(P,rs,hs)
 jumps=[]
 for name,data in grouped.items():
  P0,r0,h0=data[F(0)]
  for e,(P,r,h) in data.items():
   if not e: continue
   displacement=QS.sqrt(max(d2(a,b) for a,b in zip(P,P0)))
   dt=maximum([maximum([r[i]['date_q']-r0[i]['date_q'],r0[i]['date_q']-r[i]['date_q']]) for i in range(len(P))])
   dh=maximum([maximum([h[xy]-h0[xy],h0[xy]-h[xy]]) for xy in h])
   ds=maximum([maximum([QS.sqrt(r[i]['S'])-QS.sqrt(r0[i]['S']),QS.sqrt(r0[i]['S'])-QS.sqrt(r[i]['S'])]) for i in range(len(P))])
   require(ds.cmp(displacement)<=0,'first admissible scale interleaving bound')
   jumps.append({'fixture':name,'epsilon':str(e),'displacement':displacement.text(), 'max_scale_change':ds.text(),'max_scale_change_approx':float(ds),'max_date_change':dt.text(),'max_date_change_approx':float(dt),'max_height_change':dh.text(),'max_height_change_approx':float(dh)})
 require(len(out)==14 and total>0,'nonvacuity')
 for name in PINS: source(name)
 return {'parameters':{'K':2,'mcs':3,'eta':'1/2','kappa':'3','lambda':'1/2'},'scope':'actual AST Donnees/poids_point/MMtA fast+slow over independent exact Gamma2; no Scene/native', 'cases':out,'jumps_to_zero':jumps,'checks':total,'source_pins':PINS,'native_calls':0,'GCP':'unused'}

if __name__=='__main__': print(json.dumps(run(),sort_keys=True,indent=1))
