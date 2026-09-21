#!/usr/bin/env python3
"""Nuages LiDAR sans sol au profil u16 (2 cm) du moteur historique, depuis le brut et le masque Patchwork++ épinglé.

Le moteur global existant (`run_wspd_q34_parallel`, profil `quantized_u16_input_only`) ne lit que des sites u16 distincts
(6 octets par site). Les préparations sans sol qualifiées (`receipts/lidar_ground_20260921`) n'existent qu'en float32 et
grille 1 mm : ce préparateur applique le MÊME masque par retour à la MÊME quantification que la préparation spatiale u16
(q = floor(50·x + 32768 + 1/2) en rationnels exacts depuis le float32 décodé, déduplication globale, plans qx = 32768 puis
qy = 32768, site sur le plan du côté >=), avec la règle du pilote : un site u16 est conservé si AU MOINS UN de ses retours
n'est pas étiqueté sol (statuts 0 indécis et 2 non-sol conservés, 1 sol retiré). Les sites à décisions mixtes sont comptés.

`run` écrit sept fichiers `.u16le` par scène (non versionnés, régénérables) et un manifeste versionnable (sha256, effectifs,
conventions) ; `read` régénère tout en mémoire depuis le brut et le masque, puis exige l'égalité des sha256 et des
effectifs du manifeste. Aucun assert : tient sous python3 -O. Diagnostic de calcul seulement : ni qualité du masque, ni
mesure HGP, ni statut public.
"""
from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
ORIGIN, SCALE = 32768, 50
RAW = struct.Struct("<ffff")
SITE = struct.Struct("<HHH")
SCHEMA = "mhgp8_lidar_ground_u16_preparation_v1"
GROUND_DIR = ROOT / "morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6"
OUT_DIR = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared/ground_u16"
MANIFEST = ROOT / "morsehgp3D_v8/receipts/lidar_ground_u16_20260921/MANIFEST.json"
SCENES = ("00", "01", "02")
PIECES = ("full", "half_x_neg", "half_x_nonneg", "quarter_x_neg_y_neg", "quarter_x_neg_y_nonneg", "quarter_x_nonneg_y_neg",
          "quarter_x_nonneg_y_nonneg")
GROUND, NONGROUND, UNKNOWN = 1, 2, 0


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def quantize(value):
    require(math.isfinite(value), "valeur non finie")
    q = math.floor(Fraction(SCALE) * Fraction(value) + Fraction(2 * ORIGIN + 1, 2))
    require(0 <= q <= 65535, f"hors grille : {value}")
    return q


def piece_of(site):
    x_neg, y_neg = site[0] < ORIGIN, site[1] < ORIGIN
    half = "half_x_neg" if x_neg else "half_x_nonneg"
    quarter = f"quarter_x_{'neg' if x_neg else 'nonneg'}_y_{'neg' if y_neg else 'nonneg'}"
    return half, quarter


def prepare_scene(scene, ground_dir):
    profile_dir = ground_dir / f"scene_{scene}_float32"
    manifest = json.loads((profile_dir / "MANIFEST.json").read_text())
    require(manifest.get("schema") == "mhgp8_lidar_ground_preparation_v1" and manifest.get("status") == "prepared", "manifeste sans sol inattendu")
    raw_path = Path(manifest["input_paths"][0])
    require(raw_path.is_file(), f"brut absent : {raw_path} (non versionné, voir audits/lidar08_20260914/README.md)")
    raw = raw_path.read_bytes()
    require(len(raw) % RAW.size == 0, "taille brute non multiple de 16 octets")
    returns = len(raw) // RAW.size
    mask = (profile_dir / "mask.u8").read_bytes()
    repeat = (ground_dir / f"scene_{scene}_repeat_0.u8").read_bytes()
    require(len(mask) == returns and mask == repeat, "masque de longueur inattendue ou différent de la capture repeat_0")
    require(set(mask) <= {GROUND, NONGROUND, UNKNOWN}, "statut de masque hors {0, 1, 2}")
    status_by_site = {}
    for index, (x, y, z, _r) in enumerate(RAW.iter_unpack(raw)):
        site = (quantize(x), quantize(y), quantize(z))
        entry = status_by_site.setdefault(site, [0, 0, 0])
        entry[mask[index]] += 1
    kept = sorted(site for site, counts in status_by_site.items() if counts[NONGROUND] + counts[UNKNOWN] > 0)
    mixed = sum(1 for counts in status_by_site.values() if counts[GROUND] > 0 and counts[NONGROUND] + counts[UNKNOWN] > 0)
    rows = {name: [] for name in PIECES}
    rows["full"] = kept
    for site in kept:
        half, quarter = piece_of(site)
        rows[half].append(site)
        rows[quarter].append(site)
    payloads = {name: b"".join(SITE.pack(*s) for s in rows[name]) for name in PIECES}
    require(len(rows["half_x_neg"]) + len(rows["half_x_nonneg"]) == len(kept), "moitiés non exhaustives")
    require(sum(len(rows[p]) for p in PIECES if p.startswith("quarter")) == len(kept), "quarts non exhaustifs")
    counts = dict(returns=returns, returns_ground=mask.count(GROUND), returns_nonground=mask.count(NONGROUND), returns_unknown=mask.count(UNKNOWN),
                  raw_sites=len(status_by_site), kept_sites=len(kept), removed_sites=len(status_by_site) - len(kept), mixed_sites=mixed,
                  merged_returns=returns - len(status_by_site))
    record = dict(scene=scene, frame=raw_path.name, raw=str(raw_path.relative_to(ROOT)), raw_sha256=sha256_bytes(raw),
                  mask=str((profile_dir / "mask.u8").relative_to(ROOT)), mask_sha256=sha256_bytes(mask), counts=counts,
                  pieces={name: dict(n=len(rows[name]), bytes=len(payloads[name]), sha256=sha256_bytes(payloads[name])) for name in PIECES})
    return record, payloads


def conventions():
    return dict(profile="quantized_u16_input_only", grid="2 cm, origine 32768 au capteur",
                quantization="floor(50*x + 32768 + 1/2), rationnels exacts depuis float32, refus hors [0,65535] ou non fini",
                deduplication="globale sur (qx,qy,qz) avant masque et découpe", order="lexicographique (x,y,z)",
                keep_rule="site conservé si au moins un de ses retours a le statut 0 (indécis) ou 2 (non-sol) ; 1 (sol) retiré",
                cut="qx = 32768 puis qy = 32768 ; site sur le plan du côté >= ; restrictions disjointes et exhaustives",
                scope="entrée de calcul pour le moteur u16 ; ni qualité sémantique du masque, ni mesure HGP")


def run(args):
    started = time.time()
    scenes = []
    for scene in args.scenes:
        record, payloads = prepare_scene(scene, args.ground_dir)
        out = args.out_dir / f"scene_{scene}"
        out.mkdir(parents=True, exist_ok=True)
        for name, payload in payloads.items():
            (out / f"{name}.u16le").write_bytes(payload)
            record["pieces"][name]["path"] = str((out / f"{name}.u16le").relative_to(ROOT))
        scenes.append(record)
        c = record["counts"]
        print(json.dumps(dict(scene=scene, frame=record["frame"], returns=c["returns"], raw_sites=c["raw_sites"], kept_sites=c["kept_sites"],
                              mixed_sites=c["mixed_sites"], pieces={n: p["n"] for n, p in record["pieces"].items()})), flush=True)
    manifest = dict(schema=SCHEMA, status="prepared", phase="exploration_v8_hors_registre", backend="cpu_reference",
                    profile="quantized_u16_input_only", public_status="not_claimed", gcp_used=False, conventions=conventions(),
                    ground_dir=str(args.ground_dir.relative_to(ROOT)), preparer_sha256=sha256_bytes(Path(__file__).read_bytes()),
                    scenes=scenes, seconds=round(time.time() - started, 3), finished_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(manifest, indent=1, sort_keys=True, ensure_ascii=False) + "\n")
    print(json.dumps(dict(status="prepared", scenes=len(scenes), manifest=str(args.manifest.relative_to(ROOT)))))


def read(args):
    manifest = json.loads(args.manifest.read_text())
    require(manifest["schema"] == SCHEMA and manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False, "manifeste hors cadre")
    require(manifest["conventions"] == conventions(), "conventions différentes du préparateur courant")
    for record in manifest["scenes"]:
        fresh, payloads = prepare_scene(record["scene"], ROOT / manifest["ground_dir"])
        require(fresh["raw_sha256"] == record["raw_sha256"] and fresh["mask_sha256"] == record["mask_sha256"], f"brut ou masque altéré : scène {record['scene']}")
        require(fresh["counts"] == record["counts"], f"effectifs différents : scène {record['scene']}")
        for name in PIECES:
            require(fresh["pieces"][name]["sha256"] == record["pieces"][name]["sha256"] and fresh["pieces"][name]["n"] == record["pieces"][name]["n"],
                    f"morceau régénéré différent : scène {record['scene']} {name}")
            path = ROOT / record["pieces"][name]["path"]
            if path.is_file():
                require(sha256_bytes(path.read_bytes()) == record["pieces"][name]["sha256"], f"fichier sur disque différent : {path}")
        print(json.dumps(dict(scene=record["scene"], kept_sites=record["counts"]["kept_sites"], identical=True)), flush=True)
    print(json.dumps(dict(status="passed", scenes=len(manifest["scenes"]))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub_parsers = parser.add_subparsers(dest="operation", required=True)
    for name in ("run", "read"):
        p = sub_parsers.add_parser(name)
        p.add_argument("--manifest", type=Path, default=MANIFEST)
        p.add_argument("--ground-dir", type=Path, default=GROUND_DIR)
        if name == "run":
            p.add_argument("--out-dir", type=Path, default=OUT_DIR)
            p.add_argument("--scenes", nargs="+", default=list(SCENES))
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
