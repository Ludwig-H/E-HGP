#!/usr/bin/env python3
"""Exact, bounded counterexamples for proposed learning-target controls.

No HGP geometry, dataset, fitted model or learning qualification is produced.
The rational distributions and partitions below are explicit abstract inputs.
Checks use exceptions so that python -O exercises the same conditions.
"""

from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys


F = Fraction


def require(condition, message):
    if not condition:
        raise ValueError(message)


def record(name, values, rejected):
    require(bool(rejected), name + ": missing counterexample")
    require(all(rejected.values()), name + ": unrefuted incorrect variant")
    return {"name": name, "status": "PASS", "values": values,
            "incorrect_variants": {key: "REJECTED" for key in sorted(rejected)}}


def brier(predictions, labels, weights=None):
    require(len(predictions) == len(labels) and bool(labels), "Brier dimensions")
    if weights is None:
        weights = [F(1)] * len(labels)
    require(len(weights) == len(labels), "Brier weight dimensions")
    require(all(w >= 0 for w in weights) and sum(weights) > 0, "Brier weights")
    require(all(0 <= p <= 1 for p in predictions), "Brier probability domain")
    return sum((w * (p - y) ** 2 for p, y, w in
                zip(predictions, labels, weights)), F(0)) / sum(weights)


def radius_prior():
    labels = [0] * 9 + [1] + [0] + [1] * 9
    predicted = [F(1, 10)] * 10 + [F(9, 10)] * 10
    bs = brier(predicted, labels)
    flat = brier([F(1, 2)] * 20, labels)
    accuracy = F(sum((p >= F(1, 2)) == bool(y)
                     for p, y in zip(predicted, labels)), 20)
    require(sum(labels) == 10 and bs == F(9, 100), "radius fixture distribution")
    require(flat == F(1, 4) and accuracy == F(9, 10), "radius fixture scores")
    # A label-balanced evaluation *within* radius changes the sampling law.
    balanced_predictions = [F(1, 10)] * 2 + [F(9, 10)] * 2
    balanced_labels = [0, 1, 0, 1]
    balanced_bs = brier(balanced_predictions, balanced_labels)
    corrected_bs = brier(balanced_predictions, balanced_labels,
                         [F(9, 10), F(1, 10), F(1, 10), F(9, 10)])
    require(balanced_bs == F(41, 100) and corrected_bs == bs,
            "sampling correction failed")
    return record("radius_only_prior_and_evaluation_law", {
        "global_positive_rate": F(1, 2), "constant_point_features": True,
        "accuracy_without_point_features": accuracy, "radius_brier": bs,
        "flat_prior_brier": flat, "skill_against_flat_prior": 1 - bs / flat,
        "skill_against_radius_prior": F(0),
        "label_balanced_brier": balanced_bs, "corrected_brier": corrected_bs,
    }, {
        "global_label_balance_excludes_a_query_only_shortcut": accuracy > F(1, 2),
        "skill_against_flat_prior_demonstrates_context_use": 1 - bs / flat > 0,
        "label_rebalancing_preserves_calibration_distribution": balanced_bs != bs,
    })


def collision(a, b):
    require(len(a) == len(b), "collision dimensions")
    return sum((x * y for x, y in zip(a, b)), F(0))


def overlap(a, b):
    require(len(a) == len(b) and sum(a) == sum(b) == 1, "overlap distributions")
    return sum((min(x, y) for x, y in zip(a, b)), F(0))


def collision_and_similarity():
    one = [F(1), F(0)]
    uniform = [F(1, 2), F(1, 2)]
    partial = [F(1, 4), F(1, 4)]
    gamma_one = collision(one, one)
    gamma_uniform = collision(uniform, uniform)
    gamma_conditioned = collision(partial, partial) / sum(partial) ** 2
    require(gamma_one == 1 and gamma_uniform == gamma_conditioned == F(1, 2),
            "collision values")
    require(overlap(one, one) == overlap(uniform, uniform) == 1,
            "identical distributions must overlap fully")
    require(overlap(one, [F(0), F(1)]) == 0, "disjoint distribution overlap")
    return record("collision_is_not_distribution_similarity", {
        "identical_one_token_collision": gamma_one,
        "identical_two_token_collision": gamma_uniform,
        "coverage_conditioned_collision": gamma_conditioned,
        "identical_distribution_overlap": F(1),
    }, {
        "identical_distributions_have_unit_collision": gamma_uniform != 1,
        "conditioning_coverage_removes_incidence_concentration": gamma_conditioned != 1,
        "lower_collision_proves_distribution_disagreement": gamma_uniform < gamma_one,
    })


def probability_bounds_hold(p):
    # Pair order is (01, 12, 02). Each pair can play the consequent.
    return all(0 <= x <= 1 for x in p) and all(
        p[k] >= p[i] + p[j] - 1
        for i, j, k in ((0, 1, 2), (0, 2, 1), (1, 2, 0)))


def partition_marginals():
    partitions = [(0, 0, 1), (0, 1, 1)]
    pairs = [(0, 1), (1, 2), (0, 2)]
    p = [sum((F(partition[i] == partition[j], 2)
              for partition in partitions), F(0)) for i, j in pairs]
    require(p == [F(1, 2), F(1, 2), F(0)] and probability_bounds_hold(p),
            "valid mixture marginals")
    impossible = [F(9, 10), F(9, 10), F(1, 10)]
    require(not probability_bounds_hold(impossible), "impossible triple accepted")
    thresholded = [value >= F(1, 2) for value in p]
    require(thresholded == [True, True, False], "threshold example")
    return record("posterior_marginals_are_not_a_hard_hierarchy", {
        "valid_mixture_pair_probabilities": p,
        "impossible_pair_probabilities": impossible,
        "necessary_lower_bound_for_third_impossible_pair": F(4, 5),
        "thresholded_pairs": thresholded,
        "bounds_claimed_sufficient_for_arbitrary_size": False,
    }, {
        "posterior_connection_probabilities_obey_min_transitivity": p[2] < min(p[:2]),
        "arbitrary_pair_probabilities_define_a_partition_law": not probability_bounds_hold(impossible),
        "thresholding_marginals_preserves_transitivity": thresholded[0] and thresholded[1] and not thresholded[2],
    })


def censored_cdf():
    # Rational bin probabilities stand in for a future softmax; no network run.
    mass = [F(1, 4), F(1, 4), F(1, 8), F(3, 8)]
    require(sum(mass) == 1 and all(p >= 0 for p in mass), "CDF mass domain")
    cdf = [sum(mass[:i], F(0)) for i in range(1, 4)]
    require(cdf == [F(1, 4), F(1, 2), F(5, 8)], "CDF cumulative values")
    require(all(a <= b for a, b in zip(cdf, cdf[1:])), "CDF monotonicity")
    shorter_horizon_survival = sum(mass[2:], F(0))
    require(shorter_horizon_survival == F(1, 2), "short horizon survival")
    free_sigmoid_outputs = [F(3, 4), F(1, 4), F(7, 8)]
    return record("monotone_bins_preserve_censored_mass", {
        "bin_masses_with_tail": mass, "cdf_at_declared_thresholds": cdf,
        "survival_after_max_horizon": 1 - cdf[-1],
        "survival_after_middle_threshold": shorter_horizon_survival,
        "arbitrary_radius_interpolation_implemented": False,
    }, {
        "finite_horizon_forces_all_fusions_by_horizon": cdf[-1] != 1,
        "censoring_at_middle_threshold_is_a_middle_bin_event": shorter_horizon_survival != mass[1],
        "independent_sigmoids_are_monotone_by_construction": free_sigmoid_outputs[0] > free_sigmoid_outputs[1],
    })


def main():
    cases = [radius_prior(), collision_and_similarity(), partition_marginals(), censored_cdf()]
    result = {
        "status": "PASS", "scope": "abstract_rational_target_controls_only",
        "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "geometric_realizability_claimed": False, "engine_qualification": False,
        "training_performed": False, "tests_passed": len(cases),
        "incorrect_variants_rejected": sum(len(case["incorrect_variants"]) for case in cases),
        "cases": cases,
    }
    print(json.dumps(result, default=str, indent=2, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, TypeError, ZeroDivisionError) as error:
        print(json.dumps({"status": "FAIL", "error": str(error)}, sort_keys=True))
        sys.exit(1)
