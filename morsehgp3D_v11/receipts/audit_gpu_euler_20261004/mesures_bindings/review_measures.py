"""Read pinned published JSON and its stdlib checker; no product/cloud execution."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
PIN = "61da03749344a6acc4fea2b9eee875606cc857e8"
FRAMES = ("lidar_ng00", "lidar_ng01", "lidar_ng02")
COUNTS = (39885, 35551, 45845)
CHECKS = 0


def need(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, required=True,
                        help="Any Git repository containing the pinned publication commit")
    args = parser.parse_args()
    meta = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
    need(meta["publication_commit"] == PIN, "publication pin")
    data = {}
    for record in meta["files"]:
        raw = subprocess.check_output(["git", "show", PIN + ":" + record["git_path"]], cwd=args.repo)
        need(hashlib.sha256(raw).hexdigest() == record["sha256"], "Git blob hash " + record["path"])
        need(len(raw) == record["bytes"], "Git blob size " + record["path"])
        data[record["path"]] = raw
    # Exact published check.py is executed only on pinned JSON in a temporary directory.
    # It imports json/pathlib/sys and performs no native/GCP operation.
    with tempfile.TemporaryDirectory(prefix="ehgp-measures-reader-") as temporary:
        work = Path(temporary)
        for rel, raw in data.items():
            path = work / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
        argv = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else []) + ["-S", str(work / "check.py")]
        result = subprocess.run(argv, capture_output=True, text=True, timeout=30)
    need(result.returncode == 0 and result.stdout.strip() == "recu_mesures_verdict conforme controles176"
         and not result.stderr, "published reader result")

    def load(path):
        return json.loads(data[path])

    sessions = {}
    previous_inputs = previous_identity = None
    for session, commit, status, variants in (
            ("claudeab8", "54c167bb6466e790f5259b44870725f3e02ed43f", "completed", ("base", "q3", "new")),
            ("claudediag1", "e49ea46907e3c4bb9e0942e2205cf490748136e4", "failed_remote", ("qr", "qr2", "r1", "new"))):
        receipt = load("sessions/" + session + "/receipt.json")
        report = load("sessions/" + session + "/ab_report.json")
        need(receipt["commit"] == commit and receipt["worker_source"] == "commit:" + commit, "consumed source")
        need(receipt["status"] == status, "session status")
        need(receipt["targeted_shutdown_certified"] is True and receipt["closure"] == "stopped", "closure")
        need(receipt["observed_after"]["status"] == "TERMINATED", "target stopped")
        need(receipt["generation"] == receipt["closing_generation"] ==
             receipt["observed_after"]["lastStartTimestamp"], "generation")
        need(receipt["target"]["instance"] == receipt["observed_after"]["name"] ==
             "ehgp-v7-3b1d496aed430749ea7e049f", "target identity")
        need(receipt["target"]["zone"] == "us-central1-c", "zone")
        need(receipt["results_verified"] and receipt["data_verified_remote"], "recorded integrity")
        inputs = {x["name"]: {"sha256": x["sha256"], "size": x["size"]}
                  for x in receipt["data_files"] if x["name"].endswith(".u32le")}
        for frame, count in zip(FRAMES, COUNTS):
            need(inputs[frame + ".u32le"]["size"] == count * 12, "whole XYZ byte count")
            need(inputs[frame + ".ids.u32le"]["size"] == count * 4, "IDs byte count")
        if previous_inputs is not None:
            need(inputs == previous_inputs and report["identity"] == previous_identity, "same data and dumps across sessions")
        previous_inputs, previous_identity = inputs, report["identity"]
        requested = {(variant, frame, workers, rep) for variant in variants for frame in FRAMES
                     for workers, repetitions in (("48", 5), ("1", 1)) for rep in range(repetitions)}
        actual = {(t["variant"], t["frame"], t["workers"], t["rep"]) for t in report["timings"]}
        need(actual == requested and len(actual) == len(report["timings"]), "timing inventory and uniqueness")
        for take in report["timings"]:
            need(take["code"] == 0 and take["quiet"] and take["summary"]["status"] == "ok"
                 and take["summary"]["exit"] == "ok"
                 and take["dump_sha256"] == report["identity"][take["frame"]], "material canonical take")
            need(take["cmd"][-1] == "16379", "CPU route, no device/batch bits")
        medians = {}
        for variant in variants:
            for frame in FRAMES:
                takes = [t["summary"] for t in report["timings"]
                         if t["variant"] == variant and t["frame"] == frame and t["workers"] == "48"]
                medians[variant + "/" + frame] = {
                    "wall_ms": statistics.median(t["wall_ns"] for t in takes) / 1e6,
                    "single_pass_ms": statistics.median(t["domain_detail"]["single_pass_ns"] for t in takes) / 1e6,
                    "forest_ms": statistics.median(t["forest_ns"] for t in takes) / 1e6,
                    "cpu_seconds": statistics.median(t["cpu_seconds"] for t in takes),
                    "peak_reserved_bytes": max(t["peak_reserved_bytes"] for t in takes), "takes": len(takes)}
        sessions[session] = {
            "consumed_commit": commit, "status": status, "generation": receipt["generation"],
            "last_stop": receipt["observed_after"]["lastStopTimestamp"], "targeted_shutdown_certified": True,
            "package_sha256": receipt["package_sha256"], "results_sha256": receipt["results_sha256"],
            "plan_sha256": receipt["plan_sha256"], "input_files": inputs,
            "report_verdict": report["verdict"], "refusals": report["refusals"],
            "test_summary": report["tests"], "tsan": report["tsan"], "mutants": report["mutants"],
            "recorded_binaries": report["builds"], "recorded_source_archives": report["archives_sha256"],
            "canonical_takes": len(report["timings"]), "native_W48_medians": medians}
    extra = {}
    total_takes = 0
    for name, repetitions in (("k10_leaf16", 3), ("k10_leaf24", 3), ("w24_pinned", 5), ("w48_free", 5)):
        report = load("sessions/claudediag1/" + name + ".json")
        need(report["mode"] == 16379 and report["reps"] == repetitions and report["failures"] == 0, "CPU timing settings")
        need({r["name"] for r in report["rows"]} == set(FRAMES) and len(report["rows"]) == 3, "extra inventory")
        rows = {}
        for row in report["rows"]:
            takes = row["takes"]
            need(len(takes) == repetitions and all(t["ok"] and t["code"] == 0 for t in takes), "process-success takes")
            need(all("dump_sha256" not in t for t in takes), "explicit absence of output equality evidence")
            need(statistics.median(t["wall_ms"] for t in takes) == row["wall_median_ms"], "median")
            total_takes += len(takes)
            rows[row["name"]] = {"sites": row["sites"], "wall_median_ms": row["wall_median_ms"],
                                 "min_ms": min(t["wall_ms"] for t in takes), "max_ms": max(t["wall_ms"] for t in takes),
                                 "cpu_median_seconds": statistics.median(t["cpu_seconds"] for t in takes),
                                 "takes": len(takes), "canonical_dump_equality": "not_recorded"}
        extra[name] = {"kmax": report["kmax"], "leaf": report["leaf"], "workers": report["workers"],
                       "taskset": report["taskset"], "mode": report["mode"], "rows": rows}
    need(total_takes == 48, "extra takes count")
    summary = {"schema": "ehgp.audit.measure_bindings.review.v1", "publication_commit": PIN,
               "scope": "read-only metadata and published stdlib reader; no native/GPU/GCP", "metadata_checks": CHECKS,
               "published_reader": result.stdout.strip(), "canonical_AB_takes": 126,
               "extra_process_success_takes_without_dump_hash": total_takes, "sessions": sessions, "extra": extra,
               "K10_leaf24_over_leaf16": {f: extra["k10_leaf24"]["rows"][f]["wall_median_ms"] /
                                            extra["k10_leaf16"]["rows"][f]["wall_median_ms"] for f in FRAMES},
               "W24_pinned_over_W48_free": {f: extra["w24_pinned"]["rows"][f]["wall_median_ms"] /
                                             extra["w48_free"]["rows"][f]["wall_median_ms"] for f in FRAMES},
               "sign_test_min_two_sided_p_for_five_non_ties": 0.0625,
               "limits": ["W1 one pair per frame; descriptive", "three frames, not three sequences",
                          "W24/W48 comparison changes both worker count and affinity",
                          "no causal proof of hyperthreading or memory stalls",
                          "diag1 overall failed; no global qualification transferred",
                          "no CPU-to-GPU transfer or 100ms contract acquired",
                          "phase medians must not be summed to infer median FULL"]}
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
