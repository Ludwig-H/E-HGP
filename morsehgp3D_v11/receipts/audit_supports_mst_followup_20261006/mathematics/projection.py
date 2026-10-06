"""Proposed oracle projection: compressed constructor Kruskal, not an explicit Gamma_K MST."""
from fractions import Fraction

SPANNING_BALL = ('node','level','center','role','p','m','qmin','components','prior','supports')
SPANNING_NODE = ('level','parent','children','kind','post','balls','birth_center')

def spanning(doc, morton):
    """Select from the complete oracle document only; never reselect a native result."""
    nodes, balls = doc['nodes'], doc['balls']
    coords = [tuple(p) for p in doc['sites']]
    if len(coords) != doc['n'] or len(set(coords)) != len(coords):
        raise ValueError('spanning: inconsistent sites')
    ranks = {p:i for i,p in enumerate(sorted(coords,key=morton))}
    stars = []
    for ball in balls:
        qs = ball['supports']
        if not qs or any(not 2<=len(q)<=4 or len(set(map(tuple,q)))!=len(q) or
                         any(tuple(p) not in ranks for p in q) for q in qs):
            raise ValueError('spanning: invalid support arity/sites')
        qmin = min(map(len,qs))
        if ball['qmin'] != qmin:
            raise ValueError('spanning: inconsistent qmin')
        stars.append(min((q for q in qs if len(q)==qmin),
                         key=lambda q: tuple(sorted(ranks[tuple(p)] for p in q))))
    def ball_key(b):
        s = tuple(sorted(ranks[tuple(p)] for p in stars[b]))
        return (Fraction(balls[b]['level']), s+(doc['n'],)*(4-len(s)))
    kept, groups, births = set(), {}, {}
    for b,ball in enumerate(balls):
        v = ball['node']
        if not isinstance(v,int) or not 0<=v<len(nodes):
            raise ValueError('spanning: invalid node')
        role = ball['role']
        if role == 'naissance':
            if doc['k']==1 or nodes[v]['kind']!=1 or v in births or ball['prior'] or ball['components']!=0 or \
                    Fraction(ball['level'])!=Fraction(nodes[v]['level']):
                raise ValueError('spanning: inconsistent birth')
            births[v]=b
            kept.add(b)
        elif role == 'fusion':
            prior = ball['prior']
            if nodes[v]['kind']!=2 or Fraction(ball['level'])!=Fraction(nodes[v]['level']) or \
                    not prior or prior!=sorted(set(prior)) or len(prior)!=ball['components'] or \
                    not set(prior)<=set(nodes[v]['children']):
                raise ValueError('spanning: inconsistent fusion/prior')
            groups.setdefault(v,[]).append(b)
        elif role != 'interne':
            raise ValueError('spanning: invalid role')
    if {v for v,node in enumerate(nodes) if node['kind']==1} != set(births):
        raise ValueError('spanning: missing birth')
    for v,node in enumerate(nodes):
        if node['kind']!=2:
            continue
        children=node['children']
        if len(children)<2 or children!=sorted(set(children)):
            raise ValueError('spanning: invalid multifusion children')
        parent={c:c for c in children}
        def find(c):
            while parent[c]!=c:
                parent[c]=parent[parent[c]]
                c=parent[c]
            return c
        for b in sorted(groups.get(v,[]),key=ball_key):
            prior=balls[b]['prior']
            changed=False
            for c in prior[1:]:
                a,z=find(prior[0]),find(c)
                if a!=z:
                    parent[max(a,z)]=min(a,z)
                    changed=True
            if changed:
                kept.add(b)
        if len({find(c) for c in children})!=1:
            raise ValueError('spanning: incomplete multifusion')
    ordered=sorted(kept)  # output keeps the oracle's postorder/level/center convention
    new={old:j for j,old in enumerate(ordered)}
    out={key:doc.get(key) for key in ('k','n','sites','ids')}
    out['nodes']=[]
    for node in nodes:
        row={key:node.get(key) for key in SPANNING_NODE}
        row['balls']=[new[b] for b in node['balls'] if b in new]
        out['nodes'].append(row)
    out['balls']=[]
    for b in ordered:
        row={key:balls[b].get(key) for key in SPANNING_BALL}
        row['supports']=[stars[b]]
        out['balls'].append(row)
    return out
