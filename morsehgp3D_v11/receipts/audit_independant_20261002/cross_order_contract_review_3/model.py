"""Recherche analytique Gamma collineaire, sans import produit/reference."""
from fractions import Fraction as F
from itertools import combinations
import json


def model(xs, k):
    parts = list(combinations(range(len(xs)), k))
    beta = lambda s: F((max(xs[i] for i in s)-min(xs[i] for i in s))**2, 4)
    birth = {s: beta(s) for s in parts}
    edges = list(combinations(range(len(xs)), k+1))
    levels = sorted(set(birth.values()) | {beta(s) for s in edges})
    history, previous, nodes = [], {}, []
    for a in levels:
        active = [s for s in parts if birth[s] <= a]
        parent = {s:s for s in active}
        def root(s):
            while parent[s]!=s:
                s=parent[s]
            return s
        for e in edges:
            if beta(e)>a:
                continue
            faces = list(combinations(e,k))
            for s in faces[1:]:
                parent[root(s)] = root(faces[0])
        comps = {}
        for s in active:
            comps.setdefault(root(s), []).append(s)
        current = {}
        for members in sorted(comps.values()):
            old = sorted({previous[s] for s in members if s in previous})
            if len(old)==1:
                node = old[0]
            else:
                node = len(nodes)
                nodes.append({'birth':a, 'children':old, 'points':set()})
            for s in members:
                current[s] = node
        history.append((a,current)); previous=current
    entries=[]
    for i,x in enumerate(xs):
        nearest=sorted(range(len(xs)),key=lambda j:((xs[j]-x)**2,j))[:k]
        a=F((xs[nearest[-1]]-x)**2)
        chosen=tuple(sorted(nearest))
        state=next(v for b,v in reversed(history) if b<=a)
        node=state[chosen];nodes[node]['points'].add(i)
        entries.append((i,a,node))
    for node in nodes:
        for c in node['children']:
            node['points'] |= nodes[c]['points']
    return nodes,entries


