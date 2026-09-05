"""Check pinned audit sources without writing or promoting product status."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys


LINEAGE = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = LINEAGE / "audits/validation_current.json"
SCHEMA_V1 = "mhgp7-current-audit-validation-v1"
SCHEMA_V2 = "mhgp7-current-audit-validation-v2"


def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def validate_pins(entries: object, field: str) -> dict[str, str]:
    if not isinstance(entries, dict) or not entries:
        raise ValueError(f"{field} must be a nonempty object")
    for relative, expected in entries.items():
        if not isinstance(relative, str):
            raise ValueError(f"invalid source path: {relative}")
        name = PurePosixPath(relative)
        if (
            name.is_absolute()
            or ".." in name.parts
            or not name.parts
            or str(name) != relative
            or "\\" in relative
        ):
            raise ValueError(f"invalid source path: {relative}")
        path = (LINEAGE / name).resolve()
        if not path.is_relative_to(LINEAGE):
            raise ValueError(f"source outside v7: {relative}")
        if not isinstance(expected, str) or not re.fullmatch(
            r"[0-9a-f]{64}", expected
        ):
            raise ValueError(f"invalid SHA-256: {relative}")
    return entries


def stale_pins(entries: dict[str, str]) -> list[str]:
    stale = []
    for relative, expected in entries.items():
        path = LINEAGE / relative
        if (
            not path.is_file()
            or hashlib.sha256(path.read_bytes()).hexdigest() != expected
        ):
            stale.append(relative)
    return stale


def check_v1(manifest: dict[str, object]) -> int:
    entries = validate_pins(manifest.get("pinned_sources"), "pinned_sources")
    stale = stale_pins(entries)
    if stale:
        print("Audit à actualiser :", file=sys.stderr)
        for relative in stale:
            print(relative, file=sys.stderr)
        return 1
    print(
        f"Audit courant : {len(entries)} fichiers conformes aux hashes épinglés."
    )
    print(
        "Contrôle de fraîcheur uniquement ; aucune promotion du statut public."
    )
    return 0


def check_v2(manifest: dict[str, object]) -> int:
    common = validate_pins(manifest.get("common_pins"), "common_pins")
    variants = manifest.get("source_variants")
    if not isinstance(variants, list) or not variants:
        raise ValueError("source_variants must be a nonempty list")
    identifiers = set()
    checked = []
    # Validate every variant before considering any matching source snapshot.
    for variant in variants:
        if not isinstance(variant, dict):
            raise ValueError("source variant must be an object")
        identifier = variant.get("id")
        if not isinstance(identifier, str) or not re.fullmatch(
            r"[A-Za-z0-9][A-Za-z0-9_.-]*", identifier
        ):
            raise ValueError("source variant id must be a nonempty identifier")
        if identifier in identifiers:
            raise ValueError(f"duplicate source variant id: {identifier}")
        identifiers.add(identifier)
        scope = variant.get("qualification_scope")
        if not isinstance(scope, str) or not scope.strip():
            raise ValueError(f"invalid qualification_scope: {identifier}")
        entries = validate_pins(
            variant.get("pinned_sources"), f"pinned_sources of {identifier}"
        )
        for relative in common.keys() & entries.keys():
            if common[relative] != entries[relative]:
                raise ValueError(
                    f"conflicting common/variant pin: {identifier}/{relative}"
                )
        checked.append((identifier, scope, entries))

    common_stale = stale_pins(common)
    if common_stale:
        print("Audit à actualiser : pins communs modifiés.", file=sys.stderr)
        for relative in common_stale:
            print(relative, file=sys.stderr)
        return 1

    deltas = []
    for identifier, scope, entries in checked:
        stale = stale_pins(entries)
        if not stale:
            print(f"Audit courant : variante={identifier}")
            print(f"qualification_scope={scope}")
            print(
                f"{len(common)} pins communs et "
                f"{len(entries)} pins de variante "
                "conformes aux hashes épinglés."
            )
            print(
                "Contrôle de fraîcheur uniquement ; "
                "aucune promotion du statut public."
            )
            return 0
        deltas.append((identifier, stale))
    print(
        "Audit à actualiser : aucune variante complète ne correspond.",
        file=sys.stderr,
    )
    for identifier, stale in deltas:
        print(f"variante={identifier}", file=sys.stderr)
        for relative in stale:
            print(relative, file=sys.stderr)
    return 1


def check(manifest_path: Path) -> int:
    manifest = json.loads(
        manifest_path.read_text(encoding="utf-8"),
        object_pairs_hook=unique_object,
    )
    if not isinstance(manifest, dict):
        raise ValueError("audit manifest must be an object")
    if manifest.get("schema") == SCHEMA_V1:
        return check_v1(manifest)
    if manifest.get("schema") == SCHEMA_V2:
        return check_v2(manifest)
    raise ValueError("unsupported audit manifest")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args()
    try:
        return check(args.manifest)
    except (OSError, ValueError, TypeError) as error:
        print(f"Manifest d'audit invalide : {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
