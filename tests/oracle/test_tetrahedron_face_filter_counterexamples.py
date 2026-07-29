from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

from tools.check_tetrahedron_face_filter_counterexamples import (
    ValidationError,
    validate_fixture,
)


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "tetrahedron_face_filter_counterexamples.json"
)


def payload() -> dict[str, object]:
    with FIXTURE.open(encoding="utf-8") as stream:
        value = json.load(stream)
    assert isinstance(value, dict)
    return value


class TetrahedronFaceFilterCounterexampleTests(unittest.TestCase):
    def test_exact_counterexamples_pass(self) -> None:
        validate_fixture(payload())

    def test_face_angle_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][0]["face_audit"][1]["angle_class"] = "acute"
        with self.assertRaisesRegex(ValidationError, "angle audit mismatch"):
            validate_fixture(changed)

    def test_barycentric_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][1]["circumball"]["barycentric_coordinates"][0] = 0
        with self.assertRaisesRegex(ValidationError, "barycentric mismatch"):
            validate_fixture(changed)

    def test_support_four_verdict_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][0]["support4_admissible"] = False
        with self.assertRaisesRegex(ValidationError, "support-four verdict mismatch"):
            validate_fixture(changed)

    def test_miniball_support_tamper_fails_closed(self) -> None:
        changed = copy.deepcopy(payload())
        changed["cases"][1]["minimum_enclosing_ball"]["support_point_ids"] = [0, 1, 2]
        with self.assertRaisesRegex(ValidationError, "minimum enclosing ball mismatch"):
            validate_fixture(changed)


if __name__ == "__main__":
    unittest.main()
