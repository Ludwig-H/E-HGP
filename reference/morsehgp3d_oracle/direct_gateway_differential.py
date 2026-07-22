"""Bounded exact differential for the Phase-10 direct gateway hypotheses.

This module is deliberately an exhaustive reference oracle.  It materializes
the complete :math:`Gamma_k` filtration only after enforcing ``n <= 14`` and
``k <= 10``.  Nothing in the production backend imports this module.

Point identifiers in results (and in ``excluded_gateway_cofaces``) refer to
the lexicographically sorted exact point cloud exposed as ``result.points``.
"""

from __future__ import annotations

from dataclasses import dataclass, fields
from enum import Enum
from fractions import Fraction
from itertools import combinations
from typing import Any, Iterable, Sequence, TypeAlias, cast

from .catalog import CriticalCatalog, build_critical_catalog
from .exact import Point3, Scalar, canonicalize_points
from .gamma import (
    GammaCoface,
    GammaFiltration,
    PointLabel,
    build_gamma_filtration,
    canonical_label,
    facets_of,
)

FacetLabel: TypeAlias = PointLabel
CofaceLabel: TypeAlias = PointLabel
FacetComponent: TypeAlias = tuple[FacetLabel, ...]
FacetPartition: TypeAlias = tuple[FacetComponent, ...]
CoveredPointPartition: TypeAlias = tuple[PointLabel, ...]


class DifferentialDecision(str, Enum):
    """Overall bounded decision; equivalence is evidence, never a proof."""

    OPEN_BOUNDED_EVIDENCE_ONLY = "open_bounded_evidence_only"
    UNSUPPORTED_DEGENERACY = "unsupported_degeneracy"
    DIRECT_ALPHABET_INSUFFICIENT = "direct_alphabet_insufficient"
    GATEWAY_GENERATOR_INSUFFICIENT = "gateway_generator_insufficient"


class DifferentialVerdict(str, Enum):
    """Verdict for one of the two independently tested hypotheses."""

    EQUIVALENT = "equivalent"
    INSUFFICIENT = "insufficient"
    UNSUPPORTED_DEGENERACY = "unsupported_degeneracy"


class DifferentialEvidenceStatus(str, Enum):
    """Scientific status of this bounded falsification run.

    A deliberately amputated generator is always ``DIAGNOSTIC_ABLATION``;
    its expected failure is not a counterexample to the unablated hypothesis.
    """

    OPEN_BOUNDED_EVIDENCE_ONLY = "open_bounded_evidence_only"
    COUNTEREXAMPLE = "counterexample"
    DIAGNOSTIC_ABLATION = "diagnostic_ablation"
    UNSUPPORTED_DEGENERACY = "unsupported_degeneracy"


class DifferentialBoundary(str, Enum):
    OPEN = "open"
    CLOSED = "closed"

    @property
    def closed(self) -> bool:
        return self is DifferentialBoundary.CLOSED


class DifferentialStage(str, Enum):
    DIRECT_ALPHABET = "direct_alphabet"
    GATEWAY_GENERATOR = "gateway_generator"


class DifferentialMismatch(str, Enum):
    FACET_ABSENT = "facet_absent"
    REFERENCE_CONNECTION_MISSING = "reference_connection_missing"
    COVERED_POINT_GENEALOGY = "covered_point_genealogy"


class DifferentialProvenance(str, Enum):
    BIRTH = "birth"
    DIRECT = "direct"
    FIRST_INCIDENCE = "first_incidence"
    RESIDUAL = "residual"


class DifferentialPathElementKind(str, Enum):
    FACET = "facet"
    COFACE = "coface"


@dataclass(frozen=True, slots=True)
class DirectGatewayGammaPathElement:
    """One typed node of a canonical alternating Gamma path."""

    kind: DifferentialPathElementKind
    point_ids: PointLabel
    provenance: tuple[DifferentialProvenance, ...]


@dataclass(frozen=True, slots=True)
class DirectGatewayGammaCheckpoint:
    """Both differential comparisons at one exact open or closed cut.

    ``covered_point_partition_equivalent`` compares only this cut.  The root
    genealogy flag is the exact prefix recurrence ``previous and current facet
    partition and current covered-point partition``; the ordered checkpoints
    therefore retain the complete history in O(L) storage and a later
    reconvergence cannot erase an earlier discrepancy.
    Complete Gamma component tuples remain transient: only their diagnostic
    equality, counts, and first missing facet are persisted here.
    """

    squared_level: Fraction
    boundary: DifferentialBoundary
    direct_alphabet_reference_components: FacetPartition
    direct_alphabet_candidate_components: FacetPartition
    direct_alphabet_partition_equivalent: bool
    gateway_reference_components: FacetPartition
    gateway_candidate_components: FacetPartition
    facet_partition_equivalent: bool
    reference_covered_point_partitions: CoveredPointPartition
    candidate_covered_point_partitions: CoveredPointPartition
    covered_point_partition_equivalent: bool
    root_genealogy_and_covered_points_equivalent: bool
    full_nontrivial_facet_coverage_equivalent_diagnostic: bool
    full_reference_nontrivial_component_count: int
    full_candidate_nontrivial_component_count: int
    first_missing_full_facet_diagnostic: FacetLabel | None

    @property
    def closed(self) -> bool:
        return self.boundary.closed


@dataclass(frozen=True, slots=True)
class DirectGatewayGammaWitness:
    """First canonical witness for one failed stage and exact cut."""

    squared_level: Fraction
    boundary: DifferentialBoundary
    stage: DifferentialStage
    mismatch: DifferentialMismatch
    first_absent_facet: FacetLabel | None
    separated_facets: tuple[FacetLabel, FacetLabel] | None
    reference_components: FacetPartition
    candidate_components: FacetPartition
    gamma_path: tuple[DirectGatewayGammaPathElement, ...]
    first_facet_outside_direct_alphabet: FacetLabel | None
    first_coface_using_outside_facet: CofaceLabel | None
    first_ungenerated_coface: CofaceLabel | None
    first_missing_covered_point_id: int | None
    first_missing_relay_facet: FacetLabel | None

    @property
    def closed(self) -> bool:
        return self.boundary.closed


@dataclass(frozen=True, slots=True)
class DirectGatewayGammaDifferentialResult:
    """Complete bounded result for one canonical cloud and one order."""

    decision: DifferentialDecision
    evidence_status: DifferentialEvidenceStatus
    scope: str
    public_status: str
    diagnostic_ablation: bool
    points: tuple[Point3, ...]
    point_count: int
    order: int
    relevant_gp: bool
    degeneracy_witnesses: tuple[tuple[str, PointLabel], ...]
    birth_facets: tuple[FacetLabel, ...]
    direct_saddle_cofaces: tuple[CofaceLabel, ...]
    direct_facets: tuple[FacetLabel, ...]
    direct_alphabet: tuple[FacetLabel, ...]
    first_incidence_cofaces: tuple[CofaceLabel, ...]
    excluded_gateway_cofaces: tuple[CofaceLabel, ...]
    generated_cofaces: tuple[CofaceLabel, ...]
    direct_alphabet_verdict: DifferentialVerdict
    gateway_generator_verdict: DifferentialVerdict
    facet_partition_equivalent: bool | None
    root_genealogy_and_covered_points_equivalent: bool | None
    cut_results: tuple[DirectGatewayGammaCheckpoint, ...]
    first_failure: DirectGatewayGammaWitness | None
    direct_alphabet_witness: DirectGatewayGammaWitness | None
    gateway_generator_witness: DirectGatewayGammaWitness | None

    @property
    def checkpoints(self) -> tuple[DirectGatewayGammaCheckpoint, ...]:
        return self.cut_results

    @property
    def witness(self) -> DirectGatewayGammaWitness | None:
        return self.first_failure

    @property
    def bounded_equivalent(self) -> bool:
        return (
            self.decision is DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY
            and self.evidence_status
            is DifferentialEvidenceStatus.OPEN_BOUNDED_EVIDENCE_ONLY
            and not self.diagnostic_ablation
        )


class _DisjointSet:
    def __init__(self, values: Iterable[FacetLabel]) -> None:
        self._parent = {value: value for value in values}

    def find(self, value: FacetLabel) -> FacetLabel:
        parent = self._parent[value]
        if parent != value:
            self._parent[value] = self.find(parent)
        return self._parent[value]

    def union(self, left: FacetLabel, right: FacetLabel) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        if left_root < right_root:
            self._parent[right_root] = left_root
        else:
            self._parent[left_root] = right_root


def _active(value: Fraction, level: Fraction, boundary: DifferentialBoundary) -> bool:
    return value <= level if boundary.closed else value < level


def _components(
    facets: Iterable[FacetLabel],
    relations: Iterable[Sequence[FacetLabel]],
) -> FacetPartition:
    nodes = set(facets)
    disjoint_set = _DisjointSet(nodes)
    for raw_relation in relations:
        relation = tuple(sorted(facet for facet in raw_relation if facet in nodes))
        if len(relation) < 2:
            continue
        for facet in relation[1:]:
            disjoint_set.union(relation[0], facet)
    grouped: dict[FacetLabel, list[FacetLabel]] = {}
    for facet in sorted(nodes):
        grouped.setdefault(disjoint_set.find(facet), []).append(facet)
    return tuple(sorted(tuple(group) for group in grouped.values()))


def _project_components(
    components: FacetPartition, alphabet: frozenset[FacetLabel]
) -> FacetPartition:
    projected = tuple(
        tuple(facet for facet in component if facet in alphabet)
        for component in components
    )
    return tuple(sorted(component for component in projected if component))


def _project_components_with_full_coverage(
    components: FacetPartition, alphabet: frozenset[FacetLabel]
) -> tuple[FacetPartition, CoveredPointPartition]:
    """Project facets onto V while retaining each complete component's points."""

    records = []
    for component in components:
        projected = tuple(facet for facet in component if facet in alphabet)
        if not projected:
            continue
        covered = tuple(sorted({point_id for facet in component for point_id in facet}))
        records.append((projected, covered))
    records.sort(key=lambda record: record[0])
    return (
        tuple(record[0] for record in records),
        tuple(record[1] for record in records),
    )


def _full_component_by_projection(
    components: FacetPartition, alphabet: frozenset[FacetLabel]
) -> dict[FacetComponent, FacetComponent]:
    return {
        tuple(facet for facet in component if facet in alphabet): component
        for component in components
        if any(facet in alphabet for facet in component)
    }


def _coverage_witness_data(
    *,
    reference_components: FacetPartition,
    candidate_components: FacetPartition,
    full_reference_components: FacetPartition,
    full_candidate_components: FacetPartition,
    reference_covered_points: CoveredPointPartition,
    candidate_covered_points: CoveredPointPartition,
    alphabet: frozenset[FacetLabel],
) -> tuple[int | None, FacetLabel | None, tuple[FacetLabel, FacetLabel] | None]:
    """Find the first missing covered point and a canonical Gamma relay path."""

    reference_full = _full_component_by_projection(full_reference_components, alphabet)
    candidate_full = _full_component_by_projection(full_candidate_components, alphabet)
    candidate_coverage = {
        component: covered
        for component, covered in zip(
            candidate_components, candidate_covered_points, strict=True
        )
    }
    for projected, reference_points in zip(
        reference_components, reference_covered_points, strict=True
    ):
        candidate_points = candidate_coverage.get(projected, ())
        missing_points = tuple(sorted(set(reference_points) - set(candidate_points)))
        if not missing_points:
            continue
        missing_point = missing_points[0]
        reference_full_component = reference_full[projected]
        candidate_full_component = set(candidate_full.get(projected, ()))
        relay = next(
            (
                facet
                for facet in reference_full_component
                if missing_point in facet and facet not in candidate_full_component
            ),
            None,
        )
        if relay is None:
            relay = next(
                (facet for facet in reference_full_component if missing_point in facet),
                None,
            )
        endpoints = (projected[0], relay) if relay is not None else None
        return missing_point, relay, endpoints
    return None, None, None


def _component_by_facet(
    components: FacetPartition,
) -> dict[FacetLabel, FacetComponent]:
    return {facet: component for component in components for facet in component}


def _candidate_refines_reference(
    reference: FacetPartition, candidate: FacetPartition
) -> bool:
    """Return whether both use the same facets and candidate only splits roots."""

    reference_root_by_facet = {
        facet: root_index
        for root_index, component in enumerate(reference)
        for facet in component
    }
    candidate_facets = {facet for component in candidate for facet in component}
    if candidate_facets != set(reference_root_by_facet):
        return False
    return all(
        len({reference_root_by_facet[facet] for facet in component}) == 1
        for component in candidate
    )


def _partition_mismatch(reference: FacetPartition, candidate: FacetPartition) -> tuple[
    DifferentialMismatch | None,
    FacetLabel | None,
    tuple[FacetLabel, FacetLabel] | None,
]:
    reference_by_facet = _component_by_facet(reference)
    candidate_by_facet = _component_by_facet(candidate)
    reference_facets = set(reference_by_facet)
    candidate_facets = set(candidate_by_facet)
    missing = sorted(reference_facets - candidate_facets)
    if missing:
        return DifferentialMismatch.FACET_ABSENT, missing[0], None
    extra = sorted(candidate_facets - reference_facets)
    if extra:
        raise AssertionError("a projected candidate introduced a non-Gamma facet")

    labels = tuple(sorted(reference_facets))
    for left, right in combinations(labels, 2):
        reference_same = reference_by_facet[left] == reference_by_facet[right]
        candidate_same = candidate_by_facet[left] == candidate_by_facet[right]
        if reference_same and not candidate_same:
            return (
                DifferentialMismatch.REFERENCE_CONNECTION_MISSING,
                None,
                (left, right),
            )
        if candidate_same and not reference_same:
            raise AssertionError("a projected candidate connected two Gamma roots")
    return None, None, None


def _choose_path_endpoints(
    *,
    first_absent: FacetLabel | None,
    separated: tuple[FacetLabel, FacetLabel] | None,
    reference_components: FacetPartition,
) -> tuple[FacetLabel, FacetLabel] | None:
    if separated is not None:
        return separated
    if first_absent is None:
        return None
    for component in reference_components:
        if first_absent not in component:
            continue
        alternatives = tuple(facet for facet in component if facet != first_absent)
        if alternatives:
            return first_absent, alternatives[0]
    return None


def _canonical_gamma_path(
    start: FacetLabel,
    finish: FacetLabel,
    cofaces: Sequence[GammaCoface],
) -> tuple[tuple[DifferentialPathElementKind, PointLabel], ...]:
    facet_to_cofaces: dict[FacetLabel, list[CofaceLabel]] = {}
    coface_to_facets: dict[CofaceLabel, tuple[FacetLabel, ...]] = {}
    for coface in sorted(cofaces, key=lambda item: item.point_ids):
        coface_to_facets[coface.point_ids] = coface.facet_point_ids
        for facet in coface.facet_point_ids:
            facet_to_cofaces.setdefault(facet, []).append(coface.point_ids)

    start_token = (DifferentialPathElementKind.FACET, start)
    finish_token = (DifferentialPathElementKind.FACET, finish)
    queue = [start_token]
    queue_offset = 0
    parent: dict[
        tuple[DifferentialPathElementKind, PointLabel],
        tuple[DifferentialPathElementKind, PointLabel] | None,
    ] = {start_token: None}
    while queue_offset < len(queue):
        token = queue[queue_offset]
        queue_offset += 1
        if token == finish_token:
            break
        kind, label = token
        if kind is DifferentialPathElementKind.FACET:
            neighbors = tuple(
                (DifferentialPathElementKind.COFACE, coface)
                for coface in sorted(facet_to_cofaces.get(label, ()))
            )
        else:
            neighbors = tuple(
                (DifferentialPathElementKind.FACET, facet)
                for facet in coface_to_facets[label]
            )
        for neighbor in neighbors:
            if neighbor in parent:
                continue
            parent[neighbor] = token
            queue.append(neighbor)

    if finish_token not in parent:
        return ()
    reverse_path = []
    cursor: tuple[DifferentialPathElementKind, PointLabel] | None = finish_token
    while cursor is not None:
        reverse_path.append(cursor)
        cursor = parent[cursor]
    return tuple(reversed(reverse_path))


def _element_provenance(
    kind: DifferentialPathElementKind,
    label: PointLabel,
    *,
    births: frozenset[FacetLabel],
    direct_facets: frozenset[FacetLabel],
    direct_cofaces: frozenset[CofaceLabel],
    first_incidence_cofaces: frozenset[CofaceLabel],
    first_incidence_facets: frozenset[FacetLabel],
) -> tuple[DifferentialProvenance, ...]:
    values = []
    if kind is DifferentialPathElementKind.FACET:
        if label in births:
            values.append(DifferentialProvenance.BIRTH)
        if label in direct_facets:
            values.append(DifferentialProvenance.DIRECT)
        if label in first_incidence_facets:
            values.append(DifferentialProvenance.FIRST_INCIDENCE)
    else:
        if label in direct_cofaces:
            values.append(DifferentialProvenance.DIRECT)
        if label in first_incidence_cofaces:
            values.append(DifferentialProvenance.FIRST_INCIDENCE)
    return tuple(values) if values else (DifferentialProvenance.RESIDUAL,)


def _make_witness(
    *,
    level: Fraction,
    boundary: DifferentialBoundary,
    stage: DifferentialStage,
    mismatch: DifferentialMismatch,
    first_absent: FacetLabel | None,
    separated: tuple[FacetLabel, FacetLabel] | None,
    reference_components: FacetPartition,
    candidate_components: FacetPartition,
    active_reference_cofaces: Sequence[GammaCoface],
    alphabet: frozenset[FacetLabel],
    generated_cofaces: frozenset[CofaceLabel],
    births: frozenset[FacetLabel],
    direct_facets: frozenset[FacetLabel],
    direct_cofaces: frozenset[CofaceLabel],
    first_incidence_cofaces: frozenset[CofaceLabel],
    first_incidence_facets: frozenset[FacetLabel],
    coverage_path_endpoints: tuple[FacetLabel, FacetLabel] | None = None,
    first_missing_covered_point_id: int | None = None,
    first_missing_relay_facet: FacetLabel | None = None,
) -> DirectGatewayGammaWitness:
    endpoints = coverage_path_endpoints or _choose_path_endpoints(
        first_absent=first_absent,
        separated=separated,
        reference_components=reference_components,
    )
    raw_path = (
        _canonical_gamma_path(endpoints[0], endpoints[1], active_reference_cofaces)
        if endpoints is not None
        else ()
    )
    path = tuple(
        DirectGatewayGammaPathElement(
            kind=kind,
            point_ids=label,
            provenance=_element_provenance(
                kind,
                label,
                births=births,
                direct_facets=direct_facets,
                direct_cofaces=direct_cofaces,
                first_incidence_cofaces=first_incidence_cofaces,
                first_incidence_facets=first_incidence_facets,
            ),
        )
        for kind, label in raw_path
    )
    outside_index = next(
        (
            index
            for index, item in enumerate(path[1:-1], start=1)
            if item.kind is DifferentialPathElementKind.FACET
            and item.point_ids not in alphabet
        ),
        None,
    )
    first_outside = path[outside_index].point_ids if outside_index is not None else None
    first_coface_using_outside = None
    if outside_index is not None:
        for neighbor_index in (outside_index - 1, outside_index + 1):
            if not 0 <= neighbor_index < len(path):
                continue
            neighbor = path[neighbor_index]
            if neighbor.kind is DifferentialPathElementKind.COFACE:
                first_coface_using_outside = neighbor.point_ids
                break
    first_ungenerated = next(
        (
            item.point_ids
            for item in path
            if item.kind is DifferentialPathElementKind.COFACE
            and item.point_ids not in generated_cofaces
        ),
        None,
    )
    return DirectGatewayGammaWitness(
        squared_level=level,
        boundary=boundary,
        stage=stage,
        mismatch=mismatch,
        first_absent_facet=first_absent,
        separated_facets=separated,
        reference_components=reference_components,
        candidate_components=candidate_components,
        gamma_path=path,
        first_facet_outside_direct_alphabet=first_outside,
        first_coface_using_outside_facet=first_coface_using_outside,
        first_ungenerated_coface=first_ungenerated,
        first_missing_covered_point_id=first_missing_covered_point_id,
        first_missing_relay_facet=first_missing_relay_facet,
    )


def _canonical_exclusions(
    raw_exclusions: Iterable[Iterable[int]], *, point_count: int, order: int
) -> tuple[CofaceLabel, ...]:
    exclusions = []
    for raw_label in raw_exclusions:
        label = canonical_label(raw_label, size=order + 1)
        if label and label[-1] >= point_count:
            raise ValueError(
                "excluded gateway coface contains an unknown point identifier"
            )
        exclusions.append(label)
    return tuple(sorted(set(exclusions)))


def _critical_sets(catalog: CriticalCatalog, order: int) -> tuple[
    tuple[FacetLabel, ...],
    tuple[CofaceLabel, ...],
    tuple[FacetLabel, ...],
]:
    births = tuple(
        sorted(
            event.point_ids for event in catalog.events if event.birth_order == order
        )
    )
    direct_cofaces = tuple(
        sorted(
            event.point_ids for event in catalog.events if event.saddle_order == order
        )
    )
    direct_facets = tuple(
        sorted({facet for coface in direct_cofaces for facet in facets_of(coface)})
    )
    return births, direct_cofaces, direct_facets


def _first_incidence_cofaces(
    filtration: GammaFiltration, direct_facets: Sequence[FacetLabel]
) -> tuple[CofaceLabel, ...]:
    cofaces_by_facet: dict[FacetLabel, list[GammaCoface]] = {}
    for coface in filtration.cofaces:
        for facet in coface.facet_point_ids:
            cofaces_by_facet.setdefault(facet, []).append(coface)
    minimizers: set[CofaceLabel] = set()
    for facet in direct_facets:
        incident = cofaces_by_facet.get(facet, ())
        if not incident:
            raise AssertionError("a direct deletion facet has no one-point coface")
        minimum = min(coface.squared_level for coface in incident)
        minimizers.update(
            coface.point_ids for coface in incident if coface.squared_level == minimum
        )
    return tuple(sorted(minimizers))


def _unsupported_result(
    *,
    points: tuple[Point3, ...],
    order: int,
    catalog: CriticalCatalog,
    exclusions: tuple[CofaceLabel, ...],
) -> DirectGatewayGammaDifferentialResult:
    return DirectGatewayGammaDifferentialResult(
        decision=DifferentialDecision.UNSUPPORTED_DEGENERACY,
        evidence_status=DifferentialEvidenceStatus.UNSUPPORTED_DEGENERACY,
        scope="oracle_only",
        public_status="not_claimed",
        diagnostic_ablation=bool(exclusions),
        points=points,
        point_count=len(points),
        order=order,
        relevant_gp=False,
        degeneracy_witnesses=tuple(
            (candidate.reason, candidate.shell_ids)
            for candidate in catalog.degenerate_candidates
        ),
        birth_facets=(),
        direct_saddle_cofaces=(),
        direct_facets=(),
        direct_alphabet=(),
        first_incidence_cofaces=(),
        excluded_gateway_cofaces=exclusions,
        generated_cofaces=(),
        direct_alphabet_verdict=DifferentialVerdict.UNSUPPORTED_DEGENERACY,
        gateway_generator_verdict=DifferentialVerdict.UNSUPPORTED_DEGENERACY,
        facet_partition_equivalent=None,
        root_genealogy_and_covered_points_equivalent=None,
        cut_results=(),
        first_failure=None,
        direct_alphabet_witness=None,
        gateway_generator_witness=None,
    )


def build_direct_gateway_gamma_differential(
    points: Iterable[Sequence[Scalar] | object],
    order: int,
    *,
    excluded_gateway_cofaces: Iterable[Iterable[int]] = (),
) -> DirectGatewayGammaDifferentialResult:
    """Compare the two direct-gateway hypotheses with bounded exact Gamma.

    The structural domain is checked before constructing the catalogue or
    Gamma: ``3 <= n <= 14`` and ``2 <= order <= min(10, n - 1)``.  A RelevantGP
    violation is a returned ``UNSUPPORTED_DEGENERACY`` decision, not a partial
    comparison.  Exclusions are a diagnostic ablation of first-incidence
    cofaces only; a coface that is also a direct saddle remains generated.
    Any non-empty exclusion forces ``evidence_status=DIAGNOSTIC_ABLATION`` even
    though ``decision`` still reports the generator that was actually compared.

    Normative generator equivalence is intentionally projected onto
    ``B_k union F_dir``.  Selected cofaces may use all their deletion facets as
    transient relays.  Full-facet equality is exposed only as a diagnostic and
    cannot turn the normative verdict into a failure.
    """

    raw_points = tuple(points)
    point_count = len(raw_points)
    if not 3 <= point_count <= 14:
        raise ValueError("the direct gateway differential requires 3 <= n <= 14")
    if isinstance(order, bool) or not isinstance(order, int):
        raise TypeError("order must be an integer")
    if not 2 <= order <= min(10, point_count - 1):
        raise ValueError("order must lie in 2..min(10, n - 1)")
    exclusions = _canonical_exclusions(
        excluded_gateway_cofaces, point_count=point_count, order=order
    )

    exact_points = canonicalize_points(raw_points, reject_duplicates=True)
    catalog = build_critical_catalog(exact_points, order)
    if catalog.points != exact_points:
        raise AssertionError("critical catalogue changed canonical point identifiers")
    if not catalog.relevant_gp_complete:
        return _unsupported_result(
            points=exact_points,
            order=order,
            catalog=catalog,
            exclusions=exclusions,
        )

    filtration = build_gamma_filtration(exact_points, order)
    if (filtration.point_count, filtration.order) != (point_count, order):
        raise AssertionError("Gamma filtration metadata disagrees with the request")
    births_tuple, direct_cofaces_tuple, direct_facets_tuple = _critical_sets(
        catalog, order
    )
    gamma_facets = {facet.point_ids for facet in filtration.facets}
    gamma_cofaces = {coface.point_ids for coface in filtration.cofaces}
    if not (set(births_tuple) | set(direct_facets_tuple)) <= gamma_facets:
        raise AssertionError("critical catalogue facets are absent from Gamma")
    if not set(direct_cofaces_tuple) <= gamma_cofaces:
        raise AssertionError("critical catalogue cofaces are absent from Gamma")
    alphabet_tuple = tuple(sorted(set(births_tuple) | set(direct_facets_tuple)))
    first_incidence_tuple = _first_incidence_cofaces(filtration, direct_facets_tuple)
    unknown_exclusions = set(exclusions) - set(first_incidence_tuple)
    if unknown_exclusions:
        first_unknown = min(unknown_exclusions)
        raise ValueError(
            "excluded gateway coface is not a first-incidence minimizer: "
            f"{first_unknown!r}"
        )
    generated_tuple = tuple(
        sorted(
            set(direct_cofaces_tuple) | (set(first_incidence_tuple) - set(exclusions))
        )
    )

    births = frozenset(births_tuple)
    direct_cofaces = frozenset(direct_cofaces_tuple)
    direct_facets = frozenset(direct_facets_tuple)
    alphabet = frozenset(alphabet_tuple)
    first_incidence_cofaces = frozenset(first_incidence_tuple)
    generated_cofaces = frozenset(generated_tuple)
    first_incidence_facets = frozenset(
        facet
        for coface in filtration.cofaces
        if coface.point_ids in first_incidence_cofaces
        for facet in coface.facet_point_ids
    )
    generated_records = tuple(
        coface for coface in filtration.cofaces if coface.point_ids in generated_cofaces
    )
    if (
        not first_incidence_cofaces <= gamma_cofaces
        or not generated_cofaces <= gamma_cofaces
        or len(generated_records) != len(generated_cofaces)
    ):
        raise AssertionError("generated gateway cofaces are absent from Gamma")

    checkpoints = []
    direct_witness = None
    generator_witness = None
    genealogy_equivalent = True

    for level in filtration.critical_levels:
        for boundary in (DifferentialBoundary.OPEN, DifferentialBoundary.CLOSED):
            active_facets = frozenset(
                facet.point_ids
                for facet in filtration.facets
                if _active(facet.squared_level, level, boundary)
            )
            active_reference_cofaces = tuple(
                coface
                for coface in filtration.cofaces
                if _active(coface.squared_level, level, boundary)
            )

            full_gamma_cut = filtration.cut(
                level, closed=boundary.closed, graph_kind="gamma"
            )
            full_reference = tuple(
                component.facet_point_ids for component in full_gamma_cut.components
            )
            direct_reference = _project_components(full_reference, alphabet)
            active_alphabet = active_facets & alphabet
            direct_candidate = _components(
                active_alphabet,
                (
                    tuple(
                        facet
                        for facet in coface.facet_point_ids
                        if facet in active_alphabet
                    )
                    for coface in active_reference_cofaces
                ),
            )

            active_generated = tuple(
                coface
                for coface in generated_records
                if _active(coface.squared_level, level, boundary)
            )
            candidate_facets = set(active_alphabet)
            candidate_facets.update(
                facet for coface in active_generated for facet in coface.facet_point_ids
            )
            full_candidate = _components(
                candidate_facets,
                (coface.facet_point_ids for coface in active_generated),
            )
            gateway_reference, reference_points = (
                _project_components_with_full_coverage(full_reference, alphabet)
            )
            gateway_candidate, candidate_points = (
                _project_components_with_full_coverage(full_candidate, alphabet)
            )
            direct_equivalent = direct_reference == direct_candidate
            facet_equivalent = gateway_reference == gateway_candidate
            if not _candidate_refines_reference(
                direct_reference, direct_candidate
            ) or not _candidate_refines_reference(gateway_reference, gateway_candidate):
                raise AssertionError(
                    "a generated candidate must refine its exhaustive Gamma projection"
                )
            current_point_equivalent = reference_points == candidate_points
            genealogy_equivalent = (
                genealogy_equivalent and facet_equivalent and current_point_equivalent
            )
            nontrivial_reference = tuple(
                component for component in full_reference if len(component) > 1
            )
            nontrivial_candidate = tuple(
                component for component in full_candidate if len(component) > 1
            )
            candidate_full_facets = {
                facet for component in nontrivial_candidate for facet in component
            }
            first_missing_full_facet = next(
                (
                    facet
                    for component in nontrivial_reference
                    for facet in component
                    if facet not in candidate_full_facets
                ),
                None,
            )
            checkpoints.append(
                DirectGatewayGammaCheckpoint(
                    squared_level=level,
                    boundary=boundary,
                    direct_alphabet_reference_components=direct_reference,
                    direct_alphabet_candidate_components=direct_candidate,
                    direct_alphabet_partition_equivalent=direct_equivalent,
                    gateway_reference_components=gateway_reference,
                    gateway_candidate_components=gateway_candidate,
                    facet_partition_equivalent=facet_equivalent,
                    reference_covered_point_partitions=reference_points,
                    candidate_covered_point_partitions=candidate_points,
                    covered_point_partition_equivalent=current_point_equivalent,
                    root_genealogy_and_covered_points_equivalent=genealogy_equivalent,
                    full_nontrivial_facet_coverage_equivalent_diagnostic=(
                        nontrivial_reference == nontrivial_candidate
                    ),
                    full_reference_nontrivial_component_count=len(nontrivial_reference),
                    full_candidate_nontrivial_component_count=len(nontrivial_candidate),
                    first_missing_full_facet_diagnostic=first_missing_full_facet,
                )
            )

            if not direct_equivalent and direct_witness is None:
                mismatch, first_absent, separated = _partition_mismatch(
                    direct_reference, direct_candidate
                )
                if mismatch is None:
                    raise AssertionError("unequal direct partitions need a witness")
                direct_witness = _make_witness(
                    level=level,
                    boundary=boundary,
                    stage=DifferentialStage.DIRECT_ALPHABET,
                    mismatch=mismatch,
                    first_absent=first_absent,
                    separated=separated,
                    reference_components=direct_reference,
                    candidate_components=direct_candidate,
                    active_reference_cofaces=active_reference_cofaces,
                    alphabet=alphabet,
                    generated_cofaces=generated_cofaces,
                    births=births,
                    direct_facets=direct_facets,
                    direct_cofaces=direct_cofaces,
                    first_incidence_cofaces=first_incidence_cofaces,
                    first_incidence_facets=first_incidence_facets,
                )

            if (
                not facet_equivalent or not current_point_equivalent
            ) and generator_witness is None:
                mismatch, first_absent, separated = _partition_mismatch(
                    gateway_reference, gateway_candidate
                )
                missing_point = None
                missing_relay = None
                coverage_endpoints = None
                if mismatch is None:
                    mismatch = DifferentialMismatch.COVERED_POINT_GENEALOGY
                    missing_point, missing_relay, coverage_endpoints = (
                        _coverage_witness_data(
                            reference_components=gateway_reference,
                            candidate_components=gateway_candidate,
                            full_reference_components=full_reference,
                            full_candidate_components=full_candidate,
                            reference_covered_points=reference_points,
                            candidate_covered_points=candidate_points,
                            alphabet=alphabet,
                        )
                    )
                generator_witness = _make_witness(
                    level=level,
                    boundary=boundary,
                    stage=DifferentialStage.GATEWAY_GENERATOR,
                    mismatch=mismatch,
                    first_absent=first_absent,
                    separated=separated,
                    reference_components=gateway_reference,
                    candidate_components=gateway_candidate,
                    active_reference_cofaces=active_reference_cofaces,
                    alphabet=alphabet,
                    generated_cofaces=generated_cofaces,
                    births=births,
                    direct_facets=direct_facets,
                    direct_cofaces=direct_cofaces,
                    first_incidence_cofaces=first_incidence_cofaces,
                    first_incidence_facets=first_incidence_facets,
                    coverage_path_endpoints=coverage_endpoints,
                    first_missing_covered_point_id=missing_point,
                    first_missing_relay_facet=missing_relay,
                )

    all_facet_equivalent = all(
        checkpoint.facet_partition_equivalent for checkpoint in checkpoints
    )
    all_point_equivalent = all(
        checkpoint.root_genealogy_and_covered_points_equivalent
        for checkpoint in checkpoints
    )
    if direct_witness is not None:
        decision = DifferentialDecision.DIRECT_ALPHABET_INSUFFICIENT
    elif generator_witness is not None:
        decision = DifferentialDecision.GATEWAY_GENERATOR_INSUFFICIENT
    else:
        decision = DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY
    evidence_status = (
        DifferentialEvidenceStatus.DIAGNOSTIC_ABLATION
        if exclusions
        else (
            DifferentialEvidenceStatus.OPEN_BOUNDED_EVIDENCE_ONLY
            if decision is DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY
            else DifferentialEvidenceStatus.COUNTEREXAMPLE
        )
    )

    direct_verdict = (
        DifferentialVerdict.INSUFFICIENT
        if direct_witness is not None
        else DifferentialVerdict.EQUIVALENT
    )
    generator_verdict = (
        DifferentialVerdict.INSUFFICIENT
        if generator_witness is not None
        else DifferentialVerdict.EQUIVALENT
    )
    failures = tuple(
        witness
        for witness in (direct_witness, generator_witness)
        if witness is not None
    )
    first_failure = (
        min(
            failures,
            key=lambda witness: (
                filtration.critical_levels.index(witness.squared_level),
                0 if witness.boundary is DifferentialBoundary.OPEN else 1,
                0 if witness.stage is DifferentialStage.DIRECT_ALPHABET else 1,
            ),
        )
        if failures
        else None
    )

    return DirectGatewayGammaDifferentialResult(
        decision=decision,
        evidence_status=evidence_status,
        scope="oracle_only",
        public_status="not_claimed",
        diagnostic_ablation=bool(exclusions),
        points=exact_points,
        point_count=point_count,
        order=order,
        relevant_gp=True,
        degeneracy_witnesses=(),
        birth_facets=births_tuple,
        direct_saddle_cofaces=direct_cofaces_tuple,
        direct_facets=direct_facets_tuple,
        direct_alphabet=alphabet_tuple,
        first_incidence_cofaces=first_incidence_tuple,
        excluded_gateway_cofaces=exclusions,
        generated_cofaces=generated_tuple,
        direct_alphabet_verdict=direct_verdict,
        gateway_generator_verdict=generator_verdict,
        facet_partition_equivalent=all_facet_equivalent,
        root_genealogy_and_covered_points_equivalent=all_point_equivalent,
        cut_results=tuple(checkpoints),
        first_failure=first_failure,
        direct_alphabet_witness=direct_witness,
        gateway_generator_witness=generator_witness,
    )


_STRICT_RESULT_DATACLASSES = (
    DirectGatewayGammaPathElement,
    DirectGatewayGammaCheckpoint,
    DirectGatewayGammaWitness,
    DirectGatewayGammaDifferentialResult,
)


def _strict_result_value_equal(expected: object, observed: object) -> bool:
    """Compare an oracle result without dispatching to hostile ``__eq__`` hooks."""

    if type(observed) is not type(expected):
        return False
    if type(expected) in _STRICT_RESULT_DATACLASSES:
        expected_dataclass = cast(Any, expected)
        observed_dataclass = cast(Any, observed)
        return all(
            _strict_result_value_equal(
                getattr(expected_dataclass, field.name),
                getattr(observed_dataclass, field.name),
            )
            for field in fields(expected_dataclass)
        )
    if type(expected) is tuple:
        expected_tuple = cast(tuple[object, ...], expected)
        observed_tuple = cast(tuple[object, ...], observed)
        return len(expected_tuple) == len(observed_tuple) and all(
            _strict_result_value_equal(expected_item, observed_item)
            for expected_item, observed_item in zip(
                expected_tuple, observed_tuple, strict=True
            )
        )
    if isinstance(expected, Enum):
        return expected is observed
    if type(expected) is Fraction:
        expected_fraction = cast(Fraction, expected)
        observed_fraction = cast(Fraction, observed)
        return (
            expected_fraction.numerator == observed_fraction.numerator
            and expected_fraction.denominator == observed_fraction.denominator
        )
    if expected is None:
        return True
    if type(expected) in (bool, int, str):
        return expected == observed
    return False


def verify_direct_gateway_gamma_differential(
    points: Iterable[Sequence[Scalar] | object],
    order: int,
    observed: object,
    *,
    excluded_gateway_cofaces: Iterable[Iterable[int]] = (),
) -> bool:
    """Freshly rebuild and compare a bounded differential result.

    Trusted-input domain errors deliberately propagate from the builder.  An
    object of the wrong recursive type, or any frozen-dataclass copy with
    mutated witness or checkpoint content, fails closed.  The comparison never
    delegates to an untrusted field's ``__eq__`` implementation.
    """

    expected = build_direct_gateway_gamma_differential(
        points,
        order,
        excluded_gateway_cofaces=excluded_gateway_cofaces,
    )
    try:
        return _strict_result_value_equal(expected, observed)
    except (AttributeError, TypeError, ValueError):
        return False


__all__ = [
    "DifferentialBoundary",
    "DifferentialDecision",
    "DifferentialEvidenceStatus",
    "DifferentialMismatch",
    "DifferentialPathElementKind",
    "DifferentialProvenance",
    "DifferentialStage",
    "DifferentialVerdict",
    "DirectGatewayGammaCheckpoint",
    "DirectGatewayGammaDifferentialResult",
    "DirectGatewayGammaPathElement",
    "DirectGatewayGammaWitness",
    "build_direct_gateway_gamma_differential",
    "verify_direct_gateway_gamma_differential",
]
