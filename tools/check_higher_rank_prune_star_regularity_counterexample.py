#!/usr/bin/env python3
"""Recertify that a rank cutoff cannot certify late-Star regularity."""

from __future__ import annotations

import argparse
import json
import sys
from fractions import Fraction
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from reference.morsehgp3d_oracle.geometry import (  # noqa: E402
    BallRelation,
    classify,
    classify_points,
    minimum_enclosing_ball,
)


DEFAULT_FIXTURE = (
    REPOSITORY_ROOT
    / "tests"
    / "fixtures"
    / "regressions"
    / "higher_rank_prune_does_not_certify_star_regularity.json"
)


class ValidationError(RuntimeError):
    """Raised when the exact fixture or one of its claims is invalid."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def _integer(value: Any, path: str, *, minimum: int | None = None) -> int:
    require(
        isinstance(value, int) and not isinstance(value, bool),
        f"{path} must be an integer",
    )
    if minimum is not None:
        require(value >= minimum, f"{path} must be at least {minimum}")
    return value


def _point_ids(
    value: Any, path: str, *, point_count: int, nonempty: bool = True
) -> tuple[int, ...]:
    require(isinstance(value, list), f"{path} must be an array")
    result = tuple(
        _integer(point_id, f"{path}[{index}]", minimum=0)
        for index, point_id in enumerate(value)
    )
    if nonempty:
        require(result, f"{path} must not be empty")
    require(result == tuple(sorted(set(result))), f"{path} must be canonical")
    require(all(point_id < point_count for point_id in result), f"{path} is out of range")
    return result


def _point(value: Any, path: str) -> tuple[Fraction, Fraction, Fraction]:
    require(isinstance(value, list) and len(value) == 3, f"{path} must be a 3-vector")
    return tuple(
        Fraction(_integer(coordinate, f"{path}[{axis}]"))
        for axis, coordinate in enumerate(value)
    )  # type: ignore[return-value]


def validate_fixture(payload: dict[str, Any]) -> None:
    expected_top_level = {
        "schema_version",
        "kind",
        "fixture_id",
        "description",
        "maximum_closed_rank",
        "point_labels",
        "points",
        "direct_core_coface",
        "core_facet_first_incidence",
        "late_star_coface",
        "contradicted_claim",
        "scope_note",
    }
    require(set(payload) == expected_top_level, "fixture fields differ from schema v1")
    require(payload["schema_version"] == 1, "schema_version must be one")
    require(
        payload["kind"] == "morsehgp3d_exact_star_regularity_rank_prune_counterexample",
        "fixture kind mismatch",
    )
    require(isinstance(payload["contradicted_claim"], str), "claim must be text")
    require(isinstance(payload["scope_note"], str), "scope note must be text")

    labels = payload["point_labels"]
    raw_points = payload["points"]
    require(isinstance(labels, list) and isinstance(raw_points, list), "points and labels must be arrays")
    require(len(labels) == len(raw_points), "point and label counts differ")
    require(len(labels) == len(set(labels)), "point labels must be unique")
    points = tuple(_point(point, f"$.points[{index}]") for index, point in enumerate(raw_points))
    require(len(points) == len(set(points)), "point coordinates must be unique")
    point_count = len(points)

    direct = payload["direct_core_coface"]
    require(isinstance(direct, dict), "direct_core_coface must be an object")
    require(
        set(direct)
        == {
            "point_ids",
            "support_point_ids",
            "center",
            "squared_level",
            "interior_point_ids",
            "shell_point_ids",
        },
        "direct_core_coface fields differ from schema v1",
    )
    direct_ids = _point_ids(direct["point_ids"], "$.direct_core_coface.point_ids", point_count=point_count)
    direct_support = _point_ids(direct["support_point_ids"], "$.direct_core_coface.support_point_ids", point_count=point_count)
    direct_ball = minimum_enclosing_ball(points, direct_ids)
    direct_partition = classify_points(points, direct_ball)
    require(direct_ball.support_ids == direct_support, "direct support mismatch")
    require(direct_ball.center == _point(direct["center"], "$.direct_core_coface.center"), "direct center mismatch")
    require(direct_ball.squared_radius == _integer(direct["squared_level"], "$.direct_core_coface.squared_level"), "direct level mismatch")
    require(
        direct_partition.interior_ids
        == _point_ids(direct["interior_point_ids"], "$.direct_core_coface.interior_point_ids", point_count=point_count, nonempty=False),
        "direct interior mismatch",
    )
    require(
        direct_partition.shell_ids
        == _point_ids(direct["shell_point_ids"], "$.direct_core_coface.shell_point_ids", point_count=point_count),
        "direct shell mismatch",
    )

    first = payload["core_facet_first_incidence"]
    require(isinstance(first, dict), "core_facet_first_incidence must be an object")
    require(
        set(first)
        == {
            "facet_point_ids",
            "source_miniball_squared_level",
            "first_incidence_squared_level",
            "cominimizer_point_ids",
        },
        "core_facet_first_incidence fields differ from schema v1",
    )
    facet_ids = _point_ids(first["facet_point_ids"], "$.core_facet_first_incidence.facet_point_ids", point_count=point_count)
    require(set(facet_ids).issubset(direct_ids), "core facet is not a direct deletion")
    facet_ball = minimum_enclosing_ball(points, facet_ids)
    require(
        facet_ball.squared_radius
        == _integer(first["source_miniball_squared_level"], "$.core_facet_first_incidence.source_miniball_squared_level"),
        "core facet level mismatch",
    )
    extensions = []
    for point_id in range(point_count):
        if point_id in facet_ids:
            continue
        extension = minimum_enclosing_ball(points, (*facet_ids, point_id))
        extensions.append((extension.squared_radius, point_id))
    first_level = min(level for level, _ in extensions)
    cominimizers = tuple(point_id for level, point_id in extensions if level == first_level)
    require(
        first_level
        == _integer(first["first_incidence_squared_level"], "$.core_facet_first_incidence.first_incidence_squared_level"),
        "first-incidence level mismatch",
    )
    require(
        cominimizers
        == _point_ids(first["cominimizer_point_ids"], "$.core_facet_first_incidence.cominimizer_point_ids", point_count=point_count),
        "first-incidence co-minimizers mismatch",
    )

    late = payload["late_star_coface"]
    require(isinstance(late, dict), "late_star_coface must be an object")
    require(
        set(late)
        == {
            "point_ids",
            "support_point_ids",
            "center",
            "squared_level",
            "strict_interior_point_ids",
            "shell_point_ids",
            "rank_prune_prefix_point_ids",
            "unvisited_extra_shell_point_ids",
        },
        "late_star_coface fields differ from schema v1",
    )
    late_ids = _point_ids(late["point_ids"], "$.late_star_coface.point_ids", point_count=point_count)
    require(set(facet_ids).issubset(late_ids), "late coface is not in the core-facet Star")
    late_ball = minimum_enclosing_ball(points, late_ids)
    late_partition = classify_points(points, late_ball)
    support = _point_ids(late["support_point_ids"], "$.late_star_coface.support_point_ids", point_count=point_count)
    interiors = _point_ids(late["strict_interior_point_ids"], "$.late_star_coface.strict_interior_point_ids", point_count=point_count)
    shell = _point_ids(late["shell_point_ids"], "$.late_star_coface.shell_point_ids", point_count=point_count)
    prefix = _point_ids(late["rank_prune_prefix_point_ids"], "$.late_star_coface.rank_prune_prefix_point_ids", point_count=point_count)
    unvisited_shell = _point_ids(late["unvisited_extra_shell_point_ids"], "$.late_star_coface.unvisited_extra_shell_point_ids", point_count=point_count)
    require(late_ball.support_ids == support, "late support mismatch")
    require(late_ball.center == _point(late["center"], "$.late_star_coface.center"), "late center mismatch")
    require(late_ball.squared_radius == _integer(late["squared_level"], "$.late_star_coface.squared_level"), "late level mismatch")
    require(late_partition.interior_ids == interiors, "late strict interiors mismatch")
    require(late_partition.shell_ids == shell, "late shell mismatch")
    require(set(late_ids) - set(support) <= set(interiors), "late coface is not non-direct")
    require(set(unvisited_shell).issubset(set(shell) - set(late_ids)), "unvisited point is not an external shell witness")
    require(set(support).union(interiors).issubset(prefix), "rank prefix misses a known closed-ball point")
    require(set(unvisited_shell).isdisjoint(prefix), "rank prefix already observes the extra shell")
    maximum_closed_rank = _integer(payload["maximum_closed_rank"], "$.maximum_closed_rank", minimum=1)
    require(len(support) + len(interiors) > maximum_closed_rank, "rank cutoff does not close before the extra shell")
    require(
        all(classify(points[point_id], late_ball) is BallRelation.SHELL for point_id in unvisited_shell),
        "external shell equality was not recertified",
    )
    require(late_ball.squared_radius > first_level, "the Star counterexample is not a late coface")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", nargs="?", type=Path, default=DEFAULT_FIXTURE)
    arguments = parser.parse_args()
    try:
        with arguments.fixture.open(encoding="utf-8") as fixture_file:
            payload = json.load(fixture_file)
        require(isinstance(payload, dict), "fixture root must be an object")
        validate_fixture(payload)
    except (OSError, json.JSONDecodeError, ValidationError, ValueError) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: exact rank-prune/global-Star-regularity counterexample recertified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
