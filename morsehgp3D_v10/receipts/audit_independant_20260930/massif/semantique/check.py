#!/usr/bin/env python3
"""Bounded exact mathematics, independent of the native/reference HGP code.

Gamma_2 on collinear points: pair vertices; triple MEB links all three faces.
Every activation at the queried squared-radius level is processed before export.
The center-box bound uses the convexity of squared distance on a box, not
sampled KNN values as a certificate for the whole box.
"""
from fractions import Fraction as F
from itertools import combinations, product
import json


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def show(q):
    q = F(q)
    return str(q.numerator) if q.denominator == 1 else f"{q.numerator}/{q.denominator}"


def beta_line(values, indices):
    span = max(values[i] for i in indices) - min(values[i] for i in indices)
    return F(span * span, 4)


def gamma_cut(values, beta, closed=True):
    active = lambda b: b <= beta if closed else b < beta
    pairs = [p for p in combinations(range(len(values)), 2) if active(beta_line(values, p))]
    parent = {p: p for p in pairs}

    def find(p):
        while parent[p] != p:
            p = parent[p]
        return p

    for triple in combinations(range(len(values)), 3):
        if not active(beta_line(values, triple)):
            continue
        faces = list(combinations(triple, 2))
        need(all(p in parent for p in faces), "A triple must have all active pair faces")
        for p in faces[1:]:
            a, b = find(faces[0]), find(p)
            parent[max(a, b)] = min(a, b)
    groups = {}
    for p in pairs:
        groups.setdefault(find(p), []).append(p)
    return sorted([sorted(ps) for ps in groups.values()])


def point_sets(groups):
    return [sorted({i for p in group for i in p}) for group in groups]


def critical_pairs(values, kcat):
    records = []
    for i, j in combinations(range(len(values)), 2):
        center = F(values[i] + values[j], 2)
        beta = beta_line(values, (i, j))
        interior = [a for a, x in enumerate(values) if (F(x) - center) ** 2 < beta]
        shell = [a for a, x in enumerate(values) if (F(x) - center) ** 2 == beta]
        need(shell == [i, j], "Fixtures have complete, regular two-site shells")
        if len(interior) + 2 <= kcat + 1:
            records.append({"support": [i, j], "interior": interior, "shell": shell,
                            "qmin": 2, "beta": show(beta),
                            "lo": len(interior) + 1, "hi": min(kcat, len(interior) + 2)})
    return records


def encoded(groups):
    return {"facets": [[list(p) for p in ps] for ps in groups], "covered_points": point_sets(groups)}


def shared_point_fixture():
    values = [0, 1, 2]
    early = gamma_cut(values, F(1, 4))
    before_merge = gamma_cut(values, F(1), closed=False)
    merged = gamma_cut(values, F(1))
    need(early == [[(0, 1)], [(1, 2)]], "Two isolated Gamma vertices expected")
    need(set(point_sets(early)[0]) & set(point_sets(early)[1]) == {1}, "Covers share point1")
    need(len(before_merge) == 2 and len(merged) == 1, "First true fusion at beta1")
    cat = critical_pairs(values, 2)
    need(len(cat) == 3 and next(b for b in cat if b["support"] == [0, 2])["interior"] == [1],
         "Kcat2 retains fusion sphere")
    return {"points": values, "k": 2, "early_beta": "1/4", "early": encoded(early),
            "fusion_beta": "1", "open_at_fusion": encoded(before_merge),
            "closed_at_fusion": encoded(merged), "catalogue_kcat2": cat}


def gap_fixture():
    # Formula valid for every real/integer L>1; the receipt evaluates L=10 only.
    length = 10
    values = [0, 1, length, length + 1]
    birth = F((length - 1) ** 2, 4)
    fusion = F(length ** 2, 4)
    local = gamma_cut(values, F(1, 4))
    bridge = gamma_cut(values, birth)
    before_merge = gamma_cut(values, fusion, closed=False)
    merged = gamma_cut(values, fusion)
    need(local == [[(0, 1)], [(2, 3)]], "Two local births expected")
    need(bridge == [[(0, 1)], [(1, 2)], [(2, 3)]], "Bridge is an additional birth")
    need(len(before_merge) == 3 and len(merged) == 1, "Atomic plateau must have three parents")
    fusion_triples = [list(t) for t in combinations(range(4), 3) if beta_line(values, t) == fusion]
    need(fusion_triples == [[0, 1, 2], [1, 2, 3]], "Two simultaneous triple witnesses")
    cat2, cat3 = critical_pairs(values, 2), critical_pairs(values, 3)
    fusion_balls = [b for b in cat2 if F(b["beta"]) == fusion]
    need(len(cat2) == 5 and len(cat3) == 6 and len(fusion_balls) == 2, "Admission counts")
    need(all(len(b["interior"]) + b["qmin"] == 3 for b in fusion_balls), "K+1 fusion admission")
    coverage_only = [b for b in cat2 if len(b["interior"]) + b["qmin"] <= 2]
    need(len(coverage_only) == 3 and all(F(b["beta"]) < fusion for b in coverage_only),
         "Coverage-only universe deliberately omits fusion balls")
    return {"points": values, "k": 2, "local_beta": "1/4", "local": encoded(local),
            "bridge_birth_beta": show(birth), "bridge_birth": encoded(bridge),
            "fusion_beta": show(fusion), "open_at_fusion": encoded(before_merge),
            "closed_at_fusion": encoded(merged), "parent_count": 3,
            "fusion_triples": fusion_triples, "fusion_balls": fusion_balls,
            "catalogue_kcat2": cat2, "catalogue_kcat3_count": len(cat3),
            "coverage_only_k2_count": len(coverage_only),
            "fixed_halo_failure_condition": "H<(L-1)/2 for two tiles confined around their own pairs"}


def distance2(a, b):
    return sum((F(x) - F(y)) ** 2 for x, y in zip(a, b))


def box_bound_fixture():
    low, high = (4, 0, 0), (6, 1, 1)
    points = [(0, 0, 0), (10, 0, 0), (5, 0, 0), (5, 2, 0), (30, 0, 0), (4, 1, 1)]
    anchors = [0, 1]  # Any two distinct sites suffice for Kcat=2 in the unweighted object.
    corners = list(product(*[(a, b) for a, b in zip(low, high)]))
    radius2 = max(distance2(points[s], corner) for s in anchors for corner in corners)

    def box_distance2(point):
        return sum((F(a) - F(x)) ** 2 if x < a else (F(x) - F(b)) ** 2 if x > b else F(0)
                   for x, a, b in zip(point, low, high))

    local = [s for s, p in enumerate(points) if box_distance2(p) <= radius2]
    need(radius2 == 38 and local == [0, 1, 2, 3, 5], "Exact bound/local list expected")
    samples = list(product(*[(F(a), F(a + b, 2), F(b)) for a, b in zip(low, high)]))
    tied_queries = 0
    for center in samples:
        distances = sorted((distance2(p, center), s) for s, p in enumerate(points))
        kth = distances[1][0]
        closed = [s for d, s in distances if d <= kth]
        need(kth <= radius2 and set(closed) <= set(local), "Global closed2NN included")
        need(all(distance2(points[s], center) <= radius2 for s in anchors), "Anchor bound")
        tied_queries += len(closed) > 2
    return {"kcat": 2, "box_low": list(low), "box_high": list(high),
            "points": [list(p) for p in points], "anchors": anchors,
            "radius2": show(radius2), "closed_local_list": local,
            "sampled_query_checks": len(samples), "queries_with_more_than_k_ties": tied_queries,
            "proof": "Squared distance is convex in each coordinate: max on Q occurs at a corner. "
                     "Every c in Q has both chosen sites within R_Q; hence d_2(c)<=R_Q. "
                     "If x in closed N_2(c), dist(x,Q)<=dist(x,c)<=R_Q, including all ties.",
            "sample_limit": "The 27 sampled centers illustrate the exact calculation; they are not its universal proof."}


result = {"status": "ok", "arithmetic": "fractions.Fraction", "shared_point": shared_point_fixture(),
          "gap": gap_fixture(), "center_box_bound": box_bound_fixture(),
          "limits": ["No native execution, GPU, performance or massive-data qualification.",
                     "Gamma calculation is complete for these collinear K2 fixtures only.",
                     "The halo certificate proves inclusion, not a small local-list size or I/O bound.",
                     "Kcat=Kmax suffices for admission p+qmin<=Kmax+1; coverage p+qmin<=K is another universe."]}
print(json.dumps(result, sort_keys=True, separators=(",", ":")))
