#!/usr/bin/env python3
"""Validate a local Paragram checkout against the phase 7.1 source pin.

The checker is deliberately offline: it never fetches, clones, or initializes a
submodule. It validates the checked-out commit and tree, the recorded gitlinks,
the complete regular-file inventory, and independent SHA-256 digests.
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
import re
import subprocess
import sys
import tomllib
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "docs" / "references" / "paragram_source_pin.toml"
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
REGULAR_MODES = {"100644", "100755"}


class GitError(RuntimeError):
    """A local Git query failed."""


@dataclass(frozen=True)
class TreeEntry:
    mode: str
    kind: str
    object_id: str


def sha256(path: Path) -> str:
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


def git(checkout: Path, *arguments: str) -> bytes:
    command = ["git", "-C", str(checkout), *arguments]
    try:
        result = subprocess.run(
            command,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except OSError as exc:
        raise GitError(f"cannot execute git: {exc}") from exc
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise GitError(f"{' '.join(command)} failed: {detail or 'unknown error'}")
    return result.stdout


def git_text(checkout: Path, *arguments: str) -> str:
    return git(checkout, *arguments).decode("utf-8", errors="strict").strip()


def relative_path(value: Any, label: str, errors: list[str]) -> str | None:
    if not isinstance(value, str) or not value:
        errors.append(f"{label} must be a non-empty string")
        return None
    candidate = PurePosixPath(value)
    if candidate.is_absolute() or any(part in {"", ".", ".."} for part in candidate.parts):
        errors.append(f"{label} must be a normalized relative POSIX path: {value!r}")
        return None
    if candidate.as_posix() != value:
        errors.append(f"{label} must use normalized POSIX separators: {value!r}")
        return None
    return value


def parse_tree(checkout: Path) -> dict[str, TreeEntry]:
    entries: dict[str, TreeEntry] = {}
    raw = git(checkout, "ls-tree", "-r", "-z", "HEAD")
    for record in raw.split(b"\0"):
        if not record:
            continue
        try:
            metadata, raw_path = record.split(b"\t", maxsplit=1)
            mode, kind, object_id = metadata.decode("ascii").split(" ")
            path = raw_path.decode("utf-8", errors="strict")
        except (UnicodeDecodeError, ValueError) as exc:
            raise GitError(f"cannot parse git ls-tree record: {record!r}") from exc
        if path in entries:
            raise GitError(f"duplicate path in git tree: {path}")
        entries[path] = TreeEntry(mode=mode, kind=kind, object_id=object_id)
    return entries


def load_gitmodules(checkout: Path, errors: list[str]) -> dict[str, str]:
    path = checkout / ".gitmodules"
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    try:
        with path.open("r", encoding="utf-8") as stream:
            parser.read_file(stream)
    except (OSError, configparser.Error) as exc:
        errors.append(f".gitmodules is unreadable: {exc}")
        return {}

    urls: dict[str, str] = {}
    for section in parser.sections():
        if not section.startswith('submodule "'):
            continue
        module_path = parser.get(section, "path", fallback="")
        module_url = parser.get(section, "url", fallback="")
        if not module_path or not module_url:
            errors.append(f".gitmodules section {section!r} lacks path or url")
            continue
        if module_path in urls:
            errors.append(f".gitmodules contains duplicate path: {module_path}")
            continue
        urls[module_path] = module_url
    return urls


def require_table(data: dict[str, Any], name: str, errors: list[str]) -> dict[str, Any]:
    value = data.get(name)
    if not isinstance(value, dict):
        errors.append(f"[{name}] table is required")
        return {}
    return value


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate an existing local Paragram checkout without network access."
    )
    parser.add_argument("checkout", type=Path, help="path to the Paragram checkout")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help=f"pin manifest (default: {DEFAULT_MANIFEST})",
    )
    arguments = parser.parse_args()

    errors: list[str] = []
    manifest_path = arguments.manifest.resolve()
    try:
        data = tomllib.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        print(f"Paragram pin manifest is unreadable: {exc}", file=sys.stderr)
        return 1

    if data.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    if not isinstance(data.get("checked_on"), str) or not data.get("checked_on"):
        errors.append("checked_on must be a non-empty string")
    if not isinstance(data.get("purpose"), str) or not data.get("purpose"):
        errors.append("purpose must be a non-empty string")

    upstream = require_table(data, "upstream", errors)
    policy = require_table(data, "policy", errors)
    for field in ("name", "repository_url", "license_spdx"):
        if not isinstance(upstream.get(field), str) or not upstream.get(field):
            errors.append(f"upstream.{field} must be a non-empty string")

    expected_commit = upstream.get("commit")
    if not isinstance(expected_commit, str) or not HEX40.fullmatch(expected_commit):
        errors.append("upstream.commit must be 40 lowercase hexadecimal digits")
        expected_commit = None
    expected_tree = upstream.get("tree")
    if not isinstance(expected_tree, str) or not HEX40.fullmatch(expected_tree):
        errors.append("upstream.tree must be 40 lowercase hexadecimal digits")
        expected_tree = None
    license_path = relative_path(upstream.get("license_path"), "upstream.license_path", errors)

    if policy.get("worktree_must_be_clean") is not True:
        errors.append("policy.worktree_must_be_clean must be true")
    if policy.get("regular_file_inventory_complete") is not True:
        errors.append("policy.regular_file_inventory_complete must be true")

    audited_files = data.get("audited_file")
    if not isinstance(audited_files, list) or not audited_files:
        errors.append("at least one [[audited_file]] table is required")
        audited_files = []
    audited_by_path: dict[str, dict[str, Any]] = {}
    for index, entry in enumerate(audited_files, start=1):
        label = f"audited_file #{index}"
        if not isinstance(entry, dict):
            errors.append(f"{label} must be a table")
            continue
        path = relative_path(entry.get("path"), f"{label}.path", errors)
        expected_bytes = entry.get("bytes")
        expected_digest = entry.get("sha256")
        if (
            not isinstance(expected_bytes, int)
            or isinstance(expected_bytes, bool)
            or expected_bytes < 0
        ):
            errors.append(f"{label}.bytes must be a non-negative integer")
        if not isinstance(expected_digest, str) or not HEX64.fullmatch(expected_digest):
            errors.append(f"{label}.sha256 must be 64 lowercase hexadecimal digits")
        if path is not None:
            if path in audited_by_path:
                errors.append(f"duplicate audited_file path: {path}")
            else:
                audited_by_path[path] = entry

    submodules = data.get("submodule")
    if not isinstance(submodules, list) or not submodules:
        errors.append("at least one [[submodule]] table is required")
        submodules = []
    submodules_by_path: dict[str, dict[str, Any]] = {}
    for index, entry in enumerate(submodules, start=1):
        label = f"submodule #{index}"
        if not isinstance(entry, dict):
            errors.append(f"{label} must be a table")
            continue
        path = relative_path(entry.get("path"), f"{label}.path", errors)
        url = entry.get("url")
        gitlink = entry.get("gitlink")
        if not isinstance(url, str) or not url:
            errors.append(f"{label}.url must be a non-empty string")
        if not isinstance(gitlink, str) or not HEX40.fullmatch(gitlink):
            errors.append(f"{label}.gitlink must be 40 lowercase hexadecimal digits")
        if path is not None:
            if path in submodules_by_path:
                errors.append(f"duplicate submodule path: {path}")
            else:
                submodules_by_path[path] = entry

    checkout = arguments.checkout.resolve()
    if not checkout.is_dir():
        errors.append(f"checkout is not a directory: {checkout}")
    else:
        try:
            top_level = Path(git_text(checkout, "rev-parse", "--show-toplevel")).resolve()
            if top_level != checkout:
                errors.append(
                    "checkout must be the Git worktree root: "
                    f"got {checkout}, root is {top_level}"
                )

            actual_commit = git_text(checkout, "rev-parse", "--verify", "HEAD^{commit}")
            if expected_commit is not None and actual_commit != expected_commit:
                errors.append(f"HEAD mismatch: expected {expected_commit}, got {actual_commit}")

            actual_tree = git_text(checkout, "rev-parse", "--verify", "HEAD^{tree}")
            if expected_tree is not None and actual_tree != expected_tree:
                errors.append(f"tree mismatch: expected {expected_tree}, got {actual_tree}")

            dirty = git(checkout, "status", "--porcelain=v1", "-z", "--untracked-files=all")
            if dirty:
                records = [
                    record.decode("utf-8", errors="backslashreplace")
                    for record in dirty.split(b"\0")
                    if record
                ]
                preview = "; ".join(records[:8])
                if len(records) > 8:
                    preview += f"; ... ({len(records) - 8} more records)"
                errors.append(f"worktree is not clean: {preview}")

            tree = parse_tree(checkout)
            regular_tree_paths = {
                path
                for path, entry in tree.items()
                if entry.mode in REGULAR_MODES and entry.kind == "blob"
            }
            audited_paths = set(audited_by_path)
            for missing in sorted(regular_tree_paths - audited_paths):
                errors.append(f"tracked regular file absent from manifest: {missing}")
            for extra in sorted(audited_paths - regular_tree_paths):
                errors.append(f"audited file is not a tracked regular file in HEAD: {extra}")

            for path, entry in sorted(audited_by_path.items()):
                tree_entry = tree.get(path)
                if (
                    tree_entry is None
                    or tree_entry.mode not in REGULAR_MODES
                    or tree_entry.kind != "blob"
                ):
                    continue
                local_path = checkout.joinpath(*PurePosixPath(path).parts)
                if local_path.is_symlink() or not local_path.is_file():
                    errors.append(f"audited file is missing or not regular: {path}")
                    continue
                expected_bytes = entry.get("bytes")
                if isinstance(expected_bytes, int) and not isinstance(expected_bytes, bool):
                    actual_bytes = local_path.stat().st_size
                    if actual_bytes != expected_bytes:
                        errors.append(
                            f"byte-size mismatch for {path}: "
                            f"expected {expected_bytes}, got {actual_bytes}"
                        )
                expected_digest = entry.get("sha256")
                if isinstance(expected_digest, str) and HEX64.fullmatch(expected_digest):
                    actual_digest = sha256(local_path)
                    if actual_digest != expected_digest:
                        errors.append(
                            f"SHA-256 mismatch for {path}: "
                            f"expected {expected_digest}, got {actual_digest}"
                        )

            gitmodule_urls = load_gitmodules(checkout, errors)
            tree_gitlinks = {
                path
                for path, entry in tree.items()
                if entry.mode == "160000" and entry.kind == "commit"
            }
            declared_gitlinks = set(submodules_by_path)
            for missing in sorted(tree_gitlinks - declared_gitlinks):
                errors.append(f"gitlink absent from manifest: {missing}")
            for extra in sorted(declared_gitlinks - tree_gitlinks):
                errors.append(f"declared submodule is not a gitlink in HEAD: {extra}")
            for path, entry in sorted(submodules_by_path.items()):
                tree_entry = tree.get(path)
                if tree_entry is None or tree_entry.mode != "160000" or tree_entry.kind != "commit":
                    continue
                expected_gitlink = entry.get("gitlink")
                if isinstance(expected_gitlink, str) and tree_entry.object_id != expected_gitlink:
                    errors.append(
                        f"gitlink mismatch for {path}: "
                        f"expected {expected_gitlink}, got {tree_entry.object_id}"
                    )
                expected_url = entry.get("url")
                actual_url = gitmodule_urls.get(path)
                if actual_url != expected_url:
                    errors.append(
                        f"submodule URL mismatch for {path}: "
                        f"expected {expected_url!r}, got {actual_url!r}"
                    )

            for extra_path in sorted(set(gitmodule_urls) - declared_gitlinks):
                errors.append(f".gitmodules path absent from manifest: {extra_path}")

            if license_path is not None and license_path not in audited_by_path:
                errors.append(f"upstream.license_path is not audited: {license_path}")
        except (GitError, UnicodeDecodeError) as exc:
            errors.append(str(exc))

    if errors:
        print("Paragram source-pin validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print(
        "Validated clean Paragram checkout: "
        f"commit {expected_commit}, tree {expected_tree}, "
        f"{len(audited_by_path)} SHA-256 files, "
        f"{len(submodules_by_path)} gitlink."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
