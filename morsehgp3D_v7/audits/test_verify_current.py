"""Test complete source variants and reject malformed freshness manifests."""

from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


AUDITS = Path(__file__).resolve().parent
LINEAGE = AUDITS.parent
VERIFIER = AUDITS / "verify_current.py"
RECEIPTS = AUDITS / "receipts_front_20260905"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def run() -> dict[str, object]:
    RECEIPTS.mkdir(exist_ok=True)
    results = []
    with tempfile.TemporaryDirectory(
        dir=RECEIPTS, prefix="freshness_fixture_"
    ) as temporary:
        folder = Path(temporary)
        common_file = folder / "common.txt"
        first = folder / "first.txt"
        second = folder / "second.txt"
        manifest_path = folder / "manifest.json"
        common = str(common_file.relative_to(LINEAGE))
        a = str(first.relative_to(LINEAGE))
        b = str(second.relative_to(LINEAGE))
        common_file.write_bytes(b"common")
        first.write_bytes(b"D")
        second.write_bytes(b"D")
        manifest = {
            "schema": "mhgp7-current-audit-validation-v2",
            "common_pins": {common: sha(b"common")},
            "source_variants": [
                {
                    "id": "D",
                    "pinned_sources": {a: sha(b"D"), b: sha(b"D")},
                    "qualification_scope": "fixture_complete_D",
                },
                {
                    "id": "E",
                    "pinned_sources": {a: sha(b"E"), b: sha(b"E")},
                    "qualification_scope": "fixture_targeted_E_only",
                },
            ],
        }

        def expect(
            name: str, value: object, code: int, marker: str = "",
            raw: bool = False,
        ) -> None:
            manifest_path.write_text(
                value if raw else json.dumps(value), encoding="utf-8"
            )
            command = [sys.executable, "-B"]
            if sys.flags.optimize:
                command.append("-O")
            command += [str(VERIFIER), "--manifest", str(manifest_path)]
            completed = subprocess.run(
                command, capture_output=True, text=True, timeout=10,
                check=False,
            )
            require(
                completed.returncode == code,
                f"{name}: expected {code}, got {completed.returncode}: "
                + completed.stderr,
            )
            require(
                marker in completed.stdout + completed.stderr,
                f"{name}: missing marker {marker}",
            )
            results.append({
                "name": name, "expected_code": code,
                "observed_code": completed.returncode,
                "required_marker": marker,
            })

        expect("complete_D", manifest, 0, "variante=D")
        first.write_bytes(b"E")
        second.write_bytes(b"E")
        expect("complete_E", manifest, 0, "fixture_targeted_E_only")
        first.write_bytes(b"D")
        expect("mixed_D_E_rejected", manifest, 1, "aucune variante complète")
        first.write_bytes(b"E")
        second.write_bytes(b"D")
        expect("mixed_E_D_rejected", manifest, 1, "aucune variante complète")
        first.write_bytes(b"D")
        common_file.write_bytes(b"changed")
        expect("common_changed", manifest, 1, "pins communs modifiés")
        common_file.write_bytes(b"common")
        second.unlink()
        expect("source_missing", manifest, 1)
        second.write_bytes(b"D")
        old = {
            "schema": "mhgp7-current-audit-validation-v1",
            "pinned_sources": {a: sha(b"D")},
        }
        expect("legacy_v1", old, 0, "Audit courant")
        first.write_bytes(b"unknown")
        expect("legacy_v1_stale", old, 1, "Audit à actualiser")
        first.write_bytes(b"D")

        malformed = copy.deepcopy(manifest)
        malformed["source_variants"][1]["id"] = "D"
        expect("duplicate_variant_id", malformed, 2, "duplicate")
        malformed = copy.deepcopy(manifest)
        malformed["source_variants"][1]["pinned_sources"] = {a: "bad"}
        expect("malformed_unmatched_E", malformed, 2, "invalid SHA-256")
        malformed = copy.deepcopy(manifest)
        malformed["source_variants"][1]["pinned_sources"] = {}
        expect("empty_unmatched_variant", malformed, 2, "nonempty")
        malformed = copy.deepcopy(manifest)
        malformed["common_pins"] = {}
        expect("empty_common", malformed, 2, "nonempty")
        malformed = copy.deepcopy(manifest)
        malformed["source_variants"] = []
        expect("empty_variants", malformed, 2, "nonempty")
        malformed = copy.deepcopy(manifest)
        malformed["source_variants"] = ["D"]
        expect("nonobject_variant", malformed, 2, "object")
        for label, identifier in (("empty", ""), ("space", "D E")):
            malformed = copy.deepcopy(manifest)
            malformed["source_variants"][0]["id"] = identifier
            expect(f"invalid_id_{label}", malformed, 2, "identifier")
        for label, scope in (("empty", ""), ("nonstring", {})):
            malformed = copy.deepcopy(manifest)
            malformed["source_variants"][1]["qualification_scope"] = scope
            expect(f"invalid_scope_{label}", malformed, 2, "scope")
        for label, path in (
            ("traversal", "../outside"),
            ("absolute", str(first)),
            ("alias", "./" + a),
            ("backslash", "audits\\outside"),
        ):
            malformed = copy.deepcopy(manifest)
            malformed["source_variants"][1]["pinned_sources"] = {
                path: sha(b"D")
            }
            expect(f"invalid_path_{label}", malformed, 2, "source path")
        symlink = folder / "outside"
        symlink.symlink_to(LINEAGE.parent / "AGENTS.md")
        malformed = copy.deepcopy(manifest)
        malformed["common_pins"] = {
            str(symlink.relative_to(LINEAGE)): sha(b"D")
        }
        expect("symlink_escape", malformed, 2, "outside v7")
        malformed = copy.deepcopy(manifest)
        malformed["source_variants"][1]["pinned_sources"][common] = sha(b"E")
        expect("common_variant_conflict", malformed, 2, "conflicting")
        raw = json.dumps(manifest)
        duplicate = raw.replace(
            '"schema":', '"schema": "ignored", "schema":', 1
        )
        expect("duplicate_root_json_key", duplicate, 2, "duplicate", True)
        pin = json.dumps(a) + ": " + json.dumps(sha(b"D"))
        duplicate = raw.replace(pin, pin + ", " + pin, 1)
        expect("duplicate_nested_pin", duplicate, 2, "duplicate", True)
        expect("unsupported_schema", {"schema": "unknown"}, 2)
        expect("nonobject_manifest", [], 2, "object")
        expect("invalid_json", "{", 2, raw=True)
        expect(
            "legacy_v1_empty",
            {"schema": old["schema"], "pinned_sources": {}},
            2, "nonempty",
        )
        require(len(results) == 30, "self-test nonvacuity floor")
    return {
        "schema": "mhgp7-freshness-verifier-selftest-v1",
        "status": "passed", "public_status": "not_claimed",
        "optimized": bool(sys.flags.optimize), "tests": len(results),
        "product_tests_executed": 0, "results": results,
        "verifier_sha256": sha(VERIFIER.read_bytes()),
        "test_sha256": sha(Path(__file__).read_bytes()),
        "gcp": "not_used",
    }


def main() -> int:
    try:
        result = run()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"freshness self-test failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
