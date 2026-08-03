#!/usr/bin/env python3
"""Behavioral DBSCAN/HDBSCAN differential for an exact-relative K=2 tower."""

from __future__ import annotations

import argparse
import subprocess
import sys
import warnings
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path


SKIP_RETURN_CODE = 77
EXPECTED_SCHEMA = "morsehgp3d.point_hierarchy.sklearn.v1"
EXPECTED_POINT_COUNT = 9


@dataclass(frozen=True, slots=True)
class Dump:
    points: tuple[tuple[float, float], ...]
    dbscan_epsilon: float
    dbscan_min_samples: int
    hdbscan_min_cluster_size: int
    hdbscan_min_samples: int
    dbscan_labels: tuple[int, ...]
    eom_labels: tuple[int, ...]


def _load_sklearn() -> tuple[object, object, object]:
    try:
        import numpy as np
        from sklearn.cluster import DBSCAN, HDBSCAN
    except (ImportError, AttributeError) as error:
        print(f"SKIP: scikit-learn with HDBSCAN is unavailable: {error}")
        raise SystemExit(SKIP_RETURN_CODE) from error
    return np, DBSCAN, HDBSCAN


def _single_value(records: dict[str, list[list[str]]], key: str) -> str:
    values = records.get(key, [])
    if len(values) != 1 or len(values[0]) != 1:
        raise AssertionError(f"dump field {key!r} must occur once with one value")
    return values[0][0]


def _labels(records: dict[str, list[list[str]]], key: str) -> tuple[int, ...]:
    values = records.get(key, [])
    if len(values) != 1 or len(values[0]) != EXPECTED_POINT_COUNT:
        raise AssertionError(
            f"dump field {key!r} must contain {EXPECTED_POINT_COUNT} labels"
        )
    return tuple(int(value) for value in values[0])


def _parse_dump(text: str) -> Dump:
    records: dict[str, list[list[str]]] = {}
    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        fields = raw_line.split()
        if not fields:
            continue
        records.setdefault(fields[0], []).append(fields[1:])
        if len(fields) == 1:
            raise AssertionError(f"empty record on dump line {line_number}")

    if _single_value(records, "schema") != EXPECTED_SCHEMA:
        raise AssertionError("the C++ dump uses an unexpected schema")

    point_records = records.get("point", [])
    if len(point_records) != EXPECTED_POINT_COUNT:
        raise AssertionError(f"the dump must contain {EXPECTED_POINT_COUNT} points")
    points_by_index: dict[int, tuple[float, float]] = {}
    for fields in point_records:
        if len(fields) != 3:
            raise AssertionError("a point record must contain index, x and y")
        index = int(fields[0])
        if index in points_by_index:
            raise AssertionError(f"point index {index} occurs twice")
        points_by_index[index] = (float(fields[1]), float(fields[2]))
    if set(points_by_index) != set(range(EXPECTED_POINT_COUNT)):
        raise AssertionError("the point indices are not exactly 0,...,8")

    return Dump(
        points=tuple(points_by_index[index] for index in range(EXPECTED_POINT_COUNT)),
        dbscan_epsilon=float(_single_value(records, "dbscan_epsilon")),
        dbscan_min_samples=int(_single_value(records, "dbscan_min_samples")),
        hdbscan_min_cluster_size=int(
            _single_value(records, "hdbscan_min_cluster_size")
        ),
        hdbscan_min_samples=int(_single_value(records, "hdbscan_min_samples")),
        dbscan_labels=_labels(records, "dbscan_labels"),
        eom_labels=_labels(records, "eom_labels"),
    )


def _partition_signature(
    labels: Sequence[int],
) -> tuple[frozenset[int], frozenset[frozenset[int]]]:
    noise = frozenset(index for index, label in enumerate(labels) if label < 0)
    members_by_label: dict[int, set[int]] = {}
    for index, label in enumerate(labels):
        if label >= 0:
            members_by_label.setdefault(int(label), set()).add(index)
    clusters = frozenset(
        frozenset(members) for members in members_by_label.values()
    )
    return noise, clusters


def _assert_same_partition(
    name: str, actual: Sequence[int], expected: Sequence[int]
) -> None:
    actual_signature = _partition_signature(actual)
    expected_signature = _partition_signature(expected)
    if actual_signature != expected_signature:
        raise AssertionError(
            f"{name} partition mismatch: MorseHGP3D={actual_signature}, "
            f"scikit-learn={expected_signature}"
        )


def _run(executable: Path) -> None:
    np, DBSCAN, HDBSCAN = _load_sklearn()
    completed = subprocess.run(
        [str(executable)],
        check=True,
        capture_output=True,
        text=True,
        timeout=20,
    )
    dump = _parse_dump(completed.stdout)
    point_array = np.asarray(dump.points, dtype=float)

    dbscan_reference = DBSCAN(
        eps=dump.dbscan_epsilon,
        min_samples=dump.dbscan_min_samples,
        metric="euclidean",
    ).fit_predict(point_array)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", FutureWarning)
        hdbscan_reference = HDBSCAN(
            min_cluster_size=dump.hdbscan_min_cluster_size,
            min_samples=dump.hdbscan_min_samples,
            metric="euclidean",
            cluster_selection_method="eom",
            allow_single_cluster=False,
        ).fit_predict(point_array)

    _assert_same_partition("DBSCAN", dump.dbscan_labels, dbscan_reference)
    _assert_same_partition("HDBSCAN EOM", dump.eom_labels, hdbscan_reference)
    print("point-hierarchy DBSCAN/HDBSCAN behavioral differential passed")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    arguments = parser.parse_args()
    _run(arguments.executable)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        if error.stdout:
            print(error.stdout, file=sys.stderr, end="")
        if error.stderr:
            print(error.stderr, file=sys.stderr, end="")
        raise
