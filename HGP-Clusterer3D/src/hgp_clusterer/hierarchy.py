"""Cluster selection in a condensed tree of faces."""

from __future__ import annotations

from typing import Callable, Optional, Sequence, Union

import numpy as np

Splitting = Callable[[np.ndarray, Sequence[np.ndarray]], bool]
Method = Union[str, float]


def _roots(children: Sequence[Sequence[int]]) -> list[int]:
    is_child = np.zeros(len(children), dtype=bool)
    for ch in children:
        is_child[list(ch)] = True
    return [int(j) for j in np.flatnonzero(~is_child)]


def _eom_select(tree: dict) -> list[int]:
    children = tree["children"]
    best = np.array(tree["stability"], dtype=np.float64)
    keep = np.ones(len(children), dtype=bool)
    for i, ch in enumerate(children):
        if ch:
            total = 0.0
            for c in ch:
                total += best[c]
            if total > best[i]:
                best[i] = total
                keep[i] = False

    selected = []
    stack = _roots(children)
    while stack:
        cid = stack.pop()
        if keep[cid] or not children[cid]:
            selected.append(cid)
        else:
            stack.extend(children[cid])
    return sorted(selected)


def _cut_select(tree: dict, level: float) -> list[int]:
    children = tree["children"]
    birth = np.asarray(tree["r"], dtype=np.float64)
    parent = np.full(len(children), -1, dtype=np.int64)
    for p, ch in enumerate(children):
        parent[list(ch)] = p
    return [
        i
        for i in range(len(children))
        if birth[i] <= level and (parent[i] < 0 or birth[parent[i]] > level)
    ]


def _subtree_ranges(tree: dict) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    children = tree["children"]
    membership = tree["initial_membership"]
    n_clusters = len(children)

    visit = _roots(children)
    postorder = []
    while visit:
        cid = visit.pop()
        postorder.append(cid)
        visit.extend(children[cid])
    postorder.reverse()

    rank = np.empty(n_clusters, dtype=np.int64)
    first = np.empty(n_clusters, dtype=np.int64)
    for position, cid in enumerate(postorder):
        rank[cid] = position
        ch = children[cid]
        first[cid] = min(first[c] for c in ch) if ch else position
    last = rank + 1

    node_rank = np.full(membership.shape[0], -1, dtype=np.int64)
    assigned = membership >= 0
    node_rank[assigned] = rank[membership[assigned]]
    order = np.argsort(node_rank, kind="stable")
    counts = np.bincount(node_rank[assigned], minlength=n_clusters)
    offsets = np.empty(n_clusters + 1, dtype=np.int64)
    offsets[0] = membership.shape[0] - int(assigned.sum())
    np.cumsum(counts, out=offsets[1:])
    offsets[1:] += offsets[0]
    return order, offsets[first], offsets[last]


def _face_states(
    tree: dict,
    faces: np.ndarray,
    face_weights: np.ndarray,
    valid: np.ndarray,
    roots: Sequence[int],
) -> dict:
    children = tree["children"]
    membership = tree["initial_membership"]
    width = faces.shape[1]

    labels = membership[valid]
    order = np.argsort(labels, kind="stable")
    labels = labels[order]
    grouped_faces = faces[valid][order]
    grouped_weights = face_weights[valid][order]
    groups, starts = np.unique(labels, return_index=True)
    ends = np.append(starts[1:], labels.shape[0])

    leaves = {}
    for cid, start, end in zip(groups, starts, ends):
        points = grouped_faces[start:end].ravel()
        weights = np.repeat(grouped_weights[start:end], width)
        unique, inverse = np.unique(points, return_inverse=True)
        leaves[int(cid)] = (unique.astype(np.int32), np.bincount(inverse, weights=weights))

    subtree = set()
    stack = list(roots)
    while stack:
        cid = stack.pop()
        subtree.add(cid)
        stack.extend(children[cid])

    empty = (np.empty(0, dtype=np.int32), np.empty(0, dtype=np.float64))
    states = {}
    for cid in sorted(subtree):
        ch = children[cid]
        if not ch:
            states[cid] = leaves.get(cid, empty)
            continue
        parts = [states[c] for c in ch if states[c][0].shape[0] > 0]
        if not parts:
            states[cid] = empty
            continue
        points = np.concatenate([p for p, _ in parts])
        weights = np.concatenate([w for _, w in parts])
        unique, inverse = np.unique(points, return_inverse=True)
        states[cid] = (unique, np.bincount(inverse, weights=weights))
    return states


def select_clusters(
    tree: dict,
    method: Method,
    faces: np.ndarray,
    face_weights: np.ndarray,
    splitting: Optional[Splitting] = None,
    level: Optional[float] = None,
) -> list[np.ndarray]:
    """Select clusters of faces in a condensed tree.

    Parameters
    ----------
    tree : dict
        Output of :func:`hgp_clusterer._hierarchy.condense_tree`.
    method : {"eom", "leaf", "cut"}
        Excess of mass, leaves of the tree, or horizontal cut at ``level``.
    faces : ndarray of shape (n_faces, K), int32
        Points of each node of the tree.
    face_weights : ndarray of shape (n_faces,)
        Weight of each face in the points it contains.
    splitting : callable, optional
        ``splitting(parent_points, children_points) -> bool`` decides whether a
        selected cluster is replaced by its children, recursively.
    level : float, optional
        Filtration level of the cut, required when ``method == "cut"``.

    Returns
    -------
    list of ndarray
        Face indices (local to the tree) of each selected cluster.
    """
    children = tree["children"]
    join = np.asarray(tree["join_r"], dtype=np.float64)
    order, start, end = _subtree_ranges(tree)

    if method == "eom":
        selected = _eom_select(tree)
    elif method == "leaf":
        selected = [j for j, ch in enumerate(children) if not ch]
    elif method == "cut":
        selected = _cut_select(tree, level)
    else:
        raise ValueError(f"unknown selection method {method!r}")

    def nodes_of(cid: int) -> np.ndarray:
        nodes = order[start[cid]:end[cid]]
        if method == "cut":
            nodes = nodes[join[nodes] <= level]
        return nodes

    if splitting is None:
        clusters = [nodes_of(cid) for cid in selected]
        return [c for c in clusters if c.shape[0] > 0]

    valid = tree["initial_membership"] >= 0
    if method == "cut":
        valid &= join <= level
    states = _face_states(tree, faces, face_weights, valid, selected)

    clusters = []
    for root in selected:
        stack = [root]
        while stack:
            cid = stack.pop()
            ch = children[cid]
            points, _ = states[cid]
            if not ch or points.shape[0] == 0:
                clusters.append(nodes_of(cid))
                continue
            best_child = np.zeros(points.shape[0], dtype=np.int64)
            best_weight = np.full(points.shape[0], -1.0)
            for i, c in enumerate(ch):
                child_points, child_weights = states[c]
                if child_points.shape[0] == 0:
                    continue
                idx = np.searchsorted(points, child_points)
                better = child_weights > best_weight[idx]
                best_child[idx[better]] = i
                best_weight[idx[better]] = child_weights[better]
            parts = [points[best_child == i] for i in range(len(ch))]
            if splitting(points, parts):
                stack.extend(reversed(ch))
            else:
                clusters.append(nodes_of(cid))
    return [c for c in clusters if c.shape[0] > 0]
