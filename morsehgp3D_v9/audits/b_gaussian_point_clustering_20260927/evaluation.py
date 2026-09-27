#!/usr/bin/env python3
"""Predeclared supervised diagnostics, never inputs to clustering or EOM.

The runner keeps the frozen ARI/NMI/coverage and raw dendrogram-purity metrics.
These extras distinguish exact set recovery, one-to-one label matching, and
the *oracle* representability of each true class by a single tree branch.
Oracle branches need not form a partition or be jointly EOM-selectable.

Gaussian truth denotes generating components, not necessarily density modes.
Overlapping components need not be exactly recoverable from positions. Known-
parameter MAP is a separate model-aware classification diagnostic, neither a
clustering baseline nor a finite-sample upper bound on ARI. No truth-dependent
parameter choice, geometry computation, input deletion, or filesystem write.
"""
from __future__ import annotations

from fractions import Fraction
from numbers import Integral

import numpy as np
from scipy.optimize import linear_sum_assignment

from condensed import _legacy, validate_condensed_tree


def _labels(values, name):
    array = np.asarray(values)
    if array.ndim != 1 or not len(array):
        raise ValueError(name + ": nonempty one-dimensional integer labels required")
    if any(isinstance(x, (bool, np.bool_)) or not isinstance(x, Integral)
           for x in array.tolist()):
        raise ValueError(name + ": labels must be integers, not rounded floats")
    # Python integers avoid accidental wraparound of large external label IDs.
    return [int(x) for x in array.tolist()]


def _minimum(value):
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, Integral) or value < 2:
        raise ValueError("min_cluster_size must be an integer >= 2")
    return int(value)


def _class_info(truth, minimum):
    classes = sorted(set(x for x in truth if x >= 0))
    index = {label: i for i, label in enumerate(classes)}
    sizes = [0] * len(classes)
    for label in truth:
        if label >= 0:
            sizes[index[label]] += 1
    return classes, index, sizes


def _eligibility(classes, sizes, minimum):
    return dict(min_cluster_size=minimum,
                eligible_classes=None if minimum is None else sum(s >= minimum for s in sizes),
                classes_below_min_cluster_size=None if minimum is None else sum(s < minimum for s in sizes),
                truth_class_sizes=[dict(truth_label=c, size=s,
                    eligible=None if minimum is None else s >= minimum) for c, s in zip(classes, sizes)],
                eligibility_scope="size-only necessary condition for exact recovery, not a recoverability guarantee")


def evaluate_labels(truth, labels, *, min_cluster_size=None):
    """Extra label scores; predicted negatives are abstentions, never a cluster.

    Hungarian assignment maximizes the integer number of correctly matched
    points, not macro F1. Rows/classes and columns/predicted labels are sorted.
    Tied optimal assignments use SciPy's deterministic result for these arrays;
    the chosen classwise precision/recall is not claimed canonical under ties.
    Unmatched true classes score zero; unmatched predicted clusters still count
    in global precision and count error. True noise is excluded as a class but
    remains contamination in the sizes of predicted clusters.
    """
    truth, labels = _labels(truth, "truth"), _labels(labels, "labels")
    if len(truth) != len(labels):
        raise ValueError("truth/labels length mismatch")
    minimum = _minimum(min_cluster_size)
    classes, class_index, sizes = _class_info(truth, minimum)
    predicted = sorted(set(x for x in labels if x >= 0))
    predicted_index = {label: i for i, label in enumerate(predicted)}
    predicted_sizes = [0] * len(predicted)
    overlap = np.zeros((len(classes), len(predicted)), dtype=np.int64)
    for actual, assigned in zip(truth, labels):
        if assigned >= 0:
            j = predicted_index[assigned]
            predicted_sizes[j] += 1
            if actual >= 0:
                overlap[class_index[actual], j] += 1
    matches = {}
    if classes and predicted:
        rows, columns = linear_sum_assignment(overlap, maximize=True)
        matches = {int(i): int(j) for i, j in zip(rows, columns) if overlap[i, j] > 0}
    per_class, total_correct = [], 0
    for i, (label, size) in enumerate(zip(classes, sizes)):
        j = matches.get(i)
        tp = int(overlap[i, j]) if j is not None else 0
        found_size = predicted_sizes[j] if j is not None else 0
        total_correct += tp
        exact = any(int(overlap[i, q]) == size == predicted_sizes[q] for q in range(len(predicted)))
        per_class.append(dict(truth_label=label, size=size,
            matched_label=predicted[j] if j is not None else None,
            matched_size=found_size, intersection=tp,
            precision=tp / found_size if found_size else 0.0,
            recall=tp / size, f1=2 * tp / (size + found_size), exact=exact,
            eligible=None if minimum is None else size >= minimum))
    actual_total, assigned_total = sum(sizes), sum(predicted_sizes)
    exact_count = sum(row["exact"] for row in per_class)
    result = dict(scope="labels_all_points_true_noise_excluded_as_class",
        truth_classes=len(classes), predicted_clusters=len(predicted),
        cluster_count_error=len(predicted) - len(classes),
        cluster_count_absolute_error=abs(len(predicted) - len(classes)),
        exact_classes=exact_count,
        exact_class_fraction=exact_count / len(classes) if classes else None,
        per_class=per_class, matched_correct_points=total_correct,
        matched_macro_precision=sum(row["precision"] for row in per_class) / len(classes) if classes else None,
        matched_macro_recall=sum(row["recall"] for row in per_class) / len(classes) if classes else None,
        matched_macro_f1=sum(row["f1"] for row in per_class) / len(classes) if classes else None,
        matched_micro_precision=total_correct / assigned_total if assigned_total else 0.0,
        matched_micro_recall=total_correct / actual_total if actual_total else None,
        matched_micro_f1=2 * total_correct / (actual_total + assigned_total) if actual_total else None,
        matching_objective="maximum total intersection; one-to-one Hungarian; negative labels excluded",
        matching_ties="sorted labels, SciPy deterministic choice; per-class assignment not uniquely certified")
    result.update(_eligibility(classes, sizes, minimum))
    return result


def _recoverability(truth, candidates, *, scope, minimum):
    classes, _, sizes = _class_info(truth, minimum)
    # Each candidate supplies (node ID, total leaf mass including true noise,
    # vector of true-class intersections). Scores/ties are compared rationally.
    best = [None] * len(classes)
    candidate_count = 0
    for node, mass, count in candidates:
        if minimum is not None and mass < minimum:
            continue
        candidate_count += 1
        for i, size in enumerate(sizes):
            fraction = Fraction(2 * int(count[i]), size + mass)
            record = (fraction, int(node), mass, int(count[i]))
            if best[i] is None or fraction > best[i][0]:
                best[i] = (*record, 1)
            elif fraction == best[i][0]:
                old = best[i]
                chosen = record if node < old[1] else old[:4]
                best[i] = (*chosen, old[4] + 1)
    rows = []
    for i, (label, size) in enumerate(zip(classes, sizes)):
        fraction, node, mass, overlap, ties = best[i] if best[i] is not None else (Fraction(0), None, 0, 0, 0)
        rows.append(dict(truth_label=label, size=size, best_f1=float(fraction),
            best_f1_numerator=fraction.numerator, best_f1_denominator=fraction.denominator,
            matched_node=node, matched_size=mass, intersection=overlap,
            precision=overlap / mass if mass else 0.0, recall=overlap / size,
            exact=overlap == size == mass, tied_best_nodes=ties,
            eligible=None if minimum is None else size >= minimum))
    fractions = [record[0] if record is not None else Fraction(0) for record in best]
    result = dict(scope=scope, oracle_supervised_diagnostic=True,
        candidate_clusters=candidate_count, per_class=rows,
        macro_best_f1=float(sum(fractions) / len(classes)) if classes else None,
        size_weighted_best_f1=float(sum(f * s for f, s in zip(fractions, sizes)) / sum(sizes)) if sizes else None,
        classes_f1_ge_0_8=sum(f >= Fraction(4, 5) for f in fractions),
        classes_f1_ge_0_9=sum(f >= Fraction(9, 10) for f in fractions),
        exact_classes=sum(row["exact"] for row in rows),
        root_excluded=True, true_noise_in_candidate_mass=True,
        candidate_tie_policy="smallest node ID; exact rational F1 comparison; all ties counted",
        selection_caution="per-class oracle branches can overlap or coincide; not a common cut, EOM selection, or parameter choice")
    result.update(_eligibility(classes, sizes, minimum))
    return result


def tree_recoverability(truth, tree, *, min_cluster_size=None):
    """Raw atomic internal branches, root excluded, optionally size-filtered.

    Equal-radius internal plateaux are removed before this diagnostic, matching
    common EOM. Leaf singletons are not cluster candidates. Heights/dates are
    neither redefined nor inferred from centroids; raw purity stays separate.
    """
    truth = _labels(truth, "truth")
    minimum = _minimum(min_cluster_size)
    eom = _legacy()
    n, raw, height, root, _, _ = eom._validated_tree(tree["n"], tree["children"], tree["heights"])
    if len(truth) != n:
        raise ValueError("tree/truth length mismatch")
    if "root" in tree and tree["root"] != root:
        raise ValueError("declared tree root mismatch")
    atomic, _ = eom._atomize(raw, height, root)
    classes, index, _ = _class_info(truth, minimum)
    order, stack = [], [root]
    while stack:
        node = stack.pop()
        order.append(node)
        stack.extend(atomic.get(node, []))
    counts, masses, candidates = {}, {}, []
    for node in reversed(order):
        if node < n:
            counts[node] = np.zeros(len(classes), dtype=np.int64)
            if truth[node] >= 0:
                counts[node][index[truth[node]]] = 1
            masses[node] = 1
        else:
            counts[node] = sum((counts[child] for child in atomic[node]), start=np.zeros(len(classes), dtype=np.int64))
            masses[node] = sum(masses[child] for child in atomic[node])
            if node != root:
                candidates.append((node, masses[node], counts[node]))
    return _recoverability(truth, candidates, scope="raw_atomic_internal_branch_sets", minimum=minimum)


def condensed_recoverability(truth, condensed_tree):
    """Non-root condensed *birth sets*, including all descendant point exits.

    These are historical branch memberships, not live cuts: exits are retained
    even when a point later becomes inactive, and a zero-duration branch can be
    present. No point is discarded to improve quality. This intentionally does
    NOT call an LCA purity routine on artificial reattachments, and does not
    claim that compression preserved the original cophenetic distances.
    """
    truth = _labels(truth, "truth")
    validate_condensed_tree(condensed_tree)
    n = condensed_tree["n_points"]
    if len(truth) != n:
        raise ValueError("condensed tree/truth length mismatch")
    minimum = condensed_tree["min_cluster_size"]
    classes, index, _ = _class_info(truth, minimum)
    parents = condensed_tree["parent"]
    counts = np.zeros((len(parents), len(classes)), dtype=np.int64)
    masses = [0] * len(parents)
    for point, cluster in enumerate(condensed_tree["point_exit_parent"]):
        masses[cluster] += 1
        if truth[point] >= 0:
            counts[cluster, index[truth[point]]] += 1
    candidates = []
    for cluster in range(len(parents) - 1, -1, -1):
        if masses[cluster] != condensed_tree["mass_at_birth"][cluster]:
            raise ValueError("reconstructed descendant mass differs from cluster birth mass")
        if cluster:
            candidates.append((cluster, masses[cluster], counts[cluster].copy()))
            masses[parents[cluster]] += masses[cluster]
            counts[parents[cluster]] += counts[cluster]
    return _recoverability(truth, candidates, scope="condensed_nonroot_branch_birth_sets_not_live_cuts", minimum=minimum)
