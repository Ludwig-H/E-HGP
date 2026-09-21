#!/usr/bin/env python3
"""Contrôle croisé SPATIAL de l'auditeur B (moteur 34 gelé) : flux q3 et q4 de `run_wspd_q34_parallel` (modes
`rectangle-pair`, `boxes`, `affine`, atlas `joined`) sur les morceaux entiers de la scène 0 préparés indépendamment
(prepare_spatial_b.py : quarts, moitiés), contre les harnais V2 à descentes d'index (q3_stream_probe_v2.cpp,
q4_stream_probe_v2.cpp), qui reproduisent à l'octet près les sorties des harnais V1 sur les préfixes 1k à 8k contre les mêmes énumérations exhaustives indépendantes que pour la tranche 31.

Une exécution de la sonde du constructeur (masque 6, Local28, quatre workers, records) par point de grille donne les deux
flux ; le harnais q3 (toutes les paires résiduelles q3, citron exact, seeds propriétaires, census entier des circumboules)
et le harnais q4 (toutes les paires résiduelles q4, tétraèdres propriétaires positifs, profondeur exacte) sont rebâtis
contre la bibliothèque du commit épinglé. Égalités exigées : masses résiduelles par voie, seeds q3, boules q3 émises et
multiensemble (support, profondeur), ensemble des boules q4 distinctes (clé, profondeur), supports q4 sur leur sphère.
Sur chaque morceau la sonde est relancée avec l'atlas q4 `live` (bloc 64), le mode de la campagne chronométrée du
constructeur : mêmes comptes q3/q4 et mêmes empreintes xor/somme qu'en `joined`.
Aucun assert : rejouable sous python3 -O. Le commit est un paramètre de `run`, conservé dans le reçu et exigé par `read`.
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
MANIFEST = HERE / "SPATIAL_PIECES_SCAN0.json"
SCHEMA = "audit_b_q34_stream_crosscheck_spatial_v1"
GRID = (("quarter_xpos_ypos", 5, 100, 50), ("quarter_xpos_yneg", 5, 100, 50), ("quarter_xneg_yneg", 5, 100, 50), ("quarter_xneg_ypos", 5, 100, 50))
MODE_CHECK = tuple((p, 5) for p in ("quarter_xpos_ypos", "quarter_xpos_yneg", "quarter_xneg_yneg", "quarter_xneg_ypos"))  # relance en atlas `live` (mode de la campagne chronométrée du constructeur)
SEPARATION, WORKERS, BACKEND = 8, 4, 28
WITNESS_MODE, CENSUS_MODE, BOUNDS_MODE, ATLAS_MODE = "rectangle-pair", "boxes", "affine", "joined"


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


def split_records(records, points):
    q3 = sorted((tuple(r["support"][:3]), r["depth"]) for r in records if r["arity"] == 3)
    q4, off_sphere, q4_tetras = set(), 0, 0
    for r in records:
        if r["arity"] != 4:
            continue
        q4_tetras += 1
        ids = r["support"][:4]
        key = ball_key(*[points[i] for i in ids])
        if key is None:
            off_sphere += 1
            continue
        for i in ids:
            z = points[i]
            if key[0] * dot(z, z) + key[1] * z[0] + key[2] * z[1] + key[3] * z[2] + key[4] != 0:
                off_sphere += 1
        q4.add((key, r["depth"]))
    return q3, q4, off_sphere, q4_tetras


def digest(rows):
    return sha256_bytes(json.dumps(sorted(rows)).encode())


def harness_q3_rows(path):
    rows = []
    for line in Path(path).read_text().splitlines():
        a, b, c, depth = line.split()
        rows.append(((int(a), int(b), int(c)), int(depth)))
    return sorted(rows)


def harness_q4_balls(path):
    balls = set()
    for line in Path(path).read_text().splitlines():
        fields = line.split()
        balls.add((tuple(int(x) for x in fields[:5]), int(fields[5])))
    return balls


def compare(row):
    probe, h3, h4 = row["probe"], row["harness_q3"], row["harness_q4"]
    s3, q4 = h3["sample"]["q3"], h4["q4"]
    residual = probe["front"]["work"]["residual_pair_mass"]
    require(probe.get("witness_mode") == WITNESS_MODE and probe.get("q3_census_mode") == CENSUS_MODE and probe.get("witness_bounds_mode", BOUNDS_MODE) == BOUNDS_MODE, "modes de la sonde inattendus")
    require(residual[1] == h3["front"]["residual_pair_mass"][1] and residual[2] == h4["front"]["residual_pair_mass"][2], "masses résiduelles différentes")
    require(probe["front"]["total_unordered_pairs"] == h3["front"]["total_unordered_pairs"] == h4["front"]["total_unordered_pairs"], "nombre de paires différent")
    require(h3["exhaustive"] is True and s3["pairs"] == residual[1] and q4["pairs"] == residual[2], "un harnais n'a pas jugé toutes les paires résiduelles")
    witness = probe["work"]["witness"]
    require(probe["work"]["q3_edges"] == s3["kept"] and witness["rectangle_q3_pairs"] + witness["pair_q3_pairs"] == s3["rejectable"],
            "la recherche de témoins q3 ne rejette pas exactement les paires du citron")
    require(probe["work"]["q4_edges"] == q4["kept"] and witness["rectangle_q4_pairs"] + witness["pair_q4_pairs"] == q4["rejectable"],
            "la recherche de témoins q4 ne rejette pas exactement les paires du citron")
    require(probe["work"]["q3"]["seeds"] == s3["seeds"]["kept"], "seeds q3 des paires survivantes différents")
    require(probe["work"]["q3"]["emitted"] == probe["work"]["q3_emitted"] == s3["seeds"]["emitted_kept"] == row["probe_q3_records"] == row["harness_q3_records"],
            "boules q3 émises différentes")
    require(row["probe_q3_sha256"] == row["harness_q3_sha256"], "multiensemble q3 (support, profondeur) différent")
    require(probe["work"]["q4_emitted"] == row["probe_q4_tetras"] and row["probe_q4_off_sphere"] == 0, "records q4 incohérents ou support hors sphère")
    require(row["probe_q4_distinct"] == row["harness_q4_distinct"] and row["probe_q4_sha256"] == row["harness_q4_sha256"], "ensembles de boules q4 différents")
    require(s3["descent"]["disagreements"] == 0 and s3["descent_near_first"]["disagreements"] == 0, "descente saturante q3 en désaccord")
    require(s3["lemma"]["violations"] == 0 and s3["lemma"]["pairs"] == min(row["lemma_budget_q3"], s3["rejectable"]), "lemme q3 : violation ou budget non tenu")
    require(q4["lemma"]["violations"] == 0 and q4["lemma"]["pairs"] == min(row["lemma_budget_q4"], q4["rejectable"]), "lemme q4 : violation ou budget non tenu")
    require(s3["rejectable"] > 0 and s3["kept"] > 0 and s3["seeds"]["emitted_kept"] > 0 and q4["kept"] > 0 and q4["emitted_tetras"] > 0, "ligne vide")
    require(witness["rejected_pairs"] + witness["rectangle_pair_mass"] > 0, "les nouveaux modes n'ont rien rejeté : mesure vide")
    if row.get("mode_check") is not None:
        ref = row["mode_check"]
        require(ref["output"]["q3"] == probe["output"]["q3"] and ref["output"]["q4"] == probe["output"]["q4"] and
                ref["output"]["xor"] == probe["output"]["xor"] and ref["output"]["sum"] == probe["output"]["sum"] and
                ref["output"]["shell_ids"] == probe["output"]["shell_ids"], "les atlas joined et live n'émettent pas le même flux")


def run(args):
    worktree = args.worktree.resolve()
    head = subprocess.check_output(["git", "-C", str(worktree), "rev-parse", "HEAD"], text=True).strip()
    require(head.startswith(args.commit) and len(args.commit) >= 8, f"arbre de travail à {head[:8]}, commit demandé {args.commit}")
    require(not subprocess.check_output(["git", "-C", str(worktree), "status", "--porcelain", "--", "morsehgp3D_v8/src", "morsehgp3D_v8/bench"], text=True).strip(),
            "sources épinglées modifiées")
    manifest = json.loads(MANIFEST.read_text())
    library = worktree / "build/v8-audit/libmhgp8_p0.a"
    probe_binary = worktree / "build/v8-audit/mhgp8_wspd_q34_probe"
    binaries, builds = {}, {}
    for lane in ("q3", "q4"):
        source = HERE / f"{lane}_stream_probe_v2.cpp"
        binary = args.scratch.resolve() / f"{lane}_stream_probe"
        build = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(worktree / "morsehgp3D_v8/src"),
                 str(source), str(library), "-pthread", "-o", str(binary)]
        subprocess.run(build, check=True)
        binaries[lane], builds[lane] = binary, build
    pins = dict(commit=head, worktree=str(worktree), library_sha256=sha256(library), probe_binary_sha256=sha256(probe_binary),
                probe_source_sha256=sha256(worktree / "morsehgp3D_v8/bench/wspd_q34_probe.cpp"),
                wspd_q34_source_sha256=sha256(worktree / "morsehgp3D_v8/src/pipeline/wspd_q34.cpp"),
                witness_search_source_sha256=sha256(worktree / "morsehgp3D_v8/src/lanes/q34_witness_search.cpp"),
                q3_ball_census_source_sha256=sha256(worktree / "morsehgp3D_v8/src/lanes/q3_ball_census.cpp"),
                harness_q3_sha256=sha256(HERE / "q3_stream_probe_v2.cpp"), harness_q4_sha256=sha256(HERE / "q4_stream_probe_v2.cpp"), manifest_sha256=sha256(MANIFEST),
                runner_sha256=sha256(Path(__file__)), manifest=str(MANIFEST.relative_to(ROOT)), build_commands=builds)
    rows = []
    for piece, kmax, budget3, budget4 in GRID:
        info = manifest["pieces"][piece]
        prefix = Path(info["path"]).resolve()
        require(sha256(prefix) == info["sha256"] and prefix.stat().st_size == 6 * info["n"], f"morceau altéré : {piece}")
        n = info["n"]
        probe_cmd = [str(probe_binary), str(prefix), str(n), str(kmax), str(SEPARATION), "6", str(BACKEND), str(WORKERS), "samples", "records", WITNESS_MODE, CENSUS_MODE, BOUNDS_MODE, ATLAS_MODE]
        started = time.time()
        completed = subprocess.run(probe_cmd, capture_output=True, text=True)
        require(completed.returncode == 0, f"sonde en échec : {completed.stderr[:300]}")
        probe = json.loads(completed.stdout)
        probe_seconds = time.time() - started
        q3_rows, q4_balls, off_sphere, q4_tetras = split_records(probe.pop("records"), points_of(prefix.read_bytes(), n))
        supports = args.scratch / f"supports_{piece}_k{kmax}.txt"
        cmd3 = [str(binaries["q3"]), str(prefix), str(kmax), str(SEPARATION), "0", "1", "samples", str(supports), str(budget3)]
        started = time.time()
        completed = subprocess.run(cmd3, capture_output=True, text=True)
        require(completed.returncode == 0 and not completed.stderr, f"harnais q3 en échec : {completed.stderr[:300]}")
        h3 = json.loads(completed.stdout)
        seconds3 = time.time() - started
        balls_file = args.scratch / f"balls_{piece}_k{kmax}.txt"
        cmd4 = [str(binaries["q4"]), str(prefix), str(kmax), str(SEPARATION), str(balls_file), str(budget4)]
        started = time.time()
        completed = subprocess.run(cmd4, capture_output=True, text=True)
        require(completed.returncode == 0 and not completed.stderr, f"harnais q4 en échec : {completed.stderr[:300]}")
        h4 = json.loads(completed.stdout)
        seconds4 = time.time() - started
        mine3, mine4 = harness_q3_rows(supports), harness_q4_balls(balls_file)
        row = dict(piece=piece, n=n, kmax=kmax, lemma_budget_q3=budget3, lemma_budget_q4=budget4, piece_sha256=info["sha256"],
                   probe_command=probe_cmd, probe=probe, probe_seconds=probe_seconds,
                   probe_q3_records=len(q3_rows), probe_q3_sha256=digest(q3_rows), probe_q4_tetras=q4_tetras, probe_q4_off_sphere=off_sphere,
                   probe_q4_distinct=len(q4_balls), probe_q4_sha256=digest(q4_balls),
                   harness_q3_command=cmd3, harness_q3=h3, harness_q3_seconds=seconds3, harness_q3_records=len(mine3), harness_q3_sha256=digest(mine3),
                   harness_q4_command=cmd4, harness_q4=h4, harness_q4_seconds=seconds4, harness_q4_distinct=len(mine4), harness_q4_sha256=digest(mine4))
        if (piece, kmax) in MODE_CHECK:
            cmd = [str(probe_binary), str(prefix), str(n), str(kmax), str(SEPARATION), "6", str(BACKEND), str(WORKERS), "samples", "digest", WITNESS_MODE, CENSUS_MODE, BOUNDS_MODE, "live", "64"]
            completed = subprocess.run(cmd, capture_output=True, text=True)
            require(completed.returncode == 0, f"sonde disabled/scalar en échec : {completed.stderr[:300]}")
            row["mode_check"] = json.loads(completed.stdout)
            row["mode_check_command"] = cmd
        compare(row)
        rows.append(row)
        print(json.dumps(dict(piece=piece, n=n, kmax=kmax, q3=len(q3_rows), q4=len(q4_balls), identical=True, probe_s=round(probe_seconds, 1),
                              q3_s=round(seconds3, 1), q4_s=round(seconds4, 1))), flush=True)
    receipt = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="spatial pieces of scan 0 (independent preparation): q3 and q4 streams of run_wspd_q34_parallel in rectangle-pair/boxes/affine/joined modes vs exhaustive independent enumerations; shells not compared",
                   pins=pins, grid=[dict(piece=p, kmax=k, lemma_budget_q3=b3, lemma_budget_q4=b4) for p, k, b3, b4 in GRID],
                   separation=SEPARATION, workers=WORKERS, witness_mode=WITNESS_MODE, census_mode=CENSUS_MODE, bounds_mode=BOUNDS_MODE, atlas_mode=ATLAS_MODE,
                   rows=rows, finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", rows=len(rows), commit=head[:8], output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["pins"]["harness_q3_sha256"] == sha256(HERE / "q3_stream_probe_v2.cpp") and
            receipt["pins"]["harness_q4_sha256"] == sha256(HERE / "q4_stream_probe_v2.cpp") and
            receipt["pins"]["manifest_sha256"] == sha256(MANIFEST), "harnais ou manifeste différents du reçu")
    require(args.commit is None or receipt["pins"]["commit"].startswith(args.commit), "commit du reçu différent de celui demandé")
    manifest = json.loads(MANIFEST.read_text())
    require([(r["piece"], r["kmax"], r["lemma_budget_q3"], r["lemma_budget_q4"]) for r in receipt["rows"]] == list(GRID), "grille incomplète")
    for row in receipt["rows"]:
        require(row["piece_sha256"] == manifest["pieces"][row["piece"]]["sha256"] and row["n"] == manifest["pieces"][row["piece"]]["n"], "morceau altéré")
        compare(row)
    print(json.dumps(dict(status="passed", rows=len(receipt["rows"]), commit=receipt["pins"]["commit"][:8])))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub_parsers = parser.add_subparsers(dest="operation", required=True)
    capture = sub_parsers.add_parser("run")
    capture.add_argument("--worktree", type=Path, required=True)
    capture.add_argument("--scratch", type=Path, required=True)
    capture.add_argument("--commit", required=True, help="préfixe (≥ 8 hex) du commit épinglé de l'arbre de travail")
    capture.add_argument("--output", type=Path, default=HERE / "Q34_STREAM_CROSSCHECK_SPATIAL.json")
    reader = sub_parsers.add_parser("read")
    reader.add_argument("--commit", default=None)
    reader.add_argument("--output", type=Path, default=HERE / "Q34_STREAM_CROSSCHECK_T34.json")
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
