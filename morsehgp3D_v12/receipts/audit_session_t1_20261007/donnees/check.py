#!/usr/bin/env python3
"""Petits témoins de préparation de données ; aucun téléchargement ni jeu réel."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[4]
BASE = ROOT / "morsehgp3D_v12/bench/data"
PIN = "4147c546000b198b5239646063bfb1e3ed6d28fc"
FILES = ["v12data/__init__.py", "v12data/common.py", "verify_inputs.py", "crop_scenes.py", "replay_all.sh"]


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sources():
    hashes = {}
    for rel in FILES:
        path = BASE / rel
        data = path.read_bytes()
        name = path.relative_to(ROOT).as_posix()
        need(data == subprocess.check_output(["git", "-C", str(ROOT), "show", f"{PIN}:{name}"]),
             "source différente du pin : " + name)
        hashes[name] = hashlib.sha256(data).hexdigest()
    return hashes


def run(argv, **kwargs):
    return subprocess.run(argv, capture_output=True, text=True, timeout=30, **kwargs)


def main():
    before = sources()
    sys.path.insert(0, str(BASE))
    import numpy as np
    from v12data import common
    quantization = common.selftest_quantize()
    need(quantization == {"values_checked":28030, "mismatches":0}, "auto-contrôle grille modifié")
    with tempfile.TemporaryDirectory(prefix="v12-audit-data-") as tmp:
        work = Path(tmp)
        scene = work / "scene"
        scene.mkdir()
        q = np.array([[0,0,z] for z in range(4)], dtype=np.int64)
        output = common.prepare_outputs(scene, "column", q)
        record = output["outputs"]["raw"]
        manifest = scene / "manifest.json"
        manifest.write_text(json.dumps({"schema":common.SCHEMA_SET,"cases":[record]}))
        valid = run([sys.executable,"-S",str(BASE/"verify_inputs.py"),str(manifest)])
        need(valid.returncode == 0, "témoin nominal refusé")
        probes = []
        for name, payload, expected in [
            ("empty_manifest", {"schema":common.SCHEMA_SET,"cases":[]}, 0),
            ("null_coordinate_digest", {"schema":common.SCHEMA_SET,"cases":[dict(record,sha256=None)]}, 0),
            ("wrong_coordinate_digest", {"schema":common.SCHEMA_SET,"cases":[dict(record,sha256="0"*64)]}, 1),
        ]:
            manifest.write_text(json.dumps(payload))
            got = run([sys.executable,"-S",str(BASE/"verify_inputs.py"),str(manifest)])
            need(got.returncode == expected, "verdict du vérificateur modifié : "+name)
            probes.append({"case":name,"exit":got.returncode,"has_no_gaps": "0 ecarts" in got.stdout})
        replay = run(["bash",str(BASE/"replay_all.sh"),"verify"],
                     env=dict(os.environ, ROOT=str(work/"output"), PY=sys.executable))
        need(replay.returncode == 2 and "/scripts/verify_inputs.py" in replay.stderr
             and "No such file or directory" in replay.stderr, "chemin du pilote modifié")
        manifest.write_text(json.dumps({"schema":common.SCHEMA_SET,"cases":[record]}))
        cropped = run([sys.executable,"-I",str(BASE/"crop_scenes.py"),"--manifest",str(manifest),"--sizes","2"])
        need(cropped.returncode == 0, "découpe refusée : "+cropped.stderr)
        case = json.loads(manifest.read_text())["crops"][0]
        ids = np.fromfile(scene/case["point_ids"],"<u4").tolist()
        need(ids == [0,1] and case["count"] == 2 and case["crop"]["radius_chebyshev_mm"] == 0,
             "témoin de colonne modifié")
        # Une coupe horizontale de rayon zéro inclut les quatre sites de cette colonne, ou aucun.
        crop = {"source_sites":4,"selected_ids":ids,"selected_sites":2,"radius_xy":0,
                "sites_within_declared_horizontal_crop":4,"entire_height_preserved":False}
        # Limite complémentaire de la promesse d'identifiant minimal, sans attribuer ce cas aux données publiées.
        duplicate = common.prepare_outputs(work/"ids", "ids", np.array([[0,0,0],[0,0,0],[1,0,0]]),
                                           ids=np.array([9,2,4]))
        distinct = duplicate["outputs"]["distinct"]
        representatives = np.fromfile(work/"ids"/distinct["point_ids"],"<u4").tolist()
        need(representatives == [9,4], "départage des identifiants modifié")
        extra = {"input_ids":[9,2,4],"distinct_ids":representatives,"minimal_ids_expected":[2,4],
                 "scope":"helper accepts unsorted external IDs; no current real scene mismatch shown"}
    need(sources() == before, "sources modifiées pendant le contrôle")
    print(json.dumps({"pin":PIN,"sources":before,"numpy":np.__version__,"quantization":quantization,
        "verification":probes,"replay":{"exit":replay.returncode,"missing_sibling_script":True},
        "crop":crop,"representative_ids":extra,"scope":"synthetic, no external data or network"},
        ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
