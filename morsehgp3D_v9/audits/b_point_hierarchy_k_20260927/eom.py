#!/usr/bin/env python3
"""Shared, floating-point condensation/EOM of point trees, not a HGP proof.

Leaves are 0..n-1, internal nodes >= n, and heights are merge radii.
The fair profile is atomic equal-height plateaux, unit point masses, no root
selection, epsilon=0. Native HGP exact level keys must be retained separately.
No geometry, mutual-reachability graph, MST, cloud calls, or writes here.
"""
from __future__ import annotations

from collections import deque
from collections.abc import Mapping
import hashlib
import importlib.metadata
import math
from pathlib import Path
import sys


SUPPORTED_SKLEARN = ("1.9.1",)


def _integer(value, name, lower=0):
    # Accept NumPy integers without accepting bool or integral-valued floats.
    import numbers
    if isinstance(value, bool) or not isinstance(value, numbers.Integral) or value < lower:
        raise ValueError(f"{name} must be an integer >= {lower}")
    return int(value)


def _validated_tree(n, children, heights):
    n = _integer(n, "n", 1)
    items = children.items() if isinstance(children, Mapping) else enumerate(children)
    tree = {}
    for raw_node, raw_children in items:
        node = _integer(raw_node, "node")
        row = [] if raw_children is None else [_integer(c, "child") for c in raw_children]
        if node < n:
            if row:
                raise ValueError("point leaf has children")
            continue
        if len(row) < 2 or len(set(row)) != len(row):
            raise ValueError("internal nodes need >=2 distinct children")
        tree[node] = row
    nodes = set(range(n)) | tree.keys()
    height = {}
    for node in nodes:
        if isinstance(heights, Mapping):
            value = heights.get(node, 0.0 if node < n else None)
        else:
            value = heights[node] if node < len(heights) else None
        if value is None or isinstance(value, bool):
            raise ValueError("missing or invalid node height")
        value = float(value)
        if not math.isfinite(value) or value < 0 or (node < n and value != 0):
            raise ValueError("finite nonnegative radii and zero-height leaves required")
        height[node] = value
    parent = {}
    for node, row in tree.items():
        for child in row:
            if child not in nodes or child in parent or child == node:
                raise ValueError("missing child, multiple parent, or self cycle")
            if height[child] > height[node]:
                raise ValueError("merge radii decrease toward root")
            parent[child] = node
    roots = nodes - parent.keys()
    if len(roots) != 1:
        raise ValueError("exactly one root required; no implicit infinite root")
    root = next(iter(roots))
    order, seen, stack = [], set(), [root]
    while stack:
        node = stack.pop()
        if node in seen:
            raise ValueError("cycle in tree")
        seen.add(node)
        order.append(node)
        stack.extend(tree.get(node, ()))
    if seen != nodes:
        raise ValueError("disconnected/cyclic nodes")
    sizes, minimum = {}, {}
    for node in reversed(order):
        if node < n:
            sizes[node], minimum[node] = 1, node
        else:
            sizes[node] = sum(sizes[c] for c in tree[node])
            minimum[node] = min(minimum[c] for c in tree[node])
            tree[node].sort(key=minimum.__getitem__)
    return n, tree, height, root, sizes, minimum


def _atomize(tree, height, root):
    """Drop only INTERNAL zero-duration subdivisions, never point leaves.

    One DFS; a long equal-height binary comb does not expand every subtree.
    Float equality is intentional; no epsilon merges distinct source levels.
    """
    if root not in tree:
        return {}, 0
    out, stack, dropped = {root: []}, [(c, root) for c in reversed(tree[root])], 0
    while stack:
        node, owner = stack.pop()
        if node in tree and height[node] == height[owner]:
            dropped += 1
            stack.extend((c, owner) for c in reversed(tree[node]))
        else:
            out[owner].append(node)
            if node in tree:
                out[node] = []
                stack.extend((c, node) for c in reversed(tree[node]))
    return out, dropped


def condense_eom(n, children, heights, *, min_cluster_size, exp_z=1,
                 allow_single_cluster=False, cluster_selection_epsilon=0.0,
                 atomize_ties=True):
    """Return labels, selected condensed IDs, stabilities and replay metadata.

    children: mapping node->children, or list indexed by node (empty leaves).
    heights: mapping or node-indexed list; absent leaf entries default to zero
    for a mapping. Internal IDs need not be contiguous or topologically sorted.
    Equal EOM scores select the parent (as sklearn); root selection is refused.
    Zero radius has lambda=+inf; an interval [inf,inf] has zero duration, NOT NaN.
    Positive radii whose lambda is unrepresentable are refused, not zero-capped.
    Returned infinities are semantic zero-radius scores; encode them explicitly
    if writing strict JSON. This module itself never serializes or writes.
    """
    min_cluster_size = _integer(min_cluster_size, "min_cluster_size", 2)
    exp_z = _integer(exp_z, "exp_z", 1)
    if exp_z not in (1, 2):
        raise ValueError("this comparison supports exp_z=1 or 2 only")
    if allow_single_cluster is not False or cluster_selection_epsilon != 0.0:
        raise ValueError("fair profile requires allow_single_cluster=False, epsilon=0")
    if type(atomize_ties) is not bool:
        raise ValueError("atomize_ties must be bool")
    n, original, height, root, sizes, minimum = _validated_tree(n, children, heights)
    tree, removed = _atomize(original, height, root) if atomize_ties else (original, 0)
    warnings = ["floating_point_postprocessing_not_geometric_or_statistical_certification"]
    if any(height[node] == 0 for node in original):
        warnings.append("zero_radius_lambda_infinity_symbolic_zero_duration")
    if not atomize_ties and any(c in original and height[c] == height[p]
                                for p, row in original.items() for c in row):
        warnings.append("binary_plateau_subdivisions_preserved_may_change_zero_persistence_clusters")

    def lam(radius):
        if radius == 0:
            return math.inf
        try:
            value = (1.0 / radius) ** exp_z
        except OverflowError as error:
            raise ValueError("lambda overflows for a positive radius") from error
        if not math.isfinite(value) or value <= 0:
            raise ValueError("lambda under/overflows for a positive radius")
        return value

    def leaves(node):
        stack = [node]
        while stack:
            child = stack.pop()
            if child < n:
                yield child
            else:
                stack.extend(reversed(tree[child]))

    # Condensed IDs occupy a separate namespace: root=n, children thereafter.
    birth, cluster_parent, origin, cluster_children = {n: 0.0}, {}, {n: root}, {n: []}
    point_parent, point_exit = [-1] * n, [0.0] * n
    edges, queue = [], deque([(root, n)])
    while queue:
        node, cluster = queue.popleft()
        if node < n:  # n=1, which can never meet min_cluster_size>=2.
            point_parent[node], point_exit[node] = cluster, math.inf
            edges.append(dict(parent=cluster, child=node, lambda_value=math.inf, size=1))
            continue
        split = lam(height[node])
        large = [c for c in tree[node] if sizes[c] >= min_cluster_size]
        for child in tree[node]:
            if sizes[child] < min_cluster_size:
                for point in leaves(child):
                    point_parent[point], point_exit[point] = cluster, split
                    edges.append(dict(parent=cluster, child=point, lambda_value=split, size=1))
            elif len(large) == 1:
                queue.append((child, cluster))
            else:
                new = n + len(birth)
                birth[new], cluster_parent[new], origin[new] = split, cluster, child
                cluster_children[new] = []
                cluster_children[cluster].append(new)
                edges.append(dict(parent=cluster, child=new, lambda_value=split, size=sizes[child]))
                queue.append((child, new))

    contributions = {cluster: [] for cluster in birth}
    for edge in edges:
        end, begin = edge["lambda_value"], birth[edge["parent"]]
        duration = 0.0 if end == begin else end - begin
        if math.isnan(duration) or duration < 0:
            raise ValueError("invalid condensed lifetime")
        term = duration * edge["size"]
        if math.isfinite(duration) and not math.isfinite(term):
            raise ValueError("finite stability contribution overflow")
        contributions[edge["parent"]].append(term)

    def score_sum(terms):
        try:
            return math.fsum(terms)
        except OverflowError as error:
            raise ValueError("finite stability sum overflow") from error

    stability = {cluster: score_sum(terms) for cluster, terms in contributions.items()}
    best, chosen = {}, {}
    comparisons = []
    for cluster in reversed(list(birth)):
        descendants = score_sum(best[c] for c in cluster_children[cluster])
        own = stability[cluster]
        if math.isinf(descendants) and math.isinf(own):
            warnings.append(f"infinite_eom_tie_parent_preferred:{cluster}")
        elif cluster_children[cluster] and math.isfinite(descendants) and math.isfinite(own):
            scale = max(abs(descendants), abs(own))
            if scale and abs(descendants - own) <= 64 * sys.float_info.epsilon * scale:
                warnings.append(f"near_or_equal_float_eom_scores:{cluster}")
        chosen[cluster] = cluster != n and not (descendants > own)
        best[cluster] = own if chosen[cluster] else descendants
        comparisons.append(dict(cluster=cluster, own=own, descendant_best=descendants,
                                keep_parent=chosen[cluster]))
    selected, stack = [], [n]
    while stack:
        cluster = stack.pop()
        if chosen[cluster]:
            selected.append(cluster)
        else:
            stack.extend(reversed(cluster_children[cluster]))
    # Canonical labels by minimum leaf of the selected source subtree.
    selected.sort(key=lambda c: minimum[origin[c]])
    selected_label = {cluster: label for label, cluster in enumerate(selected)}
    inherited = {n: -1}
    for cluster in list(birth)[1:]:
        inherited[cluster] = selected_label.get(cluster, inherited[cluster_parent[cluster]])
    labels = [inherited[parent] for parent in point_parent]
    return dict(labels=labels, selected=selected, stabilities=stability,
                selected_sources={c: origin[c] for c in selected}, births=birth,
                condensed_tree=edges, point_parent=point_parent, point_exit_lambda=point_exit,
                eom_comparisons=comparisons, warnings=warnings, exp_z=exp_z,
                min_cluster_size=min_cluster_size, allow_single_cluster=False,
                cluster_selection_epsilon=0.0, atomize_ties=atomize_ties,
                contracted_internal_ties=removed, source_root=root,
                point_masses="unit", certification="not_claimed")


def equivalent_labels(first, second):
    """Partition equality up to label names, including an identical noise mask."""
    if len(first) != len(second):
        return False
    forward, backward = {}, {}
    for a, b in zip(first, second):
        a, b = int(a), int(b)
        if (a < 0) != (b < 0):
            return False
        if a < 0:
            continue
        if forward.setdefault(a, b) != b or backward.setdefault(b, a) != a:
            return False
    return True


def sklearn_tree_to_points(linkage):
    """Adapt sklearn's fitted SLT; no distances/MST are reconstructed."""
    expected = ("left_node", "right_node", "value", "cluster_size")
    if tuple(linkage.dtype.names or ()) != expected or linkage.ndim != 1:
        raise ValueError("unsupported sklearn single-linkage schema")
    n = len(linkage) + 1
    children, heights, sizes = {}, {i: 0.0 for i in range(n)}, {i: 1 for i in range(n)}
    for i, row in enumerate(linkage):
        node = n + i
        left, right = int(row["left_node"]), int(row["right_node"])
        if left not in sizes or right not in sizes or left == right:
            raise ValueError("invalid linkage ordering")
        if int(row["cluster_size"]) != sizes[left] + sizes[right]:
            raise ValueError("invalid linkage cluster_size")
        children[node], heights[node] = [left, right], float(row["value"])
        sizes[node] = sizes[left] + sizes[right]
    _validated_tree(n, children, heights)
    return n, children, heights


def hdbscan_min_samples(k, backend="sklearn"):
    """K includes self. contrib requires K-1 and cannot represent K=1."""
    k = _integer(k, "K", 1)
    if backend == "sklearn":
        return k
    if backend == "contrib" and k >= 2:
        return k - 1
    raise ValueError("unsupported backend or contrib cannot express self-counted K=1")


def sklearn_provenance():
    import sklearn.cluster._hdbscan.hdbscan as implementation
    base = Path(implementation.__file__).parent
    files = [Path(implementation.__file__)]
    for stem in ("_tree", "_linkage", "_reachability"):
        files.extend(sorted(base.glob(stem + "*.so")))
    return dict(packages={name: importlib.metadata.version(name)
                          for name in ("scikit-learn", "numpy", "scipy")},
                sha256={str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in files},
                linkage_attribute="_single_linkage_tree_", backend="sklearn")


def fit_hdbscan(points, *, k, min_cluster_size, exp_z=1, atomize_ties=True,
                max_points=100_000):
    """Local bounded sklearn fit + common EOM + native z=1 labels, always.

    Private fitted-tree API is deliberately pinned to the inspected version.
    max_points is a refusal bound, never subsampling. All points must be finite.
    For a shared z=1/z=2 run, call this once then condense_eom on result['tree'].
    """
    import numpy as np
    from sklearn.cluster import HDBSCAN
    version = importlib.metadata.version("scikit-learn")
    if version not in SUPPORTED_SKLEARN:
        raise ValueError(f"unqualified private sklearn tree API: {version}")
    k = hdbscan_min_samples(k)
    min_cluster_size = _integer(min_cluster_size, "min_cluster_size", 2)
    max_points = _integer(max_points, "max_points", 2)
    if not 2 <= len(points) <= max_points:
        raise ValueError("point count outside declared adapter bound")
    values = np.asarray(points, dtype=np.float64)
    if values.ndim != 2 or not values.shape[1] or not np.isfinite(values).all() or k > len(values):
        raise ValueError("finite dense points and 1<=K<=n required")
    model = HDBSCAN(min_cluster_size=min_cluster_size, min_samples=k, metric="euclidean",
                    alpha=1.0, algorithm="kd_tree", n_jobs=1,
                    cluster_selection_method="eom", allow_single_cluster=False,
                    cluster_selection_epsilon=0.0, max_cluster_size=None, copy=True).fit(values)
    n, children, heights = sklearn_tree_to_points(model._single_linkage_tree_)
    native = model.labels_.astype(int).tolist()
    common = condense_eom(n, children, heights, min_cluster_size=min_cluster_size,
                          exp_z=exp_z, atomize_ties=atomize_ties)
    common_z1 = common if exp_z == 1 else condense_eom(
        n, children, heights, min_cluster_size=min_cluster_size, exp_z=1, atomize_ties=atomize_ties)
    compatible = condense_eom(n, children, heights, min_cluster_size=min_cluster_size,
                              exp_z=1, atomize_ties=False)
    return dict(common=common, standard_labels_z1=native,
                common_z1_labels=common_z1["labels"],
                common_z1_matches_standard=equivalent_labels(common_z1["labels"], native),
                preserved_tree_z1_matches_standard=equivalent_labels(compatible["labels"], native),
                tree=dict(n=n, children=children, heights=heights),
                provenance=sklearn_provenance(),
                parameters=dict(k_self_included=k, min_samples=k, min_cluster_size=min_cluster_size,
                                metric="euclidean", alpha=1.0, algorithm="kd_tree", n_jobs=1,
                                max_cluster_size=None, cluster_selection_epsilon=0.0,
                                allow_single_cluster=False, max_points=max_points),
                warnings=[] if equivalent_labels(common_z1["labels"], native) else [
                    "atomic_common_EOM_differs_from_native_HDBSCAN_labels_publish_both"])
