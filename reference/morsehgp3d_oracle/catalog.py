"""Exhaustive exact critical-sphere catalogue for the phase-1 CPU oracle.

The implementation deliberately enumerates every affine-independent support
of size at most four.  It accepts the first certified generic domain only:
the complete shell must equal the well-centred minimal support.  Relevant
extra-shell equalities are preserved as explicit degeneracy records instead
of being perturbed or silently discarded.
"""

from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations
from math import comb
from typing import Iterable, Sequence

from .exact import Point3, Scalar, canonicalize_points, normalize_point
from .geometry import (
    AffineDependenceError,
    Ball,
    BallClassification,
    barycentric_coordinates,
    circumball,
    classify_points,
)


def _canonical_ids(point_ids: Iterable[int], *, allow_empty: bool = True) -> tuple[int, ...]:
    identifiers = tuple(point_ids)
    if any(isinstance(point_id, bool) or not isinstance(point_id, int) for point_id in identifiers):
        raise TypeError("point identifiers must be integers")
    if any(point_id < 0 for point_id in identifiers):
        raise ValueError("point identifiers must be non-negative")
    if len(identifiers) != len(set(identifiers)):
        raise ValueError("point identifiers must be unique")
    canonical = tuple(sorted(identifiers))
    if not canonical and not allow_empty:
        raise ValueError("identifier set must be non-empty")
    return canonical


@dataclass(frozen=True, slots=True)
class CriticalSphere:
    """One generic critical centre shared by its H0 birth and saddle orders."""

    center: Point3
    squared_radius: Fraction
    minimal_support_ids: tuple[int, ...]
    interior_ids: tuple[int, ...]
    shell_ids: tuple[int, ...]
    barycentric_coordinates: tuple[Fraction, ...]
    closed_rank: int
    birth_order: int | None
    saddle_order: int | None

    def __post_init__(self) -> None:
        center = normalize_point(self.center)
        radius = Fraction(self.squared_radius)
        support = _canonical_ids(self.minimal_support_ids, allow_empty=False)
        interior = _canonical_ids(self.interior_ids)
        shell = _canonical_ids(self.shell_ids, allow_empty=False)
        barycentric = tuple(Fraction(value) for value in self.barycentric_coordinates)
        if radius < 0:
            raise ValueError("a critical squared radius cannot be negative")
        if len(support) > 4:
            raise ValueError("a 3D minimal support has at most four points")
        if not set(support) <= set(shell):
            raise ValueError("minimal support must be contained in the complete shell")
        if set(interior) & set(shell):
            raise ValueError("interior and shell identifiers must be disjoint")
        if self.closed_rank != len(interior) + len(shell):
            raise ValueError("closed_rank must equal |interior| + |shell|")
        if len(barycentric) != len(support):
            raise ValueError("one barycentric coordinate is required per support point")
        if sum(barycentric, Fraction(0)) != 1 or any(value <= 0 for value in barycentric):
            raise ValueError("a critical minimal support must be strictly well-centred")
        if self.birth_order not in (None, self.closed_rank):
            raise ValueError("birth_order must be closed_rank when present")
        expected_saddle = self.closed_rank - 1
        if self.saddle_order is not None and (
            self.closed_rank < 2 or self.saddle_order != expected_saddle
        ):
            raise ValueError("saddle_order must be closed_rank - 1 when present")
        object.__setattr__(self, "center", center)
        object.__setattr__(self, "squared_radius", radius)
        object.__setattr__(self, "minimal_support_ids", support)
        object.__setattr__(self, "interior_ids", interior)
        object.__setattr__(self, "shell_ids", shell)
        object.__setattr__(self, "barycentric_coordinates", barycentric)

    @property
    def support_ids(self) -> tuple[int, ...]:
        """Compatibility alias used by exhaustive Gabriel code."""

        return self.minimal_support_ids

    @property
    def point_ids(self) -> tuple[int, ...]:
        """Return the closed-rank label ``I union U``."""

        return tuple(sorted(self.interior_ids + self.shell_ids))

    @property
    def canonical_key(self) -> tuple[object, ...]:
        """Thread-order-independent ordering and deduplication key."""

        return (
            self.squared_radius,
            self.closed_rank,
            self.interior_ids,
            self.shell_ids,
            self.minimal_support_ids,
            self.center,
        )

    @property
    def arms(self) -> tuple[tuple[int, ...], ...]:
        """Return the strict H0 arms, one for each removed shell point."""

        if self.saddle_order is None:
            return ()
        closed_label = self.point_ids
        return tuple(
            tuple(point_id for point_id in closed_label if point_id != removed)
            for removed in self.shell_ids
        )

    def morse_index(self, order: int) -> int:
        """Return ``closed_rank - order`` at a positive order.

        The value is a Morse index only when :meth:`is_critical_at_order` is
        true.  At lower descending orders it is merely the formal rank
        difference and its local multiplicity is zero.
        """

        if isinstance(order, bool) or not isinstance(order, int):
            raise TypeError("order must be an integer")
        if order < 1:
            raise ValueError("order must be positive")
        return self.closed_rank - order

    def is_critical_at_order(self, order: int) -> bool:
        """Return whether ``|interior| < order <= closed_rank``."""

        index = self.morse_index(order)
        return 0 <= index <= len(self.shell_ids) - 1

    def local_multiplicity(self, order: int) -> int:
        """Return the Reani--Bobrowski multiplicity, or zero off-window."""

        index = self.morse_index(order)
        upper = len(self.shell_ids) - 1
        return comb(upper, index) if 0 <= index <= upper else 0


@dataclass(frozen=True, slots=True)
class CatalogDegeneracy:
    """A relevant exact equality excluded from the first generic catalogue."""

    reason: str
    ball: Ball
    interior_ids: tuple[int, ...]
    shell_ids: tuple[int, ...]

    def __post_init__(self) -> None:
        if self.reason not in {"extra_shell", "dependent_shell"}:
            raise ValueError(f"unknown catalogue degeneracy {self.reason!r}")
        interior = _canonical_ids(self.interior_ids)
        shell = _canonical_ids(self.shell_ids, allow_empty=False)
        if set(interior) & set(shell):
            raise ValueError("degeneracy interior and shell must be disjoint")
        if not set(self.ball.support_ids) <= set(shell):
            raise ValueError("degenerate support must be contained in its shell")
        object.__setattr__(self, "interior_ids", interior)
        object.__setattr__(self, "shell_ids", shell)

    @property
    def canonical_key(self) -> tuple[object, ...]:
        return (
            self.ball.squared_radius,
            self.interior_ids,
            self.shell_ids,
            len(self.ball.support_ids),
            self.ball.support_ids,
            self.ball.center,
            self.reason,
        )


@dataclass(frozen=True, slots=True)
class CatalogStatistics:
    """Exact accounting of the exhaustive support-enumeration decisions."""

    support_candidates: int
    affine_dependent_candidates: int
    non_well_centred_candidates: int
    extra_shell_candidates: int
    above_rank_candidates: int
    accepted_supports: int
    deduplicated_events: int
    point_classifications: int

    def __post_init__(self) -> None:
        values = tuple(getattr(self, field) for field in self.__dataclass_fields__)
        if any(isinstance(value, bool) or not isinstance(value, int) or value < 0 for value in values):
            raise ValueError("catalogue statistics must be non-negative integers")
        classified = (
            self.affine_dependent_candidates
            + self.non_well_centred_candidates
            + self.extra_shell_candidates
            + self.above_rank_candidates
            + self.accepted_supports
        )
        if classified != self.support_candidates:
            raise ValueError("every support candidate must have exactly one outcome")
        if self.deduplicated_events > self.accepted_supports:
            raise ValueError("deduplicated events cannot exceed accepted supports")

    @property
    def accepted_events(self) -> int:
        return self.accepted_supports - self.deduplicated_events

    @property
    def rejected_candidates(self) -> int:
        return self.support_candidates - self.accepted_events


@dataclass(frozen=True, slots=True)
class CriticalCatalog:
    """Canonical point set, effective rank bound, events and explicit failures."""

    points: tuple[Point3, ...]
    k_max: int
    k_eff: int
    s_max: int
    events: tuple[CriticalSphere, ...]
    degenerate_candidates: tuple[CatalogDegeneracy, ...] = ()
    statistics: CatalogStatistics | None = None

    def __post_init__(self) -> None:
        if not self.points:
            raise ValueError("a critical catalogue requires a non-empty point cloud")
        if tuple(sorted(self.points)) != self.points or len(set(self.points)) != len(self.points):
            raise ValueError("catalogue points must be sorted and distinct")
        if not 1 <= self.k_max <= 10:
            raise ValueError("k_max must lie in 1..10")
        if self.k_eff != min(self.k_max, len(self.points)):
            raise ValueError("k_eff is inconsistent with k_max and point count")
        if self.s_max != min(self.k_eff + 1, len(self.points)):
            raise ValueError("s_max is inconsistent with k_eff and point count")
        if tuple(sorted(self.events, key=lambda event: event.canonical_key)) != self.events:
            raise ValueError("critical events must be canonically sorted")
        if any(event.closed_rank > self.s_max for event in self.events):
            raise ValueError("catalogue contains an event above s_max")
        if tuple(
            sorted(
                self.degenerate_candidates,
                key=lambda candidate: candidate.canonical_key,
            )
        ) != self.degenerate_candidates:
            raise ValueError("catalogue degeneracies must be canonically sorted")
        if self.statistics is None:
            raise ValueError("an exhaustive critical catalogue requires statistics")
        if self.statistics.accepted_events != len(self.events):
            raise ValueError("catalogue statistics disagree with the event count")

    @property
    def relevant_gp_complete(self) -> bool:
        """The generic-position certificate is complete iff no violation exists."""

        return not self.degenerate_candidates

    def by_rank(self, rank: int) -> tuple[CriticalSphere, ...]:
        if isinstance(rank, bool) or not isinstance(rank, int):
            raise TypeError("rank must be an integer")
        return tuple(event for event in self.events if event.closed_rank == rank)


def _well_centred_coordinates(
    points: tuple[Point3, ...], ball: Ball
) -> tuple[Fraction, ...] | None:
    support_points = tuple(points[point_id] for point_id in ball.support_ids)
    coordinates = barycentric_coordinates(ball.center, support_points)
    return coordinates if all(value > 0 for value in coordinates) else None


def _critical_event(
    ball: Ball,
    classification: BallClassification,
    barycentric: tuple[Fraction, ...],
    *,
    k_eff: int,
) -> CriticalSphere:
    closed_rank = classification.closed_rank
    return CriticalSphere(
        center=ball.center,
        squared_radius=ball.squared_radius,
        minimal_support_ids=ball.support_ids,
        interior_ids=classification.interior_ids,
        shell_ids=classification.shell_ids,
        barycentric_coordinates=barycentric,
        closed_rank=closed_rank,
        birth_order=closed_rank if closed_rank <= k_eff else None,
        saddle_order=(
            closed_rank - 1 if 2 <= closed_rank <= k_eff + 1 else None
        ),
    )


def build_critical_catalog(
    points: Iterable[Sequence[Scalar] | object], k_max: int = 10
) -> CriticalCatalog:
    """Enumerate every generic critical sphere up to the effective rank bound.

    Point identifiers refer to the lexicographically sorted exact point tuple
    returned in ``CriticalCatalog.points``.  Exact extra-shell candidates that
    violate the first certified generic domain are retained separately and set
    ``relevant_gp_complete`` to false.
    """

    if isinstance(k_max, bool) or not isinstance(k_max, int):
        raise TypeError("k_max must be an integer")
    if not 1 <= k_max <= 10:
        raise ValueError("k_max must lie in 1..10")
    canonical_points = canonicalize_points(points, reject_duplicates=True)
    if not canonical_points:
        raise ValueError("the critical catalogue requires a non-empty point cloud")
    point_count = len(canonical_points)
    k_eff = min(k_max, point_count)
    s_max = min(k_eff + 1, point_count)

    events_by_identity: dict[tuple[object, ...], CriticalSphere] = {}
    degeneracies_by_identity: dict[tuple[object, ...], CatalogDegeneracy] = {}
    support_candidates = 0
    affine_dependent_candidates = 0
    non_well_centred_candidates = 0
    extra_shell_candidates = 0
    above_rank_candidates = 0
    accepted_supports = 0
    point_classifications = 0
    for support_size in range(1, min(4, point_count) + 1):
        for support_ids in combinations(range(point_count), support_size):
            support_candidates += 1
            try:
                ball = circumball(canonical_points, support_ids)
            except AffineDependenceError:
                affine_dependent_candidates += 1
                continue
            barycentric = _well_centred_coordinates(canonical_points, ball)
            if barycentric is None:
                non_well_centred_candidates += 1
                continue
            classification = classify_points(canonical_points, ball)
            point_classifications += point_count
            if classification.shell_ids != ball.support_ids:
                extra_shell_candidates += 1
                relevant_rank_without_extra_shell = (
                    len(classification.interior_ids) + len(ball.support_ids)
                )
                if relevant_rank_without_extra_shell <= s_max:
                    candidate = CatalogDegeneracy(
                        reason="extra_shell",
                        ball=ball,
                        interior_ids=classification.interior_ids,
                        shell_ids=classification.shell_ids,
                    )
                    identity = (
                        ball.center,
                        ball.squared_radius,
                        classification.interior_ids,
                        classification.shell_ids,
                    )
                    previous = degeneracies_by_identity.get(identity)
                    if previous is None or candidate.canonical_key < previous.canonical_key:
                        degeneracies_by_identity[identity] = candidate
                continue
            if classification.closed_rank > s_max:
                above_rank_candidates += 1
                continue
            accepted_supports += 1
            event = _critical_event(
                ball, classification, barycentric, k_eff=k_eff
            )
            identity = (
                event.center,
                event.squared_radius,
                event.interior_ids,
                event.shell_ids,
            )
            previous = events_by_identity.get(identity)
            if previous is None or event.canonical_key < previous.canonical_key:
                events_by_identity[identity] = event

    events = tuple(sorted(events_by_identity.values(), key=lambda event: event.canonical_key))
    degeneracies = tuple(
        sorted(
            degeneracies_by_identity.values(),
            key=lambda candidate: candidate.canonical_key,
        )
    )
    statistics = CatalogStatistics(
        support_candidates=support_candidates,
        affine_dependent_candidates=affine_dependent_candidates,
        non_well_centred_candidates=non_well_centred_candidates,
        extra_shell_candidates=extra_shell_candidates,
        above_rank_candidates=above_rank_candidates,
        accepted_supports=accepted_supports,
        deduplicated_events=accepted_supports - len(events),
        point_classifications=point_classifications,
    )
    return CriticalCatalog(
        points=canonical_points,
        k_max=k_max,
        k_eff=k_eff,
        s_max=s_max,
        events=events,
        degenerate_candidates=degeneracies,
        statistics=statistics,
    )


def enumerate_critical_spheres(
    points: Iterable[Sequence[Scalar] | object], k_max: int = 10
) -> tuple[CriticalSphere, ...]:
    """Convenience wrapper returning only accepted generic events."""

    return build_critical_catalog(points, k_max).events
