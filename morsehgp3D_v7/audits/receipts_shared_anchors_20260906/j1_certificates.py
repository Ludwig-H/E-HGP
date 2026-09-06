"""Fixed rational certificates for the five-site J1 resolver counterexample."""
from fractions import Fraction as F
from pathlib import Path
import hashlib
import json

Scalar = int | F
Vector = tuple[Scalar, ...]
POINTS = {"A": (0, 3, 3), "B": (3, 2, 9), "C": (8, 6, 12),
          "D": (12, 9, 3), "E": (13, 6, 11)}
SOURCE_PATH = "morsehgp3D_v7/tests/full_gabriel_descent_comparison_gate.py"
SOURCE_SHA = "5e357ead0d626121cf66e15d17f0817475520663db7fe5237d9cbd7f25448a16"
# Center, squared radius, positive support weights and all five point powers.
BD = ((F(15, 2), F(11, 2), F(6)), F(83, 2), {"B": F(1, 2), "D": F(1, 2)}, (30, 0, -5, 0, 14))
CD = ((F(10), F(15, 2), F(15, 2)), F(53, 2), {"C": F(1, 2), "D": F(1, 2)}, (114, 55, 0, 0, -3))
SPECS = {
    "BD": BD, "BCD": BD, "CD": CD, "CDE": CD,
    "DE": ((F(25, 2), F(15, 2), F(7)), F(37, 2), {"D": F(1, 2), "E": F(1, 2)}, (174, 106, 29, 0, 0)),
    "BC": ((F(11, 2), F(4), F(21, 2)), F(25, 2), {"B": F(1, 2), "C": F(1, 2)}, (75, 0, 0, 111, 48)),
    "ABD": ((F(254, 41), F(230, 41), F(171, 41)), F(1909, 41),
            {"A": F(83, 246), "B": F(8, 41), "D": F(115, 246)}, (0, 0, F(744, 41), 0, F(1908, 41))),
}


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def sub(a: Vector, b: Vector) -> Vector:
    return tuple(x - y for x, y in zip(a, b))


def dot(a: Vector, b: Vector) -> Scalar:
    return sum(x * y for x, y in zip(a, b))


def encode(value: object) -> object:
    if isinstance(value, F):
        return {"numerator": value.numerator, "denominator": value.denominator}
    if isinstance(value, dict):
        return {k: encode(v) for k, v in value.items()}
    if isinstance(value, (tuple, list)):
        return [encode(v) for v in value]
    return value


def main() -> None:
    require(len(SPECS) == 7 and all(0 <= x <= 65535 for p in POINTS.values() for x in p), "fixed_u16_inventory")
    certificates, intruders, beta, gabriel = {}, {}, {}, set()
    for label, (center, radius, weights, expected_powers) in SPECS.items():
        require(sum(weights.values()) == 1 and min(weights.values()) > 0, "positive_weights:" + label)
        require(all(p in label for p in weights), "support_in_label:" + label)
        require(center == tuple(sum(w * POINTS[p][i] for p, w in weights.items()) for i in range(3)),
                "barycentric_center:" + label)
        powers = {p: dot(sub(x, center), sub(x, center)) - radius for p, x in POINTS.items()}
        require(tuple(powers.values()) == expected_powers, "fixed_powers:" + label)
        require(all(powers[p] <= 0 for p in label), "containing_ball:" + label)
        require({p for p in POINTS if powers[p] == 0} == set(weights), "regular_global_shell:" + label)
        support = list(weights)
        vectors = [sub(POINTS[p], POINTS[support[0]]) for p in support[1:]]
        gram = dot(vectors[0], vectors[0])
        if len(vectors) == 2:
            gram = gram * dot(vectors[1], vectors[1]) - dot(vectors[0], vectors[1]) ** 2
        require(gram > 0, "independent_essential_support:" + label)
        intruders[label] = [p for p in POINTS if p not in label and powers[p] < 0]
        if not intruders[label]:
            gabriel.add(label)
        beta[label] = radius
        certificates[label] = {"center": center, "radius_squared": radius, "support": support,
                               "weights": weights, "all_point_powers": powers,
                               "strict_intruders": intruders[label], "gabriel": label in gabriel}
    require(intruders["BD"] == ["C"] and intruders["CD"] == ["E"], "two_single_intruder_steps")
    require({"BCD", "CDE", "DE", "BC", "ABD"}.issubset(gabriel), "certified_directs_and_minima")
    require(beta["DE"] < beta["CD"] < beta["BD"] < beta["ABD"], "strict_levels")
    require(SPECS["BD"] == SPECS["BCD"] and SPECS["CD"] == SPECS["CDE"], "inherited_ball_no_second_MEB")
    require(beta["BC"] < beta["BD"], "alternative_minimum_strict")
    # Each transition has a certified containing coface before consumption.
    witnesses = [("BD", "CD", "BCD"), ("CD", "DE", "CDE"), ("BD", "BC", "BCD")]
    for source, target, coface in witnesses:
        require(set(source) | set(target) == set(coface) and beta[coface] == beta[source] and
                beta[target] < beta[source] < beta["ABD"], "same_preconsumption_Gamma_component")
    paths = {
        "old_J1": {"path": ["BD", "BCD"], "meb_labels": ["BD"], "census_labels": ["BD"],
                   "terminal_kind": "prior_direct_closed_anchor", "terminal": "BCD"},
        "pure_first_essential": {"path": ["BD", "CD", "DE"], "meb_labels": ["BD", "CD"],
                                 "census_labels": ["BD", "CD"], "terminal_kind": "minimum_lookup", "terminal": "DE"},
        "hybrid_retaining_J1": {"path": ["BD", "BCD"], "meb_labels": ["BD"], "census_labels": ["BD"],
                                "terminal_kind": "prior_direct_closed_anchor", "terminal": "BCD"},
        "probe_all_essential_minima": {"path": ["BD", "BC"], "meb_labels": ["BD"], "census_labels": ["BD"],
                                      "terminal_kind": "minimum_lookup", "terminal": "BC", "candidate_lookups": 2},
    }
    for row in paths.values():
        row["MEB_calls"] = len(row["meb_labels"])
        row["census_calls"] = len(row["census_labels"])
        require(all(len(label) == 2 for label in row["meb_labels"]), "pair_only_MEB_calls")
        row["P0_F_support_ordinal_sum"] = row["MEB_calls"]
    require([paths[p]["MEB_calls"] for p in paths] == [1, 2, 1, 1], "one_two_one_one")
    alternatives = []
    for removed, descendant in [("B", "CD"), ("D", "BC")]:
        require((set("BD") - {removed}) | {"C"} == set(descendant) and
                beta[descendant] < beta["BD"], "all_essential_minimum_probes")
        alternatives.append({"removed": removed, "descendant": descendant, "minimum": descendant in gabriel,
                             "squared_level": beta[descendant], "path_coface": "BCD"})
    require([row["minimum"] for row in alternatives] == [False, True], "first_miss_second_hit")
    result = {
        "schema": "mhgp7-independent-J1-fixed-certificates-v1", "status": "passed", "public_status": "not_claimed",
        "points_u16": POINTS, "certificate_count": len(certificates), "certificates": certificates,
        "consuming_coface": "ABD", "consumption_squared_level": beta["ABD"], "paths": paths,
        "Gamma_path_witnesses": witnesses,
        "all_BD_essential_replacement_candidates": alternatives,
        "alternative_facet_choice": {"replace": "D by C in BD", "path": ["BD", "BC"],
                                     "terminal": "certified_minimum_BC", "MEB_calls_if_lookup_before_MEB": 1},
        "assumptions": ["Initial BD request misses the optional cache; mandatory catalogue lookups remain available.",
                        "Complete exact catalogues supply minimum identities and the already processed closed BCD anchor.",
                        "Work counts are successful unbounded local calendars at P=0; no finite-budget refusal is replayed.",
                        "The pure rule removes the first essential PointId; the singleton intruder choices are unique."],
        "limits": ["Only these seven fixed labels have independent geometry certificates here, not all 31 subsets.",
                   "The two-call counterexample concerns this deterministic facet rule, not every choice of descent.",
                   "Retaining J1 keeps direct-anchor storage/normalization; their costs are not measured.",
                   "No query nodes, successor reads, elapsed time, memory, or product execution are qualified."],
        "constructor_comparison_observed": {"path": SOURCE_PATH, "sha256": SOURCE_SHA,
                                             "status": "worktree preparation observed, not executed or qualified",
                                             "is_execution_dependency": False},
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "engine_calls": 0, "product_imports": 0, "gcp": "not_used",
    }
    print(json.dumps(encode(result), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
