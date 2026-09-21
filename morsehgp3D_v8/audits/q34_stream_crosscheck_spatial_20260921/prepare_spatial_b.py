#!/usr/bin/env python3
"""Préparation spatiale indépendante de l'auditeur B : scène, moitiés et quarts d'un scan brut, en entiers exacts.

Entrée : un fichier KITTI `.bin` (float32 little-endian x, y, z, réflectance) dans son repère capteur. Quantification
q = floor(50·x + 32768 + 1/2) calculée en rationnels exacts depuis la valeur float32 décodée (aucun produit flottant),
refus de toute valeur non finie ou hors de [0, 65535] ; déduplication GLOBALE des triplets u16 avant découpe ; ordre
lexicographique (x, y, z) ; plans qx = 32768 puis qy = 32768, sites sur le plan du côté non négatif. Sept fichiers u16le
(scène, deux moitiés, quatre quarts) et un manifeste (sha256, effectifs, conventions). Sert de témoin indépendant de la
préparation du constructeur ; aucun assert.
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

ORIGIN, SCALE = 32768, 50
RAW = struct.Struct("<ffff")
SITE = struct.Struct("<HHH")


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def quantize(value):
    require(math.isfinite(value), "valeur non finie")
    q = math.floor(Fraction(SCALE) * Fraction(value) + Fraction(2 * ORIGIN + 1, 2))
    require(0 <= q <= 65535, f"hors grille : {value}")
    return q


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    data = args.raw.read_bytes()
    require(len(data) % RAW.size == 0, "taille brute non multiple de 16 octets")
    sites = set()
    raw_count = 0
    for x, y, z, _r in RAW.iter_unpack(data):
        raw_count += 1
        sites.add((quantize(x), quantize(y), quantize(z)))
    full = sorted(sites)
    pieces = {
        "full": full,
        "half_xneg": [s for s in full if s[0] < ORIGIN],
        "half_xpos": [s for s in full if s[0] >= ORIGIN],
        "quarter_xneg_yneg": [s for s in full if s[0] < ORIGIN and s[1] < ORIGIN],
        "quarter_xneg_ypos": [s for s in full if s[0] < ORIGIN and s[1] >= ORIGIN],
        "quarter_xpos_yneg": [s for s in full if s[0] >= ORIGIN and s[1] < ORIGIN],
        "quarter_xpos_ypos": [s for s in full if s[0] >= ORIGIN and s[1] >= ORIGIN],
    }
    require(len(pieces["half_xneg"]) + len(pieces["half_xpos"]) == len(full), "moitiés non exhaustives")
    require(len(pieces["quarter_xneg_yneg"]) + len(pieces["quarter_xneg_ypos"]) == len(pieces["half_xneg"]) and
            len(pieces["quarter_xpos_yneg"]) + len(pieces["quarter_xpos_ypos"]) == len(pieces["half_xpos"]), "quarts non exhaustifs")
    args.out.mkdir(parents=True, exist_ok=True)
    manifest = dict(schema="audit_b_spatial_preparation_v1", raw=str(args.raw), raw_sha256=hashlib.sha256(data).hexdigest(),
                    raw_bytes=len(data), raw_returns=raw_count, unique_sites=len(full), merged_returns=raw_count - len(full),
                    quantization="floor(50*x + 32768 + 1/2), rationnels exacts depuis float32, refus hors [0,65535] ou non fini",
                    deduplication="globale sur (qx,qy,qz) avant découpe", order="lexicographique (x,y,z)",
                    cut="qx = 32768 puis qy = 32768 ; site sur le plan du côté >= ; restrictions disjointes et exhaustives",
                    on_plane_x=sum(1 for s in full if s[0] == ORIGIN), on_plane_y=sum(1 for s in full if s[1] == ORIGIN), pieces={})
    for name, rows in pieces.items():
        payload = b"".join(SITE.pack(*s) for s in rows)
        path = args.out / f"{name}.u16le"
        path.write_bytes(payload)
        manifest["pieces"][name] = dict(path=str(path), n=len(rows), bytes=len(payload), sha256=hashlib.sha256(payload).hexdigest())
    args.manifest.write_text(json.dumps(manifest, indent=1, sort_keys=True) + "\n")
    print(json.dumps({k: v["n"] for k, v in manifest["pieces"].items()}), "merged", manifest["merged_returns"], "on_plane_x", manifest["on_plane_x"], "on_plane_y", manifest["on_plane_y"])
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        sys.exit(1)
