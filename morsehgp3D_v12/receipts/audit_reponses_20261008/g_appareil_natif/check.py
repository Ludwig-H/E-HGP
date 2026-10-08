#!/usr/bin/env python3
"""Rejeu des seules épingles Git du reçu ; aucun moteur ni donnée d'entrée."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check.py DEPOT")
    capture = json.loads(Path(__file__).with_name("capture.json").read_text())
    same = []
    for path, expected in capture["sources"].items():
        blobs = []
        for commit in (capture["source_commit"], capture["delivery_commit"]):
            blobs.append(subprocess.check_output(
                ["git", "show", commit + ":morsehgp3D_v12/" + path], cwd=sys.argv[1]))
        if hashlib.sha256(blobs[0]).hexdigest() != expected:
            raise SystemExit("source différente: " + path)
        if blobs[0] == blobs[1]:
            same.append(path)
    if same != capture["unchanged_at_delivery"]:
        raise SystemExit("liaison de livraison différente")
    result = {"sources_verified": len(capture["sources"]), "same_at_delivery": len(same),
              "native_run": False, "memory_formula_bytes_per_request":
              {"request": 144, "result": 52, "site_output": 76 * 4,
               "total_input_and_output": 144 + 52 + 76 * 4}}
    if result != capture["result"]:
        raise SystemExit("résultat différent")
    print(json.dumps(result, sort_keys=True, ensure_ascii=False))


if __name__ == "__main__":
    main()
