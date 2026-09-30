import sys, itertools
from fractions import Fraction as F
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C, measure as M
P = [(0,0,7),(0,9,6),(1,4,0),(0,0,1),(4,1,2)]
L = 'ABCDE'
def sub(a,b): return tuple(F(x)-F(y) for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def d2(a,b): v=sub(a,b); return dot(v,v)
def meb(ids):
    pts=[P[i] for i in ids]
    if len(pts)==2:
        c=tuple((F(x)+F(y))/2 for x,y in zip(*pts)); return c, d2(c,pts[0])
    # triangle: check obtuse
    best=None
    for i in range(3):
        a,b,o=pts[(i+1)%3],pts[(i+2)%3],pts[i]
        c=tuple((F(x)+F(y))/2 for x,y in zip(a,b)); r=d2(c,a)
        if d2(c,o)<=r:
            if best is None or r<best[1]: best=(c,r)
    if best: return best
    a,b,c=pts; u=sub(b,a); v=sub(c,a)
    uu,uv,vv=dot(u,u),dot(u,v),dot(v,v); det=uu*vv-uv*uv
    s=(vv*uu-uv*vv)/(2*det); t=(uu*vv-uv*uu)/(2*det)
    cen=tuple(F(a[k])+s*u[k]+t*v[k] for k in range(3)); return cen, d2(cen,a)
def gabriel(ids):
    c,r=meb(ids); return all(d2(c,P[j])>r for j in range(5) if j not in ids), r
cof=[]; gab={}
for t in itertools.combinations(range(5),3):
    g,r=gabriel(t); print(''.join(L[i] for i in t), r, 'G' if g else '-')
    if g: cof.append((t,r))
for e in itertools.combinations(range(5),2):
    g,r=gabriel(e)
    if g: gab[e]=r
print('gabriel pairs', [''.join(L[i] for i in e) for e in gab])
for conv in ('boundary','gabriel'):
    births=M.facet_births(cof,gab,conv)
    keep=None if conv=='boundary' else gab
    facets,plateaus=C.facet_levels(cof,keep)
    nodes,roots=C.merge_tree(facets,plateaus,births)
    print('==',conv,'AC birth', births.get((0,2)))
    for name,nd in sorted(nodes.items(), key=lambda kv: kv[1]['level']):
        print(' node',name,nd['level'],[c if isinstance(c,str) else ''.join(L[i] for i in c) for c in nd['children']])
