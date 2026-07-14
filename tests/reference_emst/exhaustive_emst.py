"""Independent exact complete-graph EMST oracle for the phase-1 tests.

The implementation deliberately imports no MorseHGP3D module.  It computes
every Euclidean edge with :class:`fractions.Fraction`, freezes components before
each equal-weight batch, and runs a deterministic Kruskal pass.  The selected
tree and the complete graph can then be replayed independently at exact open or
closed HGP thresholds.
"""

from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations, groupby
from math import isfinite
from typing import Iterable, Literal, Sequence, TypeAlias


Scalar: TypeAlias = int | float | Fraction
Point3: TypeAlias = tuple[Fraction, Fraction, Fraction]
Component: TypeAlias = tuple[int, ...]
EdgeSource: TypeAlias = Literal["complete_graph", "selected_emst"]


class EMSTInputError(ValueError):
    """Raised when a point cloud cannot denote the required finite set."""


def _exact_scalar(value: Scalar) -> Fraction:
    if isinstance(value, bool):
        raise TypeError("boolean coordinates are not allowed")
    if isinstance(value, Fraction):
        return value
    if isinstance(value, int):
        return Fraction(value)
    if isinstance(value, float):
        if not isfinite(value):
            raise EMSTInputError("EMST coordinates must be finite")
        return Fraction.from_float(0.0 if value == 0.0 else value)
    raise TypeError(f"unsupported EMST coordinate type {type(value).__name__}")


def _point_coordinates(point: Sequence[Scalar] | object) -> object:
    if hasattr(point, "coordinates"):
        coordinates = getattr(point, "coordinates")
        return coordinates() if callable(coordinates) else coordinates
    if hasattr(point, "coords"):
        coordinates = getattr(point, "coords")
        return coordinates() if callable(coordinates) else coordinates
    if all(hasattr(point, axis) for axis in ("x", "y", "z")):
        return (getattr(point, "x"), getattr(point, "y"), getattr(point, "z"))
    return point


def _normalize_points(points: Iterable[Sequence[Scalar] | object]) -> tuple[Point3, ...]:
    normalized = []
    for point in points:
        raw_coordinates = _point_coordinates(point)
        try:
            coordinates = tuple(raw_coordinates)  # type: ignore[arg-type]
        except TypeError as error:
            raise TypeError("an EMST point must expose iterable coordinates") from error
        if len(coordinates) != 3:
            raise EMSTInputError("every EMST point must have exactly three coordinates")
        normalized.append(tuple(_exact_scalar(value) for value in coordinates))
    if not normalized:
        raise EMSTInputError("the EMST oracle requires a non-empty point cloud")
    if len(set(normalized)) != len(normalized):
        raise EMSTInputError("the phase-1 EMST oracle requires distinct sites")
    return tuple(normalized)  # type: ignore[return-value]


def _squared_distance(left: Point3, right: Point3) -> Fraction:
    return sum(
        ((left_coordinate - right_coordinate) ** 2 for left_coordinate, right_coordinate in zip(left, right)),
        Fraction(0),
    )


def exact_affine_dimension(
    points: Iterable[Sequence[Scalar] | object],
) -> int:
    """Return the affine dimension using this reference's own rational rank.

    This helper exists so the EMST campaign does not rely on the generator or
    the MorseHGP3D oracle to assert which affine-dimension class it exercised.
    """

    exact_points = _normalize_points(points)
    origin = exact_points[0]
    matrix = [
        [coordinate - origin[axis] for axis, coordinate in enumerate(point)]
        for point in exact_points[1:]
    ]
    pivot_row = 0
    for column in range(3):
        pivot = next(
            (
                row
                for row in range(pivot_row, len(matrix))
                if matrix[row][column] != 0
            ),
            None,
        )
        if pivot is None:
            continue
        matrix[pivot_row], matrix[pivot] = matrix[pivot], matrix[pivot_row]
        pivot_value = matrix[pivot_row][column]
        matrix[pivot_row] = [value / pivot_value for value in matrix[pivot_row]]
        for row in range(len(matrix)):
            if row == pivot_row or matrix[row][column] == 0:
                continue
            factor = matrix[row][column]
            matrix[row] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(matrix[row], matrix[pivot_row])
            ]
        pivot_row += 1
        if pivot_row == len(matrix):
            break
    return pivot_row


class _DisjointSet:
    def __init__(self, size: int) -> None:
        self.parent = list(range(size))

    def find(self, value: int) -> int:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: int, right: int) -> bool:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return False
        if left_root < right_root:
            self.parent[right_root] = left_root
        else:
            self.parent[left_root] = right_root
        return True


def _components(disjoint_set: _DisjointSet, point_count: int) -> tuple[Component, ...]:
    grouped: dict[int, list[int]] = {}
    for point_id in range(point_count):
        grouped.setdefault(disjoint_set.find(point_id), []).append(point_id)
    return tuple(sorted(tuple(group) for group in grouped.values()))


@dataclass(frozen=True, order=True)
class EMSTEdge:
    """One exact complete-graph edge and its HGP merge level."""

    left_id: int
    right_id: int
    squared_distance: Fraction

    def __post_init__(self) -> None:
        if not 0 <= self.left_id < self.right_id:
            raise ValueError("EMST edge identifiers must satisfy 0 <= left < right")
        if self.squared_distance <= 0:
            raise ValueError("distinct EMST sites must have a positive squared distance")

    @property
    def squared_level(self) -> Fraction:
        """Return the multicover level, one quarter of the edge weight."""

        return self.squared_distance / 4

    @property
    def point_ids(self) -> tuple[int, int]:
        return (self.left_id, self.right_id)


@dataclass(frozen=True)
class EMSTMultifusion:
    """One simultaneous contraction of frozen pre-batch components."""

    child_components: tuple[Component, ...]
    merged_component: Component

    def __post_init__(self) -> None:
        if len(self.child_components) < 2:
            raise ValueError("a merge must contain at least two pre-batch components")
        if tuple(sorted(self.child_components)) != self.child_components:
            raise ValueError("merge children must be canonically ordered")
        covered = tuple(
            sorted(point_id for component in self.child_components for point_id in component)
        )
        if covered != self.merged_component:
            raise ValueError("merged_component must be the exact union of its children")

    @property
    def arity(self) -> int:
        return len(self.child_components)


@dataclass(frozen=True)
class EMSTBatch:
    """All complete-graph edges sharing one exact squared weight."""

    squared_distance: Fraction
    squared_level: Fraction
    complete_edges: tuple[EMSTEdge, ...]
    selected_edges: tuple[EMSTEdge, ...]
    pre_components: tuple[Component, ...]
    post_components: tuple[Component, ...]
    multifusions: tuple[EMSTMultifusion, ...]

    def __post_init__(self) -> None:
        if self.squared_level != self.squared_distance / 4:
            raise ValueError("EMST batch level must equal squared_distance / 4")
        if not self.complete_edges:
            raise ValueError("an EMST edge batch cannot be empty")
        if any(edge.squared_distance != self.squared_distance for edge in self.complete_edges):
            raise ValueError("all complete edges in a batch must have the same weight")
        if not set(self.selected_edges) <= set(self.complete_edges):
            raise ValueError("selected EMST edges must belong to their complete batch")


@dataclass(frozen=True)
class EMSTCut:
    """Canonical exact partition at one open or closed HGP threshold."""

    squared_level: Fraction
    closed: bool
    edge_source: EdgeSource
    components: tuple[Component, ...]


@dataclass(frozen=True)
class EMSTCounters:
    """Auditable work and batch counts for one exhaustive construction."""

    point_count: int
    distance_evaluations: int
    complete_edge_count: int
    distinct_edge_weight_count: int
    equal_weight_batch_count: int
    max_equal_weight_batch_size: int
    selected_edge_count: int
    redundant_edge_count: int
    merge_batch_count: int
    merge_event_count: int
    multifusion_count: int
    max_merge_arity: int
    replay_level_count: int


@dataclass(frozen=True)
class EMSTResult:
    """Exact complete graph, one deterministic EMST, batches, and counters."""

    points: tuple[Point3, ...]
    complete_edges: tuple[EMSTEdge, ...]
    selected_edges: tuple[EMSTEdge, ...]
    batches: tuple[EMSTBatch, ...]
    total_squared_weight: Fraction
    counters: EMSTCounters

    @property
    def point_count(self) -> int:
        return len(self.points)

    @property
    def total_hgp_weight(self) -> Fraction:
        return self.total_squared_weight / 4

    @property
    def replay_levels(self) -> tuple[Fraction, ...]:
        """Return zero and every complete-graph edge level needed for replay."""

        return (Fraction(0),) + tuple(batch.squared_level for batch in self.batches)

    def cut(
        self,
        squared_level: Fraction | int,
        *,
        closed: bool = True,
        edge_source: EdgeSource = "selected_emst",
    ) -> EMSTCut:
        """Replay either the selected tree or the complete graph exactly."""

        level = Fraction(squared_level)
        if edge_source not in ("complete_graph", "selected_emst"):
            raise ValueError(f"unknown EMST cut edge source {edge_source!r}")
        vertices_active = level >= 0 if closed else level > 0
        if not vertices_active:
            return EMSTCut(level, closed, edge_source, ())

        def active(edge: EMSTEdge) -> bool:
            return edge.squared_level <= level if closed else edge.squared_level < level

        disjoint_set = _DisjointSet(self.point_count)
        edges = self.complete_edges if edge_source == "complete_graph" else self.selected_edges
        for edge in edges:
            if active(edge):
                disjoint_set.union(edge.left_id, edge.right_id)
        return EMSTCut(
            level,
            closed,
            edge_source,
            _components(disjoint_set, self.point_count),
        )


def _frozen_multifusions(
    point_count: int,
    global_dsu: _DisjointSet,
    batch_edges: tuple[EMSTEdge, ...],
) -> tuple[EMSTMultifusion, ...]:
    pre_components = _components(global_dsu, point_count)
    component_by_point = {
        point_id: component for component in pre_components for point_id in component
    }
    component_index = {component: index for index, component in enumerate(pre_components)}
    batch_dsu = _DisjointSet(len(pre_components))
    touched: set[Component] = set()
    for edge in batch_edges:
        left_component = component_by_point[edge.left_id]
        right_component = component_by_point[edge.right_id]
        if left_component == right_component:
            continue
        touched.add(left_component)
        touched.add(right_component)
        batch_dsu.union(component_index[left_component], component_index[right_component])

    grouped: dict[int, list[Component]] = {}
    for component in sorted(touched):
        grouped.setdefault(batch_dsu.find(component_index[component]), []).append(component)
    multifusions = []
    for children in grouped.values():
        canonical_children = tuple(sorted(children))
        if len(canonical_children) < 2:
            continue
        merged = tuple(
            sorted(point_id for component in canonical_children for point_id in component)
        )
        multifusions.append(EMSTMultifusion(canonical_children, merged))
    return tuple(sorted(multifusions, key=lambda fusion: fusion.child_components))


def build_exhaustive_emst(
    points: Iterable[Sequence[Scalar] | object],
) -> EMSTResult:
    """Construct the exact complete graph and a deterministic Euclidean MST."""

    exact_points = _normalize_points(points)
    point_count = len(exact_points)
    complete_edges = tuple(
        sorted(
            (
                EMSTEdge(left_id, right_id, _squared_distance(exact_points[left_id], exact_points[right_id]))
                for left_id, right_id in combinations(range(point_count), 2)
            ),
            key=lambda edge: (edge.squared_distance, edge.left_id, edge.right_id),
        )
    )

    global_dsu = _DisjointSet(point_count)
    selected_edges: list[EMSTEdge] = []
    batches: list[EMSTBatch] = []
    for squared_distance, raw_group in groupby(
        complete_edges, key=lambda edge: edge.squared_distance
    ):
        batch_edges = tuple(raw_group)
        pre_components = _components(global_dsu, point_count)
        multifusions = _frozen_multifusions(point_count, global_dsu, batch_edges)
        batch_selected = []
        for edge in batch_edges:
            if global_dsu.union(edge.left_id, edge.right_id):
                selected_edges.append(edge)
                batch_selected.append(edge)
        post_components = _components(global_dsu, point_count)
        batches.append(
            EMSTBatch(
                squared_distance=squared_distance,
                squared_level=squared_distance / 4,
                complete_edges=batch_edges,
                selected_edges=tuple(batch_selected),
                pre_components=pre_components,
                post_components=post_components,
                multifusions=multifusions,
            )
        )

    selected = tuple(selected_edges)
    total_squared_weight = sum(
        (edge.squared_distance for edge in selected), Fraction(0)
    )
    merge_events = tuple(
        fusion for batch in batches for fusion in batch.multifusions
    )
    counters = EMSTCounters(
        point_count=point_count,
        distance_evaluations=len(complete_edges),
        complete_edge_count=len(complete_edges),
        distinct_edge_weight_count=len(batches),
        equal_weight_batch_count=sum(
            len(batch.complete_edges) > 1 for batch in batches
        ),
        max_equal_weight_batch_size=max(
            (len(batch.complete_edges) for batch in batches), default=0
        ),
        selected_edge_count=len(selected),
        redundant_edge_count=len(complete_edges) - len(selected),
        merge_batch_count=sum(bool(batch.multifusions) for batch in batches),
        merge_event_count=len(merge_events),
        multifusion_count=sum(fusion.arity >= 3 for fusion in merge_events),
        max_merge_arity=max((fusion.arity for fusion in merge_events), default=0),
        replay_level_count=1 + len(batches),
    )
    return EMSTResult(
        points=exact_points,
        complete_edges=complete_edges,
        selected_edges=selected,
        batches=tuple(batches),
        total_squared_weight=total_squared_weight,
        counters=counters,
    )
