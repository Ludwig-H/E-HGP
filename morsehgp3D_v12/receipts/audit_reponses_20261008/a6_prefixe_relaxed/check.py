#!/usr/bin/env python3
"""Un graphe axiomatise borne, pas un modele formel exhaustif de C++ ou du moteur."""
import argparse
import hashlib
import json
from pathlib import Path

HERE=Path(__file__).resolve().parent


def need(ok,why):
    if not ok:raise ValueError(why)


def closure(edges):
    adj={}
    for a,b in edges:adj.setdefault(a,set()).add(b)
    reach=set()
    for a in adj:
        todo=list(adj[a]);seen=set()
        while todo:
            b=todo.pop()
            if b in seen:continue
            seen.add(b);todo.extend(adj.get(b,()))
        reach.update((a,b) for b in seen)
    return reach


def graph(leaf_bridge=False,up_bridge=False):
    e={};threads={};mo={}
    def add(name,thread,var,op,value=None,rf=None,order='relaxed'):
        need(name not in e,'event name')
        e[name]=dict(thread=thread,var=var,op=op,value=value,rf=rf,order=order)
        threads.setdefault(thread,[]).append(name)
        if op in ('W','M'):mo.setdefault(var,[]).append(name)
    for var,value in [('leaf_t0',0),('leaf_t1',1),('leaf_u0',0),('leaf_u2',2),
                      ('up0',0),('up1',1),('up2',2),('active',0),('closed',0),
                      ('inflight',0),('busy',0),('slice',0),('hint_next',0)]:
        add('I_'+var,'I',var,'W',value)
    # Le noyau conserve le meme job pendant t puis u ; aucune fin d'aide n'est acquise avant t.
    add('K_announce','K','inflight','M',1,'I_inflight','acq_rel')
    add('K_busy','K','busy','M',1,'I_busy','acq_rel')
    add('K_slice1','K','slice','W',1,order='release')
    add('L','K','leaf_t0','R',2,'S','acquire' if leaf_bridge else 'relaxed')
    add('K_root2_t','K','up2','R',2,'I_up2')
    add('K_leaf1','K','leaf_t1','R',1,'I_leaf_t1','acquire' if leaf_bridge else 'relaxed')
    add('K_root1','K','up1','R',1,'I_up1')
    add('K_union1','K','up1','W',2,order='release' if up_bridge else 'relaxed')
    add('K_slice2','K','slice','W',2,order='release')
    add('K_leaf0_u','K','leaf_u0','R',0,'I_leaf_u0','acquire' if leaf_bridge else 'relaxed')
    add('K_root0_u','K','up0','R',0,'I_up0')
    add('K_leaf2_u','K','leaf_u2','R',2,'I_leaf_u2','acquire' if leaf_bridge else 'relaxed')
    add('K_root2_u','K','up2','R',2,'I_up2')
    add('U','K','up0','W',2,order='release' if up_bridge else 'relaxed')
    add('K_slice3','K','slice','W',3,order='release')
    add('K_close','K','closed','W',1,order='seq_cst')
    add('K_wait0','K','active','R',0,'H_leave','seq_cst')
    add('K_retire','K','inflight','M',0,'H_retire','acq_rel')
    add('H_announce','H','inflight','M',2,'K_announce','acq_rel')
    add('H_busy','H','busy','R',1,'K_busy','acquire')
    add('H_claim_slice','H','slice','R',0,'I_slice','acquire')
    add('H_claim','H','hint_next','M',2,'I_hint_next','acq_rel')
    add('H_enter','H','active','M',1,'I_active','seq_cst')
    add('H_open','H','closed','R',0,'I_closed','seq_cst')
    add('H_processed','H','slice','R',0,'I_slice','acquire')
    add('H_leaf0','H','leaf_t0','R',0,'I_leaf_t0','acquire' if leaf_bridge else 'relaxed')
    add('H','H','up0','R',2,'U','acquire' if up_bridge else 'relaxed')
    add('H_root2','H','up2','R',2,'I_up2','acquire' if up_bridge else 'relaxed')
    add('S','H','leaf_t0','W',2,order='release' if leaf_bridge else 'relaxed')
    # L'aide continue la tranche : ses lectures sur un autre parent peuvent encore voir l'ancienne version.
    add('H_leaf1','H','leaf_t1','R',1,'I_leaf_t1','acquire' if leaf_bridge else 'relaxed')
    add('H_root1','H','up1','R',1,'I_up1','acquire' if up_bridge else 'relaxed')
    add('H_store1','H','leaf_t1','W',1,order='release' if leaf_bridge else 'relaxed')
    add('H_leave','H','active','M',0,'H_enter','seq_cst')
    add('H_retire','H','inflight','M',1,'H_announce','acq_rel')
    # L'ordre de declaration ne pretend pas etre un entrelacement d'execution.
    mo['inflight']=['I_inflight','K_announce','H_announce','H_retire','K_retire']
    mo['active']=['I_active','H_enter','H_leave']
    sb={(a,b) for row in threads.values() for a,b in zip(row,row[1:])}
    common={(threads['I'][-1],threads[t][0]) for t in ('K','H')}
    sc=['H_enter','H_open','K_close','H_leave','K_wait0']
    return e,mo,sb,common,sc


def verify(leaf_bridge=False,up_bridge=False):
    e,mo,sb,common,sc=graph(leaf_bridge,up_bridge)
    writes={n for n,d in e.items() if d['op'] in ('W','M')}
    reads={n for n,d in e.items() if d['op'] in ('R','M')}
    acquire={'acquire','acq_rel','seq_cst'};release={'release','acq_rel','seq_cst'}
    sw={(d['rf'],n) for n,d in e.items() if n in reads and d['order'] in acquire
        and e[d['rf']]['order'] in release}
    # RMW adjacents incluent toute la chaine release ; sa fermeture est deja dans HB ici.
    hb=closure(sb|common|sw)
    rank={n:i for row in mo.values() for i,n in enumerate(row)}
    errors=[]
    def bad(condition,rule,a,b):
        if not condition:errors.append([rule,a,b])
    bad(not any(a==b for a,b in hb),'HB cycle','','')
    for n in sorted(reads):
        d=e[n];w=d['rf'];bad(w in writes and e[w]['var']==d['var'],'RF object',n,w)
        if d['op']=='R':bad(d['value']==e[w]['value'],'RF value',n,w)
        bad((n,w) not in hb,'14 read takes HB-future',n,w)
        if d['op']=='M':bad(rank[w]+1==rank[n],'RMW previous',n,w)
    for n,delta in [('K_announce',1),('H_announce',1),('H_retire',-1),('K_retire',-1),
                    ('H_enter',1),('H_leave',-1)]:
        bad(e[n]['value']==e[e[n]['rf']]['value']+delta,'RMW arithmetic',n,e[n]['rf'])
    for n,old,new in [('K_busy',0,1),('H_claim',0,2)]:
        bad(e[e[n]['rf']]['value']==old and e[n]['value']==new,'CAS success',n,e[n]['rf'])
    for a,b in hb:
        if a==b or e[a]['var']!=e[b]['var']:continue
        aw,bw=a in writes,b in writes;ar,br=a in reads,b in reads
        if aw and bw:bad(rank[a]<rank[b],'15 WW',a,b)
        if ar and br:bad(rank[e[a]['rf']]<=rank[e[b]['rf']],'16 RR',a,b)
        if ar and bw:bad(rank[e[a]['rf']]<rank[b],'17 RW',a,b)
        if aw and br:bad(rank[a]<=rank[e[b]['rf']],'18 WR',a,b)
    # Sans fence : ordre total SC compatible avec HB (condition suffisante, plus forte que strong-HB).
    srank={n:i for i,n in enumerate(sc)}
    bad(set(sc)=={n for n,d in e.items() if d['order']=='seq_cst'},'SC inventory','','')
    for a,b in hb:
        if a in srank and b in srank:bad(srank[a]<srank[b],'SC HB',a,b)
    co={(a,b) for row in mo.values() for i,a in enumerate(row) for b in row[i+1:]}
    co|={(e[n]['rf'],n) for n in reads}
    co|={(n,w) for n in reads for w in mo[e[n]['var']] if rank[e[n]['rf']]<rank[w] and n!=w}
    for a,b in closure(co):
        if a in srank and b in srank and a!=b:bad(srank[a]<srank[b],'SC coherence',a,b)
    return dict(accepted=not errors,events=len(e),hb_edges=len(hb),sc_order=sc,errors=sorted(errors),
                H_happens_before_U=('H','U') in hb,L_happens_before_S=('L','S') in hb)


def geometry_free_kernel(hint):
    # Etat atteignable par les deux unions anterieures (2,3) puis (2,4).
    up=[0,1,2,2,2];size=[1,1,3,1,1];events=[]
    def find(x):
        while up[x]!=x:x=up[x]
        return x
    def union(x,y):
        x,y=find(x),find(y)
        need(x!=y,'distinct roots')
        s,l=(y,x) if size[x]<size[y] else (x,y)
        up[l]=s;size[s]+=size[l];events.append([l,s])
    union(hint,1)
    cut=sorted(sorted(i for i in range(5) if find(i)==r) for r in sorted({find(i) for i in range(5)}))
    union(0,2)
    return dict(hint=hint,cut_after_t=cut,last_write=events[-1],events=events,
                final_partition=sorted({find(i) for i in range(5)}))


def main(sources):
    cap=json.loads((HERE/'capture.json').read_text())
    for p,h in cap['sources'].items():need(hashlib.sha256((sources/p).read_bytes()).hexdigest()==h,'source '+p)
    relaxed=verify();leaf=verify(leaf_bridge=True);up=verify(up_bridge=True);both=verify(True,True)
    need(relaxed['accepted'],'relaxed graph not admissible to checked clauses')
    need(not leaf['accepted'] and leaf['H_happens_before_U'],'leaf bridge fails to exclude graph')
    need(not up['accepted'] and up['L_happens_before_S'],'up bridge fails to exclude graph')
    need(not both['accepted'],'both bridges')
    traces=[geometry_free_kernel(x) for x in (0,2)]
    need(traces[0]['last_write']==traces[1]['last_write']==[0,2],'future parent depends on hint')
    need(traces[0]['cut_after_t']!=traces[1]['cut_after_t'],'no prefix difference')
    return dict(relaxed=relaxed,leaf_bridge=leaf,up_bridge=up,both_bridges=both,kernel_traces=traces,
                native_execution=False,geometric_reachability_proved=False,
                independent_formal_cpp_tool=False)


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('sources',type=Path);ap.add_argument('--check',action='store_true');args=ap.parse_args()
    result=main(args.sources)
    if args.check:need(result==json.loads((HERE/'results.json').read_text()),'stored result differs')
    print(json.dumps(result,indent=2,sort_keys=True))
