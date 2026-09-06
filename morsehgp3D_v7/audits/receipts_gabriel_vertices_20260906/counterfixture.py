"""Check 15 fixed rational MEB certificates; no MEB search or product import."""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import hashlib
import json

Scalar = int | F
Vector = tuple[Scalar, ...]

POINTS = {"A": (0, 3, 0), "B": (4, 9, 0), "C": (8, 3, 0), "D": (4, 0, 1)}
LABELS = ["".join(q) for n in range(1, 5) for q in combinations(POINTS, n)]
# All supports are fixed analytically, rather than chosen by an algorithm.
SUPPORTS = {q: q for q in LABELS}
SUPPORTS.update(ABD="BD", ACD="AC", BCD="BD", ABCD="BD")
EXPECTED_LEVELS = {"A": 0, "B": 0, "C": 0, "D": 0,
                   "AB": 13, "AC": 16, "AD": F(13, 2), "BC": 13,
                   "BD": F(41, 2), "CD": F(13, 2), "ABC": F(169, 9),
                   "ABD": F(41, 2), "ACD": 16, "BCD": F(41, 2), "ABCD": F(41, 2)}


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def dot(a: Vector, b: Vector) -> Scalar:
    return sum(x * y for x, y in zip(a, b))


def sub(a: Vector, b: Vector) -> Vector:
    return tuple(x - y for x, y in zip(a, b))


def determinant(a: Vector, b: Vector, c: Vector) -> Scalar:
    return sum(a[i] * (b[(i + 1) % 3] * c[(i + 2) % 3] -
                       b[(i + 2) % 3] * c[(i + 1) % 3]) for i in range(3))


def union(a: str, b: str) -> str:
    return "".join(sorted(set(a) | set(b)))


def encode(value: object) -> object:
    if isinstance(value, F):
        return {"numerator": value.numerator, "denominator": value.denominator}
    if isinstance(value, dict):
        return {k: encode(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [encode(v) for v in value]
    return value


def components(vertices: list[str], edges: list[tuple[str, str]]) -> list[list[str]]:
    unseen, groups = set(vertices), []
    adjacency = {v: set() for v in vertices}
    for a, b in edges:
        adjacency[a].add(b)
        adjacency[b].add(a)
    while unseen:
        pending, group = [min(unseen)], set()
        while pending:
            v = pending.pop()
            if v in unseen:
                unseen.remove(v)
                group.add(v)
                pending.extend(adjacency[v])
        groups.append(sorted(group))
    return sorted(groups)


def main() -> None:
    require(len(LABELS) == 15 and set(EXPECTED_LEVELS) == set(LABELS), "inventory")
    require(all(0 <= x <= 65535 for p in POINTS.values() for x in p), "u16")
    det = determinant(*(sub(POINTS[p], POINTS["A"]) for p in "BCD"))
    require(det == -48, "affine_dimension_three")
    certificates, beta, gabriel = {}, {}, set()
    for label in LABELS:
        support = SUPPORTS[label]
        weights = ({"A": F(13, 36), "B": F(5, 18), "C": F(13, 36)}
                   if label == "ABC" else {p: F(1, len(support)) for p in support})
        require(set(weights) == set(support) and sum(weights.values()) == 1 and
                min(weights.values()) > 0, "positive_barycentrics:" + label)
        center = tuple(sum(weights[p] * POINTS[p][i] for p in support) for i in range(3))
        diff = sub(POINTS[support[0]], center)
        radius = dot(diff, diff)
        require(radius == EXPECTED_LEVELS[label], "fixed_level:" + label)
        powers = {p: dot(sub(x, center), sub(x, center)) - radius for p, x in POINTS.items()}
        require(all(powers[p] <= 0 for p in label), "containment:" + label)
        require({p for p in POINTS if powers[p] == 0} == set(support), "global_regular_shell:" + label)
        vectors = [sub(POINTS[p], POINTS[support[0]]) for p in support[1:]]
        if vectors:
            require(dot(vectors[0], vectors[0]) > 0, "support_distinct:" + label)
        if len(vectors) == 2:
            a, b = vectors
            require(dot(a, a) * dot(b, b) > dot(a, b) ** 2, "support_affine_rank:" + label)
        beta[label] = radius
        is_gabriel = all(powers[p] > 0 for p in POINTS if p not in label)
        if is_gabriel:
            gabriel.add(label)
        certificates[label] = {"support": support, "weights": weights, "center": center,
                               "radius_squared": radius, "all_point_powers": powers, "gabriel": is_gabriel}
    for label in LABELS:
        if len(label) > 1:
            require(all(beta[label.replace(p, "")] < beta[label] for p in SUPPORTS[label]),
                    "every_support_point_essential:" + label)
    minima = sorted(q for q in gabriel if len(q) == 2)
    require(minima == ["AB", "AD", "BC", "CD"], "four_Gabriel_minima")
    require(sorted(q for q in gabriel if len(q) == 3) == ["ABC", "ACD"], "two_Gabriel_cofaces")
    crossing = {a + "--" + b: beta[union(a, b)] for a in ("AD", "CD") for b in ("AB", "BC")}
    require(set(crossing.values()) == {F(41, 2)}, "all_crossings_delayed_even_full_unions")
    synthetic = [("AD", "CD", F(16)), ("AB", "BC", F(169, 9)), ("AD", "AB", F(169, 9))]
    critical = sorted(set(beta.values()))
    cuts = [(a, side) for a in critical for side in ("open", "closed")]
    cuts += [(a, "closed") for a in [critical[0] - 1] +
             [(a + b) / 2 for a, b in zip(critical, critical[1:])] + [critical[-1] + 1]]
    states = []
    for level, side in sorted(cuts):
        active = lambda x: x < level if side == "open" else x <= level
        vertices = [q for q in LABELS if len(q) == 2 and active(beta[q])]
        edges = [(a, b) for a, b in combinations(vertices, 2) if active(beta[union(a, b)])]
        gamma = components(vertices, edges)
        elementary = components(vertices, [(a, b) for a, b in edges if len(union(a, b)) == 3])
        require(gamma == elementary, "full_and_elementary_Gamma:" + str(level))
        kept = [q for q in minima if q in vertices]
        induced = components(kept, [(a, b) for a, b in edges if a in kept and b in kept])
        compressed = components(kept, [(a, b) for a, b, w in synthetic if a in kept and b in kept and active(w)])
        projected = sorted([q for q in group if q in minima] for group in gamma)
        require(all(projected) and projected == compressed, "compressed_H0:" + str(level))
        covers = lambda groups: sorted("".join(sorted(set("".join(g)))) for g in groups)
        require(covers(gamma) == covers(compressed), "point_covers:" + str(level))
        states.append({"squared_level": level, "side": side, "Gamma": gamma,
                       "Gabriel_induced_all_unions": induced, "compressed": compressed,
                       "Gamma_point_covers": covers(gamma), "induced_point_covers": covers(induced)})
    target = next(s for s in states if s["squared_level"] == F(169, 9) and s["side"] == "closed")
    require(len(target["Gamma"]) == 1 and target["Gabriel_induced_all_unions"] == [["AB", "BC"], ["AD", "CD"]],
            "target_one_vs_two")
    require(len(states) == 19, "all_critical_sides_and_intervals")
    transitions, canonical_paths = [], {}
    for source in ("AC", "BD"):
        state = next(s for s in states if s["squared_level"] == beta[source] and s["side"] == "closed")
        group = next(g for g in state["Gamma"] if source in g)
        for intruder in POINTS:
            if intruder in source or certificates[source]["all_point_powers"][intruder] >= 0:
                continue
            for removed in SUPPORTS[source]:
                descendant = union(source.replace(removed, ""), intruder)
                coface = union(source, intruder)
                require(beta[descendant] < beta[source] == beta[coface], "strict_facet_descent")
                require(descendant in group and descendant in minima, "descent_same_Gamma_and_terminal")
                transitions.append({"source": source, "intruder": intruder, "removed": removed,
                                    "descendant": descendant, "coface": coface,
                                    "source_squared_level": beta[source], "descendant_squared_level": beta[descendant]})
        first = next(t for t in transitions if t["source"] == source)
        canonical_paths[source] = [source, first["descendant"]]
    require(len(transitions) == 6 and canonical_paths == {"AC": ["AC", "CD"], "BD": ["BD", "AD"]},
            "all_six_transitions_and_canonical_termination")
    result = {"schema": "mhgp7-gabriel-vertices-counterfixture-v1", "status": "passed",
              "public_status": "not_claimed", "engine_calls": 0, "gcp": "not_used",
              "scope": "Verification of 15 fixed rational certificates and 19 finite graph cuts; no MEB search or product import.",
              "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "points_u16": POINTS, "affine_determinant": det, "certificates": certificates,
              "certificate_count": len(certificates), "cut_count": len(states), "minima": minima,
              "cross_union_levels": crossing, "first_induced_crossing_squared_level": F(41, 2),
              "synthetic_compression_edges": synthetic, "critical_squared_levels": critical,
              "strict_facet_descent_transitions": transitions, "canonical_descent_paths": canonical_paths,
              "counterexample": target, "states": states}
    print(json.dumps(encode(result), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
