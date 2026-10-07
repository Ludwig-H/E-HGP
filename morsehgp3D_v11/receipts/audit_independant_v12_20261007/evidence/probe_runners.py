#!/usr/bin/env python3
"""Contre-epreuves du harnais sur entrees fabriquees, sans moteur ni GCP.

Les nombres de performance sont deliberement synthetiques. Aucune preuve
historique n'est modifiee. Le script qualifie le comportement des lecteurs,
pas l'exactitude du moteur.
"""
import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fixture(mode, verdict, reps):
    report = {"verdict": verdict, "refusals": [] if verdict == "conforme" else ["synthetic_dump_mismatch"],
              "cold": [], "warm_medians_ms": {}, "cold_medians_ms": {}}
    for frame in ("lidar_ng00", "lidar_ng01", "lidar_ng02"):
        for variant in ("new", "base"):
            ms = 10 if variant == "new" else 100
            summary = {"single_pass_ms": ms, "prefix_ms": ms, "domain_ms": ms, "forest_ms": ms,
                       "pipeline": {"phases": {"births_ns": ms},
                                    "orders": [{"k": k, "publish_cpu_ms": ms,
                                                "publish_cells_est_ms": ms, "publish_closes_est_ms": ms}
                                               for k in (3, 4, 5)]}}
            for rep in range(reps):
                report["cold"].append({"frame": frame, "mode": variant + ":" + mode,
                                       "code": 0, "summary": summary, "rep": rep})
        for regime in ("cold", "warm"):
            report[regime + "_medians_ms"][regime + "|" + frame + "|w48|" + mode] = {"wall_ms": 100}
            report[regime + "_medians_ms"][regime + "|" + frame + "|w48|" + mode + "_cache"] = {"wall_ms": 10}
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[4])
    ap.add_argument("--out", type=Path, default=Path(__file__).with_name("runner_probes.json"))
    args = ap.parse_args()
    base = args.repo / "morsehgp3D_v11"
    results = {"gcp_used": False, "synthetic_only": True, "judge_cases": []}
    judges = sorted((base / "receipts/developpement_20261007").glob("*/*/judge.py"))
    results["source_sha256"] = {str(p.relative_to(args.repo)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in judges + [base / "bench/gpu_ab.py", base / "tools/g4_matrix.py"]}
    cases_dir = args.out.parent / "synthetic_fixtures"
    cases_dir.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="mhgp11-evidence-") as tmp:
        tmp = Path(tmp)
        for case, verdict, reps in (("valid_six_takes", "conforme", 6),
                                    ("invalid_benchmark_six_takes", "refus", 6),
                                    ("only_one_take_instead_of_six", "conforme", 1)):
            for name, mode in (("ab_k5_16_cpu", "cpu"), ("ab_k5_24_gpu", "gpu")):
                folder = tmp / ("000_" + name) / "files" / name
                folder.mkdir(parents=True, exist_ok=True)
                text = json.dumps(fixture(mode, verdict, reps), indent=1) + "\n"
                (folder / "gpu_ab_report.json").write_text(text)
                (cases_dir / (case + "_" + mode + ".json")).write_text(text)
            for judge in judges:
                for optimized in (False, True):
                    cmd = [sys.executable, "-I"] + (["-O"] if optimized else []) + [str(judge), str(tmp)]
                    done = subprocess.run(cmd, text=True, capture_output=True, timeout=10)
                    results["judge_cases"].append({"judge": str(judge.relative_to(args.repo)),
                                                  "case": case, "optimized": optimized,
                                                  "code": done.returncode,
                                                  "warned_invalid": "banc non conforme" in done.stdout,
                                                  "adoption_printed": any("verdict " + s in done.stdout
                                                                          for s in ("garde", "adopte")),
                                                  "last_line": done.stdout.splitlines()[-1] if done.stdout else "",
                                                  "stderr": done.stderr})
        # Existing b_cuda short-circuits source identity and even source existence.
        gpu = load(base / "bench/gpu_ab.py", "audit_gpu_ab")
        work = tmp / "build_probe"
        old = work / "b_cuda/mhgp11_full_bench"
        old.parent.mkdir(parents=True)
        old.write_bytes(b"synthetic_old_binary_not_executable\n")
        log = []
        found = gpu.build(tmp / "nonexistent_new_source", work, tmp, log)
        results["gpu_build_reuse"] = {"nonexistent_source": True, "reused_existing_file": found == old,
                                      "build_log": log}

        # Control: the full matrix does reject empty/missing/conflicting results.
        matrix = load(base / "tools/g4_matrix.py", "audit_matrix")
        results["matrix_exit_codes"] = {"empty": matrix.exit_code_of([]),
                                        "absent_only": matrix.exit_code_of([{"status": "absent"}]),
                                        "ok_and_absent": matrix.exit_code_of([{"status": "ok"}, {"status": "absent"}]),
                                        "ok_and_failure": matrix.exit_code_of([{"status": "ok"}, {"status": "failed"}])}
        selected = [{"name": "mhgp11_fixture", "labels": ["unit"], "disabled": False}]
        config = {"require_labels": ["unit"], "require_labels_if_data": [], "min_tests": 1}
        stdout = "1/1 Test #1: mhgp11_fixture ............ Passed 0.01 sec\n100% tests passed, 0 tests failed out of 1\n"
        states = {}
        for case, text, junit in (("valid", stdout, None),
                                  ("no_result", "", None),
                                  ("junit_contradiction", stdout, {"mhgp11_fixture": {"state": "failed", "seconds": 0.01,
                                                                                   "message": "synthetic", "output": ""}})):
            verdict = matrix.judge_tests(selected, {"status": "ok", "exit_code": 0}, text, junit,
                                         "", matrix.LIMITS, config, False)
            states[case] = verdict["status"]
        results["matrix_judge_states"] = states

    args.out.write_text(json.dumps(results, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"judges": len(judges), "cases": len(results["judge_cases"]),
                      "invalid_benchmark_adoptions": sum(r["adoption_printed"] for r in results["judge_cases"]
                                                         if r["case"] == "invalid_benchmark_six_takes"),
                      "single_take_adoptions": sum(r["adoption_printed"] for r in results["judge_cases"]
                                                   if r["case"] == "only_one_take_instead_of_six"),
                      "gpu_build_reuse": results["gpu_build_reuse"],
                      "matrix_exit_codes": results["matrix_exit_codes"],
                      "matrix_judge_states": results["matrix_judge_states"]}, indent=2))


if __name__ == "__main__":
    main()
