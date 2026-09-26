#!/usr/bin/env python3
"""Bounded geometric falsifier for the proposed K1 guidance objective.

Uses exhaustive pair distances and graph traversal on tiny rational clouds.
The graph of intersecting closed radius-r balls has an edge when d^2 <= 4r^2;
its components give the K1 connectivity queried here. No native HGP output,
training data, learned model, or higher-order weighted relation is exercised.
All checks stay active under python -O.
"""

from fractions import Fraction
from itertools import combinations
import hashlib
import json
from pathlib import Path
import sys


F = Fraction
Point = tuple[Fraction, Fraction, Fraction]
Cloud = dict[str, Point]


class CheckFailure(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise CheckFailure(message)


def cloud(values: dict[str, int]) -> Cloud:
    """Embed a one-dimensional fixture into physical three-dimensional space."""
    result = {key: (F(value), F(0), F(0)) for key, value in values.items()}
    require(len(set(result.values())) == len(result), "fixture sites must differ")
    return result


def squared_distance(a: Point, b: Point) -> Fraction:
    return sum(((x - y) ** 2 for x, y in zip(a, b)), F(0))


def graph(points: Cloud, radius_squared: Fraction,
          closed: bool = True) -> dict[str, set[str]]:
    require(radius_squared >= 0, "negative radius squared")
    neighbours = {key: set() for key in points}
    for a, b in combinations(sorted(points), 2):
        distance = squared_distance(points[a], points[b])
        edge = distance <= 4 * radius_squared if closed else distance < 4 * radius_squared
        if edge:
            neighbours[a].add(b)
            neighbours[b].add(a)
    return neighbours


def connected(points: Cloud, a: str, b: str,
              radius_squared: Fraction, closed: bool = True) -> bool:
    require(a in points and b in points, "query site outside the declared view")
    neighbours = graph(points, radius_squared, closed)
    visited = {a}
    pending = [a]
    while pending:
        current = pending.pop()
        for other in sorted(neighbours[current]):
            if other not in visited:
                visited.add(other)
                pending.append(other)
    return b in visited


def merge_radius_squared(points: Cloud, a: str, b: str) -> Fraction:
    candidates = {F(0)}
    candidates.update(squared_distance(points[u], points[v]) / 4
                      for u, v in combinations(sorted(points), 2))
    for level in sorted(candidates):
        if connected(points, a, b, level):
            return level
    raise CheckFailure("finite Euclidean cloud failed to connect")


def observed_merge(points: Cloud, a: str, b: str,
                   horizon_squared: Fraction) -> dict:
    require(horizon_squared >= 0, "negative horizon")
    if not connected(points, a, b, horizon_squared):
        return {"status": "right_censored", "horizon_squared": horizon_squared,
                "merge_radius_squared": None}
    return {"status": "observed", "horizon_squared": horizon_squared,
            "merge_radius_squared": merge_radius_squared(points, a, b)}


def student_signature(points: Cloud, radius_squared: Fraction) -> dict:
    """Deterministic toy compiler: no teacher argument or teacher ancestry."""
    neighbours = graph(points, radius_squared)
    return {"sites": [(key, points[key]) for key in sorted(points)],
            "edges": [(a, b) for a in sorted(neighbours)
                      for b in sorted(neighbours[a]) if a < b],
            "radius_squared": radius_squared}


def record(name: str, values: dict, incorrect_variants: dict[str, bool]) -> dict:
    require(bool(incorrect_variants), name + ": no incorrect variant exercised")
    require(all(incorrect_variants.values()), name + ": variant was not refuted")
    return {"name": name, "status": "PASS", "values": values,
            "incorrect_variants": {key: "REJECTED"
                                   for key in sorted(incorrect_variants)}}


def missing_bridge() -> dict:
    teacher = cloud({"a": 0, "bridge": 1, "b": 2})
    visible = {key: teacher[key] for key in ("a", "b")}
    teacher_merge = merge_radius_squared(teacher, "a", "b")
    visible_merge = merge_radius_squared(visible, "a", "b")
    require(teacher_merge == F(1, 4) and visible_merge == F(1),
            "incorrect bridge merge radii")
    levels = [F(0), F(1, 16), F(1, 4), F(9, 16), F(1)]
    checks = 0
    for size in (2, 3):
        for subset in combinations(sorted(teacher), size):
            view = {key: teacher[key] for key in subset}
            for a, b in combinations(subset, 2):
                for level in levels:
                    require(connected(view, a, b, level)
                            <= connected(teacher, a, b, level),
                            "inclusion must preserve K1 connectivity")
                    checks += 1
    require(checks == 30, "inclusion fixture accidentally lost query coverage")
    teacher_at_event = connected(teacher, "a", "b", F(1, 4))
    require(teacher_at_event, "closed balls must connect at their contact")
    return record("missing_bridge_changes_merge_but_preserves_inclusion", {
        "teacher_merge_radius_squared": teacher_merge,
        "visible_merge_radius_squared": visible_merge,
        "query_radius_squared": F(9, 16),
        "teacher_label": connected(teacher, "a", "b", F(9, 16)),
        "visible_label": connected(visible, "a", "b", F(9, 16)),
        "inclusion_query_count": checks,
    }, {
        "require_identical_connectivity_after_decimation": teacher_merge != visible_merge,
        "reverse_the_subset_connectivity_inequality":
            teacher_at_event and not connected(visible, "a", "b", F(1, 4)),
        "use_strict_contact_for_a_closed_cut":
            teacher_at_event and not connected(teacher, "a", "b", F(1, 4), closed=False),
    })


def indistinguishable_completions() -> dict:
    visible = cloud({"a": 0, "b": 2})
    with_bridge = cloud({"a": 0, "bridge": 1, "b": 2})
    without_bridge = cloud({"a": 0, "other": 4, "b": 2})
    radius_squared = F(9, 16)
    restricted = [{key: completion[key] for key in visible}
                  for completion in (with_bridge, without_bridge)]
    require(restricted[0] == restricted[1] == visible, "visible clouds differ")
    labels = [int(connected(completion, "a", "b", radius_squared))
              for completion in (with_bridge, without_bridge)]
    require(labels == [1, 0], "completions must disagree on the target")
    mean_label = sum(labels, F(0)) / len(labels)
    best_brier = sum(((mean_label - label) ** 2 for label in labels), F(0)) / len(labels)
    hard_brier = [sum(((F(prediction) - label) ** 2 for label in labels), F(0)) / len(labels)
                  for prediction in (0, 1)]
    require(mean_label == F(1, 2) and best_brier == F(1, 4), "incorrect conditional target")
    require(hard_brier == [F(1, 2), F(1, 2)], "incorrect hard prediction errors")
    return record("same_visible_cloud_allows_opposite_teacher_targets", {
        "teacher_site_counts": [len(with_bridge), len(without_bridge)],
        "teacher_labels": labels, "conditional_target_for_equal_completion_probabilities": mean_label,
        "minimum_brier_for_equal_completion_probabilities": best_brier,
        "deterministic_binary_brier": hard_brier,
    }, {
        "claim_exact_recovery_from_the_visible_cloud_alone": len(set(labels)) == 2,
        "claim_a_richer_teacher_removes_conditional_ambiguity": best_brier > 0,
    })


def finite_horizon() -> dict:
    points = cloud({"a": 0, "b": 4})
    horizon_squared = F(1)
    observed = observed_merge(points, "a", "b", horizon_squared)
    true_merge = merge_radius_squared(points, "a", "b")
    labels = [connected(points, "a", "b", level)
              for level in (F(1, 4), F(1, 2), horizon_squared)]
    require(labels == [False, False, False], "finite-horizon negatives are valid")
    require(observed["status"] == "right_censored"
            and observed["merge_radius_squared"] is None, "missing censor status")
    require(true_merge == F(4), "fixture must merge strictly beyond the horizon")
    at_event = observed_merge(points, "a", "b", true_merge)
    require(at_event["status"] == "observed"
            and at_event["merge_radius_squared"] == true_merge, "horizon contact was censored")
    return record("finite_horizon_supports_queries_but_censors_merge_time", {
        "negative_query_labels": labels, "observation": observed,
        "exhaustive_fixture_merge_radius_squared": true_merge,
        "observation_at_actual_merge": at_event,
    }, {
        "replace_censoring_by_a_merge_at_the_horizon": true_merge != horizon_squared,
        "interpret_horizon_disconnection_as_never_connected":
            connected(points, "a", "b", true_merge),
        "censor_a_merge_exactly_on_the_closed_horizon": at_event["status"] == "observed",
    })


def teacher_topology_leakage() -> dict:
    completions = [cloud({"a": 0, "hidden": 1, "b": 2}),
                   cloud({"a": 0, "hidden": 4, "b": 2})]
    visible_views = [{key: completion[key] for key in ("a", "b")}
                     for completion in completions]
    level = F(9, 16)
    clean = [student_signature(view, level) for view in visible_views]
    require(clean[0] == clean[1] and clean[0]["edges"] == [],
            "student compiler must depend only on its visible cloud")
    clean_labels = [connected(view, "a", "b", level) for view in visible_views]
    leaked_labels = [connected(completion, "a", "b", level) for completion in completions]
    require(clean_labels == [False, False] and leaked_labels == [True, False],
            "fixture failed to distinguish retained teacher paths from recomputation")
    # Restricting original edges is insufficient to witness this leakage:
    # the hidden vertex / inherited ancestor carries the path a-hidden-b.
    restricted_teacher_edges = [[(a, b) for a, b in student_signature(completion, level)["edges"]
                                 if a in visible_views[0] and b in visible_views[0]]
                                for completion in completions]
    require(restricted_teacher_edges == [[], []], "unexpected direct visible edge")
    return record("masked_features_do_not_remove_teacher_topology_paths", {
        "clean_student_signatures_identical": clean[0] == clean[1],
        "recomputed_student_labels": clean_labels,
        "retained_teacher_connectivity": leaked_labels,
        "teacher_edges_restricted_to_visible_sites": restricted_teacher_edges,
    }, {
        "mask_hidden_coordinates_but_keep_teacher_graph_paths": leaked_labels != clean_labels,
        "restrict_teacher_components_instead_of_recomputing_student_components":
            len(set(leaked_labels)) > len(set(clean_labels)),
    })


def json_value(value):
    if isinstance(value, Fraction):
        return str(value.numerator) + "/" + str(value.denominator)
    if isinstance(value, dict):
        return {key: json_value(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_value(item) for item in value]
    return value


def main() -> int:
    results = []
    for check in (missing_bridge, indistinguishable_completions,
                  finite_horizon, teacher_topology_leakage):
        try:
            results.append(check())
        except Exception as error:
            results.append({"name": check.__name__, "status": "FAIL",
                            "error": type(error).__name__ + ": " + str(error)})
    passed = all(result["status"] == "PASS" for result in results)
    payload = {
        "schema": "zoltan.guidance_k1.v1",
        "scope": "tiny_rational_k1_closed_ball_connectivity_only",
        "geometry_scope": "one_dimensional_sites_embedded_in_three_dimensions",
        "engine_qualification": False,
        "learned_model_evaluated": False,
        "higher_order_weighted_relation_claimed": False,
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "status": "PASS" if passed else "FAIL",
        "fixture_count": len(results),
        "rejected_variant_count": sum(len(result.get("incorrect_variants", {})) for result in results),
        "fixtures": results,
    }
    print(json.dumps(json_value(payload), ensure_ascii=False, sort_keys=True, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
