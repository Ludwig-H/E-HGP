#!/usr/bin/env python3
"""Independent exact model of a proposed q2 sign-only bound; never executes product code."""
from fractions import Fraction as F
from itertools import product
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent
CHECKS=0

def need(condition,message):
 global CHECKS
 CHECKS+=1
 if not condition:raise RuntimeError(message)

def square_distance(a,b):return sum((x-y)**2 for x,y in zip(a,b))
def sign(x):return (x>0)-(x<0)

def power(a,b,z):
 return 2*square_distance(a,z)-2*sum((y-x)*(v-x) for x,y,v in zip(a,b,z))

def q2_twice_bounds(a,b,lo,hi):
 # old q2 identity: 2*power = -4H = |2z-a-b|^2-|b-a|^2.
 diameter=square_distance(a,b);lower=-diameter;upper=-diameter
 for x,y,l,h in zip(a,b,lo,hi):
  c=x+y;ll=2*l-c;hh=2*h-c
  lower+=ll*ll if ll>0 else hh*hh if hh<0 else 0
  upper+=max(ll*ll,hh*hh)
 return lower,upper

def separated_bounds(a,b,lo,hi):
 norm_low=norm_high=0;linear_low=linear_high=0
 for x,y,l,h in zip(a,b,lo,hi):
  ll=l-x;hh=h-x;n=y-x
  norm_low+=ll*ll if ll>0 else hh*hh if hh<0 else 0
  norm_high+=max(ll*ll,hh*hh)
  linear_low+=n*(-2*(hh if n>=0 else ll))
  linear_high+=n*(-2*(ll if n>=0 else hh))
 return 2*norm_low+linear_low,2*norm_high+linear_high

def morton(p):return sum(((p[j]>>bit)&1)<<(3*bit+j) for bit in range(5) for j in range(3))

def index_nodes(points,leaf_size):
 nodes=[]
 def visit(begin,end):
  at=len(nodes);nodes.append(None)
  if end-begin>leaf_size:
   middle=begin+(end-begin)//2;visit(begin,middle);visit(middle,end)
  lo=tuple(min(p[j] for p in points[begin:end]) for j in range(3))
  hi=tuple(max(p[j] for p in points[begin:end]) for j in range(3))
  nodes[at]=(begin,end,len(nodes),lo,hi)
  return at
 visit(0,len(points));return nodes

def census(points,nodes,a,b,threshold,leaf_size,coupled):
 interior=[];shell=[];cursor=0;bounds=tests=inside=outside=0
 while cursor<len(nodes) and len(interior)<threshold:
  begin,end,escape,lo,hi=nodes[cursor];bounds+=1
  lower,upper=(q2_twice_bounds if coupled else separated_bounds)(a,b,lo,hi)
  if lower>0:outside+=1;cursor=escape
  elif upper<0:
   inside+=1;interior+=list(range(begin,min(end,begin+threshold-len(interior))));cursor=escape
  elif end-begin<=leaf_size:
   for i in range(begin,end):
    if len(interior)==threshold:break
    tests+=1;v=power(a,b,points[i])
    if v<0:interior.append(i)
    elif v==0:shell.append(i)
   cursor=escape
  else:cursor+=1
 saturated=len(interior)==threshold
 return {'kind':'saturated' if saturated else 'complete','I':interior,'U':[] if saturated else shell,
         'work':{'bounds':bounds,'point_tests':tests,'inside_blocks':inside,'outside_blocks':outside}}

def main():
 for filename in ('SOURCE_BEFORE.json','SOURCE_API_BEFORE.json'):
  for path,entry in json.loads((R/filename).read_text())['files'].items():
   need(hashlib.sha256((R/entry['copy']).read_bytes()).hexdigest()==entry['sha256'],'snapshot differs '+path)
 old=(R/'source/morsehgp3D_v8/src/spindle/q2_prepared_bounds.hpp').read_text()
 cur=(R/'source/morsehgp3D_v11/src/num/predicates.cpp').read_text()
 need('distance - nearest_squared' in old and 'distance - std::max(low_squared, high_squared)' in old,'old exact q2 formula missing')
 need('terms.norm_upper += lo2 > hi2 ? lo2 : hi2;' in cur and 'sphere.numerator()[j] * terms.linear_upper[j]' in cur,'current separate bounds changed')
 old9=(R/'source/morsehgp3D_v9/src/gen/spindle/q2_prepared_bounds.hpp').read_text()
 need(old.replace('mhgp8','mhgp9::gen')==old9,'v9 q2 extrema differ beyond namespace')
 factory=(R/'source/morsehgp3D_v11/src/num/sphere.cpp').read_text()
 need('Sphere(a, n, 2, level.value(), 2,' in factory,'q2 presentation no longer has denominator2 and difference numerator')
 api=(R/'source/morsehgp3D_v11/src/num/geometry.hpp').read_text()
 gate=(R/'source/morsehgp3D_v11/tests/num/bounds_test.cpp').read_text()
 need('Signes (-1,0,1) des deux bornes de power_bounds, memes refus.' in api,
      'existing public signs no longer tied to quantitative bounds')
 need('expected{{{0, 1}, {-1, -1}, {-1, 1}}}' in gate,
      'existing gate no longer pins loose-bound signs')
 spheres=[((0,0,0),(5,0,0)),((0,0,0),(4,4,0)),((1,2,0),(4,0,3)),((2,1,4),(0,4,1))]
 boxcases=[((0,0,0),(4,4,4)),((1,0,2),(3,2,4)),((0,0,0),(0,4,2)),((1,1,1),(1,1,1)),((2,2,2),(4,4,4))]
 for a,b in spheres:
  for lo,hi in boxcases:
   lb,ub=q2_twice_bounds(a,b,lo,hi);slo,shi=separated_bounds(a,b,lo,hi)
   nearest=tuple(min(F(h),max(F(l),F(x+y,2))) for x,y,l,h in zip(a,b,lo,hi))
   need(2*power(a,b,nearest)==lb,'lower not exact continuous minimum')
   need(max(2*power(a,b,z) for z in product(*zip(lo,hi)))==ub,'upper not exact continuous maximum')
   need(2*slo<=lb<=ub<=2*shi,'coupled interval is not contained in current bound')
   grid=[sorted(set((F(l),F(h),F(l+h,2),nearest[j]))) for j,(l,h) in enumerate(zip(lo,hi))]
   for z in product(*grid):need(lb<=2*power(a,b,z)<=ub,'point outside proved bound')
 rows=[]
 for bits in (18,21,24):
  m=2**bits-1;limit=12*m*m
  need(limit<2**(2*bits+4)<2**63,'q2 signs require wider than i64')
  a=(0,0,0);b=(m,m,m);lo=(0,0,0);hi=(m,m,m)
  lb,ub=q2_twice_bounds(a,b,lo,hi)
  need(lb==-3*m*m and ub==0,'profile contact/center enclosure wrong')
  rows.append({'bits':bits,'q2_twice_power_abs_bound':limit,'bound_bit_length':limit.bit_length(),'center_sum_max':2*m,'closed_root_twice_bounds':[lb,ub]})
 # Actual integer input, complete shell on axis and perpendicular diameter directions.
 a=(1,6,6);b=(11,6,6)
 points=sorted([a,b]+[(x,6,6) for x in range(2,11)]+[(6,1,6),(6,11,6),(6,6,1),(6,6,11)]+[(0,6,6),(12,6,6)],key=morton)
 need(len(points)==len(set(points))==17,'distinct cloud changed')
 nodes=index_nodes(points,2);comparisons=[]
 for threshold in (2,9,10,18):
  old_result=census(points,nodes,a,b,threshold,2,False)
  new_result=census(points,nodes,a,b,threshold,2,True)
  for field in ('kind','I','U'):need(old_result[field]==new_result[field],'census payload changed '+field)
  true_i=[i for i,p in enumerate(points) if power(a,b,p)<0]
  true_u=[i for i,p in enumerate(points) if power(a,b,p)==0]
  need(new_result['I']==true_i[:threshold],'interior differs from scan')
  need(new_result['U']==([] if len(true_i)>=threshold else true_u),'full shell or saturated empty-shell differs')
  comparisons.append({'threshold':threshold,'current':old_result,'coupled_model':new_result})
 need(len(true_i)==9 and len(true_u)==6,'fixture incidence count wrong')
 target_lo=(2,6,6);target_hi=(10,6,6)
 oldlb,oldub=separated_bounds(a,b,target_lo,target_hi);newlb,newub=q2_twice_bounds(a,b,target_lo,target_hi)
 need(oldub==142 and newub==-36,'causal inside-block witness changed')
 contact=q2_twice_bounds(a,b,a,(10,6,6))
 need(contact[1]==0,'shell contact was excluded by bound')
 # Quantitative power is half the sign-only polynomial: never publish 2power as PowerBounds.
 half_case=q2_twice_bounds((0,0,0),(1,0,0),(0,0,0),(1,0,0))
 need(half_case==(-1,0) and F(half_case[0],2)==F(-1,2),'quantitative unit distinction lost')
 public_case=separated_bounds((0,0,0),(4,0,0),(0,0,0),(4,0,0))
 tight_case=q2_twice_bounds((0,0,0),(4,0,0),(0,0,0),(4,0,0))
 need(public_case==(-32,32) and tight_case==(-16,0),'public contract counter-guard changed')
 need(tuple(map(sign,public_case))==(-1,1) and tuple(map(sign,tight_case))==(-1,0),
      'coupled signs would change an existing public contract')
 output={'status':'PASS','checks':CHECKS,'pin':json.loads((R/'SOURCE_BEFORE.json').read_text())['pin'],
         'proposal':'new explicit census-only helper/preparer, presentation tag2; existing public power_bounds and power_bound_signs unchanged',
         'profile_bounds':rows,'case17':{'points':points,'a':a,'b':b,'leaf_size':2,'nodes':len(nodes),'censuses':comparisons},
         'causal_box':{'lo':target_lo,'hi':target_hi,'current_power_bounds':[oldlb,oldub],'proposed_twice_power_bounds':[newlb,newub]},
         'shell_contact_twice_bounds':contact,'public_quantitative_unit_guard':{'twice_power_lower':half_case[0],'power_lower':str(F(half_case[0],2))},
         'public_api_guard':{'existing_power_bounds':public_case,'new_census_twice_bounds':tight_case,
                             'existing_public_signs':tuple(map(sign,public_case)),
                             'new_census_signs':tuple(map(sign,tight_case))},
         'scope':'independent exact model of native median-index DFS and proposed sign bound; no product execution/performance qualification',
         'native_runs':0,'fits':0,'gcp_actions':0}
 print(json.dumps(output,indent=2,sort_keys=True))

if __name__=='__main__':main()
