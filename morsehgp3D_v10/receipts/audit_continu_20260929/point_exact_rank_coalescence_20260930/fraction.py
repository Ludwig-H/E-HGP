from fractions import Fraction as F
import json, math

M=511
L=M*M
base=F(L*L,4)
delta=F(1,4*M*M)
high=base+delta
weights=[F(1,2)-F(1,2*M*M)+F(1,2*M**4), F(1,2)-F(1,2*M**4), F(1,2*M*M)]
if not (sum(weights)==1 and all(w>0 for w in weights) and base<high and float(base)==float(high) and delta<F.from_float(math.ulp(float(base)))/2):
    raise SystemExit("fraction failed")
print(json.dumps(dict(M=M,L=L,coordinates=[[0,0,0],[L,0,0],[1,M,0]],base=str(base),high=str(high),delta=str(delta),ulp=str(F.from_float(math.ulp(float(base)))),weights=list(map(str,weights)),base_hex=float(base).hex(),high_hex=float(high).hex(),strict_positive=True,within_u18=L<2**18)))
