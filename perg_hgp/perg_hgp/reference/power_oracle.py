"""Exhaustive CPU oracle for the small power-HGP filtration.

This module is deliberately independent of the Gabriel predicate and of the
candidate-generation heuristics used by the production pipeline.  It is meant
for tiny validation problems only: every :math:`K`-facet and every
:math:`(K+1)`-coface is enumerated.

The additive power convention used throughout is

``power_i(x) = ||x - z_i||**2 + a_i``, with ``a_i >= 0``.

Consequently, the birth of a subset is the radius of its smallest additive
power ball, namely ``sqrt(min_x max_i power_i(x))``.  The minimax problem has
the simplex dual

``max_lambda sum_i lambda_i (||z_i||**2 + a_i)
            - ||sum_i lambda_i z_i||**2``.

In three dimensions an optimal dual law has a support of at most four sites.
``solve_additive_miniball_3d`` enumerates all such supports and checks the
primal/dual gap, including for duplicated or affinely dependent sites.
"""

from __future__ import annotations

from dataclasses import dataclass
from itertools import combinations
from math import comb
from typing import Iterable, Sequence

import numpy as np

from perg_hgp.backends.power_cover_3d_cuda.spatial_core import (
    brute_force_power_topk as _spatial_core_power_topk,
)


__all__ = [
    "AdditiveMiniball",
    "ComponentHierarchyLevel",
    "ExhaustivePowerComplex",
    "ThresholdComponents",
    "blocked_power_topk",
    "stable_power_topk",
    "solve_additive_miniball_3d",
    "enumerate_power_complex",
    "components_at_threshold",
    "component_partition_at_threshold",
    "build_component_hierarchy",
]


def _sites_and_weights(
    sites: np.ndarray | Sequence[Sequence[float]],
    additive_weights: np.ndarray | Sequence[float],
) -> tuple[np.ndarray, np.ndarray]:
    centers = np.asarray(sites, dtype=np.float64)
    weights = np.asarray(additive_weights, dtype=np.float64)
    if centers.ndim != 2 or centers.shape[1] != 3 or centers.shape[0] < 1:
        raise ValueError("sites must have shape (N, 3), N >= 1")
    if weights.shape != (centers.shape[0],):
        raise ValueError("additive_weights must have shape (N,)")
    if not np.isfinite(centers).all() or not np.isfinite(weights).all():
        raise ValueError("sites and additive_weights must be finite")

    # Variances produced by entropic regularisation are non-negative.  Permit
    # only round-off-sized negative values, as the CUDA contract does.
    weight_scale = max(1.0, float(np.max(np.abs(weights))))
    negative_tolerance = 64.0 * np.finfo(np.float64).eps * weight_scale
    if float(np.min(weights)) < -negative_tolerance:
        raise ValueError("additive power weights must be non-negative")
    return np.ascontiguousarray(centers), np.maximum(weights, 0.0)


def blocked_power_topk(
    queries: np.ndarray | Sequence[Sequence[float]],
    sites: np.ndarray | Sequence[Sequence[float]],
    additive_weights: np.ndarray | Sequence[float],
    K: int,
    *,
    query_block: int = 1_024,
    site_block: int = 4_096,
) -> tuple[np.ndarray, np.ndarray]:
    """Return the exact additive-power top-K, stably ordered by ``(value, id)``.

    The storage-bounded implementation is shared with ``spatial_core``.  This
    wrapper validates its reference-oracle contract explicitly: finite inputs,
    positive block sizes, recomputed output powers, and lexicographic ordering
    of every returned row.  Thus changing block boundaries cannot change a
    tie decision.
    """

    q = np.asarray(queries, dtype=np.float64)
    z, a = _sites_and_weights(sites, additive_weights)
    if q.ndim != 2 or q.shape[1] != 3:
        raise ValueError("queries must have shape (Q, 3)")
    if not np.isfinite(q).all():
        raise ValueError("queries must be finite")
    order = int(K)
    if order != K or not 1 <= order <= z.shape[0]:
        raise ValueError("K must be an integer in [1, N]")
    if int(query_block) != query_block or int(query_block) < 1:
        raise ValueError("query_block must be a positive integer")
    if int(site_block) != site_block or int(site_block) < 1:
        raise ValueError("site_block must be a positive integer")

    values, indices = _spatial_core_power_topk(
        q,
        z,
        a,
        order,
        query_block=int(query_block),
        site_block=int(site_block),
    )
    expected = np.sum((q[:, None, :] - z[indices]) ** 2, axis=2) + a[indices]
    if not np.array_equal(values, expected):
        # Equality is expected because both sides use float64 with the same
        # expression.  Keep an ulp-aware guard for alternate NumPy kernels.
        if not np.allclose(values, expected, rtol=8e-15, atol=0.0):
            raise RuntimeError("spatial_core returned inconsistent power values")
    if values.shape[1] > 1:
        left_value = values[:, :-1]
        right_value = values[:, 1:]
        left_id = indices[:, :-1]
        right_id = indices[:, 1:]
        ordered = (left_value < right_value) | (
            (left_value == right_value) & (left_id < right_id)
        )
        if not bool(np.all(ordered)):
            raise RuntimeError("power top-K is not stably ordered by (value, id)")
    return values, indices


# The short alias is useful in tests that compare several top-K backends.
stable_power_topk = blocked_power_topk


@dataclass(frozen=True)
class AdditiveMiniball:
    """Certified solution of a 3-D additive smallest-enclosing-ball problem."""

    center: np.ndarray
    power_radius: float
    radius: float
    support: tuple[int, ...]
    multipliers: np.ndarray
    primal_residual: float
    dual_residual: float
    active_residual: float
    stationarity_residual: float
    simplex_residual: float
    minimum_multiplier: float
    feasible: bool


@dataclass(frozen=True)
class _Normalisation:
    origin: np.ndarray
    scale: float
    weight_shift: float
    sites: np.ndarray
    weights: np.ndarray


def _normalise_miniball_problem(sites: np.ndarray, weights: np.ndarray) -> _Normalisation:
    # Subtracting a common additive weight and translating all sites preserve
    # both the centre and the active support.  Scaling keeps the exhaustive
    # KKT systems well-conditioned across unit changes.
    origin = sites[0] + np.mean(sites - sites[0], axis=0)
    relative = sites - origin
    weight_shift = float(np.min(weights))
    weight_range = float(np.max(weights) - weight_shift)
    coordinate_scale = float(np.max(np.abs(relative)))
    scale = max(coordinate_scale, float(np.sqrt(weight_range)))
    if not np.isfinite(scale) or scale == 0.0:
        scale = 1.0
    scale_squared = scale * scale
    return _Normalisation(
        origin=origin,
        scale=scale,
        weight_shift=weight_shift,
        sites=relative / scale,
        weights=(weights - weight_shift) / scale_squared,
    )


def solve_additive_miniball_3d(
    sites: np.ndarray | Sequence[Sequence[float]],
    additive_weights: np.ndarray | Sequence[float],
    *,
    tolerance: float = 1e-11,
) -> AdditiveMiniball:
    """Solve ``min_x max_i (||x-z_i||^2+a_i)`` by exhaustive supports.

    Every subset of one to four sites is considered as a possible support of
    the simplex dual.  A candidate is retained only when its multipliers are
    non-negative and its normalised primal/dual gap is below ``tolerance``.
    Returned calculations and residuals are float64.
    """

    z, a = _sites_and_weights(sites, additive_weights)
    if not np.isfinite(tolerance) or float(tolerance) <= 0.0:
        raise ValueError("tolerance must be finite and positive")
    tolerance = max(
        float(tolerance),
        4096.0 * np.finfo(np.float64).eps * max(1, z.shape[0]),
    )
    normalised = _normalise_miniball_problem(z, a)
    p = normalised.sites
    w = normalised.weights
    n_sites = p.shape[0]

    best: tuple[float, tuple[int, ...], np.ndarray, np.ndarray, float] | None = None
    smallest_rejected_gap = np.inf

    for support_size in range(1, min(4, n_sites) + 1):
        for support_tuple in combinations(range(n_sites), support_size):
            support = np.asarray(support_tuple, dtype=np.int64)
            active = p[support]
            active_weights = w[support]

            # Unknowns are the active dual multipliers.  The first equation is
            # sum(lambda)=1.  Remaining equations say that all positive-
            # multiplier sites have equal power at x=sum(lambda_i*z_i).
            system = np.empty((support_size, support_size), dtype=np.float64)
            rhs = np.empty(support_size, dtype=np.float64)
            system[0] = 1.0
            rhs[0] = 1.0
            if support_size > 1:
                differences = active[1:] - active[0]
                system[1:] = 2.0 * differences @ active.T
                lifted = np.sum(active * active, axis=1) + active_weights
                rhs[1:] = lifted[1:] - lifted[0]

            multipliers, _, _, _ = np.linalg.lstsq(
                system,
                rhs,
                rcond=64.0 * np.finfo(np.float64).eps,
            )
            equation_residual = float(np.max(np.abs(system @ multipliers - rhs)))
            if equation_residual > 32.0 * tolerance:
                continue
            if float(np.min(multipliers)) < -32.0 * tolerance:
                continue
            multipliers[np.abs(multipliers) <= 32.0 * tolerance] = 0.0
            multiplier_sum = float(np.sum(multipliers))
            if multiplier_sum <= 0.0:
                continue
            multipliers /= multiplier_sum

            center = multipliers @ active
            powers = np.sum((p - center) ** 2, axis=1) + w
            primal_value = float(np.max(powers))
            dual_value = float(
                multipliers @ (np.sum(active * active, axis=1) + active_weights)
                - center @ center
            )
            gap = max(0.0, primal_value - dual_value)
            smallest_rejected_gap = min(smallest_rejected_gap, gap)
            if gap > 128.0 * tolerance * max(1.0, abs(primal_value)):
                continue

            positive = multipliers > 0.0
            effective_support = tuple(int(i) for i in support[positive])
            effective_multipliers = multipliers[positive]
            if not effective_support:
                continue
            effective_multipliers /= np.sum(effective_multipliers)
            candidate_key = (primal_value, effective_support)
            if best is None:
                best = (
                    primal_value,
                    effective_support,
                    effective_multipliers.copy(),
                    center.copy(),
                    dual_value,
                )
            else:
                best_key = (best[0], best[1])
                # The certificate tolerance decides admissibility, but it must
                # not make a slightly larger enclosing ball win merely because
                # its support tuple is lexicographically smaller.
                objective_slack = (
                    128.0
                    * np.finfo(np.float64).eps
                    * max(1.0, abs(best[0]), abs(candidate_key[0]))
                )
                if candidate_key[0] < best_key[0] - objective_slack or (
                    abs(candidate_key[0] - best_key[0]) <= objective_slack
                    and candidate_key[1] < best_key[1]
                ):
                    best = (
                        primal_value,
                        effective_support,
                        effective_multipliers.copy(),
                        center.copy(),
                        dual_value,
                    )

    if best is None:
        raise RuntimeError(
            "no support of cardinality <= 4 passed the additive-miniball "
            f"certificate (smallest normalised gap={smallest_rejected_gap!r})"
        )

    _, support_tuple, multipliers, center_normalised, dual_normalised = best
    center = normalised.origin + normalised.scale * center_normalised
    physical_powers = np.sum((z - center) ** 2, axis=1) + a
    power_radius = float(np.max(physical_powers))
    support = np.asarray(support_tuple, dtype=np.int64)
    active_powers = physical_powers[support]
    scale_squared = normalised.scale * normalised.scale
    dual_value = normalised.weight_shift + scale_squared * dual_normalised

    # Residuals have explicit units: primal/dual/active are squared-distance
    # quantities, stationarity is a distance, and simplex is dimensionless.
    primal_residual = float(max(0.0, np.max(physical_powers - power_radius)))
    dual_residual = float(max(0.0, power_radius - dual_value))
    active_residual = float(np.max(np.abs(active_powers - power_radius)))
    # Form this affine combination around the normalisation origin.  Directly
    # multiplying coordinates translated by (say) 1e12 would manufacture a
    # translation-dependent stationarity residual through cancellation.
    stationary_center = normalised.origin + multipliers @ (
        z[support] - normalised.origin
    )
    stationarity_residual = float(np.linalg.norm(center - stationary_center))
    simplex_residual = float(abs(np.sum(multipliers) - 1.0))
    minimum_multiplier = float(np.min(multipliers))
    physical_tolerance = 256.0 * tolerance * max(
        1.0,
        abs(power_radius),
        scale_squared,
    )
    feasible = bool(
        np.isfinite(power_radius)
        and power_radius >= -physical_tolerance
        and primal_residual <= physical_tolerance
        and dual_residual <= physical_tolerance
        and active_residual <= physical_tolerance
        and stationarity_residual
        <= 256.0 * tolerance * max(1.0, normalised.scale)
        and simplex_residual <= 256.0 * tolerance
        and minimum_multiplier >= -256.0 * tolerance
        and len(support_tuple) <= 4
    )
    if not feasible:
        raise RuntimeError("additive-miniball candidate failed its float64 certificate")

    return AdditiveMiniball(
        center=np.asarray(center, dtype=np.float64),
        power_radius=max(power_radius, 0.0),
        radius=float(np.sqrt(max(power_radius, 0.0))),
        support=support_tuple,
        multipliers=np.asarray(multipliers, dtype=np.float64),
        primal_residual=primal_residual,
        dual_residual=dual_residual,
        active_residual=active_residual,
        stationarity_residual=stationarity_residual,
        simplex_residual=simplex_residual,
        minimum_multiplier=minimum_multiplier,
        feasible=feasible,
    )


@dataclass(frozen=True)
class ExhaustivePowerComplex:
    """Complete small-``n`` power complex, without a Gabriel filter."""

    K: int
    sites: np.ndarray
    additive_weights: np.ndarray
    facets: np.ndarray
    facet_power_births: np.ndarray
    facet_births: np.ndarray
    facet_miniballs: tuple[AdditiveMiniball, ...]
    cofaces: np.ndarray
    coface_power_births: np.ndarray
    coface_births: np.ndarray
    coface_miniballs: tuple[AdditiveMiniball, ...]
    edge_u: np.ndarray
    edge_v: np.ndarray
    edge_births: np.ndarray
    edge_coface_indices: np.ndarray

    @property
    def n_facets(self) -> int:
        return int(self.facets.shape[0])

    @property
    def n_cofaces(self) -> int:
        return int(self.cofaces.shape[0])

    @property
    def n_edges(self) -> int:
        return int(self.edge_u.shape[0])

    def facet_index(self, facet: Iterable[int]) -> int:
        key = tuple(sorted(int(index) for index in facet))
        if len(key) != self.K or len(set(key)) != self.K:
            raise ValueError(f"a facet must contain {self.K} distinct site ids")
        lookup = {tuple(row): index for index, row in enumerate(self.facets.tolist())}
        try:
            return int(lookup[key])
        except KeyError as error:
            raise KeyError(f"unknown facet {key}") from error


def _combination_array(n: int, size: int) -> np.ndarray:
    if size > n:
        return np.empty((0, size), dtype=np.int64)
    values = list(combinations(range(n), size))
    if not values:
        return np.empty((0, size), dtype=np.int64)
    return np.asarray(values, dtype=np.int64).reshape(-1, size)


def enumerate_power_complex(
    sites: np.ndarray | Sequence[Sequence[float]],
    additive_weights: np.ndarray | Sequence[float],
    K: int,
    *,
    tolerance: float = 1e-11,
) -> ExhaustivePowerComplex:
    """Enumerate all ``K``-facets, all ``K+1``-cofaces, and their births.

    Each coface contributes the complete elementary graph on its ``K+1``
    facets.  No empty-ball/Gabriel test is evaluated anywhere in this path.
    """

    z, a = _sites_and_weights(sites, additive_weights)
    order = int(K)
    if order != K or not 1 <= order <= z.shape[0]:
        raise ValueError("K must be an integer in [1, N]")

    facets = _combination_array(z.shape[0], order)
    cofaces = _combination_array(z.shape[0], order + 1)
    if facets.shape[0] != comb(z.shape[0], order):
        raise RuntimeError("internal facet enumeration error")

    facet_balls = tuple(
        solve_additive_miniball_3d(z[facet], a[facet], tolerance=tolerance)
        for facet in facets
    )
    coface_balls = tuple(
        solve_additive_miniball_3d(z[coface], a[coface], tolerance=tolerance)
        for coface in cofaces
    )
    facet_power_births = np.asarray(
        [ball.power_radius for ball in facet_balls], dtype=np.float64
    )
    facet_births = np.asarray([ball.radius for ball in facet_balls], dtype=np.float64)
    coface_power_births = np.asarray(
        [ball.power_radius for ball in coface_balls], dtype=np.float64
    )
    coface_births = np.asarray(
        [ball.radius for ball in coface_balls], dtype=np.float64
    )

    facet_lookup = {tuple(facet): index for index, facet in enumerate(facets.tolist())}
    edge_u: list[int] = []
    edge_v: list[int] = []
    edge_births: list[float] = []
    edge_cofaces: list[int] = []
    for coface_index, coface in enumerate(cofaces.tolist()):
        boundary = [
            facet_lookup[tuple(facet)] for facet in combinations(coface, order)
        ]
        for left, right in combinations(boundary, 2):
            edge_u.append(min(left, right))
            edge_v.append(max(left, right))
            edge_births.append(float(coface_births[coface_index]))
            edge_cofaces.append(coface_index)

    return ExhaustivePowerComplex(
        K=order,
        sites=z.copy(),
        additive_weights=a.copy(),
        facets=facets,
        facet_power_births=facet_power_births,
        facet_births=facet_births,
        facet_miniballs=facet_balls,
        cofaces=cofaces,
        coface_power_births=coface_power_births,
        coface_births=coface_births,
        coface_miniballs=coface_balls,
        edge_u=np.asarray(edge_u, dtype=np.int64),
        edge_v=np.asarray(edge_v, dtype=np.int64),
        edge_births=np.asarray(edge_births, dtype=np.float64),
        edge_coface_indices=np.asarray(edge_cofaces, dtype=np.int64),
    )


class _UnionFind:
    def __init__(self, size: int):
        self.parent = np.arange(size, dtype=np.int64)

    def find(self, item: int) -> int:
        root = int(item)
        while self.parent[root] != root:
            root = int(self.parent[root])
        while self.parent[item] != item:
            next_item = int(self.parent[item])
            self.parent[item] = root
            item = next_item
        return root

    def union(self, left: int, right: int) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        # Canonical roots make partitions deterministic under edge ordering.
        low, high = sorted((left_root, right_root))
        self.parent[high] = low


@dataclass(frozen=True)
class ThresholdComponents:
    """Facet partition after all events at or below one threshold."""

    threshold: float
    active_facet_indices: tuple[int, ...]
    labels: np.ndarray
    components: tuple[tuple[int, ...], ...]

    def facet_tuples(self, complex_: ExhaustivePowerComplex) -> tuple[tuple[tuple[int, ...], ...], ...]:
        """Return the same partition expressed as tuples of original site ids."""

        return tuple(
            tuple(tuple(int(value) for value in complex_.facets[index]) for index in part)
            for part in self.components
        )


def components_at_threshold(
    complex_: ExhaustivePowerComplex,
    threshold: float,
    *,
    atol: float = 0.0,
) -> ThresholdComponents:
    """Compute all components at ``threshold``, including isolated facets."""

    threshold = float(threshold)
    atol = float(atol)
    if np.isnan(threshold):
        raise ValueError("threshold must not be NaN")
    if not np.isfinite(atol) or atol < 0.0:
        raise ValueError("atol must be finite and non-negative")

    active = complex_.facet_births <= threshold + atol
    union_find = _UnionFind(complex_.n_facets)
    active_edges = complex_.edge_births <= threshold + atol
    for left, right in zip(
        complex_.edge_u[active_edges], complex_.edge_v[active_edges]
    ):
        if active[int(left)] and active[int(right)]:
            union_find.union(int(left), int(right))

    groups: dict[int, list[int]] = {}
    for facet_index in np.flatnonzero(active):
        root = union_find.find(int(facet_index))
        groups.setdefault(root, []).append(int(facet_index))
    components = tuple(
        tuple(group) for group in sorted(groups.values(), key=lambda values: values[0])
    )
    labels = np.full(complex_.n_facets, -1, dtype=np.int64)
    for component_index, group in enumerate(components):
        labels[np.asarray(group, dtype=np.int64)] = component_index
    return ThresholdComponents(
        threshold=threshold,
        active_facet_indices=tuple(int(value) for value in np.flatnonzero(active)),
        labels=labels,
        components=components,
    )


def component_partition_at_threshold(
    complex_: ExhaustivePowerComplex,
    threshold: float,
    *,
    atol: float = 0.0,
) -> tuple[tuple[int, ...], ...]:
    """Convenience wrapper returning only facet-index components."""

    return components_at_threshold(complex_, threshold, atol=atol).components


@dataclass(frozen=True)
class ComponentHierarchyLevel:
    """State after a simultaneous group of equal-radius events."""

    radius: float
    born_facets: tuple[int, ...]
    born_cofaces: tuple[int, ...]
    active_facet_indices: tuple[int, ...]
    components: tuple[tuple[int, ...], ...]


def _event_groups(
    values: np.ndarray,
    *,
    rtol: float,
    atol: float,
) -> list[np.ndarray]:
    if values.size == 0:
        return []
    order = np.argsort(values, kind="stable")
    groups: list[list[int]] = [[int(order[0])]]
    anchor = float(values[order[0]])
    for index_value in order[1:]:
        index = int(index_value)
        value = float(values[index])
        if np.isclose(value, anchor, rtol=rtol, atol=atol):
            groups[-1].append(index)
        else:
            groups.append([index])
            anchor = value
    return [np.asarray(group, dtype=np.int64) for group in groups]


def build_component_hierarchy(
    complex_: ExhaustivePowerComplex,
    *,
    event_rtol: float = 1e-12,
    event_atol: float = 1e-12,
) -> tuple[ComponentHierarchyLevel, ...]:
    """Return the component filtration with equal events applied atomically.

    A level is emitted only after every facet birth and every coface edge in
    its tolerance group has been applied.  No transient binary ordering is
    therefore exposed for a multiway merge.  Facets born without an incident
    coface are retained as singleton components.
    """

    if not np.isfinite(event_rtol) or event_rtol < 0.0:
        raise ValueError("event_rtol must be finite and non-negative")
    if not np.isfinite(event_atol) or event_atol < 0.0:
        raise ValueError("event_atol must be finite and non-negative")

    event_values = np.concatenate((complex_.facet_births, complex_.coface_births))
    union_find = _UnionFind(complex_.n_facets)
    active = np.zeros(complex_.n_facets, dtype=bool)
    active_cofaces = np.zeros(complex_.n_cofaces, dtype=bool)
    levels: list[ComponentHierarchyLevel] = []
    for group in _event_groups(event_values, rtol=event_rtol, atol=event_atol):
        group_values = event_values[group]
        radius = float(np.max(group_values))
        facet_mask = group < complex_.n_facets
        born_facets = tuple(int(value) for value in group[facet_mask])
        born_cofaces = tuple(
            int(value - complex_.n_facets) for value in group[~facet_mask]
        )
        # Apply exactly this tolerance group.  Re-querying with ``radius+atol``
        # could otherwise consume the first event of the following group and
        # expose it before listing it in ``born_cofaces``.
        if born_facets:
            active[np.asarray(born_facets, dtype=np.int64)] = True
        if born_cofaces:
            active_cofaces[np.asarray(born_cofaces, dtype=np.int64)] = True

        # Revisit already-born cofaces when a boundary facet appears.  In exact
        # arithmetic a boundary birth never exceeds its coface birth; separate
        # float64 miniball solves can reverse them by one ulp.  This produces
        # the same filtration as ``components_at_threshold`` even when both
        # event tolerances are explicitly set to zero.
        for left, right, coface_index in zip(
            complex_.edge_u,
            complex_.edge_v,
            complex_.edge_coface_indices,
        ):
            left_index, right_index = int(left), int(right)
            if (
                active_cofaces[int(coface_index)]
                and active[left_index]
                and active[right_index]
            ):
                union_find.union(left_index, right_index)

        groups: dict[int, list[int]] = {}
        for facet_index in np.flatnonzero(active):
            root = union_find.find(int(facet_index))
            groups.setdefault(root, []).append(int(facet_index))
        components = tuple(
            tuple(component)
            for component in sorted(groups.values(), key=lambda values: values[0])
        )
        levels.append(
            ComponentHierarchyLevel(
                radius=radius,
                born_facets=born_facets,
                born_cofaces=born_cofaces,
                active_facet_indices=tuple(
                    int(value) for value in np.flatnonzero(active)
                ),
                components=components,
            )
        )
    return tuple(levels)
