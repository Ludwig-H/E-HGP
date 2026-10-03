#!/usr/bin/env python3
"""Pure Fraction binary64 oracle, not native FENV or product execution."""
from fractions import Fraction as F
from itertools import product
import json
checks=0
modes=('nearest_even','upward','downward','toward_zero')
u=F(1,1<<52); c=1-F(1,1<<40)
def need(x,msg):
 global checks
 checks+=1
 if not x:raise RuntimeError(msg)
def pow2(e):return F(1<<e) if e>=0 else F(1,1<<(-e))
def rounded(x,mode):
 if not x:return F(0)
 need(x>0,'positive normal model')
 e=x.numerator.bit_length()-x.denominator.bit_length()
 if x<pow2(e):e-=1
 need(-1022<=e<=1023,'normal finite result')
 scaled=x/pow2(e-52);q,r=divmod(scaled.numerator,scaled.denominator)
 if mode=='upward' and r:q+=1
 elif mode=='nearest_even' and (2*r>scaled.denominator or (2*r==scaled.denominator and q%2)):q+=1
 return F(q)*pow2(e-52)
def magnitude(n,mode):
 length=n.bit_length()
 if length<=64:return rounded(F(n),mode)
 shift=length-64;word,bit=divmod(shift,64)
 words=[(n>>(64*i))&((1<<64)-1) for i in range((length+63)//64)]
 top=words[word]>>bit
 if bit:
  need(word+1<len(words),'cross-word high read in bounds')
  top|=(words[word+1]<<(64-bit))&((1<<64)-1)
 need(top==n>>shift,'C++ head extraction exact')
 return rounded(F(top),mode)*pow2(shift)
def key(n,d,mode):return rounded(magnitude(n,mode)/magnitude(d,mode),mode)
def less(a,b,ka,kb,mode):
 if ka<rounded(c*kb,mode):return True,'fast_left'
 if kb<rounded(c*ka,mode):return False,'fast_right'
 return a<b,'exact'
need(c<=(1-u)**13,'F4 margin E6+E6+product')
rows=[];fast=0;fallback=0
for B in (18,21,24):
 N,D=8*B+12,6*B+8
 levels=[(0,1),(1,1),(3,3),(1,(1<<D)-1),((1<<N)-1,1),((1<<53)-1,1<<53),((1<<53)+1,1<<53),((1<<100)-(1<<60)-1,1<<100),((1<<100)-(1<<60),1<<100),((1<<100)-(1<<60)+1,1<<100)]
 for length in (63,64,65,127,128,129,155,179,191,192,193,204):
  if length<=N:levels.append(((1<<length)-1,(1<<min(D,length-1))+1))
 # Equal exact values with distinct non-power-of-two presentations, inside all budgets.
 levels.extend([((1<<(N-3))+7,(1<<(D-3))+3),(3*((1<<(N-3))+7),3*((1<<(D-3))+3))])
 for n,d in levels:
  need(0<=n<(1<<N) and 0<d<(1<<D),'Level domain')
  for mode in modes:
   k=key(n,d,mode);x=F(n,d)
   if x:need((1-u)**6<=k/x<=(1-u)**(-6),'F3 E6 enclosure')
   else:need(k==0,'zero exact')
 pairs=[(levels[i],levels[j]) for i,j in [(0,1),(1,2),(5,6),(7,1),(8,1),(9,1),(len(levels)-2,len(levels)-1)]]
 pairs.extend((levels[i],levels[i+1]) for i in range(len(levels)-1))
 for a,b in pairs:
  for ma,mb,mc in product(modes,repeat=3):
   ka=key(*a,ma);kb=key(*b,mb);xa,xb=F(*a),F(*b)
   actual,branch=less(xa,xb,ka,kb,mc)
   need(actual==(xa<xb),'filtered comparator agrees with exact order')
   reverse,_=less(xb,xa,kb,ka,mc)
   need(not(actual and reverse),'asymmetry')
   if branch.startswith('fast'):fast+=1
   else:fallback+=1
 rows.append({'B':B,'levels':len(levels),'pairs':len(pairs),'all_key_and_comparison_modes':64})
# A weaker c=1 can incorrectly order equal exact levels whose keys were rounded on different workers.
a=F((1<<53)+1,1<<53);down=key(a.numerator,a.denominator,'downward');up=key(a.numerator,a.denominator,'upward')
need(down<up and a==a,'mixed-rounding equal exact witness')
need(not(down<rounded(c*up,'upward')),'current margin rejects false order')
print(json.dumps({'status':'PASS','scope':'independent exact binary64 rounding model, no compiled C++/FENV test','checks':checks,'profiles':rows,'fast_branches':fast,'exact_fallbacks':fallback,'E':6,'margin':str(c),'safe_range':'positive levels >=2^-152 and <=2^204, all intermediates finite normal; zero exact','strict_weak_order_proof':'each certified branch agrees with exact level comparison; exact fallback compares (level,S*,ordinal), so the entire comparator is the same strict total order','adversary':{'level':str(a),'key_down':str(down),'key_up':str(up),'unsafe_margin_1_would_order_equal_levels':True}},sort_keys=True,indent=2))
