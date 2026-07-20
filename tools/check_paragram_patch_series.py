#!/usr/bin/env python3
"""Validate and quarantine-apply the pinned Paragram patch series offline.

The source checkout is never modified.  Its phase 7.1 pin is replayed first,
then the complete patch series is applied to a temporary Git repository whose
object database uses the source checkout only as a local alternate.  No patched
tree escapes that temporary quarantine.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import os
import re
import subprocess
import sys
import tempfile
import tomllib
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
SOURCE_PIN_CHECKER = ROOT / "tools" / "check_paragram_source_pin.py"
SERIES_ROOT = ROOT / "third_party" / "paragram"
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
PATCH_ID = re.compile(r"^[a-z0-9][a-z0-9_]*$")
REGULAR_MODES = {"100644", "100755"}
ZERO_OBJECT = "0" * 40

SERIES_KEYS = {
    "schema_version",
    "source_pin_manifest",
    "repository_url",
    "base_commit",
    "base_tree",
    "final_tree",
    "complete_series_required",
    "public_status",
    "cuda_compile_status",
    "patches",
}
PATCH_KEYS = {
    "order",
    "id",
    "path",
    "bytes",
    "sha256",
    "pre_tree",
    "post_tree",
    "touched_paths",
    "added_paths",
    "modified_paths",
    "deleted_paths",
    "semantic_contract",
    "limitations",
}
PATCH_CONTRACTS = {
    "fail_closed_bvh_stack_and_adjacency_v1": (
        (
            "historical_status_codes_0_through_5_unchanged_and_"
            "traversal_stack_overflow_is_6",
            "weighted_and_unweighted_status_abi_is_one_shared_int32_enum",
            "bounded_box_traversals_return_a_typed_outcome",
            "every_full_stack_push_returns_stack_overflow_without_eviction_or_"
            "replacement",
            "every_non_strictly_excluded_far_branch_is_retained_including_zero_"
            "margin_tangencies",
            "a_preexisting_cell_error_wins_else_stack_overflow_maps_to_status_6",
            "nonzero_status_cells_write_no_raw_adjacency",
            "csr_gather_keeps_an_edge_only_when_both_endpoint_statuses_are_success",
        ),
        (
            "not_compiled_with_nvcc_or_pytorch_in_this_milestone",
            "upstream_default_stream_usage_and_asynchronous_cuda_fault_"
            "propagation_are_not_patched_or_validated",
            "no_explicit_caller_box_or_geometry_export_yet",
            "status_zero_remains_a_floating_proposal_not_a_certificate",
        ),
    ),
    "explicit_closed_aabb_f32_v1": (
        (
            "python_bounds_is_required_keyword_only_cpu_float32_shape_2x3_finite_"
            "and_strictly_ordered",
            "python_validation_preserves_the_six_binary32_words_and_passes_bounds_"
            "to_both_bindings",
            "cpp_revalidates_bounds_before_any_gpu_tensor_contiguity_allocation_or_"
            "launch",
            "weighted_and_unweighted_world_bounds_are_built_only_from_the_six_"
            "validated_caller_values",
            "convex_cell_box_planes_use_exact_caller_coordinates_without_epsilon_"
            "or_padding",
            "initial_box_radii_are_derived_from_the_initialized_cell_bounds_in_"
            "both_kernels",
            "no_implicit_point_derived_or_fixed_radius_box_fallback_remains",
        ),
        (
            "not_compiled_with_nvcc_or_pytorch_cuda_in_this_milestone",
            "input_sites_are_not_required_or_checked_to_lie_inside_the_explicit_box",
            "upstream_default_stream_usage_and_asynchronous_cuda_fault_"
            "propagation_remain_unpatched",
            "no_geometry_plane_vertex_or_incidence_export_yet",
            "status_zero_remains_a_floating_proposal_not_a_certificate",
            "box_radius_rounding_is_not_yet_a_certified_outward_bound",
            "finite_extreme_bounds_are_ingress_valid_only_and_not_gpu_qualified",
        ),
    ),
}
STATUS_VALUES = (
    ("success", 0),
    ("plane_overflow", 1),
    ("vertex_overflow", 2),
    ("empty_cell", 3),
    ("security_radius_not_reached", 4),
    ("inconsistent_boundary", 5),
    ("traversal_stack_overflow", 6),
    ("max_status_num", 7),
)
TRAVERSAL_OUTCOMES = (
    "complete",
    "empty_bvh",
    "cell_aborted",
    "stack_overflow",
)


class ValidationFailure(RuntimeError):
    """The series cannot be validated without weakening the contract."""


@dataclass(frozen=True)
class TreeEntry:
    mode: str
    kind: str
    object_id: str


@dataclass(frozen=True)
class RawPatchEntry:
    path: str
    action: str
    old_object: str
    new_object: str
    mode: str


@dataclass(frozen=True)
class PatchSpec:
    order: int
    identifier: str
    path: Path
    relative_path: str
    byte_count: int
    digest: str
    pre_tree: str
    post_tree: str
    touched_paths: tuple[str, ...]
    added_paths: tuple[str, ...]
    modified_paths: tuple[str, ...]
    deleted_paths: tuple[str, ...]
    raw_entries: tuple[RawPatchEntry, ...]


@dataclass(frozen=True)
class SeriesSpec:
    path: Path
    source_pin_path: Path
    repository_url: str
    base_commit: str
    base_tree: str
    final_tree: str
    patches: tuple[PatchSpec, ...]


def offline_environment() -> dict[str, str]:
    environment = os.environ.copy()
    redirected = {
        "GIT_ALTERNATE_OBJECT_DIRECTORIES",
        "GIT_COMMON_DIR",
        "GIT_CONFIG_COUNT",
        "GIT_CONFIG_PARAMETERS",
        "GIT_DIR",
        "GIT_EXEC_PATH",
        "GIT_INDEX_FILE",
        "GIT_NAMESPACE",
        "GIT_OBJECT_DIRECTORY",
        "GIT_PREFIX",
        "GIT_TEMPLATE_DIR",
        "GIT_WORK_TREE",
    }
    for name in tuple(environment):
        if (
            name in redirected
            or name.startswith("GIT_CONFIG_KEY_")
            or name.startswith("GIT_CONFIG_VALUE_")
        ):
            environment.pop(name, None)
    environment.update(
        {
            "GIT_ATTR_NOSYSTEM": "1",
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_CONFIG_GLOBAL": os.devnull,
            "GIT_CONFIG_SYSTEM": os.devnull,
            "GIT_NO_LAZY_FETCH": "1",
            "GIT_NO_REPLACE_OBJECTS": "1",
            "GIT_PROTOCOL_FROM_USER": "0",
            "GIT_TERMINAL_PROMPT": "0",
            "LC_ALL": "C",
        }
    )
    return environment


def run(
    command: list[str],
    *,
    cwd: Path | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess[bytes]:
    try:
        result = subprocess.run(
            command,
            cwd=cwd,
            env=offline_environment(),
            check=False,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except OSError as exc:
        raise ValidationFailure(f"cannot execute {command[0]}: {exc}") from exc
    if check and result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise ValidationFailure(
            f"command failed ({' '.join(command)}): {detail or 'unknown error'}"
        )
    return result


def git(repo: Path, *arguments: str) -> bytes:
    return run(["git", "-C", str(repo), *arguments]).stdout


def git_text(repo: Path, *arguments: str) -> str:
    try:
        return git(repo, *arguments).decode("utf-8", errors="strict").strip()
    except UnicodeDecodeError as exc:
        raise ValidationFailure("Git returned non-UTF-8 metadata") from exc


def sha256(path: Path) -> str:
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


def normalized_relative_path(
    value: Any,
    label: str,
    errors: list[str],
) -> str | None:
    if not isinstance(value, str) or not value:
        errors.append(f"{label} must be a non-empty string")
        return None
    candidate = PurePosixPath(value)
    if (
        "\\" in value
        or candidate.is_absolute()
        or any(part in {"", ".", ".."} for part in candidate.parts)
        or candidate.as_posix() != value
    ):
        errors.append(f"{label} must be a normalized relative POSIX path: {value!r}")
        return None
    return value


def root_path(value: Any, label: str, errors: list[str]) -> tuple[str, Path] | None:
    relative = normalized_relative_path(value, label, errors)
    if relative is None:
        return None
    candidate = ROOT.joinpath(*PurePosixPath(relative).parts)
    try:
        candidate.resolve().relative_to(ROOT.resolve())
    except (OSError, ValueError):
        errors.append(f"{label} escapes the repository root: {relative}")
        return None
    return relative, candidate


def exact_keys(
    table: dict[str, Any],
    expected: set[str],
    label: str,
    errors: list[str],
) -> None:
    missing = sorted(expected - table.keys())
    unknown = sorted(table.keys() - expected)
    if missing:
        errors.append(f"{label} misses keys: {', '.join(missing)}")
    if unknown:
        errors.append(f"{label} has unknown keys: {', '.join(unknown)}")


def hex40(value: Any, label: str, errors: list[str]) -> str | None:
    if not isinstance(value, str) or not HEX40.fullmatch(value):
        errors.append(f"{label} must be 40 lowercase hexadecimal digits")
        return None
    return value


def string_list(
    value: Any,
    label: str,
    errors: list[str],
    *,
    exact: tuple[str, ...] | None = None,
) -> tuple[str, ...]:
    if not isinstance(value, list) or not all(
        isinstance(item, str) and item for item in value
    ):
        errors.append(f"{label} must be a non-empty-string array")
        return ()
    result = tuple(value)
    if len(set(result)) != len(result):
        errors.append(f"{label} contains duplicates")
    if exact is not None and result != exact:
        errors.append(f"{label} does not equal the normative ordered contract")
    return result


def path_list(value: Any, label: str, errors: list[str]) -> tuple[str, ...]:
    raw = string_list(value, label, errors)
    result: list[str] = []
    for index, item in enumerate(raw):
        path = normalized_relative_path(item, f"{label}[{index}]", errors)
        if path is not None:
            result.append(path)
    if tuple(result) != tuple(sorted(result)):
        errors.append(f"{label} must be strictly lexicographically sorted")
    return tuple(result)


def strict_raw_patch_path(value: str, label: str) -> str:
    errors: list[str] = []
    result = normalized_relative_path(value, label, errors)
    if result is None or errors:
        raise ValidationFailure("; ".join(errors))
    return result


def parse_raw_patch(
    path: Path,
    *,
    touched_paths: tuple[str, ...],
    added_paths: tuple[str, ...],
    modified_paths: tuple[str, ...],
    deleted_paths: tuple[str, ...],
) -> tuple[RawPatchEntry, ...]:
    try:
        raw = path.read_bytes()
    except OSError as exc:
        raise ValidationFailure(f"cannot read raw patch {path}: {exc}") from exc
    if b"\0" in raw:
        raise ValidationFailure(f"raw patch contains a NUL byte: {path}")
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise ValidationFailure(f"raw patch is not UTF-8: {path}") from exc
    if not text.endswith("\n"):
        raise ValidationFailure(f"raw patch must end with one LF: {path}")

    forbidden_markers = (
        "GIT binary patch",
        "Binary files ",
        "copy from ",
        "copy to ",
        "dissimilarity index ",
        "old mode ",
        "new mode ",
        "rename from ",
        "rename to ",
        "similarity index ",
    )
    for line in text.splitlines():
        if any(line.startswith(marker) for marker in forbidden_markers):
            raise ValidationFailure(
                f"raw patch uses a forbidden binary/copy/rename/mode header: {line}"
            )

    lines = text.splitlines()
    diff_starts = [
        index for index, line in enumerate(lines) if line.startswith("diff --git ")
    ]
    if not diff_starts or diff_starts[0] != 0:
        raise ValidationFailure("raw patch must start with a diff --git header")
    sections = [
        lines[start:end]
        for start, end in zip(diff_starts, (*diff_starts[1:], len(lines)))
    ]

    entries: list[RawPatchEntry] = []
    observed_paths: set[str] = set()
    for section_index, section in enumerate(sections, start=1):
        header_match = re.fullmatch(
            r"diff --git a/([^ ]+) b/([^ ]+)",
            section[0],
        )
        if header_match is None:
            raise ValidationFailure(
                f"raw patch diff #{section_index} has a quoted or malformed path header"
            )
        old_path = strict_raw_patch_path(
            header_match.group(1),
            f"raw patch diff #{section_index} old path",
        )
        new_path = strict_raw_patch_path(
            header_match.group(2),
            f"raw patch diff #{section_index} new path",
        )
        if old_path != new_path:
            raise ValidationFailure(
                f"raw patch diff #{section_index} changes path {old_path!r} to {new_path!r}"
            )
        if old_path in observed_paths:
            raise ValidationFailure(f"raw patch has multiple diffs for path: {old_path}")
        observed_paths.add(old_path)

        hunk_starts = [
            index for index, line in enumerate(section) if line.startswith("@@ ")
        ]
        if not hunk_starts:
            raise ValidationFailure(f"raw patch diff for {old_path} has no text hunk")
        metadata = section[1 : hunk_starts[0]]
        index_lines = [line for line in metadata if line.startswith("index ")]
        if len(index_lines) != 1:
            raise ValidationFailure(
                f"raw patch diff for {old_path} must have exactly one index header"
            )
        index_match = re.fullmatch(
            r"index ([0-9a-f]{40})\.\.([0-9a-f]{40})(?: ([0-7]{6}))?",
            index_lines[0],
        )
        if index_match is None:
            raise ValidationFailure(f"raw patch diff for {old_path} has invalid object IDs")
        old_object, new_object, index_mode = index_match.groups()

        new_modes = [line for line in metadata if line.startswith("new file mode ")]
        deleted_modes = [
            line for line in metadata if line.startswith("deleted file mode ")
        ]
        if len(new_modes) > 1 or len(deleted_modes) > 1 or (new_modes and deleted_modes):
            raise ValidationFailure(f"raw patch diff for {old_path} has conflicting actions")
        if new_modes:
            if new_modes != ["new file mode 100644"]:
                raise ValidationFailure(
                    f"raw patch adds {old_path} with a non-100644 mode"
                )
            action = "added"
            mode = "100644"
            expected_old_header = "--- /dev/null"
            expected_new_header = f"+++ b/{old_path}"
            if old_object != ZERO_OBJECT or new_object == ZERO_OBJECT or index_mode is not None:
                raise ValidationFailure(
                    f"raw added-file index header is inconsistent for {old_path}"
                )
        elif deleted_modes:
            deleted_match = re.fullmatch(r"deleted file mode ([0-7]{6})", deleted_modes[0])
            if deleted_match is None or deleted_match.group(1) not in REGULAR_MODES:
                raise ValidationFailure(
                    f"raw patch deletes {old_path} with a non-regular base mode"
                )
            action = "deleted"
            mode = deleted_match.group(1)
            expected_old_header = f"--- a/{old_path}"
            expected_new_header = "+++ /dev/null"
            if old_object == ZERO_OBJECT or new_object != ZERO_OBJECT or index_mode is not None:
                raise ValidationFailure(
                    f"raw deleted-file index header is inconsistent for {old_path}"
                )
        else:
            action = "modified"
            mode = index_mode or ""
            expected_old_header = f"--- a/{old_path}"
            expected_new_header = f"+++ b/{old_path}"
            if (
                old_object == ZERO_OBJECT
                or new_object == ZERO_OBJECT
                or mode not in REGULAR_MODES
            ):
                raise ValidationFailure(
                    f"raw modified-file index header is inconsistent for {old_path}"
                )

        old_headers = [line for line in metadata if line.startswith("--- ")]
        new_headers = [line for line in metadata if line.startswith("+++ ")]
        if old_headers != [expected_old_header] or new_headers != [expected_new_header]:
            raise ValidationFailure(
                f"raw patch /dev/null and path headers disagree for {old_path}"
            )
        allowed_metadata = {
            index_lines[0],
            expected_old_header,
            expected_new_header,
            *new_modes,
            *deleted_modes,
        }
        unexpected = [line for line in metadata if line not in allowed_metadata]
        if unexpected:
            raise ValidationFailure(
                f"raw patch has unsupported metadata for {old_path}: {unexpected!r}"
            )
        entries.append(
            RawPatchEntry(
                path=old_path,
                action=action,
                old_object=old_object,
                new_object=new_object,
                mode=mode,
            )
        )

    entries.sort(key=lambda entry: entry.path)
    observed_by_action = {
        "added": tuple(entry.path for entry in entries if entry.action == "added"),
        "modified": tuple(entry.path for entry in entries if entry.action == "modified"),
        "deleted": tuple(entry.path for entry in entries if entry.action == "deleted"),
    }
    expected_by_action = {
        "added": added_paths,
        "modified": modified_paths,
        "deleted": deleted_paths,
    }
    if tuple(entry.path for entry in entries) != touched_paths:
        raise ValidationFailure(
            "raw diff paths do not equal the manifest touched_paths inventory"
        )
    for action, expected in expected_by_action.items():
        if observed_by_action[action] != expected:
            raise ValidationFailure(
                f"raw {action} paths differ from the manifest: "
                f"expected {expected!r}, got {observed_by_action[action]!r}"
            )
    return tuple(entries)


def resolve_series_path(raw_path: Path | None) -> Path:
    if raw_path is not None:
        return raw_path.resolve()
    candidates = sorted(SERIES_ROOT.glob("*/series.toml"))
    if len(candidates) != 1:
        raise ValidationFailure(
            "default series resolution requires exactly one "
            f"third_party/paragram/*/series.toml, found {len(candidates)}"
        )
    return candidates[0].resolve()


def load_toml(path: Path, label: str) -> dict[str, Any]:
    try:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        raise ValidationFailure(f"{label} is unreadable: {exc}") from exc
    if not isinstance(data, dict):
        raise ValidationFailure(f"{label} must contain a TOML table")
    return data


def parse_series(series_path: Path) -> SeriesSpec:
    errors: list[str] = []
    try:
        relative_series = series_path.relative_to(ROOT.resolve())
    except ValueError:
        raise ValidationFailure("series.toml must be inside the E-HGP repository")
    if series_path.is_symlink() or not series_path.is_file():
        raise ValidationFailure(f"series manifest is missing or is a symlink: {series_path}")

    data = load_toml(series_path, "patch-series manifest")
    exact_keys(data, SERIES_KEYS, "series", errors)
    if data.get("schema_version") != 1:
        errors.append("series.schema_version must be 1")
    if data.get("complete_series_required") is not True:
        errors.append("series.complete_series_required must be true")
    if data.get("public_status") != "proposal_only":
        errors.append("series.public_status must be proposal_only")
    if data.get("cuda_compile_status") != "not_run":
        errors.append("series.cuda_compile_status must be not_run")

    repository_url = data.get("repository_url")
    if not isinstance(repository_url, str) or not repository_url:
        errors.append("series.repository_url must be a non-empty string")
        repository_url = ""
    base_commit = hex40(data.get("base_commit"), "series.base_commit", errors) or ""
    base_tree = hex40(data.get("base_tree"), "series.base_tree", errors) or ""
    final_tree = hex40(data.get("final_tree"), "series.final_tree", errors) or ""

    source_pin_result = root_path(
        data.get("source_pin_manifest"),
        "series.source_pin_manifest",
        errors,
    )
    source_pin_path = source_pin_result[1] if source_pin_result else ROOT

    path_parts = relative_series.parts
    if (
        len(path_parts) != 4
        or path_parts[:2] != ("third_party", "paragram")
        or path_parts[2] != base_commit
        or path_parts[3] != "series.toml"
    ):
        errors.append(
            "series manifest must be "
            "third_party/paragram/<base_commit>/series.toml"
        )

    raw_patches = data.get("patches")
    if not isinstance(raw_patches, list) or not raw_patches:
        errors.append("series.patches must contain at least one table")
        raw_patches = []

    patches: list[PatchSpec] = []
    identifiers: set[str] = set()
    declared_patch_paths: set[Path] = set()
    for index, raw_patch in enumerate(raw_patches, start=1):
        label = f"patch #{index}"
        if not isinstance(raw_patch, dict):
            errors.append(f"{label} must be a TOML table")
            continue
        exact_keys(raw_patch, PATCH_KEYS, label, errors)

        order = raw_patch.get("order")
        if not isinstance(order, int) or isinstance(order, bool) or order != index:
            errors.append(f"{label}.order must be the contiguous value {index}")
            order = index
        identifier = raw_patch.get("id")
        if not isinstance(identifier, str) or not PATCH_ID.fullmatch(identifier):
            errors.append(f"{label}.id must match {PATCH_ID.pattern}")
            identifier = f"invalid_{index}"
        if identifier in identifiers:
            errors.append(f"duplicate patch id: {identifier}")
        identifiers.add(identifier)

        patch_path_result = root_path(raw_patch.get("path"), f"{label}.path", errors)
        if patch_path_result is None:
            relative_patch_path = ""
            patch_path = ROOT
        else:
            relative_patch_path, patch_path = patch_path_result
            if patch_path.parent.resolve() != series_path.parent.resolve():
                errors.append(f"{label}.path must be adjacent to series.toml")
            expected_prefix = f"{index:04d}-"
            if not patch_path.name.startswith(expected_prefix) or patch_path.suffix != ".patch":
                errors.append(
                    f"{label}.path basename must start with {expected_prefix!r} "
                    "and end in .patch"
                )
            if patch_path in declared_patch_paths:
                errors.append(f"duplicate patch path: {relative_patch_path}")
            declared_patch_paths.add(patch_path)

        byte_count = raw_patch.get("bytes")
        if (
            not isinstance(byte_count, int)
            or isinstance(byte_count, bool)
            or byte_count <= 0
        ):
            errors.append(f"{label}.bytes must be a positive integer")
            byte_count = 0
        digest = raw_patch.get("sha256")
        if not isinstance(digest, str) or not HEX64.fullmatch(digest):
            errors.append(f"{label}.sha256 must be 64 lowercase hexadecimal digits")
            digest = ""
        pre_tree = hex40(raw_patch.get("pre_tree"), f"{label}.pre_tree", errors) or ""
        post_tree = hex40(raw_patch.get("post_tree"), f"{label}.post_tree", errors) or ""

        touched = path_list(raw_patch.get("touched_paths"), f"{label}.touched_paths", errors)
        added = path_list(raw_patch.get("added_paths"), f"{label}.added_paths", errors)
        modified = path_list(raw_patch.get("modified_paths"), f"{label}.modified_paths", errors)
        deleted = path_list(raw_patch.get("deleted_paths"), f"{label}.deleted_paths", errors)
        action_sets = (set(added), set(modified), set(deleted))
        if any(
            action_sets[left] & action_sets[right]
            for left in range(3)
            for right in range(left + 1, 3)
        ):
            errors.append(f"{label} action path arrays must be pairwise disjoint")
        if set(touched) != set().union(*action_sets):
            errors.append(
                f"{label}.touched_paths must equal added + modified + deleted paths"
            )

        expected_contract = PATCH_CONTRACTS.get(identifier)
        if expected_contract is None:
            errors.append(f"{label}.id has no registered exact contract: {identifier}")
            expected_semantics = None
            expected_limitations = None
        else:
            expected_semantics, expected_limitations = expected_contract
        string_list(
            raw_patch.get("semantic_contract"),
            f"{label}.semantic_contract",
            errors,
            exact=expected_semantics,
        )
        string_list(
            raw_patch.get("limitations"),
            f"{label}.limitations",
            errors,
            exact=expected_limitations,
        )

        raw_entries: tuple[RawPatchEntry, ...] = ()
        if patch_path != ROOT:
            if patch_path.is_symlink() or not patch_path.is_file():
                errors.append(f"{label}.path is missing or is a symlink")
            else:
                actual_bytes = patch_path.stat().st_size
                if actual_bytes != byte_count:
                    errors.append(
                        f"{label} byte-size mismatch: expected {byte_count}, "
                        f"got {actual_bytes}"
                    )
                actual_digest = sha256(patch_path)
                if actual_digest != digest:
                    errors.append(
                        f"{label} SHA-256 mismatch: expected {digest}, "
                        f"got {actual_digest}"
                    )
                try:
                    raw_entries = parse_raw_patch(
                        patch_path,
                        touched_paths=touched,
                        added_paths=added,
                        modified_paths=modified,
                        deleted_paths=deleted,
                    )
                except ValidationFailure as exc:
                    errors.append(f"{label} raw syntax: {exc}")

        patches.append(
            PatchSpec(
                order=order,
                identifier=identifier,
                path=patch_path,
                relative_path=relative_patch_path,
                byte_count=byte_count,
                digest=digest,
                pre_tree=pre_tree,
                post_tree=post_tree,
                touched_paths=touched,
                added_paths=added,
                modified_paths=modified,
                deleted_paths=deleted,
                raw_entries=raw_entries,
            )
        )

    actual_patch_paths = {
        path.resolve()
        for path in series_path.parent.glob("*.patch")
        if path.is_file() or path.is_symlink()
    }
    resolved_declared = {path.resolve() for path in declared_patch_paths}
    for undeclared in sorted(actual_patch_paths - resolved_declared):
        errors.append(f"undeclared patch file: {undeclared.relative_to(ROOT)}")
    for missing in sorted(resolved_declared - actual_patch_paths):
        errors.append(f"declared patch absent from directory inventory: {missing}")

    expected_patch_ids = tuple(PATCH_CONTRACTS)
    observed_patch_ids = tuple(patch.identifier for patch in patches)
    if observed_patch_ids != expected_patch_ids:
        errors.append(
            "series patch ids/order must be exactly "
            f"{expected_patch_ids!r}, got {observed_patch_ids!r}"
        )

    if patches:
        if patches[0].pre_tree != base_tree:
            errors.append("first patch pre_tree must equal series.base_tree")
        for previous, current in zip(patches, patches[1:]):
            if current.pre_tree != previous.post_tree:
                errors.append(
                    f"patch #{current.order}.pre_tree must equal "
                    f"patch #{previous.order}.post_tree"
                )
        if patches[-1].post_tree != final_tree:
            errors.append("last patch post_tree must equal series.final_tree")

    if errors:
        raise ValidationFailure("invalid patch-series manifest:\n- " + "\n- ".join(errors))
    return SeriesSpec(
        path=series_path,
        source_pin_path=source_pin_path,
        repository_url=repository_url,
        base_commit=base_commit,
        base_tree=base_tree,
        final_tree=final_tree,
        patches=tuple(patches),
    )


def validate_source_pin(checkout: Path, series: SeriesSpec) -> dict[str, Any]:
    if not SOURCE_PIN_CHECKER.is_file():
        raise ValidationFailure(f"source-pin checker is missing: {SOURCE_PIN_CHECKER}")
    result = run(
        [
            sys.executable,
            "-I",
            str(SOURCE_PIN_CHECKER),
            str(checkout),
            "--manifest",
            str(series.source_pin_path),
        ],
        check=False,
    )
    if result.returncode != 0:
        detail = (result.stderr + result.stdout).decode("utf-8", errors="replace").strip()
        raise ValidationFailure(
            "phase 7.1 source-pin replay failed: " + (detail or "unknown error")
        )

    pin = load_toml(series.source_pin_path, "source-pin manifest")
    upstream = pin.get("upstream")
    if not isinstance(upstream, dict):
        raise ValidationFailure("source-pin manifest lacks [upstream]")
    comparisons = {
        "repository_url": (series.repository_url, upstream.get("repository_url")),
        "base_commit": (series.base_commit, upstream.get("commit")),
        "base_tree": (series.base_tree, upstream.get("tree")),
    }
    mismatches = [
        f"{name}: series={observed!r}, source pin={expected!r}"
        for name, (observed, expected) in comparisons.items()
        if observed != expected
    ]
    if mismatches:
        raise ValidationFailure("series/source-pin mismatch:\n- " + "\n- ".join(mismatches))
    return pin


def tree_entries(repo: Path, treeish: str) -> dict[str, TreeEntry]:
    raw = git(repo, "ls-tree", "-r", "-z", treeish)
    entries: dict[str, TreeEntry] = {}
    for record in raw.split(b"\0"):
        if not record:
            continue
        try:
            metadata, encoded_path = record.split(b"\t", maxsplit=1)
            mode, kind, object_id = metadata.decode("ascii").split(" ")
            path = encoded_path.decode("utf-8", errors="strict")
        except (UnicodeDecodeError, ValueError) as exc:
            raise ValidationFailure(f"cannot parse ls-tree record: {record!r}") from exc
        if path in entries:
            raise ValidationFailure(f"duplicate path in tree {treeish}: {path}")
        entries[path] = TreeEntry(mode=mode, kind=kind, object_id=object_id)
    return entries


def tree_actions(
    before: dict[str, TreeEntry],
    after: dict[str, TreeEntry],
) -> tuple[tuple[str, ...], tuple[str, ...], tuple[str, ...], tuple[str, ...]]:
    before_paths = set(before)
    after_paths = set(after)
    added = tuple(sorted(after_paths - before_paths))
    deleted = tuple(sorted(before_paths - after_paths))
    modified = tuple(
        sorted(
            path
            for path in before_paths & after_paths
            if before[path] != after[path]
        )
    )
    touched = tuple(sorted((*added, *modified, *deleted)))
    return touched, added, modified, deleted


def validate_raw_entries_against_trees(
    patch: PatchSpec,
    before: dict[str, TreeEntry],
    after: dict[str, TreeEntry] | None = None,
) -> None:
    for raw in patch.raw_entries:
        previous = before.get(raw.path)
        if raw.action == "added":
            if previous is not None:
                raise ValidationFailure(
                    f"patch #{patch.order} raw added path already exists: {raw.path}"
                )
        else:
            if (
                previous is None
                or previous.kind != "blob"
                or previous.mode != raw.mode
                or previous.object_id != raw.old_object
            ):
                raise ValidationFailure(
                    f"patch #{patch.order} raw base blob/mode mismatch for {raw.path}"
                )
        if after is None:
            continue
        following = after.get(raw.path)
        if raw.action == "deleted":
            if following is not None:
                raise ValidationFailure(
                    f"patch #{patch.order} raw deleted path survives: {raw.path}"
                )
            continue
        if (
            following is None
            or following.kind != "blob"
            or following.mode != raw.mode
            or following.object_id != raw.new_object
        ):
            raise ValidationFailure(
                f"patch #{patch.order} raw result blob/mode mismatch for {raw.path}"
            )


def pin_protected_paths(pin: dict[str, Any]) -> tuple[str, tuple[str, ...]]:
    upstream = pin.get("upstream", {})
    license_path = upstream.get("license_path")
    if not isinstance(license_path, str):
        raise ValidationFailure("source pin has no valid upstream.license_path")
    raw_submodules = pin.get("submodule")
    if not isinstance(raw_submodules, list) or not raw_submodules:
        raise ValidationFailure("source pin has no [[submodule]] entries")
    submodule_paths: list[str] = []
    for entry in raw_submodules:
        if not isinstance(entry, dict) or not isinstance(entry.get("path"), str):
            raise ValidationFailure("source pin contains an invalid submodule entry")
        submodule_paths.append(entry["path"])
    return license_path, tuple(sorted(submodule_paths))


def prepare_quarantine(checkout: Path, quarantine: Path, base_commit: str) -> None:
    run(["git", "init", "--quiet", str(quarantine)])
    object_directory = Path(
        git_text(
            checkout,
            "rev-parse",
            "--path-format=absolute",
            "--git-path",
            "objects",
        )
    ).resolve()
    if not object_directory.is_dir():
        raise ValidationFailure(
            f"source Git object directory is unavailable: {object_directory}"
        )
    alternates = quarantine / ".git" / "objects" / "info" / "alternates"
    alternates.parent.mkdir(parents=True, exist_ok=True)
    alternates.write_text(f"{object_directory}\n", encoding="utf-8")
    git(quarantine, "cat-file", "-e", f"{base_commit}^{{commit}}")
    git(quarantine, "checkout", "--quiet", "--detach", "--force", base_commit)


def strip_cpp_comments(text: str) -> str:
    output: list[str] = []
    index = 0
    state = "code"
    quote = ""
    while index < len(text):
        char = text[index]
        next_char = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and next_char == "/":
                state = "line_comment"
                index += 2
                continue
            if char == "/" and next_char == "*":
                state = "block_comment"
                index += 2
                continue
            if char in {'"', "'"}:
                state = "quoted"
                quote = char
            output.append(char)
            index += 1
            continue
        if state == "line_comment":
            if char == "\n":
                output.append("\n")
                state = "code"
            index += 1
            continue
        if state == "block_comment":
            if char == "*" and next_char == "/":
                state = "code"
                index += 2
            else:
                if char == "\n":
                    output.append("\n")
                index += 1
            continue
        output.append(char)
        if char == "\\" and index + 1 < len(text):
            output.append(text[index + 1])
            index += 2
            continue
        if char == quote:
            state = "code"
        index += 1
    if state in {"block_comment", "quoted"}:
        raise ValidationFailure("unterminated comment or literal in patched CUDA source")
    return "".join(output)


def compact_cpp(text: str) -> str:
    return re.sub(r"\s+", "", strip_cpp_comments(text))


def brace_block(text: str, opening: int) -> tuple[str, int]:
    if opening >= len(text) or text[opening] != "{":
        raise ValidationFailure("internal semantic parser did not start on an opening brace")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[opening : index + 1], index + 1
    raise ValidationFailure("unbalanced braces in patched CUDA source")


def named_function_body(compact: str, function_name: str) -> str:
    marker = f"{function_name}("
    start = compact.find(marker)
    if start < 0:
        raise ValidationFailure(f"patched source lacks function {function_name}")
    opening = compact.find("{", start + len(marker))
    if opening < 0:
        raise ValidationFailure(f"patched function {function_name} has no body")
    return brace_block(compact, opening)[0]


def named_if_bodies(compact: str, condition: str) -> tuple[str, ...]:
    marker = f"if({condition})"
    bodies: list[str] = []
    cursor = 0
    while True:
        start = compact.find(marker, cursor)
        if start < 0:
            return tuple(bodies)
        opening = compact.find("{", start + len(marker))
        if opening < 0:
            raise ValidationFailure(f"guarded condition {marker} has no braced body")
        body, end = brace_block(compact, opening)
        bodies.append(body)
        cursor = end


def enum_names(compact: str, declaration: str) -> tuple[str, ...]:
    marker = declaration + "{"
    start = compact.find(marker)
    if start < 0:
        raise ValidationFailure(f"patched source lacks enum {declaration}")
    body, _ = brace_block(compact, start + len(declaration))
    return tuple(item for item in body[1:-1].split(",") if item)


def read_utf8(quarantine: Path, relative: str) -> str:
    path = quarantine.joinpath(*PurePosixPath(relative).parts)
    if path.is_symlink() or not path.is_file():
        raise ValidationFailure(f"patched semantic source is missing: {relative}")
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        raise ValidationFailure(f"cannot read patched semantic source {relative}: {exc}") from exc


def read_cpp(quarantine: Path, relative: str) -> str:
    return read_utf8(quarantine, relative)


def python_function(module: ast.Module, name: str, relative: str) -> ast.FunctionDef:
    matches = [
        node
        for node in module.body
        if isinstance(node, ast.FunctionDef) and node.name == name
    ]
    if len(matches) != 1:
        raise ValidationFailure(
            f"{relative} must define exactly one Python function {name}, "
            f"got {len(matches)}"
        )
    return matches[0]


def parsed_python(quarantine: Path, relative: str) -> tuple[str, ast.Module]:
    source = read_utf8(quarantine, relative)
    try:
        return source, ast.parse(source, filename=relative)
    except SyntaxError as exc:
        raise ValidationFailure(f"patched Python source is invalid: {relative}: {exc}") from exc


def python_call_name(call: ast.Call) -> str | None:
    if isinstance(call.func, ast.Name):
        return call.func.id
    if isinstance(call.func, ast.Attribute):
        return call.func.attr
    return None


def validate_status_abi(quarantine: Path) -> None:
    status_path = "paragram/csrc/laguerre/status.cuh"
    status_compact = compact_cpp(read_cpp(quarantine, status_path))
    raw_values = enum_names(status_compact, "enumStatus:int")
    observed: list[tuple[str, int]] = []
    for item in raw_values:
        match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)=([0-9]+)", item)
        if match is None:
            raise ValidationFailure(f"status enum contains a non-explicit entry: {item}")
        observed.append((match.group(1), int(match.group(2))))
    if tuple(observed) != STATUS_VALUES:
        raise ValidationFailure(
            f"status ABI mismatch: expected {STATUS_VALUES!r}, got {tuple(observed)!r}"
        )
    if "static_assert(sizeof(Status)==sizeof(int));" not in status_compact:
        raise ValidationFailure("status ABI lacks sizeof(Status)==sizeof(int) assertion")

    laguerre_dir = quarantine / "paragram" / "csrc" / "laguerre"
    enum_owners: list[str] = []
    for path in sorted(laguerre_dir.iterdir()):
        if path.suffix not in {".h", ".cuh", ".cu", ".cpp"} or not path.is_file():
            continue
        if re.search(r"\benum\s+Status\b", strip_cpp_comments(path.read_text(encoding="utf-8"))):
            enum_owners.append(path.name)
    if enum_owners != ["status.cuh"]:
        raise ValidationFailure(
            f"Status must have one shared definition in status.cuh, got {enum_owners!r}"
        )

    for relative in (
        "paragram/csrc/laguerre/faster_convex_cell.cuh",
        "paragram/csrc/laguerre/voronoi_convex_cell.cuh",
    ):
        compact = compact_cpp(read_cpp(quarantine, relative))
        if '#include"status.cuh"' not in compact:
            raise ValidationFailure(f"{relative} does not include the shared status ABI")


def validate_stack_semantics(quarantine: Path) -> None:
    cases = (
        (
            "paragram/csrc/laguerre/pwr_bvh.cuh",
            "pwr_stack_push",
            "shrinking_power_box_query",
        ),
        (
            "paragram/csrc/laguerre/voronoi_bvh.cuh",
            "voronoi_stack_push",
            "shrinking_voronoi_box_query",
        ),
    )
    expected_push = (
        "{if(stack_top<cap){stack_idx[stack_top]=idx;"
        "stack_dist[stack_top]=dist;stack_top++;returntrue;}returnfalse;}"
    )
    for relative, push_name, traversal_name in cases:
        compact = compact_cpp(read_cpp(quarantine, relative))
        outcomes = enum_names(
            compact,
            "enumclassTraversalOutcome:unsignedchar",
        )
        if outcomes != TRAVERSAL_OUTCOMES:
            raise ValidationFailure(
                f"{relative} traversal outcomes mismatch: {outcomes!r}"
            )
        if "constintstackSize=PARAGRAM_TRAVERSAL_STACK_CAPACITY;" not in compact:
            raise ValidationFailure(f"{relative} does not use the bounded stack capacity")
        push_body = named_function_body(compact, push_name)
        if push_body != expected_push:
            raise ValidationFailure(
                f"{relative} full-stack behavior is not the fail-closed push contract"
            )
        traversal_body = named_function_body(compact, traversal_name)
        if compact.count(f"{push_name}(") != 4:
            raise ValidationFailure(f"{relative} must contain exactly three stack pushes")
        guarded_push = f"if(!{push_name}("
        if traversal_body.count(guarded_push) != 3:
            raise ValidationFailure(f"{relative} has an unguarded traversal-stack push")
        if traversal_body.count("returnTraversalOutcome::stack_overflow;") != 3:
            raise ValidationFailure(f"{relative} does not fail closed on every full push")
        tangent_guard = "if(fmaxf(diff0,diff1)<=0.f)"
        if traversal_body.count(tangent_guard) != 1:
            raise ValidationFailure(
                f"{relative} must retain exactly one zero-margin tangent far branch"
            )
        if "if(fmaxf(diff0,diff1)<0.f)" in traversal_body:
            raise ValidationFailure(
                f"{relative} silently excludes zero-margin tangent far branches"
            )
        if "far_pos" in compact or "sqrDistMax" in compact:
            raise ValidationFailure(f"{relative} retains stack replacement/eviction state")


def validate_status_priority_and_raw_adjacency(quarantine: Path) -> None:
    cases = (
        (
            "paragram/csrc/laguerre/laguerre_ultra.cu",
            "compute_bvh_power_diagram_kernel",
            "pwr_bvh",
        ),
        (
            "paragram/csrc/laguerre/voronoi_ultra.cu",
            "compute_bvh_voronoi_kernel",
            "voronoi_bvh",
        ),
    )
    for relative, kernel_name, namespace in cases:
        compact = compact_cpp(read_cpp(quarantine, relative))
        kernel = named_function_body(compact, kernel_name)
        priority = (
            "Statusfinal_status=local_status;"
            "if(final_status==success){"
            f"if(traversal_outcome=={namespace}::TraversalOutcome::stack_overflow)"
            "final_status=traversal_stack_overflow;"
            f"elseif(traversal_outcome!={namespace}::TraversalOutcome::complete)"
            "final_status=security_radius_not_reached;}"
        )
        if priority not in kernel:
            raise ValidationFailure(
                f"{relative} does not preserve local error priority over traversal status"
            )
        all_raw_writes = re.findall(r"neighbors\[[^]]+\]=", kernel)
        success_blocks = named_if_bodies(kernel, "final_status==success")
        guarded_raw_writes = [
            re.findall(r"neighbors\[[^]]+\]=", block)
            for block in success_blocks
        ]
        if not all_raw_writes or all_raw_writes not in guarded_raw_writes:
            raise ValidationFailure(
                f"{relative} can write raw adjacency for a non-success cell"
            )
        launch = "gather_adjacency_kernel<<<num_points,MAX_PLANES>>>(MAX_PLANES,"
        if launch not in compact:
            raise ValidationFailure(
                f"{relative} changes the one-block-per-cell MAX_PLANES gather launch"
            )
        gather_call = (
            "neighbors.data_ptr<int>(),status.data_ptr<int>(),"
            "tmp_adjacency.data_ptr<int>()"
        )
        if gather_call not in compact:
            raise ValidationFailure(
                f"{relative} does not pass status into CSR quarantine"
            )


def quarantine_cpu_fixture() -> None:
    statuses = (0, 6, 0, 1)
    raw_neighbors = ((1, 2), (0, 2), (0, 1, 3), (2,))
    rows: list[tuple[int, ...]] = []
    for source, neighbors in enumerate(raw_neighbors):
        rows.append(
            tuple(
                neighbor
                for neighbor in neighbors
                if 0 <= neighbor < len(statuses)
                and statuses[source] == 0
                and statuses[neighbor] == 0
            )
        )
    expected_rows = ((2,), (), (0,), ())
    if tuple(rows) != expected_rows:
        raise ValidationFailure(
            f"internal CPU quarantine fixture mismatch: expected {expected_rows}, got {rows}"
        )
    undirected_edges = {
        tuple(sorted((source, target)))
        for source, targets in enumerate(rows)
        for target in targets
        if source != target
    }
    if undirected_edges != {(0, 2)}:
        raise ValidationFailure(
            f"CPU quarantine must retain only edge 0-2, got {undirected_edges!r}"
        )
    for failed in (1, 3):
        if rows[failed] or any(failed in row for row in rows):
            raise ValidationFailure(
                f"failed CPU fixture cell {failed} leaked into quarantined adjacency"
            )


def validate_csr_quarantine(quarantine: Path) -> None:
    relative = "paragram/csrc/laguerre/adjacency.cuh"
    compact = compact_cpp(read_cpp(quarantine, relative))
    gather = named_function_body(compact, "gather_adjacency_kernel")
    if "constint*__restrict__cell_status" not in compact:
        raise ValidationFailure("CSR gather does not accept the shared cell-status array")
    predicate = (
        "boolvalid_neighbor=neighbor>=0&&neighbor<num_points&&"
        "cell_status[source]==success&&cell_status[neighbor]==success;"
        "if(!valid_neighbor)neighbor=-1;"
    )
    if predicate not in gather:
        raise ValidationFailure(
            "CSR gather does not quarantine both failed sources and failed destinations"
        )
    if gather.count("__syncthreads();") != 3:
        raise ValidationFailure(
            "CSR gather must preserve exactly three block-wide synchronization barriers"
        )
    if re.search(r"\breturn(?:;|[^A-Za-z0-9_])", gather):
        raise ValidationFailure(
            "CSR gather must not return before its block-wide synchronization barriers"
        )
    quarantine_cpu_fixture()


def validate_python_explicit_bounds(quarantine: Path) -> None:
    relative = "paragram/_api.py"
    _, module = parsed_python(quarantine, relative)
    validator = python_function(module, "_validate_bounds", relative)
    validator_compact = re.sub(r"\s+", "", ast.unparse(validator))
    required_validator_fragments = (
        "def_validate_bounds(bounds:torch.Tensor)->torch.Tensor:",
        "ifnotisinstance(bounds,torch.Tensor):",
        "ifbounds.device.type!='cpu':",
        "ifbounds.dtype!=torch.float32:",
        "ifbounds.shape!=(2,3):",
        "bounds=bounds.contiguous()",
        "ifnotbool(torch.isfinite(bounds).all().item()):",
        "ifnotbool((bounds[0]<bounds[1]).all().item()):",
        "returnbounds",
    )
    for fragment in required_validator_fragments:
        if fragment not in validator_compact:
            raise ValidationFailure(
                f"{relative} explicit-bounds validator lacks {fragment!r}"
            )
    if any(isinstance(node, ast.BinOp) for node in ast.walk(validator)):
        raise ValidationFailure(
            f"{relative} explicit-bounds validator must not alter the six binary32 words"
        )

    public_functions = (
        (
            "power_diagram",
            ("points", "weights"),
            "build_laguerre_voronoi_ultra",
            ("points", "weights", "bounds"),
        ),
        (
            "voronoi_diagram",
            ("points",),
            "build_voronoi_ultra",
            ("points", "bounds"),
        ),
    )
    for function_name, positional_names, binding_name, binding_arguments in public_functions:
        function = python_function(module, function_name, relative)
        observed_positional = tuple(argument.arg for argument in function.args.args)
        if observed_positional[: len(positional_names)] != positional_names:
            raise ValidationFailure(
                f"{relative}:{function_name} changes its required positional prefix"
            )
        keyword_names = tuple(argument.arg for argument in function.args.kwonlyargs)
        if keyword_names.count("bounds") != 1:
            raise ValidationFailure(
                f"{relative}:{function_name} must expose one keyword-only bounds"
            )
        bounds_index = keyword_names.index("bounds")
        if function.args.kw_defaults[bounds_index] is not None:
            raise ValidationFailure(
                f"{relative}:{function_name} bounds must have no default"
            )
        if "bounds" in observed_positional:
            raise ValidationFailure(
                f"{relative}:{function_name} bounds must not be positional"
            )

        validations = [
            node
            for node in ast.walk(function)
            if isinstance(node, ast.Assign)
            and len(node.targets) == 1
            and isinstance(node.targets[0], ast.Name)
            and node.targets[0].id == "bounds"
            and isinstance(node.value, ast.Call)
            and python_call_name(node.value) == "_validate_bounds"
            and len(node.value.args) == 1
            and isinstance(node.value.args[0], ast.Name)
            and node.value.args[0].id == "bounds"
        ]
        if len(validations) != 1:
            raise ValidationFailure(
                f"{relative}:{function_name} must rebind exactly one validated bounds"
            )
        binding_calls = [
            node
            for node in ast.walk(function)
            if isinstance(node, ast.Call) and python_call_name(node) == binding_name
        ]
        if len(binding_calls) != 1:
            raise ValidationFailure(
                f"{relative}:{function_name} must call exactly one {binding_name}"
            )
        binding_call = binding_calls[0]
        observed_arguments = tuple(
            argument.id if isinstance(argument, ast.Name) else ""
            for argument in binding_call.args[: len(binding_arguments)]
        )
        if observed_arguments != binding_arguments:
            raise ValidationFailure(
                f"{relative}:{function_name} does not pass validated bounds "
                f"to {binding_name}: got {observed_arguments!r}"
            )
        if validations[0].lineno >= binding_call.lineno:
            raise ValidationFailure(
                f"{relative}:{function_name} calls its binding before bounds validation"
            )

    extension = compact_cpp(read_cpp(quarantine, "paragram/csrc/ext.cpp"))
    binding_fragments = (
        'py::arg("points"),py::arg("weights"),py::arg("bounds")',
        'py::arg("points"),py::arg("bounds")',
    )
    for fragment in binding_fragments:
        if extension.count(fragment) != 1:
            raise ValidationFailure(
                "paragram/csrc/ext.cpp must expose bounds once in each binding "
                f"at the declared argument position: {fragment}"
            )


def validate_explicit_bounds_evidence_files(quarantine: Path) -> None:
    readme = read_utf8(quarantine, "README.md").lower()
    readme_fragments = (
        "`bounds` is mandatory",
        "finite cpu `torch.float32` tensor",
        "shape `(2, 3)`",
        "strictly smaller than its upper coordinate",
        "without automatic padding",
    )
    for fragment in readme_fragments:
        if fragment not in readme:
            raise ValidationFailure(
                f"README.md lacks explicit closed-box evidence: {fragment!r}"
            )

    relative = "tests/test_explicit_bounds.py"
    tests_source, tests_module = parsed_python(quarantine, relative)
    test_names = {
        node.name
        for node in tests_module.body
        if isinstance(node, ast.FunctionDef) and node.name.startswith("test_")
    }
    required_tests = {
        "test_valid_noncontiguous_bounds_preserve_binary32_words",
        "test_finite_extreme_bounds_pass_ingress_validation_only",
        "test_bounds_reject_wrong_container_dtype_shape_or_device",
        "test_bounds_reject_nonfinite_values",
        "test_bounds_reject_zero_or_negative_width",
        "test_bounds_reject_signed_zero_width",
        "test_public_bounds_parameter_is_required_and_keyword_only",
    }
    if not required_tests <= test_names:
        raise ValidationFailure(
            f"{relative} misses tests: {sorted(required_tests - test_names)!r}"
        )
    compact_tests = re.sub(r"\s+", "", tests_source)
    binary32_witnesses = (
        "validated.view(torch.int32)",
        "bounds.contiguous().view(torch.int32)",
        "inspect.signature(function).parameters[\"bounds\"]",
        "inspect.Parameter.KEYWORD_ONLY",
        "inspect.Parameter.empty",
    )
    for witness in binary32_witnesses:
        if re.sub(r"\s+", "", witness) not in compact_tests:
            raise ValidationFailure(
                f"{relative} lacks structural witness {witness!r}"
            )


def validate_cpp_explicit_bounds(quarantine: Path) -> None:
    header_relative = "paragram/csrc/laguerre/laguerre.h"
    header = compact_cpp(read_cpp(quarantine, header_relative))
    struct_fragment = (
        "structClosedAabbF32{std::array<float,3>lower;"
        "std::array<float,3>upper;};"
    )
    if struct_fragment not in header:
        raise ValidationFailure(
            f"{header_relative} lacks the six-scalar ClosedAabbF32 value type"
        )
    validator = named_function_body(header, "validate_closed_aabb_f32")
    validator_checks = (
        "bounds_in.defined()",
        "bounds_in.device().type()==at::kCPU",
        "bounds_in.scalar_type()==at::kFloat",
        "bounds_in.dim()==2&&bounds_in.size(0)==2&&bounds_in.size(1)==3",
        "torch::Tensorbounds=bounds_in.contiguous();",
        "constfloat*values=bounds.data_ptr<float>();",
        "std::isfinite(lower)&&std::isfinite(upper)",
        "lower<upper",
        "result.lower[axis]=lower;",
        "result.upper[axis]=upper;",
        "returnresult;",
    )
    for fragment in validator_checks:
        if fragment not in validator:
            raise ValidationFailure(
                f"{header_relative} C++ bounds validation lacks {fragment!r}"
            )
    contiguous_position = validator.find("bounds_in.contiguous()")
    pre_contiguous_checks = (
        "bounds_in.defined()",
        "bounds_in.device().type()==at::kCPU",
        "bounds_in.scalar_type()==at::kFloat",
        "bounds_in.dim()==2&&bounds_in.size(0)==2&&bounds_in.size(1)==3",
    )
    if contiguous_position < 0 or any(
        validator.find(check) < 0 or validator.find(check) >= contiguous_position
        for check in pre_contiguous_checks
    ):
        raise ValidationFailure(
            f"{header_relative} reads/contiguizes bounds before CPU type/shape validation"
        )
    declaration_fragments = (
        "build_laguerre_voronoi_ultra(torch::Tensorpoints_in,"
        "torch::Tensorweights_in,torch::Tensorbounds_in,",
        "build_voronoi_ultra(torch::Tensorpoints_in,torch::Tensorbounds_in,",
    )
    for fragment in declaration_fragments:
        if fragment not in header:
            raise ValidationFailure(
                f"{header_relative} does not propagate bounds in declaration {fragment!r}"
            )

    cases = (
        (
            "paragram/csrc/laguerre/laguerre_ultra.cu",
            "build_laguerre_voronoi_ultra",
            "compute_bvh_power_diagram_kernel",
            "pwr_bvh",
        ),
        (
            "paragram/csrc/laguerre/voronoi_ultra.cu",
            "build_voronoi_ultra",
            "compute_bvh_voronoi_kernel",
            "voronoi_bvh",
        ),
    )
    assignments = (
        "world_bounds.lower.x=explicit_bounds.lower[0];",
        "world_bounds.lower.y=explicit_bounds.lower[1];",
        "world_bounds.lower.z=explicit_bounds.lower[2];",
        "world_bounds.upper.x=explicit_bounds.upper[0];",
        "world_bounds.upper.y=explicit_bounds.upper[1];",
        "world_bounds.upper.z=explicit_bounds.upper[2];",
    )
    for relative, builder_name, kernel_name, namespace in cases:
        compact = compact_cpp(read_cpp(quarantine, relative))
        if "#include<c10/cuda/CUDAGuard.h>" not in compact:
            raise ValidationFailure(
                f"{relative} lacks the direct-binding CUDA device guard include"
            )
        builder = named_function_body(compact, builder_name)
        validation = (
            "ClosedAabbF32explicit_bounds=validate_closed_aabb_f32(bounds_in);"
        )
        validation_position = builder.find(validation)
        if validation_position < 0:
            raise ValidationFailure(
                f"{relative}:{builder_name} does not revalidate bounds in C++"
            )
        guard = "c10::cuda::CUDAGuarddevice_guard(points_in.device());"
        guard_position = builder.find(guard)
        if guard_position < 0 or builder.count(guard) != 1:
            raise ValidationFailure(
                f"{relative}:{builder_name} must install one points-device CUDAGuard"
            )
        if validation_position >= guard_position:
            raise ValidationFailure(
                f"{relative}:{builder_name} installs CUDAGuard before bounds validation"
            )
        metadata_checks = [
            "points_in.defined()",
            "points_in.device().type()==at::kCUDA",
            "points_in.scalar_type()==at::kFloat",
            "points_in.dim()==2&&points_in.size(1)==3",
            "initial_guesses_in.has_value()=="
            "initial_guesses_offsets_in.has_value()",
            "initial_guesses_in->defined()",
            "initial_guesses_offsets_in->defined()",
            "initial_guesses_in->device()==points_in.device()",
            "initial_guesses_offsets_in->device()==points_in.device()",
            "initial_guesses_in->scalar_type()==at::kInt",
            "initial_guesses_offsets_in->scalar_type()==at::kInt",
            "initial_guesses_in->dim()==1",
            "initial_guesses_offsets_in->dim()==1&&"
            "initial_guesses_offsets_in->size(0)==num_points+1",
        ]
        if builder_name == "build_laguerre_voronoi_ultra":
            metadata_checks.extend(
                (
                    "weights_in.defined()",
                    "weights_in.device().type()==at::kCUDA",
                    "weights_in.device()==points_in.device()",
                    "weights_in.scalar_type()==at::kFloat",
                    "weights_in.dim()==2&&weights_in.size(1)==1",
                    "weights_in.size(0)==num_points",
                )
            )
        for check in metadata_checks:
            check_position = builder.find(check)
            if check_position < 0 or check_position >= guard_position:
                raise ValidationFailure(
                    f"{relative}:{builder_name} lacks pre-guard metadata/device "
                    f"validation {check!r}"
                )
        torch_checks = [
            match.start()
            for match in re.finditer(re.escape("TORCH_CHECK("), builder)
        ]
        if not torch_checks or any(position >= guard_position for position in torch_checks):
            raise ValidationFailure(
                f"{relative}:{builder_name} has missing or post-guard TORCH_CHECK metadata"
            )

        later_gpu_operations = (
            ".contiguous()",
            "torch::empty(",
            "torch::full(",
            "torch::zeros(",
            "cudaMalloc(",
            "<<<",
        )
        for operation in later_gpu_operations:
            operation_positions = [
                match.start()
                for match in re.finditer(re.escape(operation), builder)
            ]
            if any(position < guard_position for position in operation_positions):
                raise ValidationFailure(
                    f"{relative}:{builder_name} performs {operation!r} before "
                    "metadata validation and CUDAGuard"
                )
        if builder.count("cuBQL::box_t<float,3>world_bounds;") != 1:
            raise ValidationFailure(
                f"{relative}:{builder_name} must create one explicit world_bounds"
            )
        for assignment in assignments:
            if builder.count(assignment) != 1:
                raise ValidationFailure(
                    f"{relative}:{builder_name} must assign exactly one {assignment}"
                )
        launch = (
            f"{kernel_name}<<<launch_blocks,CELL_BLOCK_STRIDE>>>("
            "num_points,world_bounds,"
        )
        if builder.count(launch) != 1:
            raise ValidationFailure(
                f"{relative}:{builder_name} does not pass the exact world_bounds "
                "to its kernel"
            )

        kernel = named_function_body(compact, kernel_name)
        if (
            f"{kernel_name}(intnum_points,cuBQL::box_t<float,3>world_bounds,"
            not in compact
        ):
            raise ValidationFailure(
                f"{relative}:{kernel_name} does not receive world_bounds by value"
            )
        cell_init = "cell.CCInit(idx,points,&local_status,min_bound,max_bound);"
        radii = (
            f"{namespace}::BoxRadiiinitial_box_radii("
            "cell.lower_bound,cell.upper_bound);"
        )
        if kernel.count(cell_init) != 1 or kernel.count(radii) != 1:
            raise ValidationFailure(
                f"{relative}:{kernel_name} must derive one BoxRadii from initialized "
                "cell bounds"
            )
        if kernel.find(cell_init) >= kernel.find(radii):
            raise ValidationFailure(
                f"{relative}:{kernel_name} derives BoxRadii before CCInit"
            )
        kernel_bounds = (
            "float3min_bound=make_float3(world_bounds.lower.x,"
            "world_bounds.lower.y,world_bounds.lower.z);",
            "float3max_bound=make_float3(world_bounds.upper.x,"
            "world_bounds.upper.y,world_bounds.upper.z);",
        )
        for fragment in kernel_bounds:
            if kernel.count(fragment) != 1:
                raise ValidationFailure(
                    f"{relative}:{kernel_name} does not unpack caller bounds exactly"
                )
        forbidden_fallbacks = (
            "root_bounds",
            "bvh.nodes[0].bounds",
            "initial_box_radii{",
            "1e10",
        )
        for forbidden in forbidden_fallbacks:
            if forbidden in compact:
                raise ValidationFailure(
                    f"{relative} retains implicit/fixed box fallback {forbidden!r}"
                )
        if re.search(r"cudaMemcpy\(&[^,]*(?:root|world)_bounds", compact):
            raise ValidationFailure(
                f"{relative} copies implicit root/world bounds from CUDA"
            )


def validate_convex_cells_use_exact_bounds(quarantine: Path) -> None:
    relatives = (
        "paragram/csrc/laguerre/faster_convex_cell.cuh",
        "paragram/csrc/laguerre/voronoi_convex_cell.cuh",
    )
    assignments = (
        "floatconstvmin_x=bbox_min.x;",
        "floatconstvmax_x=bbox_max.x;",
        "floatconstvmin_y=bbox_min.y;",
        "floatconstvmax_y=bbox_max.y;",
        "floatconstvmin_z=bbox_min.z;",
        "floatconstvmax_z=bbox_max.z;",
    )
    planes = (
        "CCSetPlane(0,1.0,0.0,0.0,-vmin_x);",
        "CCSetPlane(1,-1.0,0.0,0.0,vmax_x);",
        "CCSetPlane(2,0.0,1.0,0.0,-vmin_y);",
        "CCSetPlane(3,0.0,-1.0,0.0,vmax_y);",
        "CCSetPlane(4,0.0,0.0,1.0,-vmin_z);",
        "CCSetPlane(5,0.0,0.0,-1.0,vmax_z);",
    )
    for relative in relatives:
        compact = compact_cpp(read_cpp(quarantine, relative))
        if "CUBE_EPSILON" in compact:
            raise ValidationFailure(f"{relative} retains hidden CUBE_EPSILON padding")
        initializer = named_function_body(compact, "CCInit")
        for fragment in (*assignments, *planes):
            if initializer.count(fragment) != 1:
                raise ValidationFailure(
                    f"{relative}:CCInit must contain exactly one {fragment}"
                )
        if re.search(r"bbox_(?:min|max)\.[xyz][+-]", initializer):
            raise ValidationFailure(
                f"{relative}:CCInit pads a caller-provided bbox coordinate"
            )


def validate_semantics(quarantine: Path) -> None:
    validate_status_abi(quarantine)
    validate_stack_semantics(quarantine)
    validate_status_priority_and_raw_adjacency(quarantine)
    validate_csr_quarantine(quarantine)
    validate_python_explicit_bounds(quarantine)
    validate_cpp_explicit_bounds(quarantine)
    validate_convex_cells_use_exact_bounds(quarantine)
    validate_explicit_bounds_evidence_files(quarantine)


def apply_and_validate(
    checkout: Path,
    series: SeriesSpec,
    pin: dict[str, Any],
) -> None:
    license_path, submodule_paths = pin_protected_paths(pin)
    protected_paths = {license_path, ".gitmodules", *submodule_paths}
    for patch in series.patches:
        overlap = protected_paths & set(patch.touched_paths)
        if overlap:
            raise ValidationFailure(
                f"patch #{patch.order} declares protected paths: {sorted(overlap)!r}"
            )

    with tempfile.TemporaryDirectory(
        prefix="morsehgp3d-paragram-patch-quarantine-"
    ) as temporary:
        quarantine = Path(temporary)
        prepare_quarantine(checkout, quarantine, series.base_commit)

        initial_tree = git_text(quarantine, "write-tree")
        if initial_tree != series.base_tree:
            raise ValidationFailure(
                f"quarantine base tree mismatch: expected {series.base_tree}, "
                f"got {initial_tree}"
            )
        base_entries = tree_entries(quarantine, initial_tree)

        current_tree = initial_tree
        current_entries = base_entries
        for patch in series.patches:
            if current_tree != patch.pre_tree:
                raise ValidationFailure(
                    f"patch #{patch.order} pre-tree mismatch: "
                    f"expected {patch.pre_tree}, got {current_tree}"
                )
            validate_raw_entries_against_trees(patch, current_entries)
            git(
                quarantine,
                "apply",
                "--index",
                "--check",
                "--whitespace=error-all",
                str(patch.path),
            )
            git(
                quarantine,
                "apply",
                "--index",
                "--whitespace=error-all",
                str(patch.path),
            )
            observed_tree = git_text(quarantine, "write-tree")
            if observed_tree != patch.post_tree:
                raise ValidationFailure(
                    f"patch #{patch.order} post-tree mismatch: "
                    f"expected {patch.post_tree}, got {observed_tree}"
                )
            observed_entries = tree_entries(quarantine, observed_tree)
            validate_raw_entries_against_trees(
                patch,
                current_entries,
                observed_entries,
            )
            observed_actions = tree_actions(current_entries, observed_entries)
            expected_actions = (
                patch.touched_paths,
                patch.added_paths,
                patch.modified_paths,
                patch.deleted_paths,
            )
            if observed_actions != expected_actions:
                labels = ("touched", "added", "modified", "deleted")
                differences = [
                    f"{label}: expected {expected!r}, got {observed!r}"
                    for label, expected, observed in zip(
                        labels,
                        expected_actions,
                        observed_actions,
                    )
                    if expected != observed
                ]
                raise ValidationFailure(
                    f"patch #{patch.order} action mismatch:\n- "
                    + "\n- ".join(differences)
                )
            current_tree = observed_tree
            current_entries = observed_entries

        if current_tree != series.final_tree:
            raise ValidationFailure(
                f"final tree mismatch: expected {series.final_tree}, got {current_tree}"
            )
        if git(quarantine, "diff", "--no-ext-diff", "--quiet"):
            raise ValidationFailure("quarantine worktree differs from its patched index")

        for protected in sorted(protected_paths):
            if base_entries.get(protected) != current_entries.get(protected):
                raise ValidationFailure(
                    f"protected license/gitlink metadata changed: {protected}"
                )
        base_gitlinks = {
            path: entry
            for path, entry in base_entries.items()
            if entry.mode == "160000" and entry.kind == "commit"
        }
        final_gitlinks = {
            path: entry
            for path, entry in current_entries.items()
            if entry.mode == "160000" and entry.kind == "commit"
        }
        if base_gitlinks != final_gitlinks:
            raise ValidationFailure("the complete patch series changes Git submodule links")
        if set(base_gitlinks) != set(submodule_paths):
            raise ValidationFailure(
                "source-pin submodule inventory differs from the base-tree gitlinks"
            )

        validate_semantics(quarantine)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Offline-validate and quarantine-apply the complete pinned Paragram "
            "patch series."
        )
    )
    parser.add_argument("checkout", type=Path, help="clean pinned Paragram checkout")
    parser.add_argument(
        "--series",
        type=Path,
        help=(
            "series.toml path; by default exactly one "
            "third_party/paragram/*/series.toml must exist"
        ),
    )
    arguments = parser.parse_args()

    try:
        series_path = resolve_series_path(arguments.series)
        series = parse_series(series_path)
        checkout = arguments.checkout.resolve()
        pin = validate_source_pin(checkout, series)
        apply_and_validate(checkout, series, pin)
    except ValidationFailure as exc:
        print("Paragram patch-series validation failed:", file=sys.stderr)
        print(f"- {exc}", file=sys.stderr)
        return 1

    patch_word = "patch" if len(series.patches) == 1 else "patches"
    print(
        "Validated offline Paragram patch series: "
        f"{len(series.patches)} {patch_word}, base {series.base_tree}, "
        f"final {series.final_tree}, CPU quarantine edge 0-2 only, "
        "explicit closed AABB structure validated; "
        "proposal_only, CUDA not run."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
