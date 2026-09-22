#!/usr/bin/env python3
"""Campagne de base du régime prioritaire : moteur u16 q3/q4 sur les trois nuages LiDAR sans sol (phase 0 de la reprise).

Pour chaque scène sans sol (profil u16 2 cm, reçu `lidar_ground_u16_20260921`) : K5 avec 1 puis 8 workers, K10 avec
8 workers ; s = 8, masque 6, Local28, configuration mesurée (`rectangle-pair`, `boxes`, `affine`, `live`, bloc 64), mode
`digest`. Chaque exécution est chronométrée par GNU time (mur, CPU, RSS) et son JSON de sonde est conservé tel quel.
Le reçu épingle le commit, l'état du worktree, la sonde (sha256), les sources du pipeline et les entrées. Le lecteur
`read` exige, à scène et K égaux, des sorties (xor, somme, IDs de coquille) et des compteurs géométriques identiques entre
1 et 8 workers lorsqu'ils sont mesurés, des sorties non vides, et recalcule les grandeurs dérivées. Les temps sont ceux
d'un hôte partagé : ils orientent le développement, ils ne qualifient rien. Aucun assert : tient sous python3 -O.

v2 ferme commandes, fichiers bruts, références, sources/binaire observés et entrées avant/après ; cela ne remplace
pas l'attestation de compilation du runner de qualification. Les reçus v1 restent historiques, jamais LIVE.
Une sélection partielle sans paire ne revendique aucune identité W1/W8. La comparaison porte sur les digests
publiés, pas sur des listes complètes de supports ni sur un oracle géométrique indépendant.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import sys
import time

import run_p0_matrix as collector

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SCHEMA = "mhgp8_ground_baseline_v1"
SCHEMA_CLOSED = "mhgp8_ground_baseline_v2"
MANIFEST = ROOT / "morsehgp3D_v8/receipts/lidar_ground_u16_20260921/MANIFEST.json"
# Grille 1 mm (18 bits, profil `quantized_u18_input_only`) : les payloads sans sol `full.u32le` des trois scènes,
# préparés le 21 septembre 2026 sur la trame brute entière (translation commune, aucune coupe ni clip).
GRID_1MM = ROOT / "morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6"
INPUT_GRIDS = {"2cm": dict(profile="quantized_u16_input_only", suffix=".u16le", description="grille 2 cm, u16, origine 32768 (lidar_ground_u16_20260921)"),
               "1mm": dict(profile="quantized_u18_input_only", suffix=".u32le", description="grille 1 mm, 18 bits, translation commune par scène (lidar_ground_20260921, scene_0X_grid)")}
OUTPUT = ROOT / "morsehgp3D_v8/receipts/ground_baseline_20260921"
PIPELINE_SOURCES = ("src/pipeline/wspd_q34.cpp", "src/pipeline/wspd_q34.hpp", "src/wspd/front.cpp", "src/lanes/q34_witness_search.cpp",
                    "src/lanes/q3_ball_census.cpp", "src/lanes/q4_local.cpp", "src/lanes/q4_local_partition.cpp", "src/lanes/edge_cover.cpp",
                    "bench/wspd_q34_probe.cpp")
GRID = tuple((scene, kmax, workers) for scene in ("00", "01", "02") for kmax, workers in ((5, 1), (5, 8), (10, 8)))
SEPARATION, MASK, BACKEND = 8, 6, 28
MODES = ("samples", "digest", "rectangle-pair", "boxes", "affine", "live", "64")
# Compteurs qui dépendent de l'attribution des jobs (capacités, pics) : exclus de l'identité W1/W8.
NONLOGICAL = re.compile(r"peak|capacity|retained_bytes|storage_bytes|_bytes$|jobs|workers|started|target_jobs|completed|terminal|prefix_product")
# v1 remains readable verbatim. v2 no longer accidentally drops geometric
# terminal_* counts; only capacities/bytes are excluded from front/work.
NONLOGICAL_V2 = re.compile(r"peak|capacity|retained_bytes|storage_bytes|_bytes$")


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git(*options):
    return subprocess.check_output(["git", *options], cwd=ROOT, text=True).strip()


def logical(tree, prefix="", *, version=1):
    """Aplatit les compteurs entiers d'un sous-arbre JSON en excluant les capacités et l'ordonnancement."""
    out = {}
    if isinstance(tree, dict):
        for key, value in tree.items():
            name = f"{prefix}.{key}" if prefix else key
            if (NONLOGICAL if version == 1 else NONLOGICAL_V2).search(key):
                continue
            out.update(logical(value, name, version=version))
    elif isinstance(tree, list):
        for i, value in enumerate(tree):
            out.update(logical(value, f"{prefix}[{i}]", version=version))
    elif isinstance(tree, bool):
        pass
    elif isinstance(tree, int):
        out[prefix] = tree
    return out


def load(path):
    return collector.parse_result(Path(path).read_bytes())


def write(path, value):
    # Callers first reserve a fresh output directory; never replace an old run.
    Path(path).write_text(json.dumps(value, sort_keys=True, indent=1, allow_nan=False) + "\n")


def digest_logical(probe, version):
    value = logical({k: probe[k] for k in ("front", "work")}, version=version)
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def uint(value, name):
    require(type(value) is int and 0 <= value < (1 << 64), f"{name}: entier u64 requis")
    return value


def integers(tree, name):
    if type(tree) is dict:
        for key, value in tree.items():
            integers(value, f"{name}.{key}")
    elif type(tree) is list:
        for index, value in enumerate(tree):
            integers(value, f"{name}[{index}]")
    else:
        uint(tree, name)


def named_file(directory, name):
    require(type(name) is str and Path(name).name == name and name not in ("", ".", ".."), "nom de fichier non local")
    path = directory / name
    require(not path.is_symlink() and path.is_file(), f"fichier absent ou lien interdit : {path}")
    return path


def input_fingerprint(entry, grid):
    path = ROOT / entry["path"]
    data = path.read_bytes()
    width = 2 if grid == "2cm" else 4
    n = uint(entry["n"], "input.n")
    require(n >= 2 and len(data) == n * width * 3, "taille physique de l'entrée différente de n")
    require(hashlib.sha256(data).hexdigest() == entry["sha256"], f"entrée altérée : {path}")
    result = 14695981039346656037
    maximum = 0
    for value in (n,):
        for byte in value.to_bytes(8, "little"):
            result = ((result ^ byte) * 1099511628211) & ((1 << 64) - 1)
    for (value,) in struct.iter_unpack("<H" if width == 2 else "<I", data):
        require(value < (1 << 18), "coordonnée hors 18 bits")
        maximum = max(maximum, value)
        for byte in value.to_bytes(8, "little"):
            result = ((result ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return result, "quantized_u16_input_only" if maximum <= 65535 else "quantized_u18_input_only"


def source_paths():
    # All tracked native sources (including textual probe/test inclusions), not
    # just nine pipeline files. Untracked float32 prototypes are not compiled
    # by this target and are deliberately not promoted into this inventory.
    names = git("ls-files", "morsehgp3D_v8/src", "morsehgp3D_v8/bench", "morsehgp3D_v8/tests").splitlines()
    result = {str(ROOT / n) for n in names if Path(n).suffix in (".cpp", ".hpp", ".h")}
    result.update(str(p) for p in (Path(__file__).resolve(), HERE / "run_p0_matrix.py", HERE.parent / "CMakeLists.txt"))
    return sorted(result)


def hashes(paths):
    return {str(p): sha256(p) for p in paths}


def verify_hashes(expected, label):
    actual = hashes(expected)
    require(actual == expected, f"empreintes modifiées : {label}")
    return actual


def validate_probe(probe, row, receipt):
    """Command/input/output and accounting gate, not a geometric oracle."""
    entry = receipt["pins"]["inputs"][row["scene"]]
    expected = [receipt["pins"]["probe"], str(ROOT / entry["path"]), str(entry["n"]), str(row["kmax"]),
                str(receipt["separation"]), str(receipt["mask"]), str(receipt["q4_backend"]), str(row["workers"]), *receipt["modes"]]
    require(row["command"] == expected, "commande différente de la ligne/du plan/de l'entrée")
    require(probe["schema"] in ("mhgp8_wspd_q34_probe_v4", "mhgp8_wspd_q34_probe_v5"), "schéma de sonde non pris en charge")
    fixed = dict(status="completed", scope="global_q3_q4_candidate_stream_not_catalogue_or_full", backend="cpu_reference",
                 public_status="not_claimed", profile=receipt["profile"], n=entry["n"], source_n=entry["n"], kmax=row["kmax"],
                 s=receipt["separation"], mask=receipt["mask"], q4_backend=receipt["q4_backend"], workers=row["workers"],
                 front_mode="samples", output_mode="digest", witness_mode="rectangle-pair", q3_census_mode="boxes",
                 witness_bounds_mode="affine", q4_seed_mode="live", q4_seed_block_size=64)
    for key, value in fixed.items():
        require(type(probe.get(key)) is type(value) and probe[key] == value, f"champ sonde différent de la commande : {key}")
    require(row["n"] == entry["n"] and row["frame"] == entry["frame"], "identité de scène différente de l'entrée")
    if probe["schema"].endswith("v5"):
        require(receipt["modes"][7:] in (["atlas"], ["no-atlas"]), "mode atlas manquant ou inconnu")
        require(probe["q3_atlas_mode"] == receipt["modes"][7], "mode atlas différent de la commande")
    else:
        require(len(receipt["modes"]) == 7, "arguments superflus au schéma v4")
    for key in ("front", "work", "parallel", "workers_work", "cloud_work", "index_work", "memory"):
        integers(probe[key], key)
    output = probe["output"]
    require(set(output) == {"callbacks", "q3", "q4", "support_ids", "shell_ids", "xor", "sum"}, "champs du digest inconnus")
    for key in ("callbacks", "q3", "q4", "support_ids", "shell_ids"):
        uint(output[key], f"output.{key}")
    for key in ("xor", "sum"):
        require(type(output[key]) is str and re.fullmatch("[0-9a-f]{1,16}", output[key]) is not None, "digest invalide")
    require(output["callbacks"] == output["q3"] + output["q4"] and output["support_ids"] == 3 * output["q3"] + 4 * output["q4"], "registre des supports incohérent")
    require(output["shell_ids"] >= output["support_ids"], "coquilles incomplètes dans le registre")
    require(output["q3"] == probe["work"]["q3_emitted"] and output["q4"] == probe["work"]["q4_emitted"], "émissions/digest différents")
    for key, value in probe["timings_ms"].items():
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0, f"chrono invalide : {key}")
    if receipt["schema"] == SCHEMA_CLOSED:
        uint(probe["input_hash"], "input_hash")
        require(probe["input_hash"] == entry["fnv1a64_u64le_n_xyz"], "empreinte numérique de l'entrée différente")
    require(probe["output"] == row["output"] and probe["timings_ms"] == row["timings_ms"] and
            probe["work"]["q3_emitted"] == row["q3_emitted"] and probe["work"]["q4_emitted"] == row["q4_emitted"], "ligne différente de son JSON")


def gnu_time_fields(text):
    fields = {}
    for pattern, key, convert in (
            (r"Elapsed \(wall clock\) time .*: (.+)", "wall", lambda s: sum(float(x) * 60 ** i for i, x in enumerate(reversed(s.split(":"))))),
            (r"User time \(seconds\): ([\d.]+)", "user_s", float), (r"System time \(seconds\): ([\d.]+)", "system_s", float),
            (r"Percent of CPU this job got: (\d+)%", "cpu_percent", int), (r"Maximum resident set size \(kbytes\): (\d+)", "max_rss_kb", int)):
        matches = re.findall(pattern, text)
        require(len(matches) == 1, f"champ GNU time absent ou répété : {key}")
        fields[key] = convert(matches[0])
        require(math.isfinite(fields[key]) and fields[key] >= 0, f"champ GNU time invalide : {key}")
    require(re.findall(r"Exit status: (\d+)", text) == ["0"], "GNU time publie un échec ou aucun statut")
    fields["wall_s"] = round(fields.pop("wall"), 3)
    return fields


def inputs_2cm():
    manifest = load(MANIFEST)
    require(manifest["schema"] == "mhgp8_lidar_ground_u16_preparation_v1", "manifeste u16 sans sol inattendu")
    inputs = {}
    for record in manifest["scenes"]:
        path = ROOT / record["pieces"]["full"]["path"]
        require(path.is_file() and sha256(path) == record["pieces"]["full"]["sha256"], f"entrée absente ou altérée : {path} (régénérer par prepare_lidar_ground_u16.py run)")
        inputs[record["scene"]] = dict(path=str(path.relative_to(ROOT)), n=record["pieces"]["full"]["n"], sha256=record["pieces"]["full"]["sha256"], frame=record["frame"])
    return inputs, sha256(MANIFEST)


def inputs_1mm():
    """Les trois payloads sans sol à 1 mm (u32le, 18 bits) et leurs manifestes versionnés."""
    inputs, manifests = {}, []
    for scene in ("00", "01", "02"):
        manifest_path = GRID_1MM / f"scene_{scene}_grid/MANIFEST.json"
        manifest = load(manifest_path)
        require(manifest["schema"] == "mhgp8_lidar_ground_preparation_v1" and manifest["profile"] == "quantized_u32_fixed_grid_input_only", f"manifeste 1 mm inattendu : {manifest_path}")
        preparation = manifest["raw_preparation"]
        require(preparation["quantization"]["step_metres"] == dict(numerator=1, denominator=1000) and preparation["representation"]["suffix"] == ".u32le"
                and preparation["representation"]["point_bytes"] == 12, "préparation 1 mm hors convention (pas 1/1000 m, u32le, 12 octets)")
        require(max(preparation["coordinate_bounds"]["encoded_max"]) < (1 << 18) and min(preparation["coordinate_bounds"]["encoded_min"]) >= 0, "coordonnées hors 18 bits")
        full = manifest["datasets"]["full"]
        path = manifest_path.parent / full["points_file"]
        require(path.is_file() and sha256(path) == full["points_sha256"] and path.stat().st_size == 12 * full["sites"], f"payload 1 mm absent ou altéré : {path}")
        frame = Path(manifest["input_paths"][0]).stem
        inputs[scene] = dict(path=str(path.relative_to(ROOT)), n=full["sites"], sha256=full["points_sha256"], frame=frame, encoded_max=preparation["coordinate_bounds"]["encoded_max"],
                             translation=preparation["translation"]["vector"], manifest=str(manifest_path.relative_to(ROOT)), manifest_sha256=sha256(manifest_path))
        manifests.append(sha256(manifest_path))
    return inputs, manifests


def run(args):
    probe = args.probe.resolve()
    require(probe.is_file(), f"sonde absente : {probe}")
    require(args.extra in ([], ["atlas"], ["no-atlas"]), "seul le mode atlas optionnel est admis")
    require(not args.only or (len(set(args.only)) == len(args.only) and all(case in GRID for case in args.only)), "sélection --only inconnue ou répétée")
    args.output = args.output.resolve()
    require(not args.output.exists(), f"répertoire déjà présent, jamais recouvert : {args.output}")
    inputs, manifest_sha = inputs_2cm() if args.grid == "2cm" else inputs_1mm()
    for entry in inputs.values():
        entry["fnv1a64_u64le_n_xyz"], profile = input_fingerprint(entry, args.grid)
        require(profile == INPUT_GRIDS[args.grid]["profile"], "profil numérique réel différent du profil attendu")
    reference, reference_hashes = build_reference(args.reference, args.grid, inputs, version=2)
    sources = hashes(source_paths())
    artifacts = hashes([probe, *[p for p in (probe.parent / "CMakeCache.txt", probe.parent / "libmhgp8_p0.a") if p.is_file()]])
    input_paths = [ROOT / entry["path"] for entry in inputs.values()]
    input_paths += [MANIFEST] if args.grid == "2cm" else [ROOT / entry["manifest"] for entry in inputs.values()]
    input_pins = hashes(input_paths)
    pins = dict(git_commit=git("rev-parse", "HEAD"), worktree_status=git("status", "--porcelain", "--", "morsehgp3D_v8/src", "morsehgp3D_v8/bench", "morsehgp3D_v8/tests", "morsehgp3D_v8/CMakeLists.txt"),
                probe=str(probe), probe_sha256=sha256(probe), runner_sha256=sha256(Path(__file__)), manifest_sha256=manifest_sha,
                pipeline_sources_sha256={s: sha256(ROOT / "morsehgp3D_v8" / s) for s in PIPELINE_SOURCES}, host=subprocess.check_output(["uname", "-a"], text=True).strip(),
                cpus=subprocess.check_output(["nproc"], text=True).strip(), inputs=inputs,
                source_sha256=sources, artifact_sha256=artifacts, input_sha256=input_pins, reference_sha256=reference_hashes,
                source_coupling="observed_source_and_artifact_identity_not_build_attestation",
                affinity=sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
                cpu_max=Path("/sys/fs/cgroup/cpu.max").read_text().strip() if Path("/sys/fs/cgroup/cpu.max").is_file() else None,
                python_optimized=sys.flags.optimize, launch_command=[sys.executable, *sys.orig_argv[1:]])
    args.output.mkdir(parents=True)
    prefix = "only_" if args.only else ""
    receipt_name = "BASELINE.only.json" if args.only else "BASELINE.json"
    rows = []
    receipt = dict(schema=SCHEMA_CLOSED, status="running", phase="exploration_v8_hors_registre", backend="cpu_reference", profile=INPUT_GRIDS[args.grid]["profile"],
                   mode="developpement_phase0_base", public_status="not_claimed", gcp_used=False, input_grid=args.grid, input_grid_description=INPUT_GRIDS[args.grid]["description"], reference=reference,
                   scope="integer q3/q4 stream; digest/accounting identity only, no geometric oracle, catalogue, FULL or GPU; shared-host exploratory timings",
                   pins=pins, grid=[dict(scene=s, kmax=k, workers=w) for s, k, w in GRID], separation=SEPARATION, mask=MASK, q4_backend=BACKEND, modes=[*MODES, *args.extra], rows=rows,
                   logical_version=2, selected=[dict(index=i, scene=s, kmax=k, workers=w) for i, (s, k, w) in enumerate(GRID) if not args.only or (s, k, w) in args.only])
    write(args.output / "MANIFEST.json", {k: v for k, v in receipt.items() if k != "rows"})
    manifest_pin = sha256(args.output / "MANIFEST.json")
    records = []
    error = None
    handlers = {sig: signal.getsignal(sig) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        for sig in handlers:
            signal.signal(sig, collector.on_signal)
        for selected in receipt["selected"]:
            index, scene, kmax, workers = (selected[k] for k in ("index", "scene", "kmax", "workers"))
            entry = inputs[scene]
            command = [str(probe), str(ROOT / entry["path"]), str(entry["n"]), str(kmax), str(SEPARATION), str(MASK), str(BACKEND), str(workers), *MODES, *args.extra]
            time_file = args.output / f"{prefix}time_{index:02}_s{scene}_k{kmax}_w{workers}.txt"
            json_file = args.output / f"{prefix}probe_{index:02}_s{scene}_k{kmax}_w{workers}.json"
            record_file = args.output / f"{prefix}record_{index:02}.json"
            native = ["/usr/bin/time", "-v", "-o", str(time_file), *command]
            record = dict(command=native, exit_code=None, status="failed", stdout="", stderr="", stdout_base64="", stderr_base64="")
            started = time.time()
            load_before = Path("/proc/loadavg").read_text().split()[:3]
            try:
                require(sha256(probe) == pins["probe_sha256"], "binaire modifié avant appel")
                collector.invoke(native, dict(os.environ, LC_ALL="C"), ROOT, record, new_session=True)
                require(record["exit_code"] == 0, f"sonde en échec : code {record['exit_code']}")
                require(not record["stderr_base64"], "stderr de sonde non vide")
                json_file.write_bytes(base64.b64decode(record["stdout_base64"], validate=True))
                probe_json = load(json_file)
                timing = gnu_time_fields(time_file.read_text())
                row = dict(**selected, frame=entry["frame"], n=entry["n"], command=command,
                           started_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(started)),
                           finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                           load_before=load_before, gnu_time=timing, probe_json=json_file.name, time_file=time_file.name,
                           probe_json_sha256=sha256(json_file), time_sha256=sha256(time_file), record=record_file.name,
                           output=probe_json["output"], timings_ms=probe_json["timings_ms"], q3_emitted=probe_json["work"]["q3_emitted"], q4_emitted=probe_json["work"]["q4_emitted"],
                           logical_sha256=digest_logical(probe_json, 2))
                validate_probe(probe_json, row, receipt)
                require(sha256(probe) == pins["probe_sha256"], "binaire modifié pendant appel")
                rows.append(row)
                record["status"] = "passed"
                print(json.dumps(dict(scene=scene, kmax=kmax, workers=workers, wall_s=timing["wall_s"], cpu_s=round(timing["user_s"] + timing["system_s"], 3))), flush=True)
            finally:
                # The collector drains/reaps the entire owned process group on
                # SIGINT/SIGTERM; even failed stdout/stderr stay in this record.
                write(record_file, record)
                records.append(dict(path=record_file.name, sha256=sha256(record_file)))
                write(args.output / "BASELINE.partial.json", receipt)
        receipt["status"] = "partial" if args.only else "passed"
        check(receipt)
    except BaseException as failure:
        error = f"{type(failure).__name__}: {failure}"
        receipt["status"] = "failed"
    finally:
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    receipt["finished_utc"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    closing_errors, closing_hashes = [], {}
    for label, expected in (("sources", sources), ("artifacts", artifacts), ("inputs", input_pins), ("references", reference_hashes)):
        actual = {}
        for filename in expected:
            try:
                actual[filename] = sha256(filename)
            except OSError as failure:
                actual[filename] = None
                closing_errors.append(f"{label}: {failure}")
        closing_hashes[label] = actual
        if actual != expected:
            closing_errors.append(f"empreintes modifiées : {label}")
    if sha256(args.output / "MANIFEST.json") != manifest_pin:
        closing_errors.append("manifeste modifié pendant la capture")
    payload_pins = {r["path"]: r["sha256"] for r in records}
    payload_pins.update({r["probe_json"]: r["probe_json_sha256"] for r in rows})
    payload_pins.update({r["time_file"]: r["time_sha256"] for r in rows})
    for name, expected in payload_pins.items():
        if not (args.output / name).is_file() or sha256(args.output / name) != expected:
            closing_errors.append(f"artefact de mesure modifié : {name}")
    if closing_errors:
        receipt["status"] = "failed"
    write(args.output / receipt_name, receipt)
    write(args.output / "COMPLETION.json", dict(status="failed" if error or closing_errors else "passed", error=error,
          closing_errors=closing_errors, manifest_sha256=manifest_pin, receipt=receipt_name, receipt_sha256=sha256(args.output / receipt_name),
          artifact_sha256=payload_pins, records=records, closing_sha256=closing_hashes, source_sha256_after=closing_hashes["sources"],
          probe_sha256_after=sha256(probe) if probe.is_file() else None))
    require(error is None and not closing_errors, f"capture conservée en échec : {error}; {closing_errors}")
    print(json.dumps(dict(status=receipt["status"], rows=len(rows), output=str(args.output))))


def check(receipt):
    rows = receipt["rows"]
    require(type(rows) is list and rows, "aucune mesure")
    seen = set()
    by_key = {}
    for row in rows:
        for key in ("index", "n", "kmax", "workers", "q3_emitted", "q4_emitted"):
            uint(row[key], key)
        key = (row["scene"], row["kmax"], row["workers"])
        require(key in GRID and row["index"] < len(GRID) and GRID[row["index"]] == key and key not in seen, "ligne répétée ou hors plan")
        seen.add(key)
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
    # Identité contre un reçu de référence (autre sonde, autre commit, mêmes entrées) : à scène et K égaux, sorties et
    # compteurs géométriques identiques quel que soit le nombre de workers. C'est la porte de régression bit à bit.
    reference_pairs = 0
    if receipt.get("reference"):
        for row in rows:
            matches = [r for r in receipt["reference"]["rows"] if r["scene"] == row["scene"] and r["kmax"] == row["kmax"]]
            require(matches, f"aucune ligne de référence pour la scène {row['scene']} K{row['kmax']}")
            for match in matches:
                require(match["output"] == row["output"], f"sorties différentes de la référence : scène {row['scene']} K{row['kmax']} (W{match['workers']} de référence)")
                require(match["logical_sha256"] == row["logical_sha256"], f"compteurs géométriques différents de la référence : scène {row['scene']} K{row['kmax']}")
                reference_pairs += 1
        require(reference_pairs >= 1, "porte d'identité contre la référence vide")
    if receipt["status"] == "passed":
        require(identity_pairs >= 3, "porte d'identité W1/W8 vide")
        require([(r["scene"], r["kmax"], r["workers"]) for r in rows] == [tuple(g) for g in ((s, k, w) for s, k, w in GRID)], "grille incomplète")
    elif receipt["schema"] == SCHEMA:
        require(identity_pairs >= 1 or reference_pairs >= 1, "porte d'identité vide (reçu partiel sans paire W1/W8 ni référence)")
    if receipt["schema"] == SCHEMA_CLOSED:
        expected = [dict(index=r["index"], scene=r["scene"], kmax=r["kmax"], workers=r["workers"]) for r in rows]
        require(receipt["selected"] == expected, "plan sélectionné incomplet ou réordonné")
    return dict(worker_identity_pairs=identity_pairs, reference_identity_pairs=reference_pairs)


def path_label(path):
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def build_reference(specs, grid, inputs, *, version, ancestry=None):
    if not specs:
        return None, {}
    reference = dict(sources=[], rows=[])
    pins = {}
    seen = set()
    for spec in specs:
        directory, separator, variant = spec.rpartition(":")
        if not separator:
            directory, variant = spec, "full"
        require(variant in ("full", "only"), f"variante de référence inconnue : {spec}")
        path = (ROOT / directory).resolve() / ("BASELINE.only.json" if variant == "only" else "BASELINE.json")
        require(path not in seen, "référence répétée")
        seen.add(path)
        original, original_pins = read_receipt(path.parent, variant, ancestry=ancestry)
        require(original.get("input_grid", "2cm") == grid, "référence d'une autre grille")
        pins.update(original_pins)
        label = path_label(path)
        reference["sources"].append(dict(receipt=label, sha256=sha256(path), commit=original["pins"]["git_commit"], probe_sha256=original["pins"]["probe_sha256"]))
        for row in original["rows"]:
            actual, expected = original["pins"]["inputs"][row["scene"]], inputs[row["scene"]]
            require(all(actual[key] == expected[key] for key in ("n", "sha256", "frame")), "référence de scène contenant d'autres points/IDs")
            # Recompute v2 counters from the original raw result, never rename
            # an old digest which excluded terminal_* into the new authority.
            digest = row["logical_sha256"] if version == 1 else digest_logical(load(path.parent / row["probe_json"]), 2)
            reference["rows"].append(dict(scene=row["scene"], kmax=row["kmax"], workers=row["workers"], output=row["output"], logical_sha256=digest, receipt=label))
    return reference, pins


def read_receipt(directory, variant="full", *, check_live=False, ancestry=None):
    directory = Path(directory).resolve()
    path = named_file(directory, "BASELINE.only.json" if variant == "only" else "BASELINE.json")
    ancestry = set() if ancestry is None else set(ancestry)
    require(path not in ancestry, "cycle de références")
    ancestry.add(path)
    inspected = {str(path): sha256(path)}
    receipt = load(path)
    require(receipt["schema"] in (SCHEMA, SCHEMA_CLOSED) and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    closed = receipt["schema"] == SCHEMA_CLOSED
    version = 2 if closed else 1
    grid = receipt.get("input_grid", "2cm")
    require(grid in INPUT_GRIDS and receipt["profile"] == INPUT_GRIDS[grid]["profile"], "grille d'entrée ou profil du reçu inconnus")
    require(receipt["status"] in ("passed", "partial"), "statut de reçu inconnu")
    require(receipt["separation"] == SEPARATION and receipt["mask"] == MASK and receipt["q4_backend"] == BACKEND and receipt["modes"][:7] == list(MODES), "configuration hors contrat de ce runner")
    require(receipt["grid"] == [dict(scene=s, kmax=k, workers=w) for s, k, w in GRID], "grille du reçu altérée")
    if closed:
        require(receipt["logical_version"] == 2, "version du registre inconnue")
        manifest_path = named_file(directory, "MANIFEST.json")
        completion_path = named_file(directory, "COMPLETION.json")
        completion, manifest = load(completion_path), load(manifest_path)
        inspected.update(hashes([manifest_path, completion_path]))
        require(completion["status"] == "passed" and completion["error"] is None and completion["closing_errors"] == [], "capture non close en succès")
        require(completion["manifest_sha256"] == sha256(manifest_path) and completion["receipt"] == path.name and completion["receipt_sha256"] == sha256(path), "fermeture du manifeste/reçu altérée")
        expected_manifest = {k: v for k, v in receipt.items() if k not in ("rows", "finished_utc")}
        expected_manifest["status"] = "running"
        require(manifest == expected_manifest, "manifeste initial différent du reçu")
        require(completion["source_sha256_after"] == receipt["pins"]["source_sha256"] and completion["probe_sha256_after"] == receipt["pins"]["probe_sha256"], "sources/binaire modifiés pendant la capture")
        require(completion["closing_sha256"] == {name: receipt["pins"][field] for name, field in
                (("sources", "source_sha256"), ("artifacts", "artifact_sha256"), ("inputs", "input_sha256"), ("references", "reference_sha256"))}, "fermeture des entrées/sources/références/artefacts différente")
        for name, expected in completion["artifact_sha256"].items():
            artifact = named_file(directory, name)
            require(sha256(artifact) == expected, f"artefact de mesure altéré : {name}")
            inspected[str(artifact)] = expected
    previous_finish = None
    for row in receipt["rows"]:
        json_file = named_file(directory, row["probe_json"])
        time_file = named_file(directory, row["time_file"])
        inspected.update(hashes([json_file, time_file]))
        require(sha256(json_file) == row["probe_json_sha256"], f"JSON de sonde altéré : {json_file.name}")
        probe_json = load(json_file)
        validate_probe(probe_json, row, receipt)
        require(probe_json["profile"] == receipt["profile"] and Path(row["command"][1]).suffix == INPUT_GRIDS[grid]["suffix"], "profil ou conteneur d'entrée hors grille déclarée")
        require(digest_logical(probe_json, version) == row["logical_sha256"], "empreinte logique non reproduite")
        require(gnu_time_fields(time_file.read_text()) == row["gnu_time"], "GNU time non reproduit")
        if closed:
            require(sha256(time_file) == row["time_sha256"], "GNU time altéré")
            start = time.strptime(row["started_utc"], "%Y-%m-%dT%H:%M:%SZ")
            finish = time.strptime(row["finished_utc"], "%Y-%m-%dT%H:%M:%SZ")
            require(start <= finish and (previous_finish is None or previous_finish <= start), "chronologie des commandes incohérente")
            require(finish <= time.strptime(receipt["finished_utc"], "%Y-%m-%dT%H:%M:%SZ"), "fin de campagne antérieure à une mesure")
            previous_finish = finish
            record_file = named_file(directory, row["record"])
            record = load(record_file)
            require(record["command"] == ["/usr/bin/time", "-v", "-o", str(time_file), *row["command"]], "commande collectée différente")
            require(record["status"] == "passed" and type(record["exit_code"]) is int and record["exit_code"] == 0 and
                    record["stderr"] == record["stderr_base64"] == "", "processus collecté en échec ou stderr non vide")
            require(base64.b64decode(record["stdout_base64"], validate=True) == json_file.read_bytes() and record["stdout"] == json_file.read_text(), "stdout collecté différent du JSON brut")
            inspected[str(record_file)] = sha256(record_file)
    if closed:
        expected_artifacts = {}
        for row in receipt["rows"]:
            expected_artifacts.update({name: inspected[str(directory / name)] for name in (row["record"], row["probe_json"], row["time_file"])})
        require(completion["artifact_sha256"] == expected_artifacts and completion["records"] == [dict(path=r["record"], sha256=expected_artifacts[r["record"]]) for r in receipt["rows"]], "inventaire de clôture incomplet ou supplémentaire")
    if receipt.get("reference"):
        specs = []
        for source in receipt["reference"]["sources"]:
            reference_path = ROOT / source["receipt"]
            require(reference_path.name in ("BASELINE.json", "BASELINE.only.json") and sha256(reference_path) == source["sha256"], "reçu de référence altéré")
            specs.append(str(reference_path.parent) + (":only" if reference_path.name == "BASELINE.only.json" else ":full"))
        reference, pins = build_reference(specs, grid, receipt["pins"]["inputs"], version=version, ancestry=ancestry)
        require(reference == receipt["reference"], "référence embarquée différente des preuves d'origine")
        inspected.update(pins)
        if closed:
            require(pins == receipt["pins"]["reference_sha256"], "inventaire des références différent")
    elif closed:
        require(receipt["pins"]["reference_sha256"] == {}, "références non déclarées")
    if check_live:
        require(closed, "v1 historique : pas de clôture LIVE revendiquée")
        for key in ("source_sha256", "artifact_sha256", "input_sha256"):
            verify_hashes(receipt["pins"][key], key)
            inspected.update(receipt["pins"][key])
        for entry in receipt["pins"]["inputs"].values():
            fingerprint, profile = input_fingerprint(entry, grid)
            require(fingerprint == entry["fnv1a64_u64le_n_xyz"] and profile == receipt["profile"], "entrée numérique LIVE différente")
    check(receipt)
    verify_hashes(inspected, "fermeture des fichiers lus")
    return receipt, inspected


def read(args):
    receipt, inspected = read_receipt(args.output, args.variant, check_live=args.check_live)
    print(json.dumps(dict(status="passed", campaign_status=receipt["status"], rows=len(receipt["rows"]),
                         authority="closed_v2" if receipt["schema"] == SCHEMA_CLOSED else "historical_v1_no_source_closure",
                         live_checked=args.check_live, files_checked=len(inspected), commit=receipt["pins"]["git_commit"][:8], **check(receipt))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub_parsers = parser.add_subparsers(dest="operation", required=True)
    capture = sub_parsers.add_parser("run")
    capture.add_argument("--probe", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=OUTPUT)
    capture.add_argument("--only", type=lambda s: tuple(x.strip() for x in s.split(",")), nargs="*", default=None, help="triplets scene,K,W à exécuter seulement")
    capture.add_argument("--extra", nargs="*", default=[], help="jetons CLI ajoutés après les modes (ex. atlas)")
    capture.add_argument("--grid", choices=tuple(INPUT_GRIDS), default="2cm", help="entrées : grille 2 cm u16 (défaut) ou grille 1 mm 18 bits (u32le)")
    capture.add_argument("--reference", nargs="*", default=[], help="reçus de référence `répertoire[:full|only]` : identité des sorties et des compteurs à scène et K égaux")
    reader = sub_parsers.add_parser("read")
    reader.add_argument("--output", type=Path, default=OUTPUT)
    reader.add_argument("--variant", choices=("full", "only"), default="full")
    reader.add_argument("--check-live", action="store_true", help="v2 seulement : rehash sources, binaire, entrées et manifestes actuels")
    args = parser.parse_args()
    if getattr(args, "only", None):
        args.only = [(s, int(k), int(w)) for s, k, w in args.only]
    try:
        run(args) if args.operation == "run" else read(args)
    except (Failure, collector.InvalidReceipt, OSError, ValueError, KeyError, IndexError, TypeError) as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
