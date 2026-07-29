#!/usr/bin/env python3
"""Recertify the exact Hartigan triangle side-rank counterexample."""

from __future__ import annotations

import argparse
import json
import sys
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from reference.morsehgp3d_oracle.catalog import build_critical_catalog
from reference.morsehgp3d_oracle.gamma import build_gamma_filtration


DEFAULT_FIXTURE = (
    REPOSITORY_ROOT
    / "tests"
    / "fixtures"
    / "regressions"
    / "hartigan_triangle_all_side_ranks_above_k.json"
)


class ValidationError(RuntimeError):
    """Raised when the fixture or one of its exact certificates is invalid."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def _expect_keys(value: Any, keys: set[str], path: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{path} must be an object")
    require(
        set(value) == keys,
        f"{path} has unexpected fields: {sorted(set(value) ^ keys)!r}",
    )
    return value


def _integer(value: Any, path: str, *, minimum: int | None = None) -> int:
    require(
        isinstance(value, int) and not isinstance(value, bool),
        f"{path} must be an integer",
    )
    if minimum is not None:
        require(value >= minimum, f"{path} must be at least {minimum}")
    return value


def _fraction(value: Any, path: str) -> Fraction:
    if isinstance(value, int) and not isinstance(value, bool):
        return Fraction(value)
    record = _expect_keys(value, {"denominator", "numerator"}, path)
    numerator = _integer(record["numerator"], f"{path}.numerator")
    denominator = _integer(
        record["denominator"], f"{path}.denominator", minimum=1
    )
    result = Fraction(numerator, denominator)
    require(
        result.numerator == numerator and result.denominator == denominator,
        f"{path} must be a reduced canonical fraction",
    )
    return result


def _point_ids(
    value: Any,
    path: str,
    *,
    point_count: int,
    size: int | None = None,
) -> tuple[int, ...]:
    require(isinstance(value, list), f"{path} must be an array")
    result = tuple(
        _integer(point_id, f"{path}[{index}]", minimum=0)
        for index, point_id in enumerate(value)
    )
    if size is not None:
        require(len(result) == size, f"{path} must contain {size} point IDs")
    require(
        result == tuple(sorted(set(result))),
        f"{path} must be sorted and duplicate-free",
    )
    require(
        all(point_id < point_count for point_id in result),
        f"{path} contains an out-of-range point ID",
    )
    return result


def _points_and_labels(
    payload: dict[str, Any],
) -> tuple[tuple[str, ...], tuple[tuple[Fraction, Fraction, Fraction], ...]]:
    labels_value = payload["point_labels"]
    require(isinstance(labels_value, list), "$.point_labels must be an array")
    require(
        all(isinstance(label, str) and label for label in labels_value),
        "$.point_labels must contain nonempty strings",
    )
    labels = tuple(labels_value)
    require(len(labels) == len(set(labels)), "$.point_labels contains duplicates")

    points_value = payload["points"]
    require(isinstance(points_value, list), "$.points must be an array")
    require(len(points_value) == len(labels), "point and label counts differ")
    points = []
    for point_index, point_value in enumerate(points_value):
        require(
            isinstance(point_value, list) and len(point_value) == 3,
            f"$.points[{point_index}] must have three coordinates",
        )
        point = tuple(
            Fraction(
                _integer(
                    coordinate,
                    f"$.points[{point_index}][{coordinate_index}]",
                )
            )
            for coordinate_index, coordinate in enumerate(point_value)
        )
        points.append(point)
    result = tuple(points)
    require(len(result) == len(set(result)), "$.points contains duplicate coordinates")
    return labels, result  # type: ignore[return-value]


def _squared_distance(
    point: tuple[Fraction, Fraction, Fraction],
    center: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    return sum(
        (coordinate - center_coordinate) ** 2
        for coordinate, center_coordinate in zip(point, center)
    )


def _diameter_sphere_power(
    point: tuple[Fraction, Fraction, Fraction],
    first: tuple[Fraction, Fraction, Fraction],
    second: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    return sum(
        (coordinate - first_coordinate) * (coordinate - second_coordinate)
        for coordinate, first_coordinate, second_coordinate in zip(
            point, first, second
        )
    )


def _power_records(
    value: Any,
    path: str,
    *,
    point_count: int,
) -> tuple[tuple[int, Fraction], ...]:
    require(isinstance(value, list), f"{path} must be an array")
    records = []
    for index, raw_record in enumerate(value):
        record_path = f"{path}[{index}]"
        record = _expect_keys(raw_record, {"point_id", "value"}, record_path)
        point_id = _integer(record["point_id"], f"{record_path}.point_id", minimum=0)
        require(point_id < point_count, f"{record_path}.point_id is out of range")
        records.append((point_id, _fraction(record["value"], f"{record_path}.value")))
    require(
        tuple(point_id for point_id, _ in records)
        == tuple(sorted({point_id for point_id, _ in records})),
        f"{path} point IDs must be sorted and duplicate-free",
    )
    return tuple(records)


def _cut_record(cut: object) -> dict[str, object]:
    return {
        "active_facet_count": len(cut.active_facet_point_ids),
        "components": [
            {
                "facet_point_ids": [
                    list(facet) for facet in component.facet_point_ids
                ],
                "covered_point_ids": list(component.covered_point_ids),
            }
            for component in cut.components
        ],
    }


def _validate_target_miniball(
    payload: dict[str, Any],
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
    support_ids: tuple[int, ...],
    target_closed_rank: int,
) -> tuple[tuple[Fraction, Fraction, Fraction], Fraction]:
    record = _expect_keys(
        payload["target_miniball"],
        {
            "barycentric_coordinates",
            "center",
            "closed_rank",
            "exterior_squared_powers",
            "interior_point_ids",
            "shell_point_ids",
            "squared_level",
        },
        "$.target_miniball",
    )
    center_value = record["center"]
    require(
        isinstance(center_value, list) and len(center_value) == 3,
        "$.target_miniball.center must contain three coordinates",
    )
    center = tuple(
        _fraction(value, f"$.target_miniball.center[{index}]")
        for index, value in enumerate(center_value)
    )
    squared_level = _fraction(
        record["squared_level"], "$.target_miniball.squared_level"
    )
    require(squared_level > 0, "the target level must be positive")

    barycentric_value = record["barycentric_coordinates"]
    require(
        isinstance(barycentric_value, list)
        and len(barycentric_value) == len(support_ids),
        "the target needs one barycentric coordinate per support point",
    )
    barycentric = tuple(
        _fraction(value, f"$.target_miniball.barycentric_coordinates[{index}]")
        for index, value in enumerate(barycentric_value)
    )
    require(
        sum(barycentric, Fraction()) == 1 and all(value > 0 for value in barycentric),
        "the target support must be strictly well-centred",
    )
    reconstructed_center = tuple(
        sum(
            weight * points[point_id][axis]
            for weight, point_id in zip(barycentric, support_ids)
        )
        for axis in range(3)
    )
    require(reconstructed_center == center, "the barycentric center does not close")

    powers = tuple(_squared_distance(point, center) - squared_level for point in points)
    shell_ids = tuple(index for index, power in enumerate(powers) if power == 0)
    interior_ids = tuple(index for index, power in enumerate(powers) if power < 0)
    exterior_records = tuple(
        (index, power) for index, power in enumerate(powers) if power > 0
    )
    expected_shell = _point_ids(
        record["shell_point_ids"],
        "$.target_miniball.shell_point_ids",
        point_count=len(points),
    )
    expected_interior = _point_ids(
        record["interior_point_ids"],
        "$.target_miniball.interior_point_ids",
        point_count=len(points),
    )
    expected_exterior = _power_records(
        record["exterior_squared_powers"],
        "$.target_miniball.exterior_squared_powers",
        point_count=len(points),
    )
    require(shell_ids == expected_shell == support_ids, "target shell mismatch")
    require(interior_ids == expected_interior == (), "target interior must be empty")
    require(exterior_records == expected_exterior, "target exterior powers mismatch")
    closed_rank = _integer(
        record["closed_rank"], "$.target_miniball.closed_rank", minimum=1
    )
    require(closed_rank == len(shell_ids) + len(interior_ids), "target rank mismatch")
    require(
        closed_rank == target_closed_rank,
        "target closed rank differs from the selected bucket",
    )
    return center, squared_level  # type: ignore[return-value]


def _validate_sides(
    payload: dict[str, Any],
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
    support_ids: tuple[int, ...],
    squared_level: Fraction,
    target_closed_rank: int,
) -> None:
    audits_value = payload["side_audit"]
    require(isinstance(audits_value, list), "$.side_audit must be an array")
    expected_sides = tuple(combinations(support_ids, 2))
    require(len(audits_value) == len(expected_sides), "side audit extent mismatch")
    observed_sides = []
    admissible_side_count = 0
    for audit_index, raw_audit in enumerate(audits_value):
        path = f"$.side_audit[{audit_index}]"
        audit = _expect_keys(
            raw_audit,
            {
                "closed_rank",
                "interior_diameter_sphere_powers",
                "interior_point_ids",
                "point_ids",
                "shell_point_ids",
                "squared_level",
            },
            path,
        )
        side = _point_ids(
            audit["point_ids"], f"{path}.point_ids", point_count=len(points), size=2
        )
        observed_sides.append(side)
        first, second = (points[point_id] for point_id in side)
        side_squared_level = _squared_distance(first, second) / 4
        require(
            _fraction(audit["squared_level"], f"{path}.squared_level")
            == side_squared_level,
            f"{path} squared level mismatch",
        )
        require(
            side_squared_level < squared_level,
            f"{path} is not a strict acute-triangle facet",
        )
        powers = tuple(
            _diameter_sphere_power(point, first, second) for point in points
        )
        shell_ids = tuple(index for index, power in enumerate(powers) if power == 0)
        interior_ids = tuple(index for index, power in enumerate(powers) if power < 0)
        expected_shell = _point_ids(
            audit["shell_point_ids"],
            f"{path}.shell_point_ids",
            point_count=len(points),
        )
        expected_interior = _point_ids(
            audit["interior_point_ids"],
            f"{path}.interior_point_ids",
            point_count=len(points),
        )
        expected_powers = _power_records(
            audit["interior_diameter_sphere_powers"],
            f"{path}.interior_diameter_sphere_powers",
            point_count=len(points),
        )
        require(shell_ids == expected_shell == side, f"{path} shell mismatch")
        require(interior_ids == expected_interior, f"{path} interior mismatch")
        require(
            tuple((point_id, powers[point_id]) for point_id in interior_ids)
            == expected_powers,
            f"{path} diameter-sphere powers mismatch",
        )
        closed_rank = _integer(audit["closed_rank"], f"{path}.closed_rank", minimum=2)
        require(
            closed_rank == len(shell_ids) + len(interior_ids),
            f"{path} closed rank mismatch",
        )
        admissible_side_count += closed_rank <= target_closed_rank

    require(tuple(observed_sides) == expected_sides, "side audit is not canonical")
    recorded_admissible_count = _integer(
        payload["admissible_side_count_at_target_rank"],
        "$.admissible_side_count_at_target_rank",
        minimum=0,
    )
    require(
        recorded_admissible_count == admissible_side_count == 0,
        "the target unexpectedly has a rank-admissible side",
    )


def _validate_pair_subedge_closure(
    payload: dict[str, Any],
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
    support_ids: tuple[int, ...],
    target_closed_rank: int,
) -> None:
    record = _expect_keys(
        payload["pair_subedge_closure_audit"],
        {
            "admitted_pair_count",
            "maximum_closed_rank",
            "pair_rank_histogram",
            "target_side_carrier_counts",
            "target_triangle_generated",
        },
        "$.pair_subedge_closure_audit",
    )
    maximum_closed_rank = _integer(
        record["maximum_closed_rank"],
        "$.pair_subedge_closure_audit.maximum_closed_rank",
        minimum=2,
    )
    require(
        maximum_closed_rank == target_closed_rank,
        "the subedge closure uses the wrong rank window",
    )

    rank_histogram: dict[int, int] = {}
    admitted_closed_sets: list[frozenset[int]] = []
    for first, second in combinations(range(len(points)), 2):
        closed_set = frozenset(
            point_id
            for point_id, point in enumerate(points)
            if _diameter_sphere_power(
                point, points[first], points[second]
            )
            <= 0
        )
        rank_histogram[len(closed_set)] = (
            rank_histogram.get(len(closed_set), 0) + 1
        )
        if len(closed_set) <= maximum_closed_rank:
            admitted_closed_sets.append(closed_set)

    expected_histogram_value = record["pair_rank_histogram"]
    require(
        isinstance(expected_histogram_value, dict)
        and all(
            isinstance(key, str) and key.isdigit()
            for key in expected_histogram_value
        ),
        "$.pair_subedge_closure_audit.pair_rank_histogram must use decimal rank keys",
    )
    expected_histogram = {
        int(key): _integer(
            value,
            f"$.pair_subedge_closure_audit.pair_rank_histogram.{key}",
            minimum=0,
        )
        for key, value in expected_histogram_value.items()
    }
    require(rank_histogram == expected_histogram, "pair rank histogram mismatch")
    require(
        _integer(
            record["admitted_pair_count"],
            "$.pair_subedge_closure_audit.admitted_pair_count",
            minimum=0,
        )
        == len(admitted_closed_sets),
        "admitted pair count mismatch",
    )

    raw_carriers = record["target_side_carrier_counts"]
    require(
        isinstance(raw_carriers, list),
        "$.pair_subedge_closure_audit.target_side_carrier_counts must be an array",
    )
    expected_sides = tuple(combinations(support_ids, 2))
    require(
        len(raw_carriers) == len(expected_sides),
        "subedge carrier extent mismatch",
    )
    observed_sides = []
    observed_counts = []
    for index, raw_carrier in enumerate(raw_carriers):
        path = f"$.pair_subedge_closure_audit.target_side_carrier_counts[{index}]"
        carrier = _expect_keys(
            raw_carrier, {"carrier_pair_count", "point_ids"}, path
        )
        side = _point_ids(
            carrier["point_ids"],
            f"{path}.point_ids",
            point_count=len(points),
            size=2,
        )
        observed_sides.append(side)
        side_set = frozenset(side)
        carrier_count = sum(
            side_set <= closed_set for closed_set in admitted_closed_sets
        )
        observed_counts.append(carrier_count)
        require(
            _integer(
                carrier["carrier_pair_count"],
                f"{path}.carrier_pair_count",
                minimum=0,
            )
            == carrier_count,
            f"{path} carrier count mismatch",
        )
    require(
        tuple(observed_sides) == expected_sides,
        "subedge carrier sides are not canonical",
    )
    require(
        observed_counts == [0, 0, 0]
        and record["target_triangle_generated"] is False,
        "the rank-bounded pair subedge closure unexpectedly generates the target triangle",
    )


def _validate_catalog(
    payload: dict[str, Any],
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
    support_ids: tuple[int, ...],
    squared_level: Fraction,
) -> None:
    record = _expect_keys(
        payload["catalog_audit"],
        {
            "critical_event_count",
            "degenerate_candidate_count",
            "k_max",
            "relevant_gp_complete",
            "target_event_birth_order",
            "target_event_closed_rank",
            "target_event_saddle_order",
        },
        "$.catalog_audit",
    )
    k_max = _integer(record["k_max"], "$.catalog_audit.k_max", minimum=1)
    catalog = build_critical_catalog(points, k_max)
    require(
        record["relevant_gp_complete"] is catalog.relevant_gp_complete is True,
        "the fixture is outside RelevantGP",
    )
    require(
        _integer(
            record["degenerate_candidate_count"],
            "$.catalog_audit.degenerate_candidate_count",
            minimum=0,
        )
        == len(catalog.degenerate_candidates)
        == 0,
        "catalogue degeneracy count mismatch",
    )
    require(
        _integer(
            record["critical_event_count"],
            "$.catalog_audit.critical_event_count",
            minimum=1,
        )
        == len(catalog.events),
        "critical event count mismatch",
    )
    support_points = {points[point_id] for point_id in support_ids}
    target_events = [
        event
        for event in catalog.events
        if event.squared_radius == squared_level
        and {catalog.points[point_id] for point_id in event.minimal_support_ids}
        == support_points
    ]
    require(len(target_events) == 1, "the target critical event is not unique")
    event = target_events[0]
    expected_fields = {
        "closed_rank": "target_event_closed_rank",
        "birth_order": "target_event_birth_order",
        "saddle_order": "target_event_saddle_order",
    }
    for attribute, field in expected_fields.items():
        require(
            getattr(event, attribute)
            == _integer(record[field], f"$.catalog_audit.{field}", minimum=1),
            f"target event {attribute} mismatch",
        )


def _validate_gamma_transition(
    payload: dict[str, Any],
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
    support_ids: tuple[int, ...],
    squared_level: Fraction,
    order: int,
) -> None:
    record = _expect_keys(
        payload["gamma2_transition"],
        {
            "closed_cut",
            "exact_gabriel",
            "strict_component_count_coalesced_by_target",
            "strict_cut",
            "unique_equal_coface_point_ids",
        },
        "$.gamma2_transition",
    )
    filtration = build_gamma_filtration(points, order)
    expected_equal_coface = _point_ids(
        record["unique_equal_coface_point_ids"],
        "$.gamma2_transition.unique_equal_coface_point_ids",
        point_count=len(points),
        size=order + 1,
    )
    equal_cofaces = tuple(
        coface
        for coface in filtration.cofaces
        if coface.squared_level == squared_level
    )
    require(
        len(equal_cofaces) == 1
        and equal_cofaces[0].point_ids == expected_equal_coface == support_ids,
        "the target is not the unique equal-level coface",
    )
    target_coface = equal_cofaces[0]
    require(
        target_coface.support_ids == support_ids,
        "the target coface has the wrong miniball support",
    )
    exact_gabriel = any(
        edge.simplex_point_ids == support_ids
        and edge.squared_level == squared_level
        for edge in filtration.gabriel_hyperedges
    )
    require(
        record["exact_gabriel"] is exact_gabriel is True,
        "the target is not exact Gabriel",
    )

    strict_cut = filtration.cut(squared_level, closed=False, graph_kind="gamma")
    closed_cut = filtration.cut(squared_level, closed=True, graph_kind="gamma")
    require(
        _cut_record(strict_cut) == record["strict_cut"],
        "strict Gamma_2 cut mismatch",
    )
    require(
        _cut_record(closed_cut) == record["closed_cut"],
        "closed Gamma_2 cut mismatch",
    )
    strict_component_by_facet = strict_cut.component_by_facet
    target_strict_components = {
        strict_component_by_facet[facet].facet_point_ids
        for facet in target_coface.facet_point_ids
    }
    coalesced_count = _integer(
        record["strict_component_count_coalesced_by_target"],
        "$.gamma2_transition.strict_component_count_coalesced_by_target",
        minimum=2,
    )
    require(
        len(target_strict_components) == coalesced_count == 2,
        "the target does not coalesce two strict components",
    )
    closed_component_by_facet = closed_cut.component_by_facet
    require(
        len(
            {
                closed_component_by_facet[facet].facet_point_ids
                for facet in target_coface.facet_point_ids
            }
        )
        == 1,
        "the target facets do not share one closed component",
    )


def validate_fixture(payload: Any) -> None:
    root = _expect_keys(
        payload,
        {
            "admissible_side_count_at_target_rank",
            "catalog_audit",
            "contradicted_claims",
            "description",
            "fixture_id",
            "gamma2_transition",
            "kind",
            "order",
            "pair_subedge_closure_audit",
            "point_labels",
            "points",
            "schema_version",
            "scope_note",
            "side_audit",
            "target_miniball",
            "target_closed_rank",
            "target_support_point_ids",
        },
        "$",
    )
    require(root["schema_version"] == 1, "unsupported fixture schema")
    require(
        root["kind"] == "morsehgp3d_exact_hartigan_side_rank_counterexample",
        "fixture kind mismatch",
    )
    require(
        root["fixture_id"]
        == "hartigan-triangle-all-side-ranks-above-k-gamma2-merge-v1",
        "fixture ID mismatch",
    )
    require(
        isinstance(root["description"], str) and root["description"],
        "fixture description is missing",
    )
    require(
        isinstance(root["scope_note"], str) and root["scope_note"],
        "fixture scope note is missing",
    )
    claims = root["contradicted_claims"]
    require(
        isinstance(claims, list)
        and len(claims) == 3
        and all(isinstance(claim, str) and claim for claim in claims),
        "the fixture must state exactly three contradicted claims",
    )

    _, points = _points_and_labels(root)
    order = _integer(root["order"], "$.order", minimum=1)
    target_closed_rank = _integer(
        root["target_closed_rank"], "$.target_closed_rank", minimum=2
    )
    require(
        order == 2 and target_closed_rank == 3,
        "the certified counterexample is for order 2 and target rank 3",
    )
    support_ids = _point_ids(
        root["target_support_point_ids"],
        "$.target_support_point_ids",
        point_count=len(points),
        size=3,
    )
    _, squared_level = _validate_target_miniball(
        root, points, support_ids, target_closed_rank
    )
    _validate_sides(
        root, points, support_ids, squared_level, target_closed_rank
    )
    _validate_pair_subedge_closure(
        root, points, support_ids, target_closed_rank
    )
    _validate_catalog(root, points, support_ids, squared_level)
    _validate_gamma_transition(
        root, points, support_ids, squared_level, order
    )


def load_and_validate(path: Path) -> None:
    with path.open(encoding="utf-8") as fixture_file:
        payload = json.load(fixture_file)
    validate_fixture(payload)


def parse_arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "fixture",
        type=Path,
        nargs="?",
        default=DEFAULT_FIXTURE,
        help="fixture to recertify (default: %(default)s)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    arguments = parse_arguments(argv)
    load_and_validate(arguments.fixture)
    print(
        "exact Hartigan triangle side-rank counterexample passed: "
        f"{arguments.fixture}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (json.JSONDecodeError, OSError, ValidationError, ValueError) as error:
        print(
            f"exact Hartigan triangle side-rank counterexample failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1) from error
