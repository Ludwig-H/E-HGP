#!/usr/bin/env python3
"""Bounded counterexamples for tree selection and attention choices.

These synthetic trees and rational linear operators are not an HGP export,
a neural model, or a claim that every fixture is geometrically realizable.
No labels, external data, native engine, floating logarithms, or assertions.
"""

from fractions import Fraction
import hashlib
import itertools
import json
from pathlib import Path
import sys


F = Fraction


class CheckFailure(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise CheckFailure(message)


def record(name, values, incorrect_variants):
    require(bool(incorrect_variants), name + ": no counterexample")
    require(all(incorrect_variants.values()), name + ": unrejected variant")
    return {
        "name": name,
        "status": "PASS",
        "values": values,
        "incorrect_variants": {key: "REJECTED"
                               for key in sorted(incorrect_variants)},
    }


def frontiers(node, children):
    """Enumerate all covering antichains of this tiny rooted tree."""
    result = [(node,)]
    if node in children:
        for choices in itertools.product(
                *(frontiers(child, children) for child in children[node])):
            result.append(tuple(item for choice in choices for item in choice))
    return result


def select(node, children, scores, raw_child_bug=False):
    """Maximize the additive score; ties descend, as the documented rule did."""
    if node not in children:
        return scores[node], (node,)
    solutions = [select(child, children, scores, raw_child_bug)
                 for child in children[node]]
    split_score = sum((value for value, _ in solutions), F(0))
    comparison_score = (sum((scores[child] for child in children[node]), F(0))
                        if raw_child_bug else split_score)
    if scores[node] > comparison_score:
        return scores[node], (node,)
    return split_score, tuple(item for _, choice in solutions for item in choice)


def selection_score_and_recurrence():
    children = {"root": ("a", "b"), "a": ("a0", "a1"), "b": ("b0", "b1")}
    scores = {"root": F(9), "a": F(4), "b": F(4),
              "a0": F(3), "a1": F(3), "b0": F(3), "b1": F(3)}
    alternatives = frontiers("root", children)
    require(len(alternatives) == 5, "selection: independent frontier enumeration")
    exhaustive = max(sum((scores[node] for node in choice), F(0))
                     for choice in alternatives)
    correct, correct_frontier = select("root", children, scores)
    incorrect, incorrect_frontier = select("root", children, scores, True)
    require(exhaustive == correct == 12 and len(correct_frontier) == 4,
            "selection: optimum must be four leaves")
    require(incorrect == 9 and incorrect_frontier == ("root",),
            "selection: raw-child rule must exhibit wrong choice")

    # One synthetic true object consists of four disjoint atoms. Every tree
    # node is a subset of it, so its perfect best-instance IoU is size / 4.
    iou = {"root": F(1), "a": F(1, 2), "b": F(1, 2),
           "a0": F(1, 4), "a1": F(1, 4), "b0": F(1, 4), "b1": F(1, 4)}
    totals = [sum((iou[node] for node in choice), F(0)) for choice in alternatives]
    objective, fragmented = select("root", children, iou)
    require(totals == [F(1)] * 5 and objective == 1,
            "selection: every covering frontier must tie on perfect local IoU")
    require(len(fragmented) == 4, "selection: strict comparison descends to fragments")
    return record("selection_score_and_recurrence", {
        "covering_frontiers": len(alternatives),
        "exhaustive_optimum": exhaustive,
        "raw_child_choice_score": incorrect,
        "perfect_local_iou_all_frontier_scores": totals,
        "objects": 1,
        "selected_fragments": len(fragmented),
    }, {
        "raw_child_scores_equal_optimal_subtree_scores": incorrect != correct,
        "perfect_local_iou_prevents_fragmentation": len(fragmented) > 1,
    })


def local_relative_mass():
    masses = [F(1, 2**depth) for depth in range(4)]

    def kept_nodes(depth, threshold, root_relative=False):
        if depth == 3:
            return 1
        reference = F(1) if root_relative else masses[depth]
        if masses[depth + 1] < threshold * reference:
            return 1
        return 1 + 2 * kept_nodes(depth + 1, threshold, root_relative)

    raw = sum(2**depth for depth in range(4))
    local = kept_nodes(0, F(1, 4))
    half = kept_nodes(0, F(1, 2))
    above_half = kept_nodes(0, F(3, 5))
    root_relative = kept_nodes(0, F(1, 4), True)
    require(raw == local == half == 15, "mass: balanced tree remains intact")
    require(above_half == 1 and root_relative == 7, "mass: distinct threshold semantics")
    require(masses[-1] == F(1, 8), "mass: leaf relative mass")
    return record("local_relative_mass", {
        "raw_nodes": raw,
        "kept_local_alpha_quarter": local,
        "kept_local_alpha_half": half,
        "kept_local_alpha_three_fifths": above_half,
        "kept_root_relative_alpha_quarter": root_relative,
        "smallest_retained_root_mass": masses[-1],
    }, {
        "local_alpha_guarantees_compression": local == raw,
        "local_alpha_is_global_minimum_mass": masses[-1] < F(1, 4),
    })


def relative_bias_is_not_ultrametric():
    merge = [[F(0), F(2), F(4)], [F(2), F(0), F(4)], [F(4), F(4), F(0)]]
    birth = [F(1), F(3, 2), F(1)]
    cut = F(7, 4)
    require(all(value < cut for value in birth)
            and all(merge[i][j] > cut for i in range(3) for j in range(3) if i != j),
            "bias: all three states coexist before any pair merges")
    require(all(merge[i][k] <= max(merge[i][j], merge[j][k])
                for i in range(3) for j in range(3) for k in range(3)),
            "bias: raw merge levels must be ultrametric")
    ratio = [[merge[i][j] / birth[i] for j in range(3)] for i in range(3)]
    require(ratio[0][1] == 2 and ratio[1][0] == F(4, 3), "bias: asymmetric ratios")
    require(ratio[0][2] == 4 and ratio[1][2] == F(8, 3), "bias: expected ratios")
    require(ratio[0][2] > max(ratio[0][1], ratio[1][2]),
            "bias: normalized ratios violate strong triangle inequality")
    return record("relative_bias_is_not_ultrametric", {
        "birth_radii": birth,
        "coexisting_cut": cut,
        "raw_merge_radii": merge,
        "normalized_ratios": ratio,
        "logarithm_used": False,
    }, {
        "per_source_normalization_preserves_symmetry": ratio[0][1] != ratio[1][0],
        "per_source_normalization_preserves_ultrametric_inequality":
            ratio[0][2] > max(ratio[0][1], ratio[1][2]),
    })


def shared_event_is_not_clique_attention():
    features = [F(0), F(1), F(3)]
    clique = [[F(1, 2) if i == j else F(1, 4) for j in range(3)] for i in range(3)]
    outputs = [sum((weight * value for weight, value in zip(row, features)), F(0))
               for row in clique]
    hub = sum(features, F(0)) / len(features)
    broadcast = [hub] * len(features)
    uniform_clique = [sum(features, F(0)) / len(features)] * len(features)
    require(outputs == [F(1), F(5, 4), F(7, 4)], "event: distinct pairwise outputs")
    require(broadcast == [F(4, 3)] * 3 and uniform_clique == broadcast,
            "event: shared mean equals only this special uniform clique")
    arity = 12
    pair_edges = arity * (arity - 1)
    event_edges = 2 * arity
    require(pair_edges == 132 and event_edges == 24, "event: edge counts")
    return record("shared_event_is_not_clique_attention", {
        "linear_pairwise_outputs": outputs,
        "shared_mean_broadcast": broadcast,
        "multifusion_arity_for_count": arity,
        "directed_clique_edges_without_self": pair_edges,
        "directed_event_incidence_edges": event_edges,
        "scope": "one_scalar_shared_aggregation_then_broadcast",
    }, {
        "shared_aggregation_equals_general_pairwise_attention": outputs != broadcast,
        "sibling_clique_fits_two_edges_per_branch": pair_edges > event_edges,
    })


def json_value(value):
    if isinstance(value, F):
        return str(value)
    if isinstance(value, dict):
        return {key: json_value(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_value(item) for item in value]
    return value


def main():
    fixtures = [selection_score_and_recurrence, local_relative_mass,
                relative_bias_is_not_ultrametric, shared_event_is_not_clique_attention]
    results = []
    for fixture in fixtures:
        try:
            results.append(fixture())
        except Exception as error:
            results.append({"name": fixture.__name__, "status": "FAIL",
                            "error": type(error).__name__ + ": " + str(error)})
    passed = all(result["status"] == "PASS" for result in results)
    payload = {
        "schema": "zoltan.architecture_choices.v1",
        "scope": "synthetic_trees_and_rational_linear_operators_only",
        "engine_qualification": False,
        "geometric_realizability_claimed": False,
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
