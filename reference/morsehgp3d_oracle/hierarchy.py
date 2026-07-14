"""Semantic forests and vertical maps for the exhaustive :math:`Gamma_k` oracle."""

from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from typing import Literal, Sequence, TypeAlias

from .gamma import (
    BallFunction,
    GabrielHyperedge,
    GammaComponent,
    GammaCut,
    GammaFiltration,
    PointLabel,
    build_gamma_filtration,
    facets_of,
)


Profile: TypeAlias = Literal["full_pi0", "hgp_reduced"]


@dataclass(frozen=True)
class ForestNode:
    """A genuine birth or simultaneous multifusion, never a q=1 growth."""

    node_id: str
    order: int
    kind: Literal["birth", "merge"]
    squared_level: Fraction
    child_ids: tuple[str, ...]
    covered_point_ids: PointLabel
    source_point_labels: tuple[PointLabel, ...]


@dataclass(frozen=True)
class ForestComponent:
    """Canonical replay state of one active forest root."""

    root_id: str
    facet_point_ids: tuple[PointLabel, ...]
    covered_point_ids: PointLabel

    @property
    def semantic_key(self) -> tuple[PointLabel, ...]:
        return self.facet_point_ids


@dataclass(frozen=True)
class CoverageDelta:
    """Facets and observations first added to one root by a complete batch."""

    order: int
    squared_level: Fraction
    root_id: str
    added_facet_point_ids: tuple[PointLabel, ...]
    added_point_ids: PointLabel


@dataclass(frozen=True)
class BatchIncidence:
    """Frozen pre-lot root of one incident facet, or ``None`` if simultaneous."""

    simplex_point_ids: PointLabel
    facet_point_ids: PointLabel
    pre_lot_root_id: str | None


@dataclass(frozen=True)
class ForestBatch:
    """One exact-level contraction with all relations applied simultaneously."""

    order: int
    squared_level: Fraction
    activated_facet_point_ids: tuple[PointLabel, ...]
    relation_simplex_point_ids: tuple[PointLabel, ...]
    incidences: tuple[BatchIncidence, ...]
    pre_components: tuple[ForestComponent, ...]
    post_components: tuple[ForestComponent, ...]
    created_node_ids: tuple[str, ...]
    coverage_deltas: tuple[CoverageDelta, ...]


@dataclass(frozen=True)
class ForestCut:
    order: int
    profile: Profile
    squared_level: Fraction
    closed: bool
    components: tuple[ForestComponent, ...]

    @property
    def component_by_facet(self) -> dict[PointLabel, ForestComponent]:
        return {
            facet: component
            for component in self.components
            for facet in component.facet_point_ids
        }


@dataclass(frozen=True)
class MergeForest:
    """Compressed hierarchy plus enough batch state to replay every cut."""

    order: int
    profile: Profile
    nodes: tuple[ForestNode, ...]
    root_ids: tuple[str, ...]
    batches: tuple[ForestBatch, ...]
    coverage_log: tuple[CoverageDelta, ...]

    def cut(self, squared_level: Fraction, *, closed: bool = True) -> ForestCut:
        level = Fraction(squared_level)

        def included(batch: ForestBatch) -> bool:
            return (
                batch.squared_level <= level
                if closed
                else batch.squared_level < level
            )

        eligible = [batch for batch in self.batches if included(batch)]
        components = eligible[-1].post_components if eligible else ()
        return ForestCut(self.order, self.profile, level, closed, components)

    @property
    def critical_levels(self) -> tuple[Fraction, ...]:
        return tuple(batch.squared_level for batch in self.batches)


@dataclass(frozen=True)
class VerticalAssignment:
    source_component_facets: tuple[PointLabel, ...]
    target_component_facets: tuple[PointLabel, ...]


@dataclass(frozen=True)
class VerticalMapAtCut:
    source_order: int
    target_order: int
    squared_level: Fraction
    closed: bool
    assignments: tuple[VerticalAssignment, ...]

    @property
    def assignment_by_source(self) -> dict[tuple[PointLabel, ...], tuple[PointLabel, ...]]:
        return {
            assignment.source_component_facets: assignment.target_component_facets
            for assignment in self.assignments
        }


@dataclass(frozen=True)
class NaturalityFailure:
    earlier_level: Fraction
    earlier_closed: bool
    later_level: Fraction
    later_closed: bool
    source_component_facets: tuple[PointLabel, ...]
    lower_then_vertical: tuple[PointLabel, ...]
    vertical_then_lower: tuple[PointLabel, ...]


@dataclass(frozen=True)
class VerticalMapFamily:
    source_order: int
    target_order: int
    maps: tuple[VerticalMapAtCut, ...]
    naturality_squares_checked: int
    naturality_failures: tuple[NaturalityFailure, ...]

    @property
    def is_natural(self) -> bool:
        return not self.naturality_failures

    def assert_natural(self) -> None:
        if self.naturality_failures:
            raise ValueError(
                f"vertical map {self.source_order}->{self.target_order} "
                f"has {len(self.naturality_failures)} non-commuting squares"
            )


@dataclass(frozen=True)
class Hierarchy:
    """All exhaustive orders, horizontal forests, and exact Gamma vertical maps."""

    point_count: int
    k_eff: int
    profile: Profile
    filtrations: tuple[GammaFiltration, ...]
    forests: tuple[MergeForest, ...]
    vertical_maps: tuple[VerticalMapFamily, ...]

    @property
    def hyperedges(self) -> tuple[GabrielHyperedge, ...]:
        return tuple(
            edge for filtration in self.filtrations for edge in filtration.gabriel_hyperedges
        )

    @property
    def equal_level_batches(self) -> tuple[ForestBatch, ...]:
        return tuple(batch for forest in self.forests for batch in forest.batches)

    @property
    def coverage_log(self) -> tuple[CoverageDelta, ...]:
        return tuple(delta for forest in self.forests for delta in forest.coverage_log)

    @property
    def attachment_incidences(self) -> tuple[BatchIncidence, ...]:
        return tuple(
            incidence
            for forest in self.forests
            for batch in forest.batches
            for incidence in batch.incidences
        )


@dataclass
class _MutableComponent:
    facets: set[PointLabel]
    covered: set[int]


class _TokenDisjointSet:
    def __init__(self) -> None:
        self.parent: dict[tuple[str, object], tuple[str, object]] = {}

    def add(self, token: tuple[str, object]) -> tuple[str, object]:
        self.parent.setdefault(token, token)
        return token

    def find(self, token: tuple[str, object]) -> tuple[str, object]:
        parent = self.parent[token]
        if parent != token:
            self.parent[token] = self.find(parent)
        return self.parent[token]

    def union(self, left: tuple[str, object], right: tuple[str, object]) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        if repr(left_root) < repr(right_root):
            self.parent[right_root] = left_root
        else:
            self.parent[left_root] = right_root


def _component(root_id: str, mutable: _MutableComponent) -> ForestComponent:
    return ForestComponent(
        root_id=root_id,
        facet_point_ids=tuple(sorted(mutable.facets)),
        covered_point_ids=tuple(sorted(mutable.covered)),
    )


def _node_id(order: int, profile: Profile, index: int) -> str:
    return f"k{order}:{profile}:n{index:08d}"


def build_merge_forest(filtration: GammaFiltration, profile: Profile) -> MergeForest:
    """Reduce one exact filtration without sequentializing equal levels."""

    if profile not in ("full_pi0", "hgp_reduced"):
        raise ValueError(f"unknown profile {profile!r}")
    order = filtration.order
    facet_by_label = {facet.point_ids: facet for facet in filtration.facets}

    births_by_level: dict[Fraction, list[PointLabel]] = {}
    relations_by_level: dict[Fraction, list[tuple[PointLabel, tuple[PointLabel, ...]]]] = {}
    if profile == "full_pi0" or order == 1:
        for facet in filtration.facets:
            births_by_level.setdefault(facet.squared_level, []).append(facet.point_ids)
    if profile == "full_pi0":
        for coface in filtration.cofaces:
            relations_by_level.setdefault(coface.squared_level, []).append(
                (coface.point_ids, coface.facet_point_ids)
            )
    else:
        for hyperedge in filtration.gabriel_hyperedges:
            relations_by_level.setdefault(hyperedge.squared_level, []).append(
                (hyperedge.simplex_point_ids, hyperedge.facet_point_ids)
            )
    levels = sorted(set(births_by_level) | set(relations_by_level))

    components: dict[str, _MutableComponent] = {}
    nodes: list[ForestNode] = []
    batches: list[ForestBatch] = []
    coverage_log: list[CoverageDelta] = []

    for level in levels:
        pre_components = tuple(
            _component(root_id, components[root_id]) for root_id in sorted(components)
        )
        roots_by_facet = {
            facet: root_id
            for root_id, component in components.items()
            for facet in component.facets
        }
        birth_labels = tuple(sorted(set(births_by_level.get(level, []))))
        relations = tuple(
            sorted(relations_by_level.get(level, []), key=lambda item: item[0])
        )
        incidences = tuple(
            BatchIncidence(simplex, facet, roots_by_facet.get(facet))
            for simplex, relation_facets in relations
            for facet in relation_facets
        )

        disjoint_set = _TokenDisjointSet()

        def token_for(facet: PointLabel) -> tuple[str, object]:
            if facet in roots_by_facet:
                return disjoint_set.add(("root", roots_by_facet[facet]))
            if facet not in facet_by_label:
                raise ValueError(f"relation references unknown facet {facet!r}")
            if facet_by_label[facet].squared_level > level:
                raise ValueError("a relation precedes the birth of one of its facets")
            return disjoint_set.add(("facet", facet))

        for facet in birth_labels:
            token_for(facet)
        for _, relation_facets in relations:
            tokens = [token_for(facet) for facet in relation_facets]
            for token in tokens[1:]:
                disjoint_set.union(tokens[0], token)

        groups: dict[tuple[str, object], set[tuple[str, object]]] = {}
        for token in disjoint_set.parent:
            groups.setdefault(disjoint_set.find(token), set()).add(token)
        sources_by_group: dict[tuple[str, object], set[PointLabel]] = {
            group: set() for group in groups
        }
        for facet in birth_labels:
            sources_by_group[disjoint_set.find(token_for(facet))].add(facet)
        for simplex, relation_facets in relations:
            sources_by_group[
                disjoint_set.find(token_for(relation_facets[0]))
            ].add(simplex)

        group_records = []
        for group, tokens in groups.items():
            pre_roots = tuple(sorted(token[1] for token in tokens if token[0] == "root"))
            new_facets = tuple(sorted(token[1] for token in tokens if token[0] == "facet"))
            sources = tuple(sorted(sources_by_group[group]))
            group_records.append((pre_roots, new_facets, sources))
        group_records.sort(key=lambda record: (record[0], record[1], record[2]))

        created_node_ids = []
        batch_deltas = []
        activated_facets = []
        for pre_roots, new_facets, sources in group_records:
            all_facets = set(new_facets)
            previously_covered: set[int] = set()
            for root_id in pre_roots:
                all_facets.update(components[root_id].facets)
                previously_covered.update(components[root_id].covered)
            covered = {point_id for facet in all_facets for point_id in facet}

            if not pre_roots:
                root_id = _node_id(order, profile, len(nodes))
                nodes.append(
                    ForestNode(
                        node_id=root_id,
                        order=order,
                        kind="birth",
                        squared_level=level,
                        child_ids=(),
                        covered_point_ids=tuple(sorted(covered)),
                        source_point_labels=sources,
                    )
                )
                created_node_ids.append(root_id)
            elif len(pre_roots) == 1:
                root_id = pre_roots[0]
            else:
                root_id = _node_id(order, profile, len(nodes))
                nodes.append(
                    ForestNode(
                        node_id=root_id,
                        order=order,
                        kind="merge",
                        squared_level=level,
                        child_ids=pre_roots,
                        covered_point_ids=tuple(sorted(covered)),
                        source_point_labels=sources,
                    )
                )
                created_node_ids.append(root_id)

            for old_root in pre_roots:
                del components[old_root]
            components[root_id] = _MutableComponent(all_facets, covered)
            added_points = tuple(sorted(covered - previously_covered))
            if new_facets or added_points:
                delta = CoverageDelta(
                    order=order,
                    squared_level=level,
                    root_id=root_id,
                    added_facet_point_ids=new_facets,
                    added_point_ids=added_points,
                )
                batch_deltas.append(delta)
                coverage_log.append(delta)
            activated_facets.extend(new_facets)

        post_components = tuple(
            _component(root_id, components[root_id]) for root_id in sorted(components)
        )
        batches.append(
            ForestBatch(
                order=order,
                squared_level=level,
                activated_facet_point_ids=tuple(sorted(activated_facets)),
                relation_simplex_point_ids=tuple(simplex for simplex, _ in relations),
                incidences=tuple(
                    sorted(
                        incidences,
                        key=lambda incidence: (
                            incidence.simplex_point_ids,
                            incidence.facet_point_ids,
                        ),
                    )
                ),
                pre_components=pre_components,
                post_components=post_components,
                created_node_ids=tuple(created_node_ids),
                coverage_deltas=tuple(
                    sorted(batch_deltas, key=lambda delta: delta.root_id)
                ),
            )
        )

    return MergeForest(
        order=order,
        profile=profile,
        nodes=tuple(nodes),
        root_ids=tuple(sorted(components)),
        batches=tuple(batches),
        coverage_log=tuple(coverage_log),
    )


def build_vertical_map_at_cut(source: GammaCut, target: GammaCut) -> VerticalMapAtCut:
    """Map every source component to its unique adjacent lower-order component."""

    if source.order != target.order + 1:
        raise ValueError("vertical maps require adjacent orders")
    if (source.squared_level, source.closed) != (target.squared_level, target.closed):
        raise ValueError("vertical maps require identical exact cuts")
    if source.graph_kind != "gamma" or target.graph_kind != "gamma":
        raise ValueError("the exhaustive vertical oracle is defined on Gamma cuts")

    target_by_facet = target.component_by_facet
    assignments = []
    for source_component in source.components:
        target_components = {
            target_by_facet[target_facet]
            for source_facet in source_component.facet_point_ids
            for target_facet in facets_of(source_facet)
        }
        if len(target_components) != 1:
            raise ValueError(
                "a source Gamma component has no unique component at the lower order"
            )
        target_component = next(iter(target_components))
        assignments.append(
            VerticalAssignment(
                source_component_facets=source_component.facet_point_ids,
                target_component_facets=target_component.facet_point_ids,
            )
        )
    assignments.sort(key=lambda assignment: assignment.source_component_facets)
    return VerticalMapAtCut(
        source_order=source.order,
        target_order=target.order,
        squared_level=source.squared_level,
        closed=source.closed,
        assignments=tuple(assignments),
    )


def _horizontal_component_map(
    earlier: GammaCut | ForestCut, later: GammaCut | ForestCut
) -> dict[tuple[PointLabel, ...], tuple[PointLabel, ...]]:
    later_by_facet = later.component_by_facet
    result = {}
    for component in earlier.components:
        targets = {later_by_facet[facet] for facet in component.facet_point_ids}
        if len(targets) != 1:
            raise ValueError("Gamma components must map uniquely under increasing scale")
        result[component.facet_point_ids] = next(iter(targets)).facet_point_ids
    return result


def _audit_naturality(
    maps: Sequence[VerticalMapAtCut],
    source_cuts: Sequence[GammaCut | ForestCut],
    target_cuts: Sequence[GammaCut | ForestCut],
) -> tuple[int, tuple[NaturalityFailure, ...]]:
    checked = 0
    failures = []
    for index in range(len(maps) - 1):
        earlier_map = maps[index]
        later_map = maps[index + 1]
        source_horizontal = _horizontal_component_map(
            source_cuts[index], source_cuts[index + 1]
        )
        target_horizontal = _horizontal_component_map(
            target_cuts[index], target_cuts[index + 1]
        )
        earlier_vertical = earlier_map.assignment_by_source
        later_vertical = later_map.assignment_by_source
        for source_component, earlier_target in earlier_vertical.items():
            checked += 1
            later_source = source_horizontal[source_component]
            lower_then_vertical = later_vertical[later_source]
            vertical_then_lower = target_horizontal[earlier_target]
            if lower_then_vertical != vertical_then_lower:
                failures.append(
                    NaturalityFailure(
                        earlier_level=earlier_map.squared_level,
                        earlier_closed=earlier_map.closed,
                        later_level=later_map.squared_level,
                        later_closed=later_map.closed,
                        source_component_facets=source_component,
                        lower_then_vertical=lower_then_vertical,
                        vertical_then_lower=vertical_then_lower,
                    )
                )
    return checked, tuple(failures)


def _profile_components_by_gamma_component(
    gamma_cut: GammaCut,
    profile_cut: ForestCut,
) -> dict[tuple[PointLabel, ...], ForestComponent]:
    """Verify and return the exhaustive Gamma-to-profile component bijection.

    At order one both profiles retain every Gamma component.  At higher orders
    ``full_pi0`` also retains every component, while ``hgp_reduced`` retains
    exactly the non-trivial Gamma components.  Facet containment, not overlap
    of covered point unions, identifies the containing Gamma component.
    """

    if (gamma_cut.order, gamma_cut.squared_level, gamma_cut.closed) != (
        profile_cut.order,
        profile_cut.squared_level,
        profile_cut.closed,
    ):
        raise ValueError("Gamma and profile cuts must describe the same exact state")
    expected_gamma_components = tuple(
        component
        for component in gamma_cut.components
        if profile_cut.profile == "full_pi0"
        or profile_cut.order == 1
        or component.nontrivial
    )
    expected_keys = {
        component.facet_point_ids for component in expected_gamma_components
    }
    gamma_by_facet = gamma_cut.component_by_facet
    result: dict[tuple[PointLabel, ...], ForestComponent] = {}
    for profile_component in profile_cut.components:
        if not profile_component.facet_point_ids:
            raise ValueError("a profile component must contain at least one facet")
        containing = {
            gamma_by_facet.get(facet)
            for facet in profile_component.facet_point_ids
        }
        if None in containing or len(containing) != 1:
            raise ValueError(
                "a profile component does not lie in one unique Gamma component"
            )
        gamma_component = next(iter(containing))
        if gamma_component is None:  # narrowed above, retained for static checkers
            raise AssertionError("missing Gamma component")
        key = gamma_component.facet_point_ids
        if key not in expected_keys:
            raise ValueError(
                "the reduced profile contains a component that is trivial in Gamma"
            )
        if profile_component.covered_point_ids != gamma_component.covered_point_ids:
            raise ValueError(
                "a profile component and its Gamma container cover different points"
            )
        if key in result:
            raise ValueError(
                "multiple profile components map to the same Gamma component"
            )
        result[key] = profile_component
    if set(result) != expected_keys:
        raise ValueError(
            "the profile components are not bijective with the required Gamma components"
        )
    return result


def build_vertical_map_family(
    source: GammaFiltration, target: GammaFiltration
) -> VerticalMapFamily:
    """Build open/closed maps at every shared critical interval and audit squares."""

    if source.order != target.order + 1:
        raise ValueError("source filtration must have order target.order + 1")
    if source.point_count != target.point_count:
        raise ValueError("vertical filtrations must use the same point cloud")
    target_coface_levels = {
        coface.point_ids: coface.squared_level for coface in target.cofaces
    }
    if any(
        target_coface_levels.get(facet.point_ids) != facet.squared_level
        for facet in source.facets
    ):
        raise ValueError("adjacent filtrations have inconsistent exact levels")
    levels = sorted(set(source.critical_levels) | set(target.critical_levels))
    maps = []
    source_cuts = []
    target_cuts = []
    for level in levels:
        for closed in (False, True):
            source_cut = source.cut(level, closed=closed)
            target_cut = target.cut(level, closed=closed)
            source_cuts.append(source_cut)
            target_cuts.append(target_cut)
            maps.append(build_vertical_map_at_cut(source_cut, target_cut))

    checked, failures = _audit_naturality(maps, source_cuts, target_cuts)

    return VerticalMapFamily(
        source_order=source.order,
        target_order=target.order,
        maps=tuple(maps),
        naturality_squares_checked=checked,
        naturality_failures=failures,
    )


def build_profile_vertical_map_family(
    source: GammaFiltration,
    target: GammaFiltration,
    source_forest: MergeForest,
    target_forest: MergeForest,
    profile: Profile,
) -> VerticalMapFamily:
    """Project the exhaustive Gamma map onto one declared output profile.

    The projection is certified independently at every open and closed cut.
    For ``hgp_reduced`` it verifies the bijection between reduced Gabriel
    components and non-trivial Gamma components, then transports the exact
    Gamma target through that bijection.  Covered-point containment is never
    used to choose a target because different K-polyhedra may overlap.
    """

    if profile not in ("full_pi0", "hgp_reduced"):
        raise ValueError(f"unknown profile {profile!r}")
    if source_forest.profile != profile or target_forest.profile != profile:
        raise ValueError("vertical projection forests must use the requested profile")
    if source_forest.order != source.order or target_forest.order != target.order:
        raise ValueError("vertical projection forests and filtrations disagree")

    gamma_family = build_vertical_map_family(source, target)
    gamma_family.assert_natural()
    gamma_maps = {
        (mapping.squared_level, mapping.closed): mapping
        for mapping in gamma_family.maps
    }
    levels = sorted(set(source.critical_levels) | set(target.critical_levels))
    maps = []
    source_cuts = []
    target_cuts = []
    for level in levels:
        for closed in (False, True):
            source_gamma_cut = source.cut(level, closed=closed)
            target_gamma_cut = target.cut(level, closed=closed)
            source_profile_cut = source_forest.cut(level, closed=closed)
            target_profile_cut = target_forest.cut(level, closed=closed)
            source_index = _profile_components_by_gamma_component(
                source_gamma_cut, source_profile_cut
            )
            target_index = _profile_components_by_gamma_component(
                target_gamma_cut, target_profile_cut
            )
            gamma_map = gamma_maps[(level, closed)].assignment_by_source
            assignments = []
            for source_gamma_facets, source_component in source_index.items():
                try:
                    target_gamma_facets = gamma_map[source_gamma_facets]
                    target_component = target_index[target_gamma_facets]
                except KeyError as error:
                    raise ValueError(
                        "the exhaustive Gamma target is outside the declared profile"
                    ) from error
                assignments.append(
                    VerticalAssignment(
                        source_component_facets=source_component.facet_point_ids,
                        target_component_facets=target_component.facet_point_ids,
                    )
                )
            assignments.sort(key=lambda assignment: assignment.source_component_facets)
            maps.append(
                VerticalMapAtCut(
                    source_order=source.order,
                    target_order=target.order,
                    squared_level=level,
                    closed=closed,
                    assignments=tuple(assignments),
                )
            )
            source_cuts.append(source_profile_cut)
            target_cuts.append(target_profile_cut)

    checked, failures = _audit_naturality(maps, source_cuts, target_cuts)
    return VerticalMapFamily(
        source_order=source.order,
        target_order=target.order,
        maps=tuple(maps),
        naturality_squares_checked=checked,
        naturality_failures=failures,
    )


def build_exhaustive_hierarchy(
    points: Sequence[object],
    k_max: int,
    profile: Profile,
    *,
    ball_fn: BallFunction | None = None,
) -> Hierarchy:
    """Build every order up to ``min(k_max, n)`` from independent enumeration."""

    if isinstance(k_max, bool) or not isinstance(k_max, int):
        raise TypeError("k_max must be an integer")
    if not 1 <= k_max <= 10:
        raise ValueError("k_max must lie in 1..10")
    if profile not in ("full_pi0", "hgp_reduced"):
        raise ValueError(f"unknown profile {profile!r}")
    if not points:
        raise ValueError("the oracle requires a non-empty point cloud")
    k_eff = min(k_max, len(points))
    filtrations = tuple(
        build_gamma_filtration(points, order, ball_fn=ball_fn)
        for order in range(1, k_eff + 1)
    )
    forests = tuple(build_merge_forest(filtration, profile) for filtration in filtrations)
    vertical_maps = tuple(
        build_profile_vertical_map_family(
            filtrations[order],
            filtrations[order - 1],
            forests[order],
            forests[order - 1],
            profile,
        )
        for order in range(1, k_eff)
    )
    for vertical_map in vertical_maps:
        vertical_map.assert_natural()
    return Hierarchy(
        point_count=len(points),
        k_eff=k_eff,
        profile=profile,
        filtrations=filtrations,
        forests=forests,
        vertical_maps=vertical_maps,
    )


build_hierarchy = build_exhaustive_hierarchy
