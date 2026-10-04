#!/usr/bin/env python3
"""Modèle Γ2 exact 1D : dates seules, cellule fermée et mémo de partie.
Aucun moteur/oracle produit importé. Toutes les dates sont des rayons carrés.
"""
from fractions import Fraction as F
from itertools import combinations
import json
CHECKS=0

def need(v,message):
 global CHECKS
 CHECKS+=1
 if not v:raise ValueError(message)

def beta(x,s):return F((max(x[i] for i in s)-min(x[i] for i in s))**2,4)

def gamma(x,a,closed=True):
 active=[s for s in combinations(range(len(x)),2) if (beta(x,s)<=a if closed else beta(x,s)<a)]
 parent=list(range(len(active)))
 def find(i):
  while parent[i]!=i:i=parent[i]
  return i
 for i,s in enumerate(active):
  for j,t in enumerate(active[:i]):
   if len(set(s)&set(t))==1 and (beta(x,set(s)|set(t))<=a if closed else beta(x,set(s)|set(t))<a):parent[find(i)]=find(j)
 return {s:find(i) for i,s in enumerate(active)}

def same(g,a,b):return a in g and b in g and g[a]==g[b]

def main():
 x=(0,2,4); left=(0,1);right=(1,2);outer=(0,2)
 for cut in [F(1),F(2),F(3),F(4)]:
  g=gamma(x,cut);need(left in g and right in g,'both seeds born');need(same(g,left,right)==(cut>=4),'closed merge at4')
 need(not same(gamma(x,F(4),False),left,right),'open4 remains split')
 for cut in [F(4),F(5),F(9),F(100)]:
  for s in combinations(range(3),2):need(same(gamma(x,cut),s,left),'whole cell shares closed component from4')
 y=(0,2,10,12);a=(0,1);b=(2,3)
 need(beta(y,a)==beta(y,b)==1,'same timestamp for different cells')
 for cut in [F(1),F(2),F(4),F(16),F(24)]:need(not same(gamma(y,cut),a,b),'timestamp1 does not identify component')
 need(same(gamma(y,F(25)),a,b),'two components meet at25')
 z=(0,2,4,6);origin=(0,3);terminal=(1,2)
 need(beta(z,origin)==9 and beta(z,terminal)==1,'distinct validity and terminal dates')
 need(terminal in gamma(z,F(4)) and origin not in gamma(z,F(4)),'terminal birth does not activate origin')
 print(json.dumps(dict(checks=CHECKS,scheme='independent1DΓ2Fraction',witnesses={'pieces_before_cell':dict(X=x,cell_beta=4,terminal_beta=1,closed4_connected=True,open4_connected=False),'same_date_different_cells':dict(X=y,cell_beta=1,merge_beta=25),'invalid_terminal_date':dict(X=z,initial_beta=9,terminal_beta=1,cut=4)},scope='mathematical certificates only; no native cache/performance qualification'),indent=2,sort_keys=True))
if __name__=='__main__':main()
