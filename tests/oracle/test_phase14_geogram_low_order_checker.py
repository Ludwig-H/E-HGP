from __future__ import annotations

import json
import unittest
from fractions import Fraction
from itertools import combinations
from pathlib import Path

from reference.morsehgp3d_oracle.gamma import build_gamma_filtration
from tools.check_phase14_geogram_low_order import (
    DiagnosticInputError,
    _delaunay_endpoint_neighbor_first_incidence_audit,
    _gabriel_fusion_deadline_coverage,
    analyze_diagnostic,
)

POINTS = [
    [0, 0, 0],
    [2, 0, 0],
    [0, 2, 0],
    [0, 0, 2],
]
E5_POINTS = [[0, 0, 7], [0, 9, 6], [1, 4, 0], [0, 0, 1], [4, 1, 2]]
ROOT = Path(__file__).resolve().parents[2]


def _edges() -> list[dict[str, object]]:
    return [
        {"u": 0, "v": 1, "squared_level": "1"},
        {"u": 0, "v": 2, "squared_level": 1},
        {"u": 0, "v": 3, "squared_level": "1/1"},
    ]


def _triangle(
    a: int,
    b: int,
    c: int,
    level: str,
    status: str,
) -> dict[str, object]:
    return {
        "a": a,
        "b": b,
        "c": c,
        "squared_level": level,
        "status": status,
    }


class Phase14GeogramLowOrderCheckerTests(unittest.TestCase):
    def test_exact_ambiguity_replay_and_mandatory_fixtures(self) -> None:
        retained_records = [
            _triangle(
                0,
                1,
                2,
                "2001/1000",
                "ambiguous_requires_cpu_recertification",
            ),
            _triangle(0, 1, 3, "2", "gabriel_binary64"),
            _triangle(0, 2, 3, "2", "gabriel_binary64"),
            _triangle(
                1,
                2,
                3,
                "8/3",
                "ambiguous_requires_cpu_recertification",
            ),
        ]
        payload = {
            "input": {"point_count": 4, "points": POINTS},
            "k1": {
                "selected_edge_count": 3,
                "distinct_level_count": 1,
                "root_squared_distance": 4,
                "root_squared_level": 1,
                "selected_edges": _edges(),
            },
            "k2": {
                "accepted_triangle_count": 2,
                "ambiguous_triangle_count": 2,
                "invalid_triangle_count": 0,
                "retained_record_count": 4,
                "facet_count": 5,
                "final_component_count": 1,
                "useful_union_count": 4,
                "redundant_union_count": 0,
                "distinct_level_count": 1,
                "first_squared_level": 2,
                "root_squared_level": 2,
                "retained_records": retained_records,
                "invalid_records": [],
                "accepted_triangles": retained_records,
            },
            "summaries": {"fixture": "right-tetrahedron"},
        }

        report = analyze_diagnostic(payload)

        self.assertTrue(report["diagnostic_completed"])
        self.assertTrue(
            report["reported_summary_consistency"]["all_checked_fields_consistent"]
        )
        self.assertTrue(report["k1"]["certified_for_this_bounded_input"])
        gpu = report["k2"]["gpu_to_exact_gabriel"]
        self.assertEqual(
            gpu["catalog"]["gabriel_binary64_identity_classification"]["recall"],
            2 / 3,
        )
        self.assertTrue(gpu["catalog"]["catalog_exact_after_ambiguity_recertification"])
        self.assertEqual(gpu["catalog"]["ambiguous_reported_level_correction_count"], 1)
        self.assertTrue(gpu["reported_cut_metrics"]["all_states_exact"])
        self.assertFalse(report["k2"]["surrogate_policy"]["comparable"])
        fixtures = report["mandatory_fixture_audits"]
        self.assertTrue(fixtures["all_expected_invariants_satisfied"])
        self.assertEqual(
            fixtures["gabriel_e5_counterexample"]["all_threshold_metrics"][
                "first_divergence"
            ]["squared_level"],
            "33/2",
        )
        overlap = fixtures["overlap_k2_contract"]["witness_state_metrics"]
        self.assertFalse(overlap["component_partition_exact"])
        self.assertTrue(overlap["covered_point_collections_exact"])
        two_edge = fixtures["delaunay_two_edge_gamma2_counterexample"]
        self.assertEqual(two_edge["witness_squared_level"], "281/4")
        self.assertTrue(
            two_edge["ordinary_delaunay_exact_recertification"][
                "stored_topology_matches_exact_catalog"
            ]
        )
        self.assertEqual(two_edge["exact_gamma_coface_count"], 20)
        self.assertEqual(two_edge["two_edge_candidate_count"], 18)
        self.assertEqual(two_edge["one_edge_candidate_count"], 20)
        self.assertEqual(two_edge["two_edge_coface_recall"], 0.9)
        self.assertEqual(two_edge["omitted_cofaces"], [[1, 2, 4], [2, 4, 5]])
        self.assertEqual(two_edge["missing_active_facets"], [[2, 4]])
        self.assertEqual(
            {
                key: two_edge["two_edge_cut_metrics"]["first_divergence"][key]
                for key in ("squared_level", "boundary")
            },
            {"squared_level": "281/4", "boundary": "closed"},
        )
        self.assertFalse(two_edge["two_edge_cut_metrics"]["all_states_exact"])
        self.assertTrue(two_edge["one_edge_cut_metrics"]["all_states_exact"])
        for candidate in ("two_edge", "one_edge"):
            deadline = two_edge["gabriel_fusion_deadline"][candidate]
            self.assertTrue(deadline["passed"])
            self.assertEqual(deadline["source_count"], 9)
            self.assertEqual(deadline["connected_before_count"], 0)
            self.assertEqual(deadline["connected_at_count"], 9)
            self.assertEqual(deadline["late_count"], 0)
            self.assertEqual(deadline["never_connected_count"], 0)
        witness = two_edge["witness_state_metrics"]
        self.assertFalse(witness["active_entities_exact"])
        self.assertFalse(witness["component_partition_exact"])
        self.assertTrue(witness["covered_point_collections_exact"])
        local = fixtures["delaunay_local_gamma2_counterexample"]
        self.assertEqual(local["witness_squared_level"], "13956479554")
        self.assertTrue(
            local["ordinary_delaunay_exact_recertification"][
                "stored_topology_matches_exact_catalog"
            ]
        )
        self.assertEqual(local["exact_gamma_coface_count"], 56)
        self.assertEqual(
            local["candidate_counts"],
            {
                "two_edge": 42,
                "closed_star": 50,
                "square_clique": 50,
                "link_face_fan": 50,
                "one_edge": 56,
            },
        )
        for candidate in (
            "two_edge",
            "closed_star",
            "square_clique",
            "link_face_fan",
        ):
            self.assertEqual(local["missing_active_facets"][candidate], [[0, 1]])
            self.assertEqual(
                {
                    key: local["cut_metrics"][candidate]["first_divergence"][key]
                    for key in ("squared_level", "boundary")
                },
                {"squared_level": "13956479554", "boundary": "closed"},
            )
            self.assertTrue(
                local["witness_state_metrics"][candidate][
                    "covered_point_collections_exact"
                ]
            )
            self.assertEqual(
                local["causal_omitted_cofaces"][candidate],
                [[0, 1, 4], [0, 1, 6], [0, 1, 7]],
            )
            self.assertFalse(local["cut_metrics"][candidate]["all_states_exact"])
        for candidate, deadline in local["gabriel_fusion_deadline"].items():
            self.assertTrue(deadline["passed"], candidate)
            self.assertEqual(deadline["source_count"], 17, candidate)
            self.assertEqual(deadline["connected_before_count"], 2, candidate)
            self.assertEqual(deadline["connected_at_count"], 15, candidate)
            self.assertEqual(deadline["late_count"], 0, candidate)
            self.assertEqual(deadline["never_connected_count"], 0, candidate)
        self.assertEqual(local["missing_active_facets"]["one_edge"], [])
        self.assertEqual(local["causal_omitted_cofaces"]["one_edge"], [])
        self.assertTrue(local["cut_metrics"]["one_edge"]["all_states_exact"])
        positive = fixtures["delaunay_one_edge_gamma2_positive"]
        self.assertTrue(
            positive["ordinary_delaunay_exact_recertification"][
                "stored_topology_matches_exact_catalog"
            ]
        )
        self.assertEqual(positive["exact_gamma_coface_count"], 84)
        self.assertEqual(positive["one_edge_candidate_count"], 82)
        self.assertEqual(
            positive["omitted_zero_edge_cofaces"],
            [
                {
                    "simplex_point_ids": [0, 1, 4],
                    "squared_level": (
                        "6533652824060214379979565721490/"
                        "616187158871335654299"
                    ),
                },
                {
                    "simplex_point_ids": [5, 6, 8],
                    "squared_level": (
                        "196666935109083477399056026226/"
                        "13800525515901360121"
                    ),
                },
            ],
        )
        self.assertEqual(positive["cut_metrics"]["state_count"], 168)
        self.assertTrue(positive["cut_metrics"]["all_states_exact"])
        positive_deadline = positive["gabriel_fusion_deadline"]
        self.assertTrue(positive_deadline["passed"])
        self.assertEqual(positive_deadline["source_count"], 19)
        self.assertEqual(positive_deadline["connected_before_count"], 2)
        self.assertEqual(positive_deadline["connected_at_count"], 17)
        expected_first_incidence_counts = (
            (two_edge, 60, 58, 2),
            (local, 168, 154, 14),
            (positive, 252, 234, 18),
        )
        for fixture, exhaustive_count, reduced_count, avoided_count in (
            expected_first_incidence_counts
        ):
            first_incidence = fixture[
                "delaunay_endpoint_neighbor_first_incidence"
            ]
            self.assertTrue(first_incidence["every_first_incidence_level_exact"])
            self.assertTrue(
                first_incidence["strict_and_closed_activation_decisions_exact"]
            )
            self.assertEqual(
                first_incidence["full_third_point_evaluation_count"],
                exhaustive_count,
            )
            self.assertEqual(
                first_incidence["endpoint_neighbor_evaluation_count"],
                reduced_count,
            )
            self.assertEqual(
                first_incidence["avoided_third_point_evaluation_count"],
                avoided_count,
            )
            self.assertFalse(
                first_incidence["global_pair_or_coface_catalog_product_claimed"]
            )
            self.assertFalse(first_incidence["terminal_Morse_hierarchy_claimed"])

    def test_gabriel_deadline_accepts_closed_plateau_and_rejects_late_or_never(
        self,
    ) -> None:
        def relation(
            point_ids: tuple[int, int, int], level: int
        ) -> tuple[
            tuple[int, ...], tuple[tuple[int, ...], ...], Fraction
        ]:
            return (
                point_ids,
                tuple(combinations(point_ids, 2)),
                Fraction(level),
            )

        source = (relation((0, 1, 2), 2),)
        same_plateau = (
            relation((0, 1, 3), 2),
            relation((0, 2, 3), 2),
            relation((1, 2, 3), 2),
        )
        at = _gabriel_fusion_deadline_coverage(
            variant_name="same_plateau",
            source_relations=source,
            candidate_relations=same_plateau,
        )
        at_permuted = _gabriel_fusion_deadline_coverage(
            variant_name="same_plateau",
            source_relations=source,
            candidate_relations=tuple(reversed(same_plateau)),
        )
        self.assertTrue(at["passed"])
        self.assertEqual(at["connected_at_count"], 1)
        self.assertEqual(at["decision_sha256"], at_permuted["decision_sha256"])

        before = _gabriel_fusion_deadline_coverage(
            variant_name="before",
            source_relations=source,
            candidate_relations=(relation((0, 1, 2), 1),),
        )
        self.assertTrue(before["passed"])
        self.assertEqual(before["connected_before_count"], 1)

        late = _gabriel_fusion_deadline_coverage(
            variant_name="late",
            source_relations=source,
            candidate_relations=(relation((0, 1, 2), 3),),
        )
        self.assertFalse(late["passed"])
        self.assertEqual(late["late_count"], 1)
        self.assertEqual(late["first_failure"]["decision"], "late")

        never = _gabriel_fusion_deadline_coverage(
            variant_name="never",
            source_relations=source,
            candidate_relations=(),
        )
        self.assertFalse(never["passed"])
        self.assertEqual(never["never_connected_count"], 1)
        self.assertTrue(never["counter_partition_closed"])

    def test_delaunay_endpoint_neighbor_first_incidence_is_permutation_invariant_and_fail_closed(
        self,
    ) -> None:
        fixture = json.loads(
            (
                ROOT
                / "tests"
                / "fixtures"
                / "regressions"
                / "delaunay_two_edge_gamma2_counterexample.json"
            ).read_text(encoding="utf-8")
        )
        points = tuple(
            tuple(Fraction(coordinate) for coordinate in point)
            for point in fixture["points"]
        )
        tetrahedra = tuple(
            tuple(tetrahedron)
            for tetrahedron in fixture["ordinary_delaunay"]["tetrahedra"]
        )
        edges = {
            tuple(sorted(edge))
            for tetrahedron in tetrahedra
            for edge in combinations(tetrahedron, 2)
        }
        baseline = _delaunay_endpoint_neighbor_first_incidence_audit(
            points,
            edges,
            build_gamma_filtration(points, 2),
            path="test.baseline",
        )

        permutation = tuple(reversed(range(len(points))))
        old_to_new = {
            old_point_id: new_point_id
            for new_point_id, old_point_id in enumerate(permutation)
        }
        permuted_points = tuple(points[old_point_id] for old_point_id in permutation)
        permuted_edges = {
            tuple(sorted((old_to_new[left], old_to_new[right])))
            for left, right in edges
        }
        permuted = _delaunay_endpoint_neighbor_first_incidence_audit(
            permuted_points,
            permuted_edges,
            build_gamma_filtration(permuted_points, 2),
            path="test.permuted",
        )
        for field in (
            "facet_pair_count",
            "full_third_point_evaluation_count",
            "endpoint_neighbor_evaluation_count",
            "avoided_third_point_evaluation_count",
            "global_cominimizer_count",
            "retained_global_cominimizer_count",
        ):
            self.assertEqual(baseline[field], permuted[field], field)
        self.assertTrue(permuted["every_first_incidence_level_exact"])

        amputated_edges = edges - {(1, 5)}
        with self.assertRaisesRegex(
            DiagnosticInputError,
            "endpoint-neighbor first incidence mismatch for pair",
        ):
            _delaunay_endpoint_neighbor_first_incidence_audit(
                points,
                amputated_edges,
                build_gamma_filtration(points, 2),
                path="test.amputated",
            )

    def test_invalid_status_entries_remain_outside_the_replayed_relations(self) -> None:
        payload = {
            "input": {"points": POINTS},
            "k1": {"selected_edges": _edges()},
            "k2": {
                "accepted_triangles": [
                    _triangle(0, 1, 2, "2", "gabriel_binary64"),
                    _triangle(0, 1, 3, "2", "degenerate_or_invalid"),
                    _triangle(0, 2, 3, "2", "degenerate_or_invalid"),
                    _triangle(1, 2, 3, "8/3", "degenerate_or_invalid"),
                ],
                "restricted_gamma_records": [
                    _triangle(0, 1, 3, "2", "blocked"),
                    _triangle(0, 2, 3, "2", "gabriel_binary64"),
                    _triangle(
                        1,
                        2,
                        3,
                        "8/3",
                        "ambiguous_requires_cpu_recertification",
                    ),
                ],
            },
        }

        report = analyze_diagnostic(payload)
        catalog = report["k2"]["gpu_to_exact_gabriel"]["catalog"]

        self.assertEqual(
            catalog["binary64_plus_exact_ambiguity_identity_classification"][
                "false_negative"
            ],
            2,
        )
        self.assertFalse(
            report["k2"]["gpu_to_exact_gabriel"]["reported_cut_metrics"][
                "all_states_exact"
            ]
        )
        self.assertEqual(len(catalog["degenerate_or_invalid_triangles"]), 3)
        restricted = report["k2"]["restricted_gamma_to_exact_gamma"]
        self.assertEqual(restricted["coface_universe_classification"]["recall"], 3 / 4)
        self.assertFalse(restricted["recertified_cut_metrics"]["all_states_exact"])
        self.assertLess(
            restricted["recertified_cut_metrics"][
                "component_partition_exact_state_count"
            ],
            restricted["recertified_cut_metrics"]["state_count"],
        )

    def test_e5_exhaustive_restricted_wedges_recover_exact_gamma(self) -> None:
        restricted = [
            _triangle(a, b, c, "0", "blocked") for a, b, c in combinations(range(5), 3)
        ]
        payload = {
            "input": {"points": E5_POINTS},
            "k1": {
                "selected_edges": [
                    {"u": 0, "v": 1, "squared_level": "41/2"},
                    {"u": 0, "v": 2, "squared_level": "33/2"},
                    {"u": 0, "v": 3, "squared_level": "9"},
                    {"u": 0, "v": 4, "squared_level": "21/2"},
                ]
            },
            "k2": {
                "accepted_triangles": [],
                "restricted_gamma_records": restricted,
            },
        }

        report = analyze_diagnostic(payload)
        comparison = report["k2"]["restricted_gamma_to_exact_gamma"]

        self.assertEqual(comparison["coface_universe_classification"]["recall"], 1.0)
        self.assertEqual(comparison["reported_level_mismatch_count"], 10)
        self.assertTrue(comparison["recertified_cut_metrics"]["all_states_exact"])
        self.assertFalse(
            report["k2"]["exact_gabriel_to_exact_gamma"]["cut_metrics"][
                "all_states_exact"
            ]
        )

    def test_explicit_k2_surrogate_comparability_claim_fails_closed(self) -> None:
        payload = {
            "input": {"points": POINTS},
            "k1": {"selected_edges": _edges()},
            "k2": {
                "accepted_triangles": [],
                "comparable_to_surrogate": True,
            },
        }

        with self.assertRaisesRegex(DiagnosticInputError, "surrogate comparability"):
            analyze_diagnostic(payload)


if __name__ == "__main__":
    unittest.main()
