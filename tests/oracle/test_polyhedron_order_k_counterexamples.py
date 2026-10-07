from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

from tools.check_polyhedron_order_k_counterexamples import (
    ValidationError,
    validate_fixture,
)


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "polyhedron_order_k_counterexamples.json"
)


def _payload() -> dict[str, object]:
    with FIXTURE.open(encoding="utf-8") as fixture_file:
        payload = json.load(fixture_file)
    if not isinstance(payload, dict):
        raise TypeError("the fixture must be a JSON object")
    return payload


class PolyhedronOrderKCounterexampleTests(unittest.TestCase):
    def test_exact_fixture_passes(self) -> None:
        validate_fixture(_payload())

    def test_created_lens_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["outliers_fewer_than_k"]["subcases"][0]["trace_after"] = []
        with self.assertRaisesRegex(ValidationError, "trace after mismatch"):
            validate_fixture(changed)

    def test_merge_without_merge_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        subcase = changed["cases"]["outliers_fewer_than_k"]["subcases"][1]
        subcase["added"] = [[30, 0, 0]]
        subcase["trace_after"] = [[0, 2], [3, 4]]
        with self.assertRaisesRegex(ValidationError, "no components are merged"):
            validate_fixture(changed)

    def test_separated_control_too_close_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        subcase = changed["cases"]["outliers_fewer_than_k"]["subcases"][2]
        subcase["added"] = [[8, 0, 0]]
        subcase["trace_after"] = [[0, 2], [3, 4], [6, 7]]
        with self.assertRaisesRegex(ValidationError, "not farther than 2r"):
            validate_fixture(changed)

    def test_root_chain_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["contracted_chain_identity"]["root_chain_leaf_after"] = 0
        with self.assertRaisesRegex(ValidationError, "root chain after mismatch"):
            validate_fixture(changed)

    def test_chain_image_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["contracted_chain_identity"]["probes"][1]["image_chain_leaf"] = 0
        with self.assertRaisesRegex(ValidationError, "image chain mismatch"):
            validate_fixture(changed)

    def test_displacement_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["contracted_chain_identity"]["delta"] = 2
        with self.assertRaisesRegex(ValidationError, "delta mismatch"):
            validate_fixture(changed)

    def test_witness_k_sets_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["no_nesting_across_orders"]["subcases"][0]["witness_k_sets"] = [[0, 1]]
        with self.assertRaisesRegex(ValidationError, "witness k-sets mismatch"):
            validate_fixture(changed)

    def test_inactive_triangle_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["no_nesting_across_orders"]["subcases"][1]["radius_squared"] = 100
        with self.assertRaisesRegex(ValidationError, "containing triangle is active"):
            validate_fixture(changed)

    def test_query_outside_cell_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["no_nesting_across_orders"]["subcases"][0]["query"] = [
            {"numerator": -1, "denominator": 2},
            {"numerator": -1, "denominator": 4},
            0,
        ]
        with self.assertRaisesRegex(ValidationError, "not in the active order-k cell"):
            validate_fixture(changed)

    def test_delaunay_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["cases"]["no_nesting_across_orders"]["subcases"][0]["order_one_triangles"] = [
            [0, 1, 2],
            [0, 2, 3],
        ]
        with self.assertRaisesRegex(ValidationError, "Delaunay triangles mismatch"):
            validate_fixture(changed)


if __name__ == "__main__":
    unittest.main()
