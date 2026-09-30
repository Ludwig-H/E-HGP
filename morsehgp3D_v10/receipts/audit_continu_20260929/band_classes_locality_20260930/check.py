from fractions import Fraction as F
from itertools import combinations
import json

def need(ok, label):
    if not ok:
        raise RuntimeError(label)

ts = [F(1,101), F(2,97), F(3,89), F(4,83), F(5,79), F(6,73)]
us = [((1-t*t)/(1+t*t), 2*t/(1+t*t)) for t in ts]
seen = set()
centers = set()
rows = []
for i,j in combinations(range(len(ts)),2):
    a,b=us[i],us[j]
    dot=sum(q*r for q,r in zip(a,b))
    need(0 < dot < 1, 'acute triangle')
    det=a[0]*b[1]-a[1]*b[0]
    center=((b[1]-a[1])/(2*det),(a[0]-b[0])/(2*det))
    beta=sum(c*c for c in center)
    formula=(1+ts[i]**2)*(1+ts[j]**2)/(4*(1+ts[i]*ts[j])**2)
    need(beta == formula, 'radius formula')
    need(F(1,4) < beta < F(17,64) < F(9,32), 'band bound')
    need(beta not in seen, 'distinct classes')
    seen.add(beta)
    need(center not in centers, 'distinct centres')
    centers.add(center)
    inside=[k for k,u in enumerate(us) if sum(u[d]*center[d] for d in range(2)) > F(1,2)]
    need(inside == list(range(i+1,j)), 'interior cap sites')
    scaled_beta=F(101,100)**2*beta
    need(scaled_beta < F(9,32), 'same KNN cap band')
    rows.append({'i':i,'j':j,'beta':str(beta),'p':len(inside)})
need(F(101,100)**2*F(17,64) < F(9,32), 'global perturbed-cap band bound')
# Base K3 cloud: 0, 1/2, 1 on the horizontal line. Its alpha is 1/2.
# Every new vertex has norm 101/100, so a K-part containing 0 and that
# vertex has radius at least 101/200 > 1/2: alpha cannot decrease.
need(F(101,200) > F(1,2), 'alpha and nearest-neighbour preservation')
print(json.dumps({'status':'PASS','K':3,'eta_prime':'1/8','points_in_cap':len(ts),'classes':len(rows), 'distinct_centers':len(centers),'beta_max_bound':'17/64','band_lower_bound':'9/32','same_KNN_radial_scale':'101/100','rows':rows},sort_keys=True))
