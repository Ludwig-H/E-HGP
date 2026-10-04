#!/usr/bin/env python3
"""Independent eight-corner distance model, without the source G1 algebraic expression."""
from collections import Counter
from itertools import product
from pathlib import Path
import json

ROOT=Path(__file__).resolve().parent
CHECKS=0


def check(ok,message):
    global CHECKS
    CHECKS+=1
    if not ok:
        raise RuntimeError(message)


def morton(p):
    return sum(((p[j]>>b)&1)<<(3*b+j) for b in range(5) for j in range(3))


def run(points):
    k,leafsize,maxleaf=5,8,32
    nodes=0
    empty=0
    leaves=[]
    centre=points.index((8,8,8))
    check(len(points)==len(set(points))==9,'distinct nine sites')
    check(all(0<=v<2**18 for p in points for v in p),'coordinates admitted by all three profiles')
    check(1<=k<=12 and k+3<=leafsize<=maxleaf<=1024,'public parameter domains')

    def dist(p,c):
        return sum((a-b)**2 for a,b in zip(p,c))

    def visit(parent,lo,hi,depth):
        nonlocal nodes,empty
        nodes+=1
        check(all(0<=a<b<=2**18 for a,b in zip(lo,hi)),'valid T0 box')
        # Reservoir takes at most3K=15; n=9, so ALL parent sites are selected irrespective of sort ties/order.
        check(len(parent)<=3*k,'all reservoir witnesses available')
        corners=list(product(*[(a,b) for a,b in zip(lo,hi)]))
        kept=[]
        for i in parent:
            dominates=sum(min(dist(points[i],c)-dist(points[j],c) for c in corners)>0 for j in parent)
            check(0<=dominates<len(parent),'strict self never dominates')
            if dominates<k:
                kept.append(i)
        if not kept:
            empty+=1
            return
        adjusted_lo=tuple(max(lo[j],min(points[i][j] for i in kept)) for j in range(3))
        adjusted_hi=tuple(min(hi[j],max(points[i][j] for i in kept)+1) for j in range(3))
        if any(a>=b for a,b in zip(adjusted_lo,adjusted_hi)):
            empty+=1
            return
        widths=[b-a for a,b in zip(adjusted_lo,adjusted_hi)]
        # Python max preserves first axis on equal lengths, as split_ready.
        axis=max(range(3),key=lambda j:widths[j])
        if len(kept)<=leafsize or widths[axis]<=1:
            check(len(kept)<=maxleaf,'wideleaf guard passes')
            check(1<=len(kept)<=32,'deferred enqueue admissible')
            leaves.append((tuple(kept),adjusted_lo,adjusted_hi,depth))
            return
        mid=adjusted_lo[axis]+widths[axis]//2
        lh=list(adjusted_hi);lh[axis]=mid
        rl=list(adjusted_lo);rl[axis]=mid
        visit(kept,adjusted_lo,tuple(lh),depth+1)
        visit(kept,tuple(rl),adjusted_hi,depth+1)

    visit(list(range(9)),(0,0,0),(17,17,17),0)
    check(nodes==159 and len(leaves)==80 and empty==0,'exact node/leaf counts')
    check(all(centre in job[0] for job in leaves),'centre belongs to every leaf list')
    check(sum(len(job[0]) for job in leaves)==648,'overlapping site incidences')
    check(dict(Counter(len(job[0]) for job in leaves))=={8:72,9:8},'leaf size census')
    check(max(job[3] for job in leaves)==12,'finite depth')
    check(len(leaves)>len(points),'new count guard rejects valid batch')
    check(nodes==2*len(leaves)-1,'full binary tree consistency')
    return {'nodes':nodes,'leaves':len(leaves),'empty':empty,'sites':len(points),'max_depth':12,
            'leaf_size_counts':dict(Counter(len(job[0]) for job in leaves)),
            'queued_site_incidences':648,'all_leaves_enqueue_admissible':True,
            'common_centre_siteidx':centre,'common_centre_in_all_leaves':True,
            'new_guard_model_result':'catalogue_invariant'}


source=json.loads((ROOT/'SOURCE_BINDINGS.json').read_text())
check(source['pin']=='77db5738eb2dd5bc84ecdc4d85ade833124c58f8','published pin')
for name in ('leaf_batch.cpp','leaf_batch_cuda.cu'):
    check(source['files'][name]['guard']['text']==
          'if (view.count > view.cloud_sites) return fail(Reason::catalogue_invariant);','actual guard recorded')
points=list(product((0,16),repeat=3))+[(8,8,8)]
a=run(points)
b=run(sorted(points,key=morton))
check({k:v for k,v in a.items() if k!='common_centre_siteidx'}==
      {k:v for k,v in b.items() if k!='common_centre_siteidx'},'Morton permutation changes no count')
print(json.dumps({'status':'PASS','scope':'pure eight-corner distance model; no C++/GPU execution',
                  'pin':source['pin'],'checks':CHECKS,'profile_domain':[18,21,24],
                  'parameters':{'kmax':5,'leaf_size':8,'max_leaf':32,'max_nodes':0,
                                'single_pass':True,'pair_graph':True,'batch_leaves':True,
                                'adaptive_frontier':False,'all_weights':1},
                  'integer_fixture':points,'unordered_geometry_model':a,'Morton_model':b},
                 indent=2,sort_keys=True))
