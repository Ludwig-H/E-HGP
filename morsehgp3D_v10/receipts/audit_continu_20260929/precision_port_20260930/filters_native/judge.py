"""Independent Fraction judge of three real closed_ball expected failures.
Code 0 confirms the unchanged unsupported-domain filter misclassifies shell.
No copied approximate formulas or geometry body is used for exact judgement.
"""
from fractions import Fraction
import json
import sys

FIXTURES = [
 ("u24_reduced_false_interior",24,[(0,0,0),(15000005,15000005,0),(15000005,0,15000005)],(30000010,15000005,15000005),3,"false_interior"),
 ("u24_reduced_lost",24,[(15000010,0,15000010),(15000010,15000010,0),(0,0,0)],(-15000010,15000010,-30000020),3,"lost_shell"),
 ("u32_translated_reduced_lost",32,[(4000000000,4000000000,4000000000),(4000200005,4000000000,4000000000),(4000040001,4000200005,4000000000)],(1000025,840021,0),10,"lost_shell")
]
checks=0
def require(ok,label):
 global checks
 checks+=1
 if not ok: raise ValueError(label)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def morton(p):
 return sum(((p[a]>>b)&1)<<(3*b+a) for b in range(32) for a in range(3))
def judge(data):
 require(data["schema"]=="mhgp10_site_filter_native_failure_probe_v1","schema")
 require(data["scope"]=="unchanged_u18_bodies_outside_contract","scope")
 require(data["status"]=="EXPECTED_FAILURES_CONFIRMED","native gate status")
 require(data["gate_failures"]==0 and data["key_controls"]==9 and data["expected_failures_confirmed"]==3,"native floors")
 require(len(data["cases"])==3,"three cases")
 facts=[]
 for actual,(name,bits,points,N,D,failure) in zip(data["cases"],FIXTURES):
  require(actual["name"]==name and actual["bits_tag"]==bits,"fixture identity")
  require(actual["points_fixture_order"]==[list(p)for p in points],"exact input points")
  require(actual["N"]==[str(n)for n in N] and actual["D"]==str(D),"reduced center")
  require(actual["cloud_construction"]=="direct_private_outside_u18_factory","scope direct construction")
  anchor=points[0]
  center=tuple(Fraction(anchor[i])+Fraction(N[i],D) for i in range(3))
  radius=sum((center[i]-anchor[i])**2 for i in range(3))
  require(all(sum((Fraction(p[i])-center[i])**2 for i in range(3))==radius for p in points),"all three exact shell")
  u,v=sub(points[1],anchor),sub(points[2],anchor)
  uu,vv,uv=dot(u,u),dot(v,v),dot(u,v)
  determinant=uu*vv-uv*uv
  require(determinant>0,"noncollinear Gram matrix")
  l1=Fraction(uu*vv-uv*vv,2*determinant)
  l2=Fraction(uu*vv-uv*uu,2*determinant)
  weights=(1-l1-l2,l1,l2)
  require(all(w>0 for w in weights),"strictly positive barycentrics")
  independently_solved=tuple(Fraction(anchor[i])+l1*u[i]+l2*v[i]for i in range(3))
  require(independently_solved==center,"Gram solved center equals reduced encoding")
  require(all(0<=x<(1<<bits)for p in points for x in p),"u24/u32 coordinates")
  require(any(x>262143 for p in points for x in p),"outside supported u18 domain")
  order=sorted(range(3),key=lambda i:morton(points[i]))
  require(actual["site_original_fixture_ids"]==order,"Morton96 canonical fixture ID order")
  target=order.index(2)
  require(actual["target_site"]==target,"target mapping")
  require(actual["exact_side_keys"]==["0","0","0"],"native exact sides control")
  I,S=actual["interior"],actual["shell"]
  for values in (I,S):
   require(type(values)is list and all(type(x)is int and 0<=x<3 for x in values),"native indices")
   require(values==sorted(set(values)),"canonical native list")
  require(not(set(I)&set(S)),"interior-shell disjoint")
  require((I,S)!=([],[0,1,2]),"real native filter disagrees with exact census")
  require(actual["expected_target_failure"]==failure and actual["failure_reproduced"]is True,"native target annotation")
  require((target in I and target not in S) if failure=="false_interior"else(target not in I and target not in S),"exact target mismatch")
  maximum=0
  for p in points:
   w=sub(p,anchor)
   maximum=max(maximum,D*dot(w,w),2*sum(abs(n*x)for n,x in zip(N,w)))
  require(maximum<(1<<127),"exact side intermediates safe")
  facts.append(dict(name=name,radius_squared=str(radius),barycentric=[str(w)for w in weights],safe_side_intermediate_bits=maximum.bit_length(),expected_interior=[],expected_shell=[0,1,2],native_interior=I,native_shell=S,target_failure=failure))
 require(checks>=65,"judge non-vacuity")
 print(json.dumps(dict(status="EXPECTED_FAILURES_CONFIRMED",checks=checks,cases=facts),sort_keys=True))
 return 0

if __name__=="__main__":
 try:
  if len(sys.argv)!=2:raise ValueError("usage: judge.py NATIVE_JSON")
  raise SystemExit(judge(json.loads(sys.argv[1])))
 except (ValueError,KeyError,TypeError,ZeroDivisionError)as error:
  print(json.dumps(dict(status="FAIL",checks=checks,error=str(error)),sort_keys=True))
  raise SystemExit(1)
