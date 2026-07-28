from __future__ import annotations

import hashlib
import json
import unittest
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations, groupby
from pathlib import Path
from typing import Iterable, Sequence


ROOT = Path(__file__).resolve().parents[2]
FIXTURES = (
    ROOT
    / "tests/fixtures/regressions/surrogate_gabriel_deadline_raw_late_k1.json",
    ROOT
    / "tests/fixtures/regressions/surrogate_gabriel_deadline_raw_late_k2.json",
)
MORTON_BITS_PER_AXIS = 21

Point = tuple[Fraction, Fraction, Fraction]
Triangle = tuple[int, int, int]
TreeEdge = tuple[Fraction, int, int]


@dataclass(frozen=True)
class _Ball:
    center: Point
    squared_radius: Fraction
    support: tuple[int, ...]


@dataclass(frozen=True)
class _GabrielRecord:
    squared_level: Fraction
    triangle: Triangle
    ball: _Ball
    external_power_deltas: tuple[tuple[int, Fraction], ...]


class _DisjointSet:
    def __init__(self, size: int) -> None:
        self.parents = list(range(size))
        self.sizes = [1] * size

    def find(self, value: int) -> int:
        root = value
        while self.parents[root] != root:
            root = self.parents[root]
        while self.parents[value] != value:
            following = self.parents[value]
            self.parents[value] = root
            value = following
        return root

    def union(self, first: int, second: int) -> bool:
        first = self.find(first)
        second = self.find(second)
        if first == second:
            return False
        if self.sizes[first] < self.sizes[second] or (
            self.sizes[first] == self.sizes[second] and second < first
        ):
            first, second = second, first
        self.parents[second] = first
        self.sizes[first] += self.sizes[second]
        return True

    def connects_triangle(self, triangle: Triangle) -> bool:
        root = self.find(triangle[0])
        return all(self.find(point_id) == root for point_id in triangle[1:])


def _fraction(value: object) -> Fraction:
    if isinstance(value, bool):
        raise TypeError("booleans are not exact rational values")
    if isinstance(value, (int, str)):
        return Fraction(value)
    if isinstance(value, dict):
        return Fraction(int(value["numerator"]), int(value["denominator"]))
    raise TypeError(f"unsupported exact rational value: {value!r}")


def _load(path: Path) -> dict[str, object]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise TypeError(f"{path} must contain a JSON object")
    return payload


def _points(payload: dict[str, object]) -> tuple[Point, ...]:
    rows = payload["points"]
    if not isinstance(rows, list):
        raise TypeError("points must be an array")
    result: list[Point] = []
    for row in rows:
        if not isinstance(row, list) or len(row) != 3:
            raise TypeError("each point must have exactly three coordinates")
        point = tuple(_fraction(value) for value in row)
        if any(value.denominator != 1 or abs(value) >= 2**53 for value in point):
            raise ValueError("fixture coordinates must be exact binary64 integers")
        result.append(point)  # type: ignore[arg-type]
    if len(set(result)) != len(result):
        raise ValueError("fixture points must be distinct")
    return tuple(result)


def _subtract(first: Point, second: Point) -> Point:
    return tuple(a - b for a, b in zip(first, second))  # type: ignore[return-value]


def _dot(first: Sequence[Fraction], second: Sequence[Fraction]) -> Fraction:
    return sum((a * b for a, b in zip(first, second)), start=Fraction(0))


def _squared_distance(first: Point, second: Point) -> Fraction:
    delta = _subtract(first, second)
    return _dot(delta, delta)


def _triangle_miniball(points: Sequence[Point], triangle: Triangle) -> _Ball:
    pair_balls: list[_Ball] = []
    for first, second in combinations(triangle, 2):
        center = tuple(
            (points[first][axis] + points[second][axis]) / 2
            for axis in range(3)
        )
        squared_radius = _squared_distance(
            points[first], center  # type: ignore[arg-type]
        )
        if all(
            _squared_distance(
                points[point_id], center  # type: ignore[arg-type]
            )
            <= squared_radius
            for point_id in triangle
        ):
            pair_balls.append(
                _Ball(
                    center=center,  # type: ignore[arg-type]
                    squared_radius=squared_radius,
                    support=(first, second),
                )
            )
    if pair_balls:
        return min(pair_balls, key=lambda ball: (ball.squared_radius, ball.support))

    origin = points[triangle[0]]
    first_vector = _subtract(points[triangle[1]], origin)
    second_vector = _subtract(points[triangle[2]], origin)
    first_norm = _dot(first_vector, first_vector)
    cross_term = _dot(first_vector, second_vector)
    second_norm = _dot(second_vector, second_vector)
    determinant = first_norm * second_norm - cross_term * cross_term
    if determinant == 0:
        raise ValueError("a collinear triangle has no unique three-point support")
    first_coefficient = (
        second_norm * (first_norm - cross_term) / (2 * determinant)
    )
    second_coefficient = (
        first_norm * (second_norm - cross_term) / (2 * determinant)
    )
    center = tuple(
        origin[axis]
        + first_coefficient * first_vector[axis]
        + second_coefficient * second_vector[axis]
        for axis in range(3)
    )
    return _Ball(
        center=center,  # type: ignore[arg-type]
        squared_radius=_squared_distance(origin, center),  # type: ignore[arg-type]
        support=triangle,
    )


def _gabriel_catalog(points: Sequence[Point]) -> tuple[_GabrielRecord, ...]:
    records: list[_GabrielRecord] = []
    for triangle in combinations(range(len(points)), 3):
        ball = _triangle_miniball(points, triangle)
        deltas = tuple(
            (
                point_id,
                _squared_distance(points[point_id], ball.center)
                - ball.squared_radius,
            )
            for point_id in range(len(points))
            if point_id not in triangle
        )
        if all(delta > 0 for _, delta in deltas):
            records.append(
                _GabrielRecord(
                    squared_level=ball.squared_radius,
                    triangle=triangle,
                    ball=ball,
                    external_power_deltas=deltas,
                )
            )
    return tuple(
        sorted(records, key=lambda record: (record.squared_level, record.triangle))
    )


def _sealed_native_pdel_edges(
    payload: dict[str, object],
    points: Sequence[Point],
) -> frozenset[tuple[int, int]] | None:
    """Load PDEL edges only from a content-addressed native sidecar.

    A missing sidecar is represented by ``None`` and must make the positive
    Delaunay-membership test skip.  It must never fall back to a Python
    triangulation or turn the pending fixture metadata into a positive claim.
    """

    provenance = payload["ordinary_delaunay"]
    if not isinstance(provenance, dict):
        raise TypeError("ordinary_delaunay must be an object")
    if provenance.get("provider") != "Geogram":
        raise AssertionError("the only supported Delaunay provider is Geogram")
    if provenance.get("engine") != "PDEL":
        raise AssertionError("the only supported Delaunay engine is PDEL")
    if provenance.get("fallback_allowed") is not False:
        raise AssertionError("a Delaunay fallback must remain forbidden")

    native_artifact = provenance.get("native_artifact")
    if not isinstance(native_artifact, dict):
        raise TypeError("ordinary_delaunay.native_artifact must be an object")
    if (
        native_artifact.get("schema")
        != "morsehgp3d.phase15_delaunay_gabriel_fusion_guardrails.v1"
    ):
        raise AssertionError("the native PDEL artifact schema is not the guarded one")
    status = provenance.get("certification_status")
    if status == "pending_native_sealed_artifact":
        if native_artifact.get("path") is not None:
            raise AssertionError("a pending native artifact cannot have a path")
        if native_artifact.get("sha256") is not None:
            raise AssertionError("a pending native artifact cannot have a digest")
        return None
    if status != "sealed_native_artifact":
        raise AssertionError("unknown native PDEL certification status")

    relative_path = native_artifact.get("path")
    expected_sha256 = native_artifact.get("sha256")
    expected_schema = native_artifact.get("schema")
    if not isinstance(relative_path, str) or not relative_path:
        raise AssertionError("a sealed native artifact needs a relative path")
    if Path(relative_path).is_absolute():
        raise AssertionError("the sealed native artifact path must be relative")
    if (
        not isinstance(expected_sha256, str)
        or len(expected_sha256) != 64
        or any(character not in "0123456789abcdef" for character in expected_sha256)
    ):
        raise AssertionError("a sealed native artifact needs a lowercase SHA-256")
    if not isinstance(expected_schema, str) or not expected_schema:
        raise AssertionError("a sealed native artifact needs an expected schema")

    artifact_path = (ROOT / relative_path).resolve()
    try:
        artifact_path.relative_to(ROOT.resolve())
    except ValueError as error:
        raise AssertionError(
            "the native artifact must stay inside the repository"
        ) from error
    artifact_bytes = artifact_path.read_bytes()
    if hashlib.sha256(artifact_bytes).hexdigest() != expected_sha256:
        raise AssertionError("the native PDEL artifact SHA-256 does not match")
    artifact = json.loads(artifact_bytes)
    if not isinstance(artifact, dict):
        raise TypeError("the native PDEL artifact must contain a JSON object")
    if artifact.get("schema") != expected_schema:
        raise AssertionError("the native PDEL artifact schema does not match")
    delaunay = artifact.get("delaunay")
    if not isinstance(delaunay, dict) or delaunay.get("engine") != "PDEL":
        raise AssertionError("the sealed native artifact was not produced by PDEL")
    input_record = artifact.get("input")
    if (
        not isinstance(input_record, dict)
        or input_record.get("point_count") != len(points)
    ):
        raise AssertionError("the native artifact point count does not match")
    emitted = artifact.get("emitted_fixture")
    if not isinstance(emitted, dict):
        raise AssertionError("the native artifact must embed its fixture records")
    emitted_points = emitted.get("points")
    expected_points = [[int(value) for value in point] for point in points]
    if emitted_points != expected_points:
        raise AssertionError("the native artifact point cloud does not match")
    raw_edges = emitted.get("ordinary_delaunay_edges")
    if not isinstance(raw_edges, list):
        raise TypeError("the native artifact must embed its ordinary PDEL edges")
    edges: set[tuple[int, int]] = set()
    for raw_edge in raw_edges:
        if not isinstance(raw_edge, dict):
            raise TypeError("each native PDEL edge must be an object")
        first = raw_edge.get("u")
        second = raw_edge.get("v")
        if (
            not isinstance(first, int)
            or isinstance(first, bool)
            or not isinstance(second, int)
            or isinstance(second, bool)
            or not (0 <= first < second < len(points))
        ):
            raise AssertionError("a native PDEL edge is not canonical")
        edge = (first, second)
        if edge in edges:
            raise AssertionError("the native PDEL artifact contains duplicate edges")
        edges.add(edge)
    if len(edges) != delaunay.get("ordinary_edge_count"):
        raise AssertionError("the native PDEL edge count does not match")
    return frozenset(edges)


def _morton_order(
    points: Sequence[Point],
) -> tuple[tuple[int, ...], tuple[int, ...]]:
    canonical_to_source = tuple(sorted(range(len(points)), key=points.__getitem__))
    lower = tuple(min(point[axis] for point in points) for axis in range(3))
    upper = tuple(max(point[axis] for point in points) for axis in range(3))
    grid_size = 1 << MORTON_BITS_PER_AXIS
    codes: list[int] = []
    for source_id in canonical_to_source:
        bins: list[int] = []
        for axis in range(3):
            if lower[axis] == upper[axis]:
                quantized = 0
            elif points[source_id][axis] == upper[axis]:
                quantized = grid_size - 1
            else:
                scaled = (
                    (points[source_id][axis] - lower[axis]) * grid_size
                    / (upper[axis] - lower[axis])
                )
                quantized = scaled.numerator // scaled.denominator
            bins.append(quantized)
        code = 0
        for bit in range(MORTON_BITS_PER_AXIS):
            output_bit = 3 * bit
            code |= ((bins[0] >> bit) & 1) << (output_bit + 2)
            code |= ((bins[1] >> bit) & 1) << (output_bit + 1)
            code |= ((bins[2] >> bit) & 1) << output_bit
        codes.append(code)
    morton_canonical_ids = sorted(
        range(len(points)), key=lambda point_id: (codes[point_id], point_id)
    )
    morton_source_ids = tuple(
        canonical_to_source[point_id] for point_id in morton_canonical_ids
    )
    return canonical_to_source, morton_source_ids


def _surrogate_tree(points: Sequence[Point], order: int) -> tuple[TreeEdge, ...]:
    canonical_to_source, morton_source_ids = _morton_order(points)
    source_to_canonical = {
        source_id: canonical_id
        for canonical_id, source_id in enumerate(canonical_to_source)
    }
    morton_canonical_ids = tuple(
        source_to_canonical[source_id] for source_id in morton_source_ids
    )
    core_distances: dict[int, Fraction] = {}
    rank_neighbors: dict[int, int] = {}
    for canonical_id, source_id in enumerate(canonical_to_source):
        neighbors = sorted(
            (
                _squared_distance(points[source_id], points[other_source_id]),
                source_to_canonical[other_source_id],
            )
            for other_source_id in range(len(points))
            if other_source_id != source_id
        )
        core_distances[canonical_id] = neighbors[order - 1][0]
        rank_neighbors[canonical_id] = neighbors[order - 1][1]

    proposed: dict[tuple[int, int], Fraction] = {}

    def propose(first: int, second: int) -> None:
        endpoints = (min(first, second), max(first, second))
        squared_distance = _squared_distance(
            points[canonical_to_source[first]],
            points[canonical_to_source[second]],
        )
        weight = max(
            squared_distance,
            core_distances[first],
            core_distances[second],
        )
        previous = proposed.get(endpoints)
        proposed[endpoints] = weight if previous is None else min(previous, weight)

    for position, canonical_id in enumerate(morton_canonical_ids):
        if position != 0:
            propose(canonical_id, morton_canonical_ids[position - 1])
        propose(canonical_id, rank_neighbors[canonical_id])

    components = _DisjointSet(len(points))
    canonical_tree: list[TreeEdge] = []
    for (first, second), weight in sorted(
        proposed.items(), key=lambda item: (item[1], item[0])
    ):
        if components.union(first, second):
            canonical_tree.append((weight, first, second))
    if len(canonical_tree) != len(points) - 1:
        raise AssertionError("the surrogate graph did not produce a spanning tree")

    source_tree = [
        (
            weight,
            min(canonical_to_source[first], canonical_to_source[second]),
            max(canonical_to_source[first], canonical_to_source[second]),
        )
        for weight, first, second in canonical_tree
    ]
    return tuple(sorted(source_tree, key=lambda edge: (edge[0], edge[1], edge[2])))


def _first_connection_raw_weight(
    point_count: int, tree: Sequence[TreeEdge], triangle: Triangle
) -> Fraction | None:
    components = _DisjointSet(point_count)
    for weight, plateau in groupby(tree, key=lambda edge: edge[0]):
        for _, first, second in plateau:
            components.union(first, second)
        if components.connects_triangle(triangle):
            return weight
    return None


def _overlay_classifications(
    point_count: int,
    tree: Sequence[TreeEdge],
    catalog: Sequence[_GabrielRecord],
) -> tuple[dict[Triangle, str], dict[Triangle, str]]:
    raw_components = _DisjointSet(point_count)
    corrected_components = _DisjointSet(point_count)
    raw: dict[Triangle, str] = {}
    corrected: dict[Triangle, str] = {}
    tree_position = 0

    for source_level, source_plateau_iterator in groupby(
        catalog, key=lambda record: record.squared_level
    ):
        source_plateau = tuple(source_plateau_iterator)
        while (
            tree_position < len(tree)
            and tree[tree_position][0] / 4 < source_level
        ):
            _, first, second = tree[tree_position]
            raw_components.union(first, second)
            corrected_components.union(first, second)
            tree_position += 1

        raw_before = {
            record.triangle: raw_components.connects_triangle(record.triangle)
            for record in source_plateau
        }
        corrected_before = {
            record.triangle: corrected_components.connects_triangle(
                record.triangle
            )
            for record in source_plateau
        }

        while (
            tree_position < len(tree)
            and tree[tree_position][0] / 4 == source_level
        ):
            _, first, second = tree[tree_position]
            raw_components.union(first, second)
            corrected_components.union(first, second)
            tree_position += 1

        for record in source_plateau:
            triangle = record.triangle
            if raw_before[triangle]:
                raw[triangle] = "before"
            elif raw_components.connects_triangle(triangle):
                raw[triangle] = "at"
            else:
                raw[triangle] = "late"

        for record in source_plateau:
            triangle = record.triangle
            if not corrected_components.connects_triangle(triangle):
                corrected_components.union(triangle[0], triangle[1])
                corrected_components.union(triangle[0], triangle[2])
        for record in source_plateau:
            triangle = record.triangle
            if not corrected_components.connects_triangle(triangle):
                raise AssertionError("the correction overlay failed its postcondition")
            corrected[triangle] = (
                "before" if corrected_before[triangle] else "at"
            )

    return raw, corrected


class Phase15SurrogateGabrielDeadlineRegressionTests(unittest.TestCase):
    def test_exact_rational_gabriel_geometry(self) -> None:
        for path in FIXTURES:
            with self.subTest(fixture=path.name):
                payload = _load(path)
                points = _points(payload)
                domain = payload["domain_checks"]
                source = payload["gabriel_triangle"]
                self.assertIsInstance(domain, dict)
                self.assertIsInstance(source, dict)
                assert isinstance(domain, dict)
                assert isinstance(source, dict)

                triangle = tuple(int(value) for value in source["source_point_ids"])
                self.assertEqual(len(triangle), 3)
                target_ball = _triangle_miniball(
                    points, triangle  # type: ignore[arg-type]
                )
                self.assertEqual(
                    target_ball.squared_radius,
                    _fraction(source["squared_level_exact"]),
                )
                self.assertEqual(
                    target_ball.center,
                    tuple(_fraction(value) for value in source["center_exact"]),
                )
                self.assertEqual(
                    target_ball.support,
                    tuple(
                        int(value)
                        for value in source["minimal_support_source_point_ids"]
                    ),
                )
                self.assertEqual(
                    len(target_ball.support), source["support_cardinality"]
                )

                external_deltas = tuple(
                    (
                        point_id,
                        _squared_distance(points[point_id], target_ball.center)
                        - target_ball.squared_radius,
                    )
                    for point_id in range(len(points))
                    if point_id not in triangle
                )
                expected_deltas = tuple(
                    (int(point_id), _fraction(delta))
                    for point_id, delta in source[
                        "strict_external_power_deltas_exact"
                    ]
                )
                self.assertEqual(external_deltas, expected_deltas)
                self.assertTrue(all(delta > 0 for _, delta in external_deltas))

                catalog = _gabriel_catalog(points)
                self.assertEqual(
                    len(catalog), domain["exact_regular_gabriel_triangle_count"]
                )
                target_records = [
                    record for record in catalog if record.triangle == triangle
                ]
                self.assertEqual(len(target_records), 1)
                self.assertEqual(target_records[0].ball, target_ball)

    def test_geogram_pdel_membership_requires_a_sealed_native_artifact(
        self,
    ) -> None:
        pending: list[str] = []
        for path in FIXTURES:
            with self.subTest(fixture=path.name):
                payload = _load(path)
                points = _points(payload)
                source = payload["gabriel_triangle"]
                provenance = payload["ordinary_delaunay"]
                self.assertIsInstance(source, dict)
                self.assertIsInstance(provenance, dict)
                assert isinstance(source, dict)
                assert isinstance(provenance, dict)
                self.assertEqual(
                    provenance["entrypoint"],
                    'GEO::Delaunay::create(3,"PDEL")',
                )
                self.assertEqual(
                    provenance["required_artifact_checks"],
                    [
                        "point_cloud_matches_fixture",
                        "engine_is_PDEL",
                        "target_has_at_least_two_ordinary_Delaunay_edges",
                        (
                            "every_rational_regular_Gabriel_source_has_at_least_"
                            "two_ordinary_Delaunay_edges"
                        ),
                    ],
                )

                edges = _sealed_native_pdel_edges(payload, points)
                if edges is None:
                    pending.append(path.name)
                    continue
                triangle = tuple(
                    int(value) for value in source["source_point_ids"]
                )
                target_edge_count = sum(
                    tuple(sorted(edge)) in edges
                    for edge in combinations(triangle, 2)
                )
                self.assertGreaterEqual(target_edge_count, 2)
                catalog = _gabriel_catalog(points)
                self.assertTrue(
                    all(
                        sum(
                            tuple(sorted(edge)) in edges
                            for edge in combinations(record.triangle, 2)
                        )
                        >= 2
                        for record in catalog
                    )
                )
        if pending:
            self.skipTest(
                "native Geogram/PDEL certification pending for: "
                + ", ".join(pending)
            )

    def test_raw_lateness_and_closed_overlay_repair_are_recomputed(self) -> None:
        for path in FIXTURES:
            with self.subTest(fixture=path.name):
                payload = _load(path)
                points = _points(payload)
                source = payload["gabriel_triangle"]
                surrogate = payload["surrogate"]
                correction = payload["correction"]
                expected = payload["expected"]
                self.assertIsInstance(source, dict)
                self.assertIsInstance(surrogate, dict)
                self.assertIsInstance(correction, dict)
                self.assertIsInstance(expected, dict)
                assert isinstance(source, dict)
                assert isinstance(surrogate, dict)
                assert isinstance(correction, dict)
                assert isinstance(expected, dict)

                triangle = tuple(int(value) for value in source["source_point_ids"])
                source_level = _fraction(source["squared_level_exact"])
                order = int(surrogate["order"])
                canonical_to_source, morton_source = _morton_order(points)
                self.assertEqual(
                    canonical_to_source,
                    tuple(
                        int(value)
                        for value in surrogate["canonical_to_source_point_ids"]
                    ),
                )
                expected_morton = tuple(
                    int(value) for value in surrogate["morton_source_point_ids"]
                )
                self.assertEqual(morton_source, expected_morton)
                self.assertEqual(sorted(expected_morton), list(range(len(points))))
                self.assertTrue(
                    all(
                        0 <= point_id < len(points)
                        for point_id in expected_morton
                    )
                )

                tree = _surrogate_tree(points, order)
                expected_tree = tuple(
                    (_fraction(weight), int(first), int(second))
                    for weight, first, second in surrogate["tree_edges"]
                )
                self.assertEqual(tree, expected_tree)
                first_raw_weight = _first_connection_raw_weight(
                    len(points), tree, triangle  # type: ignore[arg-type]
                )
                self.assertEqual(
                    first_raw_weight,
                    _fraction(surrogate["raw_first_connection_squared_weight_exact"]),
                )
                self.assertIsNotNone(first_raw_weight)
                assert first_raw_weight is not None
                first_raw_level = first_raw_weight / 4
                self.assertEqual(
                    first_raw_level,
                    _fraction(surrogate["raw_first_connection_squared_level_exact"]),
                )
                self.assertGreater(first_raw_level, source_level)

                catalog = _gabriel_catalog(points)
                self.assertEqual(len(catalog), correction["source_catalog_count"])
                raw, corrected = _overlay_classifications(
                    len(points), tree, catalog
                )
                self.assertEqual(raw[triangle], expected["raw_classification"])
                self.assertEqual(
                    corrected[triangle], expected["corrected_classification"]
                )
                self.assertEqual(raw[triangle], "late")
                self.assertEqual(corrected[triangle], "at")
                self.assertEqual(
                    _fraction(
                        correction[
                            "corrected_first_connection_squared_level_exact"
                        ]
                    ),
                    source_level,
                )
                self.assertEqual(
                    correction["plateau_convention"],
                    "closed_apply_all_equal_level_relations_before_query",
                )
                self.assertEqual(
                    surrogate["point_component_adapter"],
                    "point_component_clique_lift_v1",
                )
                self.assertEqual(
                    expected["guardrail_scope"],
                    "unilateral_gabriel_fusion_deadline_not_gamma2_or_morse_exactness",
                )


if __name__ == "__main__":
    unittest.main()
