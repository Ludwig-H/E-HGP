# Necessary condition for a dead q3/q4 lane on edge ab at K: the diametral
# ball of ab (center m, which lies in every admissible centre disk) must hold
# >= K-1 (q3) or >= K-2 (q4) sites strictly inside. Exact integers (x4).
from itertools import combinations
fixtures = {
 'line12': [(2*j+(j%2),7,9) for j in range(12)],
 'shell14': [(15,10,10),(5,10,10),(10,15,10),(10,5,10),(10,10,15),(10,10,5),(13,14,10),(13,6,10),(7,14,10),(7,6,10),(10,13,14),(10,7,6),(10,10,10),(20,25,30)],
 'spatial12': [(7,42,83),(91,12,64),(33,88,9),(54,20,71),(18,61,39),(76,53,95),(42,7,24),(62,94,47),(3,29,58),(85,73,15),(29,36,97),(58,65,3)],
}
def depth(a,b,pts):
    # strictly inside ball with diameter ab  <=>  (z-a).(z-b) < 0
    return sum(1 for z in pts if z not in (a,b) and sum((z[i]-a[i])*(z[i]-b[i]) for i in range(3))<0)
for name,pts in fixtures.items():
    ds=[depth(a,b,pts) for a,b in combinations(pts,2)]
    for K in (3,5,10):
        q3=sum(1 for d in ds if d>=K-1); q4=sum(1 for d in ds if d>=K-2)
        print(f"{name:10s} n={len(pts):2d} K={K:2d} pairs={len(ds)} max_diam_depth={max(ds)} pairs_diam_depth>=K-1:{q3} >=K-2:{q4}")
