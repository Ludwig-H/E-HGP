#!/usr/bin/env python3
"""MorseHGP3D v3 — census de la profondeur faible, avec son recu complet.

CE QUE CE CENSUS MESURE, ET CE QU'IL NE MESURE PAS.

Il mesure un MINORANT de l'ensemble

    A = { p : il existe une direction e telle que le demi-espace ouvert
              { x : <x - p, e> > 0 } contienne moins de s_max points }

c'est-a-dire l'ensemble ou la borne tangente NON CONTRAINTE de `morsehgp3D_v2`
(`catalogue.cpp:radius_bound`) vaut +infini. Le minorant vient de ce que seules
D directions sont echantillonnees.

Il ne mesure PAS l'ensemble ou la borne a centre convexe R echoue. Cet ensemble
est VIDE : si le centre est dans conv(X) et la boule passe par p appartenant a X,
alors le rayon est au plus diam(X), donc R est toujours finie. Une version
anterieure de ce census presentait le tableau comme un majorant de cet
ensemble : l'enonce etait vacue et il a ete retire.

Conventions fixees explicitement, parce que le decalage d'une unite compte a
K <= 10 :
  - demi-espace OUVERT, strictement <x - p, e> > 0 ;
  - p lui-meme n'est jamais compte (il verifie <p - p, e> = 0) ;
  - un point exactement sur le plan frontiere n'est PAS compte ;
  - le critere retient donc p des qu'il est parmi les s_max premiers par
    projection dans une direction, ties inclus par argpartition.
"""

import hashlib
import json
import os
import platform
import subprocess
import sys
import time

import numpy as np

SCHEMA = "morsehgp3d.v3.census.tukey_shallow.v2"
DIRECTIONS = 4096
DIRECTION_SEED = 1
DECIMATION_SEED = 7
UNIFORM_SEED = 3


def digest_of(array):
    return hashlib.sha256(np.ascontiguousarray(array, dtype=np.float64).tobytes()).hexdigest()


def read_ply_xyz(path):
    with open(path, "rb") as handle:
        header = []
        while True:
            line = handle.readline().decode("ascii", "replace")
            header.append(line)
            if line.strip() == "end_header":
                break
        count = [int(l.split()[2]) for l in header if l.startswith("element vertex")][0]
        rows = [handle.readline().split()[:3] for _ in range(count)]
    return np.array(rows, dtype=np.float64)


def quaternion_to_matrix(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ])


def assemble_multi_capture(directory):
    """Assemble les captations brutes selon bun.conf : x -> R(q) x + t."""
    parts, manifest = [], []
    for line in open(os.path.join(directory, "bun.conf")):
        field = line.split()
        if not field or field[0] != "bmesh":
            continue
        points = read_ply_xyz(os.path.join(directory, field[1]))
        translation = np.array([float(field[2]), float(field[3]), float(field[4])])
        rotation = quaternion_to_matrix([float(f) for f in field[5:9]])
        parts.append(points @ rotation.T + translation)
        manifest.append({"mesh": field[1], "points": int(points.shape[0])})
    return np.vstack(parts), manifest


def shallow_witnesses(points, s_max, directions, seed):
    """Minorant de A : pour chaque direction, les s_max points de projection
    maximale sont des temoins. Voir la convention de demi-espace ouvert."""
    n = len(points)
    generator = np.random.default_rng(seed)
    axes = generator.normal(size=(directions, 3))
    axes /= np.linalg.norm(axes, axis=1, keepdims=True)
    marked = np.zeros(n, dtype=bool)
    k = min(s_max, n)
    for start in range(0, directions, 128):
        projection = points @ axes[start:start + 128].T
        marked[np.unique(np.argpartition(-projection, k - 1, axis=0)[:k])] = True
    return marked


def decimate(points, target, seed):
    if len(points) <= target:
        return None
    generator = np.random.default_rng(seed)
    return points[generator.choice(len(points), target, replace=False)]


def git_commit():
    try:
        return subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    except Exception:  # noqa: BLE001 — l'absence de git n'invalide pas la mesure
        return "unavailable"


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "clouds"
    out_path = sys.argv[2] if len(sys.argv) > 2 else "census_tukey_shallow.json"

    zipper = read_ply_xyz(os.path.join(root, "bunny/reconstruction/bun_zipper.ply"))
    multi, manifest = assemble_multi_capture(os.path.join(root, "bunny/data"))

    sources = {
        "bunny_zipper": {
            "origin": "https://graphics.stanford.edu/pub/3Dscanrep/bunny.tar.gz",
            "file": "bunny/reconstruction/bun_zipper.ply",
            "points": int(zipper.shape[0]),
            "digest_sha256": digest_of(zipper),
            "nature": "reconstruction fusionnee d'un scan reel",
        },
        "bunny_multi_10_captations": {
            "origin": "https://graphics.stanford.edu/pub/3Dscanrep/bunny.tar.gz",
            "assembly": "bun.conf, x -> R(quaternion) x + translation",
            "meshes": manifest,
            "points": int(multi.shape[0]),
            "digest_sha256": digest_of(multi),
            "nature": "dix captations brutes recalees, avec recouvrement",
        },
        "uniform_cube_volumetrique": {
            "origin": f"numpy default_rng({UNIFORM_SEED}).random",
            "nature": "temoin volumetrique",
        },
    }

    attempted = decided = skipped_too_small = 0
    rows = []
    started = time.time()
    for name, cloud in (("bunny_zipper", zipper),
                        ("bunny_multi_10_captations", multi),
                        ("uniform_cube_volumetrique", None)):
        for n in (5000, 20000, 50000):
            for s_max in (3, 11):
                attempted += 1
                if cloud is None:
                    sample = np.random.default_rng(UNIFORM_SEED).random((n, 3))
                else:
                    sample = decimate(cloud, n, DECIMATION_SEED)
                    if sample is None:
                        skipped_too_small += 1
                        rows.append({"cloud": name, "n": n, "s_max": s_max,
                                     "status": "skipped_cloud_smaller_than_target"})
                        continue
                marked = shallow_witnesses(sample, s_max, DIRECTIONS, DIRECTION_SEED)
                decided += 1
                rows.append({"cloud": name, "n": int(n), "s_max": int(s_max),
                             "status": "decided",
                             "sample_digest_sha256": digest_of(sample),
                             "shallow_witnesses": int(marked.sum()),
                             "percent": round(100.0 * float(marked.mean()), 4)})
    elapsed = time.time() - started

    receipt = {
        "schema": SCHEMA,
        "measures": "MINORANT de { p : tau_non_contrainte(p) = +infini }",
        "does_not_measure": ("l'ensemble ou la borne a centre convexe R echoue ; "
                             "cet ensemble est VIDE car R <= diam(X)"),
        "half_space": "ouvert, <x-p,e> > 0 ; p non compte ; frontiere non comptee",
        "direction_set": {"count": DIRECTIONS, "law": "gaussienne normalisee",
                          "generator": "numpy default_rng", "seed": DIRECTION_SEED},
        "decimation": {"algorithm": "choice sans remise", "seed": DECIMATION_SEED},
        "provenance": {
            "git_commit": git_commit(),
            "python": platform.python_version(),
            "numpy": np.__version__,
            "platform": platform.platform(),
            "processor": platform.processor() or "unknown",
        },
        "sources": sources,
        "campaign": {"attempted": attempted, "decided": decided,
                     "skipped_cloud_smaller_than_target": skipped_too_small,
                     "identity_closed": attempted == decided + skipped_too_small},
        "elapsed_seconds": round(elapsed, 3),
        "rows": rows,
    }
    with open(out_path, "w") as handle:
        json.dump(receipt, handle, indent=1, ensure_ascii=False)
    for row in rows:
        print(row)
    print(f"attempted={attempted} decided={decided} skipped={skipped_too_small} "
          f"identite_fermee={receipt['campaign']['identity_closed']}")
    return 0 if receipt["campaign"]["identity_closed"] else 1


if __name__ == "__main__":
    sys.exit(main())
