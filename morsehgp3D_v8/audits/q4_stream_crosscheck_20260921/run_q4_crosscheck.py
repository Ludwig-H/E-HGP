#!/usr/bin/env python3
"""Contrôle croisé de l'auditeur B : flux q4 (Local28) du raccord global 31 (4dbe3024) contre une énumération exhaustive indépendante.

Pour chaque préfixe du scan 0 (1k/2k/4k à K5, 1k/2k à K10), la sonde du constructeur `mhgp8_wspd_q34_probe` (masque 4,
Local28, quatre workers, mode records) et le harnais de B (toutes les paires résiduelles de la voie q4, citron exact,
tétraèdres propriétaires avec la règle `owned` du raccord, positivité stricte et profondeur en i128 sur la couverture)
doivent donner la même masse résiduelle q4 et le même ensemble de boules distinctes (clé entière réduite, profondeur).
La clé de chaque record de la sonde est recalculée ici en entiers Python depuis son support et les points du préfixe,
et chaque support doit être sur sa propre sphère. Aucun assert : rejouable sous python3 -O.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from math import gcd
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared/single_000000/n8000.u16le"
SOURCE_SHA256 = "ed5aa97941551e027a76d7c4eb4a03c739cdf0a83cbaf53035c1536bed0b5194"
COMMIT = "4dbe3024"
SCHEMA = "audit_b_q4_stream_crosscheck_v1"
GRID = ((1000, 5, 10 ** 9), (2000, 5, 2000), (4000, 5, 500), (1000, 10, 10 ** 9), (2000, 10, 2000))  # (n, K, budget du lemme)
LANE_INDEPENDENCE = ((1000, 5), (2000, 5))  # sonde relancée avec le masque 6 : voie q4 inchangée
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


def points_of(data, n):
    return [tuple(int.from_bytes(data[6 * i + 2 * axis: 6 * i + 2 * axis + 2], "little") for axis in range(3)) for i in range(n)]


def sub(p, q):
    return tuple(x - y for x, y in zip(p, q))


def dot(p, q):
    return sum(x * y for x, y in zip(p, q))


def cross(p, q):
    return (p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0])


def ball_key(a, b, c, d):
    """Clé réduite [A, Bx, By, Bz, C] de la sphère circonscrite (None si coplanaire) ; f(z) = A|z|² + B·z + C."""
    u, v, w = sub(b, a), sub(c, a), sub(d, a)
    vw, wu, uv = cross(v, w), cross(w, u), cross(u, v)
    delta = dot(u, vw)
    if delta == 0:
        return None
    big = tuple(dot(u, u) * p + dot(v, v) * q + dot(w, w) * r for p, q, r in zip(vw, wu, uv))
    sign = 1 if delta > 0 else -1
    key = [sign * delta, *[sign * (-2 * delta * ai - ni) for ai, ni in zip(a, big)], sign * (delta * dot(a, a) + dot(big, a))]
    g = 0
    for value in key:
        g = gcd(g, abs(value))
    return tuple(value // g for value in key)


def probe_balls(records, points):
    """Ensemble des (clé, profondeur) des records d'arité 4 ; compte des supports hors de leur sphère (doit être 0)."""
    balls, off_sphere, tetras = set(), 0, 0
    for record in records:
        if record["arity"] != 4:
            continue
        tetras += 1
        ids = record["support"][:4]
        key = ball_key(*[points[i] for i in ids])
        if key is None:
            off_sphere += 1
            continue
        for i in ids:
            z = points[i]
            if key[0] * dot(z, z) + key[1] * z[0] + key[2] * z[1] + key[3] * z[2] + key[4] != 0:
                off_sphere += 1
        balls.add((key, record["depth"]))
    return balls, off_sphere, tetras


def harness_balls(path):
    balls = set()
    for line in Path(path).read_text().splitlines():
        fields = line.split()
        balls.add((tuple(int(x) for x in fields[:5]), int(fields[5])))
    return balls


def digest(balls):
    return sha256_bytes(json.dumps(sorted(balls)).encode())


def compare(row):
    probe, mine = row["probe"], row["harness"]
    q4 = mine["q4"]
    require(probe["front"]["work"]["residual_pair_mass"][2] == mine["front"]["residual_pair_mass"][2], "masse résiduelle q4 différente")
    require(probe["front"]["total_unordered_pairs"] == mine["front"]["total_unordered_pairs"], "nombre de paires différent")
    require(q4["pairs"] == mine["front"]["residual_pair_mass"][2], "le harnais n'a pas jugé toutes les paires résiduelles q4")
    require(probe["work"]["q4_emitted"] == row["probe_tetras"], "registre q4 de la sonde incohérent avec ses records")
    require(row["probe_off_sphere"] == 0, "un support de la sonde n'est pas sur sa sphère")
    require(row["probe_distinct_balls"] == row["harness_distinct_balls"] and row["probe_balls_sha256"] == row["harness_balls_sha256"],
            "ensembles de boules q4 (clé, profondeur) différents")
    require(q4["lemma"]["violations"] == 0 and q4["lemma"]["pairs"] == min(row["lemma_budget"], q4["rejectable"]), "lemme du citron q4 : violation ou budget non tenu")
    require(q4["rejectable"] > 0 and q4["kept"] > 0 and q4["emitted_tetras"] > 0 and q4["positive_tetras"] > q4["emitted_tetras"], "ligne vide")
    if row.get("independence") is not None:
        ind = row["independence"]
        require(ind["front"]["work"]["residual_pair_mass"][2] == mine["front"]["residual_pair_mass"][2] and
                ind["work"]["q4_emitted"] == probe["work"]["q4_emitted"], "la voie q4 dépend du masque demandé")


def run(args):
    worktree = args.worktree.resolve()
    head = subprocess.check_output(["git", "-C", str(worktree), "rev-parse", "--short=8", "HEAD"], text=True).strip()
    require(head == COMMIT, f"arbre de travail épinglé attendu à {COMMIT}, trouvé {head}")
    require(not subprocess.check_output(["git", "-C", str(worktree), "status", "--porcelain", "--", "morsehgp3D_v8/src", "morsehgp3D_v8/bench"], text=True).strip(),
            "sources épinglées modifiées")
    require(sha256(SOURCE) == SOURCE_SHA256, "scan préparé altéré")
    library = worktree / "build/v8-audit/libmhgp8_p0.a"
    probe_binary = worktree / "build/v8-audit/mhgp8_wspd_q34_probe"
    source = HERE / "q4_stream_probe.cpp"
    harness = args.scratch.resolve() / "q4_stream_probe"
    build = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(worktree / "morsehgp3D_v8/src"),
             str(source), str(library), "-pthread", "-o", str(harness)]
    subprocess.run(build, check=True)
    pins = dict(commit=COMMIT, worktree=str(worktree), library_sha256=sha256(library), probe_binary_sha256=sha256(probe_binary),
                probe_source_sha256=sha256(worktree / "morsehgp3D_v8/bench/wspd_q34_probe.cpp"),
                q4_local_source_sha256=sha256(worktree / "morsehgp3D_v8/src/lanes/q4_local.cpp"),
                harness_sha256=sha256(source), harness_binary_sha256=sha256(harness), runner_sha256=sha256(Path(__file__)),
                source_scan=str(SOURCE.relative_to(ROOT)), source_sha256=SOURCE_SHA256, build_command=build)
    data = SOURCE.read_bytes()
    rows = []
    for n, kmax, budget in GRID:
        prefix = args.scratch / f"scan0_n{n}.u16le"
        prefix.write_bytes(data[: 6 * n])
        balls_file = args.scratch / f"balls_n{n}_k{kmax}.txt"
        probe_cmd = [str(probe_binary), str(SOURCE), str(n), str(kmax), str(SEPARATION), "4", str(BACKEND), str(WORKERS), "samples", "records"]
        started = time.time()
        completed = subprocess.run(probe_cmd, capture_output=True, text=True)
        require(completed.returncode == 0, f"sonde en échec : {completed.stderr[:300]}")
        probe = json.loads(completed.stdout)
        probe_seconds = time.time() - started
        balls, off_sphere, tetras = probe_balls(probe.pop("records"), points_of(data, n))
        harness_cmd = [str(harness), str(prefix), str(kmax), str(SEPARATION), str(balls_file), str(budget)]
        started = time.time()
        completed = subprocess.run(harness_cmd, capture_output=True, text=True)
        require(completed.returncode == 0 and not completed.stderr, f"harnais en échec : {completed.stderr[:300]}")
        mine = json.loads(completed.stdout)
        harness_seconds = time.time() - started
        mine_balls = harness_balls(balls_file)
        row = dict(n=n, kmax=kmax, lemma_budget=budget, prefix_sha256=sha256(prefix), probe_command=probe_cmd, harness_command=harness_cmd,
                   probe=probe, probe_seconds=probe_seconds, probe_tetras=tetras, probe_off_sphere=off_sphere,
                   probe_distinct_balls=len(balls), probe_balls_sha256=digest(balls),
                   harness=mine, harness_seconds=harness_seconds, harness_distinct_balls=len(mine_balls), harness_balls_sha256=digest(mine_balls))
        if (n, kmax) in LANE_INDEPENDENCE:
            cmd = [str(probe_binary), str(SOURCE), str(n), str(kmax), str(SEPARATION), "6", str(BACKEND), str(WORKERS), "samples", "digest"]
            completed = subprocess.run(cmd, capture_output=True, text=True)
            require(completed.returncode == 0, f"sonde masque 6 en échec : {completed.stderr[:300]}")
            row["independence"] = json.loads(completed.stdout)
            row["independence_command"] = cmd
        compare(row)
        rows.append(row)
        print(json.dumps(dict(n=n, kmax=kmax, balls=len(balls), tetras_probe=tetras, tetras_harness=mine["q4"]["emitted_tetras"], identical=True,
                              probe_s=round(probe_seconds, 1), harness_s=round(harness_seconds, 1))), flush=True)
    receipt = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="q4 stream of run_wspd_q34_parallel (mask 4, Local28) vs exhaustive independent enumeration of owned positive tetrahedra; shells not compared",
                   pins=pins, grid=[dict(n=n, kmax=k, lemma_budget=b) for n, k, b in GRID], separation=SEPARATION, workers=WORKERS,
                   rows=rows, finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", rows=len(rows), output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["pins"]["commit"] == COMMIT and receipt["pins"]["harness_sha256"] == sha256(HERE / "q4_stream_probe.cpp"), "harnais différent du reçu")
    require(sha256(SOURCE) == receipt["pins"]["source_sha256"] == SOURCE_SHA256, "scan préparé altéré")
    data = SOURCE.read_bytes()
    require([(r["n"], r["kmax"], r["lemma_budget"]) for r in receipt["rows"]] == list(GRID), "grille incomplète")
    for row in receipt["rows"]:
        require(row["prefix_sha256"] == sha256_bytes(data[: 6 * row["n"]]), "préfixe altéré")
        compare(row)
    print(json.dumps(dict(status="passed", rows=len(receipt["rows"]))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub_parsers = parser.add_subparsers(dest="operation", required=True)
    capture = sub_parsers.add_parser("run")
    capture.add_argument("--worktree", type=Path, required=True)
    capture.add_argument("--scratch", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE / "Q4_STREAM_CROSSCHECK.json")
    reader = sub_parsers.add_parser("read")
    reader.add_argument("--output", type=Path, default=HERE / "Q4_STREAM_CROSSCHECK.json")
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
