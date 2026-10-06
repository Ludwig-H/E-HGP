#!/usr/bin/env python3
"""Exact independent small oracle. stdlib only; no native/product invocation.

One-dimensional order-k domains are derived from all pairwise distance
inequalities. Their dual vertices/edges and two filtrations are then derived.
The examples also apply to their embedding on an axis in R^3.
"""
from fractions import Fraction as F
from itertools import combinations
import json

CHECKS = 0


def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def interval_intersection(a, b):
    lows = [x for x in (a[0], b[0]) if x is not None]
    highs = [x for x in (a[1], b[1]) if x is not None]
    lo = max(lows) if lows else None
    hi = min(highs) if highs else None
    return None if lo is not None and hi is not None and lo > hi else (lo, hi)


def clamp(x, domain):
    lo, hi = domain
    if lo is not None and x < lo:
        x = lo
    if hi is not None and x > hi:
        x = hi
    return x


def voronoi(points, q):
    domain = (None, None)
    for a in q:
        for b in points:
            if b in q:
                continue
            # (x-a)^2 <= (x-b)^2, after exact cancellation.
            mid = (a + b) / 2
            cut = (None, mid) if b > a else (mid, None)
            domain = interval_intersection(domain, cut)
            if domain is None:
                return None
    return domain


def mean(values):
    return sum(values, F(0)) / len(values)


def max_power(q, x):
    return max((x - a) ** 2 for a in q)


def mean_power(q, x):
    return mean([(x - a) ** 2 for a in q])


def merge_intervals(intervals):
    out = []
    for lo, hi in sorted(intervals):
        if not out or lo > out[-1][1]:
            out.append([lo, hi])
        else:
            out[-1][1] = max(hi, out[-1][1])
    return out


def component_at(x, components):
    hits = [i for i, (lo, hi) in enumerate(components) if lo <= x <= hi]
    need(len(hits) <= 1, "disjoint closed components")
    return hits[0] if hits else None


def oracle(points, k, radius):
    points = tuple(F(x) for x in sorted(points))
    family = list(combinations(points, k))
    full = []
    covers = []
    for q in family:
        cover = (max(q) - radius, min(q) + radius)
        if cover[0] <= cover[1]:
            covers.append(cover)
        domain = voronoi(points, q)
        if domain is None or (domain[0] is not None and domain[0] == domain[1]):
            continue
        bary = mean(q)
        # Minimax of a one-dimensional point set is minimized at its midrange.
        center = clamp((min(q) + max(q)) / 2, domain)
        power_center = clamp(bary, domain)
        clipped = interval_intersection(domain, cover) if cover[0] <= cover[1] else None
        full.append(dict(q=q, domain=domain, bary=bary,
                         alpha=max_power(q, center), witness=center,
                         beta=mean_power(q, power_center), clipped=clipped))
    full.sort(key=lambda v: v["bary"])
    edges = []
    for i, left in enumerate(full):
        for j, right in enumerate(full[i + 1:], i + 1):
            shared = interval_intersection(left["domain"], right["domain"])
            if shared is None:
                continue
            need(shared[0] is not None and shared[0] == shared[1], "1D dual edge face")
            x = shared[0]
            union = tuple(sorted(set(left["q"]) | set(right["q"])))
            need(mean_power(left["q"], x) == mean_power(right["q"], x), "power bisector")
            edges.append(dict(i=i, j=j, face=x, alpha=max_power(union, x),
                              beta=mean_power(left["q"], x)))
    comps = merge_intervals(covers)
    for v in full:
        if v["clipped"] is None:
            v["owner"] = None
        else:
            lo, hi = v["clipped"]
            owner = component_at((lo + hi) / 2, comps)
            need(owner is not None, "clipped Voronoi region belongs to Omega")
            v["owner"] = owner
    return dict(points=points, k=k, radius=radius, vertices=full, edges=edges, omega_axis=comps)


def connected_components(data, key):
    vertices = data["vertices"]
    active = [i for i, v in enumerate(vertices) if v[key] <= data["radius"] ** 2]
    parent = {i: i for i in active}

    def find(i):
        while parent[i] != i:
            i = parent[i]
        return i

    for edge in data["edges"]:
        if edge[key] > data["radius"] ** 2:
            continue
        need(edge["i"] in parent and edge["j"] in parent, "faces precede edge")
        a, b = find(edge["i"]), find(edge["j"])
        parent[b] = a
    return len({find(i) for i in active})


def norm2(x):
    return sum((a * a for a in x), F(0))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def inclusion_checks(points, k, witness, radius2):
    """Probe exact convex mixtures of the exposed nearest-set family.

    This is bounded verification of the variance proof, not a constructor
    of all Delaunay cells or a proof of general homotopy equivalence.
    """
    points = [tuple(F(a) for a in p) for p in points]
    witness = tuple(F(a) for a in witness)
    dist2 = [norm2(sub(p, witness)) for p in points]
    kth = sorted(dist2)[k - 1]
    nearest = [q for q in combinations(range(len(points)), k)
               if sum(dist2[i] for i in q) == sum(sorted(dist2)[:k])]
    need(kth <= radius2, "witness covered k times")
    probes = [((q, F(1)),) for q in nearest]
    probes += [((a, F(1, 2)), (b, F(1, 2))) for a, b in combinations(nearest, 2)]
    for mixture in probes:
        mu = [F(0) for _ in points]
        for q, weight in mixture:
            for i in q:
                mu[i] += weight
        need(all(0 <= a <= 1 for a in mu) and sum(mu) == k, "fractional k-selection")
        x = tuple(sum(mu[i] * points[i][axis] for i in range(len(points))) / k for axis in range(3))
        weighted_x = sum(mu[i] * norm2(sub(x, points[i])) for i in range(len(points))) / k
        weighted_witness = sum(mu[i] * dist2[i] for i in range(len(points))) / k
        need(weighted_x == weighted_witness - norm2(sub(x, witness)), "variance identity")
        at_x = sorted(norm2(sub(x, p)) for p in points)
        need(sum(at_x[:k]) / k <= weighted_x <= radius2, "A subset DTM sublevel")
        need(norm2(sub(x, witness)) <= radius2, "A subset Omega plus radius")
        need(at_x[k - 1] <= 4 * radius2, "A subset Omega at twice radius")
    # Reverse Hausdorff inclusion: a nearest-set barycentre is active with
    # this witness; Jensen bounds its distance to the witness by the radius.
    for q in nearest:
        bary = tuple(sum(points[i][axis] for i in q) / k for axis in range(3))
        need(norm2(sub(bary, witness)) <= radius2, "Omega witness within r of active barycentre")
    return dict(probes=len(probes), nearest_sets=len(nearest), radius_squared=radius2)


def main():
    one = oracle([0, 1, 10], 3, F(5))
    need(len(one["vertices"]) == 1 and not one["edges"], "all-sites mosaic has one vertex")
    vertex = one["vertices"][0]
    need(vertex["bary"] == F(11, 3) and vertex["alpha"] == 25, "original example derived")
    need(one["omega_axis"] == [[F(5), F(5)]], "original Omega axis")
    need(max_power(one["points"], vertex["bary"]) == F(361, 9) > 25, "A not subset Omega")
    variance = mean_power(one["points"], vertex["bary"])
    need(variance == F(182, 9), "original DTM variance")
    need(F(25) - variance == F(43, 9), "DTM ball radius squared differs")

    wrong = oracle([0, 1, 2, 11], 3, F(21, 4))
    need([v["q"] for v in wrong["vertices"]] == [(F(0), F(1), F(2)), (F(1), F(2), F(11))], "full Voronoi labels derived")
    need(len(wrong["edges"]) == 1, "one dual edge")
    edge = wrong["edges"][0]
    need(edge["face"] == F(11, 2), "shared face derived")
    need(edge["alpha"] == F(121, 4), "HGP merge squared level")
    need(edge["beta"] == F(251, 12), "DTM merge squared level")
    need(edge["beta"] < wrong["radius"] ** 2 < edge["alpha"], "DTM already merged, HGP separate")
    need(connected_components(wrong, "alpha") == 2 and connected_components(wrong, "beta") == 1, "different component counts")
    need(len(wrong["omega_axis"]) == 2, "Omega axis disconnected")
    right = wrong["vertices"][1]
    geometric_owner = component_at(right["bary"], wrong["omega_axis"])
    need(right["owner"] == 1 and geometric_owner == 0, "barycentre lies in wrong component")
    need(max_power(wrong["vertices"][0]["q"], right["bary"]) < wrong["radius"] ** 2, "wrong-component membership strict in R3")
    need(wrong["omega_axis"][0][1] < wrong["omega_axis"][1][0], "projection separates R3 components")
    wrong["geometric_owner_of_right_barycentre"] = geometric_owner
    wrong["component_counts"] = dict(HGP=2, DTM=1)

    geometric = [
        inclusion_checks([(0, 0, 0), (1, 0, 0), (10, 0, 0)], 3, (5, 0, 0), F(25)),
        inclusion_checks([(0, 0, 0), (1, 0, 0), (2, 0, 0), (11, 0, 0)], 3, (6, 0, 0), F(25)),
        inclusion_checks([(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)], 2, (0, 0, 0), F(3)),
        inclusion_checks([(1, 0, 0), (0, 1, 0), (-1, 0, 0), (0, -1, 0)], 2, (0, 0, 0), F(1)),
    ]
    return dict(verdict="conforme", checks=CHECKS, original=one,
                original_DTM_ball_radius_squared=F(43, 9),
                wrong_component_and_early_DTM_merge=wrong, geometric_probes=geometric,
                limits=["independent 1D Voronoi/filtration oracle and bounded variance probes",
                        "axis examples valid in R3, not generic 3D mosaics",
                        "no native product, LiDAR, GCP, scipy or general mesh constructor",
                        "general homotopy/Hausdorff statements require the separate mathematical proof"])


if __name__ == "__main__":
    print(json.dumps(main(), default=lambda x: str(x) if isinstance(x, F) else x, sort_keys=True, indent=2))
