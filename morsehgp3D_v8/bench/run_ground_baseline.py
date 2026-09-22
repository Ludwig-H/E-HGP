#!/usr/bin/env python3
"""Campagne de base du régime prioritaire : moteur u16 q3/q4 sur les trois nuages LiDAR sans sol (phase 0 de la reprise).

Pour chaque scène sans sol (profil u16 2 cm, reçu `lidar_ground_u16_20260921`) : K5 avec 1 puis 8 workers, K10 avec
8 workers ; s = 8, masque 6, Local28, configuration mesurée (`rectangle-pair`, `boxes`, `affine`, `live`, bloc 64), mode
`digest`. Chaque exécution est chronométrée par GNU time (mur, CPU, RSS) et son JSON de sonde est conservé tel quel.
Le reçu épingle le commit, l'état du worktree, la sonde (sha256), les sources du pipeline et les entrées. Le lecteur
`read` exige, à scène et K égaux, des sorties (xor, somme, IDs de coquille) et des compteurs géométriques identiques entre
1 et 8 workers (porte d'identité), des sorties non vides, et recalcule les grandeurs dérivées. Les temps sont ceux d'un
hôte partagé : ils orientent le développement, ils ne qualifient rien. Aucun assert : tient sous python3 -O.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SCHEMA = "mhgp8_ground_baseline_v1"
MANIFEST = ROOT / "morsehgp3D_v8/receipts/lidar_ground_u16_20260921/MANIFEST.json"
OUTPUT = ROOT / "morsehgp3D_v8/receipts/ground_baseline_20260921"
PIPELINE_SOURCES = ("src/pipeline/wspd_q34.cpp", "src/pipeline/wspd_q34.hpp", "src/wspd/front.cpp", "src/lanes/q34_witness_search.cpp",
                    "src/lanes/q3_ball_census.cpp", "src/lanes/q4_local.cpp", "src/lanes/q4_local_partition.cpp", "src/lanes/edge_cover.cpp",
                    "bench/wspd_q34_probe.cpp")
GRID = tuple((scene, kmax, workers) for scene in ("00", "01", "02") for kmax, workers in ((5, 1), (5, 8), (10, 8)))
SEPARATION, MASK, BACKEND = 8, 6, 28
MODES = ("samples", "digest", "rectangle-pair", "boxes", "affine", "live", "64")
# Compteurs qui dépendent de l'attribution des jobs (capacités, pics) : exclus de l'identité W1/W8.
NONLOGICAL = re.compile(r"peak|capacity|retained_bytes|storage_bytes|_bytes$|jobs|workers|started|target_jobs|completed|terminal|prefix_product")


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(*options):
    return subprocess.check_output(["git", *options], cwd=ROOT, text=True).strip()


def logical(tree, prefix=""):
    """Aplatit les compteurs entiers d'un sous-arbre JSON en excluant les capacités et l'ordonnancement."""
    out = {}
    if isinstance(tree, dict):
        for key, value in tree.items():
            name = f"{prefix}.{key}" if prefix else key
            if NONLOGICAL.search(key):
                continue
            out.update(logical(value, name))
    elif isinstance(tree, list):
        for i, value in enumerate(tree):
            out.update(logical(value, f"{prefix}[{i}]"))
    elif isinstance(tree, bool):
        pass
    elif isinstance(tree, int):
        out[prefix] = tree
    return out


def gnu_time_fields(text):
    fields = {}
    for pattern, key, convert in (
            (r"Elapsed \(wall clock\) time .*: (.+)", "wall", lambda s: sum(float(x) * 60 ** i for i, x in enumerate(reversed(s.split(":"))))),
            (r"User time \(seconds\): ([\d.]+)", "user_s", float), (r"System time \(seconds\): ([\d.]+)", "system_s", float),
            (r"Percent of CPU this job got: (\d+)%", "cpu_percent", int), (r"Maximum resident set size \(kbytes\): (\d+)", "max_rss_kb", int)):
        m = re.search(pattern, text)
        require(m is not None, f"champ GNU time absent : {key}")
        fields[key] = convert(m.group(1))
    fields["wall_s"] = round(fields.pop("wall"), 3)
    return fields


def run(args):
    probe = args.probe.resolve()
    require(probe.is_file(), f"sonde absente : {probe}")
    manifest = json.loads(MANIFEST.read_text())
    require(manifest["schema"] == "mhgp8_lidar_ground_u16_preparation_v1", "manifeste u16 sans sol inattendu")
    inputs = {}
    for record in manifest["scenes"]:
        path = ROOT / record["pieces"]["full"]["path"]
        require(path.is_file() and sha256(path) == record["pieces"]["full"]["sha256"], f"entrée absente ou altérée : {path} (régénérer par prepare_lidar_ground_u16.py run)")
        inputs[record["scene"]] = dict(path=str(path.relative_to(ROOT)), n=record["pieces"]["full"]["n"], sha256=record["pieces"]["full"]["sha256"], frame=record["frame"])
    pins = dict(git_commit=git("rev-parse", "HEAD"), worktree_status=git("status", "--porcelain", "--", "morsehgp3D_v8/src", "morsehgp3D_v8/bench"),
                probe=str(probe), probe_sha256=sha256(probe), runner_sha256=sha256(Path(__file__)), manifest_sha256=sha256(MANIFEST),
                pipeline_sources_sha256={s: sha256(ROOT / "morsehgp3D_v8" / s) for s in PIPELINE_SOURCES}, host=subprocess.check_output(["uname", "-a"], text=True).strip(),
                cpus=subprocess.check_output(["nproc"], text=True).strip(), inputs=inputs)
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    prefix = "only_" if args.only else ""
    receipt_name = "BASELINE.only.json" if args.only else "BASELINE.json"
    require(not (args.output / receipt_name).exists(), f"reçu déjà présent, jamais recouvert : {args.output / receipt_name}")
    rows = []
    for index, (scene, kmax, workers) in enumerate(GRID):
        if args.only and (scene, kmax, workers) not in args.only:
            continue
        entry = inputs[scene]
        command = [str(probe), str(ROOT / entry["path"]), str(entry["n"]), str(kmax), str(SEPARATION), str(MASK), str(BACKEND), str(workers), *MODES, *args.extra]
        time_file = args.output / f"{prefix}time_{index:02}_s{scene}_k{kmax}_w{workers}.txt"
        json_file = args.output / f"{prefix}probe_{index:02}_s{scene}_k{kmax}_w{workers}.json"
        require(not time_file.exists() and not json_file.exists(), f"fichiers de mesure déjà présents, jamais recouverts : {json_file.name}")
        load_before = Path("/proc/loadavg").read_text().split()[:3]
        started = time.time()
        with json_file.open("wb") as sink:
            completed = subprocess.run(["/usr/bin/time", "-v", "-o", str(time_file), *command], stdout=sink, stderr=subprocess.PIPE)
        require(completed.returncode == 0, f"sonde en échec ({scene}, K{kmax}, W{workers}) : {completed.stderr[-400:]!r}")
        probe_json = json.loads(json_file.read_text())
        timing = gnu_time_fields(time_file.read_text())
        row = dict(index=index, scene=scene, frame=entry["frame"], n=entry["n"], kmax=kmax, workers=workers, command=command, started_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(started)),
                   load_before=load_before, gnu_time=timing, probe_json=json_file.name, time_file=time_file.name, probe_json_sha256=sha256(json_file),
                   output=probe_json["output"], timings_ms=probe_json["timings_ms"], q3_emitted=probe_json["work"]["q3_emitted"], q4_emitted=probe_json["work"]["q4_emitted"],
                   logical_sha256=hashlib.sha256(json.dumps(logical({k: probe_json[k] for k in ("front", "work")}), sort_keys=True).encode()).hexdigest())
        rows.append(row)
        print(json.dumps(dict(scene=scene, kmax=kmax, workers=workers, wall_s=timing["wall_s"], cpu_s=round(timing["user_s"] + timing["system_s"], 1), cpu_percent=timing["cpu_percent"],
                              q3=row["q3_emitted"], q4=row["q4_emitted"], rss_mb=round(timing["max_rss_kb"] / 1024, 1))), flush=True)
        (args.output / "BASELINE.partial.json").write_text(json.dumps(dict(schema=SCHEMA, status="partial", pins=pins, rows=rows), sort_keys=True, indent=1) + "\n")
    receipt = dict(schema=SCHEMA, status="partial" if args.only else "passed", phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="developpement_phase0_base", public_status="not_claimed", gcp_used=False,
                   scope="u16 q3/q4 candidate stream on the three ground-free scenes: timings on a shared host (orientation only), logical counters, W1/W8 output identity",
                   pins=pins, grid=[dict(scene=s, kmax=k, workers=w) for s, k, w in GRID], separation=SEPARATION, mask=MASK, q4_backend=BACKEND, modes=[*MODES, *args.extra], rows=rows,
                   finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    check(receipt)
    (args.output / receipt_name).write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    (args.output / "BASELINE.partial.json").unlink(missing_ok=True)
    print(json.dumps(dict(status=receipt["status"], rows=len(rows), output=str(args.output.resolve().relative_to(ROOT)))))


def check(receipt):
    rows = receipt["rows"]
    by_key = {}
    for row in rows:
        require(row["q3_emitted"] > 0 and row["q4_emitted"] > 0, f"sortie vide : {row['scene']} K{row['kmax']} W{row['workers']}")
        require(row["output"]["q3"] == row["q3_emitted"] and row["output"]["q4"] == row["q4_emitted"], "registre d'émission différent du digest")
        by_key.setdefault((row["scene"], row["kmax"]), []).append(row)
    identity_pairs = 0
    for (scene, kmax), group in by_key.items():
        for other in group[1:]:
            first = group[0]
            require(other["output"] == first["output"], f"sorties différentes entre W{first['workers']} et W{other['workers']} : scène {scene} K{kmax}")
            require(other["logical_sha256"] == first["logical_sha256"], f"compteurs géométriques différents entre W{first['workers']} et W{other['workers']} : scène {scene} K{kmax}")
            identity_pairs += 1
    if receipt["status"] == "passed":
        require(identity_pairs >= 3, "porte d'identité W1/W8 vide")
        require([(r["scene"], r["kmax"], r["workers"]) for r in rows] == [tuple(g) for g in ((s, k, w) for s, k, w in GRID)], "grille incomplète")
    else:
        require(identity_pairs >= 1, "porte d'identité W1/W8 vide (reçu partiel)")


def read(args):
    receipt = json.loads((args.output / ("BASELINE.only.json" if args.variant == "only" else "BASELINE.json")).read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["status"] in ("passed", "partial"), "statut de reçu inconnu")
    for row in receipt["rows"]:
        json_file = args.output / row["probe_json"]
        require(sha256(json_file) == row["probe_json_sha256"], f"JSON de sonde altéré : {json_file.name}")
        probe_json = json.loads(json_file.read_text())
        require(probe_json["output"] == row["output"] and probe_json["work"]["q3_emitted"] == row["q3_emitted"], "ligne différente de son JSON")
        require(hashlib.sha256(json.dumps(logical({k: probe_json[k] for k in ("front", "work")}), sort_keys=True).encode()).hexdigest() == row["logical_sha256"], "empreinte logique non reproduite")
        require(gnu_time_fields((args.output / row["time_file"]).read_text()) == row["gnu_time"], "GNU time non reproduit")
    check(receipt)
    print(json.dumps(dict(status="passed", rows=len(receipt["rows"]), commit=receipt["pins"]["git_commit"][:8])))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub_parsers = parser.add_subparsers(dest="operation", required=True)
    capture = sub_parsers.add_parser("run")
    capture.add_argument("--probe", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=OUTPUT)
    capture.add_argument("--only", type=lambda s: tuple(x.strip() for x in s.split(",")), nargs="*", default=None, help="triplets scene,K,W à exécuter seulement")
    capture.add_argument("--extra", nargs="*", default=[], help="jetons CLI ajoutés après les modes (ex. atlas)")
    reader = sub_parsers.add_parser("read")
    reader.add_argument("--output", type=Path, default=OUTPUT)
    reader.add_argument("--variant", choices=("full", "only"), default="full")
    args = parser.parse_args()
    if getattr(args, "only", None):
        args.only = [(s, int(k), int(w)) for s, k, w in args.only]
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
