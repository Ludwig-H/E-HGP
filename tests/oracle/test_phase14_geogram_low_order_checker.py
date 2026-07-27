from __future__ import annotations

import unittest

from tools.check_phase14_geogram_low_order import (
    DiagnosticInputError,
    analyze_diagnostic,
)

POINTS = [
    [0, 0, 0],
    [2, 0, 0],
    [0, 2, 0],
    [0, 0, 2],
]


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
                "facet_count": 5,
                "final_component_count": 1,
                "useful_union_count": 4,
                "redundant_union_count": 0,
                "distinct_level_count": 1,
                "first_squared_level": 2,
                "root_squared_level": 2,
                "accepted_triangles": [
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
                ],
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
                ]
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
