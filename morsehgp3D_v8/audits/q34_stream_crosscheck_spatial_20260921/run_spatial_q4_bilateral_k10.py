#!/usr/bin/env python3
"""Contrôle bilatéral de la voie q4 sur les sept morceaux spatiaux de la scène 0 (moteur 34 gelé, atlas `live`).

Pour chaque morceau (quarts, moitiés, scène) : la sonde du constructeur émet tous ses records (mode `records`, témoins
`rectangle-pair`, census `boxes`, bornes `affine`, atlas q4 `live` bloc 64 : le mode de sa campagne chronométrée) ;
les records d'arité 4 sont passés au harnais `q4_bilateral_probe.cpp`, qui (1) revérifie chaque record indépendamment
(arête propriétaire, positivité stricte, profondeur exacte, coquille complète) et (2) énumère, pour M paires tirées
dans la masse résiduelle q4 du même front et conservées par le citron exact, toutes les boules q4 propriétaires
positives de profondeur < K − 2, qui doivent toutes figurer parmi les records. Exigé : records valides = records, boules
manquantes = 0, paires conservées > 0. Aucun assert.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
MANIFEST = HERE / "SPATIAL_PIECES_SCAN0.json"
SCHEMA = "audit_b_q4_bilateral_spatial_k10_v1"
PIECES = ("quarter_xpos_ypos", "quarter_xpos_yneg", "quarter_xneg_yneg", "quarter_xneg_ypos", "half_xpos", "half_xneg", "full")
KMAX = (10,)
SEPARATION, WORKERS, BACKEND, SAMPLES, SEED = 8, 4, 28, 2000, 1
MODES = ("rectangle-pair", "boxes", "affine", "live", "64")


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256(path):
    return sha256_bytes(Path(path).read_bytes())


def split_probe_output(path, records_out):
    """Lit l'en-tête JSON de la sonde (tout sauf la clé finale « records ») et écrit les records d'arité 4 en texte, en flux.
    Les records sont des objets plats (tableaux d'entiers, jamais d'accolade interne) : découpage par expression régulière
    sur des blocs de 16 Mo avec report de la fin incomplète."""
    marker = b',"records":['
    size = path.stat().st_size
    with path.open("rb") as f:
        head = f.read(min(size, 64 << 20))
    pos = head.find(marker)
    require(pos >= 0, "sortie de sonde sans records")
    probe = json.loads(head[:pos].decode() + "}")
    n_q4 = n_q3 = 0
    pattern = re.compile(rb"\{[^{}]*\}")
    with path.open("rb") as f, records_out.open("w") as out:
        f.seek(pos + len(marker))
        carry = b""
        while True:
            chunk = f.read(16 << 20)
            if not chunk:
                break
            data = carry + chunk
            last = 0
            for m in pattern.finditer(data):
                r = json.loads(m.group(0))
                if r["arity"] == 4:
                    n_q4 += 1
                    out.write(f"{r['depth']} {' '.join(str(v) for v in r['support'][:4])} | {' '.join(str(v) for v in r['shell'])}\n")
                else:
                    n_q3 += 1
                last = m.end()
            carry = data[last:]
    return probe, n_q4, n_q3


def compare(row):
    probe, h = row["probe"], row["harness"]
    r, c = h["records"], h["completeness"]
    require(probe.get("witness_mode") == MODES[0] and probe.get("q3_census_mode") == MODES[1], "modes de la sonde inattendus")
    require(probe["front"]["work"]["residual_pair_mass"][2] == h["front"]["residual_pair_mass"][2], "masse résiduelle q4 différente")
    require(probe["work"]["q4_emitted"] == r["total"] == row["records_q4"], "nombre de records q4 incohérent")
    require(r["valid"] == r["total"] and r["owner_mismatch"] == r["not_positive"] == r["depth_mismatch"] == r["depth_over_threshold"] == r["shell_mismatch"] == 0,
            "un record q4 n'est pas une boule q4 valide")
    require(c["pairs"] == SAMPLES and c["missing"] == 0 and c["kept"] > 0 and c["balls"] > 0, "complétude : boule manquante ou échantillon vide")
    require(r["total"] > 0, "aucun record")


def run(args):
    worktree = args.worktree.resolve()
    head = subprocess.check_output(["git", "-C", str(worktree), "rev-parse", "HEAD"], text=True).strip()
    require(head.startswith(args.commit) and len(args.commit) >= 8, f"arbre de travail à {head[:8]}, commit demandé {args.commit}")
    require(not subprocess.check_output(["git", "-C", str(worktree), "status", "--porcelain", "--", "morsehgp3D_v8/src", "morsehgp3D_v8/bench"], text=True).strip(),
            "sources épinglées modifiées")
    manifest = json.loads(MANIFEST.read_text())
    library = worktree / "build/v8-audit/libmhgp8_p0.a"
    probe_binary = worktree / "build/v8-audit/mhgp8_wspd_q34_probe"
    source = HERE / "q4_bilateral_probe.cpp"
    harness = args.scratch.resolve() / "q4_bilateral_probe_bin"
    build = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(worktree / "morsehgp3D_v8/src"),
             str(source), str(library), "-pthread", "-o", str(harness)]
    subprocess.run(build, check=True)
    pins = dict(commit=head, worktree=str(worktree), library_sha256=sha256(library), probe_binary_sha256=sha256(probe_binary),
                q4_local_source_sha256=sha256(worktree / "morsehgp3D_v8/src/lanes/q4_local.cpp"), harness_sha256=sha256(source),
                harness_binary_sha256=sha256(harness), runner_sha256=sha256(Path(__file__)), manifest_sha256=sha256(MANIFEST), build_command=build)
    rows = []
    for piece in PIECES:
        for kmax in KMAX:
            info = manifest["pieces"][piece]
            path = Path(info["path"]).resolve()
            require(sha256(path) == info["sha256"] and path.stat().st_size == 6 * info["n"], f"morceau altéré : {piece}")
            n = info["n"]
            probe_cmd = [str(probe_binary), str(path), str(n), str(kmax), str(SEPARATION), "6", str(BACKEND), str(WORKERS), "samples", "records", *MODES]
            started = time.time()
            probe_json = args.scratch / f"probe_{piece}_k{kmax}.json"
            with probe_json.open("wb") as sink:
                completed = subprocess.run(probe_cmd, stdout=sink, stderr=subprocess.PIPE)
            require(completed.returncode == 0, f"sonde en échec : {completed.stderr[:300]!r}")
            probe_seconds = time.time() - started
            # La sortie de la sonde peut peser plusieurs Go (records de la scène) : en-tête lu seul, records lus en flux.
            probe, n_q4, n_q3 = split_probe_output(probe_json, args.scratch / f"records_{piece}_k{kmax}.txt")
            rec_file = args.scratch / f"records_{piece}_k{kmax}.txt"
            probe_json.unlink()
            q4 = range(n_q4)
            records = range(n_q4 + n_q3)
            harness_cmd = [str(harness), str(path), str(kmax), str(SEPARATION), str(rec_file), str(SAMPLES), str(SEED)]
            started = time.time()
            completed = subprocess.run(harness_cmd, capture_output=True, text=True)
            require(completed.returncode == 0 and not completed.stderr, f"harnais en échec : {completed.stderr[:300]}")
            h = json.loads(completed.stdout)
            row = dict(piece=piece, n=n, kmax=kmax, piece_sha256=info["sha256"], probe_command=probe_cmd, probe=probe, probe_seconds=probe_seconds,
                       records_q4=len(q4), records_q3=len(records) - len(q4), records_sha256=sha256(rec_file),
                       harness_command=harness_cmd, harness=h, harness_seconds=time.time() - started)
            compare(row)
            rows.append(row)
            print(json.dumps(dict(piece=piece, n=n, kmax=kmax, records=len(q4), valid=h["records"]["valid"], kept=h["completeness"]["kept"], balls=h["completeness"]["balls"],
                                  missing=h["completeness"]["missing"], probe_s=round(probe_seconds, 1), harness_s=round(row["harness_seconds"], 1))), flush=True)
            partial = dict(schema=SCHEMA, status="partial", pins=pins, rows=rows, public_status="not_claimed", gcp_used=False)
            args.output.with_suffix(".partial.json").write_text(json.dumps(partial, sort_keys=True, indent=1) + "\n")
    receipt = dict(schema=SCHEMA, status="passed", phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="q4 records of run_wspd_q34_parallel (live atlas) on the seven spatial pieces of scan 0: independent validity of every record, completeness on sampled kept pairs",
                   pins=pins, pieces=PIECES, kmax=KMAX, separation=SEPARATION, workers=WORKERS, samples=SAMPLES, seed=SEED, modes=MODES,
                   rows=rows, finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", rows=len(rows), commit=head[:8], output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["status"] == "passed" and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    require(receipt["pins"]["harness_sha256"] == sha256(HERE / "q4_bilateral_probe.cpp") and receipt["pins"]["manifest_sha256"] == sha256(MANIFEST), "harnais ou manifeste différents du reçu")
    require(args.commit is None or receipt["pins"]["commit"].startswith(args.commit), "commit du reçu différent de celui demandé")
    manifest = json.loads(MANIFEST.read_text())
    require([r["piece"] for r in receipt["rows"]] == [p for p in PIECES for _ in KMAX], "grille incomplète")
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
    capture.add_argument("--commit", required=True)
    capture.add_argument("--output", type=Path, default=HERE / "Q4_BILATERAL_SPATIAL_K10.json")
    reader = sub_parsers.add_parser("read")
    reader.add_argument("--commit", default=None)
    reader.add_argument("--output", type=Path, default=HERE / "Q4_BILATERAL_SPATIAL_K10.json")
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
