from __future__ import annotations

import json
import unittest
from fractions import Fraction

from reference.morsehgp3d_oracle.oracle import CanonicalPoint, run_oracle
from reference.morsehgp3d_oracle.serialization import (
    CONTRACT_DOMAIN,
    binary64_bits,
    canonical_id,
    canonical_json,
    certified_point,
    exact_level,
    exact_rational3,
    serialize_oracle_cuts,
)


class SerializationPrimitiveTests(unittest.TestCase):
    def test_json_and_ids_are_canonical_and_domain_separated(self) -> None:
        left = {"b": [2, 1], "a": 0}
        right = {"a": 0, "b": [2, 1]}
        self.assertEqual(canonical_json(left), canonical_json(right))
        self.assertEqual(canonical_id("CriticalEvent", left), canonical_id("CriticalEvent", right))
        self.assertNotEqual(canonical_id("CriticalEvent", left), canonical_id("MergeNode", left))
        self.assertEqual(len(canonical_id("CriticalEvent", left)), 64)
        self.assertEqual(CONTRACT_DOMAIN, "MorseHGP3D/v2")

    def test_record_versions_do_not_enter_v2_identity_projections(self) -> None:
        version_one = {
            "schema_version": "1.0.0",
            "level": {"schema_version": "1.0.0", "numerator": "1"},
        }
        version_two = {
            "schema_version": "2.0.0",
            "level": {"schema_version": "2.0.0", "numerator": "1"},
        }
        self.assertEqual(
            canonical_id("CriticalEvent", version_one),
            canonical_id("CriticalEvent", version_two),
        )

    def test_exact_values_are_reduced_and_use_squared_units(self) -> None:
        self.assertEqual(
            exact_level(Fraction(10, 4)),
            {
                "schema_version": "2.0.0",
                "numerator": "5",
                "denominator": "2",
                "unit": "input_coordinate_unit_squared",
            },
        )
        self.assertEqual(
            exact_rational3((Fraction(1, 2), Fraction(-1, 3), 0)),
            {
                "schema_version": "2.0.0",
                "x_numerator": "3",
                "y_numerator": "-2",
                "z_numerator": "0",
                "denominator": "6",
                "unit": "input_coordinate_unit",
            },
        )

    def test_binary64_encoding_is_exact_and_normalizes_signed_zero(self) -> None:
        self.assertEqual(binary64_bits(Fraction(1, 2)), "3fe0000000000000")
        self.assertEqual(binary64_bits(-0.0), "0000000000000000")
        with self.assertRaises(ValueError):
            binary64_bits(Fraction(1, 3))

    def test_certified_point_round_trips_through_json(self) -> None:
        record = certified_point(
            CanonicalPoint(0, (Fraction(-1), Fraction(0), Fraction(2)), (7,))
        )
        self.assertEqual(json.loads(canonical_json(record)), record)
        self.assertEqual(record["coordinate_bits"][0], "bff0000000000000")

    def test_separate_cut_artifact_round_trips_every_open_and_closed_level(self) -> None:
        points = [(0, 0, 0), (2, 0, 0), (5, 1, 0), (7, -1, 0)]
        for profile, graph_kind in (
            ("full_pi0", "gamma"),
            ("hgp_reduced", "gabriel"),
        ):
            with self.subTest(profile=profile):
                result = run_oracle(points, 3, profile=profile)
                artifact = serialize_oracle_cuts(result)
                self.assertEqual(json.loads(canonical_json(artifact)), artifact)
                self.assertEqual(artifact["artifact_kind"], "morsehgp3d_oracle_cuts")
                self.assertEqual(len(artifact["orders"]), result.k_eff)
                for order_record, order_result in zip(
                    artifact["orders"], result.orders
                ):
                    self.assertEqual(
                        len(order_record["cuts"]),
                        2 * len(order_result.critical_levels),
                    )
                    self.assertEqual(
                        [
                            (cut["squared_level_exact"], cut["closed"])
                            for cut in order_record["cuts"]
                        ],
                        [
                            (exact_level(level), closed)
                            for level in order_result.critical_levels
                            for closed in (False, True)
                        ],
                    )
                    self.assertTrue(
                        all(
                            cut["graph_kind"] == graph_kind
                            for cut in order_record["cuts"]
                        )
                    )

    def test_cut_artifact_accepts_non_binary64_rational_fixtures(self) -> None:
        result = run_oracle(
            [(Fraction(1, 3), 0, 0), (Fraction(4, 3), 0, 0)],
            1,
            profile="full_pi0",
        )
        artifact = serialize_oracle_cuts(result)
        self.assertEqual(
            artifact["input_points"][0]["coordinates_exact"]["denominator"],
            "3",
        )


if __name__ == "__main__":
    unittest.main()
