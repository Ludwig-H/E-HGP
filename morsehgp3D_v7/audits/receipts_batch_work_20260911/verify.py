"""Verify the sealed accounting witness without executing C++ or compiling."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile


HERE = Path(__file__).resolve().parent
LINEAGE = HERE.parent.parent


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def main() -> None:
    names = {"README.md", "adapter_gate.cpp", "adapter_fix.patch", "source_pins.json",
             "source_snapshot.tar.gz", "boundary.json", "record.py", "verify.py"}
    seal = {}
    for line in (HERE / "SHA256SUMS").read_text().splitlines():
        expected, name = line.split("  ", 1)
        require(name in names and name not in seal, "invalid seal name")
        seal[name] = expected
        require(digest((HERE / name).read_bytes()) == expected, "seal changed: " + name)
    require(set(seal) == names, "incomplete seal")
    pins = json.loads((HERE / "source_pins.json").read_text())
    archived = {}
    with tarfile.open(HERE / "source_snapshot.tar.gz", "r:gz") as archive:
        for member in archive.getmembers():
            require(member.isfile() and member.name not in archived, "invalid archive member")
            stream = archive.extractfile(member)
            require(stream is not None, "missing member data")
            archived[member.name] = digest(stream.read())
    expected_archive = {"original/" + name: sha for name, sha in pins["files"].items()}
    expected_archive["fixed_adapter.hpp"] = pins["fixed_adapter"]["sha256"]
    require(archived == expected_archive, "archive closure mismatch")
    expected_sources = dict(expected_archive)
    expected_sources.update({"fixed/" + name: sha for name, sha in pins["files"].items()})
    expected_sources["fixed/batch_adapter.hpp"] = pins["fixed_adapter"]["sha256"]
    report = json.loads((HERE / "boundary.json").read_text())
    require(report["schema"] == "mhgp7-adapter-work-boundary-v1", "schema")
    require(report["source_archive_sha256"] == seal["source_snapshot.tar.gz"]
            and report["gate_sha256"] == seal["adapter_gate.cpp"], "compiled input mismatch")
    require(report["sources_before"] == report["sources_after"] == expected_sources,
            "compiled source snapshot mismatch")
    require(report["status"] == "passed_expected_original_failure_and_fix", "capture failed")
    require(all(report[field] is False for field in ("geometry_executed", "device_executed", "gcp_used")),
            "unexpected execution claim")
    runs = report["runs"]
    require(len(runs) == 8, "command floor")
    seen = set()
    observations = {}
    for row in runs:
        key = row["mode"], row["variant"], row["kind"]
        require(key not in seen, "duplicate command")
        seen.add(key)
        mode, variant, kind = key
        require(mode in ("O2", "SAN") and variant in ("original", "fixed")
                and kind in ("compile", "run"), "command identity")
        expected_exit = 1 if variant == "original" and kind == "run" else 0
        require(row["exit_code"] == row["expected_exit_code"] == expected_exit
                and row["stderr"] == "", "unexpected exit or diagnostic")
        if mode == "SAN":
            require(row["sanitizer_env"] == {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "sanitizer options")
        if kind == "compile":
            flags = {"-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"}
            flags |= {"-O2"} if mode == "O2" else {"-O1", "-g", "-fsanitize=address,undefined",
                                                   "-fno-omit-frame-pointer"}
            require(flags <= set(row["argv"]), "compile flags")
        else:
            require(row["argv"][-1] == "--selftest", "gate entrypoint")
            result = json.loads(row["stdout"])
            require(result == {"positive_boundary_unknown_cases_pass": True, "overflow_thrown": True,
                    "overflow_work_known": variant == "original",
                    "overflow_partial_calls": 2 if variant == "original" else 0,
                    "overflow_partial_q2": (2**64 - 1) if variant == "original" else 0,
                    "overflow_public_targets": 0, "contract_pass": variant == "fixed",
                    "geometry_executed": False, "device_executed": False}, "witness mismatch")
            observations[mode, variant] = result
    for variant in ("original", "fixed"):
        require(observations["O2", variant] == observations["SAN", variant], "O2/SAN mismatch")
    for package in pins["published_receipts"]:
        for field in ("reader", "manifest"):
            require(digest((LINEAGE / package[field]).read_bytes()) == package[field + "_sha256"],
                    "published receipt pin")
        command = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
        result = subprocess.run(command + [str(LINEAGE / package["reader"])],
                                capture_output=True, check=False)
        require(result.returncode == 0 and not result.stderr, "published reader failed")
    print(json.dumps({"status": "passed_batch_work_boundary", "commands": 8,
                      "original_contract_failures": 2, "fixed_contract_successes": 2,
                      "published_readers": 2, "source_headers": len(pins["files"]),
                      "geometry_executed": False, "device_executed": False,
                      "public_status": "not_claimed", "gcp_used": False}, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError) as error:
        print("batch accounting audit failed: " + str(error), file=sys.stderr)
        raise SystemExit(1)
