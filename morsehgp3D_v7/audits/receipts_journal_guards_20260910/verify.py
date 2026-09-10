"""Verify sealed executions and exact counter expectations; no C++ or GCP."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
HEADER = "snapshot/morsehgp3D_v7/src/forest/full_coverage_certificate.hpp"
EXPECTED_FAILURES = {
    "birth_small": "birth_size.reject",
    "birth_interior_flag": "birth_interior.reject",
    "birth_multiple_refs": "birth_ref_count.reject",
    "unsorted_parents": "parent_order.reject",
    "reader_interior_always": "reader.shell_only.closed",
    "order_above_max": "order_max.reject",
    "domain_smaller_order": "domain_below_order.reason_only",
    "k1_missing_birth": "k1_count.reject",
    "reader_absent_root": "reader.absent.shell_only",
}


def need(value: object, reason: str) -> None:
    if not value:
        raise ValueError(reason)


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def read(name: str) -> object:
    return json.loads((BASE / name).read_text())


def compute() -> dict:
    refs = read("source_refs.json")
    for name, pin in refs["live_sources_before_after"].items():
        need(digest((BASE / "snapshot" / name).read_bytes()) == pin, "snapshot " + name)
    current = (BASE / HEADER).read_text()
    need(digest(current.encode()) == refs["wip_header_sha256"] ==
         "7608e70ec0bf7df7ed726ae2388a39e800ab2db35043b4ba42c619ceef13bac0", "WIP header")
    need(digest((BASE / "baseline_full_coverage_certificate.hpp").read_bytes()) ==
         refs["baseline_header_sha256"] ==
         "e8e65b211ec01964f35f1eb60587096a9b1e144d2c4634d2cad0c2f50ae5e883", "published baseline")
    mutations = read("mutations.json")
    need(set(mutations) == set(EXPECTED_FAILURES), "mutant inventory")
    headers = {"nominal_o2": refs["wip_header_sha256"], "nominal_san": refs["wip_header_sha256"],
               "baseline_o2": refs["baseline_header_sha256"]}
    for name, change in mutations.items():
        need(current.count(change["before"]) == 1, "single-site mutation " + name)
        headers[name] = digest(current.replace(change["before"], change["after"]).encode())
    built = read("build_capture/receipt.json")
    need(built["status"] == "passed" and built["sources_stable"] and built["headers"] == headers and
         set(built["binaries"]) == set(headers), "complete build")
    sources = read("build_capture/sources_before.json")
    need(sources == read("build_capture/sources_after.json"), "build source drift")
    for name, pin in sources.items():
        need(digest((BASE / name).read_bytes()) == pin, "build input " + name)
    commands = read("build_capture/commands.json")
    need(len(commands) == 13 and commands[0]["name"] == "compiler" and
         all(c["exit_code"] == 0 and c["ended_ns"] >= c["started_ns"] for c in commands),
         "closed compiler commands")
    shared = {"snapshot/" + name for name in refs["live_sources_before_after"] if
              not name.endswith("/full_coverage_certificate.hpp")}
    for command in commands[1:]:
        name = command["name"].removeprefix("compile_")
        probe = next(arg for arg in command["argv"] if arg.endswith("/guard_probe.cpp"))
        original_base = PurePosixPath(probe).parent
        dep = (BASE / "build_capture" / (name + ".d")).read_text().replace("\\\n", " ")
        paths = {str(PurePosixPath(posixpath.normpath(p)).relative_to(original_base))
                 for p in dep.split(":", 1)[1].split()}
        selected = HEADER if name.startswith("nominal_") else (
            "baseline_full_coverage_certificate.hpp" if name == "baseline_o2" else ".work_build/" + name + ".hpp")
        need(paths == shared | {"guard_probe.cpp", selected}, "compiled closure " + name)
    for attempt in ("r1", "r2"):
        root = "test_" + attempt + "/"
        receipt = read(root + "receipt.json")
        need(receipt["sources_stable"] and receipt["binaries"] == built["binaries"] and
             read(root + "sources_before.json") == sources == read(root + "sources_after.json"),
             "test inputs " + attempt)
        calls = read(root + "commands.json")
        need(len(calls) == 12 and {c["name"] for c in calls} == set(headers) and
             all(c["ended_ns"] >= c["started_ns"] for c in calls), "closed tests " + attempt)
        expected_exits = {name: (0 if name.endswith(("_o2", "_san")) else 1) for name in headers}
        if attempt == "r1":
            expected_exits["nominal_san"] = 1
            need(receipt["status"] == "failed" and
                 "LeakSanitizer does not work under ptrace" in (BASE / root / "nominal_san.stderr").read_text(),
                 "preserved ptrace failure")
        else:
            need(receipt["status"] == "passed", "successful retry")
        need(receipt["exits"] == expected_exits and
             {c["name"]: c["exit_code"] for c in calls} == expected_exits, "expected exits " + attempt)
        for name, why in EXPECTED_FAILURES.items():
            need((BASE / root / (name + ".stdout")).read_bytes() == b"" and
                 (BASE / root / (name + ".stderr")).read_text() == "FAIL " + why + "\n",
                 "causal mutant " + name)
        sanitizer = next(c for c in calls if c["name"] == "nominal_san")
        need(sanitizer["environment_delta"] == dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
             UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"), "sanitizer policy")
    old = read("test_r2/baseline_o2.stdout")
    new = read("test_r2/nominal_o2.stdout")
    fixed = dict(fixed_checks=42, valid_cases=10, reject_cases=9, reader_checks=6)
    need(old == dict(fixed, allocation_calls=14, allocation_rejections=14, malformed_first_fault_status=1),
         "baseline observations")
    need(new == dict(fixed, allocation_calls=5, allocation_rejections=5, malformed_first_fault_status=2),
         "reserved observations")
    need((BASE / "test_r2/nominal_o2.stdout").read_bytes() ==
         (BASE / "test_r2/nominal_san.stdout").read_bytes() and
         (BASE / "test_r2/nominal_san.stderr").read_bytes() == b"", "O2/SAN equality")
    cache_refs = read("cache_source_refs.json")
    for name, pin in cache_refs.items():
        need(digest((ROOT / name).read_bytes()) == pin, "second-auditor cache evidence " + name)
    capture = next(name for name in cache_refs if name.endswith(".stdout"))
    entry_bytes = json.loads((ROOT / capture).read_text())["entry_bytes"]
    need(entry_bytes == 48, "observed cache entry ABI")
    n = 10_000_000
    slots = 1 << (16 * n - 1).bit_length()
    return dict(schema="mhgp7-journal-guards-review-v1", fixed=fixed, mutants=EXPECTED_FAILURES,
                baseline_allocations=14, reserved_allocations=5, allocation_faults_each_rejected=True,
                malformed_first_fault_status=dict(baseline="InvalidInput", reserved="ResourceExhausted"),
                initial_sanitizer_attempt="failed_ptrace_preserved", retry="same_binaries_passed_with_leaks_enabled",
                compilations=12, executions=24, compiled_project_dependencies_each=10,
                cache_capacity_correction=dict(n=n, entry_bytes=entry_bytes, slots=slots,
                                               bytes=slots * entry_bytes, gib=slots * entry_bytes // 2**30,
                                               allocation_executed=False),
                scope="bounded_structural_journal_guards_on_pinned_WIP", public_status="not_claimed",
                geometric_engine_executed=False, gcp_used=False)


def main() -> None:
    for line in (BASE / "SHA256SUMS").read_text().splitlines():
        pin, name = line.split("  ", 1)
        need(digest((BASE / name).read_bytes()) == pin, "packet integrity " + name)
    result = compute()
    need(result == read("review.json"), "recomputed observations")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
