#!/usr/bin/env python3
"""Relire les pins du raccord B/A ; aucune execution du moteur."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def require(ok, message):
    if not ok:
        raise ValueError(message)


def blob(repo, pin, path):
    return subprocess.check_output(["git", "-C", str(repo), "show", pin + ":morsehgp3D_v12/" + path])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("repo", type=Path)
    p.add_argument("--prototype", type=Path)
    args = p.parse_args()
    c = json.loads(Path(__file__).with_name("capture.json").read_text())
    require(len(c["source_sha256"]) == 27, "scope")
    for path, expected in c["source_sha256"].items():
        require(hashlib.sha256(blob(args.repo, c["pin"], path)).hexdigest() == expected, path)
    for path in c["raccord_files"]:
        require(blob(args.repo, c["pin"], path) == blob(args.repo, c["snapshot_head"], path), path)
    require(len(c["source_sha256"]) - len(c["raccord_files"]) == 20 and len(c["prototype_different_sha256"]) == 4, "port")
    if args.prototype:
        for path in c["source_sha256"]:
            if path in c["raccord_files"]:
                continue
            expected = c["prototype_different_sha256"].get(path, c["source_sha256"][path])
            require(hashlib.sha256(blob(args.prototype, c["prototype_pin"], path)).hexdigest() == expected, path)
    result = {"ok": True, "pin": c["pin"], "sources_git": 27,
              "raccord_inchange": len(c["raccord_files"]),
              "comparaison_prototype_capturee": {"identiques": 16, "differents": 4},
              "prototype_relu": args.prototype is not None,
              "preuve_semantique": "lecture documentee, pas preuve automatique C++",
              "moteur_execute": False, "gain_mesure": False}
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


if __name__ == "__main__":
    main()
