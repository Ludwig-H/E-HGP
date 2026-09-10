#!/usr/bin/env python3
"""Corpus differentiel de nuages entiers aleatoires : juge rationnel independant
contre build_full_ball_tower (overlay WIP 13:04 UTC) via tower_bridge.

Le juge ne lit aucun en-tete C++ : geometrie par elimination de Gram sur
Fraction (logique copiee en lecture de meb_rational_oracle_20260905.circumball),
catalogue complet des MEB de tous les sous-ensembles, balayage incremental des
composantes Gamma par union-find, images verticales certifiees a chaque
activation et fusion. Aucun assert : refus explicites par code de sortie.
Sortie stdout deterministe (sort_keys, aucun temps) ; progression sur stderr.
"""

from __future__ import annotations

import argparse
from fractions import Fraction as Q
import json
import math
import random
import subprocess
import sys

Point = tuple[int, int, int]
MUTANT = ""  # "" nominal ; drop_extra_ball | wrong_arity | ref_open_as_closed


class Refusal(Exception):
    def __init__(self, code: int, reason: str) -> None:
        super().__init__(reason)
        self.code = code
        self.reason = reason


def need(ok: bool, reason: str, code: int = 3) -> None:
    if not ok:
        raise Refusal(code, reason)


# ----------------------------------------------------------------------------
# Geometrie rationnelle exacte (logique de l'auditeur, copiee en lecture).
# ----------------------------------------------------------------------------

def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def circumball(points: list[Point]):
    """Gram t=|d|^2/2 par elimination rationnelle ; None si affinement dependant.

    Retourne centre, rayon carre, cle primitive [A=1..]*lcm/gcd, poids
    barycentriques des points 1.. (le poids du point 0 vaut 1 - somme)."""
    base = points[0]
    deltas = [subtract(p, base) for p in points[1:]]
    rank = len(deltas)
    matrix = [[Q(dot(a, b)) for b in deltas] + [Q(dot(a, a), 2)] for a in deltas]
    for column in range(rank):
        pivot = next((r for r in range(column, rank) if matrix[r][column]), None)
        if pivot is None:
            return None
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        divisor = matrix[column][column]
        matrix[column] = [value / divisor for value in matrix[column]]
        for row in range(rank):
            if row != column:
                scale = matrix[row][column]
                matrix[row] = [v - scale * w for v, w in zip(matrix[row], matrix[column])]
    weights = [row[-1] for row in matrix]
    center = tuple(Q(base[i]) + sum(weights[j] * deltas[j][i] for j in range(rank)) for i in range(3))
    radius = dot(subtract(center, base), subtract(center, base))
    coefficients = [Q(1)] + [-2 * c for c in center] + [dot(center, center) - radius]
    common = math.lcm(*(value.denominator for value in coefficients))
    key = [int(value * common) for value in coefficients]
    divisor = math.gcd(*key)
    key = [value // divisor for value in key]
    return {"center": center, "radius": radius, "key": tuple(key), "weights": weights}


def power(ball, point: Point) -> Q:
    delta = subtract(point, ball["center"])
    return dot(delta, delta) - ball["radius"]


def popcount(mask: int) -> int:
    return bin(mask).count("1")


# ----------------------------------------------------------------------------
# Modele exact d'un nuage : toutes les boules candidates, MEB de tout sous-ensemble.
# ----------------------------------------------------------------------------

def build_model(points: list[Point]) -> dict:
    n = len(points)
    need(2 <= n <= 20 and len(set(points)) == n, "model.distinct_bounded_input")
    for p in points:
        need(all(0 <= c <= 65535 for c in p), "model.u16_profile")
    full = (1 << n) - 1
    balls: dict[tuple, dict] = {}
    for size in range(2, min(4, n) + 1):
        for combo in __import__("itertools").combinations(range(n), size):
            ball = circumball([points[i] for i in combo])
            if ball is None:
                continue
            weights = ball["weights"]
            first = 1 - sum(weights)
            closed_hull = all(w >= 0 for w in weights) and first >= 0
            positive = all(w > 0 for w in weights) and first > 0
            if not closed_hull:
                continue
            mask = sum(1 << i for i in combo)
            key = ball["key"]
            if key not in balls:
                powers = [power(ball, p) for p in points]
                need(all(powers[i] == 0 for i in combo), "model.support_on_sphere")
                interior = sum(1 << i for i, v in enumerate(powers) if v < 0)
                shell = sum(1 << i for i, v in enumerate(powers) if v == 0)
                need(ball["radius"] > 0 and key[0] > 0, "model.positive_radius_primitive_key")
                balls[key] = dict(center=ball["center"], radius=ball["radius"], interior=interior,
                                  shell=shell, closed=interior | shell, supports=[], closed_supports=[])
            balls[key]["closed_supports"].append(mask)
            if positive:
                balls[key]["supports"].append(mask)
    for key, b in balls.items():
        need(bool(b["supports"]), "model.ball_without_positive_support")
        need(min(map(popcount, b["supports"])) == min(map(popcount, b["closed_supports"])),
             "model.qmin_closed_equals_positive")
        need(all(T & ~b["shell"] == 0 for T in b["closed_supports"]), "model.supports_inside_shell")
    level: list = [None] * (1 << n)
    owner: list = [None] * (1 << n)
    tie: list = [False] * (1 << n)
    for i in range(n):
        level[1 << i] = Q(0)
    for key, b in balls.items():
        closed = b["closed"]
        r = b["radius"]
        sub = closed
        while sub:
            if sub & (sub - 1):  # au moins deux points
                if level[sub] is None or r < level[sub]:
                    level[sub] = r
                    owner[sub] = key
                    tie[sub] = False
                elif r == level[sub] and owner[sub] != key:
                    tie[sub] = True  # juge seulement apres la passe complete
            sub = (sub - 1) & closed
    for mask in range(1, full + 1):
        need(level[mask] is not None, "model.subset_without_meb")
        need(not tie[mask], "model.meb_not_unique")
        if mask & (mask - 1):
            supports = balls[owner[mask]]["supports"]
            need(any(T & ~mask == 0 for T in supports), "model.owner_support_not_inside_subset")
        for bit in range(n):
            if mask & (1 << bit) and mask != (1 << bit):
                need(level[mask ^ (1 << bit)] <= level[mask], "model.monotonicity")
    return dict(n=n, points=points, balls=balls, level=level, owner=owner, full=full)


def catalogue(model: dict, kmax: int, ids: list[int]) -> tuple[list[dict], dict]:
    n = model["n"]
    smax = min(kmax + 1, n)
    rows = []
    excluded = 0
    for key in sorted(model["balls"]):
        b = model["balls"][key]
        p = popcount(b["interior"])
        u = popcount(b["shell"])
        qmin = min(map(popcount, b["supports"]))
        if p + qmin > smax:
            excluded += 1
            continue
        rows.append(dict(key=key, radius=b["radius"], arity=qmin, p=p, u=u,
                         interior=[ids[i] for i in range(n) if b["interior"] & (1 << i)],
                         shell=[ids[i] for i in range(n) if b["shell"] & (1 << i)]))
    extra_shell = sum(1 for r in rows if r["u"] > r["arity"])
    equal_lots = 0
    equal_lot_balls = 0
    for k in range(1, kmax + 1):
        groups: dict = {}
        for r in rows:
            lo = r["p"] + r["arity"] - 1
            hi = min(kmax, r["p"] + r["u"])
            if lo <= k <= hi:
                groups.setdefault(r["radius"], 0)
                groups[r["radius"]] += 1
        for count in groups.values():
            if count >= 2:
                equal_lots += 1
                equal_lot_balls += count
    info = dict(balls=len(rows), excluded_outside_window=excluded, extra_shell_balls=extra_shell,
                equal_level_lots=equal_lots, equal_level_lot_balls=equal_lot_balls)
    return rows, info


# ----------------------------------------------------------------------------
# Reference Gamma : balayage incremental par ordre, images verticales certifiees.
# ----------------------------------------------------------------------------

class Order:
    def __init__(self, k: int) -> None:
        self.k = k
        self.parent: dict[int, int] = {}
        self.cover: dict[int, int] = {}
        self.rep_sub: dict[int, int] = {}
        self.roots: set[int] = set()

    def find(self, mask: int) -> int:
        need(mask in self.parent, "reference.face_not_active_monotonicity")
        root = mask
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[mask] != root:
            self.parent[mask], mask = root, self.parent[mask]
        return root


def reference(model: dict, kmax: int, cuts: list[Q]) -> dict:
    """snapshots[(k, cut_index, closed)] = liste triee de (cover, image_cover|None)."""
    n = model["n"]
    level = model["level"]
    by_level: dict = {}
    for mask in range(1, model["full"] + 1):
        size = popcount(mask)
        if size > kmax + 1:
            continue
        by_level.setdefault(level[mask], {}).setdefault(size, []).append(mask)
    need(set(by_level) <= set(cuts), "reference.event_level_outside_cuts")
    orders = {k: Order(k) for k in range(1, kmax + 1)}
    snapshots: dict = {}
    stats = dict(activations=0, unions=0, merges=0, vertical_checks=0)

    def snapshot(index: int, closed: int) -> None:
        for k, order in orders.items():
            rows = []
            for root in order.roots:
                image = None
                if k >= 2:
                    lower = orders[k - 1]
                    image = lower.cover[lower.find(order.rep_sub[root])]
                rows.append((order.cover[root], image))
            rows.sort(key=lambda r: (r[0], -1 if r[1] is None else r[1]))
            snapshots[(k, index, closed)] = rows

    for index, cut in enumerate(cuts):
        if MUTANT != "ref_open_as_closed":
            snapshot(index, 0)
        events = by_level.get(cut, {})
        for k in range(1, kmax + 1):
            order = orders[k]
            for mask in events.get(k, []):
                order.parent[mask] = mask
                order.cover[mask] = mask
                order.roots.add(mask)
                stats["activations"] += 1
                if k >= 2:
                    lower = orders[k - 1]
                    subs = [mask ^ (1 << bit) for bit in range(n) if mask & (1 << bit)]
                    image = lower.find(subs[0])
                    for sub in subs[1:]:
                        need(lower.find(sub) == image, "reference.vertical_image_not_unique_at_activation")
                        stats["vertical_checks"] += 1
                    order.rep_sub[mask] = subs[0]
            for coface in events.get(k + 1, []):
                faces = [coface ^ (1 << bit) for bit in range(n) if coface & (1 << bit)]
                stats["unions"] += 1
                ra = order.find(faces[0])
                for face in faces[1:]:
                    rb = order.find(face)
                    if ra == rb:
                        continue
                    if k >= 2:
                        lower = orders[k - 1]
                        need(lower.find(order.rep_sub[ra]) == lower.find(order.rep_sub[rb]),
                             "reference.vertical_image_not_unique_at_merge")
                        stats["vertical_checks"] += 1
                    order.parent[rb] = ra
                    order.cover[ra] |= order.cover[rb]
                    del order.cover[rb]
                    order.roots.discard(rb)
                    stats["merges"] += 1
        if MUTANT == "ref_open_as_closed":
            snapshot(index, 0)
        snapshot(index, 1)
    for k, order in orders.items():
        need(len(order.roots) == 1 and order.cover[next(iter(order.roots))] == model["full"],
             "reference.final_single_component_covering_cloud")
    return dict(snapshots=snapshots, stats=stats)


# ----------------------------------------------------------------------------
# Pont produit.
# ----------------------------------------------------------------------------

def bridge_input(points: list[Point], ids: list[int], rows: list[dict], cuts: list[Q], kmax: int) -> str:
    lines = [f"point {ids[i]} {p[0]} {p[1]} {p[2]}" for i, p in enumerate(points)]
    for r in rows:
        a, b0, b1, b2, c = r["key"]
        lines.append(f"ball {a} {b0} {b1} {b2} {c} {r['radius'].numerator} {r['radius'].denominator} "
                     f"{r['arity']} | {' '.join(map(str, r['interior']))} | {' '.join(map(str, r['shell']))}")
    for t in cuts:
        lines.append(f"cut {t.numerator} {t.denominator}")
    lines.append(f"kmax {kmax}")
    lines.append("run")
    return "\n".join(lines) + "\n"


def run_bridge(binary: str, text: str) -> tuple[dict, list[dict]]:
    run = subprocess.run([binary], input=text, capture_output=True, text=True, check=False)
    lines = [json.loads(line) for line in run.stdout.splitlines() if line.strip()]
    need(bool(lines), f"bridge.no_output rc={run.returncode} stderr={run.stderr[:200]}")
    head = lines[0]
    if head.get("type") == "bridge_failure":
        raise Refusal(3, f"bridge.failure code={head.get('code')} reason={head.get('reason')}")
    need(run.returncode == 0 and head.get("type") == "tower", f"bridge.protocol rc={run.returncode}")
    need(not run.stderr, "bridge.stderr_not_empty")
    return head, lines[1:]


# ----------------------------------------------------------------------------
# Comparaison.
# ----------------------------------------------------------------------------

def compare(model: dict, ids: list[int], kmax: int, cuts: list[Q], ref: dict,
            product_lines: list[dict], cloud_desc: dict) -> dict:
    bit_of = {pid: i for i, pid in enumerate(ids)}
    cut_index = {(str(t.numerator), str(t.denominator)): i for i, t in enumerate(cuts)}
    seen: set = set()
    divergences = []
    counts = dict(cuts_compared=0, roots_compared=0, images_compared=0, ambiguous_cuts=0,
                  ambiguous_roots=0, k1_images_null=0)

    def to_mask(id_list) -> int:
        mask = 0
        for pid in id_list:
            need(pid in bit_of, "compare.unknown_point_id_in_product_cover")
            mask |= 1 << bit_of[pid]
        return mask

    for line in product_lines:
        need(line.get("type") == "cut", "compare.unexpected_line_type")
        k = line["k"]
        index = cut_index.get((line["num"], line["den"]))
        need(index is not None, "compare.unknown_cut_echo")
        closed = line["closed"]
        tag = (k, index, closed)
        need(tag not in seen, "compare.duplicate_cut_line")
        seen.add(tag)
        expected = ref["snapshots"][tag]
        actual = []
        for root in line["roots"]:
            cover = to_mask(root["cover"])
            image = root["image"]
            if isinstance(image, list):
                image = to_mask(image)
            actual.append((cover, image))
        exp_covers = sorted(c for c, _ in expected)
        act_covers = sorted(c for c, _ in actual)
        counts["cuts_compared"] += 1
        counts["roots_compared"] += len(expected)
        record = dict(cloud=cloud_desc, k=k, cut=f"{cuts[index].numerator}/{cuts[index].denominator}",
                      closed=closed)
        if exp_covers != act_covers:
            divergences.append(dict(record, kind="coverage_multiset",
                                    expected=[sorted(ids[i] for i in range(len(ids)) if c >> i & 1) for c in exp_covers],
                                    actual=[sorted(ids[i] for i in range(len(ids)) if c >> i & 1) for c in act_covers]))
            continue
        multiplicity: dict = {}
        for c in exp_covers:
            multiplicity[c] = multiplicity.get(c, 0) + 1
        ambiguous = [c for c, m in multiplicity.items() if m >= 2]
        if ambiguous:
            counts["ambiguous_cuts"] += 1
            counts["ambiguous_roots"] += sum(multiplicity[c] for c in ambiguous)
        exp_image = {c: img for c, img in expected if multiplicity[c] == 1}
        for cover, image in actual:
            if multiplicity[cover] != 1:
                continue
            if k == 1:
                if image is None:
                    counts["k1_images_null"] += 1
                else:
                    divergences.append(dict(record, kind="k1_image_not_absent", cover=sorted(
                        ids[i] for i in range(len(ids)) if cover >> i & 1), actual_image=image))
                continue
            counts["images_compared"] += 1
            want = exp_image[cover]
            if not isinstance(image, int) or image != want:
                divergences.append(dict(record, kind="vertical_image",
                                        cover=sorted(ids[i] for i in range(len(ids)) if cover >> i & 1),
                                        expected_image=sorted(ids[i] for i in range(len(ids)) if want >> i & 1),
                                        actual_image=image if not isinstance(image, int) else sorted(
                                            ids[i] for i in range(len(ids)) if image >> i & 1)))
    missing = set(ref["snapshots"]) - seen
    need(not missing, f"compare.product_missing_cut_lines count={len(missing)}")
    return dict(counts=counts, divergences=divergences)


# ----------------------------------------------------------------------------
# Generation des nuages.
# ----------------------------------------------------------------------------

def make_cloud(rng: random.Random) -> dict:
    n = rng.randint(5, 12)
    cube = rng.choice([3, 4, 5, 5, 7])
    mode = rng.choice(["planar", "planar", "flat", "full"])
    positions: set = set()
    while len(positions) < n:
        x, y = rng.randint(0, cube), rng.randint(0, cube)
        if mode == "planar":
            z = 0
        elif mode == "flat":
            z = rng.choice([0, 0, 1, cube])
        else:
            z = rng.randint(0, cube)
        positions.add((x, y, z))
    points = list(positions)
    rng.shuffle(points)
    ids = rng.sample(range(1, 2 ** 32 - 1), n)
    if rng.random() < 0.3:
        ids[0] = 2 ** 32 - 1
        ids[1] = 0
    kmax = rng.randint(2, min(n, 6))
    return dict(n=n, cube=cube, mode=mode, points=points, ids=ids, kmax=kmax)


CIRCLE25 = [(5, 0), (0, 5), (-5, 0), (0, -5), (3, 4), (4, 3), (-3, 4), (-4, 3),
            (3, -4), (4, -3), (-3, -4), (-4, -3)]  # les douze points entiers de x^2+y^2=25


def make_cocircular(rng: random.Random) -> dict:
    """Famille ciblee : coquilles larges (u jusqu'a 12), lots egaux, descentes a rayon egal."""
    n = rng.randint(5, 12)
    on_circle = rng.randint(max(3, n - 4), min(12, n))
    positions = {(x + 5, y + 5, 0) for x, y in rng.sample(CIRCLE25, on_circle)}
    outside = rng.random() < 0.5
    while len(positions) < n:
        x, y = rng.randint(0, 10), rng.randint(0, 10)
        z = rng.choice([0, 0, 0, 1, 2])
        d = (x - 5) ** 2 + (y - 5) ** 2 + z * z
        if d < 25 or (outside and d > 25 and len(positions) == n - 1):
            positions.add((x, y, z))
    points = list(positions)
    rng.shuffle(points)
    ids = rng.sample(range(1, 2 ** 32 - 1), n)
    kmax = rng.randint(2, min(n, 6))
    return dict(n=n, cube=10, mode="cocircular", points=points, ids=ids, kmax=kmax)


FIXTURES = [
    ("square", [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)], 4),
    ("growth_ABCZ", [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)], 4),
    ("inert_ball", [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2), (0, 0, 0)], 5),
    ("shell7_window", [(10, 5, 0), (0, 5, 0), (5, 10, 0), (5, 0, 0), (8, 9, 0), (2, 1, 0), (9, 8, 0)], 5),
    ("portal_equal", [(0, 2, 0), (2, 4, 0), (4, 2, 0), (2, 0, 0), (2, 2, 0), (2, 1, 0)], 6),
    ("E5", [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], 5),
    ("actual_equal_radius_descent", [(35, 100, 0), (139, 48, 0), (152, 139, 0), (48, 139, 0),
                                     (100, 36, 0), (101, 37, 0), (99, 37, 0), (100, 200, 0)], 4),
]


def process_cloud(binary: str, cloud: dict, rng: random.Random) -> dict:
    points, ids, kmax = cloud["points"], cloud["ids"], cloud["kmax"]
    model = build_model(points)
    rows, info = catalogue(model, kmax, ids)
    info["mutated"] = False
    if MUTANT in ("drop_extra_ball", "wrong_arity"):
        extra = [r for r in rows if r["u"] > r["arity"]]
        if extra:
            info["mutated"] = True
            if MUTANT == "drop_extra_ball":
                rows = [r for r in rows if r is not extra[0]]
            else:
                extra[0]["arity"] = extra[0]["u"]
    rng.shuffle(rows)
    radii = sorted({b["radius"] for b in model["balls"].values()})
    cuts = sorted({Q(0)} | set(radii) | {radii[-1] + 1})
    ref = reference(model, kmax, cuts)
    head, lines = run_bridge(binary, bridge_input(points, ids, rows, cuts, kmax))
    desc = dict(name=cloud.get("name", ""), n=cloud["n"], points=points, ids=ids, kmax=kmax,
                cube=cloud.get("cube", 0), mode=cloud.get("mode", ""))
    summary = dict(name=cloud.get("name", ""), n=cloud["n"], kmax=kmax, cube=cloud.get("cube", 0),
                   mode=cloud.get("mode", ""), cuts=len(cuts), candidate_balls=len(model["balls"]),
                   catalogue=info, reference=ref["stats"], product_status=head["status"],
                   product_reason=head["reason"], product_stats=head["stats"])
    if head["status"] != "complete_relative":
        summary["refusal"] = dict(desc, status=head["status"], reason=head["reason"])
        return summary
    need(head["orders"] == kmax, "product.orders_count")
    summary["comparison"] = compare(model, ids, kmax, cuts, ref, lines, desc)
    return summary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge", required=True)
    parser.add_argument("--seed", type=int, default=20260910)
    parser.add_argument("--clouds", type=int, default=200)
    parser.add_argument("--no-fixtures", action="store_true")
    parser.add_argument("--family", default="random", choices=["random", "cocircular"])
    parser.add_argument("--mutant", default="",
                        choices=["", "drop_extra_ball", "wrong_arity", "ref_open_as_closed"])
    args = parser.parse_args()
    global MUTANT
    MUTANT = args.mutant
    rng = random.Random(args.seed)
    summaries = []
    clouds = []
    if not args.no_fixtures:
        for name, points, kmax in FIXTURES:
            n = len(points)
            ids = [4294967295, 17, 0, 902, 2147483648, 3, 65536, 42][:n]
            clouds.append(dict(name=name, n=n, points=points, ids=ids, kmax=kmax, cube=0, mode="fixture"))
    for _ in range(args.clouds):
        clouds.append(make_cloud(rng) if args.family == "random" else make_cocircular(rng))
    totals = dict(clouds=0, orders=0, cuts_compared=0, roots_compared=0, images_compared=0,
                  ambiguous_cuts=0, ambiguous_roots=0, k1_images_null=0, candidate_balls=0, mutated_clouds=0,
                  catalogue_balls=0, excluded_outside_window=0, extra_shell_balls=0,
                  equal_level_lots=0, equal_level_lot_balls=0, facets_activated=0, unions=0,
                  merges=0, vertical_checks=0, product_refusals=0, divergences=0)
    product_totals: dict = {}
    refusals = []
    divergences = []
    refusal_reasons: dict = {}
    sizes: dict = {}
    for index, cloud in enumerate(clouds):
        print(f"cloud {index + 1}/{len(clouds)} n={cloud['n']} kmax={cloud['kmax']} mode={cloud['mode']}",
              file=sys.stderr, flush=True)
        summary = process_cloud(args.bridge, cloud, rng)
        summaries.append(summary)
        totals["clouds"] += 1
        totals["orders"] += cloud["kmax"]
        totals["candidate_balls"] += summary["candidate_balls"]
        for key in ("balls", "excluded_outside_window", "extra_shell_balls", "equal_level_lots",
                    "equal_level_lot_balls"):
            totals["catalogue_balls" if key == "balls" else key] += summary["catalogue"][key]
        totals["mutated_clouds"] += 1 if summary["catalogue"]["mutated"] else 0
        totals["facets_activated"] += summary["reference"]["activations"]
        for key in ("unions", "merges", "vertical_checks"):
            totals[key] += summary["reference"][key]
        for key, value in summary["product_stats"].items():
            product_totals[key] = product_totals.get(key, 0) + value
        sizes[str(cloud["n"])] = sizes.get(str(cloud["n"]), 0) + 1
        if "refusal" in summary:
            totals["product_refusals"] += 1
            refusals.append(summary["refusal"])
            refusal_reasons[summary["product_reason"]] = refusal_reasons.get(summary["product_reason"], 0) + 1
            continue
        for key, value in summary["comparison"]["counts"].items():
            totals[key] += value
        totals["divergences"] += len(summary["comparison"]["divergences"])
        divergences.extend(summary["comparison"]["divergences"])
    status = "passed" if not refusals and not divergences else "divergent"
    output = dict(status=status, public_status="not_claimed",
                  scope="random_integer_clouds_rational_judge_vs_overlay_wip_130402_not_wspd_completeness",
                  seed=args.seed, family=args.family, fixtures=not args.no_fixtures, mutant=MUTANT,
                  cloud_sizes=sizes, totals=totals,
                  product_stats_totals=product_totals, refusal_reasons=refusal_reasons,
                  refusals=refusals, divergences=divergences[:50], divergence_count=len(divergences),
                  clouds=summaries, engine_executed=True, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(",", ":")))
    return 0 if status == "passed" else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Refusal as refusal:
        print(f"REFUSAL code={refusal.code} reason={refusal.reason}", file=sys.stderr)
        sys.exit(refusal.code)
