#!/usr/bin/env python3
"""Run deterministic exhaustive MorseHGP3D oracle campaigns.

The default and ``--ci`` configurations are intentionally small.  The phase-1
gate can use ``--seeds-per-dimension 10000`` without changing the comparison
logic.  Counterexample files are never written unless ``--failure-dir`` is
explicitly supplied.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import random
import sys
import tempfile
import time
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations, permutations
from pathlib import Path
from typing import Callable, Iterable, Sequence


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from reference.morsehgp3d_oracle.gamma import GammaFiltration, build_gamma_filtration
from reference.morsehgp3d_oracle.generators import generate_affine_cloud
from reference.morsehgp3d_oracle.hierarchy import (
    MergeForest,
    VerticalMapFamily,
    build_merge_forest,
    build_profile_vertical_map_family,
    build_vertical_map_family,
)
from reference.morsehgp3d_oracle.oracle import canonicalize_points


Point3 = tuple[int | Fraction, int | Fraction, int | Fraction]
PROFILES = ("full_pi0", "hgp_reduced")
CI_MAX_SEEDS_PER_DIMENSION = 3
CI_MAX_POINT_COUNT = 7
CI_MAX_K_MAX = 5
CI_MAX_PERMUTATIONS = 1
CI_MAX_N_K_CASES_PER_SEED = 6


@dataclass(frozen=True)
class CampaignConfig:
    dimensions: tuple[int, ...] = (1, 2, 3)
    seed_start: int = 0
    seeds_per_dimension: int = 1
    point_counts: tuple[int, ...] = (6,)
    k_max_values: tuple[int, ...] = (3,)
    coordinate_bound: int = 16
    permutations_per_seed: int = 1
    metamorphic_stride: int = 1
    ci: bool = False
    failure_dir: Path | None = None

    def __post_init__(self) -> None:
        dimensions = tuple(sorted(set(self.dimensions)))
        point_counts = tuple(sorted(set(self.point_counts)))
        k_max_values = tuple(sorted(set(self.k_max_values)))
        if not dimensions or any(dimension not in (1, 2, 3) for dimension in dimensions):
            raise ValueError("dimensions must be a non-empty subset of 1, 2, 3")
        if isinstance(self.seed_start, bool) or not isinstance(self.seed_start, int):
            raise TypeError("seed_start must be an integer")
        integer_fields = {
            "seeds_per_dimension": self.seeds_per_dimension,
            "coordinate_bound": self.coordinate_bound,
            "permutations_per_seed": self.permutations_per_seed,
            "metamorphic_stride": self.metamorphic_stride,
        }
        for name, value in integer_fields.items():
            if isinstance(value, bool) or not isinstance(value, int):
                raise TypeError(f"{name} must be an integer")
        if self.seeds_per_dimension < 1:
            raise ValueError("seeds_per_dimension must be positive")
        if not point_counts or any(
            isinstance(point_count, bool)
            or not isinstance(point_count, int)
            or not 1 <= point_count <= 14
            for point_count in point_counts
        ):
            raise ValueError("point_counts must be non-empty integers in 1..14")
        if min(point_counts) < max(dimensions) + 1:
            raise ValueError(
                "every point count must support every requested affine dimension"
            )
        if not k_max_values or any(
            isinstance(k_max, bool)
            or not isinstance(k_max, int)
            or not 1 <= k_max <= 10
            for k_max in k_max_values
        ):
            raise ValueError("k_max_values must be non-empty integers in 1..10")
        if self.coordinate_bound < 2:
            raise ValueError("coordinate_bound must be at least 2")
        if self.permutations_per_seed < 0:
            raise ValueError("permutations_per_seed must be non-negative")
        if self.metamorphic_stride < 0:
            raise ValueError("metamorphic_stride must be non-negative")
        if self.ci and (
            self.seeds_per_dimension > CI_MAX_SEEDS_PER_DIMENSION
            or max(point_counts) > CI_MAX_POINT_COUNT
            or max(k_max_values) > CI_MAX_K_MAX
            or self.permutations_per_seed > CI_MAX_PERMUTATIONS
            or (
                len(point_counts) * len(k_max_values)
                > CI_MAX_N_K_CASES_PER_SEED
            )
        ):
            raise ValueError("--ci limits were exceeded; run the larger campaign without --ci")
        object.__setattr__(self, "dimensions", dimensions)
        object.__setattr__(self, "point_counts", point_counts)
        object.__setattr__(self, "k_max_values", k_max_values)
        if self.failure_dir is not None:
            object.__setattr__(self, "failure_dir", Path(self.failure_dir))

    @property
    def seeds(self) -> range:
        return range(self.seed_start, self.seed_start + self.seeds_per_dimension)

    @property
    def matrix_coverage(self) -> dict[str, bool]:
        pairs = tuple(
            (point_count, k_max)
            for point_count in self.point_counts
            for k_max in self.k_max_values
        )
        return {
            "n_less_than_k_max": any(n < k_max for n, k_max in pairs),
            "n_equal_to_k_max": any(n == k_max for n, k_max in pairs),
            "n_at_least_k_max_plus_one": any(n >= k_max + 1 for n, k_max in pairs),
            "multiple_k_max_values": len(self.k_max_values) > 1,
        }

    def as_manifest(self) -> dict[str, object]:
        return {
            "dimensions": list(self.dimensions),
            "seed_start": self.seed_start,
            "seeds_per_dimension": self.seeds_per_dimension,
            "point_counts": list(self.point_counts),
            "k_max_values": list(self.k_max_values),
            "coordinate_bound": self.coordinate_bound,
            "permutations_per_seed": self.permutations_per_seed,
            "metamorphic_stride": self.metamorphic_stride,
            "profiles": list(PROFILES),
            "matrix_coverage": self.matrix_coverage,
            "ci": self.ci,
            "failure_dir": str(self.failure_dir) if self.failure_dir is not None else None,
        }


@dataclass(frozen=True)
class CampaignFailure:
    invariant: str
    message: str
    context: dict[str, object]

    def as_json(self) -> dict[str, object]:
        return {
            "invariant": self.invariant,
            "message": self.message,
            "context": _jsonable(self.context),
        }


class CampaignMismatch(AssertionError):
    def __init__(self, failure: CampaignFailure) -> None:
        super().__init__(failure.message)
        self.failure = failure


@dataclass(frozen=True)
class CloudSnapshot:
    points: tuple[tuple[Fraction, Fraction, Fraction], ...]
    filtrations: tuple[GammaFiltration, ...]
    full_forests: tuple[MergeForest, ...]
    reduced_forests: tuple[MergeForest, ...]
    vertical_maps: tuple[VerticalMapFamily, ...]
    full_profile_vertical_maps: tuple[VerticalMapFamily, ...]
    reduced_profile_vertical_maps: tuple[VerticalMapFamily, ...]


COUNTER_NAMES = (
    "clouds_generated",
    "clouds_audited",
    "campaign_cases_attempted",
    "campaign_cases_audited",
    "points_generated",
    "orders_checked",
    "profile_order_checks",
    "facets_enumerated",
    "cofaces_enumerated",
    "gabriel_hyperedges_enumerated",
    "full_forest_nodes",
    "reduced_forest_nodes",
    "full_equal_level_batches",
    "reduced_equal_level_batches",
    "full_gamma_cut_comparisons",
    "reduced_gabriel_cut_comparisons",
    "reduced_gamma_nontrivial_comparisons",
    "cut_monotonicity_transitions_checked",
    "vertical_families_checked",
    "gamma_vertical_families_checked",
    "full_profile_vertical_families_checked",
    "reduced_profile_vertical_families_checked",
    "naturality_squares_checked",
    "gamma_naturality_squares_checked",
    "full_profile_naturality_squares_checked",
    "reduced_profile_naturality_squares_checked",
    "permutations_checked",
    "signed_axis_permutations_checked",
    "dyadic_translations_checked",
    "homotheties_checked",
    "homothety_levels_checked",
    "kmax_prefix_comparisons",
    "point_minimization_evaluations",
    "coordinate_minimization_evaluations",
    "coordinate_reductions_accepted",
    "minimization_evaluations",
    "failures",
)


def _new_counters() -> dict[str, int]:
    return {name: 0 for name in COUNTER_NAMES}


def _increment(counters: dict[str, int] | None, name: str, amount: int = 1) -> None:
    if counters is not None:
        counters[name] += amount


def _build_snapshot(points: Sequence[Sequence[int | Fraction]], k_max: int) -> CloudSnapshot:
    canonical_points = canonicalize_points(points)
    exact_points = tuple(point.coordinates for point in canonical_points)
    k_eff = min(k_max, len(exact_points))
    filtrations = tuple(
        build_gamma_filtration(exact_points, order)
        for order in range(1, k_eff + 1)
    )
    full_forests = tuple(
        build_merge_forest(filtration, "full_pi0") for filtration in filtrations
    )
    reduced_forests = tuple(
        build_merge_forest(filtration, "hgp_reduced") for filtration in filtrations
    )
    vertical_maps = tuple(
        build_vertical_map_family(filtrations[index], filtrations[index - 1])
        for index in range(1, len(filtrations))
    )
    full_profile_vertical_maps = tuple(
        build_profile_vertical_map_family(
            filtrations[index],
            filtrations[index - 1],
            full_forests[index],
            full_forests[index - 1],
            "full_pi0",
        )
        for index in range(1, len(filtrations))
    )
    reduced_profile_vertical_maps = tuple(
        build_profile_vertical_map_family(
            filtrations[index],
            filtrations[index - 1],
            reduced_forests[index],
            reduced_forests[index - 1],
            "hgp_reduced",
        )
        for index in range(1, len(filtrations))
    )
    return CloudSnapshot(
        exact_points,
        filtrations,
        full_forests,
        reduced_forests,
        vertical_maps,
        full_profile_vertical_maps,
        reduced_profile_vertical_maps,
    )


def _component_signature(components: Iterable[object]) -> tuple[object, ...]:
    return tuple(
        sorted(
            (
                tuple(getattr(component, "facet_point_ids")),
                tuple(getattr(component, "covered_point_ids")),
            )
            for component in components
        )
    )


def _covered_signature(components: Iterable[object]) -> tuple[tuple[int, ...], ...]:
    return tuple(
        sorted(
            tuple(getattr(component, "covered_point_ids"))
            for component in components
        )
    )


def _exact_level(level: Fraction) -> dict[str, str]:
    return {"numerator": str(level.numerator), "denominator": str(level.denominator)}


def _require_equal(
    invariant: str,
    expected: object,
    actual: object,
    *,
    context: dict[str, object],
) -> None:
    if expected != actual:
        raise CampaignMismatch(
            CampaignFailure(
                invariant=invariant,
                message=f"{invariant}: exhaustive oracle comparison differs",
                context={
                    **context,
                    "expected": _jsonable(expected),
                    "actual": _jsonable(actual),
                },
            )
        )


def _check_component_monotonicity(
    earlier_components: Iterable[object],
    later_components: Iterable[object],
    *,
    invariant: str,
    context: dict[str, object],
) -> None:
    earlier = tuple(earlier_components)
    later = tuple(later_components)
    later_by_facet = {
        tuple(facet): component
        for component in later
        for facet in getattr(component, "facet_point_ids")
    }
    for component in earlier:
        facets = tuple(getattr(component, "facet_point_ids"))
        targets = {
            tuple(later_by_facet[tuple(facet)].facet_point_ids)
            for facet in facets
            if tuple(facet) in later_by_facet
        }
        if len(targets) != 1 or any(
            tuple(facet) not in later_by_facet for facet in facets
        ):
            raise CampaignMismatch(
                CampaignFailure(
                    invariant=invariant,
                    message="a component split or disappeared as the level increased",
                    context={
                        **context,
                        "earlier_component": _jsonable(
                            (
                                facets,
                                tuple(getattr(component, "covered_point_ids")),
                            )
                        ),
                    },
                )
            )


def _check_snapshot(snapshot: CloudSnapshot, counters: dict[str, int] | None) -> None:
    for index, filtration in enumerate(snapshot.filtrations):
        order = filtration.order
        full_forest = snapshot.full_forests[index]
        reduced_forest = snapshot.reduced_forests[index]
        _increment(counters, "orders_checked")
        _increment(counters, "profile_order_checks", 2)
        _increment(counters, "facets_enumerated", len(filtration.facets))
        _increment(counters, "cofaces_enumerated", len(filtration.cofaces))
        _increment(
            counters,
            "gabriel_hyperedges_enumerated",
            len(filtration.gabriel_hyperedges),
        )
        _increment(counters, "full_forest_nodes", len(full_forest.nodes))
        _increment(counters, "reduced_forest_nodes", len(reduced_forest.nodes))
        _increment(counters, "full_equal_level_batches", len(full_forest.batches))
        _increment(counters, "reduced_equal_level_batches", len(reduced_forest.batches))

        for level in filtration.critical_levels:
            for closed in (False, True):
                context = {
                    "order": order,
                    "squared_level": _exact_level(level),
                    "closed": closed,
                }
                gamma_cut = filtration.cut(level, closed=closed, graph_kind="gamma")
                full_cut = full_forest.cut(level, closed=closed)
                _require_equal(
                    "full_forest_vs_gamma",
                    _component_signature(gamma_cut.components),
                    _component_signature(full_cut.components),
                    context={**context, "profile": "full_pi0"},
                )
                _increment(counters, "full_gamma_cut_comparisons")

                gabriel_cut = filtration.cut(level, closed=closed, graph_kind="gabriel")
                reduced_cut = reduced_forest.cut(level, closed=closed)
                _require_equal(
                    "reduced_forest_vs_gabriel",
                    _component_signature(gabriel_cut.components),
                    _component_signature(reduced_cut.components),
                    context={**context, "profile": "hgp_reduced"},
                )
                _increment(counters, "reduced_gabriel_cut_comparisons")

                if order >= 2:
                    nontrivial_gamma = tuple(
                        component
                        for component in gamma_cut.components
                        if component.nontrivial
                    )
                    _require_equal(
                        "reduced_vs_gamma_nontrivial_coverage",
                        _covered_signature(nontrivial_gamma),
                        _covered_signature(reduced_cut.components),
                        context={**context, "profile": "hgp_reduced"},
                    )
                    _increment(counters, "reduced_gamma_nontrivial_comparisons")

        ordered_cuts = tuple(
            (level, closed)
            for level in filtration.critical_levels
            for closed in (False, True)
        )
        for transition_index in range(len(ordered_cuts) - 1):
            earlier_level, earlier_closed = ordered_cuts[transition_index]
            later_level, later_closed = ordered_cuts[transition_index + 1]
            transitions = (
                (
                    "gamma_cut_monotonicity",
                    filtration.cut(earlier_level, closed=earlier_closed).components,
                    filtration.cut(later_level, closed=later_closed).components,
                ),
                (
                    "gabriel_cut_monotonicity",
                    filtration.cut(
                        earlier_level,
                        closed=earlier_closed,
                        graph_kind="gabriel",
                    ).components,
                    filtration.cut(
                        later_level,
                        closed=later_closed,
                        graph_kind="gabriel",
                    ).components,
                ),
                (
                    "full_forest_cut_monotonicity",
                    full_forest.cut(
                        earlier_level, closed=earlier_closed
                    ).components,
                    full_forest.cut(later_level, closed=later_closed).components,
                ),
                (
                    "reduced_forest_cut_monotonicity",
                    reduced_forest.cut(
                        earlier_level, closed=earlier_closed
                    ).components,
                    reduced_forest.cut(later_level, closed=later_closed).components,
                ),
            )
            for invariant, earlier_components, later_components in transitions:
                _check_component_monotonicity(
                    earlier_components,
                    later_components,
                    invariant=invariant,
                    context={
                        "order": order,
                        "earlier_squared_level": _exact_level(earlier_level),
                        "earlier_closed": earlier_closed,
                        "later_squared_level": _exact_level(later_level),
                        "later_closed": later_closed,
                    },
                )
                _increment(counters, "cut_monotonicity_transitions_checked")

    vertical_groups = (
        (
            "gamma",
            snapshot.vertical_maps,
            "gamma_vertical_families_checked",
            "gamma_naturality_squares_checked",
        ),
        (
            "full_pi0",
            snapshot.full_profile_vertical_maps,
            "full_profile_vertical_families_checked",
            "full_profile_naturality_squares_checked",
        ),
        (
            "hgp_reduced",
            snapshot.reduced_profile_vertical_maps,
            "reduced_profile_vertical_families_checked",
            "reduced_profile_naturality_squares_checked",
        ),
    )
    for scope, vertical_maps, family_counter, square_counter in vertical_groups:
        for vertical_map in vertical_maps:
            _increment(counters, "vertical_families_checked")
            _increment(counters, family_counter)
            _increment(
                counters,
                "naturality_squares_checked",
                vertical_map.naturality_squares_checked,
            )
            _increment(
                counters,
                square_counter,
                vertical_map.naturality_squares_checked,
            )
            if vertical_map.naturality_failures:
                raise CampaignMismatch(
                    CampaignFailure(
                        invariant=f"{scope}_vertical_naturality",
                        message=(
                            "an adjacent-order profile naturality square does not "
                            "commute"
                        ),
                        context={
                            "scope": scope,
                            "source_order": vertical_map.source_order,
                            "target_order": vertical_map.target_order,
                            "failure_count": len(vertical_map.naturality_failures),
                            "first_failure": repr(
                                vertical_map.naturality_failures[0]
                            ),
                        },
                    )
                )


def _permutation(
    points: tuple[Point3, ...], *, dimension: int, seed: int, index: int
) -> tuple[Point3, ...]:
    material = f"morsehgp3d-permutation:{dimension}:{seed}:{index}".encode("ascii")
    permutation_seed = int.from_bytes(hashlib.sha256(material).digest()[:8], "big")
    indices = list(range(len(points)))
    random.Random(permutation_seed).shuffle(indices)
    if len(indices) > 1 and indices == list(range(len(indices))):
        indices = indices[1:] + indices[:1]
    return tuple(points[point_index] for point_index in indices)


def _snapshot_digest(snapshot: CloudSnapshot) -> str:
    return hashlib.sha256(repr(snapshot).encode("utf-8")).hexdigest()


def _mapped_label(label: Iterable[int], point_id_map: tuple[int, ...]) -> tuple[int, ...]:
    return tuple(sorted(point_id_map[point_id] for point_id in label))


def _mapped_components(
    components: Iterable[object], point_id_map: tuple[int, ...]
) -> tuple[object, ...]:
    return tuple(
        sorted(
            (
                tuple(
                    sorted(
                        _mapped_label(facet, point_id_map)
                        for facet in getattr(component, "facet_point_ids")
                    )
                ),
                _mapped_label(
                    getattr(component, "covered_point_ids"), point_id_map
                ),
            )
            for component in components
        )
    )


def _forest_metamorphic_signature(
    forest: MergeForest,
    point_id_map: tuple[int, ...],
    level_divisor: Fraction,
) -> tuple[object, ...]:
    nodes_by_id = {node.node_id: node for node in forest.nodes}
    cache: dict[str, object] = {}

    def node_signature(node_id: str) -> object:
        if node_id in cache:
            return cache[node_id]
        node = nodes_by_id[node_id]
        signature = (
            node.kind,
            node.squared_level / level_divisor,
            _mapped_label(node.covered_point_ids, point_id_map),
            tuple(
                sorted(
                    _mapped_label(label, point_id_map)
                    for label in node.source_point_labels
                )
            ),
            tuple(sorted(node_signature(child_id) for child_id in node.child_ids)),
        )
        cache[node_id] = signature
        return signature

    def delta_signature(delta: object) -> object:
        return (
            getattr(delta, "squared_level") / level_divisor,
            node_signature(getattr(delta, "root_id")),
            tuple(
                sorted(
                    _mapped_label(facet, point_id_map)
                    for facet in getattr(delta, "added_facet_point_ids")
                )
            ),
            _mapped_label(getattr(delta, "added_point_ids"), point_id_map),
        )

    batch_signatures = []
    for batch in forest.batches:
        incidences = tuple(
            sorted(
                (
                    _mapped_label(incidence.simplex_point_ids, point_id_map),
                    _mapped_label(incidence.facet_point_ids, point_id_map),
                    (
                        node_signature(incidence.pre_lot_root_id)
                        if incidence.pre_lot_root_id is not None
                        else None
                    ),
                )
                for incidence in batch.incidences
            )
        )
        batch_signatures.append(
            (
                batch.squared_level / level_divisor,
                tuple(
                    sorted(
                        _mapped_label(facet, point_id_map)
                        for facet in batch.activated_facet_point_ids
                    )
                ),
                tuple(
                    sorted(
                        _mapped_label(simplex, point_id_map)
                        for simplex in batch.relation_simplex_point_ids
                    )
                ),
                incidences,
                _mapped_components(batch.pre_components, point_id_map),
                _mapped_components(batch.post_components, point_id_map),
                tuple(
                    sorted(node_signature(node_id) for node_id in batch.created_node_ids)
                ),
                tuple(sorted(delta_signature(delta) for delta in batch.coverage_deltas)),
            )
        )
    return (
        forest.order,
        forest.profile,
        tuple(sorted(node_signature(node.node_id) for node in forest.nodes)),
        tuple(sorted(node_signature(root_id) for root_id in forest.root_ids)),
        tuple(batch_signatures),
        tuple(sorted(delta_signature(delta) for delta in forest.coverage_log)),
    )


def _metamorphic_signature(
    snapshot: CloudSnapshot,
    point_id_map: tuple[int, ...],
    *,
    level_divisor: Fraction = Fraction(1),
) -> tuple[object, ...]:
    orders = []
    for index, filtration in enumerate(snapshot.filtrations):
        facet_records = tuple(
            sorted(
                (
                    _mapped_label(facet.point_ids, point_id_map),
                    facet.squared_level / level_divisor,
                )
                for facet in filtration.facets
            )
        )
        coface_records = tuple(
            sorted(
                (
                    _mapped_label(coface.point_ids, point_id_map),
                    coface.squared_level / level_divisor,
                    tuple(
                        sorted(
                            _mapped_label(facet, point_id_map)
                            for facet in coface.facet_point_ids
                        )
                    ),
                )
                for coface in filtration.cofaces
            )
        )
        hyperedge_records = tuple(
            sorted(
                (
                    _mapped_label(edge.simplex_point_ids, point_id_map),
                    edge.squared_level / level_divisor,
                    tuple(
                        sorted(
                            _mapped_label(facet, point_id_map)
                            for facet in edge.facet_point_ids
                        )
                    ),
                )
                for edge in filtration.gabriel_hyperedges
            )
        )
        cut_records = []
        for level in filtration.critical_levels:
            for closed in (False, True):
                cut_records.append(
                    (
                        level / level_divisor,
                        closed,
                        _mapped_components(
                            snapshot.full_forests[index].cut(
                                level, closed=closed
                            ).components,
                            point_id_map,
                        ),
                        _mapped_components(
                            snapshot.reduced_forests[index].cut(
                                level, closed=closed
                            ).components,
                            point_id_map,
                        ),
                    )
                )
        orders.append(
            (
                filtration.order,
                facet_records,
                coface_records,
                hyperedge_records,
                tuple(cut_records),
                _forest_metamorphic_signature(
                    snapshot.full_forests[index], point_id_map, level_divisor
                ),
                _forest_metamorphic_signature(
                    snapshot.reduced_forests[index], point_id_map, level_divisor
                ),
            )
        )

    vertical_records = []
    family_groups = (
        ("gamma", snapshot.vertical_maps),
        ("full_pi0", snapshot.full_profile_vertical_maps),
        ("hgp_reduced", snapshot.reduced_profile_vertical_maps),
    )
    for scope, families in family_groups:
        family_records = []
        for family in families:
            maps = tuple(
                (
                    mapping.squared_level / level_divisor,
                    mapping.closed,
                    tuple(
                        sorted(
                            (
                                tuple(
                                    sorted(
                                        _mapped_label(facet, point_id_map)
                                        for facet in assignment.source_component_facets
                                    )
                                ),
                                tuple(
                                    sorted(
                                        _mapped_label(facet, point_id_map)
                                        for facet in assignment.target_component_facets
                                    )
                                ),
                            )
                            for assignment in mapping.assignments
                        )
                    ),
                )
                for mapping in family.maps
            )
            family_records.append(
                (
                    family.source_order,
                    family.target_order,
                    maps,
                    family.naturality_squares_checked,
                    len(family.naturality_failures),
                )
            )
        vertical_records.append((scope, tuple(family_records)))
    return (len(snapshot.points), tuple(orders), tuple(vertical_records))


def _semantic_digest(signature: object) -> str:
    return hashlib.sha256(repr(signature).encode("utf-8")).hexdigest()


def _transformed_point_id_map(
    base: CloudSnapshot,
    transformed: CloudSnapshot,
    transform: Callable[
        [tuple[Fraction, Fraction, Fraction]],
        tuple[Fraction, Fraction, Fraction],
    ],
) -> tuple[int, ...]:
    transformed_to_base = {
        transform(point): point_id for point_id, point in enumerate(base.points)
    }
    if len(transformed_to_base) != len(base.points):
        raise ValueError("a metamorphic transform collapsed distinct points")
    try:
        return tuple(transformed_to_base[point] for point in transformed.points)
    except KeyError as error:
        raise ValueError("transformed point identifiers cannot be matched") from error


def _check_transformed_snapshot(
    base: CloudSnapshot,
    transformed: CloudSnapshot,
    transform: Callable[
        [tuple[Fraction, Fraction, Fraction]],
        tuple[Fraction, Fraction, Fraction],
    ],
    *,
    invariant: str,
    context: dict[str, object],
    level_divisor: Fraction = Fraction(1),
) -> None:
    identity_map = tuple(range(len(base.points)))
    transformed_map = _transformed_point_id_map(base, transformed, transform)
    _require_equal(
        invariant,
        _semantic_digest(_metamorphic_signature(base, identity_map)),
        _semantic_digest(
            _metamorphic_signature(
                transformed,
                transformed_map,
                level_divisor=level_divisor,
            )
        ),
        context=context,
    )


def _signed_axis_transform(
    *, dimension: int, seed: int
) -> tuple[
    Callable[
        [tuple[Fraction, Fraction, Fraction]],
        tuple[Fraction, Fraction, Fraction],
    ],
    tuple[int, int, int],
    tuple[int, int, int],
]:
    axis_permutations = tuple(permutations(range(3)))
    material = f"morsehgp3d-signed-axes:{dimension}:{seed}".encode("ascii")
    selector = int.from_bytes(hashlib.sha256(material).digest()[:8], "big")
    axis_order = axis_permutations[selector % len(axis_permutations)]
    selector //= len(axis_permutations)
    signs = tuple(-1 if selector & (1 << axis) else 1 for axis in range(3))
    if axis_order == (0, 1, 2) and signs == (1, 1, 1):
        signs = (-1, 1, 1)

    def transform(
        point: tuple[Fraction, Fraction, Fraction]
    ) -> tuple[Fraction, Fraction, Fraction]:
        return tuple(
            signs[axis] * point[axis_order[axis]] for axis in range(3)
        )  # type: ignore[return-value]

    return transform, axis_order, signs  # type: ignore[return-value]


def _run_metamorphic_checks(
    points: tuple[Point3, ...],
    snapshot: CloudSnapshot,
    *,
    config: CampaignConfig,
    k_max: int,
    dimension: int,
    seed: int,
    counters: dict[str, int] | None,
) -> None:
    axis_transform, axis_order, signs = _signed_axis_transform(
        dimension=dimension, seed=seed
    )
    axis_points = tuple(
        axis_transform(tuple(Fraction(value) for value in point))
        for point in points
    )
    axis_snapshot = _build_snapshot(axis_points, k_max)
    _check_transformed_snapshot(
        snapshot,
        axis_snapshot,
        axis_transform,
        invariant="signed_axis_permutation_invariance",
        context={
            "dimension": dimension,
            "seed": seed,
            "k_max": k_max,
            "axis_order": axis_order,
            "signs": signs,
        },
    )
    _increment(counters, "signed_axis_permutations_checked")

    translation = (Fraction(1, 2), Fraction(-3, 4), Fraction(5, 8))

    def translate(
        point: tuple[Fraction, Fraction, Fraction]
    ) -> tuple[Fraction, Fraction, Fraction]:
        translated = tuple(
            coordinate + offset for coordinate, offset in zip(point, translation)
        )
        if any(Fraction.from_float(float(value)) != value for value in translated):
            raise ValueError("the selected dyadic translation is not binary64-exact")
        return translated  # type: ignore[return-value]

    translated_points = tuple(
        translate(tuple(Fraction(value) for value in point)) for point in points
    )
    translated_snapshot = _build_snapshot(translated_points, k_max)
    _check_transformed_snapshot(
        snapshot,
        translated_snapshot,
        translate,
        invariant="exact_dyadic_translation_invariance",
        context={
            "dimension": dimension,
            "seed": seed,
            "k_max": k_max,
            "translation": translation,
            "binary64_exact_verified": True,
        },
    )
    _increment(counters, "dyadic_translations_checked")

    exponents = (-2, -1, 1, 2)
    exponent = exponents[(seed + dimension) % len(exponents)]
    scale = Fraction(2**exponent) if exponent > 0 else Fraction(1, 2 ** (-exponent))
    level_factor = scale * scale

    def homothety(
        point: tuple[Fraction, Fraction, Fraction]
    ) -> tuple[Fraction, Fraction, Fraction]:
        scaled = tuple(scale * coordinate for coordinate in point)
        if any(Fraction.from_float(float(value)) != value for value in scaled):
            raise ValueError("the selected homothety is not binary64-exact")
        return scaled  # type: ignore[return-value]

    scaled_points = tuple(
        homothety(tuple(Fraction(value) for value in point)) for point in points
    )
    scaled_snapshot = _build_snapshot(scaled_points, k_max)
    _check_transformed_snapshot(
        snapshot,
        scaled_snapshot,
        homothety,
        invariant="power_of_two_homothety",
        context={
            "dimension": dimension,
            "seed": seed,
            "k_max": k_max,
            "exponent_q": exponent,
            "level_factor": level_factor,
            "binary64_exact_verified": True,
        },
        level_divisor=level_factor,
    )
    _increment(counters, "homotheties_checked")
    _increment(
        counters,
        "homothety_levels_checked",
        sum(
            len(filtration.facets) + len(filtration.cofaces)
            for filtration in snapshot.filtrations
        ),
    )


def _audit_case(
    points: tuple[Point3, ...],
    *,
    config: CampaignConfig,
    k_max: int,
    dimension: int,
    seed: int,
    counters: dict[str, int] | None,
    snapshot_out: list[CloudSnapshot] | None = None,
    exercise_permutations: bool = True,
    exercise_metamorphic: bool = True,
) -> CampaignFailure | None:
    try:
        snapshot = _build_snapshot(points, k_max)
        _check_snapshot(snapshot, counters)
        permutation_count = (
            config.permutations_per_seed if exercise_permutations else 0
        )
        for permutation_index in range(permutation_count):
            permuted_points = _permutation(
                points,
                dimension=dimension,
                seed=seed,
                index=permutation_index,
            )
            permuted_snapshot = _build_snapshot(permuted_points, k_max)
            _require_equal(
                "input_permutation_invariance",
                _snapshot_digest(snapshot),
                _snapshot_digest(permuted_snapshot),
                context={
                    "dimension": dimension,
                    "seed": seed,
                    "k_max": k_max,
                    "permutation_index": permutation_index,
                },
            )
            _increment(counters, "permutations_checked")
        if (
            exercise_metamorphic
            and config.metamorphic_stride > 0
            and (seed - config.seed_start) % config.metamorphic_stride == 0
        ):
            _run_metamorphic_checks(
                points,
                snapshot,
                config=config,
                k_max=k_max,
                dimension=dimension,
                seed=seed,
                counters=counters,
            )
        if snapshot_out is not None:
            snapshot_out.append(snapshot)
        return None
    except CampaignMismatch as error:
        return error.failure
    except Exception as error:  # the campaign must turn every oracle crash into evidence
        return CampaignFailure(
            invariant="unexpected_exception",
            message=f"{type(error).__name__}: {error}",
            context={
                "dimension": dimension,
                "seed": seed,
                "k_max": k_max,
                "exception_type": type(error).__name__,
            },
        )


FailurePredicate = Callable[[tuple[Point3, ...]], CampaignFailure | None]


def minimize_failure(
    points: tuple[Point3, ...],
    failure: CampaignFailure,
    predicate: FailurePredicate,
) -> tuple[tuple[Point3, ...], CampaignFailure, int]:
    """Return a one-point-deletion-minimal reproducer for the same invariant."""

    minimized = points
    minimized_failure = failure
    evaluations = 0
    while len(minimized) > 1:
        reduced = False
        for index in range(len(minimized)):
            candidate = minimized[:index] + minimized[index + 1 :]
            evaluations += 1
            candidate_failure = predicate(candidate)
            if (
                candidate_failure is not None
                and candidate_failure.invariant == failure.invariant
            ):
                minimized = candidate
                minimized_failure = candidate_failure
                reduced = True
                break
        if not reduced:
            break
    return minimized, minimized_failure, evaluations


def _integer_shrink_candidates(value: int) -> tuple[int, ...]:
    """Return deterministic strictly smaller-magnitude integer candidates."""

    magnitude = abs(value)
    if magnitude == 0:
        return ()
    candidates = {0}
    if magnitude <= 16:
        candidates.update(range(-magnitude + 1, magnitude))
    else:
        reduced = magnitude
        while reduced > 1:
            reduced //= 2
            candidates.add(reduced)
            candidates.add(-reduced)
        candidates.update((-1, 1))
    return tuple(
        sorted(
            (
                candidate
                for candidate in candidates
                if candidate != value and abs(candidate) < magnitude
            ),
            key=lambda candidate: (abs(candidate), candidate),
        )
    )


def minimize_integer_coordinates(
    points: tuple[Point3, ...],
    failure: CampaignFailure,
    predicate: FailurePredicate,
    *,
    evaluation_budget: int = 4096,
) -> tuple[tuple[Point3, ...], CampaignFailure, int, int]:
    """Greedily shrink integer coordinates while preserving one invariant.

    Non-integer inputs are returned unchanged.  Predicate errors reject only
    the current proposal, and the explicit evaluation budget makes the pass
    fail-safe even for unusually large user-selected coordinate bounds.
    """

    if evaluation_budget < 0:
        raise ValueError("evaluation_budget must be non-negative")
    if any(
        isinstance(coordinate, bool) or not isinstance(coordinate, int)
        for point in points
        for coordinate in point
    ):
        return points, failure, 0, 0

    minimized = points
    minimized_failure = failure
    evaluations = 0
    accepted = 0
    changed = True
    while changed and evaluations < evaluation_budget:
        changed = False
        for point_index, point in enumerate(minimized):
            for coordinate_index, coordinate in enumerate(point):
                for candidate_value in _integer_shrink_candidates(coordinate):
                    if evaluations >= evaluation_budget:
                        break
                    candidate_point = tuple(
                        candidate_value if index == coordinate_index else value
                        for index, value in enumerate(point)
                    )
                    candidate_points = (
                        minimized[:point_index]
                        + (candidate_point,)  # type: ignore[arg-type]
                        + minimized[point_index + 1 :]
                    )
                    if len(set(candidate_points)) != len(candidate_points):
                        continue
                    evaluations += 1
                    try:
                        candidate_failure = predicate(candidate_points)
                    except Exception:
                        candidate_failure = None
                    if (
                        candidate_failure is not None
                        and candidate_failure.invariant == failure.invariant
                    ):
                        minimized = candidate_points
                        minimized_failure = candidate_failure
                        accepted += 1
                        changed = True
                        break
                if changed or evaluations >= evaluation_budget:
                    break
            if changed or evaluations >= evaluation_budget:
                break
    return minimized, minimized_failure, evaluations, accepted


def _snapshot_prefix_payload(
    snapshot: CloudSnapshot, common_order_count: int
) -> tuple[object, ...]:
    vertical_count = max(0, common_order_count - 1)
    return (
        snapshot.points,
        snapshot.filtrations[:common_order_count],
        snapshot.full_forests[:common_order_count],
        snapshot.reduced_forests[:common_order_count],
        snapshot.vertical_maps[:vertical_count],
        snapshot.full_profile_vertical_maps[:vertical_count],
        snapshot.reduced_profile_vertical_maps[:vertical_count],
    )


def _check_kmax_prefixes(
    snapshots: dict[int, CloudSnapshot],
    *,
    dimension: int,
    point_count: int,
    seed: int,
    counters: dict[str, int] | None,
) -> CampaignFailure | None:
    try:
        for lower_k_max, higher_k_max in combinations(sorted(snapshots), 2):
            lower = snapshots[lower_k_max]
            higher = snapshots[higher_k_max]
            common_orders = min(len(lower.filtrations), len(higher.filtrations))
            _require_equal(
                "kmax_prefix_coherence",
                _semantic_digest(
                    _snapshot_prefix_payload(lower, common_orders)
                ),
                _semantic_digest(
                    _snapshot_prefix_payload(higher, common_orders)
                ),
                context={
                    "dimension": dimension,
                    "point_count": point_count,
                    "seed": seed,
                    "lower_k_max": lower_k_max,
                    "higher_k_max": higher_k_max,
                    "common_orders": common_orders,
                },
            )
            _increment(counters, "kmax_prefix_comparisons")
        return None
    except CampaignMismatch as error:
        return error.failure


def _jsonable(value: object) -> object:
    if isinstance(value, Fraction):
        return _exact_level(value)
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, tuple):
        return [_jsonable(item) for item in value]
    if isinstance(value, list):
        return [_jsonable(item) for item in value]
    if isinstance(value, dict):
        return {str(key): _jsonable(item) for key, item in value.items()}
    if isinstance(value, (str, int, float, bool)) or value is None:
        return value
    return repr(value)


def _canonical_json(value: object, *, pretty: bool = False) -> str:
    return json.dumps(
        _jsonable(value),
        allow_nan=False,
        ensure_ascii=False,
        indent=2 if pretty else None,
        separators=None if pretty else (",", ":"),
        sort_keys=True,
    )


def _atomic_write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", suffix=".tmp", dir=path.parent
    )
    temporary_path = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
            stream.write(_canonical_json(payload, pretty=True))
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        temporary_path.replace(path)
    except BaseException:
        temporary_path.unlink(missing_ok=True)
        raise


def _failure_fixture(
    *,
    config: CampaignConfig,
    dimension: int,
    seed: int,
    point_count: int,
    k_max_values: tuple[int, ...],
    original_points: tuple[Point3, ...],
    point_deletion_minimized_points: tuple[Point3, ...],
    minimized_points: tuple[Point3, ...],
    original_failure: CampaignFailure,
    minimized_failure: CampaignFailure,
    minimization_evaluations: int,
    point_minimization_evaluations: int,
    coordinate_minimization_evaluations: int,
    coordinate_reductions_accepted: int,
) -> dict[str, object]:
    payload: dict[str, object] = {
        "schema_version": 1,
        "kind": "morsehgp3d_oracle_campaign_failure",
        "backend": "reference_cpu",
        "mode": "certified",
        "profiles": list(PROFILES),
        "dimension_requested": dimension,
        "seed": seed,
        "point_count": point_count,
        "k_max_values": list(k_max_values),
        "coordinate_bound": config.coordinate_bound,
        "permutations_per_seed": config.permutations_per_seed,
        "metamorphic_stride": config.metamorphic_stride,
        "original_points": original_points,
        "point_deletion_minimized_points": point_deletion_minimized_points,
        "minimized_points": minimized_points,
        "original_failure": original_failure.as_json(),
        "minimized_failure": minimized_failure.as_json(),
        "minimization_evaluations": minimization_evaluations,
        "point_minimization_evaluations": point_minimization_evaluations,
        "coordinate_minimization_evaluations": (
            coordinate_minimization_evaluations
        ),
        "coordinate_reductions_accepted": coordinate_reductions_accepted,
        "minimization_scope": (
            "greedy one-minimal point deletion followed by deterministic "
            "integer-coordinate shrinking"
        ),
        "reproduction": {
            "command": [
                sys.executable,
                "tools/run_oracle_campaign.py",
                "--dimensions",
                str(dimension),
                "--seed-start",
                str(seed),
                "--seeds-per-dimension",
                "1",
                "--point-count",
                str(point_count),
                "--k-max-values",
                *[str(k_max) for k_max in k_max_values],
                "--coordinate-bound",
                str(config.coordinate_bound),
                "--permutations-per-seed",
                str(config.permutations_per_seed),
                "--metamorphic-stride",
                str(config.metamorphic_stride),
            ]
        },
    }
    identity_payload = dict(payload)
    fixture_id = hashlib.sha256(
        _canonical_json(identity_payload).encode("utf-8")
    ).hexdigest()
    payload["fixture_id"] = fixture_id
    return payload


def run_campaign(config: CampaignConfig) -> dict[str, object]:
    """Run one campaign and return its JSON-safe manifest."""

    counters = _new_counters()
    dimension_elapsed_ns: dict[str, int] = {}
    cases_attempted_by_dimension = {
        str(dimension): 0 for dimension in config.dimensions
    }
    cases_completed_by_dimension = {
        str(dimension): 0 for dimension in config.dimensions
    }
    clouds_attempted_by_dimension = {
        str(dimension): 0 for dimension in config.dimensions
    }
    clouds_completed_by_dimension = {
        str(dimension): 0 for dimension in config.dimensions
    }
    failure_record: dict[str, object] | None = None
    start_ns = time.perf_counter_ns()

    def capture_failure(
        *,
        points: tuple[Point3, ...],
        dimension: int,
        point_count: int,
        seed: int,
        k_max_values: tuple[int, ...],
        failure: CampaignFailure,
        predicate: FailurePredicate,
    ) -> dict[str, object]:
        point_minimized_points, point_minimized_failure, point_evaluations = minimize_failure(
            points, failure, predicate
        )
        minimized_points, minimized_failure, coordinate_evaluations, coordinate_accepted = (
            minimize_integer_coordinates(
                point_minimized_points,
                point_minimized_failure,
                predicate,
            )
        )
        evaluations = point_evaluations + coordinate_evaluations
        _increment(
            counters, "point_minimization_evaluations", point_evaluations
        )
        _increment(
            counters,
            "coordinate_minimization_evaluations",
            coordinate_evaluations,
        )
        _increment(
            counters, "coordinate_reductions_accepted", coordinate_accepted
        )
        _increment(counters, "minimization_evaluations", evaluations)
        fixture_path: Path | None = None
        if config.failure_dir is not None:
            fixture = _failure_fixture(
                config=config,
                dimension=dimension,
                seed=seed,
                point_count=point_count,
                k_max_values=k_max_values,
                original_points=points,
                point_deletion_minimized_points=point_minimized_points,
                minimized_points=minimized_points,
                original_failure=failure,
                minimized_failure=minimized_failure,
                minimization_evaluations=evaluations,
                point_minimization_evaluations=point_evaluations,
                coordinate_minimization_evaluations=coordinate_evaluations,
                coordinate_reductions_accepted=coordinate_accepted,
            )
            fixture_path = config.failure_dir / (
                f"oracle-campaign-{fixture['fixture_id']}.json"
            )
            _atomic_write_json(fixture_path, fixture)
        return {
            "dimension": dimension,
            "point_count": point_count,
            "seed": seed,
            "k_max_values": list(k_max_values),
            "original_point_count": len(points),
            "point_deletion_minimized_point_count": len(point_minimized_points),
            "minimized_point_count": len(minimized_points),
            "minimized_points": minimized_points,
            "failure": failure.as_json(),
            "minimized_failure": minimized_failure.as_json(),
            "point_minimization_evaluations": point_evaluations,
            "coordinate_minimization_evaluations": coordinate_evaluations,
            "coordinate_reductions_accepted": coordinate_accepted,
            "fixture_path": str(fixture_path) if fixture_path is not None else None,
        }

    stop = False
    for dimension in config.dimensions:
        dimension_start_ns = time.perf_counter_ns()
        for point_count in config.point_counts:
            for seed in config.seeds:
                points = generate_affine_cloud(
                    dimension,
                    point_count,
                    seed,
                    coordinate_bound=config.coordinate_bound,
                )
                _increment(counters, "clouds_generated")
                _increment(counters, "points_generated", len(points))
                clouds_attempted_by_dimension[str(dimension)] += 1
                snapshots: dict[int, CloudSnapshot] = {}
                highest_k_max = max(config.k_max_values)

                for k_max in config.k_max_values:
                    _increment(counters, "campaign_cases_attempted")
                    cases_attempted_by_dimension[str(dimension)] += 1
                    snapshot_out: list[CloudSnapshot] = []
                    exercise_expensive_properties = k_max == highest_k_max
                    failure = _audit_case(
                        points,
                        config=config,
                        k_max=k_max,
                        dimension=dimension,
                        seed=seed,
                        counters=counters,
                        snapshot_out=snapshot_out,
                        exercise_permutations=exercise_expensive_properties,
                        exercise_metamorphic=exercise_expensive_properties,
                    )
                    if failure is not None:
                        _increment(counters, "failures")

                        def predicate(
                            candidate: tuple[Point3, ...],
                            *,
                            candidate_k_max: int = k_max,
                        ) -> CampaignFailure | None:
                            return _audit_case(
                                candidate,
                                config=config,
                                k_max=candidate_k_max,
                                dimension=dimension,
                                seed=seed,
                                counters=None,
                                exercise_permutations=exercise_expensive_properties,
                                exercise_metamorphic=exercise_expensive_properties,
                            )

                        failure_record = capture_failure(
                            points=points,
                            dimension=dimension,
                            point_count=point_count,
                            seed=seed,
                            k_max_values=(k_max,),
                            failure=failure,
                            predicate=predicate,
                        )
                        stop = True
                        break
                    if len(snapshot_out) != 1:
                        raise AssertionError("a successful audit must return one snapshot")
                    snapshots[k_max] = snapshot_out[0]
                    _increment(counters, "campaign_cases_audited")
                    cases_completed_by_dimension[str(dimension)] += 1
                if stop:
                    break

                prefix_failure = _check_kmax_prefixes(
                    snapshots,
                    dimension=dimension,
                    point_count=point_count,
                    seed=seed,
                    counters=counters,
                )
                if prefix_failure is not None:
                    _increment(counters, "failures")

                    def prefix_predicate(
                        candidate: tuple[Point3, ...]
                    ) -> CampaignFailure | None:
                        try:
                            candidate_snapshots = {
                                k_max: _build_snapshot(candidate, k_max)
                                for k_max in config.k_max_values
                            }
                        except Exception as error:
                            return CampaignFailure(
                                invariant="unexpected_exception",
                                message=f"{type(error).__name__}: {error}",
                                context={"stage": "kmax_prefix_minimization"},
                            )
                        return _check_kmax_prefixes(
                            candidate_snapshots,
                            dimension=dimension,
                            point_count=len(candidate),
                            seed=seed,
                            counters=None,
                        )

                    failure_record = capture_failure(
                        points=points,
                        dimension=dimension,
                        point_count=point_count,
                        seed=seed,
                        k_max_values=config.k_max_values,
                        failure=prefix_failure,
                        predicate=prefix_predicate,
                    )
                    stop = True
                    break

                _increment(counters, "clouds_audited")
                clouds_completed_by_dimension[str(dimension)] += 1
            if stop:
                break
        dimension_elapsed_ns[str(dimension)] = (
            time.perf_counter_ns() - dimension_start_ns
        )
        if stop:
            break

    total_elapsed_ns = time.perf_counter_ns() - start_ns
    mean_by_dimension = {
        dimension: (
            dimension_elapsed_ns[dimension] // attempted if attempted else 0
        )
        for dimension, attempted in clouds_attempted_by_dimension.items()
        if dimension in dimension_elapsed_ns
    }
    stable_identity = {
        "config": config.as_manifest(),
        "counters": counters,
        "cases_attempted_by_dimension": cases_attempted_by_dimension,
        "cases_completed_by_dimension": cases_completed_by_dimension,
        "clouds_attempted_by_dimension": clouds_attempted_by_dimension,
        "clouds_completed_by_dimension": clouds_completed_by_dimension,
        "status": "failed" if failure_record is not None else "passed",
    }
    campaign_id = hashlib.sha256(
        _canonical_json(stable_identity).encode("utf-8")
    ).hexdigest()
    return {
        "schema_version": 1,
        "kind": "morsehgp3d_oracle_campaign_manifest",
        "campaign_id": campaign_id,
        "phase": "1",
        "backend": "reference_cpu",
        "mode": "certified",
        "gcp_used": False,
        "comparison_scope": {
            "internal_consistency": [
                "full forest replay against the Gamma filtration from which it is reduced",
                "reduced forest replay against the Gabriel filtration from which it is reduced",
                "explicit cut monotonicity and separate adjacent-order naturality "
                "for Gamma, full_pi0 and hgp_reduced",
                "Kmax prefix coherence across every configured common order",
            ],
            "cross_construction_differential": [
                "covered point unions of reduced Gabriel components versus "
                "nontrivial full Gamma components"
            ],
            "metamorphic": [
                "input permutation",
                "signed axis permutation",
                "binary64-exact dyadic translation",
                "power-of-two homothety with exact squared-level scaling",
            ],
            "sampling_policy": (
                "horizontal checks run for every (dimension,n,seed,Kmax); input "
                "permutations and geometric metamorphisms run only at the largest "
                "configured Kmax for each cloud, at the declared stride"
            ),
            "independent_implementation_differential": {
                "performed": False,
                "reason": (
                    "this runner audits one exhaustive reference implementation; "
                    "external or production differentials are separate gates"
                ),
            },
        },
        "status": stable_identity["status"],
        "config": config.as_manifest(),
        "environment": {
            "python": platform.python_version(),
            "implementation": platform.python_implementation(),
            "platform": platform.platform(),
        },
        "counters": counters,
        "cases_attempted_by_dimension": cases_attempted_by_dimension,
        "cases_completed_by_dimension": cases_completed_by_dimension,
        "clouds_attempted_by_dimension": clouds_attempted_by_dimension,
        "clouds_completed_by_dimension": clouds_completed_by_dimension,
        "matrix_gate": {
            "coverage": config.matrix_coverage,
            "satisfied": all(config.matrix_coverage.values()),
            "prefix_coherence_checked": len(config.k_max_values) > 1,
            "scope": "configured n/Kmax relation matrix only",
        },
        "phase1_campaign_scale": {
            "all_affine_dimensions": config.dimensions == (1, 2, 3),
            "ten_thousand_seeds_per_dimension": (
                config.seeds_per_dimension >= 10_000
            ),
            "runner_scale_requirements_satisfied": (
                config.dimensions == (1, 2, 3)
                and config.seeds_per_dimension >= 10_000
                and all(config.matrix_coverage.values())
                and config.failure_dir is not None
                and config.permutations_per_seed > 0
                and config.metamorphic_stride > 0
            ),
            "automatic_failure_fixture_enabled": config.failure_dir is not None,
            "input_permutation_sampling_enabled": (
                config.permutations_per_seed > 0
            ),
            "geometric_metamorphic_sampling_enabled": (
                config.metamorphic_stride > 0
            ),
            "repository_phase_exit_claimed": False,
            "reason": (
                "the repository phase gate also depends on frozen fixtures, "
                "serialization evidence and reviewed artifacts outside this runner"
            ),
            "minimization_scope": (
                "counterexamples are minimized by greedy point deletion, then "
                "by deterministic bounded integer-coordinate shrinking"
            ),
        },
        "timings_ns": {
            "total": total_elapsed_ns,
            "by_dimension": dimension_elapsed_ns,
            "mean_per_generated_cloud": (
                total_elapsed_ns // counters["clouds_generated"]
                if counters["clouds_generated"]
                else 0
            ),
            "mean_per_cloud_by_dimension": mean_by_dimension,
        },
        "failure": failure_record,
    }


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run exact exhaustive Gamma/Gabriel hierarchy campaigns."
    )
    parser.add_argument(
        "--dimensions",
        nargs="+",
        type=int,
        choices=(1, 2, 3),
        default=(1, 2, 3),
        help="affine dimensions to exercise (default: 1 2 3)",
    )
    parser.add_argument("--seed-start", type=int, default=0)
    parser.add_argument(
        "--seeds-per-dimension",
        type=int,
        default=1,
        help="use 10000 for the full phase-1 gate",
    )
    parser.add_argument(
        "--point-counts",
        "--point-count",
        nargs="+",
        type=int,
        default=(6,),
        help="one or more exhaustive cloud sizes (maximum 14)",
    )
    parser.add_argument(
        "--k-max-values",
        "--k-max",
        nargs="+",
        type=int,
        default=(3,),
        help="one or more Kmax prefixes; use 3 4 5 with n=4 5 for the matrix gate",
    )
    parser.add_argument("--coordinate-bound", type=int, default=16)
    parser.add_argument("--permutations-per-seed", type=int, default=1)
    parser.add_argument(
        "--metamorphic-stride",
        type=int,
        default=1,
        help=(
            "run signed-axis, dyadic-translation and homothety checks every Nth "
            "seed; 0 disables them for a matrix-only run"
        ),
    )
    parser.add_argument(
        "--ci",
        action="store_true",
        help=(
            "enforce a short CI budget (at most 3 seeds/dimension, n=7, "
            "k=5 and 6 n/K pairs)"
        ),
    )
    parser.add_argument(
        "--failure-dir",
        type=Path,
        help="explicit directory in which a minimized failure fixture may be written",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        help="optional JSON manifest path; the manifest is always printed to stdout",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = _parser()
    arguments = parser.parse_args(argv)
    try:
        config = CampaignConfig(
            dimensions=tuple(arguments.dimensions),
            seed_start=arguments.seed_start,
            seeds_per_dimension=arguments.seeds_per_dimension,
            point_counts=tuple(arguments.point_counts),
            k_max_values=tuple(arguments.k_max_values),
            coordinate_bound=arguments.coordinate_bound,
            permutations_per_seed=arguments.permutations_per_seed,
            metamorphic_stride=arguments.metamorphic_stride,
            ci=arguments.ci,
            failure_dir=arguments.failure_dir,
        )
    except (TypeError, ValueError) as error:
        parser.error(str(error))
    manifest = run_campaign(config)
    if arguments.manifest is not None:
        _atomic_write_json(arguments.manifest, manifest)
    print(_canonical_json(manifest, pretty=True))
    return 0 if manifest["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
