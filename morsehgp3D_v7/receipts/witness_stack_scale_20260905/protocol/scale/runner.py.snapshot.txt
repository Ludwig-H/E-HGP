#!/usr/bin/env python3
"""Prepared F-only scale observations; no engine or writes without --execute."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import types

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
SOURCE = ROOT / "morsehgp3D_v7"
CANDIDATE = ROOT / "build/v7_f_qualification/mhgp7"
CANDIDATE_SHA = "ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85"
RECEIPT = ROOT / "build/v7_f_build_20260905/build_D.json"
RECEIPT_SHA = "522c950c70b60ca58759c4fa9b9a24ff995fe829b9aa1adf5b2f51b7b2177ac4"
PAIRED = ROOT / "build/v7_f_pair_20260905/runner.py"
PAIRED_SHA = "20f956612c598da256e24f8de893e7df5132f6dff0221dbc2edcdd7fe2ecce3d"
SIZES = (16000, 32000)


def load_pinned(path: Path, pin: str, name: str):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != pin:
        raise RuntimeError("authority pin changed: " + str(path))
    module = types.ModuleType(name)
    module.__file__ = str(path)
    exec(compile(data, str(path), "exec"), module.__dict__)
    return module


N = load_pinned(PAIRED, PAIRED_SHA, "scale_f_paired_authority")
R = N.R


def command(n: int) -> list[str]:
    R.require(n in SIZES, "unreviewed scale size")
    args = R.command(CANDIDATE)
    R.require(args.count("--n=8000") == 1, "unique historical n command anchor changed")
    # This is an argv substitution only. The original incidence parser already
    # accepts n as a parameter; neither its source nor output bytes are adapted.
    return [f"--n={n}" if arg == "--n=8000" else arg for arg in args]


def snapshot(baseline: dict, common) -> dict:
    R.require(R.digest(PAIRED) == PAIRED_SHA, "paired authority changed")
    value = N.snapshot(CANDIDATE, SOURCE, RECEIPT, baseline, common)
    value["scale_protocol_files"] = {
        path.name: R.digest(path) for path in
        (Path(__file__), BASE / "selftest.py", BASE / "README.md")}
    return value


def admission(baseline: dict, common) -> dict:
    R.require(R.digest(CANDIDATE) == CANDIDATE_SHA, "fixed F executable changed")
    R.require(R.digest(RECEIPT) == RECEIPT_SHA, "fixed F build receipt changed")
    R.require(CANDIDATE not in [Path(name).resolve() for name in baseline["protected_binaries"]],
              "F candidate is a protected historical CLI")
    binding = R.candidate_binding(CANDIDATE, SOURCE, RECEIPT)
    before = snapshot(baseline, common)
    N.binding_matches_snapshot(before, baseline, binding, CANDIDATE, RECEIPT)
    R.require(before["files"][str(CANDIDATE)] == CANDIDATE_SHA and
              before["files"][str(RECEIPT)] == RECEIPT_SHA, "fixed F pins changed before snapshot")
    return {"binding": binding, "before": before}


def classify(record: dict, out: Path, err: Path, usage: Path, n: int, common, incidence) -> None:
    R.require(n in SIZES and record.get("role") == "F" and record.get("n") == n,
              "scale observation identity")
    identity = dict(R.IDENTITY, n=n)
    incidence.classify(record, out, err, usage, **identity)
    if record["status"] == "engine_completed":
        try:
            record.update(R.completed_extra(out.read_text(), record, common))
        except Exception:
            for key in ("digests", "counts", "cardinalities", "silent", "pipeline_ms", "max_rss_kb"):
                record.pop(key, None)
            record["status"] = "invalid"
            raise


def decision(records: list[dict], n: int, stable: bool, failure: str | None, incidence) -> dict:
    identity = n in SIZES and len(records) == 1 and records[0].get("role") == "F" and records[0].get("n") == n
    value = incidence.summarize(records, 1, failure or (None if identity else "scale identity mismatch"), stable)
    value.update(n=n, role="F", comparison="none_F_only", slo="not_evaluated", gcp="not_used",
                 cost_scope="one_cold_process_including_digest_no_archive_shared_host_not_statistical_gain",
                 receipt_scope="independent_scale_observation_not_part_of_8000_pairs")
    return value


def execute(n: int, common, incidence) -> int:
    R.environment_guard()
    argv = command(n)
    output = BASE / f"n{n}_receipts"
    if output.exists():
        raise FileExistsError(output)
    baseline = N.baseline_binding()
    admitted = admission(baseline, common)
    before, binding = admitted["before"], admitted["binding"]
    R.require(R.CPU in os.sched_getaffinity(0), "CPU6 is outside allowed affinity")
    repository_before, host_before, started = R.repository_state(), R.host_observation(), R.utc_now()
    R.require(snapshot(baseline, common) == before, "preflight metadata boundary drift")
    output.mkdir(parents=True, exist_ok=False)
    common.atomic_json(output / "metadata.json", {
        "schema": "mhgp7-F-scale-observation-v1", "candidate": binding, "protected_historical": baseline,
        "snapshot_before": before, "repository_before": repository_before, "host_before": host_before,
        "identity": dict(R.IDENTITY, n=n), "s": 8, "smax": 11, "silent_limits": R.CAPS,
        "timeout_seconds": R.TIMEOUT, "rlimit_as_gib": R.ADDRESS_LIMIT_GIB,
        "rlimit_as_is_not_physical_RSS_limit": True, "cpu_affinity_requested": [R.CPU],
        "payload_proxy_bytes": R.MEMORY_PROXY, "archive": False, "digest": True,
        "classification_source_adaptations": 0, "argv_adaptation": "one --n=8000 token",
        "started_utc": started, "public_status": "not_claimed", "gcp": "not_used",
        "comparison": "none_F_only", "strict_thread_creation_measurement": "not_performed_by_runner",
        "effective_locale": {"LC_ALL": "C", "LANG": "C"},
        "incomplete_usage_policy": "keep_raw_time_without_inventing_RSS_for_refusal_or_censure"})
    records, failure, after = [], None, None
    common.atomic_json(output / "runs.json", records)
    try:
        R.require(snapshot(baseline, common) == before, "source/helper/binary changed before F")
        out, err, usage = (output / ("F" + suffix) for suffix in (".out", ".err", ".time"))
        wrapped = ["/usr/bin/taskset", "--cpu-list", str(R.CPU), "/usr/bin/time", "-v", "-o", str(usage), "--", *argv]
        record = {"role": "F", "n": n, "status": "invalid", "command": argv, "wrapped_command": wrapped,
                  "started_utc": R.utc_now(), "ended_utc": None, "returncode": None,
                  "timed_out": False, "wall_seconds": None, "stdout": out.name, "stderr": err.name, "usage": usage.name}
        records.append(record)
        common.atomic_json(output / "runs.json", records)
        try:
            code, censored, elapsed = common.run_process(wrapped, out, err, timeout=R.TIMEOUT,
                                                        address_limit_gib=R.ADDRESS_LIMIT_GIB)
            record.update(returncode=code, timed_out=censored, wall_seconds=elapsed)
            classify(record, out, err, usage, n, common, incidence)
        except BaseException as error:
            record["validation_error"] = f"{type(error).__name__}: {error}"
            raise
        finally:
            record["ended_utc"] = R.utc_now()
            common.atomic_json(output / "runs.json", records)
        R.require(snapshot(baseline, common) == before, "source/helper/binary changed during F")
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    repository_after = None
    try:
        repository_after = R.repository_state()
    except BaseException as error:
        failure = failure or f"{type(error).__name__}: final repository state unavailable"
    try:
        after = snapshot(baseline, common)
    except BaseException as error:
        failure = failure or f"{type(error).__name__}: terminal snapshot unavailable"
    summary = decision(records, n, before == after, failure, incidence)
    summary.update(started_utc=started, ended_utc=R.utc_now(),
                   repository_change_policy="HEAD_or_worktree_changes_recorded_source_snapshot_stability_required")
    common.atomic_json(output / "repository_after.json", repository_after)
    common.atomic_json(output / "snapshot_after.json", after)
    common.atomic_json(output / "summary.json", summary)
    common.atomic_json(output / "hashes.json", {path.name: common.sha256(path) for path in sorted(output.iterdir()) if path.is_file()})
    print(json.dumps(summary), flush=True)
    return 1 if summary["status"] != "observations_completed" else 0 if summary["engine_successes"] == 1 else 2


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, choices=SIZES, required=True)
    parser.add_argument("--execute", action="store_true")
    args = parser.parse_args(argv)
    if not args.execute:
        print(json.dumps({"status": "prepared_not_executed", "writes": False, "engine_runs": 0,
            "command": command(args.n), "timeout_seconds": R.TIMEOUT, "rlimit_as_gib": R.ADDRESS_LIMIT_GIB,
            "cpu": R.CPU, "output_would_be": str(BASE / f"n{args.n}_receipts"),
            "candidate_sha256": CANDIDATE_SHA, "build_receipt_sha256": RECEIPT_SHA,
            "classification_source_adaptations": 0, "comparison": "none_F_only",
            "execution_requires_new_root_GO": True, "public_status": "not_claimed", "gcp": "not_used"}, indent=2))
        return 0
    common, incidence = R.helpers()
    return execute(args.n, common, incidence)


if __name__ == "__main__":
    def interrupted(signum, _frame):
        raise InterruptedError(f"signal {signum}")
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    raise SystemExit(main())
