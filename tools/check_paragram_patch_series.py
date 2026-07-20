#!/usr/bin/env python3
"""Validate and quarantine-apply the pinned Paragram patch series offline.

The source checkout is never modified.  Its phase 7.1 pin is replayed first,
then the complete patch series is applied to a temporary Git repository whose
object database uses the source checkout only as a local alternate.  No patched
tree escapes that temporary quarantine.
"""

from __future__ import annotations

import argparse
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
EXPECTED_SEMANTIC_CONTRACT = (
    "historical_status_codes_0_through_5_unchanged_and_traversal_stack_overflow_is_6",
    "weighted_and_unweighted_status_abi_is_one_shared_int32_enum",
    "bounded_box_traversals_return_a_typed_outcome",
    "every_full_stack_push_returns_stack_overflow_without_eviction_or_replacement",
    "every_non_strictly_excluded_far_branch_is_retained_including_zero_margin_tangencies",
    "a_preexisting_cell_error_wins_else_stack_overflow_maps_to_status_6",
    "nonzero_status_cells_write_no_raw_adjacency",
    "csr_gather_keeps_an_edge_only_when_both_endpoint_statuses_are_success",
)
EXPECTED_LIMITATIONS = (
    "not_compiled_with_nvcc_or_pytorch_in_this_milestone",
    "upstream_default_stream_usage_and_asynchronous_cuda_fault_propagation_"
    "are_not_patched_or_validated",
    "no_explicit_caller_box_or_geometry_export_yet",
    "status_zero_remains_a_floating_proposal_not_a_certificate",
)
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

        string_list(
            raw_patch.get("semantic_contract"),
            f"{label}.semantic_contract",
            errors,
            exact=EXPECTED_SEMANTIC_CONTRACT,
        )
        string_list(
            raw_patch.get("limitations"),
            f"{label}.limitations",
            errors,
            exact=EXPECTED_LIMITATIONS,
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


def read_cpp(quarantine: Path, relative: str) -> str:
    path = quarantine.joinpath(*PurePosixPath(relative).parts)
    if path.is_symlink() or not path.is_file():
        raise ValidationFailure(f"patched semantic source is missing: {relative}")
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        raise ValidationFailure(f"cannot read patched semantic source {relative}: {exc}") from exc


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


def validate_semantics(quarantine: Path) -> None:
    validate_status_abi(quarantine)
    validate_stack_semantics(quarantine)
    validate_status_priority_and_raw_adjacency(quarantine)
    validate_csr_quarantine(quarantine)


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

        patch_arguments = [str(patch.path) for patch in series.patches]
        git(
            quarantine,
            "apply",
            "--index",
            "--check",
            "--whitespace=error-all",
            *patch_arguments,
        )

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
        f"final {series.final_tree}, CPU quarantine edge 0-2 only; "
        "proposal_only, CUDA not run."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
