"""Check audit freshness without writing files or certifying product exactness."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys


LINEAGE = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = (
    LINEAGE / "audits/receipts_20260904/validation_current.json"
)


def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def check(manifest_path: Path) -> int:
    manifest = json.loads(
        manifest_path.read_text(encoding="utf-8"),
        object_pairs_hook=unique_object,
    )
    if not isinstance(manifest, dict) or manifest.get("schema") != (
        "mhgp7-current-audit-validation-v1"
    ):
        raise ValueError("unsupported audit manifest")
    entries = manifest.get("pinned_sources")
    if not isinstance(entries, dict) or not entries:
        raise ValueError("pinned_sources must be a nonempty object")
    stale = []
    for relative, expected in entries.items():
        name = PurePosixPath(relative)
        if name.is_absolute() or ".." in name.parts or not name.parts:
            raise ValueError(f"invalid source path: {relative}")
        path = (LINEAGE / name).resolve()
        if not path.is_relative_to(LINEAGE):
            raise ValueError(f"source outside v7: {relative}")
        if not isinstance(expected, str) or not re.fullmatch(
            r"[0-9a-f]{64}", expected
        ):
            raise ValueError(f"invalid SHA-256: {relative}")
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            stale.append(relative)
    if stale:
        print("Audit à actualiser :", file=sys.stderr)
        for relative in stale:
            print(relative, file=sys.stderr)
        return 1
    print(f"Audit courant : {len(entries)} fichiers conformes aux hashes épinglés.")
    print("Contrôle de fraîcheur uniquement ; aucune promotion du statut public.")
    return 0


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
