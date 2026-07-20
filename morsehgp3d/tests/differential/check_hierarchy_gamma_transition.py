#!/usr/bin/env python3
"""Independent exact differential for one open-to-closed Gamma transition."""

from __future__ import annotations

import argparse
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from check_hierarchy_gamma import (
    InputPoint,
    emitted_point_order,
    first_difference,
    ratio_record,
    strict_json_object,
)
from reference.morsehgp3d_oracle.gamma import build_gamma_filtration


DEFAULT_TIMEOUT_SECONDS = 30


@dataclass(frozen=True)
class FixedCase:
    name: str
    input_points: tuple[InputPoint, ...]
    input_labels: tuple[str, ...]
    order: int
    squared_level: Fraction


FIXED_CASES = (
    FixedCase(
        "isolated_equal_facet",
        ((0, 0, 0), (2, 0, 0), (10, 0, 0)),
        ("A", "B", "C"),
        2,
        Fraction(1),
    ),
    FixedCase(
        "silent_equal_coface",
        ((0, 0, 0), (1, 0, 0), (2, 0, 0)),
        ("A", "B", "C"),
        1,
        Fraction(1),
    ),
    FixedCase(
        "equal_facet_coalescence",
        ((-2, 0, 0), (0, 1, 0), (2, 0, 0)),
        ("A", "B", "C"),
        2,
        Fraction(4),
    ),
    FixedCase(
        "overlapping_equal_cofaces",
        ((-2, 0, 0), (0, -3, 0), (0, 3, 0), (2, 0, 0)),
        ("A", "B", "C", "D"),
        2,
        Fraction(169, 36),
    ),
    FixedCase(
        "disconnected_q1_q0_q2",
        (
            (-4, -2, 0),
            (-4, 2, 0),
            (0, 0, 0),
            (6, 4, 0),
            (8, -2, 0),
            (40, 0, 0),
            (46, 6, 8),
            (80, 0, 0),
            (83, 3, 4),
            (86, 6, 8),
        ),
        ("A0", "A1", "A2", "A3", "A4", "B0", "B1", "C0", "C1", "C2"),
        2,
        Fraction(34),
    ),
    FixedCase(
        "order_ten_equal_coface",
        tuple((coordinate, 0, 0) for coordinate in range(11)),
        tuple(f"P{index}" for index in range(11)),
        10,
        Fraction(25),
    ),
)


def facet_record(point_ids: tuple[int, ...], squared_level: Fraction) -> dict[str, object]:
    return {
        "point_ids": list(point_ids),
        "squared_level": ratio_record(squared_level),
    }


def coface_record(coface: object) -> dict[str, object]:
    return {
        "facet_point_ids": [list(facet) for facet in coface.facet_point_ids],
        "point_ids": list(coface.point_ids),
        "squared_level": ratio_record(coface.squared_level),
    }


def component_record(component: object) -> dict[str, object]:
    return {
        "canonical_representative_facet_point_ids": list(
            component.facet_point_ids[0]
        ),
        "facet_point_ids": [list(facet) for facet in component.facet_point_ids],
    }


def group_kind(strict_component_count: int) -> str:
    if strict_component_count == 0:
        return "new_closed_component_without_strict_component"
    if strict_component_count == 1:
        return "one_strict_component_continuation"
    return "multiple_strict_component_coalescence"


def expected_document(
    fixed_case: FixedCase, actual: dict[str, object]
) -> dict[str, object]:
    canonical_points, points, _ = emitted_point_order(fixed_case, actual)
    filtration = build_gamma_filtration(points, fixed_case.order)
    strict_cut = filtration.cut(fixed_case.squared_level, closed=False)
    closed_cut = filtration.cut(fixed_case.squared_level, closed=True)

    strict_facets = tuple(
        facet
        for facet in filtration.facets
        if facet.squared_level < fixed_case.squared_level
    )
    equal_facets = tuple(
        facet
        for facet in filtration.facets
        if facet.squared_level == fixed_case.squared_level
    )
    strict_cofaces = tuple(
        coface
        for coface in filtration.cofaces
        if coface.squared_level < fixed_case.squared_level
    )
    equal_cofaces = tuple(
        coface
        for coface in filtration.cofaces
        if coface.squared_level == fixed_case.squared_level
    )

    strict_component_by_facet = {
        facet: component_index
        for component_index, component in enumerate(strict_cut.components)
        for facet in component.facet_point_ids
    }
    closed_component_by_facet = {
        facet: component_index
        for component_index, component in enumerate(closed_cut.components)
        for facet in component.facet_point_ids
    }
    equal_facet_labels = {facet.point_ids for facet in equal_facets}

    incidences = []
    for coface in equal_cofaces:
        for facet in coface.facet_point_ids:
            strict_component_index = strict_component_by_facet.get(facet)
            newly_active = facet in equal_facet_labels
            if (strict_component_index is not None) == newly_active:
                raise AssertionError(
                    f"{fixed_case.name}: equality incidence has no unique token"
                )
            incidences.append(
                {
                    "coface_point_ids": list(coface.point_ids),
                    "facet_point_ids": list(facet),
                    "newly_active_at_level": newly_active,
                    "strict_component_index": strict_component_index,
                }
            )

    strict_to_closed = []
    for component in strict_cut.components:
        representative = component.facet_point_ids[0]
        strict_to_closed.append(closed_component_by_facet[representative])
        if any(
            closed_component_by_facet[facet] != strict_to_closed[-1]
            for facet in component.facet_point_ids
        ):
            raise AssertionError(
                f"{fixed_case.name}: strict component does not refine closed cut"
            )

    affected_closed_indices = {
        closed_component_by_facet[facet.point_ids] for facet in equal_facets
    }
    affected_closed_indices.update(
        closed_component_by_facet[coface.facet_point_ids[0]]
        for coface in equal_cofaces
    )
    groups = []
    for closed_index in sorted(affected_closed_indices):
        strict_indices = [
            strict_index
            for strict_index, projection in enumerate(strict_to_closed)
            if projection == closed_index
        ]
        new_facets = [
            facet.point_ids
            for facet in equal_facets
            if closed_component_by_facet[facet.point_ids] == closed_index
        ]
        group_cofaces = []
        for coface in equal_cofaces:
            component_indices = {
                closed_component_by_facet[facet]
                for facet in coface.facet_point_ids
            }
            if len(component_indices) != 1:
                raise AssertionError(
                    f"{fixed_case.name}: equal coface spans closed components"
                )
            if closed_index in component_indices:
                group_cofaces.append(coface.point_ids)
        groups.append(
            {
                "canonical_representative_facet_point_ids": list(
                    closed_cut.components[closed_index].facet_point_ids[0]
                ),
                "closed_component_index": closed_index,
                "equal_level_coface_point_ids": [
                    list(coface) for coface in group_cofaces
                ],
                "kind": group_kind(len(strict_indices)),
                "newly_active_facet_point_ids": [
                    list(facet) for facet in new_facets
                ],
                "strict_component_indices": strict_indices,
            }
        )

    return {
        "canonical_points": canonical_points,
        "case": fixed_case.name,
        "closed_components": [
            component_record(component) for component in closed_cut.components
        ],
        "equal_level_cofaces": [
            coface_record(coface) for coface in equal_cofaces
        ],
        "equal_level_facets": [
            facet_record(facet.point_ids, facet.squared_level)
            for facet in equal_facets
        ],
        "equal_level_incidences": incidences,
        "order": fixed_case.order,
        "squared_level": ratio_record(fixed_case.squared_level),
        "strict_active_cofaces": [
            coface_record(coface) for coface in strict_cofaces
        ],
        "strict_active_facets": [
            facet_record(facet.point_ids, facet.squared_level)
            for facet in strict_facets
        ],
        "strict_component_to_closed_component_index": strict_to_closed,
        "strict_components": [
            component_record(component) for component in strict_cut.components
        ],
        "transition_groups": groups,
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="path to hierarchy_gamma_dump")
    parser.add_argument(
        "--timeout-seconds",
        type=int,
        default=DEFAULT_TIMEOUT_SECONDS,
        help="subprocess timeout (default: %(default)s)",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout_seconds <= 0:
        raise ValueError("--timeout-seconds must be positive")
    executable = arguments.executable.resolve(strict=True)
    completed = subprocess.run(
        [str(executable), "--transitions"],
        check=False,
        capture_output=True,
        text=True,
        timeout=arguments.timeout_seconds,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            "Gamma-transition dump exited with "
            f"{completed.returncode}: {completed.stderr.strip()}"
        )
    lines = completed.stdout.splitlines()
    if len(lines) != len(FIXED_CASES) or any(not line for line in lines):
        raise AssertionError(
            f"expected {len(FIXED_CASES)} nonempty dump lines, observed {len(lines)}"
        )

    documents = [strict_json_object(line) for line in lines]
    observed_names = [document.get("case") for document in documents]
    expected_names = [fixed_case.name for fixed_case in FIXED_CASES]
    if observed_names != expected_names:
        raise AssertionError(
            f"expected fixed cases {expected_names!r}, observed {observed_names!r}"
        )
    for fixed_case, actual in zip(FIXED_CASES, documents):
        expected = expected_document(fixed_case, actual)
        difference = first_difference(expected, actual)
        if difference is not None:
            raise AssertionError(f"{fixed_case.name}: {difference}")

    print("hierarchy Gamma transition differential passed for 6 exact cases")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, RuntimeError, ValueError) as error:
        print(f"hierarchy Gamma transition differential failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
