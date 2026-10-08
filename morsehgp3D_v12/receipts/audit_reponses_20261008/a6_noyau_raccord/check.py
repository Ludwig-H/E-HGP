#!/usr/bin/env python3
"""Small abstract hypergraph model and arithmetic on already-admitted FULL M metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import statistics
import subprocess
HERE=Path(__file__).resolve().parent
def need(x,msg):
    if not x:raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()

def model(births,cells):
    # A target is ('b', birth) or ('c', earlier cell), never coordinates.
    anchors=[];edges=[]
    for i,(rank,targets) in enumerate(cells):
        resolved=[]
        for kind,n in targets:
            need(kind=='b' or 0<=n<i and cells[n][0]<rank,'strict target rank')
            resolved.append(n if kind=='b' else anchors[n])
        anchors.append(resolved[0])
        edges += [((rank,i,j),resolved[0],v) for j,v in enumerate(resolved[1:],1)]
    # Prim under a unique total edge order is independent of the sequential union kernel.
    chosen=set();seen=set()
    for start in range(births):
        if start in seen:continue
        seen.add(start)
        while True:
            crossing=[e for e in edges if (e[1] in seen)!=(e[2] in seen)]
            if not crossing:break
            key,u,v=min(crossing);chosen.add(key);seen.update((u,v))
    def kernel(filtered):
        up=list(range(births));size=[1]*births;minleaf=list(range(births));top=list(range(births));element=[];celltop=[];events=[];att=[None]*births;finds=0
        def find(x):
            nonlocal finds
            finds+=1
            while up[x]!=x:up[x]=up[up[x]];x=up[x]
            return x
        def target(t):return t[1] if t[0]=='b' else element[t[1]]
        for i,(rank,ts) in enumerate(cells):
            first=anchors[i] if filtered else target(ts[0]);x=find(first)
            for j,t in enumerate(ts[1:],1):
                if filtered and (rank,i,j) not in chosen:continue
                y=find((t[1] if t[0]=='b' else anchors[t[1]]) if filtered else target(t))
                if x==y:continue
                survivor,loser=(y,x) if size[x]<size[y] else (x,y)
                ml=min(minleaf[x],minleaf[y]);events.append((rank,i,j,top[x],top[y],ml,survivor));att[loser]=(survivor,rank)
                up[loser]=survivor;size[survivor]+=size[loser];minleaf[survivor]=ml;top[survivor]=births+len(events)-1;x=survivor
            element.append(x);celltop.append(top[x])
        return (events,att,element,celltop),finds
    original,n0=kernel(False);filtered,n1=kernel(True)
    need(original==filtered,'event/attach/cell state mismatch')
    accepted={(e[0],e[1],e[2]) for e in original[0]};need(accepted==chosen,'Prim/Kruskal accepted set')
    need(n0==sum(len(ts) for _,ts in cells) and n1==len(cells)+len(chosen),'find count')
    return n0,n1,len(chosen)

def fixtures():
    cases=[(1,[(1,[('b',0)])]),(3,[(1,[('b',0),('b',1)]),(1,[('b',0),('b',2)])]),
           (3,[(1,[('b',0)]),(1,[('b',0),('b',1)]),(2,[('c',0),('b',2)])]),
           (4,[(1,[('b',0),('b',1),('b',1)]),(2,[('c',0),('b',2)]),(3,[('c',1),('b',3)])])]
    rng=random.Random(20261008)
    for _ in range(96):
        n=rng.randrange(2,9);cells=[]
        for i in range(rng.randrange(2,16)):
            rank=1+i//3;choices=[('b',j) for j in range(n)]+[('c',j) for j,c in enumerate(cells) if c[0]<rank]
            cells.append((rank,[rng.choice(choices) for _ in range(rng.randrange(1,9))]))
        cells.append((cells[-1][0]+1,[('b',j) for j in range(n)]));cases.append((n,cells))
    result=[model(*c) for c in cases]
    return dict(cases=len(cases),identity='events, attaches, element, cell_top and selected edge IDs exact',
                original_finds=sum(r[0] for r in result),filtered_finds=sum(r[1] for r in result),
                new_native_runs=0,scope='abstract ranked hypergraphs, no geometric realizability or speed claim')

def main():
    p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);p.add_argument('--returned-full',type=Path,required=True);a=p.parse_args()
    c=json.loads((HERE/'capture.json').read_bytes())
    def git(rev,path):return subprocess.check_output(['git','show',rev+':morsehgp3D_v12/'+path],cwd=a.repo)
    for path,h in c['sources'].items():need(sha(git(c['source_read'],path))==h,path)
    for path in c['same_m_read']:need(git(c['source_read'],path)==git(c['source_measurement'],path),'same pin '+path)
    records=[];conditional={}
    for name,h in c['jsonl_sha256'].items():
        data=(a.returned_full/'brut'/name).read_bytes();need(sha(data)==h,name)
        rows=[x for x in map(json.loads,data.splitlines()) if x.get('phase')=='full' and x['pass']>=37]
        need(len(rows)==37,'37 warm frames')
        for row in rows:conditional.setdefault(row['trame'],[]).append(row['etapes_ns']['P']+row['etapes_ns']['C']+row['recouvrement']['fin_g_ns'])
        selected=[x for x in rows if x['trame']=='kitti_ng_08_002119']
        need(len(selected)==1,'one warm frame');r=selected[0];rec=r['recouvrement'];ends=r['fins_par_ordre_ns'];order5=ends[4]
        maximum=max(max(x) for x in ends);need(maximum==order5[4],'last R5')
        records.append(dict(process=name,pass_index=r['pass'],wall_ns=r['wall_ns'],C_ns=r['etapes_ns']['C'],
            conditional_P_C_lastG_ns=r['etapes_ns']['P']+r['etapes_ns']['C']+rec['fin_g_ns'],opening_ns=rec['ouverture_ns'],last_G_ns=rec['fin_g_ns'],G5_end_ns=order5[0],kernel5_end_ns=order5[1],
            M5_end_ns=order5[2],V5_end_ns=order5[3],R5_end_ns=order5[4],region_end_ns=rec['fin_ns'],tower_wall_ns=rec['tour_ns'],
            interval_kernel5_after_own_G_ns=order5[1]-order5[0],interval_kernel5_after_last_G_ns=order5[1]-rec['fin_g_ns'],
            interval_R5_after_kernel5_ns=order5[4]-order5[1],tail_ns=rec['queue_ns'],T_all_thread_window_ns=r['fenetres_ns']['T']))
    need(len(records)==5,'five processes')
    med={k:statistics.median(x[k] for x in records) for k in records[0] if k not in ('process','pass_index')}
    need(len(conditional)==37 and all(len(v)==5 for v in conditional.values()),'conditional cohort')
    cm={n:statistics.median(v) for n,v in conditional.items()};flat=[x for v in conditional.values() for x in v]
    bound=dict(formula='P+C+last_G with all three unchanged; not an architectural bound',frames=37,warm_passes=len(flat),
        median_of_frame_medians_ns=statistics.median(cm.values()),max_frame_median_ns=max(cm.values()),max_raw_ns=max(flat),
        frame_medians_over_100ms=sum(v>100000000 for v in cm.values()),passes_over_100ms=sum(v>100000000 for v in flat),
        frame_medians_ns=cm)
    print(json.dumps(dict(warm_frame=records,medians_ns=med,conditional_remainder=bound,model=fixtures()),sort_keys=True,indent=2))
if __name__=='__main__':main()
