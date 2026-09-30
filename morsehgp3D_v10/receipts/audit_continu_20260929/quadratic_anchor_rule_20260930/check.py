"""Tiny exact scalar counterreview of the conditional Qκ proof."""
from fractions import Fraction as F
from pathlib import Path
import hashlib, json
ROOT=Path(__file__).resolve().parent
paths=[Path(__file__).resolve(),ROOT/"PROOF.md"]
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
before={p.name:sha(p) for p in paths}
def need(ok,msg):
    if not ok: raise RuntimeError(msg)
def root_le_difference(X,Y,L):
    # sqrt(X) <= sqrt(Y)+L, exact for X,Y,L>=0.
    z=X-Y-L*L
    return z<=0 or z*z<=4*L*L*Y
def cutoff(k,A,B):
    v=(k-1)*B-k*A
    return v<=0 or v*v<=4*A*B
counts={"scalar_cases":0,"inequalities":0,"paired_fixtures":2}
for k in (2,3,8):
    Cbar=F(2*k*(k+1),k-1)
    for a in map(F,(0,F(1,8),1,4)):
        for e in map(F,(0,F(1,16),F(1,2),4)):
            for b in (max(F(0),a-e),a,a+e):
                for t in map(F,(1,F(5,4),2,4)):
                    r=a*t
                    for t_m in map(F,(0,F(1,2),1)):
                        m=(r+a)*t_m
                        A=a*a; B=r*r
                        candidate=m*m-k*(B-A)
                        X=max(A,candidate)
                        y=max(F(0),m-e)
                        Y=max(b*b,y*y-k*((r+e)**2-b*b))
                        need(X<=4*A,"L6 parabola bound failed")
                        exact_cutoff=(k-1)*r*r-2*a*r-k*A<=0
                        need(cutoff(k,A,B)==exact_cutoff,"squared cutoff failed")
                        if not cutoff(k,A,B):
                            need(candidate<=A,"excluded candidate improves Q")
                        need(root_le_difference(X,Y,Cbar*e),"global Cbar date bound failed")
                        if candidate>A and e<=a:
                            need(m>=e,"winning candidate positivity missing")
                            need(X-Y<=2*Cbar*a*e-e*e,"scalar stability numerator failed")
                        # MY(a+e)^2 upper from QY; b<=a+e<=b+2e.
                        upper=Y+k*((a+e)**2-b*b)
                        need(root_le_difference(upper,Y,2*k*e),"owner compatibility failed")
                        pcand=m-k*(r-a); P=max(a,pcand)
                        need(X>=P*P,"Q is not >= P")
                        counts["scalar_cases"]+=1
                        counts["inequalities"]+=7
# Concrete pair: no competing cohort; and an admissible late competing cohort.
k=2;a=F(1);r=F(3,2);m=F(5,2)
need(max(a,m-k*(r-a))==F(3,2),"P fixture wrong")
need(max(a*a,m*m-k*(r*r-a*a))==F(15,4),"Q fixture wrong")
need(F(15,4)>F(9,4),"P/Q distinction vanished")
need(max(a*a,a*a-k*(a*a-a*a))==a*a,"baseline changed")
for k in range(2,9):
    need(1+k*(k-1)<k*k,"rational q upper bound fails")
    need((2*k+1)*(1<<666)<(1<<671),"candidate numerator bound fails")
need(671+600==1271 and 20*64>=1271,"Q/Q product width wrong")
need(671+200==871 and 14*64>=871,"Q/level product width wrong")
after={p.name:sha(p) for p in paths}
need(before==after,"sources changed")
print(json.dumps({"status":"SCALAR_PASS","counts":counts,"pins_before":before,
"pins_after":after,"native_executions":0,"GCP_used":False,
"limits":"Conditional L6/interleaving scalar proof only; no geometric interleaving, point owners, engine, statistics or D bound qualified."},sort_keys=True))
