from __future__ import annotations

import copy
import hashlib
import json
import unittest
from itertools import combinations
from pathlib import Path

from tools.check_phase15_gabriel_coverage import (
    CoverageInputError,
    analyze_coverage,
    recompute_emitted_fixture_sha256,
)

HEX64 = "0" * 64
ROOT = Path(__file__).resolve().parents[2]
ARCHIVED_REPORT_STEMS = (
    "phase15_gabriel_coverage_e5_c2_g4",
    "phase15_gabriel_coverage_e5_c3_g4",
    "phase15_gabriel_coverage_n50000_c10000_g4",
    "phase15_gabriel_coverage_n50000_c50000_g4",
    "phase15_gabriel_coverage_n10000001_c50000_g4",
    "phase15_gabriel_coverage_n10000001_c100000_g4",
    "phase15_gabriel_coverage_n30000001_c150000_g4",
)
ARCHIVED_CHUNK_PAIRS = (
    (
        "phase15_gabriel_coverage_e5_c2_g4",
        "phase15_gabriel_coverage_e5_c3_g4",
    ),
    (
        "phase15_gabriel_coverage_n50000_c10000_g4",
        "phase15_gabriel_coverage_n50000_c50000_g4",
    ),
    (
        "phase15_gabriel_coverage_n10000001_c50000_g4",
        "phase15_gabriel_coverage_n10000001_c100000_g4",
    ),
)


def _triangle(
    a: int,
    b: int,
    c: int,
    level: int | float,
    status: str = "gabriel_binary64",
    support: int = 3,
) -> dict[str, object]:
    return {
        "a": a,
        "b": b,
        "c": c,
        "squared_level": level,
        "status": status,
        "support_cardinality": support,
    }


def _payload(*, emit_fixture: bool = True) -> dict[str, object]:
    accepted = [
        _triangle(0, 1, 2, 0.5, support=3),
        _triangle(0, 1, 3, 0.5, support=3),
        _triangle(0, 2, 3, 0.5, support=2),
    ]
    ambiguous = _triangle(
        1,
        2,
        3,
        2,
        status="ambiguous_requires_cpu_recertification",
        support=2,
    )
    necessary = accepted[:]
    payload: dict[str, object] = {
        "schema": "morsehgp3d.phase15_delaunay_gabriel_coverage.v1",
        "git_sha": "1" * 40,
        "phase": "15",
        "backend": "ordinary_delaunay_wedge_plus_cuda_g4_aabb_grid",
        "profile": "hgp_reduced",
        "mode": "gabriel_necessary_batch",
        "deployment_status": "diagnostic_sidecar",
        "public_status": "not_claimed",
        "criterion": (
            "triangle_explicit_or_three_facets_connected_at_strictly_lower_level"
        ),
        "proof_scope": {
            "general_position_required": True,
            "complete_Voronoi_nerve_one_skeleton_required": True,
            "Geogram_SoS_topology_exactly_recertified": False,
            "every_regular_Gabriel_triangle_has_at_least_two_Delaunay_edges": True,
            "support_cardinality_two_and_three_covered": True,
            "exact_Gamma2_claimed": False,
            "exact_Gabriel_classification_claimed": False,
        },
        "architecture": {
            "edge_times_point_product_enumerated": False,
            "higher_order_Delaunay_mosaic_materialized": False,
            "ordinary_Delaunay_edges_only": True,
            "canonical_wedges_enumerated_in_bounded_GPU_chunks": True,
            "global_restricted_Gamma2_arena_materialized": False,
            "global_Gabriel_proposal_arena_materialized_for_level_sort": True,
            "same_level_candidates_can_certify_each_other": False,
        },
        "input": {
            "point_count": 4,
            "seed": 7,
            "cpu_workers_requested": 4,
            "cpu_workers_used": 4,
            "point_cloud_sha256": HEX64,
        },
        "delaunay": {
            "engine": "PDEL",
            "cell_count": 1,
            "ordinary_edge_count": 6,
            "maximum_vertex_degree": 3,
            "ordinary_edges_sha256": HEX64,
        },
        "candidate_universe": {
            "generator": "canonical_Delaunay_CSR_wedges",
            "raw_wedge_count": 12,
            "canonical_candidate_count": 4,
            "canonical_wedge_universe_sha256": HEX64,
            "chunk_count": 2,
            "triangle_chunk_vertices": 3,
            "maximum_chunk_candidate_count": 3,
        },
        "classification": {
            "blocked_count": 0,
            "gabriel_binary64_count": 3,
            "ambiguous_safety_count": 1,
            "invalid_count": 0,
            "status_partition_closed": True,
            "visited_cell_count": 4,
            "tested_point_count": 8,
        },
        "coverage": {
            "source_triangle_count": 3,
            "support_two_count": 1,
            "support_three_count": 2,
            "explicit_necessary_accepted_count": 3,
            "strictly_lower_connected_count": 0,
            "coverage_violation_count": 0,
            "accepted_partition_closed": True,
            "binary64_gate_passed": True,
            "accepted_source_sha256": HEX64,
            "necessary_accepted_sha256": HEX64,
            "ambiguous_safety_sha256": HEX64,
            "explicit_safety_batch_count": 4,
            "explicit_safety_batch_sha256": HEX64,
            "explicit_safety_batch_semantics": (
                "necessary_accepted_plus_all_ambiguous"
            ),
            "necessary_records": necessary if emit_fixture else None,
            "ambiguous_safety_records": [ambiguous] if emit_fixture else None,
        },
        "emitted_fixture": None,
    }
    if emit_fixture:
        payload["emitted_fixture"] = {
            "points": [
                [3, 3, 4],
                [-4, 5, -4],
                [-3, 0, 5],
                [-5, -1, 2],
            ],
            "ordinary_delaunay_edges": [
                {"u": u, "v": v}
                for u in range(4)
                for v in range(u + 1, 4)
            ],
            "source_records": [*accepted, ambiguous],
        }
        _seal(payload)
    return payload


def _seal(payload: dict[str, object]) -> None:
    for path, digest in recompute_emitted_fixture_sha256(payload).items():
        section, key = path.split(".", 1)
        target = payload[section]
        assert isinstance(target, dict)
        target[key] = digest


class Phase15GabrielCoverageCheckerTests(unittest.TestCase):
    def test_valid_fixture_replays_strict_lower_plateaus(self) -> None:
        report = analyze_coverage(_payload())

        self.assertTrue(report["diagnostic_completed"])
        self.assertEqual(
            report["counter_invariants"],
            {
                "status_partition_closed": True,
                "accepted_partition_closed": True,
                "support_partition_closed": True,
                "safety_partition_closed": True,
            },
        )
        replay = report["fixture_replay"]
        self.assertEqual(replay["replayed_necessary_count"], 3)
        self.assertEqual(replay["replayed_strictly_lower_connected_count"], 0)
        self.assertEqual(replay["exact_gabriel_triangle_count"], 4)
        self.assertEqual(replay["exact_gabriel_support_two_count"], 2)
        self.assertEqual(replay["exact_gabriel_support_three_count"], 2)
        self.assertTrue(replay["every_exact_gabriel_triangle_emitted"])
        self.assertEqual(
            report["digest_validation"]["mode"],
            "recomputed_from_emitted_fixture",
        )
        self.assertTrue(
            replay["every_accepted_source_is_necessary_or_strictly_lower_connected"]
        )

    def test_valid_massive_summary_may_omit_fixture_records(self) -> None:
        report = analyze_coverage(_payload(emit_fixture=False))

        self.assertTrue(report["diagnostic_completed"])
        self.assertIsNone(report["fixture_replay"])
        self.assertEqual(
            report["digest_validation"]["mode"], "producer_commitments_only"
        )
        self.assertTrue(
            report["digest_validation"][
                "producer_commitments_not_recomputable_without_records"
            ]
        )

    def test_same_plateau_records_cannot_certify_each_other(self) -> None:
        payload = _payload()
        fixture = payload["emitted_fixture"]
        coverage = payload["coverage"]
        assert isinstance(fixture, dict)
        assert isinstance(coverage, dict)
        sources = fixture["source_records"]
        assert isinstance(sources, list)
        sources[3]["status"] = "gabriel_binary64"
        sources[3]["squared_level"] = 0.5
        coverage["necessary_records"] = sources[:]
        coverage["ambiguous_safety_records"] = []
        coverage["source_triangle_count"] = 4
        coverage["support_two_count"] = 2
        coverage["support_three_count"] = 2
        coverage["explicit_necessary_accepted_count"] = 4
        coverage["strictly_lower_connected_count"] = 0
        coverage["explicit_safety_batch_count"] = 4
        classification = payload["classification"]
        assert isinstance(classification, dict)
        classification["gabriel_binary64_count"] = 4
        classification["ambiguous_safety_count"] = 0
        _seal(payload)

        report = analyze_coverage(payload)

        replay = report["fixture_replay"]
        self.assertEqual(replay["replayed_necessary_count"], 4)
        self.assertEqual(replay["replayed_strictly_lower_connected_count"], 0)

        unsafe_equal_level_omission = copy.deepcopy(payload)
        unsafe_coverage = unsafe_equal_level_omission["coverage"]
        assert isinstance(unsafe_coverage, dict)
        unsafe_coverage["necessary_records"] = sources[:3]
        unsafe_coverage["explicit_necessary_accepted_count"] = 3
        unsafe_coverage["strictly_lower_connected_count"] = 1
        unsafe_coverage["explicit_safety_batch_count"] = 3
        _seal(unsafe_equal_level_omission)
        with self.assertRaisesRegex(
            CoverageInputError, "strict-lower DSU replay"
        ):
            analyze_coverage(unsafe_equal_level_omission)

    def test_strictly_lower_connectivity_uses_only_earlier_plateaus(self) -> None:
        payload = _payload()
        fixture = payload["emitted_fixture"]
        coverage = payload["coverage"]
        classification = payload["classification"]
        assert isinstance(fixture, dict)
        assert isinstance(coverage, dict)
        assert isinstance(classification, dict)
        sources = fixture["source_records"]
        assert isinstance(sources, list)
        sources[3]["status"] = "gabriel_binary64"
        coverage["ambiguous_safety_records"] = []
        coverage["source_triangle_count"] = 4
        coverage["support_two_count"] = 2
        coverage["support_three_count"] = 2
        coverage["strictly_lower_connected_count"] = 1
        coverage["explicit_safety_batch_count"] = 3
        classification["gabriel_binary64_count"] = 4
        classification["ambiguous_safety_count"] = 0
        _seal(payload)

        report = analyze_coverage(payload)

        replay = report["fixture_replay"]
        self.assertEqual(replay["replayed_necessary_count"], 3)
        self.assertEqual(replay["replayed_strictly_lower_connected_count"], 1)

    def test_wrong_necessary_record_is_rejected(self) -> None:
        payload = _payload()
        coverage = payload["coverage"]
        assert isinstance(coverage, dict)
        records = coverage["necessary_records"]
        assert isinstance(records, list)
        coverage["necessary_records"] = records[:2]

        with self.assertRaisesRegex(CoverageInputError, "strict-lower DSU replay"):
            analyze_coverage(payload)

    def test_wrong_ambiguity_stream_is_rejected(self) -> None:
        payload = _payload()
        coverage = payload["coverage"]
        assert isinstance(coverage, dict)
        coverage["ambiguous_safety_records"] = []

        with self.assertRaisesRegex(CoverageInputError, "source ambiguities"):
            analyze_coverage(payload)

    def test_non_wedge_source_is_rejected(self) -> None:
        payload = _payload()
        fixture = payload["emitted_fixture"]
        assert isinstance(fixture, dict)
        edges = fixture["ordinary_delaunay_edges"]
        assert isinstance(edges, list)
        fixture["ordinary_delaunay_edges"] = [
            edge
            for edge in edges
            if (edge["u"], edge["v"]) not in {(0, 1), (0, 2)}
        ]

        with self.assertRaisesRegex(CoverageInputError, "not a Delaunay wedge"):
            analyze_coverage(payload)

    def test_counter_partitions_are_rejected_independently(self) -> None:
        mutations = (
            ("classification", "blocked_count", 6, "status counters"),
            ("coverage", "strictly_lower_connected_count", 2, "accepted !="),
            ("coverage", "support_three_count", 3, "support-two"),
            ("coverage", "explicit_safety_batch_count", 5, "safety batch"),
        )
        for section, key, value, message in mutations:
            with self.subTest(section=section, key=key):
                payload = _payload()
                target = payload[section]
                assert isinstance(target, dict)
                target[key] = value
                with self.assertRaisesRegex(CoverageInputError, message):
                    analyze_coverage(payload)

    def test_public_status_assumptions_architecture_and_sha_are_enforced(self) -> None:
        mutations = (
            ("public_status", "exact", "public_status"),
            ("proof_scope", "exact_Gamma2_claimed", True, "exact_Gamma2"),
            (
                "architecture",
                "edge_times_point_product_enumerated",
                True,
                "edge_times_point",
            ),
            (
                "architecture",
                "global_Gabriel_proposal_arena_materialized_for_level_sort",
                False,
                "global_Gabriel",
            ),
            (
                "candidate_universe",
                "chunk_count",
                0,
                "chunk_count must be positive",
            ),
            (
                "input",
                "point_cloud_sha256",
                "ABC",
                "lowercase hexadecimal",
            ),
        )
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                payload = copy.deepcopy(_payload())
                if len(mutation) == 3:
                    key, value, message = mutation
                    payload[key] = value
                else:
                    section, key, value, message = mutation
                    target = payload[section]
                    assert isinstance(target, dict)
                    target[key] = value
                with self.assertRaisesRegex(CoverageInputError, message):
                    analyze_coverage(payload)

    def test_all_seven_fixture_digests_are_recomputed(self) -> None:
        paths = (
            ("input", "point_cloud_sha256"),
            ("delaunay", "ordinary_edges_sha256"),
            ("candidate_universe", "canonical_wedge_universe_sha256"),
            ("coverage", "accepted_source_sha256"),
            ("coverage", "necessary_accepted_sha256"),
            ("coverage", "ambiguous_safety_sha256"),
            ("coverage", "explicit_safety_batch_sha256"),
        )
        for section, key in paths:
            with self.subTest(path=f"{section}.{key}"):
                payload = _payload()
                target = payload[section]
                assert isinstance(target, dict)
                target[key] = "f" * 64
                with self.assertRaisesRegex(
                    CoverageInputError, "does not match fixture recomputation"
                ):
                    analyze_coverage(payload)

    def test_fixture_coordinates_are_finite_binary64_and_capped(self) -> None:
        for coordinate in (float("nan"), float("inf"), "0"):
            with self.subTest(coordinate=coordinate):
                payload = _payload()
                fixture = payload["emitted_fixture"]
                assert isinstance(fixture, dict)
                points = fixture["points"]
                assert isinstance(points, list)
                points[0][0] = coordinate
                with self.assertRaisesRegex(CoverageInputError, "finite"):
                    analyze_coverage(payload)

        payload = _payload()
        input_record = payload["input"]
        universe = payload["candidate_universe"]
        fixture = payload["emitted_fixture"]
        assert isinstance(input_record, dict)
        assert isinstance(universe, dict)
        assert isinstance(fixture, dict)
        input_record["point_count"] = 15
        universe["chunk_count"] = 5
        fixture["points"] = [[0, 0, 0] for _ in range(15)]
        with self.assertRaisesRegex(CoverageInputError, "14-point"):
            analyze_coverage(payload)

    def test_fixture_derived_graph_source_and_support_counts_are_checked(self) -> None:
        mutations = (
            ("delaunay", "ordinary_edge_count", 5, "ordinary_delaunay_edge_count"),
            ("candidate_universe", "raw_wedge_count", 11, "raw_wedge_count"),
        )
        for section, key, value, message in mutations:
            with self.subTest(key=key):
                payload = _payload()
                target = payload[section]
                assert isinstance(target, dict)
                target[key] = value
                with self.assertRaisesRegex(CoverageInputError, message):
                    analyze_coverage(payload)

        payload = _payload()
        universe = payload["candidate_universe"]
        classification = payload["classification"]
        assert isinstance(universe, dict)
        assert isinstance(classification, dict)
        universe["canonical_candidate_count"] = 5
        classification["blocked_count"] = 1
        with self.assertRaisesRegex(CoverageInputError, "canonical_candidate_count"):
            analyze_coverage(payload)

        payload = _payload()
        coverage = payload["coverage"]
        assert isinstance(coverage, dict)
        coverage["support_two_count"] = 2
        coverage["support_three_count"] = 1
        with self.assertRaisesRegex(CoverageInputError, "support_two_count"):
            analyze_coverage(payload)

    def test_exact_gabriel_completeness_and_support_are_checked(self) -> None:
        payload = _payload()
        fixture = payload["emitted_fixture"]
        assert isinstance(fixture, dict)
        sources = fixture["source_records"]
        assert isinstance(sources, list)
        fixture["source_records"] = [sources[0], sources[1], sources[3]]
        with self.assertRaisesRegex(CoverageInputError, "misses exact Gabriel"):
            analyze_coverage(payload)

        payload = _payload()
        fixture = payload["emitted_fixture"]
        assert isinstance(fixture, dict)
        sources = fixture["source_records"]
        assert isinstance(sources, list)
        sources[0]["support_cardinality"] = 2
        with self.assertRaisesRegex(CoverageInputError, "exact oracle"):
            analyze_coverage(payload)

    def test_seed_workers_and_chunk_geometry_are_validated(self) -> None:
        mutations = (
            ("input", "seed", True, "seed must be"),
            ("input", "seed", 1 << 64, "unsigned 64-bit"),
            ("input", "cpu_workers_requested", 0, "must be positive"),
            ("input", "cpu_workers_used", 5, "exceeds"),
            ("candidate_universe", "chunk_count", 1, "point_count/chunk"),
            (
                "candidate_universe",
                "maximum_chunk_candidate_count",
                5,
                "exceeds the universe",
            ),
        )
        for section, key, value, message in mutations:
            with self.subTest(section=section, key=key, value=value):
                payload = _payload()
                target = payload[section]
                assert isinstance(target, dict)
                target[key] = value
                with self.assertRaisesRegex(CoverageInputError, message):
                    analyze_coverage(payload)

    def test_archived_reports_are_source_bound_and_replay_exactly(self) -> None:
        validation = ROOT / "docs" / "validation"
        for stem in ARCHIVED_REPORT_STEMS:
            with self.subTest(stem=stem):
                artifact = validation / f"{stem}.json"
                raw_artifact = artifact.read_bytes()
                actual = analyze_coverage(
                    json.loads(raw_artifact),
                    source_artifact_sha256=hashlib.sha256(raw_artifact).hexdigest(),
                    source_artifact_byte_count=len(raw_artifact),
                )
                expected = json.loads(
                    (validation / f"{stem}_check.json").read_text(
                        encoding="utf-8"
                    )
                )
                self.assertEqual(actual, expected)

    def test_archived_chunk_variants_are_scientifically_identical(self) -> None:
        validation = ROOT / "docs" / "validation"
        invariant_sections = (
            "proof_scope",
            "architecture",
            "input",
            "delaunay",
            "classification",
            "coverage",
            "emitted_fixture",
        )
        invariant_universe_fields = (
            "generator",
            "raw_wedge_count",
            "canonical_candidate_count",
            "canonical_wedge_universe_sha256",
        )
        for left_stem, right_stem in ARCHIVED_CHUNK_PAIRS:
            with self.subTest(left=left_stem, right=right_stem):
                left = json.loads(
                    (validation / f"{left_stem}.json").read_text(encoding="utf-8")
                )
                right = json.loads(
                    (validation / f"{right_stem}.json").read_text(
                        encoding="utf-8"
                    )
                )
                for field in (
                    "schema",
                    "git_sha",
                    "phase",
                    "backend",
                    "profile",
                    "mode",
                    "deployment_status",
                    "public_status",
                    "criterion",
                ):
                    self.assertEqual(left[field], right[field], field)
                for section in invariant_sections:
                    self.assertEqual(left[section], right[section], section)
                for field in invariant_universe_fields:
                    self.assertEqual(
                        left["candidate_universe"][field],
                        right["candidate_universe"][field],
                        f"candidate_universe.{field}",
                    )
                self.assertNotEqual(
                    left["candidate_universe"]["triangle_chunk_vertices"],
                    right["candidate_universe"]["triangle_chunk_vertices"],
                )

    def test_right_triangle_shell_degeneracy_fails_closed(self) -> None:
        fixture_record = json.loads(
            (
                ROOT
                / "tests"
                / "fixtures"
                / "regressions"
                / "delaunay_gabriel_unsupported_degeneracy_right_triangle.json"
            ).read_text(encoding="utf-8")
        )
        payload = _payload()
        fixture = payload["emitted_fixture"]
        assert isinstance(fixture, dict)
        fixture["points"] = fixture_record["points"]
        _seal(payload)

        with self.assertRaisesRegex(
            CoverageInputError,
            "unsupported_degeneracy.*nonminimal_simplex_shell",
        ):
            analyze_coverage(payload)

    def test_extra_non_delaunay_edge_is_rejected_after_exact_recertification(
        self,
    ) -> None:
        fixture_record = json.loads(
            (
                ROOT
                / "tests"
                / "fixtures"
                / "regressions"
                / "delaunay_two_edge_gamma2_counterexample.json"
            ).read_text(encoding="utf-8")
        )
        points = fixture_record["points"]
        tetrahedra = fixture_record["ordinary_delaunay"]["tetrahedra"]
        exact_edges = {
            tuple(sorted(edge))
            for tetrahedron in tetrahedra
            for edge in combinations(tetrahedron, 2)
        }
        extra_edge = (2, 4)
        self.assertNotIn(extra_edge, exact_edges)
        declared_edges = sorted(exact_edges | {extra_edge})

        payload = _payload()
        input_record = payload["input"]
        delaunay = payload["delaunay"]
        universe = payload["candidate_universe"]
        classification = payload["classification"]
        emitted = payload["emitted_fixture"]
        assert isinstance(input_record, dict)
        assert isinstance(delaunay, dict)
        assert isinstance(universe, dict)
        assert isinstance(classification, dict)
        assert isinstance(emitted, dict)
        emitted["points"] = points
        emitted["ordinary_delaunay_edges"] = [
            {"u": edge[0], "v": edge[1]} for edge in declared_edges
        ]
        input_record["point_count"] = len(points)
        delaunay["cell_count"] = len(tetrahedra)
        delaunay["ordinary_edge_count"] = len(declared_edges)
        degrees = [0] * len(points)
        for left, right in declared_edges:
            degrees[left] += 1
            degrees[right] += 1
        delaunay["maximum_vertex_degree"] = max(degrees)
        universe["raw_wedge_count"] = sum(
            degree * (degree - 1) // 2 for degree in degrees
        )
        edge_set = set(declared_edges)
        candidate_count = sum(
            sum(
                tuple(sorted(edge)) in edge_set
                for edge in combinations(triangle, 2)
            )
            >= 2
            for triangle in combinations(range(len(points)), 3)
        )
        universe["canonical_candidate_count"] = candidate_count
        universe["chunk_count"] = 2
        universe["triangle_chunk_vertices"] = 3
        universe["maximum_chunk_candidate_count"] = candidate_count
        classification["blocked_count"] = candidate_count - 4
        _seal(payload)

        with self.assertRaisesRegex(
            CoverageInputError,
            r"ordinary-Delaunay 1-skeleton.*extra=\[\(2, 4\)\]",
        ):
            analyze_coverage(payload)


if __name__ == "__main__":
    unittest.main()
