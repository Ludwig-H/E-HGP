#!/usr/bin/env python3
"""Falsificateur rationnel autonome des interfaces de coupes du pilote.

Ces fixtures portent sur des matrices, incidences et masses abstraites. Elles
ne prétendent ni provenir de nuages réalisables ni qualifier Morse HGP 3D.
Aucun import du moteur, aucune donnée extérieure, aucun assert : les mêmes
contrôles s'exécutent avec Python normal et avec python -O.
"""

from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys


F = Fraction
Matrix = list[list[Fraction]]


class CheckFailure(RuntimeError):
    pass


class QuotientFailure(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise CheckFailure(message)


def matrix(rows):
    result = [[F(value) for value in row] for row in rows]
    require(bool(result) and bool(result[0]), "empty matrix")
    require(all(len(row) == len(result[0]) for row in result), "ragged matrix")
    return result


def transpose(a: Matrix) -> Matrix:
    return [list(column) for column in zip(*a)]


def multiply(a: Matrix, b: Matrix) -> Matrix:
    require(len(a[0]) == len(b), "matrix dimensions do not compose")
    return [
        [sum((x * y for x, y in zip(row, column)), F(0))
         for column in transpose(b)]
        for row in a
    ]


def column(values):
    return [[F(value)] for value in values]


def flatten(a: Matrix) -> list[Fraction]:
    require(all(len(row) == 1 for row in a), "expected a column")
    return [row[0] for row in a]


def row_sums(a: Matrix) -> list[Fraction]:
    return [sum(row, F(0)) for row in a]


def stochastic(a: Matrix) -> bool:
    return all(value >= 0 for row in a for value in row) and all(
        total == 1 for total in row_sums(a)
    )


def hard(a: Matrix) -> bool:
    return stochastic(a) and all(value in (0, 1) for row in a for value in row)


def transport_mass(p: Matrix, mass: list[Fraction]) -> list[Fraction]:
    return flatten(multiply(transpose(p), column(mass)))


def mean_restriction(p: Matrix, mass: list[Fraction]) -> tuple[list[Fraction], Matrix]:
    """Q = diag(P^T mass)^-1 P^T diag(mass), positive target masses only."""
    target_mass = transport_mass(p, mass)
    require(all(value > 0 for value in target_mass), "zero target mass")
    q = [
        [p[i][j] * mass[i] / target_mass[j] for i in range(len(p))]
        for j in range(len(p[0]))
    ]
    return target_mass, q


def quotient(source_labels: list[int], target_labels: list[int],
             source_count: int, target_count: int) -> Matrix:
    """A hard quotient exists precisely when each nonempty fibre is constant."""
    if len(source_labels) != len(target_labels):
        raise QuotientFailure("different atom universes")
    fibres = [set() for _ in range(source_count)]
    for source, target in zip(source_labels, target_labels):
        if not 0 <= source < source_count or not 0 <= target < target_count:
            raise QuotientFailure("label outside declared domain")
        fibres[source].add(target)
    if any(len(images) != 1 for images in fibres):
        raise QuotientFailure("empty or split source fibre")
    destinations = [next(iter(images)) for images in fibres]
    return matrix([
        [int(target == destination) for target in range(target_count)]
        for destination in destinations
    ])


def encoding(labels: list[int], count: int) -> Matrix:
    return matrix([[int(label == index) for index in range(count)]
                   for label in labels])


def condense_multifusion(masses, alpha):
    """Simultaneous rule: zero heavy branches end, one continues, >=2 split."""
    require(0 < alpha <= 1, "alpha outside (0, 1]")
    require(all(mass > 0 for mass in masses), "nonpositive child mass")
    threshold = alpha * sum(masses, F(0))
    heavy = [index for index, mass in enumerate(masses) if mass >= threshold]
    dropped = [index for index in range(len(masses)) if index not in heavy]
    action = "terminate" if not heavy else "continue_parent" if len(heavy) == 1 else "split"
    return {"action": action, "heavy": heavy, "dropped": dropped,
            "threshold": threshold}


def record(name, values, incorrect_variants):
    require(bool(incorrect_variants), name + ": no counterexample")
    require(all(rejected for rejected in incorrect_variants.values()),
            name + ": incorrect variant was not rejected")
    return {"name": name, "status": "PASS", "values": values,
            "incorrect_variants": {variant: "REJECTED"
                                   for variant in sorted(incorrect_variants)}}


def fixed_universe():
    # W maps three points to four frozen atoms; A and B partition those atoms.
    w = matrix([["1/2", "1/3", "1/6", 0],
                [0, "1/4", "1/4", "1/2"],
                [0, 0, "1/3", "2/3"]])
    a = encoding([0, 0, 1, 2], 3)
    b = encoding([0, 0, 0, 1], 2)
    h = matrix([[1, 0], [1, 0], [0, 1]])
    fine = multiply(w, a)
    coarse = multiply(w, b)
    expected_fine = matrix([["5/6", "1/6", 0],
                            ["1/4", "1/4", "1/2"],
                            [0, "1/3", "2/3"]])
    expected_coarse = matrix([[1, 0], ["1/2", "1/2"], ["1/3", "2/3"]])
    require(stochastic(w) and hard(a) and hard(b) and hard(h), "fixed: partitions")
    require(fine == expected_fine and coarse == expected_coarse, "fixed: expected values")
    require(multiply(a, h) == b and multiply(fine, h) == coarse, "fixed: composition")
    # Same labels with different scores do not preserve the weighted diagram.
    changed_weights = matrix([["3/4", "1/4", 0, 0],
                              [0, "1/2", "1/4", "1/4"],
                              [0, 0, "1/3", "2/3"]])
    changed_coarse = multiply(changed_weights, b)
    missing_atom_partition = matrix([[1, 0], [1, 0], [0, 0], [0, 1]])
    return record("fixed_universe_and_weights", {"fine": fine, "coarse": coarse,
                  "coarse_after_score_change": changed_coarse}, {
        "recompute_scores_without_changing_ids": multiply(fine, h) != changed_coarse,
        "omit_one_atom_from_partition": not stochastic(multiply(w, missing_atom_partition)),
    })


def partial_residual():
    retained = matrix([["1/4"], [0]])
    augmented = matrix([["1/4", "3/4", 0], [0, 0, 1]])
    deficits = [1 - total for total in row_sums(retained)]
    require(deficits == [F(3, 4), F(1)], "residual: partial missing mass")
    require(stochastic(augmented), "residual: augmented partition")
    masses = transport_mass(augmented, [F(1), F(1)])
    require(masses == [F(1, 4), F(3, 4), F(1)], "residual: target masses")
    naive_only_empty = matrix([["1/4", 0, 0], [0, 0, 1]])
    renormalized_retained = matrix([[1, 0, 0], [0, 0, 1]])
    return record("partial_mass_requires_residual", {"deficits": deficits,
                  "augmented": augmented, "masses": masses}, {
        "rescue_only_empty_rows": not stochastic(naive_only_empty),
        "renormalize_survivors_and_change_vote": renormalized_retained != augmented,
    })


def weighted_means():
    p = matrix([[1, 0], [1, 0], [0, 1]])
    h = matrix([[1], [1]])
    mass = [F(1)] * 3
    values = column([0, 2, 12])
    fine_mass, q = mean_restriction(p, mass)
    fine_mean = multiply(q, values)
    coarse_mass, coarse_q = mean_restriction(h, fine_mass)
    sequential = multiply(coarse_q, fine_mean)
    _, direct_q = mean_restriction(multiply(p, h), mass)
    direct = multiply(direct_q, values)
    naive = sum(flatten(fine_mean), F(0)) / len(fine_mean)
    require(fine_mass == [F(2), F(1)], "means: fine masses")
    require(fine_mean == column([1, 12]), "means: fine means")
    require(coarse_mass == [F(3)] and direct == column([F(14, 3)]), "means: direct")
    require(sequential == direct and multiply(coarse_q, q) == direct_q, "means: composition")
    require(naive == F(13, 2), "means: naive counterexample")
    return record("mean_composition_transports_mass", {"fine_mass": fine_mass,
                  "fine_mean": fine_mean, "correct_coarse": sequential,
                  "naive_coarse": naive}, {
        "average_child_means_uniformly": naive != direct[0][0],
        "reset_child_mass_to_one": mean_restriction(h, [F(1), F(1)])[1] != coarse_q,
    })


def pur_and_skip():
    p = matrix([[1, 0], ["1/2", "1/2"], [0, 1]])
    mass = [F(1)] * 3
    target_mass, q = mean_restriction(p, mass)
    original = column([0, 0, 3])
    encoded = multiply(q, original)
    decoded = multiply(p, encoded)
    require(multiply(q, column([1, 1, 1])) == column([1, 1]), "PUR: restriction constants")
    require(multiply(p, column([1, 1])) == column([1, 1, 1]), "PUR: prolongation constants")
    require(encoded == column([0, 2]) and decoded == column([0, 1, 2]), "PUR: smoothing")
    require(sum(flatten(decoded), F(0)) == sum(flatten(original), F(0)), "PUR: integral")
    residual = [[left[0] - right[0]] for left, right in zip(original, decoded)]
    recovered = [[left[0] + right[0]] for left, right in zip(decoded, residual)]
    require(recovered == original, "PUR: explicit linear residual reconstruction")
    # Blindly adding the input skip duplicates constants; a learned skip has no
    # automatic exact-reconstruction or mass-conservation interpretation.
    doubled_constant = [[a[0] + b[0]] for a, b in zip(
        multiply(p, multiply(q, column([1, 1, 1]))), column([1, 1, 1]))]
    require(doubled_constant == column([2, 2, 2]), "PUR: plain skip counterexample")
    return record("pur_is_not_inverse_and_skip_is_explicit", {"mass": target_mass,
                  "original": original, "decoded": decoded, "linear_residual": residual}, {
        "claim_pool_then_pur_is_identity": decoded != original,
        "plain_additive_skip_preserves_constants": doubled_constant != column([1, 1, 1]),
    })


def fibre_quotient():
    sources = [0, 0, 1, 2]
    targets = [0, 0, 1, 1]
    good = quotient(sources, targets, 3, 2)
    require(good == matrix([[1, 0], [0, 1], [0, 1]]), "quotient: expected map")
    require(multiply(encoding(sources, 3), good) == encoding(targets, 2), "quotient: diagram")
    split_targets = [0, 1, 1, 1]
    split_rejected = False
    try:
        quotient(sources, split_targets, 3, 2)
    except QuotientFailure:
        split_rejected = True
    require(split_rejected, "quotient: split fibre accepted")
    incomplete_rejected = False
    try:
        quotient(sources, targets[:-1], 3, 2)
    except QuotientFailure:
        incomplete_rejected = True
    require(incomplete_rejected, "quotient: incomplete atom universe accepted")
    pick_first = good
    return record("hard_quotient_needs_constant_fibres", {"valid_map": good,
                  "split_rejected": split_rejected,
                  "different_universe_rejected": incomplete_rejected}, {
        "pick_first_image_of_split_fibre": multiply(encoding(sources, 3), pick_first)
            != encoding(split_targets, 2),
    })


def cross_order():
    source = matrix([["1/2", "1/4", "1/4"]])
    parent = matrix([[1, 0], [1, 0], [0, 1]])
    recomputed_target = matrix([["1/2", "1/2"]])
    transported = multiply(source, parent)
    require(hard(parent) and stochastic(source) and stochastic(recomputed_target), "cross-K: inputs")
    require(transported == matrix([["3/4", "1/4"]]), "cross-K: transported weights")
    return record("cross_order_parent_does_not_fix_weights", {
        "parent": parent, "transported": transported,
        "independently_normalized_target": recomputed_target,
    }, {"infer_weighted_commutation_from_parent_only": transported != recomputed_target})


def births_split_residual():
    fine = matrix([[1]])
    coarse = matrix([["1/2", "1/2"]])
    possible_hard_parents = [matrix([[1, 0]]), matrix([[0, 1]])]
    failures = [multiply(fine, parent) != coarse for parent in possible_hard_parents]
    require(all(failures), "births: an impossible hard map was found")
    require(multiply(fine, coarse) == coarse, "births: soft incidence can split")
    return record("new_atoms_cannot_split_a_hard_residual_parent", {
        "fine_residual": fine, "new_coarse_incidence": coarse,
        "hard_parent_candidates_rejected": len(failures),
    }, {"reuse_residual_as_unique_new_parent": all(failures)})


def duplicate_measures():
    # Three returns share site A and one belongs to B. Geometry stays unchanged.
    return_to_site = matrix([[1, 0], [1, 0], [1, 0], [0, 1]])
    site_to_token = matrix([[1], [1]])
    p = multiply(return_to_site, site_to_token)
    site_measure = [F(1, 3), F(1, 3), F(1, 3), F(1)]
    return_measure = [F(1)] * 4
    values = column([0, 0, 0, 10])
    site_mass, site_q = mean_restriction(p, site_measure)
    return_mass, return_q = mean_restriction(p, return_measure)
    site_mean = multiply(site_q, values)
    return_mean = multiply(return_q, values)
    require(transport_mass(return_to_site, site_measure) == [F(1), F(1)], "duplicates: site measure")
    require(transport_mass(return_to_site, return_measure) == [F(3), F(1)], "duplicates: return measure")
    require(site_mass == [F(2)] and site_mean == column([5]), "duplicates: geometric convention")
    require(return_mass == [F(4)] and return_mean == column([F(5, 2)]), "duplicates: empirical convention")
    require(multiply(return_to_site, column([7, 11])) == column([7, 7, 7, 11]), "duplicates: lift every return")
    return record("duplicate_returns_require_a_declared_measure", {
        "site_mass": site_mass, "return_mass": return_mass,
        "site_mean": site_mean, "return_mean": return_mean,
    }, {"silently_count_returns_as_geometric_sites": site_mean != return_mean})


def early_routing():
    weights = [F(3, 10), F(3, 10), F(4, 10)]
    parent_scores = [weights[0] + weights[1], weights[2]]
    require(parent_scores == [F(3, 5), F(2, 5)], "routing: parent scores")
    chosen_parent = max(range(2), key=lambda index: (parent_scores[index], -index))
    require(chosen_parent == 0, "routing: parent A wins")
    early_leaf = max([0, 1], key=lambda index: (weights[index], -index))
    flat_leaf = max(range(3), key=lambda index: (weights[index], -index))
    require(early_leaf == 0 and flat_leaf == 2, "routing: expected disagreement")
    return record("early_hard_routing_is_not_vote_after_selection", {
        "face_weights": weights, "parent_scores": parent_scores,
        "early_leaf": early_leaf, "flat_vote_leaf": flat_leaf,
    }, {"use_irreversible_routing_as_soft_vote_oracle": early_leaf != flat_leaf})


def probability_not_logits():
    assignment = matrix([["3/4", "1/4"]])
    probabilities = matrix([["4/5", "1/5"], ["1/100", "99/100"]])
    mixture = multiply(assignment, probabilities)
    require(mixture == matrix([["241/400", "159/400"]]), "probabilities: mixture")
    # If z_v=log(pi_v), the fourth power of the pooled-logit odds is
    # (pi_1[A]/pi_1[B])^3 * (pi_2[A]/pi_2[B]); no floating log is needed.
    odds_fourth_power = (probabilities[0][0] / probabilities[0][1]) ** 3 * (
        probabilities[1][0] / probabilities[1][1])
    require(odds_fourth_power == F(64, 99), "probabilities: exact logit odds")
    probability_winner = 0 if mixture[0][0] > mixture[0][1] else 1
    logit_winner = 0 if odds_fourth_power > 1 else 1
    require(probability_winner == 0 and logit_winner == 1, "probabilities: expected winners")
    return record("pur_mixes_probabilities_before_argmax", {
        "probability_mixture": mixture, "pooled_logit_odds_fourth_power": odds_fourth_power,
        "probability_winner": probability_winner, "pooled_logit_winner": logit_winner,
    }, {"replace_probability_mixture_with_softmax_of_mixed_logits": probability_winner != logit_winner})


def multifusions():
    zero = condense_multifusion([F(4), F(3), F(3)], F(1, 2))
    one = condense_multifusion([F(8), F(1), F(1)], F(1, 4))
    two = condense_multifusion([F(6), F(3), F(1)], F(1, 4))
    boundary = condense_multifusion([F(2), F(1), F(1)], F(1, 4))
    require(zero == {"action": "terminate", "heavy": [], "dropped": [0, 1, 2],
                     "threshold": F(5)}, "multifusion: zero survivors")
    require(one == {"action": "continue_parent", "heavy": [0], "dropped": [1, 2],
                    "threshold": F(5, 2)}, "multifusion: one survivor")
    require(two == {"action": "split", "heavy": [0, 1], "dropped": [2],
                    "threshold": F(5, 2)}, "multifusion: two survivors")
    require(boundary["action"] == "split" and boundary["heavy"] == [0, 1, 2], "multifusion: inclusive boundary")
    # An artificial binary tree changes the relative threshold: groups (6,4)
    # survive at the top, then (3,1) both survive their invented parent of mass 4.
    fake_top = condense_multifusion([F(6), F(4)], F(1, 4))
    fake_inner = condense_multifusion([F(3), F(1)], F(1, 4))
    require(fake_top["action"] == "split" and fake_inner["action"] == "split", "multifusion: binary counterexample")
    return record("multifusion_is_atomic_with_zero_one_or_many_survivors", {
        "zero": zero, "one": one, "two": two, "inclusive_boundary": boundary,
        "artificial_binary_inner": fake_inner,
    }, {
        "require_all_children_heavy_to_split": len(two["heavy"]) >= 2 and bool(two["dropped"]),
        "create_new_identity_for_one_survivor": one["action"] != "split",
        "keep_largest_child_when_none_heavy": zero["action"] == "terminate",
        "binarize_and_recompute_parent_threshold": bool(two["dropped"]) and not fake_inner["dropped"],
        "use_strict_threshold": len(boundary["heavy"]) != 1,
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
    checks = [fixed_universe, partial_residual, weighted_means, pur_and_skip,
              fibre_quotient, cross_order, births_split_residual,
              duplicate_measures, early_routing, probability_not_logits, multifusions]
    results = []
    for check in checks:
        try:
            results.append(check())
        except Exception as error:
            results.append({"name": check.__name__, "status": "FAIL",
                            "error": type(error).__name__ + ": " + str(error)})
    passed = all(result["status"] == "PASS" for result in results)
    payload = {
        "schema": "zoltan.cut_algebra.v1",
        "scope": "abstract_rational_algebra_only",
        "engine_qualification": False,
        "geometric_realizability_claimed": False,
        "weight_scope": "supplied_rational_weights_not_general_exact_inverse_radius_cubed",
        "condensation_scope": "child_masses_only_no_mass_directly_attached_to_event",
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
