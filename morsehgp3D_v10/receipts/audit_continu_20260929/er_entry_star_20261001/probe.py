#!/usr/bin/env python3
"""Exact K1 line/star fixture: count actual StructureER entry membership work."""
import ast
import hashlib
import json
from bisect import bisect_right
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace

ROOT=Path(__file__).absolute().parent
PIN='99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d'
def require(c,m):
 if not c: raise RuntimeError(m)
def vector_digest(v):
 return hashlib.sha256(json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True,allow_nan=False).encode('ascii')).hexdigest()
def source():
 b=(ROOT/'er_snapshot.py').read_bytes()
 require(hashlib.sha256(b).hexdigest()==PIN,'ER source pin')
 return b
class CountedDict(dict):
 probes=0
 def __contains__(self,k):
  CountedDict.probes+=1
  return dict.__contains__(self,k)
class Tree:
 def __init__(self,n,reverse_children):
  self.birth=[F(0)]*n+[F(1,4)]
  self.parent=[n]*n+[-1]
  self.death=[F(1,4)]*n+[None]
  self.children=[[] for _ in range(n)]+[list(range(n-1,-1,-1) if reverse_children else range(n))]
  self.root=n
 def __len__(self): return len(self.birth)

module=ast.parse(source())
nodes=[n for n in module.body if isinstance(n,ast.ClassDef) and n.name=='StructureER']
require(len(nodes)==1,'AST class inventory')
env={'Fraction':F,'exiger':require,'bisect_right':bisect_right,'dict':CountedDict}
exec(compile(ast.Module(body=nodes,type_ignores=[]),'er_snapshot.py','exec'),env)

def run():
 results=[]; mutant_kills=0
 for n in (2,8,32,128,512):
  for reverse_children in (False,True):
   for root_first in (False,True):
    T=Tree(n,reverse_children)
    cover=[dict(((n,F(1,4)),(i,F(0))) if root_first else ((i,F(0)),(n,F(1,4)))) for i in range(n)]
    sc=SimpleNamespace(n=n,noms=list(map(str,range(n))),ctx=SimpleNamespace(point_id=list(range(n))),arbre_masses=lambda:(T,cover,{}))
    CountedDict.probes=0
    st=env['StructureER'](sc)
    probes=CountedDict.probes
    require(probes==n*(n+1)//2,'exact child membership census')
    require(st.entrees==[[i] for i in range(n)],'actual entry oracle')
    edits=0; fixed=[]
    for cv in st.cov:
     entries=set(cv)
     for u in cv:
      entries.discard(T.parent[u]); edits+=1
     fixed.append(sorted(entries))
    require(fixed==st.entrees and edits==2*n,'parent-discard linear oracle')
    # Causal semantic mutant: removing the node itself instead of its parent.
    broken=[]
    for cv in st.cov:
     entries=set(cv)
     for u in cv: entries.discard(u)
     broken.append(sorted(entries))
    require(broken!=st.entrees,'discard-self mutant survived'); mutant_kills+=1
    results.append({'n':n,'H':n+1,'D':2*n,'root_first':root_first,'reverse_children':reverse_children,
                    'actual_child_memberships':probes,'parent_discard_operations':edits,'entry_lists_equal':True,
                    'entry_source_sha256':vector_digest(st.entrees),'entry_parent_discard_sha256':vector_digest(fixed),
                    'entry_mutant_self_sha256':vector_digest(broken)})
 source()
 return {'scope':'actual AST StructureER.__init__, exact Gamma1 star data of n integer collinear sites; no native engine/ER rule execution',
         'source_sha256':PIN,'executed_cases':results,'causal_discard_self_mutants_rejected':mutant_kills,
         'large_n_analytic_only':[{'n':n,'H':n+1,'D':2*n,'child_memberships':n*(n+1)//2,'parent_discard_operations':2*n} for n in (8000,16000,32000)],
         'alpha2':0,'positive_anchor_ER_K1_rule':'not executed / outside positive-anchor domain',
         'GCP':'unused','native_calls':0}
if __name__=='__main__': print(json.dumps(run(),sort_keys=True,indent=1))
