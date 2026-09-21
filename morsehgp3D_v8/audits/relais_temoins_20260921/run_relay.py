#!/usr/bin/env python3
"""Campagne close de l'auditeur B : relais rectangle → paires pour la recherche de témoins (front épinglé à 2629a536).

Pour chaque rectangle résiduel non rejeté par la recherche saturante de la tranche 32, une recherche exhaustive donne un
crédit commun U et une liste de candidats C ; chaque paire ne teste que C. Le reçu conserve les sorties brutes du
harnais, les pins (commit, bibliothèque, harnais, entrées) et recalcule les parts publiées depuis les comptes entiers.
Aucun assert : rejouable sous python3 -O.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PREPARED = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared"
COMMIT = "2629a536"
SCHEMA = "audit_b_relay_campaign_v1"
GRID = (("single_000000", 8000, 5), ("single_000000", 8000, 10), ("single_000000", 16000, 5), ("single_000000", 16000, 10),
        ("single_000000", 32000, 5), ("single_000000", 32000, 10), ("single_000100", 8000, 5), ("single_000200", 8000, 5))
SEPARATION, SAMPLES, SEED = 8, 2000, 1


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def derived(output):
    result = {}
    for lane in ("q3", "q4"):
        q = output[lane]
        require(q["rect_rejected_mass"] + q["exhaustive_mass"] == q["mass"] == output["front"]["residual_pair_mass"][1 if lane == "q3" else 2],
                "masses de rectangles non fermées")
        require(q["rect_rejected"] + q["exhaustive_rects"] == q["rectangles"] and q["pairs"] == q["exhaustive_mass"] and
                q["pairs_rejected_relay"] + q["pairs_kept_relay"] == q["pairs"], "comptes de paires non fermés")
        require(q["sample"]["disagreements"] == 0 and q["sample"]["pairs"] == output["samples_per_lane"], "identité du relais violée ou échantillon incomplet")
        require(sum(q["candidates_hist"]) == q["exhaustive_rects"], "histogramme des candidats non fermé")
        require(q["exhaustive_rects"] > 0 and q["pairs_kept_relay"] > 0 and q["rect_rejected"] > 0, "ligne vide")
        exhaustive_sample = q["sample"]["pairs"] - q["sample"]["in_rejected_rect"]
        result[lane] = dict(
            threshold=q["threshold"],
            rect_rejected_mass_share=q["rect_rejected_mass"] / q["mass"] if q["mass"] else 0.0,
            rect_rejected_share=q["rect_rejected"] / q["rectangles"] if q["rectangles"] else 0.0,
            pairs_rejected_share_of_residual=(q["rect_rejected_mass"] + q["pairs_rejected_relay"]) / q["mass"] if q["mass"] else 0.0,
            candidates_per_exhaustive_rect=q["candidates_sum"] / q["exhaustive_rects"],
            candidates_max=q["candidates_max"],
            universal_per_exhaustive_rect=q["universal_sum"] / q["exhaustive_rects"],
            exhaustive_visits_per_rect=q["exhaustive_visits"] / q["exhaustive_rects"],
            saturating_visits_per_rect=q["saturating_visits"] / q["rectangles"] if q["rectangles"] else 0.0,
            pair_tests_per_pair=q["pair_tests"] / q["pairs"] if q["pairs"] else 0.0,
            relay_tests_per_sampled_pair=q["sample"]["relay_tests"] / exhaustive_sample if exhaustive_sample else None,
            root_visits_per_sampled_pair=q["sample"]["root_visits"] / q["sample"]["pairs"] if q["sample"]["pairs"] else 0.0,
            relay_saturating_tests_per_sampled_pair=q["sample"]["relay_tests_saturating"] / exhaustive_sample if exhaustive_sample else None,
            root_visits_per_sampled_exhaustive_pair=q["sample"]["root_visits_exhaustive"] / exhaustive_sample if exhaustive_sample else None,
            relay_to_root_ratio=(q["sample"]["relay_tests_saturating"] / q["sample"]["root_visits_exhaustive"]) if q["sample"]["root_visits_exhaustive"] else None,
            total_relay_work=q["saturating_visits"] + q["exhaustive_visits"] + q["pair_tests"],
            total_root_work_estimate=q["saturating_visits"] + q["exhaustive_mass"] * (q["sample"]["root_visits_exhaustive"] / exhaustive_sample) if exhaustive_sample else None)
    return result


def inputs():
    rows = []
    for scan, size, kmax in GRID:
        metadata = json.loads((PREPARED / scan / "METADATA.json").read_text())
        declared = {sample["n"]: sample for sample in metadata["samples"]}
        path = PREPARED / declared[size]["path"]
        digest = sha256(path)
        require(digest == declared[size]["sha256"] and path.stat().st_size == 6 * size, f"scan préparé altéré : {path}")
        rows.append(dict(scan=scan, n=size, kmax=kmax, path=str(path.relative_to(ROOT)), sha256=digest))
    return rows


def run(args):
    worktree = args.worktree.resolve()
    head = subprocess.check_output(["git", "-C", str(worktree), "rev-parse", "--short=8", "HEAD"], text=True).strip()
    require(head == COMMIT, f"arbre de travail épinglé attendu à {COMMIT}, trouvé {head}")
    require(not subprocess.check_output(["git", "-C", str(worktree), "status", "--porcelain", "--", "morsehgp3D_v8/src"], text=True).strip(), "sources épinglées modifiées")
    library = worktree / "build/v8-audit/libmhgp8_p0.a"
    source = HERE / "relay_probe.cpp"
    binary = args.binary.resolve()
    build = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(worktree / "morsehgp3D_v8/src"),
             str(source), str(library), "-pthread", "-o", str(binary)]
    subprocess.run(build, check=True)
    pins = dict(commit=COMMIT, worktree=str(worktree), library_sha256=sha256(library), front_source_sha256=sha256(worktree / "morsehgp3D_v8/src/wspd/front.cpp"),
                harness_sha256=sha256(source), binary_sha256=sha256(binary), runner_sha256=sha256(Path(__file__)), build_command=build)
    runs = []
    for row in inputs():
        command = [str(binary), str(ROOT / row["path"]), str(row["kmax"]), str(SEPARATION), str(SAMPLES), str(SEED)]
        started = time.time()
        completed = subprocess.run(command, capture_output=True, text=True)
        require(completed.returncode == 0 and not completed.stderr, f"harnais en échec : {command} {completed.stderr[:300]}")
        output = json.loads(completed.stdout)
        require(output["schema"] == "audit_b_relay_probe_v2" and output["n"] == row["n"] and output["kmax"] == row["kmax"] and output["s"] == SEPARATION and
                output["samples_per_lane"] == SAMPLES and output["seed"] == SEED, "sortie du harnais incohérente")
        runs.append(dict(**row, command=command, wall_seconds=time.time() - started, output=output, derived=derived(output)))
        d = runs[-1]["derived"]
        print(json.dumps(dict(scan=row["scan"][-3:], n=row["n"], kmax=row["kmax"], seconds=round(runs[-1]["wall_seconds"], 1),
                              q3_candidates=round(d["q3"]["candidates_per_exhaustive_rect"], 1), q3_ratio=round(d["q3"]["relay_to_root_ratio"] or 0, 3),
                              q4_candidates=round(d["q4"]["candidates_per_exhaustive_rect"], 1), q4_ratio=round(d["q4"]["relay_to_root_ratio"] or 0, 3))), flush=True)
    receipt = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="witness-search relay rectangle to pairs, front only; no census, no q3/q4 engine, no clustering relevance",
                   pins=pins, parameters=dict(grid=[dict(scan=s, n=n, kmax=k) for s, n, k in GRID], separation=SEPARATION, samples_per_lane=SAMPLES, seed=SEED, mask=6),
                   runs=runs, run_count=len(runs), finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    require(len(runs) == len(GRID), "campagne incomplète")
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", runs=len(runs), output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["pins"]["commit"] == COMMIT and receipt["pins"]["harness_sha256"] == sha256(HERE / "relay_probe.cpp"), "harnais différent du reçu")
    for run_ in receipt["runs"]:
        require(sha256(ROOT / run_["path"]) == run_["sha256"], "scan préparé altéré depuis le reçu")
        require(run_["derived"] == derived(run_["output"]), "parts dérivées non reproduites")
    require(receipt["run_count"] == len(receipt["runs"]) == len(GRID), "campagne incomplète")
    print(json.dumps(dict(status="passed", runs=len(receipt["runs"]))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--worktree", type=Path, required=True)
    capture.add_argument("--binary", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE / "RELAY_CHECKS.json")
    reader = sub.add_parser("read")
    reader.add_argument("--output", type=Path, default=HERE / "RELAY_CHECKS.json")
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
