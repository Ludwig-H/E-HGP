"""Check one frozen native FULL export against the exact Gamma2 nerve.

Usage: python3 -B check_native_five_sites.py native.stdout
Only geometry/ball resolution is native; majority remains an audit consumer.
"""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'reference'))
import hgp10_ref as R


def require(ok,message):
    if not ok:
        raise RuntimeError(message)


P=[(1,1,0),(2,1,0),(0,2,0),(0,0,0),(0,1,1)]
native_to_input={}
orders={}
mode=None
for line in Path(sys.argv[1]).read_text().splitlines():
    t=line.split()
    if t[0]=='site':
        native_to_input[int(t[1])]=P.index(tuple(map(int,t[2:5])))
    elif t[0]=='mode':
        mode=t[1]
        orders[mode]={'nodes':{},'points':{},'balls':[]}
    elif t[0]=='error':
        raise RuntimeError(line)
    elif t[1]=='2':
        o=orders[mode]
        if t[0]=='node':
            birth=tuple(sorted(native_to_input[int(x)] for x in t[7:]))
            o['nodes'][int(t[2])]=(int(t[3]),F(int(t[4]),int(t[5])),birth)
        elif t[0]=='point':
            o['points'][native_to_input[int(t[2])]]=(int(t[3]),F(int(t[4]),int(t[5])))
        elif t[0]=='ball':
            o['balls'].append((int(t[3]),F(int(t[4]),int(t[5])),
                               tuple(sorted(native_to_input[int(x)] for x in t[6:]))))
require(set(native_to_input.values())==set(range(5)),'bad Morton correspondence')
pairs=list(combinations(range(5),2))
pb={q:R.meb(P,q)[0] for q in pairs}
tb={q:R.meb(P,q)[0] for q in combinations(range(5),3)}
proper={(b.level,tuple(sorted(b.I+b.U))) for b in R.critical_balls(P)
        if b.level>0 and b.p+b.qmin<=2 and len(b.I)+len(b.U)>=2}
cuts=sorted(set(pb.values())|set(tb.values()))


def gamma(beta):
    d=R.DSU()
    for q,b in pb.items():
        if b<=beta:
            d.find(q)
    for q,b in tb.items():
        if b<=beta:
            faces=list(combinations(q,2))
            for f in faces[1:]:
                d.union(faces[0],f)
    return d


def ancestor(nodes,node,beta):
    require(nodes[node][1]<=beta,'node not born at queried date')
    while nodes[node][0]>=0 and nodes[nodes[node][0]][1]<=beta:
        node=nodes[node][0]
    return node


def component(nodes,node,beta,d):
    target=ancestor(nodes,node,beta)
    roots={d.find(seed) for v,(_,birth,seed) in nodes.items()
           if seed and birth<=beta and ancestor(nodes,v,beta)==target}
    require(len(roots)==1,'native component has zero/disconnected Gamma seeds')
    return next(iter(roots))


birth_checks=ball_checks=0
for mode,o in orders.items():
    nodes=o['nodes']
    require(nodes,'missing K2 forest')
    for _,birth,seed in nodes.values():
        if seed:
            require(pb[seed]==birth,'native birth witness has wrong MEB date')
    for beta in cuts:
        d=gamma(beta)
        native_groups={}
        exact_groups={}
        for v,(_,birth,seed) in nodes.items():
            if not seed or birth>beta:
                continue
            native_groups.setdefault(ancestor(nodes,v,beta),set()).add(v)
            exact_groups.setdefault(d.find(seed),set()).add(v)
        require(set(map(frozenset,native_groups.values()))==set(map(frozenset,exact_groups.values())),
                'native continuous birth partition disagrees with Gamma2')
        require(len(native_groups)==len({d.find(q) for q,b in pb.items() if b<=beta}),
                'Gamma component missing from native FULL')
        birth_checks+=1
    if mode=='cover':
        seen=set()
        for node,beta,pop in o['balls']:
            if (beta,pop) not in proper:
                continue
            seen.add((beta,pop))
            require(ancestor(nodes,node,beta)==node,'ball_node is dead at its own date')
            for cut in cuts:
                if cut<beta:
                    continue
                d=gamma(cut)
                want={d.find(q) for q in combinations(pop,2)}
                require(want=={component(nodes,node,cut,d)},'native ball resolved to wrong component')
                ball_checks+=1
        require(seen==proper,'native export omitted a proper K2 witness')
    node,beta=o['points'][0]
    require(beta==(F(1) if mode=='core' else F(1,4)),'wrong entry date for point 0')
    d=gamma(beta)
    require(component(nodes,node,beta,d)==d.find((0,1)),'wrong point-0 entry component')
print('status ok; modes core/cover; Morton IDs mapped by exact coordinates;',
      birth_checks,'native continuous cuts;',len(proper),'proper witnesses;',
      ball_checks,'ball-component checks; majority not native')
