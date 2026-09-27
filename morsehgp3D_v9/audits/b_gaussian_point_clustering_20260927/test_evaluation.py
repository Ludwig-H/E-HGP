#!/usr/bin/env python3
"""Offline metric fixtures and independent small-set oracles; no captures."""
from __future__ import annotations

import copy
from fractions import Fraction
import itertools
import json
import random

from condensed import point_clusterer_from_tree
from evaluation import evaluate_labels, tree_recoverability, condensed_recoverability


def need(value, message):
    if not value:
        raise RuntimeError(message)


def close(actual, expected):
    need(abs(actual - float(expected)) < 1e-13, f"{actual!r} != {expected!r}")


def refuse(function, *args, **kwargs):
    try:
        function(*args, **kwargs)
    except (ValueError, KeyError, TypeError):
        return
    raise RuntimeError("invalid input accepted")


def balanced():
    return dict(n=8, children={8: [0, 1], 9: [2, 3], 10: [4, 5], 11: [6, 7],
                             12: [8, 9], 13: [10, 11], 14: [12, 13]},
                heights={8: 1, 9: 1, 10: 1, 11: 1, 12: 2, 13: 2, 14: 3})


def brute_match(truth, labels):
    actual = sorted(set(x for x in truth if x >= 0))
    predicted = sorted(set(x for x in labels if x >= 0))
    # Add dummy zero-score columns so every true row is assigned independently
    # of the production rectangular Hungarian solver.
    columns = predicted + [None] * max(0, len(actual) - len(predicted))
    best = 0
    for permutation in itertools.permutations(range(len(columns)), len(actual)):
        value = sum(sum(t == a and p == columns[j] for t, p in zip(truth, labels))
                    for a, j in zip(actual, permutation))
        best = max(best, value)
    return best


def oracle_raw(truth, tree, minimum):
    children, heights, n = tree["children"], tree["heights"], tree["n"]
    parents = {child: node for node, row in children.items() for child in row}
    root = next(node for node in set(children) | set(range(n)) if node not in parents)
    members = {point: {point} for point in range(n)}
    # The random fixtures below create every parent after all its children.
    for node in sorted(children):
        members[node] = set().union(*(members[child] for child in children[node]))
    candidates = [members[node] for node in children if node != root and
                  heights[node] != heights[parents[node]] and
                  (minimum is None or len(members[node]) >= minimum)]
    scores = []
    for label in sorted(set(x for x in truth if x >= 0)):
        target = {p for p, value in enumerate(truth) if value == label}
        scores.append(max((Fraction(2 * len(target & group), len(target) + len(group))
                           for group in candidates), default=Fraction(0)))
    return candidates, scores


def main():
    checks = 0
    truth = [0, 0, 1, 1]
    perfect = evaluate_labels(truth, [9, 9, 2, 2], min_cluster_size=2)
    need(perfect["exact_classes"] == 2 and perfect["cluster_count_error"] == 0, "permuted exact recovery")
    close(perfect["matched_macro_f1"], 1)
    need(perfect["eligible_classes"] == 2, "size eligibility")
    checks += 1

    partial = evaluate_labels(truth, [0, 0, -1, -1], min_cluster_size=3)
    need(partial["exact_classes"] == 1 and partial["cluster_count_error"] == -1, "noise is not a cluster")
    need(partial["classes_below_min_cluster_size"] == 2, "ineligible truth classes not silently removed")
    close(partial["matched_macro_recall"], Fraction(1, 2))
    close(partial["matched_micro_precision"], 1)
    close(partial["matched_micro_f1"], Fraction(2, 3))
    checks += 1

    all_noise = evaluate_labels(truth, [-1] * 4)
    need(all_noise["exact_classes"] == 0 and all_noise["matched_correct_points"] == 0, "abstention not exact recovery")
    close(all_noise["matched_macro_f1"], 0)
    true_noise = evaluate_labels([-1, -1], [4, 4])
    need(true_noise["truth_classes"] == 0 and true_noise["exact_class_fraction"] is None,
         "no true classes does not claim exact recovery")
    checks += 1

    contaminated = evaluate_labels([0, 0, 1, 1, -1], [9, 9, 8, 8, 9])
    need(contaminated["exact_classes"] == 1, "true noise remains contamination")
    close(contaminated["per_class"][0]["precision"], Fraction(2, 3))
    close(contaminated["matched_micro_precision"], Fraction(4, 5))
    split = evaluate_labels(truth, [1, 2, 3, 4])
    need(split["predicted_clusters"] == 4 and split["cluster_count_absolute_error"] == 2, "extra unmatched clusters count")
    close(split["matched_micro_precision"], Fraction(1, 2))
    close(split["matched_micro_recall"], Fraction(1, 2))
    checks += 1

    # Labels are categorical integers; negative values are all abstentions.
    huge = evaluate_labels([10**30, 10**30], [10**35, 10**35])
    need(huge["exact_classes"] == 1, "no large-label integer wrap")
    for a, b in (([], []), ([0.0], [0]), ([True], [0]), ([0], [1.0]), ([0], [0, 1]), ([[0]], [0])):
        refuse(evaluate_labels, a, b)
    refuse(evaluate_labels, truth, truth, min_cluster_size=1)
    checks += 1

    rng = random.Random(20260927)
    for _ in range(180):
        t = [rng.randrange(-1, 3) for _ in range(8)]
        p = [rng.randrange(-1, 4) for _ in range(8)]
        score = evaluate_labels(t, p)
        need(score["matched_correct_points"] == brute_match(t, p), "independent exhaustive matching oracle")
        exact = sum(any({i for i, x in enumerate(t) if x == c} ==
                        {i for i, x in enumerate(p) if x == d}
                        for d in set(p) if d >= 0) for c in set(t) if c >= 0)
        need(score["exact_classes"] == exact, "independent exact community oracle")
    checks += 1

    tree = balanced()
    four = [0, 0, 1, 1, 2, 2, 3, 3]
    raw = tree_recoverability(four, tree)
    need(raw["candidate_clusters"] == 6 and raw["exact_classes"] == 4, "raw exact branch recovery")
    close(raw["macro_best_f1"], 1)
    filtered = tree_recoverability(four, tree, min_cluster_size=3)
    close(filtered["macro_best_f1"], Fraction(2, 3))
    need(filtered["candidate_clusters"] == 2, "raw size-filtering")
    need(tree_recoverability(four, tree, min_cluster_size=5)["candidate_clusters"] == 0, "root always excluded")
    checks += 1

    atomic = copy.deepcopy(tree)
    atomic["heights"] = {node: 1 for node in tree["children"]}
    flat = tree_recoverability(four, atomic)
    need(flat["candidate_clusters"] == 0 and flat["macro_best_f1"] == 0, "fake equal-height branches removed")
    mixed = tree_recoverability([0, 1, 0, 1, 2, 3, 2, 3], tree)
    close(mixed["macro_best_f1"], Fraction(2, 3))
    checks += 1

    for m in (2, 3, 5, 100):
        values = []
        for z in (1, 2):
            output = point_clusterer_from_tree(tree, m, z)
            c = condensed_recoverability(four, output["condensed_tree"])
            need(c["candidate_clusters"] == output["stats"]["condensed_nonroot_clusters"], "all non-root clusters evaluated")
            need(output["stats"]["point_exits"] == 8, "no points silently dropped")
            values.append(c)
        need(values[0] == values[1], "lambda exponent cannot change condensed birth memberships")
        expected = 1 if m == 2 else Fraction(2, 3) if m == 3 else 0
        close(values[0]["macro_best_f1"], expected)
    checks += 1

    # A size-eligible raw branch can prolong the structural root and therefore
    # is deliberately not a selectable/non-root condensed branch.
    continuation = dict(n=5, children={5: [0, 1], 6: [2, 3], 7: [5, 6], 8: [7, 4]},
                        heights={5: 1, 6: 1, 7: 2, 8: 3})
    targets = [0, 0, 0, 0, 1]
    raw_score = tree_recoverability(targets, continuation, min_cluster_size=2)
    clean = point_clusterer_from_tree(continuation, 2)["condensed_tree"]
    condensed_score = condensed_recoverability(targets, clean)
    close(raw_score["per_class"][0]["best_f1"], 1)
    close(condensed_score["per_class"][0]["best_f1"], Fraction(2, 3))
    need(condensed_score["scope"].endswith("not_live_cuts"), "no false raw purity or cut equivalence")
    checks += 1

    # Compare full raw membership sets rather than the production count DP.
    for _ in range(100):
        n, active, children, height = 10, list(range(10)), {}, {i: 0 for i in range(10)}
        node = n
        while len(active) > 1:
            a, b = rng.sample(active, 2)
            active.remove(a); active.remove(b)
            children[node] = [a, b]
            height[node] = max(height[a], height[b]) + rng.randrange(0, 3)
            active.append(node)
            node += 1
        fixture = dict(n=n, children=children, heights=height)
        target = [rng.randrange(-1, 4) for _ in range(n)]
        for m in (None, 2, 4, 20):
            candidates, fractions = oracle_raw(target, fixture, m)
            result = tree_recoverability(target, fixture, min_cluster_size=m)
            need(result["candidate_clusters"] == len(candidates), "oracle candidate count")
            need([Fraction(row["best_f1_numerator"], row["best_f1_denominator"])
                  for row in result["per_class"]] == fractions, "independent raw set F1 oracle")
        for m in (2, 4):
            compact = point_clusterer_from_tree(fixture, m)["condensed_tree"]
            groups = [set() for _ in compact["parent"]]
            for point, end in enumerate(compact["point_exit_parent"]):
                # Independent point-by-point ancestry paths, not a bottom-up
                # vector accumulation as used by the production diagnostic.
                while end >= 0:
                    groups[end].add(point)
                    end = compact["parent"][end]
            score = condensed_recoverability(target, compact)
            for row in score["per_class"]:
                actual = {p for p, label in enumerate(target) if label == row["truth_label"]}
                expected = max((Fraction(2 * len(actual & group), len(actual) + len(group))
                                for group in groups[1:]), default=Fraction(0))
                need(Fraction(row["best_f1_numerator"], row["best_f1_denominator"]) == expected,
                     "independent condensed ancestry-set oracle")
            raw_filtered = tree_recoverability(target, fixture, min_cluster_size=m)
            need(all(a["best_f1"] <= b["best_f1"] for a, b in
                     zip(score["per_class"], raw_filtered["per_class"])),
                 "condensation cannot add a better raw branch birth set")
    checks += 1

    corrupt = copy.deepcopy(clean)
    corrupt["point_exit_ids"][0] = corrupt["point_exit_ids"][1]
    refuse(condensed_recoverability, targets, corrupt)
    refuse(condensed_recoverability, targets[:-1], clean)
    refuse(tree_recoverability, four[:-1], tree)
    # All outputs have standard JSON-safe scalars, including no-candidate cases.
    json.dumps([perfect, all_noise, true_noise, flat, condensed_score], allow_nan=False)
    checks += 1
    print(json.dumps(dict(status="PASS", checks=checks, exhaustive_matching_fixtures=180,
                          raw_tree_oracle_fixtures=100, raw_tree_oracle_evaluations=400,
                          condensed_tree_oracle_evaluations=200,
                          GPU_used=False, GCP_used=False), sort_keys=True))


if __name__ == "__main__":
    main()
