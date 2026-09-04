#!/usr/bin/env python3
"""Local final Release qualification; never builds or relinks the product CLI."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
import xml.etree.ElementTree as ET


OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]
SOURCE = ROOT / "morsehgp3D_v7"
BUILD = ROOT / "build/v7"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name: str, value: object) -> None:
    with (OUT / name).open("x") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())


def source_inventory() -> list[dict]:
    paths = [SOURCE / "CMakeLists.txt"]
    for folder in ("src", "cli", "oracle", "tests", "cmake", "bench", "receipts/conformite_v5"):
        paths.extend(p for p in (SOURCE / folder).rglob("*") if p.is_file() and
                     "__pycache__" not in p.parts and p.suffix != ".pyc")
    return [{"path": str(p.relative_to(ROOT)), "sha256": digest(p), "size": p.stat().st_size}
            for p in sorted(set(paths))]


def binaries() -> list[dict]:
    return [{"path": str(p.relative_to(ROOT)), "sha256": digest(p), "size": p.stat().st_size}
            for p in sorted(BUILD.glob("mhgp7*")) if p.is_file()]


def drain(process: subprocess.Popen) -> None:
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        pass
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    process.wait(timeout=10)


def run(label: str, command: list[str], limit: int) -> dict:
    started = time.monotonic()
    record = {"argv": command, "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
              "timeout_seconds": limit, "status": "running"}
    save(label + ".attempt.json", record)
    print(f"START {label}", flush=True)
    process = None
    with (OUT / (label + ".stdout")).open("xb") as stdout, (OUT / (label + ".stderr")).open("xb") as stderr:
        try:
            process = subprocess.Popen(command, cwd=ROOT, stdout=stdout, stderr=stderr,
                                       stdin=subprocess.DEVNULL, start_new_session=True,
                                       env=dict(os.environ, LC_ALL="C", LANG="C", PYTHONDONTWRITEBYTECODE="1"))
            code = process.wait(timeout=limit)
            record.update(exit_code=code, status="completed" if code == 0 else "failed")
        except subprocess.TimeoutExpired:
            record.update(exit_code=124, status="censored")
        except BaseException as error:
            record.update(exit_code=130, status="interrupted", error=f"{type(error).__name__}: {error}")
            raise
        finally:
            if process is not None:
                drain(process)
            record["elapsed_seconds"] = time.monotonic() - started
            save(label + ".result.json", record)
    print(f"END {label} rc={record['exit_code']} elapsed={record['elapsed_seconds']:.3f}s", flush=True)
    return record


def main() -> int:
    if (OUT / "sources_before.json").exists():
        raise RuntimeError("receipt already attempted; do not overwrite")
    baseline = source_inventory()
    product_hash = digest(BUILD / "mhgp7")
    save("sources_before.json", baseline)
    save("binaries_before.json", binaries())
    save("environment.json", {"schema": "mhgp7.local.release-qualification.v1", "public_status": "not_claimed",
         "scope": "cpu_release_gates_shared_host_not_slo", "product_sha256": product_hash,
         "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
         "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
         "affinity_cpus": sorted(os.sched_getaffinity(0)), "runner_sha256": digest(Path(__file__)),
         "uname": list(os.uname()), "gcp": "not_used"})
    results = {}
    failure = None
    try:
        for label, command, limit in [
            ("configure", ["cmake", "-S", str(SOURCE), "-B", str(BUILD), "-DCMAKE_BUILD_TYPE=Release"], 120),
            ("build_missing", ["cmake", "--build", str(BUILD), "--parallel", "2", "--target", "mhgp7_perm_residence_gate"], 600),
            ("inventory", ["ctest", "--test-dir", str(BUILD), "--show-only=json-v1", "-L", "^gate$"], 60),
        ]:
            results[label] = run(label, command, limit)
            if results[label]["exit_code"] != 0:
                raise RuntimeError(label + " failed")
            if digest(BUILD / "mhgp7") != product_hash:
                raise RuntimeError("product binary changed before qualification")
        save("binaries_tested.json", binaries())
        save("configuration.json", {str(path.relative_to(ROOT)): digest(path) for path in
             (BUILD / "CMakeCache.txt", BUILD / "CTestTestfile.cmake", BUILD / "Makefile")})
        results["ctest"] = run("ctest", ["ctest", "--test-dir", str(BUILD), "--output-on-failure", "--no-tests=error",
                               "-L", "^gate$", "--parallel", "2", "--output-junit", str(OUT / "ctest.junit.xml")], 7200)
        for optimized in (False, True):
            label = "incidence_campaign_optimized" if optimized else "incidence_campaign"
            results[label] = run(label, [sys.executable, "-B", *(["-O"] if optimized else []),
                                        str(SOURCE / "tests/incidence_campaign_gate.py")], 180)
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
        print(f"FAILURE {failure}", flush=True)
    finally:
        after = source_inventory()
        save("sources_after.json", after)
        final_binaries = binaries()
        save("binaries_after.json", final_binaries)
        final_hash = digest(BUILD / "mhgp7")
        junit = None
        if (OUT / "ctest.junit.xml").exists():
            cases = ET.parse(OUT / "ctest.junit.xml").getroot().findall(".//testcase")
            junit = {"tests": len(cases), "failed": [case.get("name") for case in cases
                     if case.find("failure") is not None or case.find("error") is not None],
                     "skipped": [case.get("name") for case in cases if case.find("skipped") is not None]}
        inventory = json.loads((OUT / "inventory.stdout").read_text()) if (OUT / "inventory.stdout").exists() else {}
        summary = {"status": "passed" if not failure and all(record["exit_code"] == 0 for record in results.values())
                   and set(results) == {"configure", "build_missing", "inventory", "ctest", "incidence_campaign", "incidence_campaign_optimized"}
                   and baseline == after and product_hash == final_hash else "invalid_or_failed",
                   "failure": failure, "source_stable": baseline == after, "product_unchanged": product_hash == final_hash,
                   "product_sha256_before": product_hash, "product_sha256_after": final_hash,
                   "ctest_inventory_count": len(inventory.get("tests", [])), "junit": junit,
                   "standalone_incidence_tests": 2, "public_status": "not_claimed", "gcp": "not_used",
                   "results": results}
        save("summary.json", summary)
        save("receipt_manifest.json", [{"path": path.name, "sha256": digest(path), "size": path.stat().st_size}
             for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
        print(json.dumps({k: v for k, v in summary.items() if k != "results"}, sort_keys=True), flush=True)
    return 0 if summary["status"] == "passed" else 1


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
