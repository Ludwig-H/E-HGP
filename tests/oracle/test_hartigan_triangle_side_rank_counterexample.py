from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

from tools.check_hartigan_triangle_side_rank_counterexample import (
    ValidationError,
    validate_fixture,
)


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "hartigan_triangle_all_side_ranks_above_k.json"
)


def _payload() -> dict[str, object]:
    with FIXTURE.open(encoding="utf-8") as fixture_file:
        payload = json.load(fixture_file)
    assert isinstance(payload, dict)
    return payload


class HartiganTriangleSideRankCounterexampleTests(unittest.TestCase):
    def test_exact_fixture_passes(self) -> None:
        validate_fixture(_payload())

    def test_target_closed_rank_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["target_miniball"]["closed_rank"] = 4
        with self.assertRaisesRegex(ValidationError, "target rank mismatch"):
            validate_fixture(changed)

    def test_side_closed_rank_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["side_audit"][0]["closed_rank"] = 3
        with self.assertRaisesRegex(ValidationError, "closed rank mismatch"):
            validate_fixture(changed)

    def test_external_power_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["target_miniball"]["exterior_squared_powers"][0]["value"] = 0
        with self.assertRaisesRegex(ValidationError, "exterior powers mismatch"):
            validate_fixture(changed)

    def test_subedge_carrier_count_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["pair_subedge_closure_audit"]["target_side_carrier_counts"][0][
            "carrier_pair_count"
        ] = 1
        with self.assertRaisesRegex(ValidationError, "carrier count mismatch"):
            validate_fixture(changed)

    def test_strict_gamma_cut_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["gamma2_transition"]["strict_cut"]["components"][0][
            "facet_point_ids"
        ].pop()
        with self.assertRaisesRegex(ValidationError, "strict Gamma_2 cut mismatch"):
            validate_fixture(changed)


if __name__ == "__main__":
    unittest.main()
