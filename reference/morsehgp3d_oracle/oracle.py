"""Independent orchestration for the exhaustive MorseHGP3D CPU oracle.

The geometry and combinatorics live in sibling reference modules.  This file
only validates/canonicalizes the input, runs every effective order, and keeps
the resulting artifacts together.  It intentionally imports no production
package.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from fractions import Fraction
from math import comb, isfinite
from typing import Any, Iterable, Protocol, Sequence, TypeAlias, runtime_checkable


Scalar: TypeAlias = int | float | Fraction
Point3: TypeAlias = tuple[Fraction, Fraction, Fraction]
Profile: TypeAlias = str

SUPPORTED_PROFILES = frozenset({"hgp_reduced", "full_pi0"})


class UnsupportedDegeneracyError(ValueError):
    """Raised when the phase-1 generic-domain gate is not closed."""


@dataclass(frozen=True, slots=True)
class CanonicalPoint:
    """One exact point after deterministic coordinate ordering."""

    point_id: int
    coordinates: Point3
    source_indices: tuple[int, ...]


@dataclass(frozen=True, slots=True)
class OracleAttachment:
    """One strict arm frozen against its pre-lot forest root.

    Attachments are part of the conditional ``full_pi0`` reconstruction
    contract.  The reduced profile has no attachment records: its Gabriel
    relations already identify the roots directly.
    """

    event: Any
    order: int
    removed_shell_id: int
    arm_point_ids: tuple[int, ...]
    event_squared_level: Fraction
    target_node_id: str


@dataclass(frozen=True, slots=True)
class OracleOrderResult:
    """All artifacts produced for one multicover order."""

    order: int
    filtration: Any
    critical_levels: tuple[Fraction, ...]
    cuts: tuple[Any, ...]
    forest: Any
    hyperedges: tuple[Any, ...] = ()
    attachments: tuple[Any, ...] = ()
    equal_level_batches: tuple[Any, ...] = ()
    coverage_log: tuple[Any, ...] = ()

    def __post_init__(self) -> None:
        if self.order < 1:
            raise ValueError("an oracle order is one-based")
        if tuple(sorted(set(self.critical_levels))) != self.critical_levels:
            raise ValueError("critical_levels must be strictly increasing")


@dataclass(frozen=True, slots=True)
class OracleResult:
    """Internal, backend-independent result prior to JSON serialization."""

    input_points: tuple[CanonicalPoint, ...]
    k_max: int
    k_eff: int
    s_max: int
    profile: Profile
    critical_catalog: tuple[Any, ...]
    orders: tuple[OracleOrderResult, ...]
    vertical_maps: tuple[Any, ...]
    metadata: dict[str, Any] = field(default_factory=dict, compare=False, repr=False)

    def __post_init__(self) -> None:
        if not self.input_points:
            raise ValueError("the oracle requires at least one point")
        if self.profile not in SUPPORTED_PROFILES:
            raise ValueError(f"unsupported profile {self.profile!r}")
        if self.k_eff != min(self.k_max, len(self.input_points)):
            raise ValueError("k_eff is inconsistent with k_max and the input")
        if self.s_max != min(self.k_eff + 1, len(self.input_points)):
            raise ValueError("s_max is inconsistent with k_eff and the input")
        expected_orders = tuple(range(1, self.k_eff + 1))
        if tuple(result.order for result in self.orders) != expected_orders:
            raise ValueError("orders must contain exactly 1..k_eff")
        if len(self.vertical_maps) != max(0, self.k_eff - 1):
            raise ValueError("one vertical map is required for every adjacent order")

    @property
    def points(self) -> tuple[Point3, ...]:
        return tuple(point.coordinates for point in self.input_points)

    @property
    def forests(self) -> tuple[Any, ...]:
        return tuple(result.forest for result in self.orders)

    @property
    def cuts_by_order(self) -> tuple[tuple[Any, ...], ...]:
        return tuple(result.cuts for result in self.orders)

    @property
    def gabriel_hyperedges(self) -> tuple[Any, ...]:
        return tuple(edge for result in self.orders for edge in result.hyperedges)

    @property
    def attachments(self) -> tuple[Any, ...]:
        return tuple(item for result in self.orders for item in result.attachments)

    @property
    def equal_level_batches(self) -> tuple[Any, ...]:
        return tuple(batch for result in self.orders for batch in result.equal_level_batches)

    @property
    def coverage_log(self) -> tuple[Any, ...]:
        return tuple(delta for result in self.orders for delta in result.coverage_log)

    def for_order(self, order: int) -> OracleOrderResult:
        if not 1 <= order <= self.k_eff:
            raise IndexError(f"order {order} is outside 1..{self.k_eff}")
        return self.orders[order - 1]


@runtime_checkable
class OracleEngine(Protocol):
    """Small adapter boundary around the shared reference modules.

    Keeping this protocol at orchestration level avoids imposing a second API
    on geometry, catalogue, gamma, or hierarchy.  The default adapter below is
    the only place that knows their concrete function signatures.
    """

    def build_catalog(
        self, points: tuple[Point3, ...], k_eff: int
    ) -> Sequence[Any]: ...

    def build_order(
        self,
        points: tuple[Point3, ...],
        order: int,
        profile: Profile,
        catalog: tuple[Any, ...],
    ) -> OracleOrderResult: ...

    def build_vertical_map(
        self,
        source: OracleOrderResult,
        target: OracleOrderResult,
        profile: Profile,
    ) -> Any: ...


def _as_fraction(value: Scalar) -> Fraction:
    if isinstance(value, bool):
        raise TypeError("boolean coordinates are not allowed")
    if isinstance(value, Fraction):
        return value
    if isinstance(value, int):
        return Fraction(value)
    if isinstance(value, float):
        if not isfinite(value):
            raise ValueError("coordinates must be finite")
        if value == 0.0:
            return Fraction(0)
        return Fraction.from_float(value)
    raise TypeError(f"unsupported coordinate type {type(value).__name__}")


def canonicalize_points(points: Iterable[Sequence[Scalar]]) -> tuple[CanonicalPoint, ...]:
    """Interpret coordinates exactly, reject duplicates, and assign stable IDs."""

    decoded: list[tuple[Point3, int]] = []
    for source_index, raw_point in enumerate(points):
        if len(raw_point) != 3:
            raise ValueError("every point must contain exactly three coordinates")
        point = tuple(_as_fraction(value) for value in raw_point)
        decoded.append((point, source_index))
    if not decoded:
        raise ValueError("the oracle requires a non-empty point cloud")

    decoded.sort(key=lambda item: (item[0], item[1]))
    for previous, current in zip(decoded, decoded[1:]):
        if previous[0] == current[0]:
            raise ValueError(
                "duplicate coordinates are outside the phase-1 set oracle contract"
            )
    return tuple(
        CanonicalPoint(
            point_id=point_id,
            coordinates=coordinates,
            source_indices=(source_index,),
        )
        for point_id, (coordinates, source_index) in enumerate(decoded)
    )


def run_oracle(
    points: Iterable[Sequence[Scalar]],
    k_max: int,
    *,
    profile: Profile = "full_pi0",
    engine: OracleEngine | None = None,
) -> OracleResult:
    """Compute the exhaustive reference result for every effective order."""

    if isinstance(k_max, bool) or not isinstance(k_max, int):
        raise TypeError("k_max must be an integer")
    if not 1 <= k_max <= 10:
        raise ValueError("k_max must lie in 1..10")
    if profile not in SUPPORTED_PROFILES:
        raise ValueError(f"unsupported profile {profile!r}")

    input_points = canonicalize_points(points)
    exact_points = tuple(point.coordinates for point in input_points)
    k_eff = min(k_max, len(exact_points))
    s_max = min(k_eff + 1, len(exact_points))
    active_engine = engine if engine is not None else _default_engine()

    catalog = tuple(active_engine.build_catalog(exact_points, k_eff))
    orders = tuple(
        active_engine.build_order(exact_points, order, profile, catalog)
        for order in range(1, k_eff + 1)
    )
    vertical_maps = tuple(
        active_engine.build_vertical_map(orders[source - 1], orders[source - 2], profile)
        for source in range(2, k_eff + 1)
    )
    export_metrics = getattr(active_engine, "export_metrics", None)
    metadata = (
        {"reference_metrics": export_metrics()}
        if callable(export_metrics)
        else {}
    )
    return OracleResult(
        input_points=input_points,
        k_max=k_max,
        k_eff=k_eff,
        s_max=s_max,
        profile=profile,
        critical_catalog=catalog,
        orders=orders,
        vertical_maps=vertical_maps,
        metadata=metadata,
    )


def _default_engine() -> OracleEngine:
    """Load the concrete sibling-module adapter only when it is needed."""

    return SharedReferenceEngine()


class SharedReferenceEngine:
    """Concrete adapter for the sibling exhaustive reference modules."""

    def __init__(self) -> None:
        self._metrics: dict[str, int] = {}

    def export_metrics(self) -> dict[str, int]:
        return dict(sorted(self._metrics.items()))

    def build_catalog(
        self, points: tuple[Point3, ...], k_eff: int
    ) -> Sequence[Any]:
        from .catalog import build_critical_catalog

        catalog = build_critical_catalog(points, k_eff)
        statistics = catalog.statistics
        if statistics is None:
            raise AssertionError("the exhaustive catalogue omitted its statistics")
        expected_support_candidates = sum(
            comb(len(points), support_size)
            for support_size in range(1, min(4, len(points)) + 1)
        )
        if statistics.support_candidates != expected_support_candidates:
            raise AssertionError(
                "the reference catalogue did not enumerate the complete 3D support universe"
            )
        self._metrics = {
            "catalog_support_candidates": statistics.support_candidates,
            "catalog_affine_dependent_candidates": (
                statistics.affine_dependent_candidates
            ),
            "catalog_non_well_centred_candidates": (
                statistics.non_well_centred_candidates
            ),
            "catalog_extra_shell_candidates": statistics.extra_shell_candidates,
            "catalog_above_rank_candidates": statistics.above_rank_candidates,
            "catalog_accepted_supports": statistics.accepted_supports,
            "catalog_deduplicated_events": statistics.deduplicated_events,
            "catalog_accepted_events": statistics.accepted_events,
            "catalog_rejected_candidates": statistics.rejected_candidates,
            "catalog_point_classifications": statistics.point_classifications,
            # This backend enumerates every possible affine-independent
            # miniball support directly. It invokes no restricted-cell
            # cascade, so no canonical child or active cross-incidence is
            # required by this enumeration strategy.
            "catalog_support_universe_complete": 1,
            "canonical_cells_required": 0,
            "canonical_cells_closed": 0,
            "active_cross_incidences_required": 0,
            "active_cross_incidences_closed": 0,
            "gamma_miniball_queries": 0,
            "gamma_facets_enumerated": 0,
            "gamma_cofaces_enumerated": 0,
        }
        if not catalog.relevant_gp_complete:
            witnesses = ", ".join(
                f"{candidate.reason}:{candidate.shell_ids!r}"
                for candidate in catalog.degenerate_candidates[:4]
            )
            raise UnsupportedDegeneracyError(
                "the exhaustive hierarchy oracle is restricted to the phase-1 "
                f"generic domain; exact catalogue degeneracies: {witnesses}"
            )
        return tuple(catalog.events)

    def build_order(
        self,
        points: tuple[Point3, ...],
        order: int,
        profile: Profile,
        catalog: tuple[Any, ...],
    ) -> OracleOrderResult:
        from .gamma import build_gamma_filtration
        from .geometry import minimum_enclosing_ball
        from .hierarchy import build_merge_forest

        def counted_miniball(exact_points: Sequence[object], point_ids: Iterable[int]) -> Any:
            self._metrics["gamma_miniball_queries"] += 1
            return minimum_enclosing_ball(exact_points, point_ids)

        filtration = build_gamma_filtration(points, order, ball_fn=counted_miniball)
        self._metrics["gamma_facets_enumerated"] += len(filtration.facets)
        self._metrics["gamma_cofaces_enumerated"] += len(filtration.cofaces)
        forest = build_merge_forest(filtration, profile)  # type: ignore[arg-type]
        graph_kind = "gamma" if profile == "full_pi0" else "gabriel"
        cuts = tuple(
            filtration.cut(level, closed=closed, graph_kind=graph_kind)
            for level in filtration.critical_levels
            for closed in (False, True)
        )
        attachments = (
            _build_full_pi0_attachments(catalog, forest, order)
            if profile == "full_pi0"
            else ()
        )
        return OracleOrderResult(
            order=order,
            filtration=filtration,
            critical_levels=filtration.critical_levels,
            cuts=cuts,
            forest=forest,
            hyperedges=filtration.gabriel_hyperedges,
            attachments=attachments,
            equal_level_batches=forest.batches,
            coverage_log=forest.coverage_log,
        )

    def build_vertical_map(
        self,
        source: OracleOrderResult,
        target: OracleOrderResult,
        profile: Profile,
    ) -> Any:
        from .hierarchy import build_profile_vertical_map_family

        vertical_map = build_profile_vertical_map_family(
            source.filtration,
            target.filtration,
            source.forest,
            target.forest,
            profile,  # type: ignore[arg-type]
        )
        vertical_map.assert_natural()
        return vertical_map


def _build_full_pi0_attachments(
    catalog: tuple[Any, ...], forest: Any, order: int
) -> tuple[OracleAttachment, ...]:
    """Join critical index-one arms with the frozen pre-lot incidences."""

    batches_by_level = {batch.squared_level: batch for batch in forest.batches}
    attachments: list[OracleAttachment] = []
    for event in catalog:
        if event.saddle_order != order:
            continue
        simplex = tuple(event.point_ids)
        batch = batches_by_level.get(event.squared_radius)
        if batch is None:
            raise ValueError(
                f"critical event {simplex!r} has no full_pi0 batch at "
                f"level {event.squared_radius}"
            )
        incidence_by_arm = {
            incidence.facet_point_ids: incidence
            for incidence in batch.incidences
            if incidence.simplex_point_ids == simplex
        }
        for removed_shell_id in event.shell_ids:
            arm = tuple(point_id for point_id in simplex if point_id != removed_shell_id)
            incidence = incidence_by_arm.get(arm)
            if incidence is None or incidence.pre_lot_root_id is None:
                raise ValueError(
                    "a strict critical arm has no frozen pre-lot forest root: "
                    f"event={simplex!r}, arm={arm!r}, order={order}"
                )
            attachments.append(
                OracleAttachment(
                    event=event,
                    order=order,
                    removed_shell_id=removed_shell_id,
                    arm_point_ids=arm,
                    event_squared_level=event.squared_radius,
                    target_node_id=incidence.pre_lot_root_id,
                )
            )
    return tuple(
        sorted(
            attachments,
            key=lambda item: (
                item.event_squared_level,
                tuple(item.event.point_ids),
                item.removed_shell_id,
            ),
        )
    )
