#!/usr/bin/env python3
"""Resumable fail-closed quality campaign for exact MorseHGP3D sources."""

from __future__ import annotations

import argparse
from collections import defaultdict
import copy
from fractions import Fraction
import hashlib
import math
import os
from pathlib import Path
import resource
import signal
import time
from typing import Any, NoReturn, Sequence
import zlib

from campaign_runtime import (
    CampaignSpool,
    RuntimeContractError,
    atomic_publish,
    campaign_identity,
    canonical_bytes,
    exact_git_sha,
    exact_sha256,
    fsync_directory,
    natural,
    read_canonical_json,
    require_regular_file,
    run_canonical_json_process,
    safe_relative_file,
    sha256_bytes,
    sha256_file,
)
import point_hierarchy_quality_generator as generator


PLAN_SCHEMA = "morsehgp3d.point_hierarchy.quality_campaign_plan.v2"
TRANSACTION_SCHEMA = "morsehgp3d.point_hierarchy.quality_transaction.v2"
PRODUCER_CAPABILITY_SCHEMA = "morsehgp3d.point_hierarchy.quality_producer_capabilities.v2"
VERIFIER_CAPABILITY_SCHEMA = "morsehgp3d.point_hierarchy.quality_verifier_capabilities.v2"
SOURCE_CERTIFICATE_SCHEMA = "morsehgp3d.point_hierarchy.exact_source_certificate.v2"
HIERARCHY_CERTIFICATE_SCHEMA = "morsehgp3d.point_hierarchy.laminar_certificate.v2"
SOURCE_VERIFICATION_SCHEMA = "morsehgp3d.point_hierarchy.external_source_verification.v2"
HIERARCHY_VERIFICATION_SCHEMA = "morsehgp3d.point_hierarchy.external_laminarity_verification.v2"
METRICS_SCHEMA = "morsehgp3d.point_hierarchy.independent_quality_metrics.v2"
SUMMARY_SCHEMA = "morsehgp3d.point_hierarchy.quality_campaign_summary.v2"
PRODUCER_CAPABILITY_ARGUMENT = "--morsehgp3d-point-quality-producer-capabilities-v2"
VERIFIER_CAPABILITY_ARGUMENT = "--morsehgp3d-point-quality-verifier-capabilities-v2"
SOURCE_ARGUMENT = "--morsehgp3d-point-quality-source-v2"
REDUCTION_ARGUMENT = "--morsehgp3d-point-quality-reduction-v2"
BASELINE_ARGUMENT = "--morsehgp3d-point-quality-baseline-v2"
VERIFY_SOURCE_ARGUMENT = "--morsehgp3d-point-quality-verify-source-v2"
VERIFY_HIERARCHY_ARGUMENT = "--morsehgp3d-point-quality-verify-hierarchy-v2"
DEFAULT_PLAN = Path(__file__).with_name("point_hierarchy_quality_campaign_v2.json")
GENERATOR_PATH = Path(generator.__file__).resolve()
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "exact_t1_through_t10_prefix_quality_v2"
K_VALUES = (1, 5, 10)
EXP_Z_VALUES = (1, 2, 3)
MINIMUM_CLUSTER_SIZE = 20
MAXIMUM_ORDER = 10
DATASET_COUNT = 6
SOURCE_BUILD_COUNT = 6
REDUCTION_COUNT = 54
BASELINE_COUNT = 18
TRANSACTION_COUNT = 84


class ContractError(ValueError):
    """A quality document escaped the frozen v2 contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def _profiles() -> list[dict[str, Any]]:
    return [
        {"generator": "dyadic_separated_balls_v2", "id": "separated_8", "parameters": {"ball_radius_denominator_exponent": 10, "center_grid_side": 2, "minimum_center_separation_denominator_exponent": 2}, "seed": 7101, "true_cluster_count": 8, "true_noise_fraction_denominator": 100, "true_noise_fraction_numerator": 2},
        {"generator": "dyadic_balanced_multiscale_clusters_v2", "id": "multiscale_64", "parameters": {"center_grid_side": 4, "cluster_size_ratio": 8, "radius_scale_level_count": 8, "radius_scale_ratio": 64}, "seed": 7201, "true_cluster_count": 64, "true_noise_fraction_denominator": 100, "true_noise_fraction_numerator": 5},
        {"generator": "dyadic_piecewise_linear_tubes_v2", "id": "filaments_24", "parameters": {"minimum_filament_separation_denominator_exponent": 6, "segment_count_per_filament": 8, "tube_radius_denominator_exponent": 12}, "seed": 7301, "true_cluster_count": 24, "true_noise_fraction_denominator": 100, "true_noise_fraction_numerator": 5},
        {"generator": "dyadic_imbalanced_bridged_pairs_v2", "id": "bridged_96", "parameters": {"bridge_fraction_denominator": 100, "bridge_fraction_numerator": 2, "bridge_tube_radius_denominator_exponent": 13, "cluster_size_ratio": 16, "inter_pair_separation_denominator_exponent": 3, "paired_center_separation_denominator_exponent": 7}, "seed": 7401, "true_cluster_count": 96, "true_noise_fraction_denominator": 100, "true_noise_fraction_numerator": 10},
    ]


def expected_plan_document(*, entry_gate_satisfied: bool = False) -> dict[str, Any]:
    return {
        "backend": BACKEND,
        "campaign_entry_gate_satisfied": entry_gate_satisfied,
        "claims": {"deployment_status": "architecture_only", "public_exact_status_claimed": False, "quality_campaign_promotes_exact": False},
        "counts": {"baseline_transactions": BASELINE_COUNT, "datasets": DATASET_COUNT, "exact_t1_through_t10_source_builds": SOURCE_BUILD_COUNT, "reduction_render_transactions": REDUCTION_COUNT, "transactions": TRANSACTION_COUNT},
        "datasets": {"fifty_k": {"point_count": 50_000, "profiles": _profiles()}, "massive": {"point_counts": [10_000_001, 30_000_000], "profile": copy.deepcopy(_profiles()[1])}},
        "generator": {"bounded_memory_streaming": True, "coordinate_compression": "zlib_level_1", "coordinate_denominator_exponent": generator.COORDINATE_DENOMINATOR_EXPONENT, "coordinate_record_bytes": generator.COORDINATE_RECORD_BYTES, "every_coordinate_materialized": True, "generator_dependencies": {"numpy": "2.5.1"}, "generator_id": generator.GENERATOR_ID, "module_file": GENERATOR_PATH.name, "raw_cloud_digest_domain_ascii": generator.RAW_CLOUD_DIGEST_DOMAIN[:-1].decode("ascii"), "raw_coordinate_stream_is_input_authority": True, "streaming_block_point_count": generator.BLOCK_POINT_COUNT, "truth_generated_outside_morse_binary": True},
        "matrix": {"exp_z": list(EXP_Z_VALUES), "k": list(K_VALUES), "minimum_cluster_size": MINIMUM_CLUSTER_SIZE},
        "mode": MODE,
        "options": {"dbscan": {"epsilon_policy": "q90_non_self_neighbor_distance_exact_dyadic", "minimum_cluster_size": MINIMUM_CLUSTER_SIZE, "reference_order": "K", "tie_policy": "include_complete_equal_distance_shell"}, "eom": {"allow_single_cluster": False, "minimum_cluster_size": MINIMUM_CLUSTER_SIZE}, "min_samples": "K_including_self_sklearn_1_9", "order_weights": "uniform_exact_rational_1_over_K", "simplex_point_weighting": "inverse_radius"},
        "profile": PROFILE,
        "protocol": {"baseline_dependencies": {"joblib": "1.5.3", "numpy": "2.5.1", "scikit_learn": "1.9.0", "scipy": "1.18.0", "threadpoolctl": "3.6.0"}, "producer_capability_argument": PRODUCER_CAPABILITY_ARGUMENT, "producer_capability_schema": PRODUCER_CAPABILITY_SCHEMA, "scientific_verifier_is_distinct_required": True, "verifier_capability_argument": VERIFIER_CAPABILITY_ARGUMENT, "verifier_capability_schema": VERIFIER_CAPABILITY_SCHEMA},
        "schema": PLAN_SCHEMA,
        "source": {"all_orders_same_cloud": True, "build_once_per_dataset": True, "complete_order_range": [1, MAXIMUM_ORDER], "prefixes": list(K_VALUES), "surrogate_allowed": False, "synthetic_preconstructed_tower_allowed": False},
        "timeouts_seconds": {"baseline": 600, "generator": {"fifty_k": 120, "massive": 1800}, "reduction": 1800, "source": 7200, "verifier": 1800},
        "transaction_store": {"atomic_fsync_receipts": True, "committed_attempts_retired_after_archive": True, "graceful_stop_margin_seconds": 600, "implementation": "CampaignSpool", "partial_session_summaries": True, "resume_from_next_uncommitted_ordinal": True, "source_bundle_persisted_in_spool": True, "unreferenced_artifacts_retired_on_resume": True},
    }


def read_plan(path: Path) -> tuple[dict[str, Any], str]:
    value, payload = read_canonical_json(path, "point quality v2 plan")
    gate = value.get("campaign_entry_gate_satisfied")
    require(type(gate) is bool, "campaign entry gate must be Boolean")
    require(value == expected_plan_document(entry_gate_satisfied=gate), "quality plan differs from frozen v2")
    return value, sha256_bytes(payload)


def dataset_instances(plan: dict[str, Any]) -> list[dict[str, Any]]:
    instances = [
        {"dataset": profile, "dataset_id": f"50k-{profile['id']}", "point_count": plan["datasets"]["fifty_k"]["point_count"], "role": "fifty_k"}
        for profile in plan["datasets"]["fifty_k"]["profiles"]
    ]
    instances.extend(
        {"dataset": plan["datasets"]["massive"]["profile"], "dataset_id": f"n{count}-multiscale_64", "point_count": count, "role": "massive"}
        for count in plan["datasets"]["massive"]["point_counts"]
    )
    require(len(instances) == DATASET_COUNT, "dataset matrix cardinality differs")
    return instances


def expected_transactions(plan: dict[str, Any]) -> list[dict[str, Any]]:
    transactions: list[dict[str, Any]] = []
    for instance in dataset_instances(plan):
        common = {"dataset": instance["dataset"], "dataset_id": instance["dataset_id"], "point_count": instance["point_count"], "scale_role": instance["role"]}
        generator_id = f"{instance['dataset_id']}-generator"
        source_id = f"{instance['dataset_id']}-source-t1-t10"
        transactions.append({**common, "role": "generator", "run_id": generator_id, "schema": TRANSACTION_SCHEMA})
        transactions.append({**common, "generator_run_id": generator_id, "role": "source", "run_id": source_id, "schema": TRANSACTION_SCHEMA})
        for k_value in K_VALUES:
            for exp_z in EXP_Z_VALUES:
                transactions.append({
                    **common,
                    "exp_z": {"denominator": 1, "numerator": exp_z},
                    "maximum_order": k_value,
                    "options": {"dbscan_reference_order": k_value, "eom_allow_single_cluster": False, "minimum_cluster_size": MINIMUM_CLUSTER_SIZE, "order_weights": [{"denominator": k_value, "numerator": 1, "order": order} for order in range(1, k_value + 1)], "simplex_point_weighting": "inverse_radius"},
                    "role": "reduction",
                    "run_id": f"{instance['dataset_id']}-k{k_value}-z{exp_z}",
                    "schema": TRANSACTION_SCHEMA,
                    "source_run_id": source_id,
                })
        for k_value in K_VALUES:
            transactions.append({**common, "maximum_order": k_value, "min_samples": k_value, "minimum_cluster_size": MINIMUM_CLUSTER_SIZE, "role": "baseline", "run_id": f"{instance['dataset_id']}-baseline-k{k_value}", "schema": TRANSACTION_SCHEMA})
    require(len(transactions) == TRANSACTION_COUNT, "transaction matrix cardinality differs")
    return transactions


def producer_contract() -> dict[str, Any]:
    return {"all_trees_t1_through_t10_complete": True, "baseline_dependencies": {"joblib": "1.5.3", "numpy": "2.5.1", "scikit_learn": "1.9.0", "scipy": "1.18.0", "threadpoolctl": "3.6.0"}, "baseline_emits_labels_not_metrics": True, "build_source_once_per_cloud": True, "exact_source_certificates": True, "hierarchy_emits_labels_not_metrics": True, "k_prefixes": list(K_VALUES), "materialized_coordinate_stream_required": True, "point_mst_surrogate": False, "public_exact_status_claimed": False, "sklearn_hdbscan_available": True, "sklearn_min_samples_includes_self": True, "source_authority_replay_delegated_to_external_verifier": True, "source_bundle_is_durable_and_content_bound": True, "surrogate_source_used": False, "synthetic_preconstructed_tower_allowed": False}


def verifier_contract() -> dict[str, Any]:
    return {"coordinate_stream_digest_replay": True, "hierarchy_certificate_replay": True, "independent_implementation": True, "laminarity_all_cuts_replay": True, "public_exact_status_claimed": False, "source_bundle_replay": True, "source_certificate_replay": True, "source_prefix_digest_replay": True}


def _validate_capability(value: Any, *, kind: str, binary_sha256: str, binary_size: int, plan_sha256: str) -> dict[str, Any]:
    schema = PRODUCER_CAPABILITY_SCHEMA if kind == "producer" else VERIFIER_CAPABILITY_SCHEMA
    contract = producer_contract() if kind == "producer" else verifier_contract()
    require(isinstance(value, dict) and set(value) == {"backend", "binary_sha256", "binary_size_bytes", "contract", "git_sha", "mode", "plan_sha256", "profile", "schema"}, f"{kind} capability fields differ")
    require(value["schema"] == schema and value["backend"] == BACKEND and value["profile"] == PROFILE and value["mode"] == MODE, f"{kind} capability identity differs")
    require(exact_sha256(value["binary_sha256"], f"{kind} binary digest") == binary_sha256, f"{kind} binary digest differs")
    require(natural(value["binary_size_bytes"], f"{kind} binary size", positive=True) == binary_size, f"{kind} binary size differs")
    require(exact_sha256(value["plan_sha256"], f"{kind} plan digest") == plan_sha256, f"{kind} plan digest differs")
    exact_git_sha(value["git_sha"], f"{kind} Git SHA")
    require(value["contract"] == contract, f"{kind} scientific contract differs")
    return value


def _artifact_descriptor(value: Any, label: str) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"file", "sha256", "size_bytes"}, f"{label} descriptor fields differ")
    require(isinstance(value["file"], str) and value["file"] and Path(value["file"]).name == value["file"], f"{label} file must be local")
    exact_sha256(value["sha256"], f"{label} digest")
    natural(value["size_bytes"], f"{label} size", positive=True)
    return value


def _resources(value: Any, label: str) -> dict[str, int]:
    require(isinstance(value, dict) and set(value) == {"peak_device_bytes", "peak_host_bytes", "wall_time_ns"}, f"{label} resource fields differ")
    natural(value["peak_device_bytes"], f"{label} peak device bytes")
    natural(value["peak_host_bytes"], f"{label} peak host bytes", positive=True)
    natural(value["wall_time_ns"], f"{label} wall time", positive=True)
    return value


def _harness_resources(started_ns: int) -> dict[str, int]:
    peak_kib = max(1, int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss))
    return {"peak_device_bytes": 0, "peak_host_bytes": peak_kib * 1024, "wall_time_ns": max(1, time.monotonic_ns() - started_ns)}


def _read_local_artifact(root: Path, descriptor: dict[str, Any], label: str) -> dict[str, Any]:
    descriptor = _artifact_descriptor(descriptor, label)
    path = root / descriptor["file"]
    value, payload = read_canonical_json(path, label)
    require(len(payload) == descriptor["size_bytes"] and sha256_bytes(payload) == descriptor["sha256"], f"{label} artifact binding differs")
    return value


def _import_artifact(spool: CampaignSpool, attempt: Path, descriptor: dict[str, Any], *, ordinal: int, label: str, suffix: str = ".json") -> dict[str, Any]:
    descriptor = _artifact_descriptor(descriptor, label)
    require(suffix in {".bin", ".json"}, "artifact archive suffix differs")
    destination = f"artifacts/{ordinal:05d}-{label}-{descriptor['sha256']}{suffix}"
    relative = spool.import_regular_file(source_root=attempt, source_file=descriptor["file"], destination_file=destination, expected_size_bytes=descriptor["size_bytes"], expected_sha256=descriptor["sha256"], label=label)
    return {"file": relative, "sha256": descriptor["sha256"], "size_bytes": descriptor["size_bytes"]}


def _artifact_reference(value: Any, label: str) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"file", "sha256", "size_bytes"}, f"{label} reference fields differ")
    safe_relative_file(value["file"], f"{label} reference file")
    exact_sha256(value["sha256"], f"{label} reference digest")
    natural(value["size_bytes"], f"{label} reference size", positive=True)
    return value


def _read_spool_artifact(spool: CampaignSpool, reference: Any, label: str) -> dict[str, Any]:
    reference = _artifact_reference(reference, label)
    path = spool.physical_root() / reference["file"]
    value, payload = read_canonical_json(path, label)
    require(len(payload) == reference["size_bytes"] and sha256_bytes(payload) == reference["sha256"], f"{label} archive binding differs")
    return value


def _validate_binary_reference(root: Path, reference: Any, label: str) -> Path:
    reference = _artifact_reference(reference, label)
    path = root / reference["file"]
    require_regular_file(path, label)
    require(path.stat().st_size == reference["size_bytes"] and sha256_file(path, label) == reference["sha256"], f"{label} archive binding differs")
    return path


def _archived_artifact_references(value: Any) -> list[dict[str, Any]]:
    references: list[dict[str, Any]] = []
    if isinstance(value, dict):
        if set(value) == {"file", "sha256", "size_bytes"} and isinstance(value.get("file"), str) and value["file"].startswith("artifacts/"):
            references.append(_artifact_reference(value, "receipt artifact"))
        else:
            for item in value.values():
                references.extend(_archived_artifact_references(item))
    elif isinstance(value, list):
        for item in value:
            references.extend(_archived_artifact_references(item))
    return references


def _retire_committed_attempt(spool: CampaignSpool, *, ordinal: int, request_sha256: str, receipt: dict[str, Any]) -> None:
    root = spool.physical_root()
    attempts_parent = root / "attempts"
    attempt = attempts_parent / f"{ordinal:05d}-{exact_sha256(request_sha256, 'retired attempt request digest')}"
    require(attempt.is_dir() and not attempt.is_symlink() and attempt.parent == attempts_parent, "committed attempt directory is invalid")
    references = _archived_artifact_references(receipt)
    bindings = {(reference["sha256"], reference["size_bytes"]): reference for reference in references}
    for entry in sorted(attempt.iterdir(), key=lambda path: path.name):
        require(entry.is_file() and not entry.is_symlink() and entry.parent == attempt, "committed attempt contains a non-regular staged object")
        binding = (sha256_file(entry, "committed staged artifact"), entry.stat().st_size)
        require(binding in bindings, "committed attempt contains an unauthenticated staged artifact")
        archive = root / bindings[binding]["file"]
        require_regular_file(archive, "archived counterpart")
        require(not os.path.samefile(entry, archive), "attempt cleanup would delete a referenced artifact")
        try:
            entry.unlink()
        except OSError as error:
            fail(f"cannot retire committed staged artifact: {error}")
    fsync_directory(attempt)


def _retire_unreferenced_archives(spool: CampaignSpool, receipts: list[dict[str, Any]]) -> None:
    root = spool.physical_root()
    archive = root / "artifacts"
    require(archive.is_dir() and not archive.is_symlink(), "campaign artifact archive is invalid")
    referenced = {reference["file"] for receipt in receipts for reference in _archived_artifact_references(receipt)}
    for relative in referenced:
        reference_path = root / relative
        require(reference_path.parent == archive, "receipt artifact escaped the archive")
        require_regular_file(reference_path, "referenced campaign artifact")
    for entry in sorted(archive.iterdir(), key=lambda path: path.name):
        require(entry.is_file() and not entry.is_symlink() and entry.parent == archive, "campaign artifact archive contains a non-regular object")
        relative = entry.relative_to(root).as_posix()
        if relative in referenced:
            continue
        try:
            entry.unlink()
        except OSError as error:
            fail(f"cannot retire unreferenced campaign artifact: {error}")
    fsync_directory(archive)


def validate_materialized_cloud(
    value: Any,
    *,
    root: Path,
    coordinate_reference: Any,
    transaction: dict[str, Any],
    generator_sha256: str,
    truth_sha256: str,
    label: str,
) -> str:
    expected_fields = {"coordinate_encoding", "coordinate_stream", "dataset", "generator_id", "generator_sha256", "geometry_contract", "point_count", "raw_cloud_sha256", "raw_coordinate_bytes", "schema", "truth_rle_sha256"}
    require(isinstance(value, dict) and set(value) == expected_fields, f"{label} manifest fields differ")
    point_count = transaction["point_count"]
    require(value["schema"] == generator.CLOUD_SCHEMA and value["generator_id"] == generator.GENERATOR_ID, f"{label} manifest identity differs")
    require(value["dataset"] == transaction["dataset"] and value["point_count"] == point_count, f"{label} dataset binding differs")
    require(value["generator_sha256"] == generator_sha256 and value["truth_rle_sha256"] == truth_sha256, f"{label} generator/truth binding differs")
    require(value["geometry_contract"] == generator.geometry_contract(transaction["dataset"], point_count), f"{label} geometry properties differ")
    encoding = value["coordinate_encoding"]
    require(encoding == {"compression": "zlib_level_1", "coordinate_denominator_exponent": generator.COORDINATE_DENOMINATOR_EXPONENT, "point_id_domain": [0, point_count], "record_bytes": generator.COORDINATE_RECORD_BYTES, "record_layout": "little_endian_signed_int64_x_y_z"}, f"{label} coordinate encoding differs")
    require(value["raw_coordinate_bytes"] == point_count * generator.COORDINATE_RECORD_BYTES, f"{label} raw coordinate length differs")
    raw_cloud_sha256 = exact_sha256(value["raw_cloud_sha256"], f"{label} raw cloud digest")
    manifest_stream = _artifact_descriptor(value["coordinate_stream"], f"{label} coordinate stream")
    reference = _artifact_reference(coordinate_reference, f"{label} coordinate stream")
    require(reference["sha256"] == manifest_stream["sha256"] and reference["size_bytes"] == manifest_stream["size_bytes"], f"{label} coordinate descriptor binding differs")
    path = root / reference["file"]
    require_regular_file(path, f"{label} coordinate stream")
    compressed_digest = hashlib.sha256()
    raw_digest = hashlib.sha256(generator.RAW_CLOUD_DIGEST_DOMAIN)
    compressed_size = 0
    raw_size = 0
    decompressor = zlib.decompressobj()
    try:
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                compressed_size += len(block)
                compressed_digest.update(block)
                pending = block
                while True:
                    payload = decompressor.decompress(pending, 1024 * 1024)
                    raw_size += len(payload)
                    raw_digest.update(payload)
                    pending = decompressor.unconsumed_tail
                    if pending or len(payload) == 1024 * 1024:
                        continue
                    break
        payload = decompressor.flush(1024 * 1024)
        raw_size += len(payload)
        raw_digest.update(payload)
    except (OSError, zlib.error) as error:
        fail(f"cannot replay {label} coordinate stream: {error}")
    require(decompressor.eof and not decompressor.unused_data and not decompressor.unconsumed_tail, f"{label} coordinate stream is truncated or concatenated")
    require(compressed_size == reference["size_bytes"] and compressed_digest.hexdigest() == reference["sha256"], f"{label} compressed coordinate binding differs")
    require(raw_size == point_count * generator.COORDINATE_RECORD_BYTES, f"{label} does not materialize exactly one coordinate record per PointId")
    require(raw_digest.hexdigest() == raw_cloud_sha256, f"{label} raw coordinate digest differs")
    return raw_cloud_sha256


def validate_label_rle(value: Any, *, point_count: int, semantics: str | None = None) -> list[tuple[int, int, int]]:
    require(isinstance(value, dict) and set(value) in ({"point_count", "runs", "schema", "semantics"}, {"point_count", "runs", "schema", "selected_nodes", "semantics"}), "label RLE fields differ")
    require(value["schema"] == generator.LABEL_RLE_SCHEMA and value["point_count"] == point_count, "label RLE identity differs")
    if semantics is not None:
        require(value["semantics"] == semantics, "label RLE semantics differ")
    require(isinstance(value["runs"], list) and value["runs"], "label RLE is empty")
    parsed: list[tuple[int, int, int]] = []
    cursor = 0
    previous_label: int | None = None
    for record in value["runs"]:
        require(isinstance(record, list) and len(record) == 3 and all(type(item) is int for item in record), "label RLE record differs")
        begin, end, label = record
        require(begin == cursor and end > begin and end <= point_count and label >= -1, "label RLE does not partition PointId exactly once")
        require(label != previous_label, "adjacent equal RLE labels are not canonical")
        parsed.append((begin, end, label))
        cursor, previous_label = end, label
    require(cursor == point_count, "label RLE does not cover every PointId")
    return parsed


def _contingency(truth: list[tuple[int, int, int]], prediction: list[tuple[int, int, int]], predicate: Any = None) -> dict[tuple[int, int], int]:
    result: dict[tuple[int, int], int] = defaultdict(int)
    left = right = 0
    position = 0
    while left < len(truth) and right < len(prediction):
        tb, te, tl = truth[left]
        pb, pe, pl = prediction[right]
        require(max(tb, pb) == position, "RLE sweep lost a PointId")
        end = min(te, pe)
        if predicate is None or predicate(tl, pl):
            result[(tl, pl)] += end - position
        position = end
        if te == end:
            left += 1
        if pe == end:
            right += 1
    require(left == len(truth) and right == len(prediction), "RLE sweep ended asymmetrically")
    return dict(result)


def _ari_nmi(table: dict[tuple[int, int], int]) -> tuple[float, float]:
    n = sum(table.values())
    if n == 0:
        return 0.0, 0.0
    rows: dict[int, int] = defaultdict(int)
    columns: dict[int, int] = defaultdict(int)
    for (row, column), count in table.items():
        rows[row] += count
        columns[column] += count
    choose_total = n * (n - 1) // 2
    sum_cells = sum(count * (count - 1) // 2 for count in table.values())
    sum_rows = sum(count * (count - 1) // 2 for count in rows.values())
    sum_columns = sum(count * (count - 1) // 2 for count in columns.values())
    if choose_total == 0:
        ari = 1.0
    else:
        expected = Fraction(sum_rows * sum_columns, choose_total)
        denominator = Fraction(sum_rows + sum_columns, 2) - expected
        ari = float((Fraction(sum_cells, 1) - expected) / denominator) if denominator else 1.0
    mutual = 0.0
    for (row, column), count in table.items():
        mutual += (count / n) * math.log((count * n) / (rows[row] * columns[column]))
    row_entropy = -sum((count / n) * math.log(count / n) for count in rows.values())
    column_entropy = -sum((count / n) * math.log(count / n) for count in columns.values())
    denominator = 0.5 * (row_entropy + column_entropy)
    nmi = mutual / denominator if denominator else 1.0
    return max(-1.0, min(1.0, ari)), max(0.0, min(1.0, nmi))


def compute_metrics(truth_value: dict[str, Any], prediction_value: dict[str, Any], *, point_count: int) -> dict[str, Any]:
    truth = validate_label_rle(truth_value, point_count=point_count, semantics="ground_truth")
    prediction = validate_label_rle(prediction_value, point_count=point_count)
    all_table = _contingency(truth, prediction)
    inlier_table = _contingency(truth, prediction, lambda true, _pred: true >= 0)
    classified_table = _contingency(truth, prediction, lambda true, pred: true >= 0 and pred >= 0)
    ari_all, nmi_all = _ari_nmi(all_table)
    ari_inliers, nmi_inliers = _ari_nmi(inlier_table)
    ari_classified, nmi_classified = _ari_nmi(classified_table)
    true_noise = sum(count for (true, _), count in all_table.items() if true < 0)
    predicted_noise = sum(count for (_, pred), count in all_table.items() if pred < 0)
    correct_noise = sum(count for (true, pred), count in all_table.items() if true < 0 and pred < 0)
    classified = point_count - predicted_noise
    true_inliers = point_count - true_noise
    classified_true_inliers = sum(classified_table.values())
    true_sizes: dict[int, int] = defaultdict(int)
    pred_sizes: dict[int, int] = defaultdict(int)
    for (true, _pred), count in inlier_table.items():
        true_sizes[true] += count
    for (true, pred), count in classified_table.items():
        pred_sizes[pred] += count
    predicted_labels = {pred for (_true, pred) in all_table if pred >= 0}
    dominant_by_true = sum(max((count for (true2, _), count in classified_table.items() if true2 == true), default=0) for true in true_sizes)
    dominant_by_pred = sum(max((count for (_, pred2), count in classified_table.items() if pred2 == pred), default=0) for pred in pred_sizes)
    classified_mass = sum(classified_table.values())
    fragments = []
    for true, size in true_sizes.items():
        threshold = max(1, (size + 99) // 100)
        fragments.append(sum(1 for (true2, _), count in classified_table.items() if true2 == true and count >= threshold))
    fragments.sort()
    p95_fragment = fragments[min(len(fragments) - 1, math.ceil(0.95 * len(fragments)) - 1)] if fragments else 0
    ratio = lambda numerator, denominator: numerator / denominator if denominator else 0.0
    precision = ratio(correct_noise, predicted_noise)
    recall = ratio(correct_noise, true_noise)
    quality = {
        "adjusted_rand_all": ari_all,
        "adjusted_rand_classified_only": ari_classified,
        "adjusted_rand_true_inliers": ari_inliers,
        "classified_fraction": ratio(classified, point_count),
        "cluster_count_ratio": ratio(len(predicted_labels), len(true_sizes)),
        "fragmentation_error": 1.0 - ratio(dominant_by_true, classified_mass),
        "fusion_error": 1.0 - ratio(dominant_by_pred, classified_mass),
        "mean_significant_fragments_per_true_cluster": ratio(sum(fragments), len(fragments)),
        "noise_f1": 2.0 * precision * recall / (precision + recall) if precision + recall else 0.0,
        "noise_precision": precision,
        "noise_recall": recall,
        "normalized_mutual_info_all": nmi_all,
        "normalized_mutual_info_classified_only": nmi_classified,
        "normalized_mutual_info_true_inliers": nmi_inliers,
        "p95_significant_fragments_per_true_cluster": p95_fragment,
        "true_inlier_coverage": ratio(classified_true_inliers, true_inliers),
    }
    counts = {"classified_point_count": classified, "classified_true_inlier_point_count": classified_true_inliers, "correctly_detected_noise_point_count": correct_noise, "overlapping_assignment_count": 0, "point_count": point_count, "predicted_cluster_count": len(predicted_labels), "predicted_noise_point_count": predicted_noise, "true_cluster_count": len(true_sizes), "true_inlier_point_count": true_inliers, "true_noise_point_count": true_noise}
    return {"counts": counts, "quality": quality, "schema": METRICS_SCHEMA}


def _intervals(value: Any, *, point_count: int, label: str) -> list[tuple[int, int]]:
    require(isinstance(value, list), f"{label} intervals must be a list")
    result: list[tuple[int, int]] = []
    previous = 0
    for record in value:
        require(isinstance(record, list) and len(record) == 2 and all(type(item) is int for item in record), f"{label} interval differs")
        begin, end = record
        require(0 <= begin < end <= point_count and (not result or begin > previous), f"{label} intervals overlap or are noncanonical")
        result.append((begin, end))
        previous = end
    return result


def _union_intervals(groups: list[list[tuple[int, int]]]) -> list[tuple[int, int]]:
    flat = sorted(interval for group in groups for interval in group)
    result: list[tuple[int, int]] = []
    for begin, end in flat:
        require(not result or begin >= result[-1][1], "hierarchy children overlap")
        if result and begin == result[-1][1]:
            result[-1] = (result[-1][0], end)
        else:
            result.append((begin, end))
    return result


def _validate_dbscan_calibration(value: Any, *, transaction: dict[str, Any], raw_cloud_sha256: str) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"calculation_receipt_sha256", "epsilon_squared", "neighbor_rank", "quantile_denominator", "quantile_numerator", "raw_cloud_sha256", "reference_order", "tie_policy"}, "DBSCAN calibration fields differ")
    epsilon = value["epsilon_squared"]
    require(isinstance(epsilon, dict) and set(epsilon) == {"denominator", "numerator"}, "DBSCAN epsilon fields differ")
    require(isinstance(epsilon["numerator"], str) and isinstance(epsilon["denominator"], str) and epsilon["numerator"].isdigit() and epsilon["denominator"].isdigit(), "DBSCAN epsilon is not canonical")
    numerator, denominator = int(epsilon["numerator"]), int(epsilon["denominator"])
    require(numerator > 0 and denominator > 0 and math.gcd(numerator, denominator) == 1, "DBSCAN epsilon is not positive reduced")
    k_value = transaction["maximum_order"]
    require(value["reference_order"] == k_value and value["neighbor_rank"] == (1 if k_value == 1 else k_value - 1), "DBSCAN reference order or neighbor rank differs")
    require(value["quantile_numerator"] == 90 and value["quantile_denominator"] == 100 and value["tie_policy"] == "include_complete_equal_distance_shell", "DBSCAN quantile or tie policy differs")
    require(value["raw_cloud_sha256"] == raw_cloud_sha256, "DBSCAN calibration changed the raw cloud")
    _self_receipt(value, "calculation_receipt_sha256")
    return value


def validate_hierarchy_certificate(value: Any, *, transaction: dict[str, Any], source: dict[str, Any], dbscan_labels_sha256: str, eom_labels_sha256: str) -> tuple[dict[int, list[tuple[int, int]]], set[int]]:
    require(isinstance(value, dict) and set(value) == {"dbscan_calibration", "dbscan_labels_sha256", "eom_labels_sha256", "exp_z", "nodes", "options", "order_forest_digests", "point_count", "public_exact_status_claimed", "reduction_receipt_sha256", "root_id", "schema", "source_bundle_sha256", "source_certificate_sha256", "vertical_map_digests"}, "hierarchy certificate fields differ")
    require(value["schema"] == HIERARCHY_CERTIFICATE_SCHEMA and value["point_count"] == transaction["point_count"], "hierarchy certificate identity differs")
    require(value["exp_z"] == transaction["exp_z"] and value["options"] == transaction["options"], "hierarchy options differ")
    require(value["source_certificate_sha256"] == source["certificate_sha256"] and value["source_bundle_sha256"] == source["source_bundle_sha256"] and value["public_exact_status_claimed"] is False, "hierarchy source binding or claim differs")
    require(value["dbscan_labels_sha256"] == dbscan_labels_sha256 and value["eom_labels_sha256"] == eom_labels_sha256, "hierarchy certificate does not bind its label streams")
    _validate_dbscan_calibration(value["dbscan_calibration"], transaction=transaction, raw_cloud_sha256=source["raw_cloud_sha256"])
    k_value = transaction["maximum_order"]
    require(value["order_forest_digests"] == source["order_forest_digests"][:k_value], "hierarchy changed a source order prefix")
    require(value["vertical_map_digests"] == source["vertical_map_digests"][: max(0, k_value - 1)], "hierarchy changed a vertical prefix")
    exact_sha256(value["reduction_receipt_sha256"], "hierarchy reduction receipt")
    require(isinstance(value["nodes"], list) and value["nodes"], "hierarchy has no nodes")
    nodes: dict[int, dict[str, Any]] = {}
    coverage: dict[int, list[tuple[int, int]]] = {}
    for node in value["nodes"]:
        require(isinstance(node, dict) and set(node) == {"children", "coverage", "id", "parent", "terminal"}, "hierarchy node fields differ")
        node_id = natural(node["id"], "hierarchy node ID")
        require(node_id not in nodes and isinstance(node["children"], list) and type(node["terminal"]) is bool, "hierarchy node identity differs")
        require(node["parent"] is None or type(node["parent"]) is int, "hierarchy parent differs")
        require(all(type(child) is int for child in node["children"]) and len(set(node["children"])) == len(node["children"]), "hierarchy children differ")
        nodes[node_id] = node
        coverage[node_id] = _intervals(node["coverage"], point_count=transaction["point_count"], label="hierarchy coverage")
    root = value["root_id"]
    require(root in nodes and nodes[root]["parent"] is None, "hierarchy root differs")
    for node_id, node in nodes.items():
        if node_id != root:
            require(node["parent"] in nodes and node_id in nodes[node["parent"]]["children"], "hierarchy node lacks one consistent parent")
        for child in node["children"]:
            require(child in nodes and nodes[child]["parent"] == node_id, "hierarchy child-parent relation differs")
    visiting: set[int] = set()
    visited: set[int] = set()
    def visit(node_id: int) -> None:
        require(node_id not in visiting, "hierarchy contains a cycle")
        if node_id in visited:
            return
        visiting.add(node_id)
        for child in nodes[node_id]["children"]:
            visit(child)
        visiting.remove(node_id)
        visited.add(node_id)
    visit(root)
    require(visited == set(nodes), "hierarchy contains unreachable nodes")
    terminal_nodes: set[int] = set()
    for node_id in reversed(list(visited)):
        node = nodes[node_id]
        if node["children"]:
            require(node["terminal"] is False and _union_intervals([coverage[child] for child in node["children"]]) == coverage[node_id], "hierarchy parent coverage differs from disjoint children")
        else:
            require(node["terminal"] is True, "hierarchy leaf is not terminal")
            terminal_nodes.add(node_id)
    require(coverage[root] == [(0, transaction["point_count"])], "hierarchy root does not cover every point")
    require(_union_intervals([coverage[node] for node in terminal_nodes]) == coverage[root], "terminal nodes do not assign every PointId exactly once")
    return coverage, terminal_nodes


def validate_selection(value: Any, *, method: str, point_count: int, coverage: dict[int, list[tuple[int, int]]]) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"point_count", "runs", "schema", "selected_nodes", "semantics"}, "selection label artifact fields differ")
    runs = validate_label_rle(value, point_count=point_count, semantics=method)
    require(isinstance(value["selected_nodes"], list), "selected node mapping differs")
    selected: dict[int, int] = {}
    for record in value["selected_nodes"]:
        require(isinstance(record, dict) and set(record) == {"label", "node_id"}, "selected node record differs")
        label, node_id = record["label"], record["node_id"]
        require(type(label) is int and label >= 0 and type(node_id) is int and node_id in coverage and label not in selected, "selected node identity differs")
        selected[label] = node_id
    by_label: dict[int, list[tuple[int, int]]] = defaultdict(list)
    for begin, end, label in runs:
        if label >= 0:
            by_label[label].append((begin, end))
    require(set(by_label) == set(selected), "selection labels and nodes differ")
    for label, intervals in by_label.items():
        require(intervals == coverage[selected[label]], "selected node coverage differs from label stream")
    _union_intervals([coverage[node] for node in selected.values()])
    return value


def _self_receipt(value: dict[str, Any], field: str) -> None:
    exact_sha256(value.get(field), field)
    payload = {key: item for key, item in value.items() if key != field}
    require(value[field] == sha256_bytes(canonical_bytes(payload)), f"{field} differs")


def validate_source_certificate(value: Any, *, cloud_sha256: str, cloud_manifest_sha256: str, truth_sha256: str, source_bundle_sha256: str, source_bundle_size_bytes: int) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"authority_id", "cloud_authority_binding_sha256", "cloud_manifest_sha256", "complete_order_range", "exact_source_certified", "maximum_order", "order_forest_digests", "public_exact_status_claimed", "raw_cloud_sha256", "schema", "source_bundle_sha256", "source_bundle_size_bytes", "surrogate_source_used", "truth_rle_sha256", "vertical_map_digests"}, "source certificate fields differ")
    require(value["schema"] == SOURCE_CERTIFICATE_SCHEMA and value["maximum_order"] == MAXIMUM_ORDER and value["complete_order_range"] == [1, MAXIMUM_ORDER], "source certificate order range differs")
    require(value["raw_cloud_sha256"] == cloud_sha256 and value["cloud_manifest_sha256"] == cloud_manifest_sha256 and value["truth_rle_sha256"] == truth_sha256, "source certificate changed the requested cloud")
    require(value["exact_source_certified"] is True and value["surrogate_source_used"] is False and value["public_exact_status_claimed"] is False, "source certificate exactness flags differ")
    exact_sha256(value["authority_id"], "source authority ID")
    require(value["source_bundle_sha256"] == source_bundle_sha256 and value["source_bundle_size_bytes"] == source_bundle_size_bytes, "source certificate changed the durable source bundle")
    require(isinstance(value["order_forest_digests"], list) and len(value["order_forest_digests"]) == MAXIMUM_ORDER, "source order digest count differs")
    for index, record in enumerate(value["order_forest_digests"], start=1):
        require(isinstance(record, dict) and record.get("order") == index and set(record) == {"forest_sha256", "order"}, "source order digest differs")
        exact_sha256(record["forest_sha256"], "source forest digest")
    require(isinstance(value["vertical_map_digests"], list) and len(value["vertical_map_digests"]) == MAXIMUM_ORDER - 1, "source vertical digest count differs")
    for order, record in enumerate(value["vertical_map_digests"], start=2):
        require(isinstance(record, dict) and record == {"from_order": order, "to_order": order - 1, "vertical_sha256": record.get("vertical_sha256")}, "source vertical digest differs")
        exact_sha256(record["vertical_sha256"], "source vertical digest")
    binding = sha256_bytes(canonical_bytes({"authority_id": value["authority_id"], "cloud_manifest_sha256": cloud_manifest_sha256, "order_forest_digests": value["order_forest_digests"], "raw_cloud_sha256": cloud_sha256, "source_bundle_sha256": source_bundle_sha256, "source_bundle_size_bytes": source_bundle_size_bytes, "truth_rle_sha256": truth_sha256, "vertical_map_digests": value["vertical_map_digests"]}))
    require(value["cloud_authority_binding_sha256"] == binding, "cloud/source authority binding differs")
    return value


def validate_external_source(value: Any, *, source_sha256: str, source_bundle_sha256: str, cloud_sha256: str, cloud_manifest_sha256: str, coordinate_stream_sha256: str, verifier_sha256: str) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"all_orders_replayed", "bundle_replayed", "cloud_binding_replayed", "cloud_manifest_sha256", "coordinate_stream_sha256", "public_exact_status_claimed", "raw_cloud_sha256", "receipt_sha256", "schema", "source_bundle_sha256", "source_certificate_sha256", "verifier_sha256", "vertical_maps_replayed"}, "external source verification fields differ")
    require(value["schema"] == SOURCE_VERIFICATION_SCHEMA and value["source_certificate_sha256"] == source_sha256 and value["source_bundle_sha256"] == source_bundle_sha256 and value["raw_cloud_sha256"] == cloud_sha256 and value["cloud_manifest_sha256"] == cloud_manifest_sha256 and value["coordinate_stream_sha256"] == coordinate_stream_sha256 and value["verifier_sha256"] == verifier_sha256, "external source verification binding differs")
    require(value["all_orders_replayed"] is True and value["vertical_maps_replayed"] is True and value["bundle_replayed"] is True and value["cloud_binding_replayed"] is True and value["public_exact_status_claimed"] is False, "external source replay is incomplete")
    _self_receipt(value, "receipt_sha256")
    return value


def validate_external_hierarchy(value: Any, *, hierarchy_sha256: str, source_sha256: str, source_bundle_sha256: str, verifier_sha256: str) -> dict[str, Any]:
    require(isinstance(value, dict) and set(value) == {"acyclic", "all_cuts_laminar", "children_disjoint", "hierarchy_certificate_sha256", "parent_unique", "public_exact_status_claimed", "receipt_sha256", "root_covers_all_points", "schema", "source_bundle_sha256", "source_certificate_sha256", "terminal_unique", "verifier_sha256"}, "external hierarchy verification fields differ")
    require(value["schema"] == HIERARCHY_VERIFICATION_SCHEMA and value["hierarchy_certificate_sha256"] == hierarchy_sha256 and value["source_certificate_sha256"] == source_sha256 and value["source_bundle_sha256"] == source_bundle_sha256 and value["verifier_sha256"] == verifier_sha256, "external hierarchy verification binding differs")
    for field in ("acyclic", "all_cuts_laminar", "children_disjoint", "parent_unique", "root_covers_all_points", "terminal_unique"):
        require(value[field] is True, f"external laminarity replay failed: {field}")
    require(value["public_exact_status_claimed"] is False, "external verifier promoted public exactness")
    _self_receipt(value, "receipt_sha256")
    return value


def _base_receipt(transaction: dict[str, Any]) -> dict[str, Any]:
    return {"request_sha256": sha256_bytes(canonical_bytes(transaction)), "role": transaction["role"], "run_id": transaction["run_id"], "status": "complete"}


def _run_json(binary: Path, argument: str, request: dict[str, Any], timeout: int, label: str, cwd: Path | None = None) -> dict[str, Any]:
    return run_canonical_json_process([str(binary), argument], request=request, timeout_seconds=float(timeout), label=label, cwd=cwd)


def _generate_with_timeout(*, transaction: dict[str, Any], generator_sha256: str, attempt: Path, timeout: int) -> dict[str, Any]:
    def timeout_handler(_signum: int, _frame: Any) -> NoReturn:
        fail("generator transaction timeout")

    previous_timer = signal.getitimer(signal.ITIMER_REAL)
    require(previous_timer == (0.0, 0.0), "generator cannot replace an active process timer")
    previous_handler = signal.signal(signal.SIGALRM, timeout_handler)
    signal.setitimer(signal.ITIMER_REAL, float(timeout))
    try:
        return generator.generate(dataset=transaction["dataset"], point_count=transaction["point_count"], generator_sha256=generator_sha256, output_directory=attempt)
    finally:
        signal.setitimer(signal.ITIMER_REAL, 0.0)
        signal.signal(signal.SIGALRM, previous_handler)


def _transaction_generator(spool: CampaignSpool, attempt: Path, transaction: dict[str, Any], *, ordinal: int, generator_sha256: str, timeout: int) -> dict[str, Any]:
    started_ns = time.monotonic_ns()
    descriptors = _generate_with_timeout(transaction=transaction, generator_sha256=generator_sha256, attempt=attempt, timeout=timeout)
    require(time.monotonic_ns() - started_ns <= timeout * 1_000_000_000, "generator transaction timeout")
    require(isinstance(descriptors, dict) and set(descriptors) == {"cloud", "coordinates", "raw_cloud_sha256", "truth"}, "generator output descriptors differ")
    cloud = _read_local_artifact(attempt, descriptors["cloud"], "generated cloud")
    truth = _read_local_artifact(attempt, descriptors["truth"], "generated truth")
    validate_label_rle(truth, point_count=transaction["point_count"], semantics="ground_truth")
    raw_cloud_sha256 = validate_materialized_cloud(cloud, root=attempt, coordinate_reference=descriptors["coordinates"], transaction=transaction, generator_sha256=generator_sha256, truth_sha256=descriptors["truth"]["sha256"], label="generated cloud")
    require(descriptors["raw_cloud_sha256"] == raw_cloud_sha256, "generator raw cloud descriptor differs")
    return {**_base_receipt(transaction), "cloud": _import_artifact(spool, attempt, descriptors["cloud"], ordinal=ordinal, label="cloud"), "coordinates": _import_artifact(spool, attempt, descriptors["coordinates"], ordinal=ordinal, label="coordinates", suffix=".bin"), "generator_sha256": generator_sha256, "raw_cloud_sha256": raw_cloud_sha256, "resources": _harness_resources(started_ns), "truth": _import_artifact(spool, attempt, descriptors["truth"], ordinal=ordinal, label="truth")}


def _transaction_source(spool: CampaignSpool, attempt: Path, transaction: dict[str, Any], *, ordinal: int, generator_receipt: dict[str, Any], producer: Path, verifier: Path, verifier_sha256: str, timeouts: dict[str, int]) -> dict[str, Any]:
    cloud_path = spool.physical_root() / generator_receipt["cloud"]["file"]
    coordinate_path = spool.physical_root() / generator_receipt["coordinates"]["file"]
    truth_path = spool.physical_root() / generator_receipt["truth"]["file"]
    invocation = {"cloud_manifest_file": str(cloud_path), "cloud_manifest_sha256": generator_receipt["cloud"]["sha256"], "coordinate_stream_file": str(coordinate_path), "coordinate_stream_sha256": generator_receipt["coordinates"]["sha256"], "coordinate_stream_size_bytes": generator_receipt["coordinates"]["size_bytes"], "output_directory": str(attempt), "raw_cloud_sha256": generator_receipt["raw_cloud_sha256"], "schema": "morsehgp3d.point_quality.source_invocation.v2", "transaction": transaction, "truth_file": str(truth_path), "truth_sha256": generator_receipt["truth"]["sha256"]}
    output = _run_json(producer, SOURCE_ARGUMENT, invocation, timeouts["source"], "exact source transaction", attempt)
    require(isinstance(output, dict) and set(output) == {"resources", "source_bundle", "source_certificate"}, "source producer output fields differ")
    resources = _resources(output["resources"], "source producer")
    bundle_descriptor = _artifact_descriptor(output["source_bundle"], "source bundle")
    _validate_binary_reference(attempt, bundle_descriptor, "source bundle")
    certificate = _read_local_artifact(attempt, output["source_certificate"], "source certificate")
    validate_source_certificate(certificate, cloud_sha256=generator_receipt["raw_cloud_sha256"], cloud_manifest_sha256=generator_receipt["cloud"]["sha256"], truth_sha256=generator_receipt["truth"]["sha256"], source_bundle_sha256=bundle_descriptor["sha256"], source_bundle_size_bytes=bundle_descriptor["size_bytes"])
    verify_request = {"cloud_manifest_file": str(cloud_path), "cloud_manifest_sha256": generator_receipt["cloud"]["sha256"], "coordinate_stream_file": str(coordinate_path), "coordinate_stream_sha256": generator_receipt["coordinates"]["sha256"], "raw_cloud_sha256": generator_receipt["raw_cloud_sha256"], "schema": "morsehgp3d.point_quality.source_verify_request.v2", "source_bundle_file": str(attempt / bundle_descriptor["file"]), "source_bundle_sha256": bundle_descriptor["sha256"], "source_bundle_size_bytes": bundle_descriptor["size_bytes"], "source_certificate_file": str(attempt / output["source_certificate"]["file"]), "source_certificate_sha256": output["source_certificate"]["sha256"]}
    verification = _run_json(verifier, VERIFY_SOURCE_ARGUMENT, verify_request, timeouts["verifier"], "external source verification")
    validate_external_source(verification, source_sha256=output["source_certificate"]["sha256"], source_bundle_sha256=bundle_descriptor["sha256"], cloud_sha256=generator_receipt["raw_cloud_sha256"], cloud_manifest_sha256=generator_receipt["cloud"]["sha256"], coordinate_stream_sha256=generator_receipt["coordinates"]["sha256"], verifier_sha256=verifier_sha256)
    return {**_base_receipt(transaction), "generator_run_id": transaction["generator_run_id"], "resources": resources, "source_bundle": _import_artifact(spool, attempt, bundle_descriptor, ordinal=ordinal, label="source-bundle", suffix=".bin"), "source_certificate": _import_artifact(spool, attempt, output["source_certificate"], ordinal=ordinal, label="source"), "verification": verification}


def _transaction_reduction(spool: CampaignSpool, attempt: Path, transaction: dict[str, Any], *, ordinal: int, generator_receipt: dict[str, Any], source_receipt: dict[str, Any], producer: Path, verifier: Path, verifier_sha256: str, timeouts: dict[str, int]) -> dict[str, Any]:
    source_path = spool.physical_root() / source_receipt["source_certificate"]["file"]
    source_bundle_path = spool.physical_root() / source_receipt["source_bundle"]["file"]
    truth_value = _read_spool_artifact(spool, generator_receipt["truth"], "truth labels")
    source_value = _read_spool_artifact(spool, source_receipt["source_certificate"], "source certificate")
    source_context = {**source_value, "certificate_sha256": source_receipt["source_certificate"]["sha256"]}
    invocation = {"output_directory": str(attempt), "schema": "morsehgp3d.point_quality.reduction_invocation.v2", "source_bundle_file": str(source_bundle_path), "source_bundle_sha256": source_receipt["source_bundle"]["sha256"], "source_bundle_size_bytes": source_receipt["source_bundle"]["size_bytes"], "source_certificate_file": str(source_path), "source_certificate_sha256": source_receipt["source_certificate"]["sha256"], "transaction": transaction}
    output = _run_json(producer, REDUCTION_ARGUMENT, invocation, timeouts["reduction"], "Morse reduction transaction", attempt)
    require(isinstance(output, dict) and set(output) == {"dbscan_labels", "eom_labels", "hierarchy_certificate", "resources"}, "Morse producer must emit certificates, labels and resources only")
    resources = _resources(output["resources"], "Morse reduction")
    hierarchy = _read_local_artifact(attempt, output["hierarchy_certificate"], "hierarchy certificate")
    coverage, _ = validate_hierarchy_certificate(hierarchy, transaction=transaction, source=source_context, dbscan_labels_sha256=output["dbscan_labels"]["sha256"], eom_labels_sha256=output["eom_labels"]["sha256"])
    dbscan = _read_local_artifact(attempt, output["dbscan_labels"], "DBSCAN labels")
    eom = _read_local_artifact(attempt, output["eom_labels"], "EOM labels")
    validate_selection(dbscan, method="dbscan_radius", point_count=transaction["point_count"], coverage=coverage)
    validate_selection(eom, method="excess_of_mass", point_count=transaction["point_count"], coverage=coverage)
    verify_request = {"hierarchy_certificate_file": str(attempt / output["hierarchy_certificate"]["file"]), "hierarchy_certificate_sha256": output["hierarchy_certificate"]["sha256"], "schema": "morsehgp3d.point_quality.hierarchy_verify_request.v2", "source_bundle_file": str(source_bundle_path), "source_bundle_sha256": source_receipt["source_bundle"]["sha256"], "source_certificate_file": str(source_path), "source_certificate_sha256": source_receipt["source_certificate"]["sha256"]}
    verification = _run_json(verifier, VERIFY_HIERARCHY_ARGUMENT, verify_request, timeouts["verifier"], "external hierarchy verification")
    validate_external_hierarchy(verification, hierarchy_sha256=output["hierarchy_certificate"]["sha256"], source_sha256=source_receipt["source_certificate"]["sha256"], source_bundle_sha256=source_receipt["source_bundle"]["sha256"], verifier_sha256=verifier_sha256)
    return {**_base_receipt(transaction), "dbscan_labels": _import_artifact(spool, attempt, output["dbscan_labels"], ordinal=ordinal, label="dbscan"), "dbscan_metrics": compute_metrics(truth_value, dbscan, point_count=transaction["point_count"]), "eom_labels": _import_artifact(spool, attempt, output["eom_labels"], ordinal=ordinal, label="eom"), "eom_metrics": compute_metrics(truth_value, eom, point_count=transaction["point_count"]), "hierarchy_certificate": _import_artifact(spool, attempt, output["hierarchy_certificate"], ordinal=ordinal, label="hierarchy"), "resources": resources, "source_run_id": transaction["source_run_id"], "verification": verification}


def _transaction_baseline(spool: CampaignSpool, attempt: Path, transaction: dict[str, Any], *, ordinal: int, generator_receipt: dict[str, Any], producer: Path, timeout: int) -> dict[str, Any]:
    cloud_path = spool.physical_root() / generator_receipt["cloud"]["file"]
    coordinate_path = spool.physical_root() / generator_receipt["coordinates"]["file"]
    truth_value = _read_spool_artifact(spool, generator_receipt["truth"], "baseline truth labels")
    invocation = {"cloud_manifest_file": str(cloud_path), "cloud_manifest_sha256": generator_receipt["cloud"]["sha256"], "coordinate_stream_file": str(coordinate_path), "coordinate_stream_sha256": generator_receipt["coordinates"]["sha256"], "coordinate_stream_size_bytes": generator_receipt["coordinates"]["size_bytes"], "output_directory": str(attempt), "raw_cloud_sha256": generator_receipt["raw_cloud_sha256"], "schema": "morsehgp3d.point_quality.baseline_invocation.v2", "transaction": transaction}
    output = _run_json(producer, BASELINE_ARGUMENT, invocation, timeout, "baseline transaction", attempt)
    allowed = {"complete"} if transaction["scale_role"] == "fifty_k" else {"complete", "not_run_resource_cap"}
    require(isinstance(output, dict) and output.get("status") in allowed, "baseline status differs")
    if output["status"] != "complete":
        require(set(output) == {"reason", "resources", "status"} and isinstance(output["reason"], str) and output["reason"], "unexecuted baseline output differs")
        receipt = _base_receipt(transaction)
        receipt.update({"reason": output["reason"], "resources": _resources(output["resources"], "baseline preflight"), "status": "not_run_resource_cap"})
        return receipt
    require(set(output) == {"dbscan_labels", "hdbscan_labels", "resources", "status"}, "baseline producer must emit labels and resources only")
    resources = _resources(output["resources"], "baseline")
    dbscan = _read_local_artifact(attempt, output["dbscan_labels"], "baseline DBSCAN labels")
    hdbscan = _read_local_artifact(attempt, output["hdbscan_labels"], "baseline HDBSCAN labels")
    validate_label_rle(dbscan, point_count=transaction["point_count"], semantics="sklearn_dbscan")
    validate_label_rle(hdbscan, point_count=transaction["point_count"], semantics="sklearn_hdbscan_eom")
    return {**_base_receipt(transaction), "dbscan_labels": _import_artifact(spool, attempt, output["dbscan_labels"], ordinal=ordinal, label="baseline-dbscan"), "dbscan_metrics": compute_metrics(truth_value, dbscan, point_count=transaction["point_count"]), "hdbscan_labels": _import_artifact(spool, attempt, output["hdbscan_labels"], ordinal=ordinal, label="baseline-hdbscan"), "hdbscan_metrics": compute_metrics(truth_value, hdbscan, point_count=transaction["point_count"]), "resources": resources}


def _validate_committed_receipt(spool: CampaignSpool, transaction: dict[str, Any], receipt: dict[str, Any], state: dict[str, dict[str, dict[str, Any]]], *, generator_sha256: str, verifier_sha256: str) -> None:
    base = _base_receipt(transaction)
    require(receipt.get("run_id") == base["run_id"] and receipt.get("role") == base["role"] and receipt.get("request_sha256") == base["request_sha256"], "committed receipt transaction binding differs")
    _resources(receipt.get("resources"), f"committed {transaction['role']}")
    role, dataset_id = transaction["role"], transaction["dataset_id"]
    if role == "generator":
        cloud = _read_spool_artifact(spool, receipt.get("cloud"), "committed cloud")
        truth = _read_spool_artifact(spool, receipt.get("truth"), "committed truth")
        validate_label_rle(truth, point_count=transaction["point_count"], semantics="ground_truth")
        require(receipt.get("generator_sha256") == generator_sha256, "committed generator executable binding differs")
        raw_cloud_sha256 = validate_materialized_cloud(cloud, root=spool.physical_root(), coordinate_reference=receipt.get("coordinates"), transaction=transaction, generator_sha256=generator_sha256, truth_sha256=receipt["truth"]["sha256"], label="committed cloud")
        require(receipt.get("raw_cloud_sha256") == raw_cloud_sha256, "committed raw cloud binding differs")
        state["generator"][dataset_id] = receipt
    elif role == "source":
        generator_receipt = state["generator"].get(dataset_id)
        require(generator_receipt is not None, "source receipt precedes generator")
        _validate_binary_reference(spool.physical_root(), receipt.get("source_bundle"), "committed source bundle")
        source = _read_spool_artifact(spool, receipt.get("source_certificate"), "committed source")
        validate_source_certificate(source, cloud_sha256=generator_receipt["raw_cloud_sha256"], cloud_manifest_sha256=generator_receipt["cloud"]["sha256"], truth_sha256=generator_receipt["truth"]["sha256"], source_bundle_sha256=receipt["source_bundle"]["sha256"], source_bundle_size_bytes=receipt["source_bundle"]["size_bytes"])
        validate_external_source(receipt.get("verification"), source_sha256=receipt["source_certificate"]["sha256"], source_bundle_sha256=receipt["source_bundle"]["sha256"], cloud_sha256=generator_receipt["raw_cloud_sha256"], cloud_manifest_sha256=generator_receipt["cloud"]["sha256"], coordinate_stream_sha256=generator_receipt["coordinates"]["sha256"], verifier_sha256=verifier_sha256)
        state["source"][dataset_id] = receipt
    elif role == "reduction":
        generator_receipt, source_receipt = state["generator"].get(dataset_id), state["source"].get(dataset_id)
        require(generator_receipt is not None and source_receipt is not None, "reduction receipt precedes source")
        source = _read_spool_artifact(spool, source_receipt["source_certificate"], "replayed source")
        hierarchy = _read_spool_artifact(spool, receipt.get("hierarchy_certificate"), "replayed hierarchy")
        coverage, _ = validate_hierarchy_certificate(hierarchy, transaction=transaction, source={**source, "certificate_sha256": source_receipt["source_certificate"]["sha256"]}, dbscan_labels_sha256=receipt["dbscan_labels"]["sha256"], eom_labels_sha256=receipt["eom_labels"]["sha256"])
        truth = _read_spool_artifact(spool, generator_receipt["truth"], "replayed truth")
        for method, labels_key, metrics_key in (("dbscan_radius", "dbscan_labels", "dbscan_metrics"), ("excess_of_mass", "eom_labels", "eom_metrics")):
            labels = _read_spool_artifact(spool, receipt.get(labels_key), f"replayed {method} labels")
            validate_selection(labels, method=method, point_count=transaction["point_count"], coverage=coverage)
            require(receipt.get(metrics_key) == compute_metrics(truth, labels, point_count=transaction["point_count"]), "committed independent metric differs")
        validate_external_hierarchy(receipt.get("verification"), hierarchy_sha256=receipt["hierarchy_certificate"]["sha256"], source_sha256=source_receipt["source_certificate"]["sha256"], source_bundle_sha256=source_receipt["source_bundle"]["sha256"], verifier_sha256=verifier_sha256)
    elif role == "baseline":
        generator_receipt = state["generator"].get(dataset_id)
        require(generator_receipt is not None, "baseline receipt precedes generator")
        if receipt.get("status") == "not_run_resource_cap":
            require(transaction["scale_role"] == "massive" and isinstance(receipt.get("reason"), str), "committed baseline omission differs")
            return
        truth = _read_spool_artifact(spool, generator_receipt["truth"], "replayed baseline truth")
        for semantics, labels_key, metrics_key in (("sklearn_dbscan", "dbscan_labels", "dbscan_metrics"), ("sklearn_hdbscan_eom", "hdbscan_labels", "hdbscan_metrics")):
            labels = _read_spool_artifact(spool, receipt.get(labels_key), f"replayed {semantics} labels")
            validate_label_rle(labels, point_count=transaction["point_count"], semantics=semantics)
            require(receipt.get(metrics_key) == compute_metrics(truth, labels, point_count=transaction["point_count"]), "committed baseline metric differs")
    else:
        fail("unknown committed transaction role")


def _transaction_timeout_budget(transaction: dict[str, Any], timeouts: dict[str, Any]) -> int:
    role = transaction["role"]
    if role == "generator":
        return timeouts["generator"][transaction["scale_role"]]
    if role == "source":
        return timeouts["source"] + timeouts["verifier"]
    if role == "reduction":
        return timeouts["reduction"] + timeouts["verifier"]
    return timeouts[role]


def execute(*, plan_path: Path, producer: Path, verifier: Path, spool_parent: Path, output: Path, stop_before_epoch: int | None = None) -> dict[str, Any]:
    plan, plan_sha256 = read_plan(plan_path)
    require(plan["campaign_entry_gate_satisfied"] is True, "quality campaign launch blocked: campaign_entry_gate_satisfied=false")
    require(stop_before_epoch is None or (type(stop_before_epoch) is int and stop_before_epoch > 0), "stop-before epoch must be a positive integer")
    for path, label in ((producer, "producer"), (verifier, "scientific verifier"), (GENERATOR_PATH, "tracked generator")):
        require(path.is_absolute(), f"{label} path must be absolute")
        require_regular_file(path, label)
    require(producer != verifier and producer.resolve(strict=True) != verifier.resolve(strict=True), "scientific verifier must be a distinct executable")
    require(output.is_absolute() and spool_parent.is_absolute(), "output and spool paths must be absolute")
    require(os.access(producer, os.X_OK) and os.access(verifier, os.X_OK), "producer and verifier must be executable")
    producer_sha, verifier_sha, generator_sha = sha256_file(producer, "producer"), sha256_file(verifier, "verifier"), sha256_file(GENERATOR_PATH, "generator")
    require(producer_sha != verifier_sha, "scientific verifier binary hash must differ from producer")
    producer_capability = _run_json(producer, PRODUCER_CAPABILITY_ARGUMENT, {}, 20, "producer capability")
    verifier_capability = _run_json(verifier, VERIFIER_CAPABILITY_ARGUMENT, {}, 20, "verifier capability")
    _validate_capability(producer_capability, kind="producer", binary_sha256=producer_sha, binary_size=producer.stat().st_size, plan_sha256=plan_sha256)
    _validate_capability(verifier_capability, kind="verifier", binary_sha256=verifier_sha, binary_size=verifier.stat().st_size, plan_sha256=plan_sha256)
    combined_capabilities = {"generator_sha256": generator_sha, "producer": producer_capability, "verifier": verifier_capability}
    capabilities_sha = sha256_bytes(canonical_bytes(combined_capabilities))
    combined_binary_sha = sha256_bytes(canonical_bytes({"generator": generator_sha, "producer": producer_sha, "verifier": verifier_sha}))
    require(producer_capability["git_sha"] == verifier_capability["git_sha"], "producer and verifier Git SHA differ")
    identity = campaign_identity(git_sha=producer_capability["git_sha"], binary_sha256=combined_binary_sha, plan_sha256=plan_sha256, capabilities_sha256=capabilities_sha)
    transactions = expected_transactions(plan)
    receipts: list[dict[str, Any]] = []
    state: dict[str, dict[str, dict[str, Any]]] = {"generator": {}, "source": {}}
    with CampaignSpool(spool_parent, identity) as spool:
        assert spool.head is not None
        require(spool.next_ordinal <= len(transactions), "spool contains too many transactions")
        for ordinal, commit in enumerate(spool.head["commits"]):
            require(ordinal < len(transactions) and commit["ordinal"] == ordinal, "spool transaction order differs")
            request, _ = read_canonical_json(spool.physical_root() / commit["request_file"], "committed quality request")
            require(request == transactions[ordinal], "spool request differs from frozen matrix")
            receipt, _ = read_canonical_json(spool.physical_root() / commit["receipt_file"], "committed quality receipt")
            _validate_committed_receipt(spool, request, receipt, state, generator_sha256=generator_sha, verifier_sha256=verifier_sha)
            _retire_committed_attempt(spool, ordinal=ordinal, request_sha256=commit["request_sha256"], receipt=receipt)
            receipts.append(receipt)
        _retire_unreferenced_archives(spool, receipts)
        stop_reason = "all_transactions_complete"
        margin = plan["transaction_store"]["graceful_stop_margin_seconds"]
        for ordinal in range(spool.next_ordinal, len(transactions)):
            transaction = transactions[ordinal]
            timeout_budget = _transaction_timeout_budget(transaction, plan["timeouts_seconds"])
            if stop_before_epoch is not None and time.time() + timeout_budget + margin > stop_before_epoch:
                stop_reason = "insufficient_time_for_next_transaction"
                break
            request_file, request_sha = spool.publish_request(ordinal, transaction)
            attempt = spool.prepare_attempt_directory(ordinal, request_sha)
            role, dataset_id = transaction["role"], transaction["dataset_id"]
            if role == "generator":
                receipt = _transaction_generator(spool, attempt, transaction, ordinal=ordinal, generator_sha256=generator_sha, timeout=plan["timeouts_seconds"]["generator"][transaction["scale_role"]])
            elif role == "source":
                receipt = _transaction_source(spool, attempt, transaction, ordinal=ordinal, generator_receipt=state["generator"][dataset_id], producer=producer, verifier=verifier, verifier_sha256=verifier_sha, timeouts=plan["timeouts_seconds"])
            elif role == "reduction":
                receipt = _transaction_reduction(spool, attempt, transaction, ordinal=ordinal, generator_receipt=state["generator"][dataset_id], source_receipt=state["source"][dataset_id], producer=producer, verifier=verifier, verifier_sha256=verifier_sha, timeouts=plan["timeouts_seconds"])
            elif role == "baseline":
                receipt = _transaction_baseline(spool, attempt, transaction, ordinal=ordinal, generator_receipt=state["generator"][dataset_id], producer=producer, timeout=plan["timeouts_seconds"]["baseline"])
            else:
                fail("unknown transaction role")
            _validate_committed_receipt(spool, transaction, receipt, state, generator_sha256=generator_sha, verifier_sha256=verifier_sha)
            spool.commit_receipt(ordinal=ordinal, request_file=request_file, request_sha256=request_sha, receipt=receipt)
            _retire_committed_attempt(spool, ordinal=ordinal, request_sha256=request_sha, receipt=receipt)
            receipts.append(receipt)
        assert spool.head is not None
        complete = len(receipts) == len(transactions)
        summary = {"campaign_id": identity["campaign_id"], "claims": plan["claims"], "completed_transaction_count": len(receipts), "exact_source_build_count": sum(receipt["role"] == "source" for receipt in receipts), "next_transaction_ordinal": len(receipts), "ordered_receipt_chain_sha256": spool.head["ordered_receipt_chain_sha256"], "plan_sha256": plan_sha256, "receipts": receipts, "schema": SUMMARY_SCHEMA, "session_stop_before_epoch": stop_before_epoch, "status": "complete" if complete else "partial_resumable", "stop_reason": "all_transactions_complete" if complete else stop_reason, "total_transaction_count": len(transactions)}
    if summary["status"] == "complete":
        require(summary["exact_source_build_count"] == SOURCE_BUILD_COUNT and summary["completed_transaction_count"] == TRANSACTION_COUNT, "campaign completion counts differ")
    require(not output.exists() and not output.is_symlink(), "quality campaign output already exists")
    output.parent.mkdir(parents=True, exist_ok=True)
    atomic_publish(output, canonical_bytes(summary))
    return summary


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    parser.add_argument("--producer", type=Path, required=True)
    parser.add_argument("--verifier", type=Path, required=True)
    parser.add_argument("--spool", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--stop-before-epoch", type=int)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    options = parse_arguments(arguments)
    execute(plan_path=options.plan, producer=options.producer, verifier=options.verifier, spool_parent=options.spool, output=options.output, stop_before_epoch=options.stop_before_epoch)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ContractError, RuntimeContractError, OSError) as error:
        print(f"point-hierarchy quality campaign rejected: {error}", file=os.sys.stderr)
        raise SystemExit(1) from error
