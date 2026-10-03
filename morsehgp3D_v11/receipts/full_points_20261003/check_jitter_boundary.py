#!/usr/bin/env python3
"""Exact boundary perturbations, research consumer only; no native or fit.

The source is the six-site equilateral integer fixture, translated by +2 then
scaled by 1000. Thirty-two draws, fixed seed 867933, move every coordinate by
exactly -1 or +1 grid unit, hence every labelled site by sqrt(3). The independent
Gram/Gamma encoder comes from the adjacent bounded consumer test, not a product
geometry implementation. IoU is diagnostic and is never claimed Lipschitz.

Exact redundancy, unit sites and fixed k: each Gamma_k component covers the
union of its k-parties, hence at least k sites, so m<=k qualifies every cover.
For m=k+1 a qualified component has at least TWO Gamma vertices. A spanning
tree's edges are active (k+1)-cofaces; their site sets cover that component and
are connected by shared k-parties. They therefore generate the same point block
under the unfiltered upper (k+1) covers. Conversely every upper component maps
into one qualified lower component by the Gamma/FULL vertical inclusion. Both
refinements imply identical partitions, active sites, entries and merge heights:
closure(k,k+1)=closure(k+1,k+1). This is not a union-of-K laminarity claim.
For m>k+1 there is no such identity; the line 0,2,4 at beta1 versus beta4 is an
explicit counterexample to identifying k1/m3 with k3/m3.
"""
from fractions import Fraction
from itertools import combinations
import hashlib
import json
from pathlib import Path
import random
import sys

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE / "experiment"))
from qualified import NONE, analyse, load, members, tree_lca  # noqa: E402
from test_qualified import Oracle  # noqa: E402

SEED, DRAWS = 867933, 32
POINTS = tuple(tuple(1000*(x+2) for x in point) for point in
               ((-1, -1, 0), (-1, 0, -1), (0, 0, 0),
                (1, 1, 0), (2, 2, 0), (2, 1, 1)))
LABELS = [0, 0, 0, 1, 1, 1]
METHODS = ("qualified_m3", "core", "first_cover_canonical", "first_cover_lca_all_ties")
CHECKS = 0


def require(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def radius_bound(a, b, epsilon_squared=3):
    """|sqrt(a)-sqrt(b)| <= sqrt(epsilon_squared), without square roots."""
    high, low = sorted((Fraction(a), Fraction(b)), reverse=True)
    delta = high-low-epsilon_squared
    return delta <= 0 or delta*delta <= 4*epsilon_squared*low


def project(points):
    require(len(set(points)) == 6, "same six distinct unit sites")
    require(all(0 <= value < 2**21 for point in points for value in point), "u21 domain")
    model = Oracle(points, 3)
    data = load(model.encode())
    answer = analyse(data, LABELS, orders=(2, 3), thresholds=(3,))
    return {k: {"qualified_m3": row["qualified"]["3"],
                **{name: row[name] for name in METHODS[1:]}}
            for k, row in answer["orders"].items()}


def signature(result):
    tree = result["tree"]
    groups = []
    for node in range(tree["leaves"], len(tree["height"])):
        parent = tree["parent"][node]
        require(parent == NONE or Fraction(tree["height"][node]) < Fraction(tree["height"][parent]),
                "no zero-duration internal point branch")
        group = members(tree, node)
        if len(group) < tree["leaves"]:
            groups.append("".join(chr(65+i) for i in group))
    return sorted(groups)


def pair_heights(result):
    tree = result["tree"]
    answer = {}
    for i, j in combinations(range(6), 2):
        node = tree_lca(tree, i, j)
        require(node is not None, "all six sites eventually connect")
        answer[chr(65+i)+chr(65+j)] = tree["height"][node]
    return answer


def description(result):
    return dict(nontrivial_branches=signature(result), pair_heights=pair_heights(result),
                entry_dates=result["entry_dates"], best_iou=result["best_iou"],
                target_fusion_heights=result["selected_group_fusion_heights"])


def close_covers(covers, n, m):
    parent = list(range(n))
    active = set()

    def find(site):
        while parent[site] != site:
            site = parent[site]
        return site

    for cover in covers:
        if len(cover) >= m:
            active.update(cover)
            anchor = min(cover)
            for site in cover:
                parent[find(site)] = find(anchor)
    groups = {}
    for site in active:
        groups.setdefault(find(site), set()).add(site)
    # Inactive singleton completion is identical on the two sides only after
    # checking these active blocks; inactive points never enter the IoU score.
    return {frozenset(group) for group in groups.values()}


def identities():
    fixtures = {
        "line024": [(0, 0, 0), (2, 0, 0), (4, 0, 0)],
        "line01269": [(x, 0, 0) for x in (0, 1, 2, 6, 9)],
        "square": [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)],
        "octa": [(15, 10, 10), (5, 10, 10), (10, 15, 10), (10, 5, 10),
                 (10, 10, 15), (10, 10, 5)],
        "tetra_center": [(0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2), (1, 1, 1)],
        "cube": [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)],
        "generic6": [(0, 0, 0), (9, 1, 0), (2, 8, 1), (5, 5, 7), (1, 3, 9), (8, 8, 8)],
        "equilateral": POINTS,
    }
    output = []
    for name, points in fixtures.items():
        n = len(points)
        model = Oracle(points, n)
        count = 0
        for beta in model.levels:
            covers = {k: [set(i for part in group for i in part) for group in model.gamma(k, beta)]
                      for k in range(1, n+1)}
            for k in range(1, n+1):
                require(all(len(cover) >= k for cover in covers[k]), "a Gamma component covers at least k sites")
                unfiltered = close_covers(covers[k], n, 1)
                for m in range(1, k+1):
                    require(close_covers(covers[k], n, m) == unfiltered,
                            "m<=k has identical active partition")
                    count += 1
                if k < n:
                    require(close_covers(covers[k], n, k+1) == close_covers(covers[k+1], n, k+1),
                            "closure(k,k+1)=closure(k+1,k+1), including active sites")
                    count += 1
        output.append(dict(name=name, sites=n, cuts=len(model.levels), identity_checks=count))
    chain = Oracle(fixtures["line024"], 3)
    require(chain.qualified_groups(1, Fraction(1), 3) == {frozenset(range(3))},
            "k1 m3 chain qualifies at beta1")
    require(chain.qualified_groups(3, Fraction(1), 3) == set() and
            chain.qualified_groups(3, Fraction(4), 3) == {frozenset(range(3))},
            "k3 m3 chain first qualifies at beta4")
    return output


def main():
    identity_cases = identities()
    baseline = project(POINTS)
    original = {k: {name: description(result) for name, result in order.items()}
                for k, order in baseline.items()}
    require(original["2"]["qualified_m3"]["nontrivial_branches"] == ["ABC", "DEF"],
            "qualified preserves exact equilateral targets")
    require(original["2"]["first_cover_lca_all_ties"]["nontrivial_branches"] == ["AB", "EF"],
            "original first-cover LCA loses both target triangles")
    require(all(original[k]["qualified_m3"]["best_iou"][str(t)]["iou_exact"] == "1/1"
                for k in ("2", "3") for t in (0, 1)), "original qualified perfect target IoU")
    rng = random.Random(SEED)
    trials = []
    counters = {k: {name: dict(changed_branches=0, changed_iou=0,
                               pair_radius_bound_violations=0, entry_radius_bound_violations=0,
                               core_two_epsilon_violations=0)
                    for name in METHODS} for k in ("2", "3")}
    bound_checks = 0
    for draw in range(DRAWS):
        offsets = tuple(tuple(1 if rng.getrandbits(1) else -1 for _ in range(3)) for _ in range(6))
        points = tuple(tuple(x+delta for x, delta in zip(point, offset))
                       for point, offset in zip(POINTS, offsets))
        require(all(sum(delta*delta for delta in offset) == 3 for offset in offsets),
                "paired labelled displacement exactly sqrt(3)")
        current = project(points)
        changes = {}
        for k in ("2", "3"):
            changes[k] = {}
            for name in METHODS:
                old, new = original[k][name], description(current[k][name])
                pairs = [pair for pair in old["pair_heights"]
                         if not radius_bound(old["pair_heights"][pair], new["pair_heights"][pair])]
                entries = [i for i, (left, right) in enumerate(zip(old["entry_dates"], new["entry_dates"]))
                           if not radius_bound(left, right)]
                changed_iou = any(old["best_iou"][str(t)]["iou_exact"] != new["best_iou"][str(t)]["iou_exact"]
                                  for t in (0, 1))
                changed_branches = old["nontrivial_branches"] != new["nontrivial_branches"]
                counter = counters[k][name]
                counter["changed_branches"] += changed_branches
                counter["changed_iou"] += changed_iou
                counter["pair_radius_bound_violations"] += len(pairs)
                counter["entry_radius_bound_violations"] += len(entries)
                if name == "qualified_m3":
                    require(not pairs and not entries, "qualified entry/pair radius epsilon bound")
                    bound_checks += 21
                if name == "core":
                    own_pairs = [pair for pair in old["pair_heights"]
                                 if not radius_bound(old["pair_heights"][pair], new["pair_heights"][pair], 12)]
                    own_entries = [i for i, (left, right) in enumerate(zip(old["entry_dates"], new["entry_dates"]))
                                   if not radius_bound(left, right, 12)]
                    require(not own_pairs and not own_entries, "core own two-epsilon entry/merge bound")
                    counter["core_two_epsilon_violations"] += len(own_pairs)+len(own_entries)
                changes[k][name] = dict(nontrivial_branches=new["nontrivial_branches"],
                    iou={str(t): new["best_iou"][str(t)]["iou_exact"] for t in (0, 1)},
                    pair_radius_bound_violations=pairs, entry_radius_bound_violations=entries,
                    pair_heights=new["pair_heights"], entry_dates=new["entry_dates"])
        # Exact redundancy of these two settings, not merely a tolerance match.
        for field in ("pair_heights", "entry_dates"):
            require(changes["2"]["qualified_m3"][field] == changes["3"]["qualified_m3"][field],
                    "K2 m3 equals K3 m3 at labelled sites")
        trials.append(dict(draw=draw, offsets=offsets, methods=changes))
    require(counters["2"]["first_cover_lca_all_ties"]["changed_branches"] > 0,
            "perturbation actually exercises first-cover LCA branch discontinuity")
    dependencies = {str(path.relative_to(HERE)): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in (HERE / "experiment" / "qualified.py", HERE / "experiment" / "test_qualified.py")}
    print(json.dumps(dict(status="pass", checks=CHECKS, seed=SEED, draws=DRAWS,
        epsilon_squared=3, qualified_radius_checks=bound_checks, points=POINTS,
        unit="integer millimetre grid; all reported heights are squared grid radii",
        original=original, counts=counters, trials=trials, dependencies=dependencies,
        identity_cases=identity_cases,
        identity_proof="m<=k: every cover contains a k-part. m=k+1: lower qualified component has >=2 Gamma vertices; a spanning tree of active (k+1)-cofaces covers and connects all its sites. Upper FULL inclusion gives the reverse refinement. Active partitions, entries and heights agree.",
        nonidentity_counterexample=dict(points=[0, 2, 4], k1_m3_entry_beta="1/1", k3_m3_entry_beta="4/1"),
        bound_policy="epsilon bound asserted for qualified only; core own 2epsilon separately checked; first-cover baselines have no asserted uniform bound",
        metric_policy="active groups only; inactive singletons complete the total partition but are excluded from IoU",
        scope="bounded exact perturbations; no native, sklearn fit, GCP or IoU stability claim",
        first_cover_choice="synthetic encoder deterministic first record; no native canonical choice qualification",
        identity="same six labelled sites; no deduplication or lost IDs"), sort_keys=True))


if __name__ == "__main__":
    main()
