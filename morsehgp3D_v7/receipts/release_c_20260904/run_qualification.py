#!/usr/bin/env python3
"""Fresh CPU qualification C; inert unless explicitly executed after a campaign."""
from __future__ import annotations

import argparse
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import xml.etree.ElementTree as ET


OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]
SOURCE = ROOT / "morsehgp3D_v7"
BUILD = ROOT / "build/v7_c_qualification"
LIVE_CLI = ROOT / "build/v7/mhgp7"
BASELINE_B = ROOT / "build/v7_mono_baseline/mhgp7_B"
BENCH = SOURCE / "bench/compare_v6_v7.py"
HELPER = OUT.with_name("release_20260904") / "run_release.py"
REQUIRED_TESTS = {
    "mhgp7_mono_inline", "mhgp7_mono_inline_late_a", "mhgp7_mono_inline_late_b",
    "mhgp7_mono_inline_early_alloc", "mhgp7_axis_bounds",
    *("mhgp7_axis_bounds_" + name for name in (
        "axis-argmin-floor-only", "axis-argmin-ceil-always", "axis-argmin-no-clip",
        "axis-argmin-narrow-coefficient", "axis-argmin-max-min")),
    "mhgp7_archive_cleanup", "mhgp7_archive", "mhgp7_archive_optimized",
    "mhgp7_archive_api", "mhgp7_census_direct", "mhgp7_thread_failure",
    "mhgp7_silent_incidence", "mhgp7_incidence_campaign", "mhgp7_incidence_campaign_optimized",
    "mhgp7_compare_campaign", "mhgp7_compare_campaign_optimized",
}


def module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("module unavailable")
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


H = module("release_c_helpers", HELPER)
H.OUT = OUT
PAIR = module("release_c_pair_scope", BENCH)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def read(path: Path):
    return json.loads(path.read_text())


def validate_terminal_campaign(campaign: Path, snapshot: dict, binary: Path) -> dict:
    """A sealed terminal outcome is mandatory; censures need not be successes."""
    hashes = read(campaign / "hashes.json")
    require(isinstance(hashes, dict) and {"metadata.json", "summary.json", "runs.json", "runner.py"} <= hashes.keys(),
            "campaign terminal seal incomplete")
    for name, digest in hashes.items():
        require(isinstance(name, str) and Path(name).name == name and name not in (".", ".."), "campaign seal path")
        require(isinstance(digest, str) and re.fullmatch(r"[a-f0-9]{64}", digest) is not None, "campaign seal digest")
        path = campaign / name
        require(path.is_file() and not path.is_symlink() and H.digest(path) == digest, "campaign seal mismatch: " + name)
    meta, summary, runs = (read(campaign / name) for name in ("metadata.json", "summary.json", "runs.json"))
    require(summary.get("status") in ("completed", "invalid") and summary.get("source_stable") is True,
            "campaign not terminal on stable sources")
    require(isinstance(runs, list) and summary.get("runs") == len(runs), "campaign terminal count mismatch")
    require(meta.get("source_sha256") == snapshot, "campaign sources differ from qualification C")
    role = meta.get("binary_roles", {}).get("candidate", {})
    require(role.get("path") == str(binary) and role.get("wire_version") == "v7", "campaign candidate identity")
    require(meta.get("binaries", {}).get(str(binary)) == H.digest(binary), "campaign candidate bytes changed")
    require(meta.get("serial_stages_requested") is True, "expected serialized B/C campaign")
    return {"path": str(campaign), "seal_sha256": H.digest(campaign / "hashes.json"),
            "status": summary["status"], "runs": summary["runs"], "source_stable": True,
            "note": "terminal censures remain failures; this prerequisite does not promote the campaign"}


def active_measurements(proc: Path = Path("/proc")) -> list[dict]:
    """Read-only visible-process check, not a global host-idleness certificate."""
    found = []
    for process in proc.iterdir():
        if not process.name.isdigit() or int(process.name) == os.getpid():
            continue
        try:
            argv = [arg.decode(errors="replace") for arg in (process / "cmdline").read_bytes().split(b"\0") if arg]
            if not argv:
                continue
            if argv[0] in (str(LIVE_CLI), str(BASELINE_B)) or str(BENCH) in argv or \
                    "morsehgp3D_v7/bench/compare_v6_v7.py" in argv:
                found.append({"pid": int(process.name), "argv": argv})
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue
    return found


def commands(build_jobs: int, test_jobs: int) -> list[tuple[str, list[str], int]]:
    return [
        ("cmake_version", ["cmake", "--version"], 30),
        ("compiler_version", ["c++", "--version"], 30),
        ("configure", ["cmake", "-S", str(SOURCE), "-B", str(BUILD), "-DCMAKE_BUILD_TYPE=Release",
                       "-DMHGP7_ENABLE_CUDA=OFF", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"], 120),
        ("build_all", ["cmake", "--build", str(BUILD), "--parallel", str(build_jobs)], 3600),
        ("inventory", ["ctest", "--test-dir", str(BUILD), "--show-only=json-v1", "-L", "^gate$"], 60),
        ("ctest", ["ctest", "--test-dir", str(BUILD), "--output-on-failure", "--no-tests=error",
                   "-L", "^gate$", "--parallel", str(test_jobs), "--output-junit", str(OUT / "ctest.junit.xml")], 7200),
    ]


def product_binary_inventory() -> list[dict]:
    return [{"path": str(path.relative_to(ROOT)), "sha256": H.digest(path), "size": path.stat().st_size}
            for path in sorted(BUILD.glob("mhgp7*")) if path.is_file()]


def compile_binding() -> dict:
    compile_commands = read(BUILD / "compile_commands.json")
    require(isinstance(compile_commands, list) and compile_commands, "empty compilation inventory")
    product = [record for record in compile_commands if "CMakeFiles/mhgp7.dir/" in record.get("command", "")]
    require(len(product) == 1, "ambiguous product compilation command")
    command = product[0]["command"]
    require("-std=c++20" in command and "-O3" in command and "-DNDEBUG" in command,
            "product compilation is not the requested C++20 Release")
    require(not any(flag in command for flag in ("MHGP7_TESTING", "MHGP7_PROFILE", "-ffast-math", "-Ofast", "-fsanitize")),
            "instrumented or unsafe product compilation flags")
    require(Path(product[0]["directory"]).resolve() == BUILD, "product compiled outside isolated build")
    configuration = [BUILD / name for name in ("CMakeCache.txt", "CTestTestfile.cmake", "Makefile", "compile_commands.json")]
    configuration.extend(sorted((BUILD / "CMakeFiles").rglob("*.o.d")))
    configuration.extend(sorted((BUILD / "CMakeFiles").rglob("flags.make")))
    configuration.extend(sorted((BUILD / "CMakeFiles").rglob("link.txt")))
    return {"build_was_absent_before_attempt": True, "product_compile_command": product[0],
            "source_binary_binding": "fresh isolated build command and source hashes at phase boundaries, not immutable hermetic build",
            "configuration_files": [{"path": str(path.relative_to(ROOT)), "sha256": H.digest(path)} for path in configuration]}


def execute(args) -> int:
    require(not BUILD.exists(), "isolated build already exists; never overwrite or reuse it")
    require(not (OUT / "sources_before.json").exists(), "qualification receipt already attempted")
    require(args.after_campaign is not None, "--after-campaign is required for execution")
    campaign = args.after_campaign.resolve(strict=True)
    baseline = H.source_inventory()
    measured_scope = PAIR.source_snapshot(ROOT)
    protected = {str(path): H.digest(path) for path in (LIVE_CLI, BASELINE_B)}
    prerequisite = validate_terminal_campaign(campaign, measured_scope, LIVE_CLI)
    require(not active_measurements(), "a visible measurement process is still active; wait for root coordination")
    H.save("sources_before.json", baseline)
    H.save("measurement_source_scope.json", measured_scope)
    H.save("campaign_prerequisite.json", prerequisite)
    H.save("protected_binaries_before.json", protected)
    H.save("environment.json", {"schema": "mhgp7.local.release-c-fresh.v1", "public_status": "not_claimed",
           "scope": "fresh_cpu_release_gates_not_performance_not_cuda", "gcp": "not_used",
           "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
           "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
           "uname": list(os.uname()), "affinity_cpus": sorted(os.sched_getaffinity(0)),
           "runner_sha256": H.digest(Path(__file__)), "helper_sha256": H.digest(HELPER),
           "options": {key: str(value) if isinstance(value, Path) else value for key, value in vars(args).items()}})
    results, inventory, binding, tested = {}, {}, None, None
    failure = None
    try:
        BUILD.mkdir(mode=0o700)
        BUILD.chmod(0o700)
        for label, command, limit in commands(args.build_jobs, args.test_jobs):
            require(H.source_inventory() == baseline and PAIR.source_snapshot(ROOT) == measured_scope,
                    "sources changed before " + label)
            require(all(H.digest(Path(path)) == digest for path, digest in protected.items()), "protected binary changed")
            require(not active_measurements(), "measurement started before " + label)
            results[label] = H.run(label, command, limit)
            if results[label]["exit_code"]:
                raise RuntimeError(label + " failed")
            H.save("sources_after_" + label + ".json", H.source_inventory())
            if label == "build_all":
                tested = product_binary_inventory()
                H.save("binaries_tested.json", tested)
                binding = compile_binding()
                H.save("build_binding.json", binding)
            if label == "inventory":
                inventory = read(OUT / "inventory.stdout")
                names = {test["name"] for test in inventory.get("tests", [])}
                require(len(names) >= 292 and REQUIRED_TESTS <= names,
                        "qualification inventory incomplete: " + repr(sorted(REQUIRED_TESTS - names)))
                H.save("required_tests.json", sorted(REQUIRED_TESTS))
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
        print("FAILURE " + failure, flush=True)
    finally:
        after = H.source_inventory()
        final_binaries = product_binary_inventory()
        protected_after = {str(path): H.digest(path) for path in (LIVE_CLI, BASELINE_B)}
        H.save("sources_after.json", after)
        H.save("binaries_after.json", final_binaries)
        H.save("protected_binaries_after.json", protected_after)
        junit = None
        if (OUT / "ctest.junit.xml").exists():
            cases = ET.parse(OUT / "ctest.junit.xml").getroot().findall(".//testcase")
            junit = {"tests": len(cases), "names": sorted(case.get("name") for case in cases),
                     "failed": [case.get("name") for case in cases if case.find("failure") is not None or case.find("error") is not None],
                     "skipped": [case.get("name") for case in cases if case.find("skipped") is not None]}
        inventory_names = {test["name"] for test in inventory.get("tests", [])}
        passed = not failure and len(results) == len(commands(args.build_jobs, args.test_jobs)) and \
            all(record["exit_code"] == 0 for record in results.values()) and baseline == after and \
            measured_scope == PAIR.source_snapshot(ROOT) and protected_after == protected and \
            tested == final_binaries and binding is not None and junit is not None and \
            set(junit["names"]) == inventory_names and not junit["failed"] and not junit["skipped"]
        fresh_cli = BUILD / "mhgp7"
        fresh_sha = H.digest(fresh_cli) if fresh_cli.exists() else None
        summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
                   "public_status": "not_claimed", "gcp": "not_used", "sources_stable": baseline == after,
                   "protected_binaries_unchanged": protected_after == protected,
                   "qualification_binaries_stable": tested == final_binaries,
                   "fresh_product_sha256": fresh_sha, "measured_product_sha256": protected[str(LIVE_CLI)],
                   "fresh_product_identical_to_measured": fresh_sha == protected[str(LIVE_CLI)],
                   "qualification": "fresh full CPU gate execution; no reused gate result",
                   "measurement_transfer": "requires matching binary SHA; this is not an SLO certificate",
                   "junit": junit, "results": results}
        H.save("summary.json", summary)
        H.save("receipt_manifest.json", [{"path": path.name, "sha256": H.digest(path), "size": path.stat().st_size}
               for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
        print(json.dumps({key: value for key, value in summary.items() if key != "results"}), flush=True)
    return 0 if passed else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true", help="use only after explicit root GO and completed B/C measurements")
    parser.add_argument("--after-campaign", type=Path)
    parser.add_argument("--build-jobs", type=int, default=2)
    parser.add_argument("--test-jobs", type=int, default=2)
    args = parser.parse_args()
    require(1 <= args.build_jobs <= 4 and 1 <= args.test_jobs <= 4, "jobs outside bounded domain1..4")
    if not args.execute:
        print(json.dumps({"status": "prepared_not_executed", "build": str(BUILD),
                          "required_tests": sorted(REQUIRED_TESTS),
                          "commands": commands(args.build_jobs, args.test_jobs), "gcp": "not_used"}, indent=2))
        return 0
    return execute(args)


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
