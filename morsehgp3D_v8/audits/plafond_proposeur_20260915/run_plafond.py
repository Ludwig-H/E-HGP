#!/usr/bin/env python3
"""Reçu du plafond de tout proposeur de témoins du front q2 (auditeur B, 15 sept. 2026).

Lance ``witness_ceiling`` (compilé contre les sources épinglées) sur les familles et
tailles d'intérêt, analyse ses lignes et grave PLAFOND_PROPOSEUR_CHECKS.json avec les
contrôles de cohérence : la fenêtre historique L = K ne rejette aucun rectangle émis,
bloc <= fenêtre 4K <= plafond n'est PAS exigé (le bloc et les fenêtres sont des
proposeurs distincts) mais chaque proposeur est borné par le plafond, et les
histogrammes totalisent les rectangles et la masse. Tient sous ``python3 -O``.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
CONFIGS_FULL = [
    (8000, "uniform", 10, 8), (8000, "uniform", 5, 8),
    (8000, "clusters", 10, 8), (8000, "clusters", 5, 8),
    (8000, "terrain", 10, 8), (8000, "terrain", 5, 8),
    (8000, "rows", 10, 8), (8000, "rows", 5, 8),
    (8000, "uniform", 10, 12),
    (16000, "uniform", 10, 8),
    (32000, "uniform", 10, 8), (32000, "uniform", 5, 8),
    (32000, "clusters", 10, 8), (32000, "clusters", 5, 8),
]
CONFIGS_QUICK = [(2000, "uniform", 10, 8), (2000, "terrain", 5, 8)]


def sha256_of(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def parse_hist(line):
    out = {}
    for token in line.split(":", 1)[1].split():
        key, value = token.rsplit(":", 1)
        out[key] = int(value)
    return out


def parse_output(text):
    lines = [line for line in text.splitlines() if line.strip()]
    entry = {}
    for token in lines[0].split():
        key, value = token.split("=", 1)
        if key in ("family", "front"):
            entry[key] = value
        elif "." in value:
            entry[key] = float(value)
        else:
            entry[key] = int(value)
    for line in lines[1:]:
        name = line.strip().split(":", 1)[0]
        entry[name] = parse_hist(line)
    return entry


def check_entry(entry):
    failures = []
    rects = entry["rects"]
    mass = entry["mass"]
    if entry["wink_rects"] != 0:
        failures.append("historical window L=K rejects emitted rectangles")
    for key in ("block_rects", "win2k_rects", "win4k_rects"):
        if entry[key] > entry["ceil_rects"]:
            failures.append(key + " exceeds the ceiling")
    if entry["win2k_rects"] > entry["win4k_rects"]:
        failures.append("window 2K rejects more than window 4K")
    if sum(entry["rect_hist"].values()) != rects:
        failures.append("rect_hist does not total the rectangles")
    if sum(entry["mass_hist"].values()) != mass:
        failures.append("mass_hist does not total the pair mass")
    if sum(entry["block_hist"].values()) != rects:
        failures.append("block_hist does not total the rectangles")
    if entry["ceil_rects"] != entry["rect_hist"][str(entry["kmax"])]:
        failures.append("ceil_rects differs from the last histogram bin")
    return failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True)
    parser.add_argument("--engine-commit", required=True)
    parser.add_argument("--out", default=str(HERE / "PLAFOND_PROPOSEUR_CHECKS.json"))
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--seed", type=int, default=3)
    args = parser.parse_args()
    configs = CONFIGS_QUICK if args.quick else CONFIGS_FULL
    entries = []
    status = "pass"
    for n, family, kmax, s in configs:
        command = [args.binary, str(n), family, str(kmax), str(s), str(args.seed), "samples"]
        started = time.time()
        completed = subprocess.run(command, capture_output=True, text=True, check=False)
        if completed.returncode != 0:
            entries.append({"n": n, "family": family, "kmax": kmax, "s": s,
                            "status": "error", "stderr": completed.stderr[-2000:]})
            status = "fail"
            print("ERROR", n, family, kmax, s, file=sys.stderr)
            continue
        entry = parse_output(completed.stdout)
        entry["wall_seconds"] = round(time.time() - started, 1)
        failures = check_entry(entry)
        entry["failures"] = failures
        entry["status"] = "pass" if not failures else "fail"
        if failures:
            status = "fail"
        entries.append(entry)
        print(f"{family} n={n} K={kmax} s={s}: rects={entry['rects']} ceil={entry['ceil_rect_share']:.3f}"
              f" block={entry['block_rect_share']:.3f} win2k={entry['win2k_rect_share']:.3f}"
              f" win4k={entry['win4k_rect_share']:.3f} mass_ceil={entry['ceil_mass_share']:.3f}"
              f" ({entry['wall_seconds']} s) {entry['status']}", flush=True)
    receipt = {
        "title": "Plafond de tout proposeur de témoins du front q2 (rectangles émis, prédicat de boîte du front)",
        "auditor": "B",
        "date": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "engine_commit": args.engine_commit,
        "harness": "witness_ceiling.cpp",
        "harness_sha256": sha256_of(HERE / "witness_ceiling.cpp"),
        "runner_sha256": sha256_of(__file__),
        "seed": args.seed,
        "quick": args.quick,
        "definitions": {
            "ceil": "rectangles émis (lane q2) ayant au moins K sites z avec H_min(A.box, B.box, {z}) > 0 ; "
                    "aucun proposeur de témoins ponctuels de boîte ne peut rejeter les autres, ni au produit ni à un ancêtre",
            "block": "plus haut nœud du chemin de descente du front (vers le milieu du produit) dont la borne conjointe "
                     "A.box x B.box x Z.box est strictement positive et dont la taille atteint K",
            "win2k/win4k": "fenêtre de L = 2K / 4K rangs autour du pivot du front (formule du front, rangs de A/B sautés) "
                           "cumulant K crédits stricts ; wink (L = K) doit valoir 0 sur les rectangles émis",
            "mass": "somme de |A| x |B| sur les rectangles de la classe (candidats du census)",
        },
        "status": status,
        "entries": entries,
    }
    pathlib.Path(args.out).write_text(json.dumps(receipt, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print("status", status, "->", args.out)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
