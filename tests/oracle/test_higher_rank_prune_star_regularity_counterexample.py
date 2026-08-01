from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

from tools.check_higher_rank_prune_star_regularity_counterexample import (
    ValidationError,
    validate_fixture,
)


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "higher_rank_prune_does_not_certify_star_regularity.json"
)


def _payload() -> dict[str, object]:
    with FIXTURE.open(encoding="utf-8") as fixture_file:
        payload = json.load(fixture_file)
    assert isinstance(payload, dict)
    return payload


class HigherRankPruneStarRegularityCounterexampleTests(unittest.TestCase):
    def test_exact_fixture_passes(self) -> None:
        validate_fixture(_payload())

    def test_removed_shell_witness_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["late_star_coface"]["unvisited_extra_shell_point_ids"] = [11]
        with self.assertRaisesRegex(ValidationError, "external shell"):
            validate_fixture(changed)


if __name__ == "__main__":
    unittest.main()
