#!/usr/bin/env python3
"""Independently replay the Phase 15 industrial k=1/k=2 guards.

The input is the binary ``MHGPIE2E`` export written outside the timed region by
``morsehgp3d_gpu_guarded_industrial_e2e``.  This checker deliberately rebuilds
the generated cloud from the exported seed, computes an ordinary 3-D Delaunay
triangulation with SciPy/Qhull, and never consumes the runner's neighbor graph.

Two unilateral scientific guards are replayed:

* order one must reproduce the canonical ordinary-Delaunay EMST, including
  its binary64 edge levels and every closed connectivity deadline;
* order two must connect every valid triangle having at least two ordinary
  Delaunay edges no later than its triangle-miniball level.  In accordance
  with the existing surrogate convention, a raw mutual-reachability squared
  weight is compared after the exact dyadic conversion ``weight / 4`` and an
  equal-level plateau is accepted.

The second source universe is intentionally stronger than the historical
conditional Gabriel guardrail.  The report also gives the old
``gabriel_binary64 / ambiguous / blocked`` partition, but does not turn that
binary64 classification into an exact Gabriel or Gamma_2 claim.  Ordinary
Delaunay and the global wedge replay are offline oracle sidecars only.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Iterator, Mapping, Sequence

import numpy as np


SCHEMA = "morsehgp3d.phase15_guarded_industrial_scientific_guards.v5"
RUNNER_SCHEMA_V4 = "morsehgp3d.phase15.guarded_industrial_candidate.v4"
RUNNER_SCHEMA_V5 = "morsehgp3d.phase15.guarded_industrial_candidate.v5"
# Historical imports keep naming the v4 contract explicitly.
RUNNER_SCHEMA = RUNNER_SCHEMA_V4
RUNNER_MAXIMUM_ORDER = 10
COMPONENT_BRIDGE_MAXIMUM_COMPONENT_COUNT = 256
COMPONENT_BRIDGE_MAXIMUM_CENTROID_DISTANCE_EVALUATION_COUNT = 256 * 255
COMPONENT_BRIDGE_MEMBER_PROJECTION_BUDGET_FACTOR = 16
COMPONENT_BRIDGE_MAXIMUM_PAIR_COUNT = 256 * 6
COMPONENT_BRIDGE_MAXIMUM_CROSS_DISTANCE_EVALUATION_COUNT = 64 * 1536
EXPORT_MAGIC = b"MHGPIE2E"
TREE_MAGIC = b"H0TREE01"
EXPORT_VERSION = 1
TREE_VERSION = 1
ENDIAN_MARKER = 0x01020304
UINT64_MASK = (1 << 64) - 1
UINT32_MAX = (1 << 32) - 1
PREDICATE_TOLERANCE_MULTIPLIER = 1024.0
FAMILIES = (
    "affine_uniform_binary64",
    "jittered_dyadic_grid3d",
    "balanced_multiscale_clusters",
)
FNV1A_OFFSET_BASIS = 1469598103934665603
FNV1A_PRIME = 1099511628211

_GIT_SHA = re.compile(r"[0-9a-f]{40}")
_SHA256 = re.compile(r"[0-9a-f]{64}")
_RUNNER_DIGEST = re.compile(r"0x[0-9a-f]{16}")
_CUDA_VERSION = re.compile(r"CUDA Version [0-9]+\.[0-9]+\.[0-9]+")
_CUDA_COPYRIGHT = re.compile(
    r"Container image Copyright \(c\) [0-9]{4}-[0-9]{4}, "
    r"NVIDIA CORPORATION & AFFILIATES\. All rights reserved\."
)
_CUDA_BANNER_FIXED_LINES = (
    "==========",
    "== CUDA ==",
    "==========",
    "This container image and its contents are governed by the NVIDIA Deep Learning Container License.",
    "By pulling and using the container, you accept the terms and conditions of this license:",
    "https://developer.nvidia.com/ngc/nvidia-deep-learning-container-license",
    "A copy of this license is made available in this container at /NGC-DL-CONTAINER-LICENSE for your convenience.",
)


class ScientificGuardError(ValueError):
    """The export or oracle replay cannot support a fail-closed decision."""


@dataclass(frozen=True)
class ExportedTree:
    order: int
    first: np.ndarray
    second: np.ndarray
    squared_levels: np.ndarray
    squared_level_bits: np.ndarray


@dataclass(frozen=True)
class IndustrialExport:
    point_count: int
    seed: int
    tree_count: int
    trees: Mapping[int, ExportedTree]
    artifact_sha256: str


@dataclass(frozen=True)
class DelaunayOracle:
    simplex_count: int
    edges: np.ndarray
    qhull_options: str
    exact_translation_used: bool
    deterministic_joggle_used: bool
    primary_failure: str | None


@dataclass(frozen=True)
class _CsrGraph:
    offsets: np.ndarray
    neighbors: np.ndarray
    edge_keys: np.ndarray
    raw_wedge_count: int
    maximum_degree: int


@dataclass(frozen=True)
class _Miniballs:
    centers: np.ndarray
    squared_radii: np.ndarray
    support_cardinalities: np.ndarray
    valid: np.ndarray
    ambiguous: np.ndarray


@dataclass(frozen=True)
class _ExactMiniball:
    squared_radius: Fraction
    support_cardinality: int


class _DisjointSet:
    def __init__(self, size: int) -> None:
        self.parent = list(range(size))
        self.sizes = [1] * size
        self.component_count = size

    def find(self, value: int) -> int:
        root = value
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[value] != value:
            following = self.parent[value]
            self.parent[value] = root
            value = following
        return root

    def union(self, left: int, right: int) -> bool:
        left = self.find(left)
        right = self.find(right)
        if left == right:
            return False
        if self.sizes[left] < self.sizes[right] or (
            self.sizes[left] == self.sizes[right] and right < left
        ):
            left, right = right, left
        self.parent[right] = left
        self.sizes[left] += self.sizes[right]
        self.component_count -= 1
        return True


def _read_exact(data: memoryview, offset: int, size: int, role: str) -> tuple[memoryview, int]:
    end = offset + size
    if end > len(data):
        raise ScientificGuardError(f"truncated export while reading {role}")
    return data[offset:end], end


def _read_u32(data: memoryview, offset: int, role: str) -> tuple[int, int]:
    raw, offset = _read_exact(data, offset, 4, role)
    return struct.unpack_from("<I", raw)[0], offset


def _read_u64(data: memoryview, offset: int, role: str) -> tuple[int, int]:
    raw, offset = _read_exact(data, offset, 8, role)
    return struct.unpack_from("<Q", raw)[0], offset


def _validate_tree_arrays(
    *,
    point_count: int,
    order: int,
    first: np.ndarray,
    second: np.ndarray,
    level_bits: np.ndarray,
) -> np.ndarray:
    if len(first) != point_count - 1:
        raise ScientificGuardError(
            f"order {order} must contain exactly point_count - 1 merges"
        )
    if np.any(first >= point_count) or np.any(second >= point_count):
        raise ScientificGuardError(f"order {order} contains an out-of-range PointId")
    if np.any(first >= second):
        raise ScientificGuardError(f"order {order} merge endpoints must satisfy u < v")
    native_bits = np.asarray(level_bits, dtype=np.uint64)
    levels = native_bits.view(np.float64)
    if np.any((native_bits >> np.uint64(63)) != 0):
        raise ScientificGuardError(f"order {order} contains a negative signed level")
    if np.any(~np.isfinite(levels)):
        raise ScientificGuardError(f"order {order} contains a non-finite level")
    pair_keys = (first.astype(np.uint64) << np.uint64(32)) | second.astype(np.uint64)
    if len(levels) > 1:
        descending = levels[1:] < levels[:-1]
        tied_out_of_order = (levels[1:] == levels[:-1]) & (
            pair_keys[1:] <= pair_keys[:-1]
        )
        if np.any(descending | tied_out_of_order):
            raise ScientificGuardError(
                f"order {order} merges are not strictly ordered by (level, u, v)"
            )
    components = _DisjointSet(point_count)
    for left, right in zip(first, second):
        if not components.union(int(left), int(right)):
            raise ScientificGuardError(f"order {order} merge records contain a cycle")
    if components.component_count != 1:
        raise ScientificGuardError(f"order {order} merge records do not span")
    return levels.copy()


def read_industrial_export(
    path: Path,
    *,
    expected_point_count: int | None = 50_000,
    expected_tree_count: int | None = 10,
) -> IndustrialExport:
    try:
        payload = path.read_bytes()
    except OSError as error:
        raise ScientificGuardError(f"cannot read export {path}: {error}") from error
    data = memoryview(payload)
    offset = 0
    magic, offset = _read_exact(data, offset, 8, "export magic")
    if bytes(magic) != EXPORT_MAGIC:
        raise ScientificGuardError("the export magic is not MHGPIE2E")
    version, offset = _read_u32(data, offset, "export version")
    marker, offset = _read_u32(data, offset, "endianness marker")
    point_count, offset = _read_u64(data, offset, "point count")
    seed, offset = _read_u64(data, offset, "seed")
    tree_count, offset = _read_u64(data, offset, "tree count")
    if version != EXPORT_VERSION or marker != ENDIAN_MARKER:
        raise ScientificGuardError("the export version or endianness marker is unsupported")
    if point_count < 4 or point_count > UINT32_MAX:
        raise ScientificGuardError("point_count must lie in 4..2^32-1")
    if expected_point_count is not None and point_count != expected_point_count:
        raise ScientificGuardError(
            f"point_count={point_count} differs from expected {expected_point_count}"
        )
    if expected_tree_count is not None and tree_count != expected_tree_count:
        raise ScientificGuardError(
            f"tree_count={tree_count} differs from expected {expected_tree_count}"
        )
    if tree_count == 0 or tree_count > 64:
        raise ScientificGuardError("tree_count must lie in 1..64")

    retained: dict[int, ExportedTree] = {}
    seen_orders: set[int] = set()
    record_dtype = np.dtype([("u", "<u8"), ("v", "<u8"), ("bits", "<u8")])
    for tree_index in range(tree_count):
        prefix = f"tree[{tree_index}]"
        magic, offset = _read_exact(data, offset, 8, f"{prefix} magic")
        if bytes(magic) != TREE_MAGIC:
            raise ScientificGuardError(f"{prefix} magic is not H0TREE01")
        tree_version, offset = _read_u32(data, offset, f"{prefix} version")
        reserved, offset = _read_u32(data, offset, f"{prefix} reserved")
        tree_points, offset = _read_u64(data, offset, f"{prefix} point count")
        order, offset = _read_u64(data, offset, f"{prefix} order")
        tree_seed, offset = _read_u64(data, offset, f"{prefix} seed")
        merge_count, offset = _read_u64(data, offset, f"{prefix} merge count")
        if tree_version != TREE_VERSION or reserved != 0:
            raise ScientificGuardError(f"{prefix} header is unsupported")
        if tree_points != point_count or tree_seed != seed:
            raise ScientificGuardError(f"{prefix} header disagrees with the container")
        if order == 0 or order > tree_count or order in seen_orders:
            raise ScientificGuardError(f"{prefix} has an invalid or duplicate order")
        seen_orders.add(order)
        if merge_count != point_count - 1:
            raise ScientificGuardError(f"{prefix} has a wrong merge count")
        byte_count = merge_count * record_dtype.itemsize
        raw, offset = _read_exact(data, offset, byte_count, f"{prefix} records")
        records = np.frombuffer(raw, dtype=record_dtype, count=merge_count)
        first = np.asarray(records["u"], dtype=np.uint64).copy()
        second = np.asarray(records["v"], dtype=np.uint64).copy()
        bits = np.asarray(records["bits"], dtype=np.uint64).copy()
        levels = _validate_tree_arrays(
            point_count=point_count,
            order=int(order),
            first=first,
            second=second,
            level_bits=bits,
        )
        retained[int(order)] = ExportedTree(
            int(order), first, second, levels, bits
        )
    if offset != len(data):
        raise ScientificGuardError("the export contains trailing bytes")
    expected_orders = set(range(1, int(tree_count) + 1))
    if seen_orders != expected_orders:
        raise ScientificGuardError("the export orders are not exactly 1..tree_count")
    if set(retained) != expected_orders:
        raise ScientificGuardError("all exported orders must be retained")
    return IndustrialExport(
        int(point_count),
        seed,
        int(tree_count),
        retained,
        hashlib.sha256(payload).hexdigest(),
    )


def _reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ScientificGuardError(f"duplicate runner report JSON key: {key}")
        result[key] = value
    return result


def _reject_non_finite_json(value: str) -> None:
    raise ScientificGuardError(f"non-finite runner report JSON value: {value}")


def _validate_cuda_banner(prefix: str, *, path: Path) -> None:
    lines = tuple(line.strip() for line in prefix.splitlines() if line.strip())
    if len(lines) != 9:
        raise ScientificGuardError(f"{path}: unrecognized text before JSON report")
    if lines[:3] != _CUDA_BANNER_FIXED_LINES[:3]:
        raise ScientificGuardError(f"{path}: invalid CUDA banner")
    if _CUDA_VERSION.fullmatch(lines[3]) is None:
        raise ScientificGuardError(f"{path}: invalid CUDA version banner")
    if _CUDA_COPYRIGHT.fullmatch(lines[4]) is None:
        raise ScientificGuardError(f"{path}: invalid CUDA copyright banner")
    if lines[5:] != _CUDA_BANNER_FIXED_LINES[3:]:
        raise ScientificGuardError(f"{path}: invalid CUDA license banner")


def _load_runner_report(path: Path) -> tuple[dict[str, object], str]:
    try:
        raw_bytes = path.read_bytes()
        raw = raw_bytes.decode("utf-8")
    except (OSError, UnicodeError) as error:
        raise ScientificGuardError(
            f"cannot read runner report {path}: {error}"
        ) from error
    json_start = raw.find("{")
    if json_start < 0:
        raise ScientificGuardError(f"{path}: runner report has no JSON object")
    prefix = raw[:json_start]
    if prefix.strip():
        _validate_cuda_banner(prefix, path=path)
    try:
        report = json.loads(
            raw[json_start:].strip(),
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=_reject_non_finite_json,
        )
    except json.JSONDecodeError as error:
        raise ScientificGuardError(
            f"cannot parse runner report {path}: {error}"
        ) from error
    if type(report) is not dict:
        raise ScientificGuardError(f"{path}: runner report must be a JSON object")
    return report, hashlib.sha256(raw_bytes).hexdigest()


def _require_local_commit(git_sha: str) -> None:
    repository = Path(__file__).resolve().parents[1]
    try:
        completed = subprocess.run(
            ["git", "-C", str(repository), "cat-file", "-e", f"{git_sha}^{{commit}}"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    except OSError as error:
        raise ScientificGuardError(
            f"cannot verify expected Git commit: {error}"
        ) from error
    if completed.returncode != 0:
        raise ScientificGuardError(
            f"expected Git SHA is not a commit in {repository}"
        )


def _fnv1a_digest_word(digest: int, word: int) -> int:
    for byte in range(8):
        digest ^= (word >> (byte * 8)) & 0xFF
        digest = (digest * FNV1A_PRIME) & UINT64_MASK
    return digest


def runner_hierarchy_digest(tree: ExportedTree) -> str:
    """Replay the runner's FNV-1a digest over exact little-endian words."""

    digest = _fnv1a_digest_word(FNV1A_OFFSET_BASIS, tree.order)
    for first, second, level_bits in zip(
        tree.first, tree.second, tree.squared_level_bits
    ):
        digest = _fnv1a_digest_word(digest, int(first))
        digest = _fnv1a_digest_word(digest, int(second))
        digest = _fnv1a_digest_word(digest, int(level_bits))
    return f"0x{digest:016x}"


def _binary64_bits(value: object, *, role: str) -> int:
    if type(value) not in (int, float):
        raise ScientificGuardError(f"{role} must be a finite non-negative number")
    try:
        binary64 = float(value)
    except (OverflowError, ValueError) as error:
        raise ScientificGuardError(
            f"{role} cannot be represented as binary64"
        ) from error
    if not math.isfinite(binary64) or binary64 < 0.0:
        raise ScientificGuardError(f"{role} must be a finite non-negative number")
    return struct.unpack("<Q", struct.pack("<d", binary64))[0]


def _sha256_file(path: Path, *, role: str) -> tuple[str, int]:
    digest = hashlib.sha256()
    size = 0
    try:
        with path.open("rb") as artifact:
            while chunk := artifact.read(1024 * 1024):
                digest.update(chunk)
                size += len(chunk)
    except OSError as error:
        raise ScientificGuardError(f"cannot read {role} {path}: {error}") from error
    if size == 0:
        raise ScientificGuardError(f"{role} {path} must not be empty")
    return digest.hexdigest(), size


def _bind_runner_report(
    exported: IndustrialExport,
    *,
    runner_report_path: Path,
    family: str,
    expected_git_sha: str,
    runner_binary_path: Path,
    expected_runner_binary_sha256: str,
) -> dict[str, object]:
    if _GIT_SHA.fullmatch(expected_git_sha) is None:
        raise ScientificGuardError(
            "expected Git SHA must be one lowercase full 40-hex SHA"
        )
    if _SHA256.fullmatch(expected_runner_binary_sha256) is None:
        raise ScientificGuardError(
            "expected runner binary SHA-256 must be one lowercase 64-hex digest"
        )
    _require_local_commit(expected_git_sha)
    runner_binary_sha256, runner_binary_size = _sha256_file(
        runner_binary_path, role="runner binary"
    )
    if runner_binary_sha256 != expected_runner_binary_sha256:
        raise ScientificGuardError(
            "runner binary bytes disagree with the expected SHA-256 commitment"
        )
    report, report_sha256 = _load_runner_report(runner_report_path)
    runner_schema = report.get("schema")
    if runner_schema not in (RUNNER_SCHEMA_V4, RUNNER_SCHEMA_V5):
        raise ScientificGuardError(
            "runner report schema must be one supported guarded industrial schema"
        )
    reported_binary_sha256 = report.get("runner_binary_sha256")
    if (
        type(reported_binary_sha256) is not str
        or _SHA256.fullmatch(reported_binary_sha256) is None
    ):
        raise ScientificGuardError(
            "runner report runner_binary_sha256 is not canonical"
        )
    reported_binary_size = report.get("runner_binary_size_bytes")
    if type(reported_binary_size) is not int or reported_binary_size <= 0:
        raise ScientificGuardError(
            "runner report runner_binary_size_bytes must be positive"
        )
    if (
        reported_binary_sha256 != runner_binary_sha256
        or reported_binary_size != runner_binary_size
    ):
        raise ScientificGuardError(
            "runner report self-attestation disagrees with the runner binary bytes"
        )
    if report.get("family") != family:
        raise ScientificGuardError("runner report family disagrees with --family")
    if (
        type(report.get("point_count")) is not int
        or report["point_count"] != exported.point_count
    ):
        raise ScientificGuardError("runner report point_count disagrees with the export")
    maximum_order = report.get("maximum_order")
    if runner_schema == RUNNER_SCHEMA_V4:
        if type(maximum_order) is not int or maximum_order != exported.tree_count:
            raise ScientificGuardError(
                "runner report tree_count disagrees with the export"
            )
        reported_hierarchy_count = exported.tree_count
    else:
        if type(maximum_order) is not int or maximum_order != RUNNER_MAXIMUM_ORDER:
            raise ScientificGuardError(
                "v5 runner report maximum_order must equal 10"
            )
        if (
            type(report.get("export_maximum_order")) is not int
            or report["export_maximum_order"] != exported.tree_count
        ):
            raise ScientificGuardError(
                "v5 runner report export_maximum_order disagrees with the export"
            )
        if exported.tree_count < 2 or exported.tree_count > RUNNER_MAXIMUM_ORDER:
            raise ScientificGuardError(
                "v5 runner export must contain contiguous orders 1 and 2"
            )
        reported_hierarchy_count = RUNNER_MAXIMUM_ORDER
        expected_report_literals = {
            "mode": (
                "warm_fresh_cloud_lbvh_top10_capped_"
                "component_bridges_parallel_h0"
            ),
            "component_bridge_policy": (
                "top10_components_capped_centroid_knn_top8_"
                "projection_cross_min"
            ),
            "component_bridge_maximum_component_count": (
                COMPONENT_BRIDGE_MAXIMUM_COMPONENT_COUNT
            ),
            "component_bridge_maximum_centroid_distance_evaluation_count": (
                COMPONENT_BRIDGE_MAXIMUM_CENTROID_DISTANCE_EVALUATION_COUNT
            ),
            "component_bridge_member_projection_budget_factor": (
                COMPONENT_BRIDGE_MEMBER_PROJECTION_BUDGET_FACTOR
            ),
            "component_bridge_maximum_pair_count": COMPONENT_BRIDGE_MAXIMUM_PAIR_COUNT,
            "component_bridge_maximum_cross_distance_evaluation_count": (
                COMPONENT_BRIDGE_MAXIMUM_CROSS_DISTANCE_EVALUATION_COUNT
            ),
        }
        for key, expected in expected_report_literals.items():
            if report.get(key) != expected or type(report.get(key)) is not type(expected):
                raise ScientificGuardError(
                    f"v5 runner report {key} disagrees with the bounded contract"
                )
    if report.get("git_sha") != expected_git_sha:
        raise ScientificGuardError(
            "runner report Git SHA disagrees with --expected-git-sha"
        )

    runs = report.get("runs")
    if type(runs) is not list or not runs:
        raise ScientificGuardError("runner report runs must be a non-empty array")
    exported_runs: list[tuple[int, dict[str, object]]] = []
    for run_offset, raw_run in enumerate(runs):
        if type(raw_run) is not dict:
            raise ScientificGuardError(
                f"runner report runs[{run_offset}] must be an object"
            )
        if "oracle_export_path" not in raw_run:
            raise ScientificGuardError(
                f"runner report runs[{run_offset}] lacks oracle_export_path"
            )
        export_path = raw_run["oracle_export_path"]
        if export_path is None:
            continue
        if type(export_path) is not str or not export_path:
            raise ScientificGuardError(
                f"runner report runs[{run_offset}].oracle_export_path is invalid"
            )
        exported_runs.append((run_offset, raw_run))
    if len(exported_runs) != 1:
        raise ScientificGuardError(
            "runner report must contain exactly one run with a tree export"
        )
    run_offset, run = exported_runs[0]
    if run.get("warmup") is not False or type(run.get("measured_index")) is not int:
        raise ScientificGuardError("the uniquely exported runner run must be measured")
    if type(run.get("run_index")) is not int or run["run_index"] != run_offset:
        raise ScientificGuardError("the exported runner run_index is inconsistent")
    if type(run.get("oracle_export_ns")) is not int or run["oracle_export_ns"] <= 0:
        raise ScientificGuardError("the exported runner run must report export work")
    if type(run.get("seed")) is not int or run["seed"] != exported.seed:
        raise ScientificGuardError("runner report seed disagrees with the export")

    if runner_schema == RUNNER_SCHEMA_V5:
        component_count = run.get("top_k_component_count")
        if (
            type(component_count) is not int
            or component_count < 1
            or component_count > COMPONENT_BRIDGE_MAXIMUM_COMPONENT_COUNT
        ):
            raise ScientificGuardError(
                "v5 exported run top_k_component_count must lie in 1..256"
            )
        centroid_count = run.get(
            "component_bridge_centroid_distance_evaluation_count"
        )
        if (
            type(centroid_count) is not int
            or centroid_count != component_count * (component_count - 1)
        ):
            raise ScientificGuardError(
                "v5 exported run centroid distance count must equal C(C-1)"
            )
        projection_budget = run.get(
            "component_bridge_member_projection_evaluation_budget"
        )
        expected_projection_budget = (
            COMPONENT_BRIDGE_MEMBER_PROJECTION_BUDGET_FACTOR * exported.point_count
        )
        if (
            type(projection_budget) is not int
            or projection_budget != expected_projection_budget
        ):
            raise ScientificGuardError(
                "v5 exported run projection budget must equal 16n"
            )
        projection_count = run.get(
            "component_bridge_member_projection_evaluation_count"
        )
        if (
            type(projection_count) is not int
            or projection_count < 0
            or projection_count > projection_budget
        ):
            raise ScientificGuardError(
                "v5 exported run projection count exceeds its budget"
            )
        cross_budget = run.get("component_bridge_cross_distance_evaluation_budget")
        if (
            type(cross_budget) is not int
            or cross_budget
            != COMPONENT_BRIDGE_MAXIMUM_CROSS_DISTANCE_EVALUATION_COUNT
        ):
            raise ScientificGuardError(
                "v5 exported run cross-distance budget must equal 64*1536"
            )
        cross_count = run.get("component_bridge_cross_distance_evaluation_count")
        if (
            type(cross_count) is not int
            or cross_count < 0
            or cross_count > cross_budget
        ):
            raise ScientificGuardError(
                "v5 exported run cross-distance count exceeds its budget"
            )
        bridge_pair_count = run.get("component_bridge_pair_count")
        if (
            type(bridge_pair_count) is not int
            or bridge_pair_count < 0
            or bridge_pair_count > COMPONENT_BRIDGE_MAXIMUM_PAIR_COUNT
        ):
            raise ScientificGuardError(
                "v5 exported run bridge pair count exceeds 1536"
            )
        support_count = run.get("component_bridge_support_evaluation_count")
        if (
            type(support_count) is not int
            or support_count != projection_count + cross_count
        ):
            raise ScientificGuardError(
                "v5 exported run support count must equal projection plus cross"
            )
        if (
            run.get("component_bridge_budget_satisfied") is not True
            or run.get("component_bridge_budget_stop_reason") != "none"
        ):
            raise ScientificGuardError(
                "v5 exported run must satisfy the component bridge budget"
            )
        if component_count == 1 and any(
            count != 0
            for count in (
                bridge_pair_count,
                support_count,
                projection_count,
                cross_count,
            )
        ):
            raise ScientificGuardError(
                "v5 connected exported run must report zero component bridge work"
            )
        materialized_count = RUNNER_MAXIMUM_ORDER * (exported.point_count - 1)
        for key, expected in (
            ("materialized_merge_record_count", materialized_count),
            ("released_merge_record_count", materialized_count),
            ("retained_merge_record_count", 0),
            ("exported_order_count", exported.tree_count),
            (
                "exported_merge_record_count",
                exported.tree_count * (exported.point_count - 1),
            ),
        ):
            if run.get(key) != expected or type(run.get(key)) is not int:
                raise ScientificGuardError(
                    f"v5 exported run {key} disagrees with the export/release contract"
                )
        if run.get("merge_records_released_after_digest_and_export") is not True:
            raise ScientificGuardError(
                "v5 exported run did not release merge records after digest/export"
            )

    hierarchies = run.get("hierarchies")
    if type(hierarchies) is not list or len(hierarchies) != reported_hierarchy_count:
        raise ScientificGuardError(
            "the exported runner run has a wrong hierarchy count"
        )
    replayed: list[dict[str, object]] = []
    for offset, raw_hierarchy in enumerate(hierarchies):
        role = f"runner report runs[{run_offset}].hierarchies[{offset}]"
        if type(raw_hierarchy) is not dict:
            raise ScientificGuardError(f"{role} must be an object")
        order = offset + 1
        if (
            type(raw_hierarchy.get("order")) is not int
            or raw_hierarchy["order"] != order
        ):
            raise ScientificGuardError(f"{role}.order is not canonical")
        if (
            type(raw_hierarchy.get("merge_record_count")) is not int
            or raw_hierarchy["merge_record_count"] != exported.point_count - 1
        ):
            raise ScientificGuardError(f"{role}.merge_record_count is wrong")
        reported_digest = raw_hierarchy.get("digest")
        if (
            type(reported_digest) is not str
            or _RUNNER_DIGEST.fullmatch(reported_digest) is None
        ):
            raise ScientificGuardError(f"{role}.digest is not canonical")
        reported_root = raw_hierarchy.get("root_squared_level")
        reported_root_bits = _binary64_bits(
            reported_root, role=f"{role}.root_squared_level"
        )
        if order > exported.tree_count:
            continue
        tree = exported.trees[order]
        replayed_digest = runner_hierarchy_digest(tree)
        if reported_digest != replayed_digest:
            raise ScientificGuardError(
                f"{role}.digest disagrees with the exported merge records"
            )
        exported_root_bits = int(tree.squared_level_bits[-1])
        if reported_root_bits != exported_root_bits:
            raise ScientificGuardError(
                f"{role}.root_squared_level disagrees with the export"
            )
        replayed.append(
            {
                "order": order,
                "digest": replayed_digest,
                "root_squared_level": float(tree.squared_levels[-1]),
                "root_squared_level_bits": f"0x{exported_root_bits:016x}",
            }
        )

    return {
        "runner_report_path": str(runner_report_path),
        "runner_report_sha256": report_sha256,
        "runner_schema": runner_schema,
        "git_sha": expected_git_sha,
        "local_commit_object_verified": True,
        "runner_binary_path": str(runner_binary_path),
        "runner_binary_sha256": runner_binary_sha256,
        "expected_runner_binary_sha256": expected_runner_binary_sha256,
        "runner_binary_size_bytes": runner_binary_size,
        "runner_binary_bytes_hashed": True,
        "runner_report_binary_self_attestation_matched": True,
        "exported_measured_run_index": run["run_index"],
        "exported_measured_index": run["measured_index"],
        "exported_run_seed": run["seed"],
        **(
            {
                "reported_hierarchy_metadata_count": reported_hierarchy_count,
                "exported_hierarchy_count": exported.tree_count,
            }
            if runner_schema == RUNNER_SCHEMA_V5
            else {}
        ),
        "hierarchies": replayed,
        "all_hierarchy_digests_match_export": True,
        "all_root_squared_levels_match_export": True,
    }


def splitmix64(value: int) -> int:
    value = (value + 0x9E3779B97F4A7C15) & UINT64_MASK
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & UINT64_MASK
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & UINT64_MASK
    return (value ^ (value >> 31)) & UINT64_MASK


def _rotl64(value: int, shift: int) -> int:
    shift &= 63
    return ((value << shift) | (value >> (64 - shift))) & UINT64_MASK


def _grid_side_for(count: int) -> int:
    side = max(1, int(round(count ** (1.0 / 3.0))))
    while side**3 < count:
        side += 1
    while side > 1 and (side - 1) ** 3 >= count:
        side -= 1
    return side


def reconstruct_canonical_cloud(
    family: str, point_count: int, seed: int
) -> tuple[np.ndarray, np.ndarray]:
    if family not in FAMILIES:
        raise ScientificGuardError(f"unsupported family {family!r}")
    if point_count < 1 or seed < 0 or seed > UINT64_MASK:
        raise ScientificGuardError("the cloud parameters are outside their domains")
    bits = np.empty((point_count, 3), dtype=np.uint64)
    one_exponent = 0x3FF0000000000000
    if family == "affine_uniform_binary64":
        mantissa_mask = (1 << 52) - 1
        step = max(1, mantissa_mask // point_count)
        rotated = _rotl64(seed, 23)
        for logical in range(point_count):
            bits[logical, 0] = one_exponent | (logical * step)
            bits[logical, 1] = one_exponent | (
                splitmix64(logical ^ seed) & mantissa_mask
            )
            bits[logical, 2] = one_exponent | (
                splitmix64(logical ^ rotated) & mantissa_mask
            )
    elif family == "jittered_dyadic_grid3d":
        denominator = 1 << 30
        side = _grid_side_for(point_count)
        stride = (denominator - 16) // (side + 1)
        coordinates = np.empty((point_count, 3), dtype=np.float64)
        for logical in range(point_count):
            cell = (logical % side, (logical // side) % side, logical // (side * side))
            for axis in range(3):
                random = splitmix64(
                    logical
                    ^ seed
                    ^ ((0x9E3779B97F4A7C15 * (axis + 1)) & UINT64_MASK)
                )
                jitter = random % 7 - 3
                numerator = (cell[axis] + 1) * stride + jitter
                coordinates[logical, axis] = 1.0 + math.ldexp(float(numerator), -30)
        bits = coordinates.view(np.uint64).copy()
    else:
        center_unit = 1 << 48
        for logical in range(point_count):
            cluster = logical % 64
            local = logical // 64
            scale = local % 8
            grid = (cluster % 4, (cluster // 4) % 4, cluster // 16)
            for axis in range(3):
                radius = 1 << (scale + 38)
                random = splitmix64(
                    logical
                    ^ seed
                    ^ ((0xD6E8FEB86659FD93 * (axis + 1)) & UINT64_MASK)
                )
                bucket = random % (radius * 2 + 1) - radius
                center = (grid[axis] * 2 + 1) * center_unit
                mantissa = center + bucket
                if not 0 < mantissa < (1 << 52):
                    raise ScientificGuardError("multiscale mantissa overflow")
                bits[logical, axis] = one_exponent | mantissa

    permutation = np.lexsort((bits[:, 2], bits[:, 1], bits[:, 0]))
    canonical_bits = np.ascontiguousarray(bits[permutation])
    if len(np.unique(canonical_bits, axis=0)) != point_count:
        raise ScientificGuardError("the reconstructed cloud contains duplicates")
    points = canonical_bits.view(np.float64).reshape(point_count, 3).copy()
    if np.any(~np.isfinite(points)) or np.any(points <= 0.0):
        raise ScientificGuardError("the reconstructed cloud is not positive finite")
    return points, canonical_bits


def _load_spatial() -> tuple[object, object, type[BaseException]]:
    try:
        from scipy.spatial import Delaunay, QhullError, cKDTree
    except ImportError as error:
        raise ScientificGuardError("SciPy is required for the independent oracle") from error
    return Delaunay, cKDTree, QhullError


def _validated_delaunay_arrays(
    triangulation: object, point_count: int
) -> tuple[np.ndarray, np.ndarray]:
    simplices = np.asarray(triangulation.simplices, dtype=np.int64)
    if simplices.ndim != 2 or simplices.shape[1] != 4 or len(simplices) == 0:
        raise ScientificGuardError("SciPy did not return finite 3-D tetrahedra")
    if np.any(simplices < 0) or np.any(simplices >= point_count):
        raise ScientificGuardError("SciPy returned an out-of-range tetrahedron")
    if len(np.unique(simplices)) != point_count:
        raise ScientificGuardError("the Delaunay triangulation omitted an input point")
    pairs = ((0, 1), (0, 2), (0, 3), (1, 2), (1, 3), (2, 3))
    occurrences = np.concatenate(
        [np.sort(simplices[:, pair], axis=1) for pair in pairs], axis=0
    )
    edges = np.unique(occurrences, axis=0).astype(np.uint64, copy=False)
    if len(edges) < point_count - 1:
        raise ScientificGuardError("the ordinary Delaunay graph cannot span")
    return simplices, np.ascontiguousarray(edges)


def _failure_head(error: BaseException) -> str:
    return str(error).splitlines()[0]


def build_delaunay_oracle(points: np.ndarray) -> DelaunayOracle:
    Delaunay, _, QhullError = _load_spatial()
    primary_failure = None
    qhull_options = "SciPy_default_Qbb_Qc_Qz_Q12_Qt"
    exact_translation_used = False
    try:
        triangulation = Delaunay(points)
        simplices, edges = _validated_delaunay_arrays(triangulation, len(points))
    except (QhullError, ScientificGuardError) as primary_error:
        primary_failure = _failure_head(primary_error)
        if np.any(points < 1.0) or np.any(points >= 2.0):
            raise ScientificGuardError(
                "SciPy/Qhull Delaunay failed on the original input and the exact "
                "subtract-one fallback is only certified for coordinates in [1, 2): "
                f"default={primary_failure!r}"
            ) from primary_error
        exact_translation_used = True
        try:
            # Sterbenz's lemma makes this subtraction exact for points in [1, 2).
            # Keep SciPy's defaults unchanged: no QJ option or other perturbation
            # is permitted in this fail-closed scientific oracle.
            triangulation = Delaunay(points - 1.0)
            simplices, edges = _validated_delaunay_arrays(triangulation, len(points))
        except (QhullError, ScientificGuardError) as fallback_error:
            raise ScientificGuardError(
                "SciPy/Qhull Delaunay failed on both the original input and its "
                f"exact subtract-one translation: default={primary_failure!r}; "
                f"fallback={_failure_head(fallback_error)!r}"
            ) from fallback_error
    return DelaunayOracle(
        len(simplices),
        edges,
        qhull_options,
        exact_translation_used,
        False,
        primary_failure,
    )


def squared_distances(
    points: np.ndarray, first: np.ndarray, second: np.ndarray
) -> np.ndarray:
    dx = points[first, 0] - points[second, 0]
    dy = points[first, 1] - points[second, 1]
    dz = points[first, 2] - points[second, 2]
    xx = dx * dx
    yy = dy * dy
    zz = dz * dz
    return (xx + yy) + zz


def canonical_kruskal(
    point_count: int,
    edges: np.ndarray,
    weights: np.ndarray,
) -> ExportedTree:
    if edges.shape != (len(weights), 2):
        raise ScientificGuardError("the weighted edge arrays disagree")
    ordering = np.lexsort((edges[:, 1], edges[:, 0], weights))
    components = _DisjointSet(point_count)
    selected: list[int] = []
    for position in ordering:
        left, right = edges[position]
        if components.union(int(left), int(right)):
            selected.append(int(position))
            if len(selected) == point_count - 1:
                break
    if len(selected) != point_count - 1:
        raise ScientificGuardError("canonical Kruskal did not close")
    chosen = np.asarray(selected, dtype=np.int64)
    selected_weights = np.asarray(weights[chosen], dtype=np.float64).copy()
    return ExportedTree(
        0,
        np.asarray(edges[chosen, 0], dtype=np.uint64).copy(),
        np.asarray(edges[chosen, 1], dtype=np.uint64).copy(),
        selected_weights,
        selected_weights.view(np.uint64).copy(),
    )


class WeightedTreePaths:
    def __init__(self, point_count: int, tree: ExportedTree) -> None:
        sources = np.concatenate((tree.first, tree.second)).astype(np.int64)
        targets = np.concatenate((tree.second, tree.first)).astype(np.int64)
        weights = np.concatenate((tree.squared_levels, tree.squared_levels))
        order = np.lexsort((targets, sources))
        sources = sources[order]
        targets = targets[order]
        weights = weights[order]
        degrees = np.bincount(sources, minlength=point_count)
        offsets = np.empty(point_count + 1, dtype=np.int64)
        offsets[0] = 0
        np.cumsum(degrees, out=offsets[1:])
        parent = np.full(point_count, -1, dtype=np.int64)
        parent_weight = np.zeros(point_count, dtype=np.float64)
        depth = np.zeros(point_count, dtype=np.int64)
        parent[0] = 0
        stack = [0]
        visited = 0
        while stack:
            node = stack.pop()
            visited += 1
            for position in range(int(offsets[node]), int(offsets[node + 1])):
                neighbor = int(targets[position])
                if neighbor == int(parent[node]):
                    continue
                if parent[neighbor] != -1:
                    raise ScientificGuardError("path index received a cyclic tree")
                parent[neighbor] = node
                parent_weight[neighbor] = weights[position]
                depth[neighbor] = depth[node] + 1
                stack.append(neighbor)
        if visited != point_count:
            raise ScientificGuardError("path index received a disconnected tree")
        level_count = max(1, point_count.bit_length())
        ancestors = np.empty((level_count, point_count), dtype=np.int64)
        maxima = np.empty((level_count, point_count), dtype=np.float64)
        ancestors[0] = parent
        maxima[0] = parent_weight
        for level in range(1, level_count):
            middle = ancestors[level - 1]
            ancestors[level] = ancestors[level - 1, middle]
            maxima[level] = np.maximum(maxima[level - 1], maxima[level - 1, middle])
        self.depth = depth
        self.ancestors = ancestors
        self.maxima = maxima

    def maximum_on_paths(self, first: np.ndarray, second: np.ndarray) -> np.ndarray:
        left = np.asarray(first, dtype=np.int64).copy()
        right = np.asarray(second, dtype=np.int64).copy()
        if left.shape != right.shape:
            raise ScientificGuardError("path query extents disagree")
        maximum = np.zeros(left.shape, dtype=np.float64)
        swap = self.depth[left] < self.depth[right]
        temporary = left[swap].copy()
        left[swap] = right[swap]
        right[swap] = temporary
        difference = self.depth[left] - self.depth[right]
        for level in range(len(self.ancestors)):
            mask = (difference & (1 << level)) != 0
            if np.any(mask):
                maximum[mask] = np.maximum(
                    maximum[mask], self.maxima[level, left[mask]]
                )
                left[mask] = self.ancestors[level, left[mask]]
        for level in range(len(self.ancestors) - 1, -1, -1):
            left_parent = self.ancestors[level, left]
            right_parent = self.ancestors[level, right]
            mask = left_parent != right_parent
            if np.any(mask):
                maximum[mask] = np.maximum(
                    maximum[mask], self.maxima[level, left[mask]]
                )
                maximum[mask] = np.maximum(
                    maximum[mask], self.maxima[level, right[mask]]
                )
                left[mask] = left_parent[mask]
                right[mask] = right_parent[mask]
        distinct = left != right
        if np.any(distinct):
            maximum[distinct] = np.maximum(
                maximum[distinct], self.maxima[0, left[distinct]]
            )
            maximum[distinct] = np.maximum(
                maximum[distinct], self.maxima[0, right[distinct]]
            )
        return maximum


def _pair_keys(first: np.ndarray, second: np.ndarray) -> np.ndarray:
    return (first.astype(np.uint64) << np.uint64(32)) | second.astype(np.uint64)


def evaluate_k1(
    points: np.ndarray,
    oracle: DelaunayOracle,
    candidate: ExportedTree,
) -> dict[str, object]:
    candidate_geometry = squared_distances(
        points, candidate.first.astype(np.int64), candidate.second.astype(np.int64)
    )
    level_identity = candidate_geometry.view(np.uint64) == candidate.squared_level_bits
    first_level_mismatch = None
    if not np.all(level_identity):
        index = int(np.flatnonzero(~level_identity)[0])
        first_level_mismatch = {
            "u": int(candidate.first[index]),
            "v": int(candidate.second[index]),
            "serialized_squared_level": float(candidate.squared_levels[index]),
            "recomputed_squared_distance": float(candidate_geometry[index]),
        }
    oracle_weights = squared_distances(
        points, oracle.edges[:, 0].astype(np.int64), oracle.edges[:, 1].astype(np.int64)
    )
    emst = canonical_kruskal(len(points), oracle.edges, oracle_weights)
    candidate_keys = np.sort(_pair_keys(candidate.first, candidate.second))
    oracle_keys = np.sort(_pair_keys(emst.first, emst.second))
    identity_match = bool(np.array_equal(candidate_keys, oracle_keys))
    overlap = int(np.intersect1d(candidate_keys, oracle_keys, assume_unique=True).size)
    paths = WeightedTreePaths(len(points), candidate)
    candidate_deadlines = paths.maximum_on_paths(emst.first, emst.second)
    before = candidate_deadlines < emst.squared_levels
    at = candidate_deadlines == emst.squared_levels
    late = candidate_deadlines > emst.squared_levels
    deadline_ratios = candidate_deadlines / emst.squared_levels
    lateness_ulps = np.zeros(len(emst.first), dtype=np.uint64)
    candidate_bits = candidate_deadlines.view(np.uint64)
    oracle_bits = emst.squared_levels.view(np.uint64)
    lateness_ulps[late] = candidate_bits[late] - oracle_bits[late]
    first_late = None
    if np.any(late):
        index = int(np.flatnonzero(late)[0])
        first_late = {
            "u": int(emst.first[index]),
            "v": int(emst.second[index]),
            "oracle_squared_level": float(emst.squared_levels[index]),
            "candidate_connection_squared_level": float(candidate_deadlines[index]),
        }
    # Edge identity is diagnostic only: on a tied Delaunay plateau, a different
    # spanning witness is equivalent when every canonical EMST edge reaches
    # the same closed connectivity deadline.
    passed = bool(np.all(level_identity) and not np.any(late))
    return {
        "oracle": "canonical_Kruskal_on_SciPy_ordinary_Delaunay_edges",
        "level_convention": "binary64_squared_distance_recipe",
        "boundary": "closed_post_plateau",
        "oracle_selected_edge_count": len(emst.first),
        "candidate_selected_edge_count": len(candidate.first),
        "canonical_edge_identity_match": identity_match,
        "canonical_edge_identity_required": False,
        "canonical_edge_overlap_count": overlap,
        "serialized_levels_match_recomputed_distances": bool(np.all(level_identity)),
        "first_level_mismatch": first_level_mismatch,
        "connected_before_count": int(np.count_nonzero(before)),
        "connected_at_count": int(np.count_nonzero(at)),
        "late_count": int(np.count_nonzero(late)),
        "never_connected_count": 0,
        "maximum_connection_to_deadline_ratio": float(deadline_ratios.max()),
        "maximum_squared_level_minus_oracle": float(
            np.max(candidate_deadlines - emst.squared_levels)
        ),
        "maximum_lateness_ulps": int(lateness_ulps.max(initial=0)),
        "first_late_witness": first_late,
        "guard_passed": passed,
    }


def _build_csr(point_count: int, edges: np.ndarray) -> _CsrGraph:
    sources = np.concatenate((edges[:, 0], edges[:, 1])).astype(np.int64)
    targets = np.concatenate((edges[:, 1], edges[:, 0])).astype(np.int64)
    order = np.lexsort((targets, sources))
    sources = sources[order]
    targets = targets[order]
    degrees = np.bincount(sources, minlength=point_count)
    offsets = np.empty(point_count + 1, dtype=np.int64)
    offsets[0] = 0
    np.cumsum(degrees, out=offsets[1:])
    edge_keys = np.sort(_pair_keys(edges[:, 0], edges[:, 1]))
    raw_wedges = int(np.sum(degrees * (degrees - 1) // 2, dtype=np.int64))
    return _CsrGraph(
        offsets,
        targets,
        edge_keys,
        raw_wedges,
        int(degrees.max(initial=0)),
    )


def iter_owned_two_edge_triangles(
    point_count: int,
    edges: np.ndarray,
    *,
    batch_size: int,
) -> tuple[_CsrGraph, Iterator[np.ndarray]]:
    if batch_size <= 0:
        raise ScientificGuardError("triangle batch size must be positive")
    graph = _build_csr(point_count, edges)

    def iterator() -> Iterator[np.ndarray]:
        pending: list[np.ndarray] = []
        pending_count = 0
        for center in range(point_count):
            begin = int(graph.offsets[center])
            end = int(graph.offsets[center + 1])
            neighbors = graph.neighbors[begin:end]
            degree = len(neighbors)
            if degree < 2:
                continue
            left_positions, right_positions = np.triu_indices(degree, 1)
            first = neighbors[left_positions]
            second = neighbors[right_positions]
            lo = np.minimum(first, second).astype(np.uint64)
            hi = np.maximum(first, second).astype(np.uint64)
            keys = (lo << np.uint64(32)) | hi
            locations = np.searchsorted(graph.edge_keys, keys)
            bounded = locations < len(graph.edge_keys)
            third_edge = np.zeros(len(keys), dtype=bool)
            third_edge[bounded] = graph.edge_keys[locations[bounded]] == keys[bounded]
            owned = (~third_edge) | ((center < first) & (center < second))
            if not np.any(owned):
                continue
            centers = np.full(int(np.count_nonzero(owned)), center, dtype=np.int64)
            triples = np.column_stack((centers, first[owned], second[owned]))
            triples.sort(axis=1)
            pending.append(triples.astype(np.int64, copy=False))
            pending_count += len(triples)
            if pending_count >= batch_size:
                combined = np.concatenate(pending, axis=0)
                start = 0
                while len(combined) - start >= batch_size:
                    yield np.ascontiguousarray(combined[start : start + batch_size])
                    start += batch_size
                pending = [np.ascontiguousarray(combined[start:])] if start < len(combined) else []
                pending_count = len(combined) - start
        if pending_count:
            yield np.ascontiguousarray(np.concatenate(pending, axis=0))

    return graph, iterator()


def triangle_miniballs(points: np.ndarray, triangles: np.ndarray) -> _Miniballs:
    a = points[triangles[:, 0]]
    b = points[triangles[:, 1]]
    c = points[triangles[:, 2]]
    u = b - a
    v = c - a
    ab2 = ((u[:, 0] * u[:, 0]) + (u[:, 1] * u[:, 1])) + u[:, 2] * u[:, 2]
    ac2 = ((v[:, 0] * v[:, 0]) + (v[:, 1] * v[:, 1])) + v[:, 2] * v[:, 2]
    dot = ((u[:, 0] * v[:, 0]) + (u[:, 1] * v[:, 1])) + u[:, 2] * v[:, 2]
    bc2 = (ab2 + ac2) - (2.0 * dot)
    finite_sides = (
        (ab2 > 0.0)
        & (ac2 > 0.0)
        & (bc2 > 0.0)
        & np.isfinite(ab2)
        & np.isfinite(ac2)
        & np.isfinite(bc2)
    )
    scale = (ab2 + ac2) + bc2
    finite_scale = np.isfinite(scale)
    tolerance = (
        PREDICATE_TOLERANCE_MULTIPLIER
        * np.finfo(np.float64).eps
        * np.maximum(scale, np.finfo(np.float64).tiny)
    )
    near_right = (
        (np.abs(bc2 - (ab2 + ac2)) <= tolerance)
        | (np.abs(ac2 - (ab2 + bc2)) <= tolerance)
        | (np.abs(ab2 - (ac2 + bc2)) <= tolerance)
    )
    centers = np.zeros((len(triangles), 3), dtype=np.float64)
    radii = np.zeros(len(triangles), dtype=np.float64)
    support = np.zeros(len(triangles), dtype=np.uint8)
    valid = finite_sides & finite_scale
    ambiguous = np.zeros(len(triangles), dtype=bool)

    bc_long = valid & (bc2 >= ab2 + ac2)
    ac_long = valid & ~bc_long & (ac2 >= ab2 + bc2)
    ab_long = valid & ~bc_long & ~ac_long & (ab2 >= ac2 + bc2)
    diameter = bc_long | ac_long | ab_long
    centers[bc_long] = b[bc_long] + (c[bc_long] - b[bc_long]) * 0.5
    centers[ac_long] = a[ac_long] + (c[ac_long] - a[ac_long]) * 0.5
    centers[ab_long] = a[ab_long] + (b[ab_long] - a[ab_long]) * 0.5
    radii[bc_long] = 0.25 * bc2[bc_long]
    radii[ac_long] = 0.25 * ac2[ac_long]
    radii[ab_long] = 0.25 * ab2[ab_long]
    support[diameter] = 2
    ambiguous[diameter] = near_right[diameter]

    acute = valid & ~diameter
    product = ab2 * ac2
    dot_product = dot * dot
    determinant = product - dot_product
    determinant_scale = np.maximum(product, dot_product)
    determinant_tolerance = (
        PREDICATE_TOLERANCE_MULTIPLIER
        * np.finfo(np.float64).eps
        * np.maximum(determinant_scale, np.finfo(np.float64).tiny)
    )
    determinant_invalid = acute & (
        ~np.isfinite(determinant) | ~np.isfinite(determinant_scale)
    )
    valid[determinant_invalid] = False
    fallback = acute & ~determinant_invalid & ~(
        determinant > determinant_tolerance
    )
    if np.any(fallback):
        longest = ab2.copy()
        left = a.copy()
        right = b.copy()
        choose_ac = fallback & (ac2 > longest)
        longest[choose_ac] = ac2[choose_ac]
        right[choose_ac] = c[choose_ac]
        choose_bc = fallback & (bc2 > longest)
        longest[choose_bc] = bc2[choose_bc]
        left[choose_bc] = b[choose_bc]
        right[choose_bc] = c[choose_bc]
        centers[fallback] = left[fallback] + (right[fallback] - left[fallback]) * 0.5
        radii[fallback] = 0.25 * longest[fallback]
        support[fallback] = 2
        ambiguous[fallback] = True

    circum = acute & ~determinant_invalid & ~fallback
    if np.any(circum):
        denominator = 2.0 * determinant[circum]
        alpha = ac2[circum] * (ab2[circum] - dot[circum]) / denominator
        beta = ab2[circum] * (ac2[circum] - dot[circum]) / denominator
        centers[circum] = (
            a[circum] + alpha[:, None] * u[circum]
        ) + beta[:, None] * v[circum]
        delta = centers[circum] - a[circum]
        radii[circum] = (
            (delta[:, 0] * delta[:, 0]) + (delta[:, 1] * delta[:, 1])
        ) + delta[:, 2] * delta[:, 2]
        support[circum] = 3
        ambiguous[circum] = near_right[circum]

    valid &= (
        np.all(np.isfinite(centers), axis=1)
        & np.isfinite(radii)
        & (radii > 0.0)
    )
    support[~valid] = 0
    ambiguous[~valid] = False
    return _Miniballs(centers, radii, support, valid, ambiguous)


def exact_triangle_miniball(
    points: np.ndarray, triangle: Sequence[int]
) -> _ExactMiniball:
    sites = tuple(
        tuple(Fraction.from_float(float(coordinate)) for coordinate in points[index])
        for index in triangle
    )
    a, b, c = sites

    def subtract(
        left: Sequence[Fraction], right: Sequence[Fraction]
    ) -> tuple[Fraction, Fraction, Fraction]:
        return tuple(x - y for x, y in zip(left, right))  # type: ignore[return-value]

    def dot(left: Sequence[Fraction], right: Sequence[Fraction]) -> Fraction:
        return sum((x * y for x, y in zip(left, right)), Fraction(0))

    u = subtract(b, a)
    v = subtract(c, a)
    w = subtract(c, b)
    ab2 = dot(u, u)
    ac2 = dot(v, v)
    bc2 = dot(w, w)
    if min(ab2, ac2, bc2) <= 0:
        raise ScientificGuardError(
            "exact miniball recertification received duplicate triangle vertices"
        )
    if bc2 >= ab2 + ac2:
        return _ExactMiniball(bc2 / 4, 2)
    if ac2 >= ab2 + bc2:
        return _ExactMiniball(ac2 / 4, 2)
    if ab2 >= ac2 + bc2:
        return _ExactMiniball(ab2 / 4, 2)
    uv = dot(u, v)
    determinant = ab2 * ac2 - uv * uv
    if determinant <= 0:
        raise ScientificGuardError(
            "an exact acute triangle has a non-positive Gram determinant"
        )
    return _ExactMiniball(ab2 * ac2 * bc2 / (4 * determinant), 3)


def classify_gabriel_binary64(
    points: np.ndarray,
    triangles: np.ndarray,
    balls: _Miniballs,
    tree: object,
    *,
    workers: int,
) -> np.ndarray:
    # 0 blocked, 1 gabriel_binary64, 2 ambiguous, 3 invalid.
    statuses = np.full(len(triangles), 3, dtype=np.uint8)
    valid_indices = np.flatnonzero(balls.valid)
    if len(valid_indices) == 0:
        return statuses
    valid_triangles = triangles[valid_indices]
    centers = balls.centers[valid_indices]
    neighbor_count = min(4, len(points))
    _, ids = tree.query(centers, k=neighbor_count, eps=0.0, p=2, workers=workers)
    ids = np.asarray(ids, dtype=np.int64)
    if ids.ndim == 1:
        ids = ids[:, None]
    own = (
        (ids == valid_triangles[:, 0, None])
        | (ids == valid_triangles[:, 1, None])
        | (ids == valid_triangles[:, 2, None])
    )
    external = (~own) & (ids >= 0) & (ids < len(points))
    if np.any(~np.any(external, axis=1)):
        raise ScientificGuardError("cKDTree did not return a non-source neighbor")
    external_ids = np.where(external, ids, 0)
    external_points = points[external_ids]
    dx = external_points[:, :, 0] - centers[:, None, 0]
    dy = external_points[:, :, 1] - centers[:, None, 1]
    dz = external_points[:, :, 2] - centers[:, None, 2]
    distances = ((dx * dx) + (dy * dy)) + dz * dz
    distances[~external] = np.inf
    nearest_distance = distances.min(axis=1)
    radii = balls.squared_radii[valid_indices]
    center_scale = np.maximum.reduce(
        (
            centers[:, 0] * centers[:, 0],
            centers[:, 1] * centers[:, 1],
            centers[:, 2] * centers[:, 2],
        )
    )
    tolerance = (
        PREDICATE_TOLERANCE_MULTIPLIER
        * np.finfo(np.float64).eps
        * (np.abs(nearest_distance) + np.abs(radii) + center_scale + 1.0)
    )
    blocked = nearest_distance < radii - tolerance
    boundary = np.abs(nearest_distance - radii) <= tolerance
    local = np.ones(len(valid_indices), dtype=np.uint8)
    local[blocked] = 0
    local[balls.ambiguous[valid_indices] | (~blocked & boundary)] = 2
    statuses[valid_indices] = local
    return statuses


def _squared_distance_lower_bounds(
    points: np.ndarray,
    first: np.ndarray,
    second: np.ndarray,
) -> np.ndarray:
    """Return outward-rounded lower bounds for exact binary64 distances.

    Each input coordinate denotes an exact dyadic rational.  One predecessor
    and successor enclose the exact result of every correctly-rounded
    subtraction.  Squaring the minimum absolute endpoint and rounding every
    multiplication/addition down therefore produces a lower bound, including
    when Sterbenz' lemma does not apply.  A non-finite intermediate is mapped
    to zero so that the adaptive filter falls back to exact arithmetic.
    """

    lower = np.zeros(len(first), dtype=np.float64)
    with np.errstate(over="ignore", invalid="ignore", under="ignore"):
        for axis in range(3):
            difference = points[first, axis] - points[second, axis]
            difference_lower = np.nextafter(difference, -np.inf)
            difference_upper = np.nextafter(difference, np.inf)
            crosses_zero = (difference_lower <= 0.0) & (difference_upper >= 0.0)
            minimum_absolute = np.minimum(
                np.abs(difference_lower), np.abs(difference_upper)
            )
            minimum_absolute[crosses_zero] = 0.0
            squared = minimum_absolute * minimum_absolute
            squared_lower = np.nextafter(squared, -np.inf)
            squared_lower = np.maximum(squared_lower, 0.0)
            summed = lower + squared_lower
            lower = np.nextafter(summed, -np.inf)
            lower = np.maximum(lower, 0.0)
    lower[~np.isfinite(lower)] = 0.0
    return lower


def _triangle_longest_edge_squared_lower_bounds(
    points: np.ndarray, triangles: np.ndarray
) -> np.ndarray:
    """Return a lower bound for the exact longest squared triangle edge."""

    ab = _squared_distance_lower_bounds(
        points, triangles[:, 0], triangles[:, 1]
    )
    ac = _squared_distance_lower_bounds(
        points, triangles[:, 0], triangles[:, 2]
    )
    bc = _squared_distance_lower_bounds(
        points, triangles[:, 1], triangles[:, 2]
    )
    return np.maximum(np.maximum(ab, ac), bc)


def _deadline_partition(
    triangles: np.ndarray,
    squared_radii: np.ndarray,
    paths: WeightedTreePaths,
    support_cardinalities: np.ndarray | None = None,
    *,
    points: np.ndarray | None = None,
) -> tuple[dict[str, object], np.ndarray]:
    if points is not None:
        return _adaptive_deadline_partition(
            points,
            triangles,
            squared_radii,
            paths,
            support_cardinalities,
        )
    ab = paths.maximum_on_paths(triangles[:, 0], triangles[:, 1])
    ac = paths.maximum_on_paths(triangles[:, 0], triangles[:, 2])
    bc = paths.maximum_on_paths(triangles[:, 1], triangles[:, 2])
    raw_connection = np.maximum(np.maximum(ab, ac), bc)
    raw_deadline = np.ldexp(squared_radii, 2)
    converted_connection = np.ldexp(raw_connection, -2)
    ratios = converted_connection / squared_radii
    before = raw_connection < raw_deadline
    at = raw_connection == raw_deadline
    late = raw_connection > raw_deadline
    raw_excess = raw_connection - raw_deadline
    connection_bits = converted_connection.view(np.uint64)
    radius_bits = squared_radii.view(np.uint64)
    lateness_ulps = np.zeros(len(triangles), dtype=np.uint64)
    lateness_ulps[late] = connection_bits[late] - radius_bits[late]
    late_support_two = 0
    late_support_three = 0
    if support_cardinalities is not None:
        late_support_two = int(np.count_nonzero(late & (support_cardinalities == 2)))
        late_support_three = int(np.count_nonzero(late & (support_cardinalities == 3)))

    def witness(index: int) -> dict[str, object]:
        result: dict[str, object] = {
            "point_ids": [int(value) for value in triangles[index]],
            "source_squared_radius": float(squared_radii[index]),
            "source_squared_radius_bits": f"0x{int(radius_bits[index]):016x}",
            "raw_connection_squared_weight": float(raw_connection[index]),
            "raw_squared_weight_minus_four_radius": float(raw_excess[index]),
            "converted_connection_squared_level": float(converted_connection[index]),
            "converted_connection_squared_level_bits": (
                f"0x{int(connection_bits[index]):016x}"
            ),
            "lateness_ulps": int(lateness_ulps[index]),
        }
        if support_cardinalities is not None:
            result["support_cardinality"] = int(support_cardinalities[index])
        return result

    first_late = None
    maximum_late = None
    if np.any(late):
        index = int(np.flatnonzero(late)[0])
        first_late = witness(index)
        maximum_index = int(np.argmax(lateness_ulps))
        maximum_late = witness(maximum_index)
    return (
        {
            "source_count": len(triangles),
            "connected_before_count": int(np.count_nonzero(before)),
            "connected_at_count": int(np.count_nonzero(at)),
            "late_count": int(np.count_nonzero(late)),
            "never_connected_count": 0,
            "maximum_source_squared_radius": float(squared_radii.max()),
            "maximum_raw_connection_squared_weight": float(raw_connection.max()),
            "maximum_converted_connection_squared_level": float(
                converted_connection.max()
            ),
            "maximum_converted_level_minus_source_level": float(
                np.max(converted_connection - squared_radii)
            ),
            "maximum_raw_squared_weight_minus_four_radius": float(
                raw_excess.max()
            ),
            "maximum_lateness_ulps": int(lateness_ulps.max(initial=0)),
            "maximum_connection_to_deadline_ratio": float(ratios.max()),
            "late_support_two_count": late_support_two,
            "late_support_three_count": late_support_three,
            "late_one_ulp_count": int(np.count_nonzero(lateness_ulps == 1)),
            "late_two_ulp_count": int(np.count_nonzero(lateness_ulps == 2)),
            "late_more_than_two_ulp_count": int(
                np.count_nonzero(lateness_ulps > 2)
            ),
            "first_late_witness": first_late,
            "maximum_late_witness": maximum_late,
            "passed": not bool(np.any(late)),
        },
        late,
    )


def _adaptive_deadline_partition(
    points: np.ndarray,
    triangles: np.ndarray,
    squared_radii: np.ndarray,
    paths: WeightedTreePaths,
    support_cardinalities: np.ndarray | None = None,
) -> tuple[dict[str, object], np.ndarray]:
    """Decide deadlines without accepting against an over-rounded radius.

    The squared radius of every ball containing a triangle is at least one
    quarter of its longest squared edge.  A strictly smaller connection level
    can consequently be accepted from the outward-rounded lower bound alone.
    Every other source, including every apparent equality, is compared as an
    exact dyadic ``Fraction(raw_connection) / 4`` against the exact triangle
    miniball.  The filter is unilateral: uncertainty can only trigger more
    exact work, never a positive decision.
    """

    ab = paths.maximum_on_paths(triangles[:, 0], triangles[:, 1])
    ac = paths.maximum_on_paths(triangles[:, 0], triangles[:, 2])
    bc = paths.maximum_on_paths(triangles[:, 1], triangles[:, 2])
    raw_connection = np.maximum(np.maximum(ab, ac), bc)
    converted_connection = np.ldexp(raw_connection, -2)
    longest_edge_lower_bounds = _triangle_longest_edge_squared_lower_bounds(
        points, triangles
    )
    # Compare the serialized raw weight directly: raw / 4 is strictly below
    # max(edge^2) / 4 without introducing another rounded operation.
    certified_before = raw_connection < longest_edge_lower_bounds
    exact_indices = np.flatnonzero(~certified_before)

    decisions = np.full(len(triangles), -1, dtype=np.int8)
    exact_balls: dict[int, _ExactMiniball] = {}
    exact_converted: dict[int, Fraction] = {}
    exact_excess: dict[int, Fraction] = {}
    exact_ratios: dict[int, Fraction] = {}
    for raw_index in exact_indices:
        index = int(raw_index)
        ball = exact_triangle_miniball(points, triangles[index])
        converted = Fraction.from_float(float(raw_connection[index])) / 4
        excess = converted - ball.squared_radius
        exact_balls[index] = ball
        exact_converted[index] = converted
        exact_excess[index] = excess
        exact_ratios[index] = converted / ball.squared_radius
        decisions[index] = -1 if excess < 0 else 0 if excess == 0 else 1

    before = decisions < 0
    at = decisions == 0
    late = decisions > 0
    raw_deadline = np.ldexp(squared_radii, 2)
    raw_excess = raw_connection - raw_deadline
    connection_bits = converted_connection.view(np.uint64)
    radius_bits = squared_radii.view(np.uint64)
    lateness_ulps = np.zeros(len(triangles), dtype=np.uint64)
    binary64_late = connection_bits > radius_bits
    lateness_ulps[binary64_late] = (
        connection_bits[binary64_late] - radius_bits[binary64_late]
    )

    exact_support = np.asarray(support_cardinalities, dtype=np.uint8).copy() if (
        support_cardinalities is not None
    ) else np.zeros(len(triangles), dtype=np.uint8)
    for index, ball in exact_balls.items():
        exact_support[index] = ball.support_cardinality

    def witness(index: int) -> dict[str, object]:
        ball = exact_balls[index]
        converted = exact_converted[index]
        excess = exact_excess[index]
        result: dict[str, object] = {
            "point_ids": [int(value) for value in triangles[index]],
            "source_squared_radius_exact": _fraction_text(ball.squared_radius),
            "source_squared_radius": float(ball.squared_radius),
            "source_squared_radius_binary64_recipe": float(squared_radii[index]),
            "source_squared_radius_binary64_recipe_bits": (
                f"0x{int(radius_bits[index]):016x}"
            ),
            "raw_connection_squared_weight": float(raw_connection[index]),
            "raw_squared_weight_minus_four_radius_exact": _fraction_text(4 * excess),
            "converted_connection_squared_level_exact": _fraction_text(converted),
            "converted_connection_squared_level": float(converted),
            "converted_connection_squared_level_bits": (
                f"0x{int(connection_bits[index]):016x}"
            ),
            "lateness_ulps": int(lateness_ulps[index]),
            "support_cardinality": ball.support_cardinality,
            "decision_certification": "exact_dyadic_Fraction_triangle_miniball",
        }
        return result

    first_late = witness(int(np.flatnonzero(late)[0])) if np.any(late) else None
    maximum_late = None
    if np.any(late):
        maximum_index = max(
            (int(index) for index in np.flatnonzero(late)),
            key=lambda index: exact_excess[index],
        )
        maximum_late = witness(maximum_index)

    safe_upper_ratios = np.asarray([], dtype=np.float64)
    if np.any(certified_before):
        with np.errstate(divide="ignore", invalid="ignore"):
            safe_upper_ratios = np.nextafter(
                raw_connection[certified_before]
                / longest_edge_lower_bounds[certified_before],
                np.inf,
            )
    exact_ratio_values = [float(value) for value in exact_ratios.values()]
    ratio_upper_bound = max(
        float(safe_upper_ratios.max(initial=-math.inf)),
        max(exact_ratio_values, default=-math.inf),
    )
    exact_source_values = [float(ball.squared_radius) for ball in exact_balls.values()]
    maximum_source = max(
        float(squared_radii[certified_before].max(initial=-math.inf)),
        max(exact_source_values, default=-math.inf),
    )
    exact_difference_values = [float(value) for value in exact_excess.values()]
    maximum_difference = max(
        float((converted_connection - squared_radii)[certified_before].max(initial=-math.inf)),
        max(exact_difference_values, default=-math.inf),
    )
    exact_raw_excess_values = [float(4 * value) for value in exact_excess.values()]
    maximum_raw_excess = max(
        float(raw_excess[certified_before].max(initial=-math.inf)),
        max(exact_raw_excess_values, default=-math.inf),
    )
    late_support_two = int(np.count_nonzero(late & (exact_support == 2)))
    late_support_three = int(np.count_nonzero(late & (exact_support == 3)))
    return (
        {
            "source_count": len(triangles),
            "connected_before_count": int(np.count_nonzero(before)),
            "connected_at_count": int(np.count_nonzero(at)),
            "late_count": int(np.count_nonzero(late)),
            "never_connected_count": 0,
            "maximum_source_squared_radius": maximum_source,
            "maximum_raw_connection_squared_weight": float(raw_connection.max()),
            "maximum_converted_connection_squared_level": float(
                converted_connection.max()
            ),
            "maximum_converted_level_minus_source_level": maximum_difference,
            "maximum_raw_squared_weight_minus_four_radius": maximum_raw_excess,
            "maximum_lateness_ulps": int(lateness_ulps[late].max(initial=0)),
            "maximum_connection_to_deadline_ratio": ratio_upper_bound,
            "maximum_connection_to_deadline_ratio_is_upper_bound": True,
            "late_support_two_count": late_support_two,
            "late_support_three_count": late_support_three,
            "late_with_zero_binary64_recipe_ulp_count": int(
                np.count_nonzero(late & (lateness_ulps == 0))
            ),
            "late_one_ulp_count": int(np.count_nonzero(late & (lateness_ulps == 1))),
            "late_two_ulp_count": int(np.count_nonzero(late & (lateness_ulps == 2))),
            "late_more_than_two_ulp_count": int(
                np.count_nonzero(late & (lateness_ulps > 2))
            ),
            "certified_float_before_source_count": int(
                np.count_nonzero(certified_before)
            ),
            "exact_fraction_decision_source_count": len(exact_indices),
            "adaptive_decision_partition_closed": bool(
                np.count_nonzero(certified_before) + len(exact_indices)
                == len(triangles)
            ),
            "first_late_witness": first_late,
            "maximum_late_witness": maximum_late,
            "passed": not bool(np.any(late)),
        },
        late,
    )


def _fraction_text(value: Fraction) -> str:
    if value.denominator == 1:
        return str(value.numerator)
    return f"{value.numerator}/{value.denominator}"


def _exact_deadline_partition(
    points: np.ndarray,
    triangles: np.ndarray,
    paths: WeightedTreePaths,
) -> tuple[dict[str, object], np.ndarray, np.ndarray]:
    ab = paths.maximum_on_paths(triangles[:, 0], triangles[:, 1])
    ac = paths.maximum_on_paths(triangles[:, 0], triangles[:, 2])
    bc = paths.maximum_on_paths(triangles[:, 1], triangles[:, 2])
    raw_connections = np.maximum(np.maximum(ab, ac), bc)
    balls = [exact_triangle_miniball(points, triangle) for triangle in triangles]
    decisions: list[int] = []
    converted_levels: list[Fraction] = []
    raw_excesses: list[Fraction] = []
    ratios: list[Fraction] = []
    for raw, ball in zip(raw_connections, balls):
        converted = Fraction.from_float(float(raw)) / 4
        converted_levels.append(converted)
        raw_excesses.append(Fraction.from_float(float(raw)) - 4 * ball.squared_radius)
        ratios.append(converted / ball.squared_radius)
        decisions.append(
            -1
            if converted < ball.squared_radius
            else 0
            if converted == ball.squared_radius
            else 1
        )
    decision_array = np.asarray(decisions, dtype=np.int8)
    late = decision_array > 0
    support = np.asarray(
        [ball.support_cardinality for ball in balls], dtype=np.uint8
    )

    def witness(index: int) -> dict[str, object]:
        ball = balls[index]
        converted = converted_levels[index]
        return {
            "point_ids": [int(value) for value in triangles[index]],
            "source_squared_radius_exact": _fraction_text(ball.squared_radius),
            "source_squared_radius": float(ball.squared_radius),
            "raw_connection_squared_weight": float(raw_connections[index]),
            "raw_squared_weight_minus_four_radius_exact": _fraction_text(
                raw_excesses[index]
            ),
            "converted_connection_squared_level_exact": _fraction_text(converted),
            "converted_connection_squared_level": float(converted),
            "support_cardinality": ball.support_cardinality,
            "recertification": "exact_dyadic_Fraction_triangle_miniball",
        }

    first_late = witness(int(np.flatnonzero(late)[0])) if np.any(late) else None
    maximum_late = None
    if np.any(late):
        late_indices = np.flatnonzero(late)
        maximum_index = max(
            (int(index) for index in late_indices),
            key=lambda index: converted_levels[index] - balls[index].squared_radius,
        )
        maximum_late = witness(maximum_index)
    maximum_source = max(ball.squared_radius for ball in balls)
    maximum_converted = max(converted_levels)
    maximum_difference = max(
        converted - ball.squared_radius
        for converted, ball in zip(converted_levels, balls)
    )
    maximum_raw_excess = max(raw_excesses)
    maximum_ratio = max(ratios)
    return (
        {
            "source_count": len(triangles),
            "connected_before_count": int(np.count_nonzero(decision_array < 0)),
            "connected_at_count": int(np.count_nonzero(decision_array == 0)),
            "late_count": int(np.count_nonzero(late)),
            "never_connected_count": 0,
            "maximum_source_squared_radius": float(maximum_source),
            "maximum_raw_connection_squared_weight": float(raw_connections.max()),
            "maximum_converted_connection_squared_level": float(
                maximum_converted
            ),
            "maximum_converted_level_minus_source_level": float(
                maximum_difference
            ),
            "maximum_raw_squared_weight_minus_four_radius": float(
                maximum_raw_excess
            ),
            "maximum_lateness_ulps": 0,
            "maximum_connection_to_deadline_ratio": float(maximum_ratio),
            "late_support_two_count": int(np.count_nonzero(late & (support == 2))),
            "late_support_three_count": int(
                np.count_nonzero(late & (support == 3))
            ),
            "late_with_zero_binary64_recipe_ulp_count": int(
                np.count_nonzero(late)
            ),
            "late_one_ulp_count": 0,
            "late_two_ulp_count": 0,
            "late_more_than_two_ulp_count": 0,
            "certified_float_before_source_count": 0,
            "exact_fraction_decision_source_count": len(triangles),
            "adaptive_decision_partition_closed": True,
            "first_late_witness": first_late,
            "maximum_late_witness": maximum_late,
            "passed": not bool(np.any(late)),
        },
        late,
        support,
    )


def evaluate_k2(
    points: np.ndarray,
    oracle: DelaunayOracle,
    candidate: ExportedTree,
    *,
    batch_size: int,
    workers: int,
) -> dict[str, object]:
    _, cKDTree, _ = _load_spatial()
    kd_tree = cKDTree(points, balanced_tree=True, compact_nodes=True)
    paths = WeightedTreePaths(len(points), candidate)
    graph, batches = iter_owned_two_edge_triangles(
        len(points), oracle.edges, batch_size=batch_size
    )
    counter_names = (
        "source_count",
        "connected_before_count",
        "connected_at_count",
        "late_count",
        "certified_float_before_source_count",
        "exact_fraction_decision_source_count",
    )
    maximum_names = (
        "maximum_source_squared_radius",
        "maximum_raw_connection_squared_weight",
        "maximum_converted_connection_squared_level",
        "maximum_converted_level_minus_source_level",
        "maximum_raw_squared_weight_minus_four_radius",
        "maximum_lateness_ulps",
        "maximum_connection_to_deadline_ratio",
    )
    late_support_names = (
        "late_support_two_count",
        "late_support_three_count",
        "late_with_zero_binary64_recipe_ulp_count",
        "late_one_ulp_count",
        "late_two_ulp_count",
        "late_more_than_two_ulp_count",
    )
    all_counts = {name: 0 for name in counter_names}
    gabriel_counts = {name: 0 for name in counter_names}
    all_maxima = {name: -math.inf for name in maximum_names}
    gabriel_maxima = {name: -math.inf for name in maximum_names}
    all_late_support = {name: 0 for name in late_support_names}
    gabriel_late_support = {name: 0 for name in late_support_names}
    status_counts = np.zeros(4, dtype=np.int64)
    support_counts = np.zeros(4, dtype=np.int64)
    first_all_late = None
    first_gabriel_late = None
    maximum_all_late = None
    maximum_gabriel_late = None
    first_invalid = None
    exact_miniball_recertified_count = 0
    candidate_count = 0
    triangle_digest = hashlib.sha256()
    triangle_digest.update(b"MorseHGP3D/phase15/industrial-scientific-guard/two-edge-wedges/v1/")
    triangle_digest.update(struct.pack("<Q", len(points)))
    for triangles in batches:
        candidate_count += len(triangles)
        triangle_digest.update(triangles.astype("<u8", copy=False).tobytes(order="C"))
        balls = triangle_miniballs(points, triangles)
        invalid = ~balls.valid
        if np.any(invalid) and first_invalid is None:
            first_invalid = [int(value) for value in triangles[int(np.flatnonzero(invalid)[0])]]
        statuses = classify_gabriel_binary64(
            points, triangles, balls, kd_tree, workers=workers
        )
        status_counts += np.bincount(statuses, minlength=4)
        support_counts += np.bincount(
            balls.support_cardinalities.astype(np.int64), minlength=4
        )[:4]
        valid_indices = np.flatnonzero(balls.valid)
        if len(valid_indices):
            partition, late = _deadline_partition(
                triangles[valid_indices],
                balls.squared_radii[valid_indices],
                paths,
                balls.support_cardinalities[valid_indices],
                points=points,
            )
            for key in counter_names:
                all_counts[key] += int(partition[key])
            for key in maximum_names:
                value = (
                    int(partition[key])
                    if key == "maximum_lateness_ulps"
                    else float(partition[key])
                )
                all_maxima[key] = max(all_maxima[key], value)
            for key in late_support_names:
                all_late_support[key] += int(partition[key])
            if np.any(late) and first_all_late is None:
                first_all_late = partition["first_late_witness"]
            batch_maximum = partition["maximum_late_witness"]
            if batch_maximum is not None and (
                maximum_all_late is None
                or int(batch_maximum["lateness_ulps"])
                > int(maximum_all_late["lateness_ulps"])
            ):
                maximum_all_late = batch_maximum
        invalid_indices = np.flatnonzero(invalid)
        if len(invalid_indices):
            partition, late, exact_support = _exact_deadline_partition(
                points, triangles[invalid_indices], paths
            )
            exact_miniball_recertified_count += len(invalid_indices)
            status_counts[3] -= len(invalid_indices)
            support_counts[0] -= len(invalid_indices)
            support_counts += np.bincount(
                exact_support.astype(np.int64), minlength=4
            )[:4]
            for key in counter_names:
                all_counts[key] += int(partition[key])
            for key in maximum_names:
                value = (
                    int(partition[key])
                    if key == "maximum_lateness_ulps"
                    else float(partition[key])
                )
                all_maxima[key] = max(all_maxima[key], value)
            for key in late_support_names:
                all_late_support[key] += int(partition[key])
            if np.any(late) and first_all_late is None:
                first_all_late = partition["first_late_witness"]
            if np.any(late):
                maximum_all_late = partition["maximum_late_witness"]
        accepted_indices = np.flatnonzero(statuses == 1)
        if len(accepted_indices):
            partition, late = _deadline_partition(
                triangles[accepted_indices],
                balls.squared_radii[accepted_indices],
                paths,
                balls.support_cardinalities[accepted_indices],
                points=points,
            )
            for key in counter_names:
                gabriel_counts[key] += int(partition[key])
            for key in maximum_names:
                value = (
                    int(partition[key])
                    if key == "maximum_lateness_ulps"
                    else float(partition[key])
                )
                gabriel_maxima[key] = max(
                    gabriel_maxima[key], value
                )
            for key in late_support_names:
                gabriel_late_support[key] += int(partition[key])
            if np.any(late) and first_gabriel_late is None:
                first_gabriel_late = partition["first_late_witness"]
            batch_maximum = partition["maximum_late_witness"]
            if batch_maximum is not None and (
                maximum_gabriel_late is None
                or int(batch_maximum["lateness_ulps"])
                > int(maximum_gabriel_late["lateness_ulps"])
            ):
                maximum_gabriel_late = batch_maximum

    all_partition = {
        **all_counts,
        **{
            key: (
                None
                if value == -math.inf
                else int(value)
                if key == "maximum_lateness_ulps"
                else value
            )
            for key, value in all_maxima.items()
        },
        **all_late_support,
        "never_connected_count": 0,
        "unsupported_count": 0,
        "exact_miniball_recertified_source_count": (
            all_counts["exact_fraction_decision_source_count"]
        ),
        "binary64_invalid_exactly_recertified_source_count": (
            exact_miniball_recertified_count
        ),
        "adaptive_decision_partition_closed": bool(
            all_counts["certified_float_before_source_count"]
            + all_counts["exact_fraction_decision_source_count"]
            == all_counts["source_count"]
        ),
        "first_late_witness": first_all_late,
        "maximum_late_witness": maximum_all_late,
        "passed": bool(
            all_counts["late_count"] == 0 and status_counts[3] == 0
        ),
    }
    gabriel_partition = {
        **gabriel_counts,
        **{
            key: (
                None
                if value == -math.inf
                else int(value)
                if key == "maximum_lateness_ulps"
                else value
            )
            for key, value in gabriel_maxima.items()
        },
        **gabriel_late_support,
        "never_connected_count": 0,
        "unsupported_ambiguous_count": int(status_counts[2]),
        "adaptive_decision_partition_closed": bool(
            gabriel_counts["certified_float_before_source_count"]
            + gabriel_counts["exact_fraction_decision_source_count"]
            == gabriel_counts["source_count"]
        ),
        "first_late_witness": first_gabriel_late,
        "maximum_late_witness": maximum_gabriel_late,
        "regular_binary64_guardrail_passed": gabriel_counts["late_count"] == 0,
        "fail_closed_guardrail_passed": bool(
            gabriel_counts["late_count"] == 0 and status_counts[2] == 0
        ),
    }
    stronger_passed = bool(all_partition["passed"])
    return {
        "source_universe": "all_unique_triangles_with_at_least_two_ordinary_Delaunay_edges",
        "historical_subset": "gabriel_binary64_from_the_same_two_edge_wedge_universe",
        "level_conversion": "raw_squared_weight_exact_dyadic_divide_by_4",
        "source_level_convention": (
            "certified_longest_edge_lower_bound_for_strictly_before_then_"
            "exact_dyadic_Fraction_triangle_miniball_on_every_boundary_risk"
        ),
        "boundary": "closed_post_plateau",
        "raw_centered_wedge_count": graph.raw_wedge_count,
        "unique_two_edge_triangle_count": candidate_count,
        "maximum_delaunay_vertex_degree": graph.maximum_degree,
        "two_edge_triangle_sha256": triangle_digest.hexdigest(),
        "classification": {
            "blocked_count": int(status_counts[0]),
            "gabriel_binary64_count": int(status_counts[1]),
            "ambiguous_requires_cpu_recertification_count": int(status_counts[2]),
            "degenerate_or_invalid_count": int(status_counts[3]),
            "binary64_invalid_exactly_recertified_count": (
                exact_miniball_recertified_count
            ),
            "support_two_count": int(support_counts[2]),
            "support_three_count": int(support_counts[3]),
            "support_partition_closed": bool(
                int(support_counts[2] + support_counts[3]) == candidate_count
            ),
            "first_binary64_invalid_triangle": first_invalid,
            "first_unresolved_invalid_triangle": (
                first_invalid if status_counts[3] != 0 else None
            ),
            "status_partition_closed": (
                int(status_counts.sum()) + exact_miniball_recertified_count
                == candidate_count
            ),
            "exact_Gabriel_classification_claimed": False,
        },
        "all_two_edge_triangle_deadline": all_partition,
        "historical_gabriel_binary64_deadline": gabriel_partition,
        "stronger_all_two_edge_guard_passed": stronger_passed,
        "guard_passed": stronger_passed,
    }


def _sha256_u64_rows(rows: np.ndarray, domain: bytes) -> str:
    builder = hashlib.sha256()
    builder.update(domain)
    builder.update(np.asarray(rows, dtype="<u8").tobytes(order="C"))
    return builder.hexdigest()


def analyze_export(
    path: Path,
    *,
    family: str,
    runner_report_path: Path,
    expected_git_sha: str,
    runner_binary_path: Path,
    expected_runner_binary_sha256: str,
    expected_point_count: int | None = 50_000,
    expected_tree_count: int | None = 10,
    triangle_batch_size: int = 250_000,
    workers: int = -1,
) -> dict[str, object]:
    exported = read_industrial_export(
        path,
        expected_point_count=expected_point_count,
        expected_tree_count=expected_tree_count,
    )
    runner_binding = _bind_runner_report(
        exported,
        runner_report_path=runner_report_path,
        family=family,
        expected_git_sha=expected_git_sha,
        runner_binary_path=runner_binary_path,
        expected_runner_binary_sha256=expected_runner_binary_sha256,
    )
    points, point_bits = reconstruct_canonical_cloud(
        family, exported.point_count, exported.seed
    )
    oracle = build_delaunay_oracle(points)
    k1 = evaluate_k1(points, oracle, exported.trees[1])
    k2 = evaluate_k2(
        points,
        oracle,
        exported.trees[2],
        batch_size=triangle_batch_size,
        workers=workers,
    )
    final_binary_sha256, final_binary_size = _sha256_file(
        runner_binary_path, role="runner binary"
    )
    if (
        final_binary_sha256 != runner_binding["runner_binary_sha256"]
        or final_binary_sha256 != expected_runner_binary_sha256
        or final_binary_size != runner_binding["runner_binary_size_bytes"]
    ):
        raise ScientificGuardError(
            "runner binary changed during the scientific replay"
        )
    runner_binding["runner_binary_reverified_after_scientific_replay"] = True
    passed = bool(k1["guard_passed"] and k2["guard_passed"])
    return {
        "schema": SCHEMA,
        "input": {
            "export_path": str(path),
            "export_sha256": exported.artifact_sha256,
            "family": family,
            "point_count": exported.point_count,
            "seed": exported.seed,
            "tree_count": exported.tree_count,
            "point_cloud_binary64_bits_sha256": _sha256_u64_rows(
                point_bits,
                b"MorseHGP3D/phase15/industrial-scientific-guard/points/v1/",
            ),
        },
        "provenance": {
            "export_sha256": exported.artifact_sha256,
            **runner_binding,
        },
        "oracle": {
            "engine": "scipy.spatial.Delaunay_Qhull",
            "qhull_options": oracle.qhull_options,
            "exact_translation_used": oracle.exact_translation_used,
            "deterministic_joggle_used": oracle.deterministic_joggle_used,
            "primary_qhull_failure": oracle.primary_failure,
            "role": "offline_independent_sidecar_outside_timed_product_path",
            "tetrahedron_count": oracle.simplex_count,
            "ordinary_delaunay_edge_count": len(oracle.edges),
            "ordinary_delaunay_edges_sha256": _sha256_u64_rows(
                oracle.edges,
                b"MorseHGP3D/phase15/industrial-scientific-guard/delaunay-edges/v1/",
            ),
            "higher_order_Delaunay_mosaic_materialized": False,
        },
        "k1": k1,
        "k2": k2,
        "scope": {
            "timed_path_modified": False,
            "exact_Morse_hierarchy_claimed": False,
            "exact_Gamma2_claimed": False,
            "public_status": "not_claimed",
        },
        "result": "scientific_guards_satisfied" if passed else "scientific_guards_failed",
    }


def _git_sha_argument(value: str) -> str:
    if _GIT_SHA.fullmatch(value) is None:
        raise argparse.ArgumentTypeError(
            "expected one lowercase full 40-hex Git SHA"
        )
    return value


def _sha256_argument(value: str) -> str:
    if _SHA256.fullmatch(value) is None:
        raise argparse.ArgumentTypeError(
            "expected one lowercase 64-hex SHA-256 digest"
        )
    return value


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path, help="MHGPIE2E binary export")
    parser.add_argument("--family", required=True, choices=FAMILIES)
    parser.add_argument(
        "--runner-report",
        required=True,
        type=Path,
        help="paired v4/v5 runner report containing the uniquely exported measured run",
    )
    parser.add_argument(
        "--expected-git-sha",
        required=True,
        type=_git_sha_argument,
        help="full commit SHA from which the runner report and binary were built",
    )
    parser.add_argument(
        "--runner-binary",
        required=True,
        type=Path,
        help="runner binary artifact whose bytes produced the paired export",
    )
    parser.add_argument(
        "--expected-runner-binary-sha256",
        required=True,
        type=_sha256_argument,
        help="independent SHA-256 commitment expected for --runner-binary",
    )
    parser.add_argument("--expected-point-count", type=int, default=50_000)
    parser.add_argument("--expected-tree-count", type=int, default=10)
    parser.add_argument("--triangle-batch-size", type=int, default=250_000)
    parser.add_argument(
        "--workers",
        type=int,
        default=-1,
        help="cKDTree workers (-1 uses all available CPUs)",
    )
    parser.add_argument("--output", type=Path)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    options = _build_parser().parse_args(argv)
    if options.expected_point_count < 4 or options.expected_tree_count < 2:
        print("expected counts are outside the industrial export domain", file=sys.stderr)
        return 2
    if options.triangle_batch_size <= 0 or options.workers == 0 or options.workers < -1:
        print(
            "batch size must be positive and workers must be -1 or positive",
            file=sys.stderr,
        )
        return 2
    try:
        report = analyze_export(
            options.export,
            family=options.family,
            runner_report_path=options.runner_report,
            expected_git_sha=options.expected_git_sha,
            runner_binary_path=options.runner_binary,
            expected_runner_binary_sha256=(
                options.expected_runner_binary_sha256
            ),
            expected_point_count=options.expected_point_count,
            expected_tree_count=options.expected_tree_count,
            triangle_batch_size=options.triangle_batch_size,
            workers=options.workers,
        )
    except ScientificGuardError as error:
        print(f"scientific guard input error: {error}", file=sys.stderr)
        return 2
    try:
        encoded = json.dumps(
            report,
            sort_keys=True,
            separators=(",", ":"),
            allow_nan=False,
        ) + "\n"
    except (TypeError, ValueError) as error:
        print(f"the scientific report is not canonical JSON: {error}", file=sys.stderr)
        return 2
    if options.output is None:
        sys.stdout.write(encoded)
    else:
        try:
            options.output.write_text(encoded, encoding="utf-8")
        except OSError as error:
            print(f"cannot write report: {error}", file=sys.stderr)
            return 2
    return 0 if report["result"] == "scientific_guards_satisfied" else 1


if __name__ == "__main__":
    raise SystemExit(main())
