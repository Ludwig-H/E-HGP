from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

from tools.check_gabriel_carrier_strict_interior_extra_shell_matrix import (
    ValidationError,
    validate_fixture,
)


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "gabriel_carrier_strict_interior_extra_shell_matrix.json"
)


def payload() -> dict[str, object]:
    with FIXTURE.open(encoding="utf-8") as stream:
        value = json.load(stream)
    assert isinstance(value, dict)
    return value


class GabrielCarrierStrictInteriorExtraShellTests(unittest.TestCase):
    def test_exact_matrix_passes(self) -> None:
        validate_fixture(payload())

    def test_strict_interior_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][1]["expected"]["strict_interior_ids"] = []
        with self.assertRaisesRegex(ValidationError, "strict interior mismatch"):
            validate_fixture(changed)

    def test_extra_shell_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][0]["expected"]["extra_shell_ids"] = [2]
        with self.assertRaisesRegex(ValidationError, "extra shell mismatch"):
            validate_fixture(changed)

    def test_ball_level_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][3]["ball"]["squared_level"] = 24
        with self.assertRaisesRegex(ValidationError, "ball mismatch"):
            validate_fixture(changed)

    def test_carried_simplex_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][0]["expected"]["carried_simplex_ids"].pop()
        with self.assertRaisesRegex(ValidationError, "carried simplex mismatch"):
            validate_fixture(changed)

    def test_closed_rank_window_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][2]["expected"][
            "closed_rank_at_most_target_cardinality"
        ] = True
        with self.assertRaisesRegex(
            ValidationError, "closed-rank window verdict mismatch"
        ):
            validate_fixture(changed)

    def test_case_matrix_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][1]["case_id"] = "pair-one-strict-renamed"
        with self.assertRaisesRegex(
            ValidationError, "canonical strict-interior/shell matrix"
        ):
            validate_fixture(changed)


if __name__ == "__main__":
    unittest.main()
