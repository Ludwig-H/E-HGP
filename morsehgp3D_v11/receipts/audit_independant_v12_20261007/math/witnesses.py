#!/usr/bin/env python3
"""Temoins mathematiques de contrelecture; Fraction et Q(sqrt(3)), sans moteur natif."""
from fractions import Fraction as F
from itertools import combinations
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'reference'))
from hgp11_ref.definition import Definition
from hgp11_ref.constructive import Reference
from hgp11_ref.judge import cut_at
from hgp11_ref.supports import Supports


def require(test, message):
    if not test:
        raise RuntimeError(message)


def interval_cuts():
    comparisons = 0
    for n in range(1, 6):
        for sites in combinations(range(5), n):
            levels = sorted({F(abs(x-y), 2) for x in sites for y in sites})
            radii = sorted(set(levels + [(a+b)/2 for a, b in zip(levels, levels[1:])] + [levels[-1]+1]))
            d = Definition([(x, 0, 0) for x in sites])
            for k in range(1, n+1):
                tree = d.order(k)
                for radius in radii:
                    for strict in (False, True):
                        intervals = []
                        for ids in combinations(range(n), k):
                            lo = sites[ids[-1]] - radius
                            hi = sites[ids[0]] + radius
                            if lo < hi or (lo == hi and not strict):
                                intervals.append((lo, hi, sum(1 << i for i in ids)))
                        groups = []
                        for lo, hi, mask in sorted(intervals):
                            if groups and (lo < groups[-1][1] or (lo == groups[-1][1] and not strict)):
                                old = groups[-1]
                                groups[-1] = (old[0], max(old[1], hi), old[2] | mask)
                            else:
                                groups.append((lo, hi, mask))
                        cuts = cut_at(tree, radius*radius)[0 if strict else 1]
                        expected = sorted(g[2] for g in groups)
                        actual = sorted(c[1] for c in cuts)
                        require(actual == expected, ('interval', sites, k, radius, strict, actual, expected))
                        comparisons += 1
    return {'clouds': 31, 'cuts': comparisons, 'status': 'pass'}


class Q3:
    """a+b*sqrt(3); ordre exact par signes et carres rationnels."""
    def __init__(self, a=0, b=0):
        if isinstance(a, Q3):
            self.a, self.b = a.a, a.b
        else:
            self.a, self.b = F(a), F(b)
    def __add__(self, other):
        o = Q3(other)
        return Q3(self.a+o.a, self.b+o.b)
    __radd__ = __add__
    def __neg__(self):
        return Q3(-self.a, -self.b)
    def __sub__(self, other):
        return self + -Q3(other)
    def __rsub__(self, other):
        return Q3(other) + -self
    def __mul__(self, other):
        o = Q3(other)
        return Q3(self.a*o.a+3*self.b*o.b, self.a*o.b+self.b*o.a)
    __rmul__ = __mul__
    def __truediv__(self, other):
        o = Q3(other)
        den = o.a*o.a-3*o.b*o.b
        require(den != 0, 'zero divisor')
        return self*Q3(o.a/den, -o.b/den)
    def sign(self):
        if self.b == 0:
            return (self.a > 0)-(self.a < 0)
        if self.a == 0 or (self.a > 0) == (self.b > 0):
            return 1 if self.b > 0 else -1
        delta = self.a*self.a-3*self.b*self.b
        return ((delta > 0)-(delta < 0)) * (1 if self.a > 0 else -1)
    def __lt__(self, other):
        return (self-other).sign() < 0
    def __le__(self, other):
        return (self-other).sign() <= 0
    def __eq__(self, other):
        o = Q3(other)
        return self.a == o.a and self.b == o.b
    def pair(self):
        return [str(self.a), str(self.b)]


def six_points():
    s = Q3(0, 1)
    points = [(-s, Q3(1)), (-s, Q3(-1)), (Q3(0), Q3(0)),
              (Q3(2), Q3(0)), (2+s, Q3(1)), (2+s, Q3(-1))]
    def dist(a, b):
        return sum((x-y)*(x-y) for x, y in zip(a, b))
    def meb(ids):
        candidates = [(points[i], Q3()) for i in ids]
        for i, j in combinations(ids, 2):
            c = tuple((a+b)/2 for a, b in zip(points[i], points[j]))
            candidates.append((c, dist(c, points[i])))
        for i, j, k in combinations(ids, 3):
            a, b, c = [points[x] for x in (i, j, k)]
            u, v = [x-y for x, y in zip(b, a)], [x-y for x, y in zip(c, a)]
            det = 2*(u[0]*v[1]-u[1]*v[0])
            if det == 0:
                continue
            un, vn = sum(x*x for x in u), sum(x*x for x in v)
            center = (a[0]+(un*v[1]-vn*u[1])/det,
                      a[1]+(u[0]*vn-v[0]*un)/det)
            candidates.append((center, dist(center, a)))
        valid = [radius for center, radius in candidates if all(dist(points[i], center) <= radius for i in ids)]
        require(bool(valid), 'no meb')
        return min(valid)
    beta = {ids: meb(ids) for n in range(2, 7) for ids in combinations(range(6), n)}
    a0 = 2+s
    cuts = [Q3(F(37, 10)), a0, Q3(F(15, 4)), Q3(4)]
    answers = []
    for a in cuts:
        active = [ids for ids in combinations(range(6), 2) if beta[ids] <= a]
        parents = {ids: ids for ids in active}
        def find(x):
            while parents[x] != x:
                x = parents[x]
            return x
        for ids in combinations(range(6), 3):
            if beta[ids] <= a:
                faces = list(combinations(ids, 2))
                for f in faces[1:]:
                    parents[find(f)] = find(faces[0])
        b0 = len({find(x) for x in active})
        counts = [sum(beta[ids] <= a for ids in combinations(range(6), n)) for n in range(2, 7)]
        chi = sum((-1)**(n-2)*(n-1)*counts[n-2] for n in range(2, 7))
        answers.append({'a': a.pair(), 'cech_counts_2_to_6': counts, 'b0': b0, 'chi': chi, 'b1': b0-chi})
    require([x['b1'] for x in answers] == [0, 2, 2, 0], answers)
    # L'absence de valeur de MEB dans (2+sqrt3, 4) rend les nombres constants sur toute la plage.
    levels_inside = [v for v in beta.values() if a0 < v and v < 4]
    require(not levels_inside, 'unexpected level inside hole interval')
    return {'cuts': answers, 'critical_gap_certified': True,
            'scope': 'Projection sur le plan est une retraction de Omega_2 dans R3; beta2=0 pour la region plane.'}


def support_limits():
    # Fixture geometrique de 7 sites : quatre sommets du tetraedre et trois
    # interieurs, docs/MATHEMATIQUES.md:901 et reference/test_supports.py:80.
    # Les groupes ne sont pas codes : Supports(pts).canonical(5) les reconstruit
    # depuis l'oracle A. Le selecteur DSU ci-dessous est abstrait, pas natif.
    pts = [(20, 20, 20), (20, 0, 0), (0, 20, 0), (0, 0, 20),
           (10, 10, 10), (11, 10, 10), (10, 11, 10)]
    out = Supports(pts).canonical(5)
    groups = [b['prior'] for b in out['balls'] if b['level'] == '800/3' and b['role'] == 'fusion']
    require(len(groups) == 4 and all(len(g) == 3 for g in groups), groups)
    parents = {v: v for g in groups for v in g}
    def find(x):
        while parents[x] != x:
            x = parents[x]
        return x
    kept = []
    successful = []
    for g in groups:
        n = 0
        for v in g[1:]:
            a, b = find(g[0]), find(v)
            if a != b:
                parents[b] = a
                n += 1
        if n:
            kept.append(g)
            successful.append(n)
    require(len(kept) == 3 and sum(successful) == 5, (kept, successful))
    cycle_rank = sum(map(len, kept)) - len(parents) - len(kept) + 1
    require(cycle_rank == 1, cycle_rank)
    growth = Supports([(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)]).canonical(3)
    internal = [b for b in growth['balls'] if b['level'] == '25' and b['role'] == 'interne']
    require(len(internal) == 1 and internal[0]['p'] + internal[0]['m'] == 4, internal)
    return {'tetrahedron_k5': {'hyperedges_all': groups, 'kept': kept,
            'useful_unions': successful, 'incidence_graph_cycle_rank': cycle_rank},
            'growth_abcz': {'internal_ball_level': '25', 'population': 4,
                            'conclusion': 'MST sans boules internes ne reconstruit pas la croissance du polyedre.'}}


def d2():
    pts = [(2, 10, 0), (18, 10, 0), (10, 20, 0), (9, 3, 0), (11, 3, 0)]
    d, r = Definition(pts), Reference(pts, 2)
    level = F(1681, 25)
    admitted = sorted({b.level for b in r.balls})
    preceding = max(a for a in admitted if a < level)
    require(preceding == 41 and d.beta((0, 1)) == 64, (preceding, d.beta((0, 1))))
    return {'catalogue_preceding_level': '41', 'strict_trace_birth': '64', 'event': str(level)}


def main():
    out = {'source_commit': '33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae',
           'engine_execution': False, 'gcp_used': False,
           'intervals': interval_cuts(), 'six_points_exact': six_points(),
           'supports_semantics': support_limits(), 'd2': d2()}
    print(json.dumps(out, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
