#!/usr/bin/env python3
"""Bounded fresh cases: current A/B and dynamic FULL-component coverage.

No native/build/fit/GCP. The own Gram arithmetic and exhaustive subset graph
check the frozen Python targets. This third calculation still relies on the
same mathematical MEB<=4 and Gamma_k nerve theorems; it is not advertised as
a third independent proof of those theorems. Unit distinct sites only.

Coverage at a closed (respectively open) cut is the union of sites of all
active k-parts in its Gamma component. Independently, emit every strong
critical ball, with its complete population, at its own level; raise each
population to its current Gamma component. These two relations must agree.
One first incidence per site would NOT suffice for this check.
"""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import ast
import hashlib
import math
import types
import json
import sys

ROOT = Path(__file__).resolve().parent
REF = ROOT / 'git_source/morsehgp3D_v11/reference'
sys.path.insert(0, str(REF))
from hgp11_ref import Definition, Reference, judge

CHECKS = 0


def need(ok, what):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(what)


def solve(A, b):
    rows = [list(map(Q, r))+[Q(v)] for r, v in zip(A, b)]
    for j in range(len(b)):
        pivot = next((i for i in range(j, len(b)) if rows[i][j]), None)
        if pivot is None:
            return None
        rows[pivot], rows[j] = rows[j], rows[pivot]
        t = rows[j][j]
        rows[j] = [x/t for x in rows[j]]
        for i in range(len(b)):
            if i != j:
                t = rows[i][j]
                rows[i] = [x-t*y for x, y in zip(rows[i], rows[j])]
    return [r[-1] for r in rows]


def circumsphere(X, part):
    a = X[part[0]]
    d = [tuple(x-y for x, y in zip(X[i], a)) for i in part[1:]]
    if not d:
        return tuple(map(Q, a)), Q(0), (Q(1),)
    gram = [[sum(x*y for x, y in zip(v, w)) for w in d] for v in d]
    coefficients = solve(gram, [Q(sum(x*x for x in v), 2) for v in d])
    if coefficients is None:
        return None
    c = tuple(Q(a[j])+sum(t*v[j] for t, v in zip(coefficients, d)) for j in range(3))
    radius = sum((x-y)**2 for x, y in zip(c, a))
    return c, radius, (1-sum(coefficients),)+tuple(coefficients)


class Own:
    def __init__(self, X, K):
        self.X, self.n, self.K = X, len(X), K
        self.candidates, self.critical, self.cache = [], {}, {}
        for q in range(1, min(4, self.n)+1):
            for part in combinations(range(self.n), q):
                sphere = circumsphere(X, part)
                if sphere is None:
                    continue
                c, b, weights = sphere
                inner = {i for i, x in enumerate(X) if sum((v-w)**2 for v, w in zip(x, c)) < b}
                shell = {i for i, x in enumerate(X) if sum((v-w)**2 for v, w in zip(x, c)) == b}
                self.candidates.append((b, c, inner | shell))
                if all(w > 0 for w in weights):
                    obj = self.critical.setdefault((c, b), dict(q=q, inner=inner, shell=shell, supports=[]))
                    obj['supports'].append(frozenset(part))
        self.candidates.sort(key=lambda p: (p[0], p[1]))
        self.parts = {k: list(combinations(range(self.n), k)) for k in range(1, min(K+1, self.n)+1)}

    def meb(self, part):
        key = tuple(sorted(part))
        if key not in self.cache:
            p = set(part)
            winner = next(c for c in self.candidates if p <= c[2])
            self.cache[key] = winner[:2]
        return self.cache[key]

    def components(self, k, beta, opened):
        accepted = lambda b: b < beta if opened else b <= beta
        vertices = [p for p in self.parts[k] if accepted(self.meb(p)[0])]
        graph = {p: set() for p in vertices}
        for part in self.parts.get(k+1, []):
            if accepted(self.meb(part)[0]):
                faces = list(combinations(part, k))
                for p in faces:
                    graph[p].update(faces)
        unseen, components = set(vertices), []
        while unseen:
            start = min(unseen)
            unseen.remove(start)
            component, stack = {start}, [start]
            while stack:
                p = stack.pop()
                for q in graph[p] & unseen:
                    unseen.remove(q)
                    component.add(q)
                    stack.append(q)
            components.append(component)
        return components

    def strong_cover(self, k, beta, opened, components):
        owner = {p: j for j, comp in enumerate(components) for p in comp}
        result = [set() for _ in components]
        for (_c, b), obj in self.critical.items():
            if not (b < beta if opened else b <= beta):
                continue
            p, q, m = len(obj['inner']), obj['q'], len(obj['shell'])
            if p+q <= k <= p+m:
                population = obj['inner'] | obj['shell']
                part = tuple(sorted(population)[:k])
                need(part in owner, 'strong population witness is active at its proper cut')
                result[owner[part]].update(population)
        return result



def exact_private_primitives():
    """Execute only frozen arithmetic and _node_core AST nodes, never imports/fit.

    The private target Rad arithmetic is shared for comparison only; the FULL
    graph and the meeting formula are constructed separately in this checker.
    """
    arithmetic = ROOT/'git_source/morsehgp3D_v11/bench/points_radius.py'
    nodes = ast.parse(arithmetic.read_text()).body
    selected = [n for n in nodes if getattr(n, 'name', '') in ('square_ratio', 'sqrt_bounds', 'Refusal')]
    env = {'Fraction': Q, 'math': math}
    exec(compile(ast.Module(body=selected, type_ignores=[]), str(arithmetic), 'exec'), env)
    private = ROOT/'private_before/modele/scripts/modele_lib.py'
    nodes = ast.parse(private.read_text()).body
    target = next(n for n in nodes if isinstance(n, ast.ClassDef) and n.name == 'PointHierarchy')
    method = next(n for n in target.body if isinstance(n, ast.FunctionDef) and n.name == '_node_core')
    selected = [n for n in nodes if getattr(n, 'name', '') in ('_classes', 'Rad', 'rcmp', 'rmax')]+[method]
    env.update(ZERO=Q(0), prad=types.SimpleNamespace(**{k: env[k] for k in ('square_ratio', 'sqrt_bounds')}))
    exec(compile(ast.Module(body=selected, type_ignores=[]), str(private), 'exec'), env)
    return env


EXACT = exact_private_primitives()
Rad, rcmp, rmax = (EXACT[k] for k in ('Rad', 'rcmp', 'rmax'))


class BAdapter:
    """Tiny frozen-definition adapter, no private oracle / ndarray execution."""
    def __init__(self, X, res, m):
        self.n, self.res = len(X), res
        self.parent = [-1]*len(res.nodes)
        self.birth = [Rad.sqrt(n.level) for n in res.nodes]
        for v,n in enumerate(res.nodes):
            for c in n.children:
                self.parent[c] = v
        self.e, self.owner = [], []
        for i in range(self.n):
            qualified = [(cut.level,v) for cut in res.cuts for v,cov,_core in cut.closed
                         if cov.bit_count() >= m and cov >> i & 1]
            t = min(b for b,_ in qualified)
            p = next(v for b,v in qualified if b == t)
            delta = Rad.rat(0)
            for b,v in qualified:
                meeting = self.level_meet(p,t,v,b)
                delta = rmax(delta, Rad.sqrt(meeting)-Rad.sqrt(b))
            e = Rad.sqrt(t)+delta
            self.e.append(e)
            self.owner.append(self.alive_at(p,e))
        EXACT['_node_core'](self)

    def lca(self, a, b):
        seen = set()
        while a >= 0:
            seen.add(a)
            a = self.parent[a]
        while b not in seen:
            b = self.parent[b]
            if b < 0:
                raise RuntimeError('unexpected roots in finite qualified profile')
        return b

    def level_meet(self, a, ta, b, tb):
        w = self.lca(a,b)
        return max(ta,tb,self.res.nodes[w].level)

    def alive_at(self, v, r):
        while self.parent[v] >= 0 and rcmp(self.birth[self.parent[v]],r) <= 0:
            v = self.parent[v]
        return v

    def predicted_B(self, i):
        core = self.res.core[i]
        w = self.lca(self.owner[i], core.nodes)
        return rmax(self.e[i], Rad.sqrt(core.level), self.birth[w])


def check_B_meeting(X,res):
    b = BAdapter(X,res,res.k+1)
    for i in range(len(X)):
        predicted = b.predicted_B(i)
        need(rcmp(predicted,b.sB[i]) == 0, 'B current-owner core scan equals dated H/core meeting')
        need(rcmp(predicted,b.e[i]) >= 0, 'B never counts before H entry')
        need(rcmp(predicted,Rad.sqrt(res.core[i].level)) >= 0, 'B never counts before core activation, k includes self')
    return b


def main():
    fixtures = [
        ('three_equal_pair_births', [(0,0,0),(2,0,0),(0,5,0),(2,5,0),(8,0,0),(10,0,0),(6,3,4)]),
        ('nonregular_coplanar', [(0,0,0),(6,0,0),(0,6,0),(6,6,0),(3,3,0),(3,0,0),(9,2,0)]),
        ('mixed_shells', [(0,0,2),(2,0,0),(0,2,0),(4,2,0),(2,4,0),(2,2,4),(2,2,2)]),
        ('affine_obtuse', [(0,0,0),(8,0,0),(1,2,0),(4,9,0),(10,7,0),(6,3,0)]),
        ('genuine_three_dimensional', [(0,0,0),(5,1,0),(0,6,2),(2,0,7),(7,5,4),(3,3,3)]),
        ('collinear_irregular', [(v,0,0) for v in (0,1,4,9,10,16,21)]),
    ]
    # Units and a translated input near the top of the declared B-domain.
    base = fixtures[-2][1]
    fixtures.append(('same_ids_grid_units', [tuple(17*v+11 for v in x) for x in base]))
    fixtures.append(('u21_translated_domain', [tuple(2**21-200+v for v in x) for x in base]))
    rows = []
    for name, X in fixtures:
        need(len(set(X)) == len(X), 'distinct unit sites')
        K = min(4, len(X))
        own = Own(X, K)
        A, B = Definition(X), Reference(X, K, admission='single')
        # The catalogue test is geometric: canon IDs themselves depend on Morton.
        expected = {(c, b) for (c, b), o in own.critical.items() if b == 0 or len(o['inner'])+o['q'] <= K+1}
        actual = {(b.center, b.level) for b in B.balls}
        need(actual == expected, 'bounded catalogue completeness including zero internal singleton records')
        prev_A = prev_B = None
        local_checks = 0
        for (c, b), obj in own.critical.items():
            if b == 0:
                continue
            shell, inner = sorted(obj['shell']), obj['inner']
            for t in range(1, len(shell)+1):
                for part in combinations(shell, t):
                    not_separable = any(s <= set(part) for s in obj['supports'])
                    strict = own.meb(inner | set(part))[0] < b
                    need(strict == (not not_separable), 'T2 strict trace equivalence at boundary degeneracies')
                    local_checks += 1
        cuts, component_covers, b_meetings = 0, 0, 0
        for k in range(1, K+1):
            a, b = A.order(k), B.order(k)
            need(judge.coherence(a, len(X), prev_A) is None, 'current definition coherence')
            need(judge.coherence(b, len(X), prev_B) is None, 'current constructive coherence')
            need(not judge.compare_orders(a, b), 'A equals B, all tree/cut/entry/vertical fields')
            for cut in a.cuts:
                for opened, entries in ((True, cut.opened), (False, cut.closed)):
                    comps = own.components(k, cut.level, opened)
                    covered = [set().union(*(set(p) for p in comp)) for comp in comps]
                    strong = own.strong_cover(k, cut.level, opened, comps)
                    need(covered == strong, 'complete dynamic coverage from all strong records, component by component')
                    actual_masks = sorted(mask for _v, mask, _core in entries)
                    own_masks = sorted(sum(1 << i for i in cov) for cov in covered)
                    need(actual_masks == own_masks, 'coverage global, not first incidence or point-DSU closure')
                    # Explicit core owner: k nearest points (ties arbitrary but connected) at each active site.
                    owner = {p: j for j, comp in enumerate(comps) for p in comp}
                    core_masks = [0]*len(comps)
                    for i, x in enumerate(X):
                        near = sorted((sum((v-w)**2 for v,w in zip(x,z)), j) for j,z in enumerate(X))
                        d = near[k-1][0]
                        if d < cut.level if opened else d <= cut.level:
                            part = tuple(sorted(j for _,j in near[:k]))
                            core_masks[owner[part]] |= 1 << i
                    expected_records = sorted((sum(1 << i for i in cov), core_masks[j]) for j,cov in enumerate(covered))
                    actual_records = sorted((cov, core) for _v,cov,core in entries)
                    need(expected_records == actual_records, 'core belongs to its exact global coverage component')
                    cuts += 1
                    component_covers += len(comps)
            if k >= 2:
                check_B_meeting(X,a)
                b_meetings += len(X)
            prev_A, prev_B = a, b
        rows.append(dict(name=name, n=len(X), orders=K, catalogue_balls=len(B.balls),
                         cuts=cuts, components=component_covers, strict_trace_checks=local_checks, B_meetings=b_meetings))
    meta = json.loads((ROOT/'SOURCES.json').read_text())
    for source in meta['git_sources']:
        path = ROOT/'git_source'/source['path']
        need(hashlib.sha256(path.read_bytes()).hexdigest() == source['sha256'], 'snapshot dependency hash')
    for source in meta['private_before']:
        path = ROOT/'private_before'/source['path']
        need(hashlib.sha256(path.read_bytes()).hexdigest() == source['sha256'], 'private AST target snapshot hash')
    print(json.dumps(dict(pin=meta['pin'], scope='bounded pure reference/Gram audit only', checks=CHECKS,
                         fixtures=rows, active_orders=sum(r['orders'] for r in rows),
                         closed_and_open_cuts=sum(r['cuts'] for r in rows),
                         B_meetings=sum(r['B_meetings'] for r in rows),
                         result='A/B, complete dynamic strong coverage and B meeting formula conform on fresh bounded cases'),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
