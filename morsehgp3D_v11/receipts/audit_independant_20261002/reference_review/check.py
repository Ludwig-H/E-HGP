#!/usr/bin/env python3
"""Petit troisième juge : Γ complet sur droites, plus géométries gravées.

Chargement uniquement d'une copie /tmp des sources figées. Toutes les
unions de deux k-parties sont jugées par min/max sur la droite : aucun
MEB, solveur, intgeom ou canonical_nodes du produit dans cette vérité.
"""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import hashlib
import json
import sys


def require(ok, why):
    if not ok:
        raise ValueError(why)


def beta(axis, part):
    vals = [axis[i] for i in part]
    return F((max(vals)-min(vals))**2,4)


def direct(axis,k,level,closed):
    inside = (lambda x:x <= level) if closed else (lambda x:x < level)
    parts = [p for p in combinations(range(len(axis)),k) if inside(beta(axis,p))]
    parent = list(range(len(parts)))
    def root(i):
        while parent[i] != i:
            i = parent[i]
        return i
    for i,j in combinations(range(len(parts)),2):
        if inside(beta(axis,tuple(sorted(set(parts[i])|set(parts[j]))))):
            a,b = root(i),root(j)
            parent[b] = a
    groups = {}
    for i,p in enumerate(parts):
        groups.setdefault(root(i),[]).append(p)
    return sorted(tuple(sorted(g)) for g in groups.values())


def ancestor(res,v,level,closed=True):
    while res.parent[v] >= 0:
        p = res.parent[v]
        if not (res.nodes[p].level <= level if closed else res.nodes[p].level < level):
            break
        v = p
    return v


def mapping(route,res,axis,k,groups,level,closed):
    nodes = {}
    for group in groups:
        actual = set()
        for part in group:
            start = route.node_at(k,part,beta(axis,part))
            actual.add(ancestor(res,start,level,closed))
        require(len(actual) == 1, "composante Γ partagée entre deux nœuds")
        v = actual.pop()
        require(v not in nodes, "deux composantes Γ fusionnées trop tôt")
        nodes[v] = group
    return nodes


def run(reference):
    sys.path.insert(0,str(reference))
    from hgp11_ref import Definition, Reference
    from hgp11_ref import judge
    guards = orders = cut_checks = verticals = 0
    lines = [("three",(0,2,4),3), ("five",(0,1,2,6,9),4),
             ("weighted",(0,0,2,2,5),5), ("equal",(3,3,3,3),4)]
    for name,axis,kmax in lines:
        pts = [(x,0,0) for x in axis]
        routes = [Definition(pts),Reference(pts,kmax)]
        for k in range(1,kmax+1):
            near = [tuple(sorted(range(len(axis)),key=lambda y:((axis[x]-axis[y])**2,y))[:k])
                    for x in range(len(axis))]
            core_levels = [F(sorted((axis[x]-axis[y])**2 for y in range(len(axis)))[k-1])
                           for x in range(len(axis))]
            levels = {beta(axis,p) for size in range(1,len(axis)+1)
                      for p in combinations(range(len(axis)),size)} | set(core_levels)
            sorted_levels = sorted(levels)
            cuts = sorted(levels | {(a+b)/2 for a,b in zip(sorted_levels,sorted_levels[1:])})
            for route in routes:
                res = route.order(k)
                for level in cuts:
                    for closed in (False,True):
                        groups = direct(axis,k,level,closed)
                        nodes = mapping(route,res,axis,k,groups,level,closed)
                        expected = []
                        for v,g in nodes.items():
                            cov = sum(1 << x for x in set(y for p in g for y in p))
                            cor = sum(1 << x for x in range(len(axis))
                                      if (core_levels[x] <= level if closed else core_levels[x] < level)
                                      and tuple(sorted(near[x])) in g)
                            expected.append((v,cov,cor))
                        require(tuple(sorted(expected)) == res.cut(level)[1 if closed else 0],
                                f"coupe {name} K{k} niveau{level} fermé{closed}")
                        guards += 1
                        cut_checks += 1
                        if closed and k > 1:
                            prev = route.order(k-1)
                            for v,g in nodes.items():
                                image = ancestor(prev,res.lower[v],level)
                                for part in g:
                                    subset = part[:-1]
                                    start = route.node_at(k-1,subset,beta(axis,subset))
                                    require(ancestor(prev,start,level) == image,"verticale de chaque k-partie")
                                    guards += 1
                                    verticals += 1
                for x in range(len(axis)):
                    cover_level = min(beta(axis,p) for p in combinations(range(len(axis)),k) if x in p)
                    groups = direct(axis,k,cover_level,True)
                    nodes = mapping(route,res,axis,k,groups,cover_level,True)
                    wanted = frozenset(v for v,g in nodes.items() if any(x in p for p in g))
                    require(res.cover[x].level == cover_level and res.cover[x].nodes == wanted,"cover complet")
                    require(res.core[x].level == core_levels[x],"date core exacte")
                    guards += 2
                orders += 1

    special = [
        ("equilateral_3d",[(1,0,0),(0,1,0),(0,0,1)],3),
        ("right_triangle",[(0,0,0),(6,0,0),(0,8,0)],3),
        ("square",[(0,0,0),(2,0,0),(0,2,0),(2,2,0)],4),
        ("regular_tetra",[(0,0,0),(2,2,0),(2,0,2),(0,2,2)],4),
        ("triangles_integer",[(268,3000,0),(268,1000,0),(2000,2000,0),
                              (4000,2000,0),(5732,3000,0),(5732,1000,0)],3)]
    summaries = []
    for name,pts,kmax in special:
        errors,a,b = judge.compare_cloud(pts,kmax)
        require(not errors,"deux voies "+name+str(errors))
        guards += 1
        for ball in b.balls:
            inner,shell = [],[]
            for i,p in enumerate(pts):
                power = sum((F(x)-c)**2 for x,c in zip(p,ball.center))-ball.level
                if power < 0: inner.append(i)
                if power == 0: shell.append(i)
            require(sorted(b.inp[x] for x in ball.I) == inner and
                    sorted(b.inp[x] for x in ball.U) == shell,"I/U rationnels indépendants")
            guards += 1
        if name == "equilateral_3d":
            res = a.order(2)
            require(len(res.nodes) == 4 and res.nodes[-1].level == F(2,3) and
                    len(res.nodes[-1].children) == 3,"triangle exact : fusion ternaire")
            require(sorted((e[1] for e in res.cut(F(2,3))[0])) == [3,5,6],"trois arêtes avant contact")
            guards += 2
        elif name == "right_triangle":
            ball = next(x for x in b.balls if x.level == 25)
            require(ball.qmin == 2 and ball.m == 3 and ball.extended,"support antipodal + coquille entière")
            guards += 1
        elif name == "square":
            ball = next(x for x in b.balls if x.level == 2)
            require(ball.qmin == 2 and ball.m == 4 and len(b.cell(ball,2)[1]) == 4,
                    "quatre morceaux exacts, diagonales non séparables")
            require(b.cell(ball,3)[0] == "birth","naissance K3 sur coquille étendue")
            guards += 2
        elif name == "regular_tetra":
            ball = next(x for x in b.balls if x.level == 3)
            require(ball.qmin == 4 and ball.m == 4 and ball.p == 0 and ball.center == (F(1),F(1),F(1)),
                    "support tetra strict exact")
            res = a.order(3)
            require(res.nodes[-1].level == 3 and len(res.nodes[-1].children) == 4,
                    "plateau K3 quatre parents")
            guards += 2
        else:
            res = a.order(2)
            tri = F(249978000484,187489)
            require(sorted(e[1] for e in res.cut(tri)[1]) == [7,12,56],"FULL : ABC, CD, DEF")
            require(res.nodes[-1].level == 3731956 and len(res.nodes[-1].children) == 3,
                    "plateau global trois parents")
            require(all(e.nodes == len(res.nodes)-1 for e in res.core),"core après réunion globale")
            guards += 3
        summaries.append({"fixture":name,"points":len(pts),"orders":kmax,"balls":len(b.balls)})
    return {"status":"REFERENCE_BOUNDED_INDEPENDENT_PASS","guards":guards,
            "analytic_line_routes_orders":orders,"cut_checks":cut_checks,"vertical_incidence_checks":verticals,
            "line_cases":[x[0] for x in lines],"special_cases":summaries,
            "native_engine_calls":0,"scope":"copie initiale en construction; pas suites fast/full ni profil natif qualifié"}


if __name__ == "__main__":
    base = Path(__file__).resolve().parent
    if (base/"SHA256SUMS").exists():
        for row in (base/"SHA256SUMS").read_text().splitlines():
            sha,relative = row.split("  ",1)
            require(hashlib.sha256((base/relative).read_bytes()).hexdigest() == sha,"SHA256 "+relative)
    if len(sys.argv) != 2:
        raise ValueError("copie /tmp/reference attendue")
    print(json.dumps(run(Path(sys.argv[1])),sort_keys=True,ensure_ascii=False,indent=2))
