"""Independent exhaustive construction of the filtered graphs :math:`Gamma_k`.

This module is deliberately combinatorial.  Its only geometric dependency is a
callable with the signature ``minimum_enclosing_ball(points, subset_ids)``.  It
never imports CUDA or production reduction code.
"""

from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations
from typing import Callable, Iterable, Literal, Protocol, Sequence, TypeAlias


PointId: TypeAlias = int
PointLabel: TypeAlias = tuple[PointId, ...]
Point3: TypeAlias = tuple[Fraction, Fraction, Fraction]
GraphKind: TypeAlias = Literal["gamma", "gabriel"]


class BallLike(Protocol):
    """Structural return type of the exact miniball primitive."""

    center: Sequence[Fraction]
    squared_radius: Fraction
    support_ids: Sequence[int]


BallFunction: TypeAlias = Callable[[Sequence[object], Iterable[int]], BallLike]


def canonical_label(point_ids: Iterable[int], *, size: int | None = None) -> PointLabel:
    """Return a sorted, duplicate-free point label and validate its cardinality."""

    raw_label = tuple(point_ids)
    if any(
        isinstance(point_id, bool) or not isinstance(point_id, int)
        for point_id in raw_label
    ):
        raise TypeError("point identifiers must be integers")
    label = tuple(sorted(raw_label))
    if len(label) != len(set(label)):
        raise ValueError("point labels must not contain duplicates")
    if any(point_id < 0 for point_id in label):
        raise ValueError("point identifiers must be non-negative")
    if size is not None and len(label) != size:
        raise ValueError(f"expected a label of size {size}, got {len(label)}")
    return label


def facets_of(simplex: PointLabel) -> tuple[PointLabel, ...]:
    """Return all codimension-one facets in canonical lexicographic order."""

    if len(simplex) < 2:
        return ()
    return tuple(
        sorted(simplex[:index] + simplex[index + 1 :] for index in range(len(simplex)))
    )


@dataclass(frozen=True, order=True)
class GammaFacet:
    """One vertex of :math:`Gamma_k`, born at its exact miniball level."""

    point_ids: PointLabel
    squared_level: Fraction


@dataclass(frozen=True, order=True)
class GammaCoface:
    """One elementary adjacency carrier of cardinality ``order + 1``."""

    point_ids: PointLabel
    squared_level: Fraction
    facet_point_ids: tuple[PointLabel, ...]
    center: Point3
    support_ids: PointLabel


@dataclass(frozen=True, order=True)
class GabrielHyperedge:
    """A coface whose miniball has no strictly interior external observation."""

    order: int
    simplex_point_ids: PointLabel
    facet_point_ids: tuple[PointLabel, ...]
    squared_level: Fraction
    center: Point3
    support_ids: PointLabel


@dataclass(frozen=True)
class GammaComponent:
    """A connected component represented by facets and their point union."""

    facet_point_ids: tuple[PointLabel, ...]
    covered_point_ids: PointLabel

    @property
    def nontrivial(self) -> bool:
        return len(self.facet_point_ids) > 1


@dataclass(frozen=True)
class GammaCut:
    """Canonical component state at an exact open or closed threshold."""

    order: int
    squared_level: Fraction
    closed: bool
    graph_kind: GraphKind
    active_facet_point_ids: tuple[PointLabel, ...]
    components: tuple[GammaComponent, ...]

    @property
    def component_by_facet(self) -> dict[PointLabel, GammaComponent]:
        return {
            facet: component
            for component in self.components
            for facet in component.facet_point_ids
        }


@dataclass(frozen=True)
class GammaBatch:
    """Every birth and elementary adjacency sharing one exact level."""

    order: int
    squared_level: Fraction
    facet_births: tuple[GammaFacet, ...]
    cofaces: tuple[GammaCoface, ...]
    gabriel_hyperedges: tuple[GabrielHyperedge, ...]


class _DisjointSet:
    def __init__(self, values: Iterable[PointLabel]) -> None:
        self.parent = {value: value for value in values}

    def find(self, value: PointLabel) -> PointLabel:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: PointLabel, right: PointLabel) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        if left_root < right_root:
            self.parent[right_root] = left_root
        else:
            self.parent[left_root] = right_root


@dataclass(frozen=True)
class GammaFiltration:
    """Complete exhaustive filtration for one order."""

    point_count: int
    order: int
    facets: tuple[GammaFacet, ...]
    cofaces: tuple[GammaCoface, ...]
    gabriel_hyperedges: tuple[GabrielHyperedge, ...]

    def __post_init__(self) -> None:
        if self.point_count < 1:
            raise ValueError("point_count must be positive")
        if not 1 <= self.order <= self.point_count:
            raise ValueError("order must lie in 1..point_count")
        expected_facets = tuple(sorted(facet.point_ids for facet in self.facets))
        if expected_facets != tuple(facet.point_ids for facet in self.facets):
            raise ValueError("facets must be canonically sorted")
        expected_cofaces = tuple(sorted(coface.point_ids for coface in self.cofaces))
        if expected_cofaces != tuple(coface.point_ids for coface in self.cofaces):
            raise ValueError("cofaces must be canonically sorted")

    @property
    def critical_levels(self) -> tuple[Fraction, ...]:
        return tuple(
            sorted(
                {facet.squared_level for facet in self.facets}
                | {coface.squared_level for coface in self.cofaces}
            )
        )

    @property
    def batches(self) -> tuple[GammaBatch, ...]:
        hyperedges_by_level: dict[Fraction, list[GabrielHyperedge]] = {}
        for hyperedge in self.gabriel_hyperedges:
            hyperedges_by_level.setdefault(hyperedge.squared_level, []).append(hyperedge)
        return tuple(
            GammaBatch(
                order=self.order,
                squared_level=level,
                facet_births=tuple(
                    facet for facet in self.facets if facet.squared_level == level
                ),
                cofaces=tuple(
                    coface for coface in self.cofaces if coface.squared_level == level
                ),
                gabriel_hyperedges=tuple(
                    sorted(
                        hyperedges_by_level.get(level, []),
                        key=lambda edge: edge.simplex_point_ids,
                    )
                ),
            )
            for level in self.critical_levels
        )

    def cut(
        self,
        squared_level: Fraction,
        *,
        closed: bool = True,
        graph_kind: GraphKind = "gamma",
        include_isolated: bool | None = None,
    ) -> GammaCut:
        """Compute components without choosing a floating sentinel around a level.

        ``closed=False`` means the strict sublevel at the same exact value.  For
        the Gabriel graph, isolated facets are included by default only at order
        one, matching the normative reduced profile.
        """

        level = Fraction(squared_level)
        if graph_kind not in ("gamma", "gabriel"):
            raise ValueError(f"unknown graph kind {graph_kind!r}")
        if include_isolated is None:
            include_isolated = graph_kind == "gamma" or self.order == 1

        def active(value: Fraction) -> bool:
            return value <= level if closed else value < level

        all_active_facets = {
            facet.point_ids for facet in self.facets if active(facet.squared_level)
        }
        if graph_kind == "gamma":
            active_relations = [
                coface.facet_point_ids
                for coface in self.cofaces
                if active(coface.squared_level)
            ]
        else:
            active_relations = [
                edge.facet_point_ids
                for edge in self.gabriel_hyperedges
                if active(edge.squared_level)
            ]

        if include_isolated:
            active_facets = set(all_active_facets)
        else:
            active_facets = {
                facet for relation in active_relations for facet in relation
            }
        disjoint_set = _DisjointSet(active_facets)
        for relation in active_relations:
            relation_facets = [facet for facet in relation if facet in active_facets]
            if len(relation_facets) != len(relation):
                raise ValueError("an active coface references an inactive facet")
            for facet in relation_facets[1:]:
                disjoint_set.union(relation_facets[0], facet)

        grouped: dict[PointLabel, list[PointLabel]] = {}
        for facet in sorted(active_facets):
            grouped.setdefault(disjoint_set.find(facet), []).append(facet)
        components = []
        for facet_group in grouped.values():
            canonical_facets = tuple(sorted(facet_group))
            covered = canonical_label(
                {point_id for facet in canonical_facets for point_id in facet}
            )
            components.append(GammaComponent(canonical_facets, covered))
        components.sort(key=lambda component: component.facet_point_ids)
        return GammaCut(
            order=self.order,
            squared_level=level,
            closed=closed,
            graph_kind=graph_kind,
            active_facet_point_ids=tuple(sorted(active_facets)),
            components=tuple(components),
        )

    def closed_cuts(self, *, graph_kind: GraphKind = "gamma") -> tuple[GammaCut, ...]:
        return tuple(self.cut(level, graph_kind=graph_kind) for level in self.critical_levels)


def _point_coordinates(point: object) -> Point3:
    if hasattr(point, "coordinates"):
        coordinates = getattr(point, "coordinates")
        coordinates = coordinates() if callable(coordinates) else coordinates
    elif hasattr(point, "coords"):
        coordinates = getattr(point, "coords")
        coordinates = coordinates() if callable(coordinates) else coordinates
    elif all(hasattr(point, axis) for axis in ("x", "y", "z")):
        coordinates = (getattr(point, "x"), getattr(point, "y"), getattr(point, "z"))
    else:
        coordinates = point
    try:
        values = tuple(Fraction(value) for value in coordinates)  # type: ignore[arg-type]
    except (TypeError, ValueError, ZeroDivisionError) as error:
        raise TypeError("each point must expose three exact coordinates") from error
    if len(values) != 3:
        raise ValueError("MorseHGP3D points must have exactly three coordinates")
    return values  # type: ignore[return-value]


def _default_ball_function() -> BallFunction:
    from .geometry import minimum_enclosing_ball

    return minimum_enclosing_ball


def _coerce_ball(ball: BallLike, subset: PointLabel) -> tuple[Point3, Fraction, PointLabel]:
    try:
        center_values = tuple(Fraction(value) for value in ball.center)
        squared_radius = Fraction(ball.squared_radius)
        support_ids = canonical_label(ball.support_ids)
    except (AttributeError, TypeError, ValueError, ZeroDivisionError) as error:
        raise TypeError("minimum_enclosing_ball returned an invalid exact ball") from error
    if len(center_values) != 3:
        raise ValueError("a miniball center must have three coordinates")
    if squared_radius < 0:
        raise ValueError("a squared miniball radius cannot be negative")
    if not support_ids or not set(support_ids) <= set(subset):
        raise ValueError("miniball support_ids must be a non-empty subset of the label")
    return center_values, squared_radius, support_ids  # type: ignore[return-value]


def _squared_distance(point: Point3, center: Point3) -> Fraction:
    return sum((coordinate - center_coordinate) ** 2 for coordinate, center_coordinate in zip(point, center))


def build_gamma_filtration(
    points: Sequence[object],
    order: int,
    *,
    ball_fn: BallFunction | None = None,
) -> GammaFiltration:
    """Exhaustively enumerate all ``k`` facets and ``k+1`` cofaces."""

    point_count = len(points)
    if point_count < 1:
        raise ValueError("the oracle requires a non-empty point cloud")
    if not 1 <= order <= point_count:
        raise ValueError("order must lie in 1..len(points)")
    exact_points = tuple(_point_coordinates(point) for point in points)
    miniball = ball_fn or _default_ball_function()

    facets = []
    for raw_label in combinations(range(point_count), order):
        label = canonical_label(raw_label, size=order)
        _, squared_level, _ = _coerce_ball(miniball(exact_points, label), label)
        facets.append(GammaFacet(label, squared_level))

    cofaces = []
    hyperedges = []
    if order < point_count:
        for raw_simplex in combinations(range(point_count), order + 1):
            simplex = canonical_label(raw_simplex, size=order + 1)
            center, squared_level, support_ids = _coerce_ball(
                miniball(exact_points, simplex), simplex
            )
            facet_labels = facets_of(simplex)
            coface = GammaCoface(
                point_ids=simplex,
                squared_level=squared_level,
                facet_point_ids=facet_labels,
                center=center,
                support_ids=support_ids,
            )
            cofaces.append(coface)
            external_ids = set(range(point_count)) - set(simplex)
            if all(
                _squared_distance(exact_points[point_id], center) >= squared_level
                for point_id in external_ids
            ):
                hyperedges.append(
                    GabrielHyperedge(
                        order=order,
                        simplex_point_ids=simplex,
                        facet_point_ids=facet_labels,
                        squared_level=squared_level,
                        center=center,
                        support_ids=support_ids,
                    )
                )

    return GammaFiltration(
        point_count=point_count,
        order=order,
        facets=tuple(sorted(facets, key=lambda facet: facet.point_ids)),
        cofaces=tuple(sorted(cofaces, key=lambda coface: coface.point_ids)),
        gabriel_hyperedges=tuple(
            sorted(hyperedges, key=lambda edge: edge.simplex_point_ids)
        ),
    )


def build_gabriel_hyperedges(
    points: Sequence[object],
    order: int,
    *,
    ball_fn: BallFunction | None = None,
) -> tuple[GabrielHyperedge, ...]:
    """Convenience entry point for the exhaustive K-Gabriel hypergraph."""

    return build_gamma_filtration(points, order, ball_fn=ball_fn).gabriel_hyperedges
