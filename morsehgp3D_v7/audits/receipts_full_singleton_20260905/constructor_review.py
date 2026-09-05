"""Read-only cross-check of the constructor's singleton qualification captures."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import shlex
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
PACKET = ROOT / "morsehgp3D_v7/receipts/full_gabriel_singleton_20260905"
SOURCE = "21b77d29a4ba2bca453b602a8faa4564a978f4ba71af5167c164faae4ef0e1a5"
RECEIPT = "e3b64a03bbeeac6dfd773d9c54c2c559d6fc7e0d1630b7557a4a32734a91ea97"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(name: str) -> object:
    return json.loads((PACKET / name).read_text())


def review() -> dict:
    inventory = read("manifest.json")
    require(set(inventory) | {"manifest.json", "SHA256SUMS"} == {
        str(p.relative_to(PACKET)) for p in PACKET.rglob("*") if p.is_file()
    }, "packet inventory closure")
    for relative, identity in inventory.items():
        path = PACKET / relative
        require(path.resolve().is_relative_to(PACKET), "inventory escape")
        require(sha(path) == identity["sha256"], relative)
        require(path.stat().st_size == identity["bytes"], relative + " size")
    sums = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, relative = line.split("  ", 1)
        require(relative not in sums, "duplicate checksum")
        path = PACKET / relative
        require(path.resolve().is_relative_to(PACKET), "checksum escape")
        require(sha(path) == digest, relative)
        sums[relative] = digest
    require(len(sums) == len(inventory) + 1, "checksum closure")
    receipt = read("qualification/receipt.json")
    publication = read("publication.json")
    require(sha(PACKET / "qualification/receipt.json") == RECEIPT, "receipt pin")
    require(publication["capture_receipt_sha256"] == RECEIPT, "publication binding")
    require(receipt["status"] == "completed", "completion")
    require(receipt["producer_sha256"] == publication["producer_sha256"] == SOURCE,
            "producer binding")
    sources = read("qualification/sources_before.json")
    require(sources == read("qualification/sources_after.json"), "source drift")
    require(sha(PACKET / "qualification/sources_before.json") == receipt["source_sha256"],
            "source map binding")
    require(len(sources) == 584, "source floor")
    for relative, digest in sources.items():
        require(sha(ROOT / relative) == digest, relative + " current source drift")
    tests = receipt["tests"]
    require(len(tests) == 17 and len(receipt["targets"]) == 7, "test floor")
    phases = {}
    for mode in ("release", "san"):
        prefix = f"qualification/{mode}/"
        summary = read(prefix + "summary.json")
        for command in summary["commands"]:
            require(command["exit_code"] == command["expected_code"] == 0,
                    mode + " command exit")
            require(command["status"] == "completed" and command["error"] is None,
                    mode + " command completion")
            for stream, identity in command["raw"].items():
                require(sha(PACKET / prefix / (command["label"] + "." + stream))
                        == identity["sha256"], mode + " command raw binding")
        binaries = read(prefix + "binaries.json")
        require(binaries == read(prefix + "binaries_after.json"), "binary drift")
        require(set(binaries) == set(receipt["targets"]), "binary inventory")
        bindings = read(prefix + "compile_binding.json")["targets"]
        for target, binding in bindings.items():
            argv = shlex.split(binding["compile"]["command"])
            require(all(flag in argv for flag in (
                "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"
            )), "compiler contract")
            require(("-DMHGP7_TESTING=1" in argv) ==
                    (target == "mhgp7_full_gabriel_singleton_gate"), "testing scope")
            if mode == "san":
                require("-fsanitize=address,undefined" in argv, "sanitizer compile")
            for relative, digest in binding["project_dependencies"].items():
                require(sources[relative] == digest, "dependency source binding")
        rows = read(prefix + "inventory.stdout")["tests"]
        require(len(rows) == 17 and {r["name"] for r in rows} == set(tests), "CTest inventory")
        for row in rows:
            binary, arg, code = tests[row["name"]]
            argv = row["command"]
            require(f"-DEXPECTED={code}" in argv and f"-DARGS={arg}" in argv,
                    "test argument/code")
            require("-DCMD=" + binaries[binary]["path"] in argv, "test binary")
        junit = ET.fromstring((PACKET / prefix / "ctest.junit.xml").read_bytes())
        require(junit.get("tests") == "17" and all(junit.get(k) == "0" for k in
                ("failures", "disabled", "skipped")), "JUnit aggregate")
        cases = junit.findall("testcase")
        require({c.get("name") for c in cases} == set(tests) and len(cases) == 17,
                "JUnit names")
        require(all(c.get("status") == "run" and c.find("failure") is None
                    for c in cases), "JUnit execution")
        raw = (PACKET / prefix / "LastTest.stdout").read_text()
        lines = [line for line in raw.splitlines() if line.startswith("full_")
                 and "floor=" in line]
        require(len(lines) == 11, "untruncated summary floor")
        require(all(" failures=0 floor=1" in line for line in lines), "test floors")
        reports = {}
        for label in ("full_gabriel_allocation", "full_gabriel_lazy_allocation",
                      "full_gabriel_singleton mode=--selftest",
                      "full_gabriel_singleton mode=--rejects"):
            selected = [line for line in lines if line.startswith(label + " ")]
            require(len(selected) == 1, "summary uniqueness")
            reports[label] = {k: int(v) for k, v in re.findall(r"(\w+)=(\d+)", selected[0])}
        for label, count in (("full_gabriel_allocation", 49),
                             ("full_gabriel_lazy_allocation", 209)):
            row = reports[label]
            require(row["allocations"] == row["fault_runs"] == row["denied"] == count
                    and row["escaped"] == 0, "allocation floor")
        require(reports["full_gabriel_singleton mode=--selftest"]["pairs"] == 181,
                "nominal pairs")
        require(reports["full_gabriel_singleton mode=--rejects"]["refused"] == 357,
                "refusal pairs")
        if mode == "san":
            environment = read(prefix + "environment.json")
            require(environment["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1",
                    "LeakSanitizer")
        phases[mode] = {"tests": 17, "binaries": 7, "reports": reports}
    require(phases["release"] == phases["san"], "cross-build semantic summaries")
    return {"status": "pass", "scope": "constructor captures inspected, no engine rerun",
            "packet": str(PACKET.relative_to(ROOT)), "receipt_sha256": RECEIPT,
            "producer_sha256": SOURCE, "inventory_entries": len(inventory),
            "checksum_entries": len(sums), "source_pins": len(sources),
            "v7_source_pins": 62, "boost_headers": 521,
            "historical_depfile_inventory_only": 1, "phases": phases,
            "fresh_not_hermetic": True, "performance_qualified": False,
            "public_status": "not_claimed", "gcp": "not_used"}


if __name__ == "__main__":
    print(json.dumps(review(), indent=2, sort_keys=True))
