"""Inspect published CPU receipts independently, without rerun or promotion."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
V7 = ROOT / "morsehgp3D_v7"
RECEIPTS = V7 / "receipts"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path: Path) -> object:
    return json.loads(path.read_text(), object_pairs_hook=unique_object)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def junit(data: bytes, expected: list[str]) -> dict[str, object]:
    require(
        bool(expected) and len(set(expected)) == len(expected),
        "bad expected names",
    )
    root = ET.fromstring(data)
    require(root.tag == "testsuite", "unexpected JUnit root")
    cases = root.findall("testcase")
    names = [case.get("name") for case in cases]
    require(len(names) == len(set(names)), "duplicate JUnit names")
    require(set(names) == set(expected), "JUnit names mismatch")
    require(int(root.get("tests", "-1")) == len(cases), "JUnit count mismatch")
    for field in ("failures", "errors", "skipped"):
        require(int(root.get(field, "0")) == 0, f"JUnit {field}")
    for case in cases:
        require(case.get("status") == "run", "JUnit status not run")
        require(
            not any(
                case.findall(tag) for tag in ("failure", "error", "skipped")
            ),
            "JUnit failure/error/skipped element",
        )
    return {"tests": len(cases), "unique_names": True, "all_run": True,
            "failure_error_skipped": 0, "names": sorted(names)}


def self_test() -> dict[str, object]:
    nominal = (
        b'<testsuite tests="1"><testcase name="x" status="run"/></testsuite>'
    )
    require(junit(nominal, ["x"])["tests"] == 1, "positive self-test")
    mutants = {
        "empty": b'<testsuite tests="0"/>',
        "duplicate": nominal.replace(
            b'</testsuite>', b'<testcase name="x" status="run"/></testsuite>'
        ),
        "missing_name": nominal.replace(b'name="x"', b'name="y"'),
        "status": nominal.replace(b'status="run"', b'status="notrun"'),
        "count": nominal.replace(b'tests="1"', b'tests="2"'),
        "failure": nominal.replace(b'/>', b'><failure/></testcase>'),
        "error": nominal.replace(b'/>', b'><error/></testcase>'),
        "skipped": nominal.replace(b'/>', b'><skipped/></testcase>'),
        "summary_failure": nominal.replace(
            b'tests="1"', b'tests="1" failures="1"'
        ),
    }
    for name, mutant in mutants.items():
        try:
            junit(mutant, ["x"])
        except ValueError:
            continue
        raise ValueError(f"surviving JUnit mutant: {name}")
    return {"positive": 1, "rejected": sorted(mutants), "assert_used": False}


def inventory_check(
    entries: list[dict[str, object]], base: Path
) -> dict[str, object]:
    require(bool(entries), "empty file inventory")
    paths = [item["path"] for item in entries]
    require(len(paths) == len(set(paths)), "duplicate inventory path")
    mismatched = []
    missing = []
    for item in entries:
        path = base / item["path"]
        if not path.is_file():
            missing.append(item["path"])
        elif (
            digest(path) != item["sha256"]
            or path.stat().st_size != item.get("size", item.get("bytes"))
        ):
            mismatched.append(item["path"])
    return {
        "count": len(entries), "missing": missing, "mismatched": mismatched
    }


def inspect() -> dict[str, object]:
    result = {
        "schema": "mhgp7-independent-qualification-inspection-v1",
        "captured_utc": datetime.now(timezone.utc).isoformat(),
        "source_head": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip(),
        "branch": subprocess.check_output(
            ["git", "branch", "--show-current"], cwd=ROOT, text=True
        ).strip(),
        "phase": "exploration_v7_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only",
        "mode": "audit_independant_math_and_architecture",
        "public_status": "not_claimed", "gcp": "not_used",
        "product_tests_executed_by_this_inspection": 0,
        "self_test": self_test(), "seals": {}, "campaigns": {},
        "input_sha256": {},
    }
    for path in (Path(__file__), V7 / "audits/verify_current.py",
                 V7 / "audits/receipts_20260904/validation_current.json"):
        result["input_sha256"][str(path.relative_to(ROOT))] = digest(path)
    for name in (
        "meb_full_release_20260905", "meb_lazy_integrated_20260905",
        "arithmetic_gates_20260904",
    ):
        folder = RECEIPTS / name
        entries = []
        for line in (folder / "SHA256SUMS").read_text().splitlines():
            expected, relative = line.split(maxsplit=1)
            relative = relative.removeprefix("*")
            base = ROOT if relative.startswith("morsehgp3D_v7/") else folder
            path = base / relative
            require(
                path.resolve().is_relative_to(folder.resolve()),
                "seal path traversal",
            )
            require(
                path.is_file() and digest(path) == expected,
                f"bad seal: {path}",
            )
            entries.append(str(path.relative_to(folder)))
        require(
            bool(entries) and len(set(entries)) == len(entries),
            "empty/duplicate seal",
        )
        actual = {
            str(path.relative_to(folder))
            for path in folder.rglob("*") if path.is_file()
        }
        require(
            actual == set(entries) | {"SHA256SUMS"}, f"seal coverage: {folder}"
        )
        result["seals"][name] = {
            "files": len(entries), "complete_public_coverage": True
        }
        seal = folder / "SHA256SUMS"
        result["input_sha256"][str(seal.relative_to(ROOT))] = digest(seal)
    boost = RECEIPTS / "arithmetic_boost_20260904"
    entries = read_json(boost / "manifest.json")["files"]
    checked = inventory_check(entries, boost)
    require(
        not checked["missing"] and not checked["mismatched"],
        "Boost public seal",
    )
    actual = {
        str(path.relative_to(boost))
        for path in boost.rglob("*") if path.is_file()
    }
    require(
        actual == {item["path"] for item in entries} | {"manifest.json"},
        "Boost seal coverage",
    )
    result["seals"][boost.name] = checked
    seal = boost / "manifest.json"
    result["input_sha256"][str(seal.relative_to(ROOT))] = digest(seal)

    configurations = (
        (
            "meb_full_release_20260905/full_release", 323,
            "../expected_test_names.json",
        ),
        ("meb_lazy_integrated_20260905/release", 32, "expected_names.json"),
        ("meb_lazy_integrated_20260905/sanitized", 32, "expected_names.json"),
        (
            "arithmetic_gates_20260904/full_release", 316,
            "../expected_test_names.json",
        ),
    )
    for name, count, expectation in configurations:
        folder = RECEIPTS / name
        expected = read_json(folder / expectation)
        require(len(expected) == count, "expected count drift")
        verdict = junit((folder / "ctest.junit.xml").read_bytes(), expected)
        inventory = read_json(folder / "inventory.stdout")["tests"]
        observed = [item["name"] for item in inventory]
        require(
            len(observed) == len(set(observed))
            and set(observed) == set(expected), "inventory mismatch",
        )
        sources = read_json(folder / "sources_before.json")
        binaries = read_json(folder / "binaries_tested.json")
        require(
            sources == read_json(folder / "sources_after.json"),
            "source drift during campaign",
        )
        require(
            binaries == read_json(folder / "binaries_after.json"),
            "binary drift during campaign",
        )
        for snapshot in folder.glob("sources_after_*.json"):
            require(
                sources == read_json(snapshot),
                f"intermediate source drift: {snapshot}",
            )
        summary = read_json(folder / "summary.json")
        require(
            summary["status"] == "passed" and summary["failure"] is None,
            "campaign failed",
        )
        for stage, command in summary["results"].items():
            require(
                command["exit_code"] == 0 and command["status"] == "completed",
                f"failed command: {stage}",
            )
        verdict.update({
            "sources_during_campaign_stable": True,
            "binaries_during_campaign_stable": True,
            "sources_current": inventory_check(sources, ROOT),
            "binaries_current": inventory_check(binaries, ROOT),
            "historical_commands": summary["results"],
        })
        if name.startswith("meb_"):
            for key in ("sources_current", "binaries_current"):
                require(
                    not verdict[key]["missing"]
                    and not verdict[key]["mismatched"], f"D freshness: {key}",
                )
        result["campaigns"][name] = verdict

    arithmetic = RECEIPTS / "arithmetic_gates_20260904"
    inventory = read_json(arithmetic / "targeted.inventory.json")["tests"]
    expected_arithmetic = [item["name"] for item in inventory]
    require(len(expected_arithmetic) == 24, "arithmetic targeted count")
    for mode in ("release", "sanitized"):
        result["campaigns"][f"arithmetic_targeted_{mode}"] = junit(
            (arithmetic / f"targeted.{mode}.junit.xml").read_bytes(),
            expected_arithmetic,
        )
    result["arithmetic_targeted_binaries_current"] = {}
    bindings = read_json(arithmetic / "targeted.build_binding.json")["builds"]
    for mode, targets in bindings.items():
        for name, binding in targets.items():
            require(
                digest(ROOT / binding["binary_path"])
                == binding["binary_sha256"], "arithmetic binary changed",
            )
            key = f"{mode}/{name}"
            result["arithmetic_targeted_binaries_current"][key] = (
                binding["binary_sha256"]
            )
    for kind, count in (("integer", 8), ("lanes", 16)):
        names = [
            name for name in expected_arithmetic
            if (
                name.startswith("mhgp7_arithmetic_integer")
                or name == "mhgp7_arithmetic_u320_word4"
            ) == (kind == "integer")
        ]
        require(len(names) == count, "Boost expected count")
        result["campaigns"][f"boost_{kind}"] = junit(
            (boost / f"{kind}.junit.xml").read_bytes(), names
        )

    provenance = read_json(boost / "provenance.json")
    compiled_branch = provenance["compiled_branch"]
    for key in ("macro_dump", "actual_object_dependencies"):
        entry = compiled_branch[key]
        require(
            digest(ROOT / entry["original_path"]) == entry["sha256"],
            f"Boost private {key} changed",
        )
    macro_path = ROOT / compiled_branch["macro_dump"]["original_path"]
    macros = macro_path.read_text()
    dependency_record = compiled_branch["actual_object_dependencies"]
    dependencies = (ROOT / dependency_record["original_path"]).read_text()
    require(
        "#define INTEGER_GATE_BOOST 1" in macros
        and "#define BOOST_VERSION 108300" in macros, "Boost macro authority",
    )
    for header in dependency_record["required_private_headers"]:
        require(
            "build/v7_boost_gate/extracted/usr/include/" + header
            in dependencies, "Boost object dependency",
        )
    boost_binaries = read_json(boost / "binaries.json")
    for name, entry in boost_binaries.items():
        path = ROOT / "build/v7_boost_gate/build/v7" / name
        require(
            digest(path) == entry["sha256"]
            and path.stat().st_size == entry["bytes"], "Boost binary binding",
        )
    require(
        "authority=obig_literals_and_boost"
        in (boost / "integer_authority.stdout").read_text(),
        "Boost runtime authority",
    )
    result["boost_authority"] = {
        "INTEGER_GATE_BOOST": 1, "BOOST_VERSION": 108300,
        "actual_private_macro_and_dependency_pins_verified": True,
        "binaries_current": boost_binaries,
        "lanes_authority": "OBig_and_literals_without_Boost",
    }
    full_d = RECEIPTS / "meb_full_release_20260905"
    targeted_d = RECEIPTS / "meb_lazy_integrated_20260905"
    binding = read_json(full_d / "full_release/build_binding.json")
    for path, expected in binding["configuration"].items():
        require(
            digest(ROOT / path) == expected, "D build configuration changed"
        )
    require(
        digest(Path(binding["product_path"])) == binding["product_sha256"],
        "D CLI binding",
    )
    result["D_build_binding_current"] = binding
    result["sanitizer_effective_options_receipt"] = read_json(
        targeted_d / "sanitizer_effective_options.json"
    )

    compression = read_json(full_d / "LastTest.compression.json")
    data = gzip.decompress((ROOT / compression["destination"]).read_bytes())
    require(
        len(data) == compression["decompressed_bytes"]
        and hashlib.sha256(data).hexdigest()
        == compression["decompressed_sha256"], "compressed log integrity",
    )
    require(
        data == (ROOT / compression["source"]).read_bytes(),
        "original full log differs",
    )
    result["full_log"] = {
        "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
        "original_identical": True,
    }
    result["original_public_copies"] = {}
    pairs = read_json(full_d / "copy_map.json")
    transformations = {}
    for item in pairs:
        source, destination = ROOT / item["source"], ROOT / item["destination"]
        transformation = item["projection"]
        require(
            transformation in ("identity", "append_single_LF"),
            "unknown full export transformation",
        )
        require(
            destination.read_bytes() == source.read_bytes()
            + (b"\n" if transformation == "append_single_LF" else b""),
            "full export mismatch",
        )
        transformations[transformation] = (
            transformations.get(transformation, 0) + 1
        )
    result["original_public_copies"]["full_D"] = transformations
    pairs = read_json(targeted_d / "export_manifest.json")["files"]
    transformations = {}
    for item in pairs:
        source = (ROOT / item["source"]).read_bytes()
        destination = (targeted_d / item["published"]).read_bytes()
        transformation = item["transformation"]
        require(
            transformation in ("copy_bytes", "append_LF"),
            "unknown export transformation",
        )
        require(
            destination == source
            + (b"\n" if transformation == "append_LF" else b""),
            "targeted export mismatch",
        )
        transformations[transformation] = (
            transformations.get(transformation, 0) + 1
        )
    result["original_public_copies"]["targeted_D"] = transformations
    result["prior_audit_freshness"] = {}
    for optimize in (False, True):
        command = [sys.executable, "-B"] + (["-O"] if optimize else [])
        command.append(str(V7 / "audits/verify_current.py"))
        completed = subprocess.run(
            command, cwd=ROOT, capture_output=True, text=True,
            timeout=30, check=False,
        )
        require(completed.returncode in (0, 1), "prior audit manifest invalid")
        key = "optimized" if optimize else "normal"
        result["prior_audit_freshness"][key] = {
            "argv": command, "exit_code": completed.returncode,
            "stdout": completed.stdout, "stderr": completed.stderr,
        }
    result["status"] = "published_receipts_integrity_and_D_binding_verified"
    result["limitations"] = [
        "No product test rerun in this inspection", "No hermetic rebuild",
        "No global mathematical exactness certificate",
        "No SLO or GPU qualification",
        "Historic C source pins are not D qualification",
        "Audit freshness is distinct from product qualification",
    ]
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    arguments = parser.parse_args()
    try:
        result = self_test() if arguments.self_test else inspect()
    except (OSError, ValueError, TypeError, KeyError, ET.ParseError) as error:
        print(f"qualification inspection failed: {error}", file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
