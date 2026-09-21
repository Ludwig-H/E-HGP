#!/usr/bin/env python3
"""Contrôle croisé de l'auditeur B : flux q3 du raccord global 31 (4dbe3024) contre une énumération exhaustive indépendante.

Pour chaque préfixe 1k/2k/4k du scan 0 (K5, et K10 jusqu'à 2k), la sonde du constructeur `mhgp8_wspd_q34_probe`
(masque 2, Local28, quatre workers, mode records) et le harnais de B en mode exhaustif (toutes les paires résiduelles,
règle de propriété du raccord, census entier Δ|z−a|² < (z−a)·N sur la couverture des paires conservées, lemme du
citron sur les paires rejetables) doivent produire les mêmes masses résiduelles, le même nombre de seeds q3, le même
nombre de boules émises et le même multiensemble (support trié, profondeur). Aucun assert : rejouable sous python3 -O.
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
SOURCE = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared/single_000000/n8000.u16le"
SOURCE_SHA256 = "ed5aa97941551e027a76d7c4eb4a03c739cdf0a83cbaf53035c1536bed0b5194"
COMMIT = "4dbe3024"
SCHEMA = "audit_b_q3_stream_crosscheck_v1"
GRID = ((1000, 5, 10 ** 9), (2000, 5, 2000), (4000, 5, 1000), (1000, 10, 10 ** 9), (2000, 10, 2000))  # (n, K, budget du lemme)
LANE_INDEPENDENCE = ((1000, 5), (2000, 5))  # sonde relancée avec le masque 6 : voie q3 inchangée
SEPARATION, WORKERS, BACKEND = 8, 4, 28


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256(path):
    return sha256_bytes(Path(path).read_bytes())


def canonical_records(records):
    rows = sorted((tuple(r["support"][:3]), r["depth"]) for r in records if r["arity"] == 3)
    return rows, sha256_bytes(json.dumps(rows).encode())


def harness_rows(path):
    rows = []
    for line in Path(path).read_text().splitlines():
        a, b, c, depth = line.split()
        rows.append(((int(a), int(b), int(c)), int(depth)))
    rows.sort()
    return rows, sha256_bytes(json.dumps(rows).encode())


def compare(row):
    """Vérifications d'une ligne, depuis les comptes conservés dans le reçu."""
    probe, mine = row["probe"], row["harness"]
    q3 = mine["sample"]["q3"]
    require(probe["front"]["work"]["residual_pair_mass"][1] == mine["front"]["residual_pair_mass"][1], "masse résiduelle q3 différente")
    require(probe["front"]["total_unordered_pairs"] == mine["front"]["total_unordered_pairs"], "nombre de paires différent")
    require(probe["work"]["q3"]["seeds"] == q3["seeds"]["rejectable"] + q3["seeds"]["kept"], "seeds q3 différents")
    require(probe["work"]["q3"]["emitted"] == probe["work"]["q3_emitted"] == q3["seeds"]["emitted_kept"] == row["records_q3"] == row["harness_records"],
            "boules q3 émises différentes")
    require(row["records_sha256"] == row["harness_sha256"], "multiensemble (support, profondeur) différent")
    require(q3["descent"]["disagreements"] == 0 and q3["descent_near_first"]["disagreements"] == 0, "descente saturante en désaccord")
    require(q3["lemma"]["violations"] == 0 and q3["lemma"]["pairs"] == min(row["lemma_budget"], q3["rejectable"]), "lemme du citron : violation ou budget non tenu")
    require(mine["exhaustive"] is True and q3["pairs"] == mine["front"]["residual_pair_mass"][1], "le harnais n'a pas jugé toutes les paires résiduelles")
    require(q3["rejectable"] > 0 and q3["kept"] > 0 and q3["seeds"]["emitted_kept"] > 0, "ligne vide")
    if row.get("independence") is not None:
        ind = row["independence"]
        require(ind["front"]["work"]["residual_pair_mass"][1] == mine["front"]["residual_pair_mass"][1] and
                ind["work"]["q3"]["seeds"] == probe["work"]["q3"]["seeds"] and ind["work"]["q3"]["emitted"] == probe["work"]["q3"]["emitted"],
                "la voie q3 dépend du masque demandé")


def run(args):
    worktree = args.worktree.resolve()
    head = subprocess.check_output(["git", "-C", str(worktree), "rev-parse", "--short=8", "HEAD"], text=True).strip()
    require(head == COMMIT, f"arbre de travail épinglé attendu à {COMMIT}, trouvé {head}")
    require(not subprocess.check_output(["git", "-C", str(worktree), "status", "--porcelain", "--", "morsehgp3D_v8/src", "morsehgp3D_v8/bench"], text=True).strip(),
            "sources épinglées modifiées")
    require(sha256(SOURCE) == SOURCE_SHA256, "scan préparé altéré")
    library = worktree / "build/v8-audit/libmhgp8_p0.a"
    probe_binary = worktree / "build/v8-audit/mhgp8_wspd_q34_probe"
    source = HERE / "q3_stream_probe.cpp"
    harness = args.scratch.resolve() / "q3_stream_probe"
    build = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(worktree / "morsehgp3D_v8/src"),
             str(source), str(library), "-pthread", "-o", str(harness)]
    subprocess.run(build, check=True)
    pins = dict(commit=COMMIT, worktree=str(worktree), library_sha256=sha256(library), probe_binary_sha256=sha256(probe_binary),
                probe_source_sha256=sha256(worktree / "morsehgp3D_v8/bench/wspd_q34_probe.cpp"),
                harness_sha256=sha256(source), harness_binary_sha256=sha256(harness), runner_sha256=sha256(Path(__file__)),
                source_scan=str(SOURCE.relative_to(ROOT)), source_sha256=SOURCE_SHA256, build_command=build)
    data = SOURCE.read_bytes()
    rows = []
    for n, kmax, budget in GRID:
        prefix = args.scratch / f"scan0_n{n}.u16le"
        prefix.write_bytes(data[: 6 * n])
        supports = args.scratch / f"supports_n{n}_k{kmax}.txt"
        probe_cmd = [str(probe_binary), str(SOURCE), str(n), str(kmax), str(SEPARATION), "2", str(BACKEND), str(WORKERS), "samples", "records"]
        started = time.time()
        completed = subprocess.run(probe_cmd, capture_output=True, text=True)
        require(completed.returncode == 0, f"sonde en échec : {completed.stderr[:300]}")
        probe = json.loads(completed.stdout)
        probe_seconds = time.time() - started
        records, records_sha = canonical_records(probe.pop("records"))
        harness_cmd = [str(harness), str(prefix), str(kmax), str(SEPARATION), "0", "1", "samples", str(supports), str(budget)]
        started = time.time()
        completed = subprocess.run(harness_cmd, capture_output=True, text=True)
        require(completed.returncode == 0 and not completed.stderr, f"harnais en échec : {completed.stderr[:300]}")
        mine = json.loads(completed.stdout)
        harness_seconds = time.time() - started
        mine_rows, mine_sha = harness_rows(supports)
        row = dict(n=n, kmax=kmax, lemma_budget=budget, prefix_sha256=sha256(prefix), probe_command=probe_cmd, harness_command=harness_cmd,
                   probe=probe, probe_seconds=probe_seconds, records_q3=len(records), records_sha256=records_sha,
                   harness=mine, harness_seconds=harness_seconds, harness_records=len(mine_rows), harness_sha256=mine_sha)
        if (n, kmax) in LANE_INDEPENDENCE:
            cmd = [str(probe_binary), str(SOURCE), str(n), str(kmax), str(SEPARATION), "6", str(BACKEND), str(WORKERS), "samples", "digest"]
            completed = subprocess.run(cmd, capture_output=True, text=True)
            require(completed.returncode == 0, f"sonde masque 6 en échec : {completed.stderr[:300]}")
            row["independence"] = json.loads(completed.stdout)
            row["independence_command"] = cmd
        compare(row)
        rows.append(row)
        print(json.dumps(dict(n=n, kmax=kmax, seeds=probe["work"]["q3"]["seeds"], emitted=probe["work"]["q3"]["emitted"], identical=True,
                              probe_s=round(probe_seconds, 1), harness_s=round(harness_seconds, 1))), flush=True)
    receipt = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="q3 stream of run_wspd_q34_parallel (mask 2, Local28 unused) vs exhaustive independent enumeration; q4 not judged",
                   pins=pins, grid=[dict(n=n, kmax=k, lemma_budget=b) for n, k, b in GRID], separation=SEPARATION, workers=WORKERS,
                   rows=rows, finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", rows=len(rows), output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["pins"]["commit"] == COMMIT and receipt["pins"]["harness_sha256"] == sha256(HERE / "q3_stream_probe.cpp"), "harnais différent du reçu")
    require(sha256(SOURCE) == receipt["pins"]["source_sha256"] == SOURCE_SHA256, "scan préparé altéré")
    data = SOURCE.read_bytes()
    require([(r["n"], r["kmax"], r["lemma_budget"]) for r in receipt["rows"]] == [list(x) for x in GRID] or
            [(r["n"], r["kmax"], r["lemma_budget"]) for r in receipt["rows"]] == list(GRID), "grille incomplète")
    for row in receipt["rows"]:
        require(row["prefix_sha256"] == sha256_bytes(data[: 6 * row["n"]]), "préfixe altéré")
        compare(row)
    print(json.dumps(dict(status="passed", rows=len(receipt["rows"]))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--worktree", type=Path, required=True)
    capture.add_argument("--scratch", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE / "Q3_STREAM_CROSSCHECK.json")
    reader = sub.add_parser("read")
    reader.add_argument("--output", type=Path, default=HERE / "Q3_STREAM_CROSSCHECK.json")
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
