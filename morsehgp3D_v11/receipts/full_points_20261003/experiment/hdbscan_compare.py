"""Official sklearn HDBSCAN hierarchy, with equal-level merges observed atomically.

No fit runs at import time.  This is a diagnostic of hierarchy contents, not
automatic cluster selection.  The G4 campaign pins scikit-learn 1.7.2.
"""
from __future__ import annotations

import hashlib
import math
import time


class HdbscanRefusal(ValueError):
    """The requested official hierarchy could not be attested."""


def _require(condition, reason):
    if not condition:
        raise HdbscanRefusal(reason)


def _linkage_rows(tree, n):
    """Validate sklearn's structured linkage and return plain typed rows."""
    import numpy as np

    _require(isinstance(tree, np.ndarray), "sklearn_linkage_not_ndarray")
    _require(tree.ndim == 1 and tree.shape == (n - 1,), "sklearn_linkage_shape")
    expected = ("left_node", "right_node", "value", "cluster_size")
    _require(tree.dtype.names == expected, "sklearn_linkage_fields")
    for field in ("left_node", "right_node", "cluster_size"):
        _require(tree.dtype[field].kind in "iu", "sklearn_linkage_integer_field:" + field)
    _require(tree.dtype["value"].kind == "f", "sklearn_linkage_value_field")
    sizes = [1] * n + [0] * (n - 1)
    used = [False] * (2 * n - 1)
    rows = []
    previous = 0.0
    for ordinal, row in enumerate(tree):
        a, b = int(row["left_node"]), int(row["right_node"])
        height, size = float(row["value"]), int(row["cluster_size"])
        node = n + ordinal
        _require(0 <= a < node and 0 <= b < node and a != b,
                 "sklearn_linkage_child_index")
        _require(not used[a] and not used[b], "sklearn_linkage_reused_child")
        _require(math.isfinite(height) and height >= previous,
                 "sklearn_linkage_nonfinite_or_unsorted_height")
        _require(size == sizes[a] + sizes[b], "sklearn_linkage_size")
        used[a] = used[b] = True
        sizes[node] = size
        rows.append((a, b, height, size))
        previous = height
    _require(sizes[-1] == n and all(used[:-1]) and not used[-1],
             "sklearn_linkage_not_one_tree")
    return rows


def score_linkage(tree, labels):
    """Score only components after an entire level, using the shared judge.

    Singleton leaves are present at level zero, the conventional uncondensed
    single-linkage tree.  A level-zero merge is observed before any singleton
    scores, so zero-duration binary refinements are never candidates.
    """
    import numpy as np
    from qualified import HierarchyScorer

    truth = np.asarray(labels)
    _require(truth.ndim == 1 and truth.dtype.kind in "iu"
             and bool((truth >= -2).all()), "hdbscan_labels_shape_dtype_domain")
    n = len(truth)
    _require(n >= 2, "sklearn_linkage_needs_two_sites")
    rows = _linkage_rows(tree, n)
    scorer = HierarchyScorer([int(label) for label in truth])
    for site in range(n):
        scorer.activate(site, 0.0)
    if rows[0][2] > 0.0:
        scorer.observe(0.0)
    representatives = list(range(n)) + [-1] * (n - 1)
    index = 0
    while index < len(rows):
        height = rows[index][2]
        end = index + 1
        while end < len(rows) and rows[end][2] == height:
            end += 1
        for ordinal in range(index, end):
            a, b, _height, _size = rows[ordinal]
            left, right = representatives[a], representatives[b]
            _require(left >= 0 and right >= 0, "sklearn_linkage_missing_representative")
            scorer.union(left, right)
            representatives[n + ordinal] = left
        scorer.observe(height)
        index = end
    return scorer.finish()


def analyze_hdbscan(points, labels, k):
    """Fit the official pinned estimator on the exact same integer sites.

    Raises HdbscanRefusal with a named reason instead of silently replacing an
    absent private API or a different sklearn version with another producer.
    """
    import numpy as np
    try:
        import sklearn
        from sklearn.cluster import HDBSCAN
    except ImportError as error:
        raise HdbscanRefusal("sklearn_official_hdbscan_unavailable") from error

    _require(sklearn.__version__ == "1.7.2", "sklearn_version_not_1.7.2:" + sklearn.__version__)
    raw = np.asarray(points)
    _require(raw.ndim == 2 and raw.shape[1] == 3 and raw.shape[0] >= 2,
             "hdbscan_points_shape")
    _require(raw.dtype.kind in "iuf" and raw.dtype.kind != "b", "hdbscan_points_dtype")
    _require(bool(np.isfinite(raw).all()), "hdbscan_points_nonfinite")
    _require(bool((raw >= 0).all()) and bool((raw < 2 ** 24).all())
             and bool((raw == np.floor(raw)).all()), "hdbscan_points_not_quantized_u24")
    n = len(raw)
    _require(type(k) is int and 1 <= k <= n, "hdbscan_k_domain")
    truth = np.asarray(labels)
    _require(truth.shape == (n,) and truth.dtype.kind in "iu", "hdbscan_labels_shape_dtype")
    _require(bool((truth >= -2).all()), "hdbscan_labels_domain")
    xyz = np.array(raw, dtype=np.float64, order="C", copy=True)
    _require(bool((xyz == raw).all()), "hdbscan_integer_conversion_not_exact")
    parameters = dict(min_samples=k, min_cluster_size=2, metric="euclidean",
                      algorithm="kd_tree", alpha=1.0, leaf_size=40, n_jobs=1,
                      cluster_selection_method="eom", allow_single_cluster=False,
                      cluster_selection_epsilon=0.0, copy=True)
    model = HDBSCAN(**parameters)
    started = time.monotonic()
    try:
        model.fit(xyz)
    except Exception as error:
        raise HdbscanRefusal("sklearn_fit_failed:" + type(error).__name__ + ":" + str(error)[:300]) from error
    fit_seconds = time.monotonic() - started
    _require(hasattr(model, "_single_linkage_tree_"), "sklearn_single_linkage_api_absent")
    tree = model._single_linkage_tree_
    started = time.monotonic()
    result = score_linkage(tree, truth)
    analysis_seconds = time.monotonic() - started
    result.update({
        "status": "ok", "method": "sklearn.cluster.HDBSCAN",
        "sklearn_version": sklearn.__version__, "parameters": parameters,
        "n": n, "k": k, "coordinate_unit": "input_integer_grid_unit",
        "height_units": "mutual_reachability_distance_grid_units",
        "float64_integer_conversion": "exact; no rescaling or subsampling",
        "selection_scope": "complete hierarchy diagnostic; fitted labels unused",
        "singleton_policy": "all sites at level0; level0 merges atomic",
        "plateau_policy": "exact equality of returned float64 levels; no tolerance",
        "single_linkage_api": "_single_linkage_tree_",
        "single_linkage_shape": list(tree.shape),
        "single_linkage_fields": list(tree.dtype.names),
        "single_linkage_sha256": hashlib.sha256(tree.tobytes()).hexdigest(),
        "input_xyz_u32le_sha256": hashlib.sha256(np.asarray(raw, dtype="<u4").tobytes()).hexdigest(),
        "fit_seconds": fit_seconds, "analysis_seconds": analysis_seconds,
    })
    return result


def selftest():
    """Tiny linkage fixtures only; never calls sklearn.fit or native code."""
    import numpy as np

    dtype = [("left_node", "<i8"), ("right_node", "<i8"),
             ("value", "<f8"), ("cluster_size", "<i8")]
    tree = np.array([(0, 1, 1.0, 2), (3, 2, 1.0, 3)], dtype=dtype)
    result = score_linkage(tree, [0, 0, 1])
    _require(result["best_iou"]["0"]["intersection"] == 2
             and result["best_iou"]["0"]["valid_size"] == 3,
             "selftest_binary_subgroup_counted")
    checks = 1
    array_result = score_linkage(tree, np.asarray([0, 0, 1], dtype="<i4"))
    _require(array_result == result, "selftest_numpy_labels_differ")
    checks += 1
    zero = tree.copy()
    zero["value"] = 0.0
    result = score_linkage(zero, [0, 0, 1])
    _require(result["best_iou"]["1"]["valid_size"] == 3,
             "selftest_zero_duration_singleton_counted")
    checks += 1
    corruptions = []
    for value in (-1.0, float("nan"), float("inf"), 0.5):
        mutant = tree.copy()
        mutant[1]["value"] = value
        corruptions.append(mutant)
    mutant = tree.copy()
    mutant[1]["left_node"] = 0
    corruptions.append(mutant)
    mutant = tree.copy()
    mutant[1]["cluster_size"] = 2
    corruptions.append(mutant)
    mutant = tree.copy()
    mutant[0]["right_node"] = 3
    corruptions.append(mutant)
    corruptions.extend([tree[:1], np.asarray([[0, 1, 1, 2], [3, 2, 1, 3]])])
    for mutant in corruptions:
        try:
            _linkage_rows(mutant, 3)
        except HdbscanRefusal:
            checks += 1
        else:
            raise HdbscanRefusal("selftest_corrupt_linkage_accepted")
    return dict(status="ok", checks=checks,
                scope="toy linkage/scorer only; no fit, native, or GCP")


if __name__ == "__main__":
    import json
    import sys

    if sys.argv[1:] != ["--selftest"]:
        print("usage: hdbscan_compare.py --selftest", file=sys.stderr)
        raise SystemExit(2)
    print(json.dumps(selftest(), allow_nan=False, sort_keys=True))
