#!/usr/bin/env python3
"""Independent exact differential for the bounded catalogue-arm Gamma overlay.

The oracle deliberately does not reproduce the canonical miniball descent.
It derives every expected target from the initial critical arm in the direct
``Fraction`` Gamma cut, then checks that the observed terminal facet belongs
to that same independently reconstructed strict component.
"""

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
from reference.morsehgp3d_oracle.catalog import build_critical_catalog
from reference.morsehgp3d_oracle.gamma import build_gamma_filtration
from reference.morsehgp3d_oracle.hierarchy import build_merge_forest

DEFAULT_TIMEOUT_SECONDS = 60
COMPLETE_DECISION = "complete_exhaustive_catalog_saddle_arm_gamma_overlay"


@dataclass(frozen=True)
class FixedCase:
    name: str
    input_points: tuple[InputPoint, ...]
    input_labels: tuple[str, ...]
    order: int


FIXED_CASES = (
    FixedCase(
        "mirror_simultaneous_q5",
        ((-2, 0, 0), (0, -3, 0), (0, 3, 0), (2, 0, 0)),
        ("A", "B", "C", "D"),
        2,
    ),
    FixedCase(
        "shared_terminal_interior",
        ((-8, 1, 0), (-5, -7, 0), (-3, -8, 0), (4, 8, 0), (5, -7, 0)),
        ("A", "B", "C", "D", "E"),
        3,
    ),
)


def exact_level(record: object, path: str) -> Fraction:
    if not isinstance(record, dict) or set(record) != {"denominator", "numerator"}:
        raise AssertionError(f"{path} is not an exact level record")
    denominator = record["denominator"]
    numerator = record["numerator"]
    if not isinstance(denominator, str) or not isinstance(numerator, str):
        raise AssertionError(f"{path} numerator and denominator must be strings")
    value = Fraction(int(numerator), int(denominator))
    if str(value.numerator) != numerator or str(value.denominator) != denominator:
        raise AssertionError(f"{path} is not a reduced canonical fraction")
    return value


def canonical_label(value: object, size: int, path: str) -> tuple[int, ...]:
    if not isinstance(value, list) or len(value) != size:
        raise AssertionError(f"{path} must be a point label of size {size}")
    if any(isinstance(item, bool) or not isinstance(item, int) for item in value):
        raise AssertionError(f"{path} contains a non-integer point identifier")
    label = tuple(value)
    if label != tuple(sorted(set(label))):
        raise AssertionError(f"{path} is not sorted and duplicate-free")
    return label


def target_key(record: dict[str, object], order: int, path: str) -> tuple[object, ...]:
    closed_label = canonical_label(
        record.get("event_closed_point_ids"),
        order + 1,
        f"{path}.event_closed_point_ids",
    )
    level = exact_level(
        record.get("event_squared_level"), f"{path}.event_squared_level"
    )
    removed = record.get("removed_shell_point_id")
    if isinstance(removed, bool) or not isinstance(removed, int):
        raise AssertionError(f"{path}.removed_shell_point_id is not an integer")
    return level, closed_label, removed


def reduced_group_kind(batch: object, event_facets: tuple[tuple[int, ...], ...]) -> str:
    event_facet_set = set(event_facets)
    post_components = [
        component
        for component in batch.post_components
        if event_facet_set <= set(component.facet_point_ids)
    ]
    if len(post_components) != 1:
        raise AssertionError(
            "an exact saddle does not select one reduced post-lot group"
        )
    post_facets = set(post_components[0].facet_point_ids)
    prior_count = sum(
        set(component.facet_point_ids) <= post_facets
        for component in batch.pre_components
    )
    if prior_count == 0:
        return "birth"
    if prior_count == 1:
        return "continuation"
    return "multifusion"


def expected_records(
    fixed_case: FixedCase,
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
) -> tuple[list[dict[str, object]], dict[tuple[int, ...], Fraction]]:
    catalog = build_critical_catalog(points, fixed_case.order)
    if catalog.points != points:
        raise AssertionError(
            f"{fixed_case.name}: catalogue changed canonical point IDs"
        )
    if not catalog.relevant_gp_complete:
        raise AssertionError(f"{fixed_case.name}: fixed case violates RelevantGP")

    filtration = build_gamma_filtration(points, fixed_case.order)
    facet_levels = {facet.point_ids: facet.squared_level for facet in filtration.facets}
    reduced_forest = build_merge_forest(filtration, "hgp_reduced")
    reduced_batches = {batch.squared_level: batch for batch in reduced_forest.batches}
    strict_cuts: dict[Fraction, object] = {}
    records = []

    for event in catalog.events:
        if event.saddle_order != fixed_case.order:
            continue
        level = event.squared_radius
        if level not in strict_cuts:
            strict_cuts[level] = filtration.cut(level, closed=False, graph_kind="gamma")
        strict_cut = strict_cuts[level]
        component_by_facet = strict_cut.component_by_facet
        event_cofaces = [
            coface
            for coface in filtration.cofaces
            if coface.point_ids == event.point_ids and coface.squared_level == level
        ]
        if len(event_cofaces) != 1:
            raise AssertionError("a critical saddle has no unique exact Gamma coface")
        batch = reduced_batches.get(level)
        if batch is None:
            raise AssertionError("a critical saddle has no reduced equality batch")
        group_kind = reduced_group_kind(batch, event_cofaces[0].facet_point_ids)

        for removed_shell_id in event.shell_ids:
            initial_facet = tuple(
                point_id for point_id in event.point_ids if point_id != removed_shell_id
            )
            target_component = component_by_facet.get(initial_facet)
            if target_component is None:
                raise AssertionError(
                    "a strict initial critical arm has no Gamma component"
                )
            retained_by_reduction = any(
                component.facet_point_ids == target_component.facet_point_ids
                for component in batch.pre_components
            )
            if retained_by_reduction != target_component.nontrivial:
                raise AssertionError(
                    "the exact reduced pre-lot state disagrees with strict Gamma"
                )
            records.append(
                {
                    "event_closed_point_ids": list(event.point_ids),
                    "event_squared_level": ratio_record(level),
                    "group_kind": group_kind,
                    "initial_facet_point_ids": list(initial_facet),
                    "reduced_component_kind": (
                        "prior_nontrivial_reduced_root"
                        if retained_by_reduction
                        else "omitted_isolated_facet"
                    ),
                    "removed_shell_point_id": removed_shell_id,
                    "target_component_facet_point_ids": [
                        list(facet) for facet in target_component.facet_point_ids
                    ],
                }
            )
    records.sort(
        key=lambda record: (
            exact_level(record["event_squared_level"], "expected.event_squared_level"),
            tuple(record["event_closed_point_ids"]),
            record["removed_shell_point_id"],
        )
    )
    return records, facet_levels


def validate_case(fixed_case: FixedCase, actual: dict[str, object]) -> None:
    expected_top_level_fields = {
        "arm_targets",
        "canonical_points",
        "case",
        "decision",
        "order",
    }
    if set(actual) != expected_top_level_fields:
        raise AssertionError(
            "$ has unexpected fields: "
            f"{sorted(set(actual) ^ expected_top_level_fields)!r}"
        )
    canonical_points, points, _ = emitted_point_order(fixed_case, actual)
    for field, expected in (
        ("canonical_points", canonical_points),
        ("case", fixed_case.name),
        ("decision", COMPLETE_DECISION),
        ("order", fixed_case.order),
    ):
        difference = first_difference(expected, actual.get(field), f"$.{field}")
        if difference is not None:
            raise AssertionError(difference)

    actual_records = actual.get("arm_targets")
    if not isinstance(actual_records, list):
        raise AssertionError("$.arm_targets is not an array")
    expected, facet_levels = expected_records(fixed_case, points)
    if len(actual_records) != len(expected):
        raise AssertionError(
            f"$.arm_targets: expected {len(expected)} records, observed {len(actual_records)}"
        )

    expected_keys = [
        target_key(record, fixed_case.order, f"expected[{index}]")
        for index, record in enumerate(expected)
    ]
    actual_keys = []
    for index, record in enumerate(actual_records):
        if not isinstance(record, dict):
            raise AssertionError(f"$.arm_targets[{index}] is not an object")
        expected_fields = set(expected[index]) | {"terminal_facet_point_ids"}
        if set(record) != expected_fields:
            raise AssertionError(
                f"$.arm_targets[{index}] has unexpected fields: "
                f"{sorted(set(record) ^ expected_fields)!r}"
            )
        actual_keys.append(
            target_key(record, fixed_case.order, f"$.arm_targets[{index}]")
        )
    if actual_keys != expected_keys:
        raise AssertionError(
            f"$.arm_targets semantic keys differ: expected {expected_keys!r}, "
            f"observed {actual_keys!r}"
        )

    for index, (expected_record, actual_record) in enumerate(
        zip(expected, actual_records)
    ):
        for field, expected_value in expected_record.items():
            difference = first_difference(
                expected_value,
                actual_record.get(field),
                f"$.arm_targets[{index}].{field}",
            )
            if difference is not None:
                raise AssertionError(difference)

        terminal = canonical_label(
            actual_record.get("terminal_facet_point_ids"),
            fixed_case.order,
            f"$.arm_targets[{index}].terminal_facet_point_ids",
        )
        target_facets = {
            tuple(facet)
            for facet in expected_record["target_component_facet_point_ids"]
        }
        if terminal not in target_facets:
            raise AssertionError(
                f"$.arm_targets[{index}]: terminal facet is outside the "
                "independently reconstructed initial-arm component"
            )
        event_level = exact_level(
            expected_record["event_squared_level"],
            f"$.arm_targets[{index}].event_squared_level",
        )
        if facet_levels.get(terminal, event_level) >= event_level:
            raise AssertionError(
                f"$.arm_targets[{index}]: terminal facet is not strictly pre-lot"
            )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "executable", type=Path, help="path to the catalogue-arm Gamma overlay dump"
    )
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
        [str(executable)],
        check=False,
        capture_output=True,
        text=True,
        timeout=arguments.timeout_seconds,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            "catalogue-arm Gamma overlay dump exited with "
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
        validate_case(fixed_case, actual)

    print("catalogue-arm Gamma overlay differential passed for 2 exact cases")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        AssertionError,
        OSError,
        RuntimeError,
        ValueError,
        subprocess.TimeoutExpired,
    ) as error:
        print(
            f"catalogue-arm Gamma overlay differential failed: {error}", file=sys.stderr
        )
        raise SystemExit(1) from error
