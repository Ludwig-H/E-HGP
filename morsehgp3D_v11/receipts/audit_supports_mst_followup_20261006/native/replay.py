"""Abstract attachments/graph models only; no geometry or native runs."""
import argparse,itertools,json,hashlib
from pathlib import Path

def check(ok,msg):
    if not ok:raise ValueError(msg)

def connected(n,edges,a,b):
    seen={a};stack=[a]
    while stack:
        u=stack.pop()
        for x,y in edges:
            v=y if x==u else x if y==u else None
            if v is not None and v not in seen:seen.add(v);stack.append(v)
    return b in seen

def selector(parents,ranks,balls,mode='spanning'):
    if mode=='all':return list(range(len(balls)))
    parent=list(range(len(parents)));sizes=[1]*len(parents);chosen=[]
    def find(v):
        while parent[v]!=v:parent[v]=parent[parent[v]];v=parent[v]
        return v
    def unite(a,b):
        a,b=find(a),find(b)
        if a==b:return False
        if sizes[a]<sizes[b] or (sizes[a]==sizes[b] and b<a):a,b=b,a
        parent[b]=a;sizes[a]+=sizes[b];return True
    for i,(role,att,rank,prior) in enumerate(balls):
        if role=='birth':chosen.append(i);continue
        if role=='internal':continue
        check(role=='merge' and ranks[att]==rank and prior,'merge metadata')
        changed=False
        for j,child in enumerate(prior):
            check(0<=child<len(parent) and parents[child]==att and ranks[child]<rank,'open child family')
            check(j==0 or prior[j-1]<child,'sorted unique prior')
            if j:changed=unite(prior[0],child) or changed
        if changed:chosen.append(i)
    for att in range(len(parents)):
        children=[c for c,up in enumerate(parents) if up==att]
        if not children:continue
        check(len(children)>=2 and all(ranks[c]<ranks[att] for c in children),'forest family')
        check(all(find(c)==find(children[0]) for c in children),'disconnected multifusion refused')
    return chosen

def proof():
    checked=0
    for n in range(2,6):
        possible=list(itertools.combinations(range(n),2))
        for mask in range(1<<len(possible)):
            edges=[e for i,e in enumerate(possible) if mask>>i&1]
            if not all(connected(n,edges,0,v) for v in range(n)):continue
            parents=[n]*n+[None];ranks=[0]*n+[1]
            balls=[('merge',n,1,list(e)) for e in edges]
            keep=selector(parents,ranks,balls)
            reference=[];accepted=[]
            for i,(a,b) in enumerate(edges):
                if not connected(n,accepted,a,b):reference.append(i);accepted.append((a,b))
            check(keep==reference and len(keep)==n-1,'independent BFS Kruskal comparison')
            check(selector(parents,ranks,balls,'all')==list(range(len(balls))),'all path')
            checked+=1
    check(checked==771,'graph inventory')
    # Families children(4), children(5), children(6) are disjoint; 4/5 stay fresh DSU IDs until next rank.
    parents=[4,4,5,5,6,6,None];ranks=[0,0,0,0,1,1,2]
    balls=[('birth',0,0,[]),('merge',4,1,[0,1]),('merge',5,1,[2,3]),('merge',4,1,[0,1]),('merge',6,2,[4,5]),('merge',6,2,[4,5]),('internal',6,3,[])]
    keep=selector(parents,ranks,balls);check(keep==[0,1,2,4],'multilevel families')
    hyper=[('merge',3,1,[0,1,2]),('merge',3,1,[1,2]),('merge',3,1,[0])]
    check(selector([3,3,3,None],[0,0,0,1],hyper)==[0],'hyperedge and single-component cycle')
    invalid=[('merge',3,1,[0,2])]
    try:selector([3,3,4,None,None],[0,0,0,1,1],invalid)
    except ValueError:rejected=True
    else:rejected=False
    check(rejected,'different parent refused')
    disconnected=[('merge',4,1,[0,1]),('merge',4,1,[2,3])]
    try:selector([4,4,4,4,None],[0,0,0,0,1],disconnected)
    except ValueError as e: disconnected_rejected=str(e)=='disconnected multifusion refused'
    else:disconnected_rejected=False
    check(disconnected_rejected,'two disjoint pairs cannot certify a four-child multifusion')
    # Source-level bounds only: temporaries charged simultaneously; mask remains at output first admission.
    b,n,F,S=3,4,200,40
    selector_peak=b+8*n;first_peak=b+F;after_sort=F
    peak=max(selector_peak,first_peak,F+S)
    check((selector_peak,first_peak,after_sort,peak)==(35,203,200,240),'stage byte arithmetic')
    return dict(scope='Abstract attachments + independent BFS graph oracle; no cloud geometry/native/GCP qualification',native_runs=0,cloud_runs=0,graphs_BFS_compared=checked,triangle=dict(prior=[[0,1],[0,2],[1,2]],selected=[0,1],all=[0,1,2]),multilevel_selected=keep,hyperedge_selected=[0],invalid_different_parent_refused=rejected,disconnected_four_children_refused=disconnected_rejected,memory_formula=dict(selector='B_all*sizeof(u8)+2*N*sizeof(u32)',first_coexistence='B_all*sizeof(u8)+HierarchyAdmission.first()',count_and_fill='old first+second, selector released',example=dict(assumed_first=F,assumed_second=S,selector_peak=selector_peak,first_peak=first_peak,after_sort=after_sort,peak=peak)),all_outputs_model_unchanged=True)

p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',type=Path);a=p.parse_args();r=proof()
if a.check:
    check(json.loads(a.check.read_text())==r,'frozen JSON differs')
    print('MST proposal model PASS:771 connected graphs vs BFS Kruskal, disjoint multi-level families, hyperedge/cycle and memory phases; native0 cloud0')
else:print(json.dumps(r,ensure_ascii=False,indent=2,sort_keys=True))
