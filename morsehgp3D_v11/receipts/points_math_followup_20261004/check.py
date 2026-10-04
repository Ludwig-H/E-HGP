#!/usr/bin/env python3
"""Exact bounded witnesses for two additions to the points discussion.

This is an independent Gram/MEB/Gamma_2 calculation, with no product import,
native execution or fit. README proves the infinite counterexample, not these
finite tests. Locally finite alone does not make a bottleneck infimum attained;
the example is deterministic and says nothing about its probability for PPP.
The other example discriminates kappa=1/2 on the same coverage profile.
"""
from fractions import Fraction as Q
from itertools import combinations
import json

CHECKS = 0


def need(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(message)


def dot(x, y):
    return sum(a*b for a, b in zip(x, y))


def sphere(points):
    a = points[0]
    if len(points) == 1:
        return a, Q(0)
    v = tuple(b-c for b, c in zip(points[1], a))
    if len(points) == 2:
        center = tuple(c+d/2 for c, d in zip(a, v))
    else:
        w = tuple(b-c for b, c in zip(points[2], a))
        vv, vw, ww = dot(v, v), dot(v, w), dot(w, w)
        det = vv*ww-vw*vw
        if not det:
            return None
        s, t = (vv*ww-vw*ww)/(2*det), (ww*vv-vw*vv)/(2*det)
        center = tuple(c+s*d+t*e for c, d, e in zip(a, v, w))
    delta = tuple(c-d for c, d in zip(center, a))
    return center, dot(delta, delta)


class Gamma:
    def __init__(self, points):
        self.points = tuple(tuple(map(Q, p)) for p in points)
        self.pairs = tuple(combinations(range(len(points)), 2))
        self.triples = tuple(combinations(range(len(points)), 3))
        self.beta = {}
        for part in self.pairs + self.triples:
            choices = []
            for arity in range(1, len(part)+1):
                for support in combinations(part, arity):
                    candidate = sphere([self.points[i] for i in support])
                    if candidate is None:
                        continue
                    center, beta = candidate
                    if all(dot(tuple(x-y for x, y in zip(self.points[i], center)),
                               tuple(x-y for x, y in zip(self.points[i], center))) <= beta
                           for i in part):
                        choices.append(beta)
            self.beta[part] = min(choices)

    def partition(self, beta):
        parent = {p: p for p in self.pairs if self.beta[p] <= beta}

        def find(p):
            while parent[p] != p:
                p = parent[p]
            return p

        for part in self.triples:
            if self.beta[part] <= beta:
                faces = list(combinations(part, 2))
                for f in faces[1:]:
                    parent[find(f)] = find(faces[0])
        groups = {}
        for p in parent:
            groups.setdefault(find(p), set()).update(p)
        return sorted(tuple(sorted(g)) for g in groups.values())

    def meet(self, p, q):
        parent = {f: f for f in self.pairs}

        def find(f):
            while parent[f] != f:
                f = parent[f]
            return f

        for part in sorted(self.triples, key=lambda t: self.beta[t]):
            faces = list(combinations(part, 2))
            for f in faces[1:]:
                parent[find(f)] = find(faces[0])
            if find(p) == find(q):
                return self.beta[part]
        raise RuntimeError("finite Gamma disconnected")


def pattern(N):
    points = [(Q(0), Q(0), Q(0))]
    upper, lower = [], []
    for n in range(-N, N+1):
        a, b = Q(1, 10*(abs(n)+1)), Q(1, 100*(abs(n)+1)**2)
        for x, y, group in [(Q(n, 2)+a, 1, upper),
                            (Q(n, 2)+a+b, 1, upper),
                            (Q(n, 2)-a, -1, lower),
                            (Q(n, 2)-a-b, -1, lower)]:
            group.append(len(points))
            points.append((x, Q(y), Q(0)))
    return points, upper, lower


def main():
    finite = []
    t2 = Q(10121, 40000)
    need(Q(61, 200) < Q(1, 3) and Q(1, 9) < t2,
         "all row triples connect before first qualification")
    need((1+Q(39, 100)**2)/4 > t2,
         "no site from another row group is incident at first qualification")
    previous = None
    for N in (1, 2):
        points, upper, lower = pattern(N)
        need(len(set(points)) == len(points), "distinct finite prefix")
        need(set(points) == {tuple(-a for a in p) for p in points}, "half-turn symmetry")
        gamma = Gamma(points)
        a = points.index((Q(1, 10), Q(1), Q(0)))
        b = points.index((Q(11, 100), Q(1), Q(0)))
        c = points.index((Q(-1, 10), Q(-1), Q(0)))
        d = points.index((Q(-11, 100), Q(-1), Q(0)))
        need(gamma.beta[tuple(sorted((0, a, b)))] == t2, "exact first upper qualification")
        need(gamma.beta[tuple(sorted((0, c, d)))] == t2, "exact reflected qualification")
        before = (Q(101, 400)+t2)/2
        need(not any(0 in g and len(g) >= 3 for g in gamma.partition(before)),
             "qualification not before t")
        wanted = sorted([tuple(sorted([0]+upper)), tuple(sorted([0]+lower))])
        for beta in (t2, Q(1)):
            actual = [g for g in gamma.partition(beta) if 0 in g and len(g) >= 3]
            need(actual == wanted, "two qualified origin owners at closed cut")
        mu = gamma.meet(tuple(sorted((0, a))), tuple(sorted((0, c))))
        need(mu > 1, "finite prefix has a genuinely attained meeting above one")
        if previous is not None:
            need(mu < previous, "meetings decrease as farther bridges are admitted")
        previous = mu
        finite.append(dict(prefix=N, sites=len(points), first_qualified_beta=str(t2),
                           meeting_beta=str(mu), origin_owners_at_one=2,
                           kappa1_entry_beta=str(mu), kappa2_entry_beta=str(mu)))

    bridges = []
    for n in (1, 2, 4, 8, 16):
        a, b = Q(1, 10*(n+1)), Q(1, 100*(n+1)**2)
        points = [(Q(n, 2)+a, 1, 0), (Q(n, 2)+a+b, 1, 0),
                  (Q(n, 2)-a, -1, 0)]
        gamma = Gamma(points)
        beta = 1+(a+b/2)**2
        need(gamma.beta[(0, 1, 2)] == beta and beta > 1, "exact bridging coface above one")
        bridges.append(dict(n=n, beta=str(beta)))
    need(all(Q(x['beta']) > Q(y['beta']) for x, y in zip(bridges, bridges[1:])),
         "bridging upper bounds decrease")

    # Same finite profile, different kappa: future qualified rival in R1.
    points = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0),
              (44, 0, 0), (54, 0, 0), (-20, 10, 0), (-20, -10, 0)]
    gamma = Gamma(points)
    need(gamma.beta[(0, 1, 2)] == 100, "R1 first qualified primary")
    need(gamma.beta[(0, 6, 7)] == Q(625, 4), "R1 rival qualification")
    need(gamma.meet((0, 1), (0, 6)) == 250, "R1 qualified rival meeting")
    # Every event on every qualified origin branch is included, not just this rival.
    for beta in sorted(set(gamma.beta.values())):
        qualified = [g for g in gamma.partition(beta) if 0 in g and len(g) >= 3]
        expected = 0 if beta < 100 else 1 if beta < Q(625, 4) else 2 if beta < 250 else 1
        need(len(qualified) == expected, "R1 exhaustive qualified origin branch count")
        if Q(625, 4) <= beta < 250:
            need((0, 6, 7) in qualified, "only independent rival is the prescribed branch")
    need(250 > Q(29, 2)**2, "R1 kappa1 entry sqrt250-5/2 exceeds F12")
    need(250 < 17**2 and 250 > 15**2,
         "R1 kappa2 entry sqrt250-5 is between t10 and F12")
    print(json.dumps(dict(scope="finite exact witnesses; infinite proof in README; no PPP claim",
                          checks=CHECKS, finite_prefixes=finite, bridges=bridges,
                          infinite=dict(first_qualified_beta=str(t2), infimum_meeting_radius="1",
                                        meeting_at_one=False, proposed_entry_radius="1",
                                        owners_at_one=2),
                          E1=dict(profile="R1, k2 m3", t="10", rival_birth="25/2",
                                  rival_meeting="sqrt250", kappa1_entry="sqrt250-5/2",
                                  kappa2_entry="sqrt250-5", diagnostic_cut="12",
                                  kappa1_active=False, kappa2_active=True)),
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
