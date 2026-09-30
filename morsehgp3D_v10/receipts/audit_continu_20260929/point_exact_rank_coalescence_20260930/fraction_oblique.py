from fractions import Fraction as F
import json, math

M,L,t=512,261120,2
D2=L*L+t*t
V2=1+M*M
dot=L+t*M
cross=L*M-t
power=V2-dot
base=F(D2,4)
delta=F(power*power*D2,4*cross*cross)
high=base+delta
cz=F(D2*power,2*cross*cross)
cb=F(V2*(D2-dot),2*cross*cross)
ca=1-cb-cz
cx,cy=cb*L+cz,cb*t+cz*M
if not (power==1 and cross!=0 and D2>dot>0 and all(w>0 for w in (ca,cb,cz)) and ca+cb+cz==1 and cx*cx+cy*cy==high and base<high and max(L,t,M)<2**18):
    raise SystemExit("fraction oblique failed")
print(json.dumps(dict(M=M,L=L,t=t,coordinates=[[0,0,0],[L,t,0],[1,M,0]],D2=D2,cross=cross,power=power,base=str(base),high=str(high),delta=str(delta),center=[str(cx),str(cy),"0"],weights=[str(ca),str(cb),str(cz)],ulp=str(F.from_float(math.ulp(float(base)))),half_ulp=str(F.from_float(math.ulp(float(base)))/2),correct_rounding_base_hex=float(base).hex(),correct_rounding_high_hex=float(high).hex(),strict_positive=True,within_u18=True)))
