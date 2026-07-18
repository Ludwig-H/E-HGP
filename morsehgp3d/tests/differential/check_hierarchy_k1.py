#!/usr/bin/env python3
"""Independent exact differential for the Phase-5 k=1 hierarchy anchor.

The checker deliberately does not import C++ bindings or production helpers.
It classifies every diametric pair directly with :class:`fractions.Fraction`,
reduces the resulting rank-two graph with its own frozen-batch DSU, and uses
the stdlib-only ``tests.reference_emst`` implementation for the complete-graph
EMST path.  The exhaustive Python Gamma oracle is a third, separately written
check of the complete and Gabriel graph cuts.
"""

from __future__ import annotations

import argparse
import copy
import json
import math
import struct
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations, groupby
from pathlib import Path
from typing import Iterable, Sequence, TypeAlias


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from reference.morsehgp3d_oracle.gamma import build_gamma_filtration
from reference.morsehgp3d_oracle.generators import generate_affine_cloud
from tests.reference_emst import build_exhaustive_emst, exact_affine_dimension


PROTOCOL_HEADER = "morsehgp3d-hierarchy-k1-v1"
RESULT_SCHEMA = "morsehgp3d.hierarchy_k1_dump.v1"
DEFAULT_TIMEOUT_SECONDS = 180
MAX_POINT_COUNT = 14
UINT64_MASK = (1 << 64) - 1
NEGATIVE_ZERO_BITS = 1 << 63
EXPONENT_MASK = 0x7FF << 52

PointWords: TypeAlias = tuple[str, str, str]
ExactPoint: TypeAlias = tuple[Fraction, Fraction, Fraction]
Component: TypeAlias = tuple[int, ...]
Components: TypeAlias = tuple[Component, ...]
# Edges are sorted by exact squared length and then canonical endpoints.
Edge: TypeAlias = tuple[Fraction, int, int]
Fusion: TypeAlias = tuple[Fraction, Components, Component]


@dataclass(frozen=True)
class CampaignCase:
    case_id: int
    label: str
    source_points: tuple[PointWords, ...]
    tags: tuple[str, ...] = ()


@dataclass(frozen=True)
class PairAnalysis:
    u: int
    v: int
    center: ExactPoint
    squared_length: Fraction
    level: Fraction
    interior_ids: Component
    shell_ids: Component
    exterior_count: int
    classification: str

    @property
    def edge(self) -> Edge:
        return (self.squared_length, self.u, self.v)


@dataclass(frozen=True)
class ExpectedCase:
    document: dict[str, object]
    pairs: tuple[PairAnalysis, ...]
    rank_two_fusions: tuple[Fusion, ...]
    complete_edges: tuple[Edge, ...]
    rank_two_edges: tuple[Edge, ...]
    levels: tuple[Fraction, ...]


class DisjointSet:
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


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def strict_json_object(line: str) -> dict[str, object]:
    def reject_constant(value: str) -> object:
        raise ValueError(f"non-finite JSON constant {value!r}")

    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key {key!r}")
            result[key] = value
        return result

    value = json.loads(
        line,
        parse_constant=reject_constant,
        object_pairs_hook=reject_duplicates,
    )
    if not isinstance(value, dict):
        raise ValueError("each dump line must contain one JSON object")
    return value


def first_difference(expected: object, actual: object, path: str = "$"
) -> str | None:
    if type(expected) is not type(actual):
        return (
            f"{path}: expected type {type(expected).__name__}, "
            f"observed {type(actual).__name__}"
        )
    if isinstance(expected, dict):
        expected_keys = set(expected)
        actual_keys = set(actual)  # type: ignore[arg-type]
        if expected_keys != actual_keys:
            missing = sorted(expected_keys - actual_keys)
            extra = sorted(actual_keys - expected_keys)
            return f"{path}: missing keys {missing!r}, extra keys {extra!r}"
        for key in sorted(expected):
            difference = first_difference(
                expected[key], actual[key], f"{path}.{key}"  # type: ignore[index]
            )
            if difference is not None:
                return difference
        return None
    if isinstance(expected, list):
        if len(expected) != len(actual):  # type: ignore[arg-type]
            actual_length = len(actual)  # type: ignore[arg-type]
            return (
                f"{path}: expected length {len(expected)}, "
                f"observed {actual_length}"
            )
        for index, (expected_item, actual_item) in enumerate(
            zip(expected, actual)  # type: ignore[arg-type]
        ):
            difference = first_difference(
                expected_item, actual_item, f"{path}[{index}]"
            )
            if difference is not None:
                return difference
        return None
    if expected != actual:
        return f"{path}: expected {expected!r}, observed {actual!r}"
    return None


def bits_from_word(word: str) -> int:
    if len(word) != 16 or word.lower() != word:
        raise AssertionError(f"noncanonical binary64 word: {word!r}")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise AssertionError(f"invalid binary64 word: {word!r}") from error
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError("the hierarchy campaign contains a non-finite coordinate")
    return bits


def canonical_word(word: str) -> str:
    bits = bits_from_word(word)
    if bits == NEGATIVE_ZERO_BITS:
        bits = 0
    return f"{bits:016x}"


def total_order_key(word: str) -> int:
    bits = bits_from_word(canonical_word(word))
    if bits & NEGATIVE_ZERO_BITS:
        return (~bits) & UINT64_MASK
    return bits ^ NEGATIVE_ZERO_BITS


def fraction_from_word(word: str) -> Fraction:
    value = struct.unpack(">d", bytes.fromhex(canonical_word(word)))[0]
    return Fraction.from_float(value)


def binary64_word(value: Fraction | int | float) -> str:
    if isinstance(value, bool):
        raise TypeError("boolean coordinates are forbidden")
    encoded = value if isinstance(value, float) else float(Fraction(value))
    if not math.isfinite(encoded):
        raise AssertionError(f"coordinate {value!r} is not finite binary64")
    if not isinstance(value, float) and Fraction.from_float(encoded) != Fraction(value):
        raise AssertionError(f"coordinate {value!r} is not exactly binary64")
    return struct.pack(">d", encoded).hex()


def point_words(
    x: Fraction | int | float,
    y: Fraction | int | float,
    z: Fraction | int | float,
) -> PointWords:
    return (binary64_word(x), binary64_word(y), binary64_word(z))


def exact_point(words: PointWords) -> ExactPoint:
    return tuple(fraction_from_word(word) for word in words)  # type: ignore[return-value]


def canonicalize_points(
    source_points: tuple[PointWords, ...],
) -> tuple[list[dict[str, object]], tuple[ExactPoint, ...]]:
    candidates: list[tuple[tuple[int, int, int], PointWords, int]] = []
    for source_index, source_words in enumerate(source_points):
        words = tuple(canonical_word(word) for word in source_words)
        keys = tuple(total_order_key(word) for word in words)
        candidates.append((keys, words, source_index))  # type: ignore[arg-type]
    candidates.sort(key=lambda candidate: candidate[0])
    for index in range(1, len(candidates)):
        if candidates[index - 1][1] == candidates[index][1]:
            raise AssertionError("a hierarchy campaign case contains duplicates")

    records = []
    points = []
    for point_id, (_, words, source_index) in enumerate(candidates):
        records.append(
            {
                "id": point_id,
                "input_bits": list(words),
                "source_index": source_index,
            }
        )
        points.append(exact_point(words))
    return records, tuple(points)


def squared_distance(left: ExactPoint, right: ExactPoint) -> Fraction:
    return sum(
        ((left[axis] - right[axis]) ** 2 for axis in range(3)), Fraction()
    )


def ratio_record(value: Fraction | int) -> dict[str, str]:
    exact = Fraction(value)
    return {
        "denominator": str(exact.denominator),
        "numerator": str(exact.numerator),
    }


def center_record(center: ExactPoint) -> dict[str, dict[str, str]]:
    return {
        "x": ratio_record(center[0]),
        "y": ratio_record(center[1]),
        "z": ratio_record(center[2]),
    }


def edge_record(edge: Edge) -> dict[str, object]:
    squared_length, u, v = edge
    return {
        "level": ratio_record(squared_length / 4),
        "squared_length": ratio_record(squared_length),
        "u": u,
        "v": v,
    }


def components_record(components: Components) -> list[list[int]]:
    return [list(component) for component in components]


def fusion_record(fusion: Fusion) -> dict[str, object]:
    level, children, merged = fusion
    return {
        "child_components": components_record(children),
        "level": ratio_record(level),
        "merged_component": list(merged),
    }


def hierarchy_records(
    point_count: int, fusions: tuple[Fusion, ...]
) -> tuple[list[dict[str, object]], int]:
    nodes: list[dict[str, object]] = []
    node_by_component: dict[Component, int] = {}
    for point_id in range(point_count):
        component = (point_id,)
        node_by_component[component] = point_id
        nodes.append(
            {
                "children": [],
                "id": point_id,
                "level": ratio_record(0),
                "point_ids": [point_id],
            }
        )

    for level, children, merged in fusions:
        try:
            child_ids = [node_by_component[child] for child in children]
        except KeyError as error:
            raise AssertionError(
                "a hierarchy fusion references an absent strict child"
            ) from error
        if merged in node_by_component:
            raise AssertionError("a hierarchy component was created twice")
        node_id = len(nodes)
        nodes.append(
            {
                "children": child_ids,
                "id": node_id,
                "level": ratio_record(level),
                "point_ids": list(merged),
            }
        )
        node_by_component[merged] = node_id

    full_component = tuple(range(point_count))
    if full_component not in node_by_component:
        raise AssertionError("the hierarchy has no canonical root component")
    return nodes, node_by_component[full_component]


def components(disjoint_set: DisjointSet, point_count: int) -> Components:
    grouped: dict[int, list[int]] = {}
    for point_id in range(point_count):
        grouped.setdefault(disjoint_set.find(point_id), []).append(point_id)
    return tuple(sorted(tuple(group) for group in grouped.values()))


def graph_cut(
    point_count: int,
    edges: Sequence[Edge],
    level: Fraction,
    *,
    closed: bool,
) -> Components:
    vertices_active = level >= 0 if closed else level > 0
    if not vertices_active:
        return ()
    disjoint_set = DisjointSet(point_count)
    for squared_length, u, v in edges:
        edge_level = squared_length / 4
        if edge_level <= level if closed else edge_level < level:
            disjoint_set.union(u, v)
    return components(disjoint_set, point_count)


def frozen_fusions(
    point_count: int,
    global_dsu: DisjointSet,
    batch_edges: tuple[Edge, ...],
    level: Fraction,
) -> tuple[Fusion, ...]:
    pre_components = components(global_dsu, point_count)
    component_by_point = {
        point_id: component
        for component in pre_components
        for point_id in component
    }
    component_index = {
        component: index for index, component in enumerate(pre_components)
    }
    batch_dsu = DisjointSet(len(pre_components))
    touched: set[Component] = set()
    for _, u, v in batch_edges:
        left = component_by_point[u]
        right = component_by_point[v]
        if left == right:
            continue
        touched.add(left)
        touched.add(right)
        batch_dsu.union(component_index[left], component_index[right])

    grouped: dict[int, list[Component]] = {}
    for component in sorted(touched):
        root = batch_dsu.find(component_index[component])
        grouped.setdefault(root, []).append(component)
    result = []
    for raw_children in grouped.values():
        children = tuple(sorted(raw_children))
        if len(children) < 2:
            continue
        merged = tuple(sorted(point for child in children for point in child))
        result.append((level, children, merged))
    return tuple(sorted(result, key=lambda fusion: fusion[1]))


def reduce_edges(
    point_count: int, edges: Sequence[Edge]
) -> tuple[tuple[Edge, ...], tuple[Fusion, ...]]:
    ordered_edges = tuple(sorted(edges))
    global_dsu = DisjointSet(point_count)
    selected = []
    fusions = []
    for squared_length, raw_batch in groupby(
        ordered_edges, key=lambda edge: edge[0]
    ):
        batch_edges = tuple(raw_batch)
        fusions.extend(
            frozen_fusions(
                point_count,
                global_dsu,
                batch_edges,
                squared_length / 4,
            )
        )
        for edge in batch_edges:
            _, u, v = edge
            if global_dsu.union(u, v):
                selected.append(edge)
    return tuple(selected), tuple(fusions)


def analyze_pairs(points: tuple[ExactPoint, ...]) -> tuple[PairAnalysis, ...]:
    analyses = []
    for u, v in combinations(range(len(points)), 2):
        center = tuple((points[u][axis] + points[v][axis]) / 2 for axis in range(3))
        squared_length = squared_distance(points[u], points[v])
        level = squared_length / 4
        distances = tuple(squared_distance(point, center) for point in points)
        interior = tuple(
            point_id
            for point_id, distance in enumerate(distances)
            if distance < level
        )
        shell = tuple(
            point_id
            for point_id, distance in enumerate(distances)
            if distance == level
        )
        if interior:
            classification = "interior_blocked"
        elif shell == (u, v):
            classification = "rank_two_critical"
        else:
            classification = "extra_shell_degeneracy"
        analyses.append(
            PairAnalysis(
                u=u,
                v=v,
                center=center,  # type: ignore[arg-type]
                squared_length=squared_length,
                level=level,
                interior_ids=interior,
                shell_ids=shell,
                exterior_count=len(points) - len(interior) - len(shell),
                classification=classification,
            )
        )
    return tuple(analyses)


def pair_record(pair: PairAnalysis) -> dict[str, object]:
    return {
        "center": center_record(pair.center),
        "classification": pair.classification,
        "closed_rank": len(pair.interior_ids) + len(pair.shell_ids),
        "exterior_count": pair.exterior_count,
        "interior_ids": list(pair.interior_ids),
        "level": ratio_record(pair.level),
        "shell_ids": list(pair.shell_ids),
        "squared_length": ratio_record(pair.squared_length),
        "u": pair.u,
        "v": pair.v,
    }


def gamma_partition(cut: object) -> Components:
    raw_components = getattr(cut, "components")
    return tuple(sorted(component.covered_point_ids for component in raw_components))


def oracle_emst_edges(raw_edges: Iterable[object]) -> tuple[Edge, ...]:
    return tuple(
        (
            Fraction(getattr(edge, "squared_distance")),
            int(getattr(edge, "left_id")),
            int(getattr(edge, "right_id")),
        )
        for edge in raw_edges
    )


def oracle_emst_fusions(emst: object) -> tuple[Fusion, ...]:
    result = []
    for batch in getattr(emst, "batches"):
        for fusion in batch.multifusions:
            result.append(
                (
                    Fraction(batch.squared_level),
                    tuple(tuple(child) for child in fusion.child_components),
                    tuple(fusion.merged_component),
                )
            )
    return tuple(result)


def certify_python_oracles(
    points: tuple[ExactPoint, ...],
    pairs: tuple[PairAnalysis, ...],
    complete_edges: tuple[Edge, ...],
    gabriel_edges: tuple[Edge, ...],
    levels: tuple[Fraction, ...],
    emst: object,
    gamma: object,
) -> None:
    cofaces = {
        tuple(coface.point_ids): coface for coface in getattr(gamma, "cofaces")
    }
    if set(cofaces) != {(pair.u, pair.v) for pair in pairs}:
        raise AssertionError("the Gamma oracle did not enumerate the pair universe")
    for pair in pairs:
        coface = cofaces[(pair.u, pair.v)]
        if Fraction(coface.squared_level) != pair.level:
            raise AssertionError("direct and Gamma pair levels differ")
        if tuple(Fraction(value) for value in coface.center) != pair.center:
            raise AssertionError("direct and Gamma diametric centers differ")

    gamma_gabriel = tuple(
        sorted(
            (
                Fraction(edge.squared_level) * 4,
                int(edge.simplex_point_ids[0]),
                int(edge.simplex_point_ids[1]),
            )
            for edge in getattr(gamma, "gabriel_hyperedges")
        )
    )
    if gamma_gabriel != gabriel_edges:
        raise AssertionError("direct and Gamma Gabriel pair catalogues differ")
    if oracle_emst_edges(getattr(emst, "complete_edges")) != complete_edges:
        raise AssertionError("direct and independent EMST complete graphs differ")

    point_count = len(points)
    for level in levels:
        for closed in (False, True):
            complete = graph_cut(
                point_count, complete_edges, level, closed=closed
            )
            gabriel = graph_cut(
                point_count, gabriel_edges, level, closed=closed
            )
            oracle_complete = getattr(emst, "cut")(
                level, closed=closed, edge_source="complete_graph"
            ).components
            gamma_complete = gamma_partition(
                getattr(gamma, "cut")(
                    level, closed=closed, graph_kind="gamma"
                )
            )
            gamma_gabriel_cut = gamma_partition(
                getattr(gamma, "cut")(
                    level, closed=closed, graph_kind="gabriel"
                )
            )
            if not (
                complete == oracle_complete == gamma_complete
                and gabriel == gamma_gabriel_cut
                and complete == gabriel
            ):
                raise AssertionError(
                    "direct, EMST, Gamma and Gabriel cuts disagree at "
                    f"level={level}, closed={closed}"
                )


def expected_result(case: CampaignCase) -> ExpectedCase:
    canonical_records, points = canonicalize_points(case.source_points)
    pairs = analyze_pairs(points)
    complete_edges = tuple(sorted(pair.edge for pair in pairs))
    rank_two_edges = tuple(
        sorted(
            pair.edge
            for pair in pairs
            if pair.classification == "rank_two_critical"
        )
    )
    gabriel_edges = tuple(
        sorted(
            pair.edge
            for pair in pairs
            if pair.classification != "interior_blocked"
        )
    )
    rank_two_selected, rank_two_fusions = reduce_edges(
        len(points), rank_two_edges
    )
    emst = build_exhaustive_emst(points)
    emst_selected = oracle_emst_edges(emst.selected_edges)
    emst_fusions = oracle_emst_fusions(emst)
    if rank_two_fusions != emst_fusions:
        raise AssertionError(
            f"case {case.case_id}: direct rank-two and EMST multifusions differ"
        )

    levels = tuple(sorted({Fraction()} | {pair.level for pair in pairs}))
    gamma = build_gamma_filtration(points, 1)
    certify_python_oracles(
        points,
        pairs,
        complete_edges,
        gabriel_edges,
        levels,
        emst,
        gamma,
    )

    cuts = []
    for level in levels:
        for closed in (False, True):
            complete_cut = graph_cut(
                len(points), complete_edges, level, closed=closed
            )
            selected_emst_cut = tuple(
                emst.cut(level, closed=closed).components
            )
            rank_two_cut = graph_cut(
                len(points), rank_two_edges, level, closed=closed
            )
            selected_rank_two_cut = graph_cut(
                len(points), rank_two_selected, level, closed=closed
            )
            gabriel_cut = graph_cut(
                len(points), gabriel_edges, level, closed=closed
            )
            cuts.append(
                {
                    "closed": closed,
                    "complete_graph": components_record(complete_cut),
                    "gabriel_diagnostic_graph": components_record(gabriel_cut),
                    "level": ratio_record(level),
                    "rank_two_graph": components_record(rank_two_cut),
                    "selected_emst": components_record(selected_emst_cut),
                    "selected_rank_two_witness": components_record(
                        selected_rank_two_cut
                    ),
                }
            )

    total_squared_weight = sum(
        (edge[0] for edge in emst_selected), Fraction()
    )
    total_rank_two_squared_weight = sum(
        (edge[0] for edge in rank_two_selected), Fraction()
    )
    emst_nodes, emst_root = hierarchy_records(len(points), emst_fusions)
    rank_two_nodes, rank_two_root = hierarchy_records(
        len(points), rank_two_fusions
    )
    has_extra_shell = any(
        pair.classification == "extra_shell_degeneracy" for pair in pairs
    )
    all_true_certificate = {
        "all_selected_witness_edges_are_rank_two": True,
        "anchor_equivalence_certified": True,
        "closed_cuts_match": True,
        "comparison_level_count": len(levels),
        "exact_pair_decisions_complete": True,
        "multifusions_match": True,
        "pair_universe_matches_emst": True,
        "selected_tree_edges_match": emst_selected == rank_two_selected,
        "selected_tree_hgp_weight_matches": True,
        "selected_tree_hierarchy_matches": True,
        "selected_tree_squared_weight_matches": True,
        "strict_cuts_match": True,
    }
    document: dict[str, object] = {
        "canonical_points": canonical_records,
        "case_id": case.case_id,
        "catalog": {
            "catalog_status": (
                "unsupported_degeneracy" if has_extra_shell else "supported"
            ),
            "gabriel_edges": [
                edge_record(edge) for edge in gabriel_edges
            ],
            "pairs": [pair_record(pair) for pair in pairs],
            "rank_two_edges": [edge_record(edge) for edge in rank_two_edges],
        },
        "certificate": all_true_certificate,
        "cuts": cuts,
        "emst": {
            "multifusions": [fusion_record(fusion) for fusion in emst_fusions],
            "nodes": emst_nodes,
            "root_node_id": emst_root,
            "selected_edges": [edge_record(edge) for edge in emst_selected],
            "total_hgp_weight": ratio_record(total_squared_weight / 4),
            "total_squared_weight": ratio_record(total_squared_weight),
        },
        "locally_supported": not has_extra_shell,
        "rank_two": {
            "multifusions": [
                fusion_record(fusion) for fusion in rank_two_fusions
            ],
            "nodes": rank_two_nodes,
            "root_node_id": rank_two_root,
            "selected_edges": [
                edge_record(edge) for edge in rank_two_selected
            ],
            "total_hgp_weight": ratio_record(
                total_rank_two_squared_weight / 4
            ),
            "total_squared_weight": ratio_record(
                total_rank_two_squared_weight
            ),
        },
        "schema": RESULT_SCHEMA,
    }
    return ExpectedCase(
        document,
        pairs,
        rank_two_fusions,
        complete_edges,
        rank_two_edges,
        levels,
    )


def point_cloud_words(
    points: Iterable[Sequence[Fraction | int | float]],
) -> tuple[PointWords, ...]:
    return tuple(point_words(point[0], point[1], point[2]) for point in points)


def load_missing_lnn_case(path: Path) -> CampaignCase:
    payload = json.loads(path.read_text(encoding="utf-8"))
    points = payload.get("points")
    if not isinstance(points, list) or len(points) != 12:
        raise ValueError("the permanent missing-LNN fixture must contain 12 points")
    normalized = []
    for point in points:
        if (
            not isinstance(point, list)
            or len(point) != 3
            or any(isinstance(value, bool) or not isinstance(value, int) for value in point)
        ):
            raise ValueError("the missing-LNN fixture must contain integer triples")
        normalized.append(tuple(point))
    return CampaignCase(
        12,
        "gabriel-pair-missing-fixed-lnn",
        point_cloud_words(normalized),
        ("missing_lnn",),
    )


def targeted_cases(missing_lnn_fixture: Path) -> tuple[CampaignCase, ...]:
    one_ulp_inside = math.nextafter(1.0, 0.0)
    one_ulp_outside = math.nextafter(1.0, math.inf)
    maximum = float.fromhex("0x1.fffffffffffffp+1023")
    minimum_subnormal = float.fromhex("0x0.0000000000001p-1022")
    cube = tuple(
        (x, y, z)
        for x in (-1, 1)
        for y in (-1, 1)
        for z in (-1, 1)
    )
    cases = [
        CampaignCase(
            1,
            "singleton-zero-birth",
            point_cloud_words(((2, -1, 4),)),
            ("singleton",),
        ),
        CampaignCase(
            2,
            "exact-quarter-level",
            point_cloud_words(((1, 1, 1), (0, 0, 0))),
            ("quarter",),
        ),
        CampaignCase(
            3,
            "blocked-collinear-chain",
            point_cloud_words(((8, 0, 0), (0, 0, 0), (2, 0, 0))),
            ("blocked",),
        ),
        CampaignCase(
            4,
            "right-triangle-extra-shell",
            point_cloud_words(((0, 1, 0), (1, 0, 0), (-1, 0, 0))),
            ("extra_shell",),
        ),
        CampaignCase(
            5,
            "one-ulp-inside-diametric-shell",
            point_cloud_words(((-1, 0, 0), (0, one_ulp_inside, 0), (1, 0, 0))),
            ("ulp_inside",),
        ),
        CampaignCase(
            6,
            "one-ulp-outside-diametric-shell",
            point_cloud_words(((-1, 0, 0), (0, one_ulp_outside, 0), (1, 0, 0))),
            ("ulp_outside",),
        ),
        CampaignCase(
            7,
            "two-disjoint-equal-fusions",
            point_cloud_words(((12, 0, 0), (2, 0, 0), (10, 0, 0), (0, 0, 0))),
            ("disjoint",),
        ),
        CampaignCase(
            8,
            "square-extra-shell-multifusion",
            point_cloud_words(((1, 1, 0), (-1, -1, 0), (1, -1, 0), (-1, 1, 0))),
            ("square", "extra_shell"),
        ),
        CampaignCase(
            9,
            "regular-tetrahedron-rank-two-cycle",
            point_cloud_words(((1, -1, -1), (-1, -1, 1), (1, 1, 1), (-1, 1, -1))),
            ("tetrahedron",),
        ),
        CampaignCase(
            10,
            "six-way-collinear-multifusion",
            point_cloud_words(tuple((index, 0, 0) for index in reversed(range(6)))),
            ("arity_six",),
        ),
        CampaignCase(
            11,
            "cube-eight-way-multifusion",
            point_cloud_words(tuple(reversed(cube))),
            ("cube", "extra_shell"),
        ),
        load_missing_lnn_case(missing_lnn_fixture),
        CampaignCase(
            13,
            "dyadic-fractional-cloud",
            point_cloud_words(
                (
                    (Fraction(5, 8), Fraction(-1, 2), Fraction(3, 4)),
                    (Fraction(-3, 8), Fraction(1, 4), Fraction(1, 2)),
                    (Fraction(1, 8), Fraction(7, 8), Fraction(-1, 4)),
                    (Fraction(-7, 8), Fraction(-3, 4), Fraction(5, 8)),
                    (Fraction(3, 8), Fraction(1, 2), Fraction(-7, 8)),
                )
            ),
            ("dyadic",),
        ),
        CampaignCase(
            14,
            "binary64-exponent-extremes",
            point_cloud_words(
                (
                    (maximum, 0.0, 0.0),
                    (-maximum, -0.0, 0.0),
                    (0.0, minimum_subnormal, 0.0),
                    (0.0, 0.0, minimum_subnormal),
                )
            ),
            ("extreme",),
        ),
    ]
    return tuple(cases)


def affine_cases() -> tuple[CampaignCase, ...]:
    cases = []
    for dimension in (1, 2, 3):
        for point_count in range(dimension + 1, MAX_POINT_COUNT + 1):
            seed = 50_000 * dimension + point_count
            points = generate_affine_cloud(
                dimension,
                point_count,
                seed=seed,
                coordinate_bound=31,
            )
            if exact_affine_dimension(points) != dimension:
                raise AssertionError(
                    f"generator seed {seed} did not produce affine dimension {dimension}"
                )
            cases.append(
                CampaignCase(
                    1_000 + 100 * dimension + point_count,
                    f"affine-d{dimension}-n{point_count}-seed{seed}",
                    point_cloud_words(points),
                    (f"affine_d{dimension}",),
                )
            )
    return tuple(cases)


def validate_strong_case(case: CampaignCase, expected: ExpectedCase) -> None:
    pairs = {(pair.u, pair.v): pair for pair in expected.pairs}
    fusions = expected.rank_two_fusions
    classifications = {pair.classification for pair in expected.pairs}
    if "singleton" in case.tags and (pairs or fusions):
        raise AssertionError("singleton fixture unexpectedly contains an event")
    if "quarter" in case.tags:
        pair = pairs[(0, 1)]
        if pair.squared_length != 3 or pair.level != Fraction(3, 4):
            raise AssertionError("quarter fixture lost its exact 3/4 level")
    if "blocked" in case.tags and "interior_blocked" not in classifications:
        raise AssertionError("blocked fixture has no strict interior witness")
    if "extra_shell" in case.tags and "extra_shell_degeneracy" not in classifications:
        raise AssertionError("extra-shell fixture lost its shell degeneracy")
    if "ulp_inside" in case.tags:
        if pairs[(0, 2)].classification != "interior_blocked":
            raise AssertionError("one-ULP-inside pair is not strictly blocked")
    if "ulp_outside" in case.tags:
        if pairs[(0, 2)].classification != "rank_two_critical":
            raise AssertionError("one-ULP-outside pair is not rank-two critical")
    if "disjoint" in case.tags:
        same_level = [fusion for fusion in fusions if fusion[0] == 1]
        if len(same_level) != 2:
            raise AssertionError("disjoint fixture lost its two level-one fusions")
    if "square" in case.tags:
        rank_two_count = sum(
            pair.classification == "rank_two_critical" for pair in expected.pairs
        )
        extra_count = sum(
            pair.classification == "extra_shell_degeneracy"
            for pair in expected.pairs
        )
        if rank_two_count != 4 or extra_count != 2:
            raise AssertionError("square fixture lost its 4/2 rank-two/shell split")
    if "tetrahedron" in case.tags:
        if any(
            pair.classification != "rank_two_critical" for pair in expected.pairs
        ):
            raise AssertionError("regular tetrahedron lost a rank-two edge")
    if "arity_six" in case.tags and max(
        (len(fusion[1]) for fusion in fusions), default=0
    ) < 6:
        raise AssertionError("six-way fixture lost its atomic multifusion")
    if "cube" in case.tags and max(
        (len(fusion[1]) for fusion in fusions), default=0
    ) < 8:
        raise AssertionError("cube fixture lost its eight-way multifusion")
    if "missing_lnn" in case.tags:
        target = pairs[(0, 6)]
        if target.squared_length != 40_000 or target.level != 10_000:
            raise AssertionError("missing-LNN fixture lost its target pair level")
        if target.classification != "rank_two_critical":
            raise AssertionError("missing-LNN target is no longer rank-two critical")
        _, points = canonicalize_points(case.source_points)
        for endpoint in (0, 6):
            closer_count = sum(
                point_id != endpoint
                and squared_distance(points[endpoint], point) < 40_000
                for point_id, point in enumerate(points)
            )
            if closer_count != 5:
                raise AssertionError(
                    "missing-LNN target endpoint lost its five closer observations"
                )


def case_protocol_line(case: CampaignCase) -> str:
    tokens = ["case", str(case.case_id), str(len(case.source_points))]
    for words in case.source_points:
        tokens.extend(words)
    return " ".join(tokens)


def fraction_from_ratio_record(value: object, label: str) -> Fraction:
    if not isinstance(value, dict) or set(value) != {"denominator", "numerator"}:
        raise AssertionError(f"{label} is not a canonical ratio object")
    denominator = value["denominator"]
    numerator = value["numerator"]
    if not isinstance(denominator, str) or not isinstance(numerator, str):
        raise AssertionError(f"{label} ratio fields must be strings")
    try:
        result = Fraction(int(numerator), int(denominator))
    except (ValueError, ZeroDivisionError) as error:
        raise AssertionError(f"{label} is not a valid exact ratio") from error
    if ratio_record(result) != value:
        raise AssertionError(f"{label} is not reduced canonically")
    return result


def edge_from_record(value: object, label: str) -> Edge:
    if not isinstance(value, dict) or set(value) != {
        "level",
        "squared_length",
        "u",
        "v",
    }:
        raise AssertionError(f"{label} is not a canonical edge object")
    u = value["u"]
    v = value["v"]
    if type(u) is not int or type(v) is not int or not 0 <= u < v:
        raise AssertionError(f"{label} has invalid canonical endpoints")
    squared_length = fraction_from_ratio_record(
        value["squared_length"], f"{label}.squared_length"
    )
    level = fraction_from_ratio_record(value["level"], f"{label}.level")
    edge = (squared_length, u, v)
    if squared_length <= 0 or level != squared_length / 4:
        raise AssertionError(f"{label} has an invalid exact weight convention")
    if edge_record(edge) != value:
        raise AssertionError(f"{label} is not serialized canonically")
    return edge


def selected_edges_from_document(
    document: dict[str, object], section_name: str
) -> tuple[Edge, ...]:
    section = document.get(section_name)
    if not isinstance(section, dict):
        raise AssertionError(f"$.{section_name} is not an object")
    records = section.get("selected_edges")
    if not isinstance(records, list):
        raise AssertionError(f"$.{section_name}.selected_edges is not an array")
    return tuple(
        edge_from_record(record, f"$.{section_name}.selected_edges[{index}]")
        for index, record in enumerate(records)
    )


def normalize_valid_witnesses(
    expected: ExpectedCase, actual: dict[str, object]
) -> dict[str, object]:
    point_count = len(expected.document["canonical_points"])  # type: ignore[arg-type]
    emst_edges = selected_edges_from_document(actual, "emst")
    rank_two_edges = selected_edges_from_document(actual, "rank_two")
    target_weight = fraction_from_ratio_record(
        expected.document["emst"]["total_squared_weight"],  # type: ignore[index]
        "$.emst.total_squared_weight",
    )

    for label, edges, allowed_edges in (
        ("emst", emst_edges, expected.complete_edges),
        ("rank_two", rank_two_edges, expected.rank_two_edges),
    ):
        if edges != tuple(sorted(edges)) or len(set(edges)) != len(edges):
            raise AssertionError(f"$.{label}.selected_edges is not canonical")
        if len(edges) != point_count - 1:
            raise AssertionError(
                f"$.{label}.selected_edges does not contain n-1 edges"
            )
        if not set(edges) <= set(allowed_edges):
            raise AssertionError(
                f"$.{label}.selected_edges escaped its certified graph"
            )
        if sum((edge[0] for edge in edges), Fraction()) != target_weight:
            raise AssertionError(
                f"$.{label}.selected_edges has the wrong exact total weight"
            )
        for level in expected.levels:
            for closed in (False, True):
                if graph_cut(point_count, edges, level, closed=closed) != graph_cut(
                    point_count,
                    expected.complete_edges,
                    level,
                    closed=closed,
                ):
                    raise AssertionError(
                        f"$.{label}.selected_edges changes the cut at "
                        f"level={level}, closed={closed}"
                    )

    certificate = actual.get("certificate")
    if not isinstance(certificate, dict):
        raise AssertionError("$.certificate is not an object")
    diagnostic = certificate.get("selected_tree_edges_match")
    if type(diagnostic) is not bool:
        raise AssertionError(
            "$.certificate.selected_tree_edges_match is not Boolean"
        )
    if diagnostic != (emst_edges == rank_two_edges):
        raise AssertionError(
            "selected_tree_edges_match contradicts the serialized witnesses"
        )

    normalized = copy.deepcopy(expected.document)
    normalized["emst"]["selected_edges"] = actual["emst"][  # type: ignore[index]
        "selected_edges"
    ]
    normalized["rank_two"]["selected_edges"] = actual["rank_two"][  # type: ignore[index]
        "selected_edges"
    ]
    normalized["certificate"]["selected_tree_edges_match"] = diagnostic  # type: ignore[index]
    return normalized


def verify_alternate_equal_weight_witness_is_accepted(
    expected: ExpectedCase,
) -> None:
    point_count = len(expected.document["canonical_points"])  # type: ignore[arg-type]
    original = tuple(
        edge_from_record(record, "$.rank_two.selected_edges")
        for record in expected.document["rank_two"]["selected_edges"]  # type: ignore[index]
    )
    target_weight = sum((edge[0] for edge in original), Fraction())
    alternate: tuple[Edge, ...] | None = None
    for candidate in combinations(expected.rank_two_edges, point_count - 1):
        if candidate == original or sum(
            (edge[0] for edge in candidate), Fraction()
        ) != target_weight:
            continue
        if all(
            graph_cut(point_count, candidate, level, closed=closed)
            == graph_cut(
                point_count,
                expected.complete_edges,
                level,
                closed=closed,
            )
            for level in expected.levels
            for closed in (False, True)
        ):
            alternate = candidate
            break
    if alternate is None:
        raise AssertionError(
            "the equal-weight witness fixture has no alternate valid tree"
        )

    mutated = copy.deepcopy(expected.document)
    mutated["rank_two"]["selected_edges"] = [  # type: ignore[index]
        edge_record(edge) for edge in alternate
    ]
    mutated["certificate"]["selected_tree_edges_match"] = False  # type: ignore[index]
    normalized = normalize_valid_witnesses(expected, mutated)
    difference = first_difference(normalized, mutated)
    if difference is not None:
        raise AssertionError(
            "a valid alternate equal-weight witness was rejected: " + difference
        )


def run_differential(
    executable: Path,
    cases: tuple[CampaignCase, ...],
    timeout_seconds: int,
) -> None:
    expected_cases = []
    for case in cases:
        expected = expected_result(case)
        validate_strong_case(case, expected)
        if "square" in case.tags:
            verify_alternate_equal_weight_witness_is_accepted(expected)
        expected_cases.append(expected)

    payload = "\n".join(
        [
            PROTOCOL_HEADER,
            *(case_protocol_line(case) for case in cases),
            "end",
            "",
        ]
    )
    completed = subprocess.run(
        [str(executable)],
        input=payload,
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout_seconds,
    )
    if completed.returncode != 0:
        raise AssertionError(
            "the C++ k=1 hierarchy dump failed with status "
            f"{completed.returncode}: {completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("the C++ k=1 hierarchy dump wrote unexpected diagnostics")

    output_lines = completed.stdout.splitlines()
    if len(output_lines) != len(cases):
        raise AssertionError(
            f"the C++ dump emitted {len(output_lines)} lines for {len(cases)} cases"
        )
    for case, expected, line in zip(cases, expected_cases, output_lines):
        actual = strict_json_object(line)
        if line != canonical_json(actual):
            raise AssertionError(
                f"case {case.case_id} ({case.label}) output is not canonical JSON"
            )
        normalized_expected = normalize_valid_witnesses(expected, actual)
        difference = first_difference(normalized_expected, actual)
        if difference is not None:
            raise AssertionError(
                f"case {case.case_id} ({case.label}) differs: {difference}"
            )

    covered_sizes = {len(case.source_points) for case in cases}
    expected_sizes = set(range(1, MAX_POINT_COUNT + 1))
    if covered_sizes != expected_sizes:
        raise AssertionError(
            f"campaign size coverage differs: expected {sorted(expected_sizes)}, "
            f"observed {sorted(covered_sizes)}"
        )
    classifications = {
        pair.classification
        for expected in expected_cases
        for pair in expected.pairs
    }
    expected_classifications = {
        "rank_two_critical",
        "extra_shell_degeneracy",
        "interior_blocked",
    }
    if classifications != expected_classifications:
        raise AssertionError(
            "campaign did not cover all exact pair classifications: "
            f"{sorted(classifications)}"
        )
    print(
        "k=1 hierarchy differential: "
        f"{len(cases)} exact cases passed; dimensions 1..3 and sizes "
        f"1..{MAX_POINT_COUNT} covered"
    )


def parse_arguments() -> argparse.Namespace:
    default_fixture = (
        REPOSITORY_ROOT
        / "tests"
        / "fixtures"
        / "regressions"
        / "gabriel_pair_missing_lnn.json"
    )
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="hierarchy_k1_dump executable")
    parser.add_argument(
        "--missing-lnn-fixture",
        type=Path,
        default=default_fixture,
        help="permanent Gabriel pair outside a fixed local-neighbor proposal",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=DEFAULT_TIMEOUT_SECONDS,
        help="C++ subprocess timeout in seconds",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout <= 0:
        raise ValueError("--timeout must be positive")
    cases = targeted_cases(arguments.missing_lnn_fixture) + affine_cases()
    if len({case.case_id for case in cases}) != len(cases):
        raise AssertionError("hierarchy campaign case identifiers are not unique")
    if max(len(case.source_points) for case in cases) != MAX_POINT_COUNT:
        raise AssertionError("hierarchy campaign does not reach n=14")
    run_differential(arguments.executable, cases, arguments.timeout)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        AssertionError,
        OSError,
        ValueError,
        subprocess.TimeoutExpired,
    ) as error:
        print(f"check_hierarchy_k1: {error}", file=sys.stderr)
        raise SystemExit(2) from error
