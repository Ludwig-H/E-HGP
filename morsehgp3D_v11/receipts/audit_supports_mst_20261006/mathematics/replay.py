#!/usr/bin/env python3
"""Exact three-site K1 counterexample; no native product execution."""
import argparse
from fractions import Fraction as F
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

CHECKS = 0
def require(ok, message):
    global CHECKS
    if not ok:
        raise RuntimeError(message)
    CHECKS += 1

def morton(p):
    return sum(((x>>b)&1)<<(3*b+a) for a,x in enumerate(p) for b in range(24))

def distance(a,b):
    return sum((x-y)**2 for x,y in zip(a,b))

class DSU:
    def __init__(self,n): self.parent=list(range(n))
    def find(self,x):
        while self.parent[x]!=x: x=self.parent[x]
        return x
    def unite(self,a,b):
        a,b=self.find(a),self.find(b)
        if a==b:return False
        self.parent[max(a,b)]=min(a,b)
        return True

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--repo',default='/workspaces/E-HGP');args=parser.parse_args()
    base=Path(__file__).parent;manifest=json.loads((base/'sources.json').read_text())
    for rel,info in manifest['wip'].items():
        data=(base/'source'/rel).read_bytes()
        require(hashlib.sha256(data).hexdigest()==info['sha256'],'WIP capture changed')
    for path,info in manifest['git'].items():
        data=subprocess.check_output(['git','-C',args.repo,'show',manifest['pin']+':'+path])
        require(hashlib.sha256(data).hexdigest()==info['sha256'],'Git source changed')
    hierarchy=(base/'source/src/supports/hierarchy.cpp').read_text()
    require('tree.attachment().role()[i] != BallRole::internal' in hierarchy,'observed filter absent')

    points=[(0,0,0),(1,1,0),(1,0,1)]
    require(points==sorted(points,key=morton),'Morton order')
    require([morton(p) for p in points]==[0,3,5],'Morton keys')
    # Birth nodes at K1 use XYZ lexicographic order; supports use Morton SiteIdx.
    birth_points=sorted(points)
    site_to_node=[birth_points.index(p) for p in points]
    require(site_to_node==[0,2,1],'actual birth-node ordering')
    balls=[]
    for i,j in itertools.combinations(range(3),2):
        center=tuple(F(a+b,2) for a,b in zip(points[i],points[j]))
        radius=distance(center,points[i])
        interior=[k for k,p in enumerate(points) if distance(center,p)<radius]
        shell=[k for k,p in enumerate(points) if distance(center,p)==radius]
        require(radius==F(1,2),'same rational level')
        require(interior==[] and shell==[i,j],'all three pairs are Gabriel/regular')
        third=3-i-j
        require(distance(center,points[third])==F(3,2),'third strictly outside')
        balls.append(dict(support=[i,j],prior=sorted([site_to_node[i],site_to_node[j]]),rank=1,
                          radius_squared=str(radius),p=0,m=2,qmin=2))
    # Forest consumes the whole rank before closing its multifusion.
    full=DSU(3)
    accepted=[]
    for ball in balls:
        success=full.unite(*ball['prior'])
        accepted.append(success)
    require(accepted==[True,True,False],'third plateau edge closes a cycle')
    require(len({full.find(i) for i in range(3)})==1,'FULL plateau merges all births')
    node_rank=[0,0,0,1];parents=[3,3,3,None]
    for ball in balls:
        att={parents[n] if parents[n] is not None and node_rank[parents[n]]==ball['rank'] else n
             for n in ball['prior']}
        require(att=={3},'closed attachment is multifusion')
        ball['attached_node']=3
        ball['role']='merge' if node_rank[3]==ball['rank'] else 'internal'
    kept=[b['support'] for b in balls if b['role']!='internal']
    kruskal=[b['support'] for b,success in zip(balls,accepted) if success]
    require(kept==[[0,1],[0,2],[1,2]],'WIP keeps cyclic triangle')
    require(kruskal==[[0,1],[0,2]],'canonical Kruskal chooses two supports')
    require(len(kept)>len(points)-1,'not a spanning tree')
    require(sum(F(b['radius_squared']) for b in balls)==F(3,2),'kept cost')
    require(sum(F(b['radius_squared']) for b,success in zip(balls,accepted) if success)==1,'MST cost')
    # Fix by a local DSU on the multifusion's children, without changing FULL or its metadata.
    local=DSU(3);chosen=[]
    for ball in balls:
        first=ball['prior'][0];changed=False
        for other in ball['prior'][1:]: changed=local.unite(first,other) or changed
        if changed:chosen.append(ball['support'])
    require(chosen==kruskal,'local multifusion DSU selects the same associated supports')
    print(json.dumps(dict(status='FAIL_observed_role_filter_contract__PASS_portable_counterexample',checks=CHECKS,
        native_executed=False,wip_head=manifest['head'],sites_morton=points,birth_node_of_site=site_to_node,
        k=1,balls=balls,kept_by_role_filter=kept,associated_supports_of_canonical_kruskal=kruskal,
        full_atomic_multifusion=dict(node=3,rank=1,children=[0,1,2]),
        limits='Exact geometry and abstract DSU/attachment model anchored to source; no product binary, file writer, or native campaign executed.'),sort_keys=True,indent=2))

if __name__=='__main__':main()
