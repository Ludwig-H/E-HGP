#!/usr/bin/env python3
"""Campagne close de l'auditeur B : voies q3/q4 du front WSPD sur les scans LiDAR préparés (sources c5308651).

Aucun assert : le reçu est rejoué sous python3 -O. Tout est épinglé : commit des sources, bibliothèque et harnais
(sha256), scans d'entrée (sha256 comparés aux METADATA de la préparation lidar08), commande et sortie brute de
chaque exécution. Les parts et intervalles publiés sont recalculés depuis les comptes entiers du reçu.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PREPARED = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared"
COMMIT = "c5308651ba31aa7178e759b08fa2a593bb7fd6b1"
SCHEMA = "audit_b_front_lanes_lidar_campaign_v3"
SCANS = ("single_000000", "single_000100", "single_000200")
SIZES = (8000, 16000, 32000)
KMAX = (5, 10)
SEPARATION = 8
SAMPLES = 2000
SEED = 1


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def wilson(successes, trials, z=1.959964):
    """Intervalle de Wilson à 95 % pour une proportion (bornes en fraction)."""
    if trials == 0:
        return [0.0, 0.0]
    p = successes / trials
    denominator = 1 + z * z / trials
    centre = (p + z * z / (2 * trials)) / denominator
    half = z * math.sqrt(p * (1 - p) / trials + z * z / (4 * trials * trials)) / denominator
    return [max(0.0, centre - half), min(1.0, centre + half)]


def derived(output):
    """Parts et intervalles calculés depuis les comptes entiers d'une exécution."""
    result = {}
    for lane in ("q3", "q4"):
        ceiling, sample, front = output["ceiling"][lane], output["sample"][lane], output["front"]
        lane_index = 1 if lane == "q3" else 2
        mass = ceiling["mass"]
        require(mass == front["residual_pair_mass"][lane_index], "la masse plafonnée n'est pas la masse résiduelle du front")
        require(ceiling["rectangles"] == front["lane_rectangles"][lane_index], "rectangles plafonnés différents des rectangles du front")
        require(sum(ceiling["rect_hist"]) == ceiling["rectangles"] and sum(ceiling["mass_hist"]) == mass, "histogrammes non fermés")
        require(ceiling["windows"]["L1K"]["rectangles"] == 0 and ceiling["windows"]["L1K"]["mass"] == 0,
                "la fenêtre K rejette un rectangle que le front a émis")
        require(sample["pairs"] == output["samples_per_lane"] and sample["rejectable"] + sample["kept"] == sample["pairs"] and
                sample["rejectable_with_ceiling"] + sample["rejectable_without_ceiling"] == sample["rejectable"] and
                sample["witnesses_total"] == sample["witnesses_universal_box"] + sample["witnesses_nonuniversal_outside"] +
                sample["witnesses_in_a"] + sample["witnesses_in_b"], "comptes d'échantillon non fermés")
        descent, cover, seeds, lemma = sample["descent"], sample["cover"], sample["seeds"], sample["lemma"]
        require(descent["disagreements"] == 0, "la descente saturante contredit le balayage complet")
        require(lemma["violations"] == 0, "un seed q3 d'une paire rejetable a une profondeur sous le seuil : lemme du citron violé")
        require(output["index"]["leaves_with_several_ranks"] == 0, "feuille de l'index à plusieurs rangs : convention du harnais invalide")
        require(descent["visits_rejectable"] + descent["visits_kept"] == descent["node_visits"], "visites de descente non fermées")
        near = sample["descent_near_first"]
        require(near["disagreements"] == 0 and near["visits_rejectable"] + near["visits_kept"] == near["node_visits"],
                "la descente milieu d'abord contredit le balayage complet ou n'est pas fermée")
        if lane == "q3":
            require(lemma["pairs"] == min(100, sample["rejectable"]) and (lemma["pairs"] == 0 or lemma["seeds"] > 0),
                    "vérification du lemme vide ou incomplète")
            require(seeds["emitted_kept"] >= seeds["kept_with_emission"], "émissions incohérentes")
        pairs = sample["pairs"]
        total_pairs = front["total_unordered_pairs"]
        rejectable, kept = sample["rejectable"], sample["kept"]
        result[lane] = dict(
            threshold=ceiling["threshold"],
            residual_mass=mass, residual_mass_share_of_all_pairs=mass / total_pairs if total_pairs else 0.0,
            rectangles=ceiling["rectangles"],
            ceiling_rectangle_share=ceiling["rectangles_at_threshold"] / ceiling["rectangles"] if ceiling["rectangles"] else 0.0,
            ceiling_mass_share=ceiling["mass_at_threshold"] / mass if mass else 0.0,
            window_2k_mass_share=ceiling["windows"]["L2K"]["mass"] / mass if mass else 0.0,
            window_4k_mass_share=ceiling["windows"]["L4K"]["mass"] / mass if mass else 0.0,
            sample_pairs=pairs,
            rejectable_share=sample["rejectable"] / pairs if pairs else 0.0,
            rejectable_share_wilson95=wilson(sample["rejectable"], pairs),
            rejectable_reachable_by_box_proposer_share=(sample["rejectable_with_ceiling"] / sample["rejectable"]) if sample["rejectable"] else None,
            rejectable_reachable_by_box_proposer_wilson95=wilson(sample["rejectable_with_ceiling"], sample["rejectable"]) if sample["rejectable"] else None,
            kept_share=sample["kept"] / pairs if pairs else 0.0,
            witness_share_universal_box=sample["witnesses_universal_box"] / sample["witnesses_total"] if sample["witnesses_total"] else None,
            witness_share_nonuniversal_outside=sample["witnesses_nonuniversal_outside"] / sample["witnesses_total"] if sample["witnesses_total"] else None,
            witness_share_in_factors=(sample["witnesses_in_a"] + sample["witnesses_in_b"]) / sample["witnesses_total"] if sample["witnesses_total"] else None,
            mean_witnesses_per_pair=sample["witnesses_total"] / pairs if pairs else 0.0,
            descent_visits_per_pair=descent["node_visits"] / pairs if pairs else 0.0,
            descent_visits_per_rejectable_pair=descent["visits_rejectable"] / rejectable if rejectable else None,
            descent_visits_per_kept_pair=descent["visits_kept"] / kept if kept else None,
            descent_visits_max=descent["visits_max"],
            descent_leaf_tests_per_pair=descent["leaf_tests"] / pairs if pairs else 0.0,
            near_first_visits_per_rejectable_pair=near["visits_rejectable"] / rejectable if rejectable else None,
            near_first_visits_per_kept_pair=near["visits_kept"] / kept if kept else None,
            near_first_visits_max=near["visits_max"],
            seed_mass_reduction_factor=((seeds["rejectable"] + seeds["kept"]) / seeds["kept"] if seeds["kept"] else None) if lane == "q3" else None,
            cover_sites_per_rejectable_pair=cover["sites_rejectable"] / rejectable if rejectable else None,
            cover_sites_per_kept_pair=cover["sites_kept"] / kept if kept else None,
            cover_sites_kept_max=cover["sites_kept_max"],
            tight_cover_per_kept_pair=cover["tight_kept"] / kept if kept else None,
            seeds_per_rejectable_pair=(seeds["rejectable"] / rejectable if rejectable else None) if lane == "q3" else None,
            seeds_per_kept_pair=(seeds["kept"] / kept if kept else None) if lane == "q3" else None,
            seeds_kept_max=seeds["kept_max"] if lane == "q3" else None,
            emitted_per_kept_pair=(seeds["emitted_kept"] / kept if kept else None) if lane == "q3" else None,
            kept_pairs_with_emission_share=(seeds["kept_with_emission"] / kept if kept else None) if lane == "q3" else None,
            census_tests_per_kept_pair=(seeds["census_tests_kept"] / kept if kept else None) if lane == "q3" else None,
            lemma_pairs_checked=lemma["pairs"], lemma_seeds_checked=lemma["seeds"], lemma_violations=lemma["violations"])
    return result


def inputs():
    rows = []
    for scan in SCANS:
        metadata = json.loads((PREPARED / scan / "METADATA.json").read_text())
        declared = {sample["n"]: sample for sample in metadata["samples"]}
        for size in SIZES:
            path = PREPARED / declared[size]["path"]
            digest = sha256(path)
            require(digest == declared[size]["sha256"] and path.stat().st_size == declared[size]["bytes"] == 6 * size,
                    f"scan préparé altéré : {path}")
            rows.append(dict(scan=scan, n=size, path=str(path.relative_to(ROOT)), sha256=digest, frame=metadata["frame"],
                             quantization=metadata["quantization"], sampling=metadata["sampling"]))
    return rows


def run(args):
    worktree = args.worktree.resolve()
    head = subprocess.check_output(["git", "-C", str(worktree), "rev-parse", "HEAD"], text=True).strip()
    require(head == COMMIT, f"l'arbre de travail épinglé n'est pas à {COMMIT}")
    require(not subprocess.check_output(["git", "-C", str(worktree), "status", "--porcelain", "--", "morsehgp3D_v8/src"], text=True).strip(),
            "sources épinglées modifiées")
    library = worktree / "build/v8-audit/libmhgp8_p0.a"
    source = HERE / "front_lanes_probe.cpp"
    binary = args.binary.resolve()
    build = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(worktree / "morsehgp3D_v8/src"),
             str(source), str(library), "-pthread", "-o", str(binary)]
    subprocess.run(build, check=True)
    pins = dict(commit=COMMIT, worktree=str(worktree), library_sha256=sha256(library), harness_sha256=sha256(source),
                binary_sha256=sha256(binary), runner_sha256=sha256(Path(__file__)), build_command=build,
                compiler=subprocess.check_output(["g++", "--version"], text=True).splitlines()[0])
    scans = inputs()
    runs = []
    for row in scans:
        for kmax in KMAX:
            command = [str(binary), str(ROOT / row["path"]), str(kmax), str(SEPARATION), str(SAMPLES), str(SEED)]
            started = time.time()
            completed = subprocess.run(command, capture_output=True, text=True)
            require(completed.returncode == 0 and not completed.stderr, f"harnais en échec : {command} {completed.stderr[:300]}")
            output = json.loads(completed.stdout)
            require(output["schema"] == "audit_b_front_lanes_lidar_v3" and output["n"] == row["n"] and output["kmax"] == kmax and
                    output["s"] == SEPARATION and output["mask"] == 6 and output["front_mode"] == "samples" and
                    output["seed"] == SEED and output["samples_per_lane"] == SAMPLES, "sortie du harnais incohérente avec sa commande")
            runs.append(dict(scan=row["scan"], n=row["n"], kmax=kmax, s=SEPARATION, command=command, wall_seconds=time.time() - started,
                             output=output, derived=derived(output)))
            print(json.dumps(dict(scan=row["scan"], n=row["n"], kmax=kmax, seconds=round(runs[-1]["wall_seconds"], 1),
                                  q3=runs[-1]["derived"]["q3"]["ceiling_mass_share"], q4=runs[-1]["derived"]["q4"]["ceiling_mass_share"])), flush=True)
    receipt = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="front q3/q4 lanes only; no census, no q3/q4 engine, no clustering relevance", pins=pins, inputs=scans,
                   parameters=dict(sizes=SIZES, kmax=KMAX, separation=SEPARATION, samples_per_lane=SAMPLES, seed=SEED, mask=6, front_mode="samples"),
                   runs=runs, run_count=len(runs), finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    require(len(runs) == len(SCANS) * len(SIZES) * len(KMAX), "campagne incomplète")
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", runs=len(runs), output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["pins"]["commit"] == COMMIT and receipt["pins"]["harness_sha256"] == sha256(HERE / "front_lanes_probe.cpp"),
            "le harnais du reçu n'est pas celui du dossier")
    for row in receipt["inputs"]:
        require(sha256(ROOT / row["path"]) == row["sha256"], "scan préparé altéré depuis le reçu")
    require(receipt["run_count"] == len(receipt["runs"]) == len(SCANS) * len(SIZES) * len(KMAX), "campagne incomplète")
    for run_ in receipt["runs"]:
        require(run_["derived"] == derived(run_["output"]), "parts dérivées non reproduites depuis les comptes")
    print(json.dumps(dict(status="passed", runs=len(receipt["runs"]))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--worktree", type=Path, required=True, help="arbre de travail détaché à c5308651, bibliothèque build/v8-audit")
    capture.add_argument("--binary", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE / "FRONT_LANES_CHECKS.json")
    reader = sub.add_parser("read")
    reader.add_argument("--output", type=Path, default=HERE / "FRONT_LANES_CHECKS.json")
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
