#!/usr/bin/env python3
"""Falsificateur rationnel des opérateurs de guidage inter-vues.

Les affectations sont des matrices abstraites fournies, pas des sorties HGP.
Les IDs désignent des retours communs déclarés ; aucun appariement géométrique,
export natif, entraînement ou propriété statistique n'est qualifié ici.
"""

from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys


F = Fraction


class CheckFailure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise CheckFailure(message)


def matrix(rows):
    result = [[F(value) for value in row] for row in rows]
    require(bool(result) and bool(result[0]), "empty matrix")
    require(all(len(row) == len(result[0]) for row in result), "ragged matrix")
    return result


def transpose(a):
    return [list(column) for column in zip(*a)]


def multiply(a, b):
    require(len(a[0]) == len(b), "incompatible matrix dimensions")
    return [[sum((x * y for x, y in zip(row, column)), F(0))
             for column in transpose(b)] for row in a]


def sums(a):
    return [sum(row, F(0)) for row in a]


def coupling(a, b, measure, widths):
    """G = A_J^T diag(nu_J) B_J; rows may omit declared reserve mass."""
    na, nb = widths
    require(na > 0 and nb > 0, "nonpositive token count")
    for view, width in ((a, na), (b, nb)):
        for row in view.values():
            require(len(row) == width, "assignment width mismatch")
            require(all(value >= 0 for value in row), "negative membership")
            require(sum(row, F(0)) <= 1, "membership sum exceeds one")
    shared = sorted(set(a).intersection(b))
    require(all(i in measure and measure[i] > 0 for i in shared),
            "missing or nonpositive common comparison measure")
    g = [[sum((measure[i] * a[i][u] * b[i][v] for i in shared), F(0))
          for v in range(nb)] for u in range(na)]
    return shared, g


def conditional(g):
    """Condition on matched mass; None is an explicit no-target row."""
    return [[value / total for value in row] if total > 0 else None
            for row, total in zip(g, sums(g))]


def token_mass(view, shared, measure, count):
    return [sum((measure[i] * view[i][u] for i in shared), F(0))
            for u in range(count)]


def pooled(view, shared, measure, features, count):
    mass = token_mass(view, shared, measure, count)
    return [sum((measure[i] * view[i][u] * features[i] for i in shared), F(0)) / m
            if m > 0 else None for u, m in enumerate(mass)]


def squared_distance(a, b):
    require(len(a) == len(b), "feature dimensions differ")
    return sum(((x - y) ** 2 for x, y in zip(a, b)), F(0))


def graph_energy(g, a, b):
    require(len(g) == len(a) and len(g[0]) == len(b), "graph feature dimensions")
    return sum((g[u][v] * (a[u] - b[v]) ** 2
                for u in range(len(a)) for v in range(len(b))), F(0))


def record(name, values, incorrect_variants):
    require(bool(incorrect_variants), name + ": missing counterexample")
    require(all(incorrect_variants.values()), name + ": unrefuted variant")
    return {"name": name, "status": "PASS", "values": values,
            "incorrect_variants": {key: "REJECTED" for key in sorted(incorrect_variants)}}


def shared_ids_and_measure():
    a = dict(zip([10, 20, 30], matrix([[1, 0], ["1/2", "1/2"], [0, 1]])))
    b = dict(zip([20, 30, 40], matrix([[1, 0], ["1/4", "3/4"], [0, 1]])))
    nu = {10: F(1), 20: F(1), 30: F(2), 40: F(1)}
    shared, g = coupling(a, b, nu, (2, 2))
    require(shared == [20, 30], "IDs: expected intersection")
    require(g == matrix([["1/2", 0], [1, "3/2"]]), "IDs: exact coupling")
    ma = token_mass(a, shared, nu, 2)
    mb = token_mass(b, shared, nu, 2)
    require(ma == [F(1, 2), F(5, 2)] and mb == [F(3, 2), F(3, 2)], "IDs: masses")
    require(sums(g) == ma and sums(transpose(g)) == mb, "IDs: marginals")
    require(sum(sums(g), F(0)) == 3, "IDs: shared total")
    # Position-wise pairing invents links between different original returns.
    wrong_g = multiply(transpose(list(a.values())), list(b.values()))
    other_measure = {20: F(1), 30: F(1)}
    incompatible_mb = token_mass(b, shared, other_measure, 2)
    return record("shared_original_ids_and_one_comparison_measure", {
        "shared_ids": shared, "coupling": g, "source_mass": ma, "target_mass": mb,
        "row_conditionals": conditional(g), "position_paired_coupling": wrong_g,
        "target_mass_under_another_measure": incompatible_mb,
    }, {
        "pair_rows_by_position_instead_of_original_id": wrong_g != g,
        "claim_two_view_specific_measures_have_one_conserved_coupling":
            sum(ma, F(0)) != sum(incompatible_mb, F(0)),
    })


def null_and_partial_reserve():
    nu = {7: F(2)}
    a, b = {7: [F(1, 4)]}, {7: [F(1, 2)]}
    shared, g = coupling(a, b, nu, (1, 1))
    eligible = token_mass(a, shared, nu, 1)
    matched = sums(g)
    confidence = [matched[0] / eligible[0]]
    require(g == matrix([["1/4"]]) and eligible == [F(1, 2)], "reserve: exact masses")
    require(confidence == [F(1, 2)] and conditional(g) == matrix([[1]]), "reserve: conditional")
    augmented_a = {7: [F(1, 4), F(3, 4)]}
    augmented_b = {7: [F(1, 2), F(1, 2)]}
    _, complete = coupling(augmented_a, augmented_b, nu, (2, 2))
    require(complete == matrix([["1/4", "1/4"], ["3/4", "3/4"]]), "reserve: augmented")
    require(sum(sums(complete), F(0)) == 2, "reserve: total conserved")
    empty_ids, empty = coupling({1: [F(1)]}, {2: [F(1)]}, {}, (1, 1))
    _, unrepresented = coupling({7: [F(1)]}, {7: [F(0)]}, nu, (1, 1))
    require(empty_ids == [] and empty == matrix([[0]]), "empty: no invented common ID")
    require(conditional(empty) == [None] and conditional(unrepresented) == [None], "empty: mask")
    # A no-target row contributes no loss. Fabricated zero/uniform targets do.
    prediction, masked_loss = F(3), F(0)
    fabricated_zero_loss = prediction ** 2
    fabricated_uniform_loss = (prediction - 1) ** 2
    require(fabricated_zero_loss == 9 and fabricated_uniform_loss == 4, "empty: fabricated supervision")
    return record("partial_coverage_and_no_target_are_explicit", {
        "signal_coupling": g, "eligible_source_mass": eligible, "matched_mass": matched,
        "matched_fraction": confidence, "conditional_despite_partial_coverage": conditional(g),
        "with_reserve_channels": complete, "empty_overlap": empty,
        "empty_overlap_conditional": conditional(empty),
        "unrepresented_target_conditional": conditional(unrepresented),
        "masked_no_target_loss": masked_loss,
        "fabricated_zero_target_loss": fabricated_zero_loss,
        "fabricated_uniform_target_loss": fabricated_uniform_loss,
    }, {
        "call_row_normalized_signal_full_coverage": confidence != [F(1)],
        "discard_partial_reserve_without_recording_lost_mass": sum(sums(g), F(0)) != 2,
        "give_empty_overlap_a_uniform_or_zero_training_target":
            fabricated_zero_loss != masked_loss and fabricated_uniform_loss != masked_loss,
    })


def soft_self_transport():
    p = matrix([[1, 0], ["1/2", "1/2"], [0, 1]])
    view = dict(enumerate(p))
    _, g = coupling(view, view, {i: F(1) for i in view}, (2, 2))
    t = conditional(g)
    require(g == matrix([["5/4", "1/4"], ["1/4", "5/4"]]), "self: coupling")
    require(t == matrix([["5/6", "1/6"], ["1/6", "5/6"]]), "self: conditional")
    twice = multiply(t, t)
    require(twice == matrix([["13/18", "5/18"], ["5/18", "13/18"]]), "self: round trip")
    features = matrix([[0], [6]])
    transported = multiply(t, features)
    require(transported == matrix([[1], [5]]), "self: smoothing witness")
    return record("soft_overlap_transport_is_not_identity", {
        "assignment": p, "coupling": g, "conditional": t,
        "round_trip": twice, "features": features, "transported_features": transported,
    }, {
        "treat_same_view_overlap_as_exact_token_identity": t != matrix([[1, 0], [0, 1]]),
        "require_round_trip_to_preserve_arbitrary_features": multiply(twice, features) != features,
    })


def crop_conditioning():
    teacher = {1: [F(1)], 2: [F(1)]}
    student = {1: [F(1)]}
    nu = {1: F(1), 2: F(1)}
    teacher_features, student_features = {1: F(0), 2: F(10)}, {1: F(0)}
    shared, g = coupling(teacher, student, nu, (1, 1))
    full_mass = token_mass(teacher, [1, 2], nu, 1)
    shared_mass = token_mass(teacher, shared, nu, 1)
    full_mean = pooled(teacher, [1, 2], nu, teacher_features, 1)
    restricted_mean = pooled(teacher, shared, nu, teacher_features, 1)
    student_mean = pooled(student, shared, nu, student_features, 1)
    full_denominator = [[g[0][0] / full_mass[0]]]
    require(full_mass == [F(2)] and shared_mass == [F(1)], "crop: masses")
    require(full_mean == [F(5)] and restricted_mean == student_mean == [F(0)], "crop: means")
    require(full_denominator == matrix([["1/2"]]) and conditional(g) == matrix([[1]]), "crop: conditioning")
    completion_loss = squared_distance(full_mean, student_mean)
    invariance_loss = squared_distance(restricted_mean, student_mean)
    require(completion_loss == 25 and invariance_loss == 0, "crop: distinct tasks")
    return record("restrict_ids_before_pooling_for_overlap_agreement", {
        "common_ids": shared, "full_teacher_mass": full_mass,
        "shared_teacher_mass": shared_mass, "full_teacher_mean": full_mean,
        "shared_teacher_mean": restricted_mean, "student_mean": student_mean,
        "normalization_by_full_mass": full_denominator,
        "conditional_on_shared_mass": conditional(g),
        "privileged_completion_loss": completion_loss, "overlap_agreement_loss": invariance_loss,
    }, {
        "call_full_mass_normalization_a_conditional_probability": sums(full_denominator) != [F(1)],
        "equate_full_context_completion_with_shared_id_invariance": completion_loss != invariance_loss,
    })


def common_teacher_weights_and_collapse():
    p = matrix([[1, 0], ["1/2", "1/2"], [0, 1]])
    teacher_membership = dict(enumerate(p))
    shared = [0, 1, 2]
    nu = {i: F(1) for i in shared}
    features = {0: F(0), 1: F(2), 2: F(4)}
    teacher = pooled(teacher_membership, shared, nu, features, 2)
    student = pooled(teacher_membership, shared, nu, features, 2)
    constants = {i: F(7) for i in shared}
    collapsed_teacher = pooled(teacher_membership, shared, nu, constants, 2)
    collapsed_student = pooled(teacher_membership, shared, nu, constants, 2)
    require(teacher == student == [F(2, 3), F(10, 3)], "pool: same mask and weights")
    require(collapsed_teacher == collapsed_student == [F(7), F(7)], "pool: constant fixed point")
    consistency_loss = squared_distance(collapsed_teacher, collapsed_student)
    pairs = [(0, 1), (0, 2), (1, 2)]
    fixed_targets = [squared_distance(p[i], p[j]) for i, j in pairs]
    require(fixed_targets == [F(1, 2), F(2), F(1, 2)], "relations: exact targets")
    collapse_relation_loss = sum((target ** 2 for target in fixed_targets), F(0))
    attained_relation_loss = sum(((squared_distance(p[i], p[j]) - target) ** 2
                                  for (i, j), target in zip(pairs, fixed_targets)), F(0))
    require(consistency_loss == 0 and collapse_relation_loss == F(9, 2), "relations: nonconstant target")
    require(attained_relation_loss == 0, "relations: feasible nonconstant representation")
    return record("common_teacher_weights_align_aggregation_but_do_not_prevent_collapse", {
        "common_weight_pooled_features": teacher,
        "collapsed_teacher_and_student": collapsed_teacher,
        "collapsed_consistency_loss": consistency_loss,
        "fixed_membership_squared_distance_targets": fixed_targets,
        "constant_feature_relation_loss": collapse_relation_loss,
        "membership_feature_relation_loss": attained_relation_loss,
        "scope": "one_fixed_assignment_three_pairs_not_a_semantic_or_training_guarantee",
    }, {
        "claim_identical_pooling_or_stop_gradient_excludes_constant_representations": consistency_loss == 0,
        "claim_fixed_nonconstant_relation_targets_accept_constant_features_at_zero_loss":
            collapse_relation_loss > attained_relation_loss,
    })


def graph_agreement_smooths():
    g = matrix([["5/4", "1/4"], ["1/4", "5/4"]])
    distinct = [F(0), F(6)]
    constants = [F(3), F(3)]
    exact_identity_energy = graph_energy(g, distinct, distinct)
    collapsed_energy = graph_energy(g, constants, constants)
    require(exact_identity_energy == 18 and collapsed_energy == 0, "graph: smoothness energy")
    disconnected = matrix([[2, 0], [0, 3]])
    separate_constant_energy = graph_energy(disconnected, distinct, distinct)
    mean = sum(distinct, F(0)) / len(distinct)
    variance = sum(((value - mean) ** 2 for value in distinct), F(0)) / len(distinct)
    require(separate_constant_energy == 0 and variance == 9, "graph: component-wise collapse")
    return record("all_positive_overlap_edges_as_positives_encourage_component_collapse", {
        "overlap_graph": g, "identical_distinct_token_features": distinct,
        "identity_feature_energy": exact_identity_energy, "constant_feature_energy": collapsed_energy,
        "disconnected_graph": disconnected, "component_constant_energy": separate_constant_energy,
        "component_constant_global_variance": variance,
    }, {
        "interpret_all_overlap_edges_as_preserving_distinct_token_features": exact_identity_energy > collapsed_energy,
        "infer_within_component_diversity_from_positive_global_variance":
            separate_constant_energy == 0 and variance > 0,
    })


def json_value(value):
    if isinstance(value, Fraction):
        return str(value.numerator) + "/" + str(value.denominator)
    if isinstance(value, dict):
        return {key: json_value(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_value(item) for item in value]
    return value


def main():
    checks = [shared_ids_and_measure, null_and_partial_reserve, soft_self_transport,
              crop_conditioning, common_teacher_weights_and_collapse, graph_agreement_smooths]
    results = []
    for check in checks:
        try:
            results.append(check())
        except Exception as error:
            results.append({"name": check.__name__, "status": "FAIL",
                            "error": type(error).__name__ + ": " + str(error)})
    passed = all(result["status"] == "PASS" for result in results)
    payload = {
        "schema": "zoltan.guidance_algebra.v1",
        "scope": "abstract_rational_algebra_only",
        "engine_qualification": False,
        "geometric_realizability_claimed": False,
        "training_claimed": False,
        "weight_scope": "supplied_rational_memberships_and_common_return_measure",
        "correspondence_scope": "intersection_of_declared_original_return_ids_only",
        "noncollapse_scope": "constant_representation_falsified_for_one_fixed_nonconstant_relation_target",
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
