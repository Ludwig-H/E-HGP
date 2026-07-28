from __future__ import annotations

import copy
import unittest
from fractions import Fraction

from tools.check_phase15_gabriel_fusion_guardrail import (
    EXACT_LEVEL_CONVENTION,
    INPUT_SCHEMA,
    NATIVE_REPRESENTATION,
    POINT_LIFT_ADAPTER,
    POINT_LIFT_SEMANTICS,
    POINT_TREE_REPRESENTATION,
    SOURCE_DOMAIN,
    GuardrailInputError,
    analyze_guardrail,
)


def _level(value: int | Fraction | str) -> str:
    return str(Fraction(value))


def _triangle(
    triangle_id: str,
    point_ids: tuple[int, int, int],
    level: int | Fraction | str,
    *,
    geometry_status: str = "regular_exact",
) -> dict[str, object]:
    return {
        "triangle_id": triangle_id,
        "point_ids": list(point_ids),
        "squared_level_exact": _level(level),
        "geometry_status": geometry_status,
    }


def _event(
    event_id: str,
    level: int | Fraction | str,
    facets: list[tuple[int, int]],
) -> dict[str, object]:
    return {
        "event_id": event_id,
        "squared_level_exact": _level(level),
        "facets": [list(facet) for facet in sorted(facets)],
    }


def _native_payload(
    triangles: list[dict[str, object]],
    events: list[dict[str, object]],
    *,
    point_count: int,
) -> dict[str, object]:
    triangles.sort(
        key=lambda item: (
            Fraction(str(item["squared_level_exact"])),
            str(item["triangle_id"]),
            tuple(item["point_ids"]),  # type: ignore[arg-type]
        )
    )
    events.sort(
        key=lambda item: (
            Fraction(str(item["squared_level_exact"])),
            str(item["event_id"]),
        )
    )
    return {
        "schema": INPUT_SCHEMA,
        "phase": "15",
        "point_count": point_count,
        "source_domain": SOURCE_DOMAIN,
        "source_level_convention": EXACT_LEVEL_CONVENTION,
        "source_triangles": triangles,
        "candidate": {
            "representation": NATIVE_REPRESENTATION,
            "level_convention": EXACT_LEVEL_CONVENTION,
            "events": events,
        },
    }


def _point_tree_payload(
    *,
    point_count: int = 3,
    source_level: int | Fraction | str = 2,
) -> dict[str, object]:
    return {
        "schema": INPUT_SCHEMA,
        "phase": "15",
        "point_count": point_count,
        "source_domain": SOURCE_DOMAIN,
        "source_level_convention": EXACT_LEVEL_CONVENTION,
        "source_triangles": [_triangle("T012", (0, 1, 2), source_level)],
        "candidate": {
            "representation": POINT_TREE_REPRESENTATION,
            "level_convention": "exact_squared_distance",
            "adapter": {
                "adapter_id": POINT_LIFT_ADAPTER,
                "component_lift": POINT_LIFT_SEMANTICS,
                "input_level_convention": "exact_squared_distance",
                "output_level_convention": EXACT_LEVEL_CONVENTION,
                "level_conversion": "divide_by_4",
                "level_conversion_documentation": (
                    "Divide every exact squared point distance by four before "
                    "the closed pair-facet clique-lift query."
                ),
            },
            "tree_edges": [
                {"edge_id": "e01", "u": 0, "v": 1, "weight_exact": "4"},
                {"edge_id": "e12", "u": 1, "v": 2, "weight_exact": "8"},
            ],
        },
    }


class Phase15GabrielFusionGuardrailCheckerTests(unittest.TestCase):
    def test_early_connection_is_accepted(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 2)],
            [_event("e", 1, [(0, 1), (0, 2), (1, 2)])],
            point_count=3,
        )

        report = analyze_guardrail(payload)

        self.assertTrue(report["guardrail_passed"])
        self.assertEqual(report["counts"]["before_count"], 1)
        self.assertEqual(report["counts"]["at_count"], 0)
        self.assertEqual(report["records"][0]["classification"], "before")
        self.assertEqual(
            report["records"][0]["candidate_connection_squared_level_exact"],
            "1",
        )

    def test_equal_connection_uses_closed_boundary(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 2)],
            [_event("e", 2, [(0, 1), (0, 2), (1, 2)])],
            point_count=3,
        )

        report = analyze_guardrail(payload)

        self.assertTrue(report["guardrail_passed"])
        self.assertEqual(report["counts"]["at_count"], 1)
        self.assertEqual(report["boundary"], "closed")
        self.assertTrue(
            report["records"][0]["witness"][
                "closed_plateau_applied_before_query"
            ]
        )
        self.assertEqual(report["first_at_level_witness"]["triangle_id"], "T012")

    def test_entire_plateau_is_unioned_before_query(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 5)],
            [
                _event("e0", 5, [(0, 1), (0, 2)]),
                _event("e1", 5, [(0, 2), (1, 2)]),
            ],
            point_count=3,
        )

        report = analyze_guardrail(payload)

        self.assertTrue(report["guardrail_passed"])
        self.assertEqual(report["counts"]["at_count"], 1)
        witness = report["records"][0]["witness"]
        self.assertEqual(
            witness["pair_connection_squared_levels_exact"], ["5", "5", "5"]
        )
        self.assertTrue(witness["connection_level_is_pairwise_maximum"])
        self.assertEqual(report["candidate_audit"]["candidate_plateau_count"], 1)

    def test_post_plateau_source_is_before_while_pre_plateau_source_is_late(
        self,
    ) -> None:
        payload = _native_payload(
            [
                _triangle("pre", (0, 1, 2), Fraction(9, 2)),
                _triangle("post", (3, 4, 5), 6),
            ],
            [
                _event("pre-e", 5, [(0, 1), (0, 2), (1, 2)]),
                _event("post-e", 5, [(3, 4), (3, 5), (4, 5)]),
            ],
            point_count=6,
        )

        report = analyze_guardrail(payload)

        by_id = {record["triangle_id"]: record for record in report["records"]}
        self.assertEqual(by_id["pre"]["classification"], "late")
        self.assertEqual(by_id["post"]["classification"], "before")
        self.assertFalse(report["guardrail_passed"])

    def test_late_connection_fails_with_canonical_witness(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 2)],
            [_event("e", 3, [(0, 1), (0, 2), (1, 2)])],
            point_count=3,
        )

        report = analyze_guardrail(payload)

        self.assertFalse(report["guardrail_passed"])
        self.assertEqual(report["counts"]["late_count"], 1)
        witness = report["first_failure_witness"]
        self.assertEqual(witness["triangle_id"], "T012")
        self.assertEqual(witness["classification"], "late")
        self.assertEqual(witness["candidate_connection_squared_level_exact"], "3")

    def test_never_connected_fails_and_reports_final_components(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 2)],
            [_event("e", 1, [(0, 1), (0, 2)])],
            point_count=3,
        )

        report = analyze_guardrail(payload)

        self.assertFalse(report["guardrail_passed"])
        self.assertEqual(report["counts"]["never_count"], 1)
        record = report["records"][0]
        self.assertIsNone(record["candidate_connection_squared_level_exact"])
        labels = record["witness"]["final_component_labels"]
        self.assertEqual(labels[0], labels[1])
        self.assertNotEqual(labels[0], labels[2])

    def test_five_way_partition_closes_and_unsupported_degeneracy_fails_closed(
        self,
    ) -> None:
        payload = _native_payload(
            [
                _triangle("before", (0, 1, 2), 2),
                _triangle("at", (3, 4, 5), 2),
                _triangle("late", (6, 7, 8), 2),
                _triangle("never", (9, 10, 11), 2),
                _triangle(
                    "unsupported",
                    (12, 12, 14),
                    2,
                    geometry_status="unsupported_degeneracy",
                ),
            ],
            [
                _event("before-e", 1, [(0, 1), (0, 2), (1, 2)]),
                _event("at-e", 2, [(3, 4), (3, 5), (4, 5)]),
                _event("late-e", 3, [(6, 7), (6, 8), (7, 8)]),
                _event("never-e", 3, [(9, 10), (9, 11)]),
            ],
            point_count=15,
        )

        report = analyze_guardrail(payload)

        counts = report["counts"]
        self.assertEqual(
            {name: counts[f"{name}_count"] for name in (
                "before",
                "at",
                "late",
                "never",
                "unsupported",
            )},
            {"before": 1, "at": 1, "late": 1, "never": 1, "unsupported": 1},
        )
        self.assertEqual(counts["source_triangle_count"], 5)
        self.assertEqual(counts["supported_triangle_count"], 4)
        self.assertEqual(counts["satisfied_count"], 2)
        self.assertEqual(counts["violation_count"], 3)
        self.assertTrue(report["invariants"]["classification_partition_closed"])
        self.assertTrue(report["invariants"]["supported_partition_closed"])
        unsupported = next(
            record
            for record in report["records"]
            if record["classification"] == "unsupported"
        )
        self.assertEqual(unsupported["witness"]["kind"], "unsupported_degeneracy")

    def test_digests_and_optional_producer_claims_are_recomputed(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 2)],
            [_event("e", 1, [(0, 1), (0, 2), (1, 2)])],
            point_count=3,
        )
        first = analyze_guardrail(payload)
        digests = first["digests"]
        payload["claims"] = {
            "normalized_input_sha256": digests["normalized_input_sha256"],
            "classification_sha256": digests["classification_sha256"],
            "witness_sha256": digests["witness_sha256"],
            "counts": first["counts"],
            "guardrail_passed": first["guardrail_passed"],
        }

        second = analyze_guardrail(payload)

        self.assertTrue(second["claim_validation"]["producer_claims_present"])
        self.assertTrue(second["claim_validation"]["producer_claims_verified"])
        self.assertRegex(second["digests"]["classification_sha256"], r"^[0-9a-f]{64}$")
        self.assertRegex(second["digests"]["witness_sha256"], r"^[0-9a-f]{64}$")

        tampered = copy.deepcopy(payload)
        tampered["claims"]["classification_sha256"] = "0" * 64
        with self.assertRaisesRegex(
            GuardrailInputError,
            "claims.classification_sha256 does not match recomputation",
        ):
            analyze_guardrail(tampered)

    def test_point_tree_requires_explicit_clique_lift_and_exact_conversion(
        self,
    ) -> None:
        payload = _point_tree_payload()

        report = analyze_guardrail(payload)

        self.assertTrue(report["guardrail_passed"])
        self.assertEqual(report["counts"]["at_count"], 1)
        self.assertEqual(
            report["records"][0]["candidate_connection_squared_level_exact"],
            "2",
        )
        self.assertFalse(report["candidate_audit"]["native_k2_facet_domain"])
        self.assertTrue(report["candidate_audit"]["point_component_lift_used"])
        self.assertFalse(report["candidate_audit"]["gamma2_exactness_claimed"])
        self.assertEqual(
            report["scientific_scope"],
            "explicit_point_component_clique_lift_proxy_not_native_gamma2",
        )

        missing = copy.deepcopy(payload)
        del missing["candidate"]["adapter"]
        with self.assertRaisesRegex(GuardrailInputError, "candidate.adapter"):
            analyze_guardrail(missing)

    def test_point_tree_wrong_level_convention_fails_closed(self) -> None:
        payload = _point_tree_payload()
        payload["candidate"]["adapter"]["level_conversion"] = "identity"

        with self.assertRaisesRegex(
            GuardrailInputError,
            "incompatible input level convention or level conversion",
        ):
            analyze_guardrail(payload)

        payload = _point_tree_payload()
        payload["candidate"]["level_convention"] = "binary64_squared_distance"
        with self.assertRaisesRegex(
            GuardrailInputError,
            "candidate.level_convention must be 'exact_squared_distance'",
        ):
            analyze_guardrail(payload)

    def test_invalid_point_tree_cycle_is_rejected(self) -> None:
        payload = _point_tree_payload(point_count=4)
        payload["candidate"]["tree_edges"] = [
            {"edge_id": "e01", "u": 0, "v": 1, "weight_exact": "4"},
            {"edge_id": "e02", "u": 0, "v": 2, "weight_exact": "4"},
            {"edge_id": "e12", "u": 1, "v": 2, "weight_exact": "4"},
        ]

        with self.assertRaisesRegex(GuardrailInputError, "contains a cycle"):
            analyze_guardrail(payload)

    def test_binary64_level_or_weight_is_rejected(self) -> None:
        payload = _native_payload(
            [_triangle("T012", (0, 1, 2), 2)],
            [_event("e", 1, [(0, 1), (0, 2), (1, 2)])],
            point_count=3,
        )
        payload["source_triangles"][0]["squared_level_exact"] = 2.0
        with self.assertRaisesRegex(
            GuardrailInputError, "binary64 floats are forbidden"
        ):
            analyze_guardrail(payload)

        payload = _point_tree_payload()
        payload["candidate"]["tree_edges"][0]["weight_exact"] = 4.0
        with self.assertRaisesRegex(
            GuardrailInputError, "binary64 floats are forbidden"
        ):
            analyze_guardrail(payload)


if __name__ == "__main__":
    unittest.main()
