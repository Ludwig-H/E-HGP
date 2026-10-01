#!/usr/bin/env python3
"""Read-only exact Gamma2 diagnostic of actual MMtA_CR AST, no native Scene."""
import ast
import hashlib
import itertools
import json
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace

ROOT=Path(__file__).absolute().parent
BASE_SHA='357f3233ea749e80b0e611cfdb80dd2b85add2e587889f1da876407a371bbb19'
blob=(ROOT/'base_snapshot.py').read_bytes()
if hashlib.sha256(blob).hexdigest()!=BASE_SHA: raise RuntimeError('base SHA')
g={'__name__':'base_snapshot','__file__':str(ROOT/'base_snapshot.py')}
exec(compile(blob,str(ROOT/'base_snapshot.py'),'exec'),g)
pg=g['pg']; QS=g['QS']; Rayon=g['RayonAdapter']; require=g['require']

class HierarchieAdapter:
 def __init__(self,sc,nom,mode,foret,dates,owners):
  self.dates=dates; self.proprietaires=owners

pg.update({'KAPPA':F(3),'ETA':F(1,2),'LAM':F(1,2),'RG':SimpleNamespace(Hierarchie=HierarchieAdapter)})
extra=('adm_coeur','Regle','_repli_racine','mmta','niveau_coeur','mmta_cr')
exec(compile(g['select'](g['source']('principe_snapshot.py'),extra),'principe_snapshot.py','exec'),pg)

def evaluate(P,mcs,variant):
 D,beta=g['tree'](P); D.sc.foret=D.T; D._audit_beta=beta
 result=pg['mmta_cr'](D.sc,mcs,variante=variant,D=D)
 baseline=pg['mmta'](D.sc,mcs,D=D)
 _a,A=D.admissibilite(mcs)
 rows=[]; checks=0
 for x in range(len(P)):
  detail=result.details[x]; date=result.h.dates[x].qs; owner=result.h.proprietaires[x]
  require(owner in D.cov[x] and date.cmp(QS.sqrt(D.cov[x][owner]))>=0,'NP')
  require(A[owner] is not None and date.cmp(QS.sqrt(A[owner]))>=0,'admissibility')
  rc=detail['rc']; require(rc is not None,'rc finite')
  require(date.cmp(QS.sqrt(rc))<=0,'CR entry bound')
  Cc=D.anc(D.coeur[x],rc)
  require(D.anc(owner,rc)==Cc,'CR owner at rc')
  cv={v:c for v,c in D.cov[x].items() if D.sous(v,Cc) or D.sous(Cc,v)}
  port,Ax,Ah,E2=pg['poids_point'](D,x,mcs,F(1,2),F(1,2),False,cv)
  fast=pg['mmt_pondere'](D,port,Ax,E2,F(3)); slow=pg['mmt_pondere_lent'](D,port,Ax,E2,F(3))
  require(fast is not None and slow is not None,'positive mass')
  require(fast['date'].cmp(slow['date'])==0,'fast slow date')
  require(all(fast[k]==slow[k] for k in ('O','T_half','W','E2')),'fast slow state')
  require(fast['W']>=F(1,2)*Ax,'W guard restricted hereditary chain')
  checks+=10
  rows.append({'date_q':date,'date':date.text(),'date_approx':float(date),'owner':owner,
               'baseline_date':baseline.h.dates[x].qs.text(),'baseline_date_approx':float(baseline.h.dates[x].qs),
               'core':D.coeur[x],'dk2':str(D.dk2[x]),'rc':str(rc),'Cc':Cc,
               'Ax':str(Ax),'Ahat':str(Ah),'E2':str(E2),'W':str(fast['W']),
               'raw_date':fast['date'].text(),'T_half':str(fast['T_half']),
               'profile':[[v,str(c),str(port[v][1]),str(port[v][2])] for v,c in cv.items()],
               'all_profile':[[v,str(c)] for v,c in D.cov[x].items()]})
 heights={}; baseline_heights={}
 for x,y in itertools.combinations(range(len(P)),2):
  C=D.lca(rows[x]['owner'],rows[y]['owner'])
  heights[(x,y)]=g['maximum']([rows[x]['date_q'],rows[y]['date_q'],QS.sqrt(D.T.birth[C])])
  B=D.lca(baseline.h.proprietaires[x],baseline.h.proprietaires[y])
  baseline_heights[(x,y)]=g['maximum']([baseline.h.dates[x].qs,baseline.h.dates[y].qs,QS.sqrt(D.T.birth[B])])
 # Independent actual-point core census at every critical squared radius, not FULL covered-label count.
 ac,Ac=D.admissibilite_coeur(mcs)
 expected=[None]*D.nn
 for s in sorted(set(D.T.birth+D.dk2)):
  counts={}
  for y in range(len(P)):
   if D.dk2[y]<=s:
    C=D.anc(D.coeur[y],s); counts[C]=counts.get(C,0)+1
  for C,count in counts.items():
   if count>=mcs and expected[C] is None: expected[C]=s
 require(ac==expected,'independent real-point core census'); checks+=D.nn
 return rows,heights,checks,D,baseline_heights

def run():
 out=[]; grouped={}; total=0
 for variant in ('noyau','amas'):
  for e in (F(-1,128),F(0),F(1,128),F(1,1024),F(1,8192)):
   P=((F(0),F(0)),(F(10),F(0)),(F(211,10)-e,F(0)),(F(-11),F(36,5)),(F(-11),F(-36,5)))
   rows,heights,checks,D,baseline_heights=evaluate(P,3,variant)
   # Geometric plateau and core-threshold formulas; no empirical convergence claim needed.
   target=(F(111,10)-e)**2
   if variant=='noyau':
    require(rows[0]['rc']==str(target),'analytic core threshold')
    require(heights[(0,1)].cmp(QS.sqrt(F(12321,100)) if e<=0 else QS.sqrt((F(211,10)-e)**2/4))==0,'analytic pair-height jump')
    require(baseline_heights[(0,1)].cmp(QS.sqrt(F(12321,100)))==0,'full MMtA positive control')
    require(rows[0]['Ax']=='25','stable unfiltered and restricted anchor')
    rival_ports=sum(F(c)==F(4321,100) for _v,c,_a,_om in rows[0]['profile'])
    require(rival_ports==(0 if e>0 else 2),'hard mask genuinely exercised')
    checks+=5
   if e==0 and variant=='noyau':
    q=F(4321,100); gg=F(18671041,302500); J=F(12321,100); A0=F(44521,400); E=gg+F(25,2)
    omega=(J-A0)/F(25,2); L=omega*(E-25); G=2*(gg-q)+(E-gg); mu=(G-L)/(G+L)
    oracle=F(111,10)-15*mu
    require(rows[0]['date_q'].cmp(QS.rat(oracle))==0 and oracle==F(15588789463113,1458264845330),'independent exact plateau date')
    require(F(rows[0]['W'])==L+G and mu>0,'independent branch masses')
    checks+=2
   total+=checks
   clean=[{k:v for k,v in r.items() if k!='date_q'} for r in rows]
   out.append({'fixture':'core_comparability_plateau','epsilon':str(e),'variant':variant,
    'coordinates':[[str(t) for t in p] for p in P],'rows':clean,
    'tree':{'birth':list(map(str,D.T.birth)),'parent':D.T.parent},
    'triple_MEBs':[[list(t),str(b)] for t,b in D._audit_beta.items() if len(t)==3],
    'heights':[[list(xy),q.text(),float(q)] for xy,q in heights.items()],
    'baseline_heights':[[list(xy),q.text(),float(q)] for xy,q in baseline_heights.items()]})
   grouped.setdefault(variant,{})[e]=(rows,heights)
 jumps=[]
 for variant,data in grouped.items():
  r0,h0=data[F(0)]
  for e,(rs,hs) in data.items():
   if not e: continue
   dt=g['maximum']([g['maximum']([rs[i]['date_q']-r0[i]['date_q'],r0[i]['date_q']-rs[i]['date_q']]) for i in range(len(rs))])
   dh=g['maximum']([g['maximum']([hs[xy]-h0[xy],h0[xy]-hs[xy]]) for xy in hs])
   jumps.append({'variant':variant,'epsilon':str(e),'date_change':dt.text(),'date_change_approx':float(dt),
                 'height_change':dh.text(),'height_change_approx':float(dh)})
 for name in g['PINS']: g['source'](name)
 grid=[]
 for M in (1,13,816):
  hs=[]; ds=[]
  for offset in (1,0):
   P=tuple((F(x),F(y)) for x,y in ((110*M,72*M),(210*M,72*M),(321*M-offset,72*M),(0,144*M),(0,0)))
   rows,heights,checks,D,_base=evaluate(P,3,'noyau'); total+=checks
   require(all(0<=t<2**18 for p in P for t in p),'u18 coordinates')
   hs.append(heights[(0,1)]); ds.append(rows[0]['date'])
  jump=hs[1]-hs[0]
  require(jump.cmp(QS.rat(F(11*M+1,2)))==0,'integer-grid analytic pair-height jump')
  total+=2
  grid.append({'M':M,'max_coordinate':321*M,'single_site_move':1,'height_perturbed':hs[0].text(),
               'height_plateau':hs[1].text(),'height_jump':jump.text(),'point_x_dates':ds})
 return {'scope':'actual AST mmta_cr / Donnees / weighted kernels over independent Gamma2; final Hierarchie is a data-only adapter',
         'parameters':{'K':2,'mcs':3,'kappa':'3','eta':'1/2','lambda':'1/2'},
         'cases':out,'jumps':jumps,'integer_grid_twins':grid,'checks':total,'native_calls':0,'GCP':'unused','source_pins':g['PINS']}

if __name__=='__main__': print(json.dumps(run(),sort_keys=True,indent=1))
