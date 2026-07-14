#!/usr/bin/env python3
"""Verify the completeness and byte-level integrity of local reference PDFs."""

from __future__ import annotations

import hashlib
import sys
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REFERENCE_ROOT = ROOT / "docs" / "references"
MANIFEST = REFERENCE_ROOT / "references.toml"
REQUIRED_FIELDS = {
    "id",
    "title",
    "authors",
    "path",
    "source_url",
    "source_status",
    "version",
    "doi",
    "doi_status",
    "license",
    "license_url",
    "redistribution_note",
    "bytes",
    "sha256",
}


def digest(path: Path) -> str:
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


def main() -> int:
    errors: list[str] = []
    try:
        data = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        print(f"Reference manifest is unreadable: {exc}", file=sys.stderr)
        return 1

    if data.get("schema_version") != 1:
        errors.append("references.toml: schema_version must be 1")

    entries = data.get("reference")
    if not isinstance(entries, list) or not entries:
        errors.append("references.toml: at least one [[reference]] entry is required")
        entries = []

    declared_paths: set[Path] = set()
    identifiers: set[str] = set()

    for index, entry in enumerate(entries, start=1):
        label = f"reference #{index}"
        if not isinstance(entry, dict):
            errors.append(f"{label}: entry must be a TOML table")
            continue

        missing = sorted(REQUIRED_FIELDS - entry.keys())
        if missing:
            errors.append(f"{label}: missing fields: {', '.join(missing)}")
            continue

        identifier = entry["id"]
        label = str(identifier)
        if not isinstance(identifier, str) or not identifier:
            errors.append(f"reference #{index}: id must be a non-empty string")
        elif identifier in identifiers:
            errors.append(f"{label}: duplicate id")
        identifiers.add(str(identifier))

        authors = entry["authors"]
        if not isinstance(authors, list) or not authors or not all(
            isinstance(author, str) and author for author in authors
        ):
            errors.append(f"{label}: authors must be a non-empty string array")

        for field in (
            "title",
            "path",
            "source_status",
            "version",
            "doi_status",
            "license",
            "redistribution_note",
        ):
            if not isinstance(entry[field], str) or not entry[field]:
                errors.append(f"{label}: {field} must be a non-empty string")

        for field in ("source_url", "doi", "license_url"):
            if not isinstance(entry[field], str):
                errors.append(f"{label}: {field} must be a string, possibly empty")

        raw_path = Path(str(entry["path"]))
        path = (ROOT / raw_path).resolve()
        try:
            path.relative_to(REFERENCE_ROOT.resolve())
        except ValueError:
            errors.append(f"{label}: path escapes docs/references: {raw_path}")
            continue

        if path in declared_paths:
            errors.append(f"{label}: duplicate path: {raw_path}")
        declared_paths.add(path)

        if path.suffix.lower() != ".pdf":
            errors.append(f"{label}: path is not a PDF: {raw_path}")
            continue
        if not path.is_file():
            errors.append(f"{label}: missing PDF: {raw_path}")
            continue

        expected_bytes = entry["bytes"]
        if not isinstance(expected_bytes, int) or expected_bytes <= 0:
            errors.append(f"{label}: bytes must be a positive integer")
        elif path.stat().st_size != expected_bytes:
            errors.append(
                f"{label}: size mismatch for {raw_path}: "
                f"expected {expected_bytes}, got {path.stat().st_size}"
            )

        expected_sha = entry["sha256"]
        if (
            not isinstance(expected_sha, str)
            or len(expected_sha) != 64
            or any(char not in "0123456789abcdef" for char in expected_sha)
        ):
            errors.append(f"{label}: sha256 must be 64 lowercase hexadecimal digits")
        else:
            actual_sha = digest(path)
            if actual_sha != expected_sha:
                errors.append(
                    f"{label}: SHA-256 mismatch for {raw_path}: "
                    f"expected {expected_sha}, got {actual_sha}"
                )

    actual_paths = {path.resolve() for path in REFERENCE_ROOT.rglob("*.pdf")}
    for undeclared in sorted(actual_paths - declared_paths):
        errors.append(f"undeclared PDF: {undeclared.relative_to(ROOT)}")
    for non_pdf in sorted(declared_paths - actual_paths):
        if non_pdf.exists():
            errors.append(f"declared path is not found by PDF inventory: {non_pdf}")

    if errors:
        print("Reference validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print(f"Validated {len(entries)} reference PDFs and their SHA-256 digests.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
