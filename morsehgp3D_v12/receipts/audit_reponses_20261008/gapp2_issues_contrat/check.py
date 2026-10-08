#!/usr/bin/env python3
"""Source Git et generation textuelle seulement : aucun C++ execute."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
PREFIX = "morsehgp3D_v12/"


def require(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    cap = json.loads((HERE / "capture.json").read_text())
    repo = Path(sys.argv[1]).resolve() if len(sys.argv) == 2 else HERE
    pin = cap["commit"]

    def git(*args):
        return subprocess.check_output(["git", "-C", str(repo), *args])

    source = {}
    for rel, expected in cap["sources"].items():
        data = git("show", pin + ":" + PREFIX + rel)
        require(hashlib.sha256(data).hexdigest() == expected, "source: " + rel)
        source[rel] = data
    contract = "docs/CONTRAT_TOUR.md"
    require(source[contract] == git("show", pin + "^:" + PREFIX + contract), "contrat modifie")
    changed = git("diff-tree", "--no-commit-id", "--name-only", "-r", pin).decode().splitlines()
    require(not any(x.startswith(PREFIX + "src/") for x in changed), "produit modifie")
    with tempfile.TemporaryDirectory(prefix="gapp2-text-") as raw:
        tmp = Path(raw)
        names = ("microbancs/mes_g_appareil/generer_l4_hd.py", "src/tower/proposal.hpp",
                 "microbancs/mes_t2d_b/bras_t2d_b.json")
        paths = []
        for name in names:
            p = tmp / Path(name).name
            p.write_bytes(source[name])
            paths.append(str(p))
        command = [sys.executable] + (["-O"] if sys.flags.optimize else []) + paths
        generated = subprocess.check_output(command)
    native = source["microbancs/mes_g_appareil/noyau_g.hpp"]
    require(native.count(generated) == 1, "bloc genere absent ou multiple")
    for suffix in (b"L4HD", b"L4F32HD"):
        require(generated.count(b"class DWelzl" + suffix + b" {") == 1, "classe manquante")
    # Borne arithmetique du chemin entier, pas un test de son execution C++.
    bound = 3 * ((1 << 30) - 1) ** 2
    require(bound < (1 << 63), "borne i64 B30")
    require(3 * ((1 << 31) - 1) ** 2 >= (1 << 63), "contre-borne B31")
    result = {
        "source_hashes": len(source), "changed_files": len(changed),
        "product_sources_changed": 0, "contract_changed": False,
        "generated_block_bytes": len(generated),
        "generated_block_sha256": hashlib.sha256(generated).hexdigest(),
        "generated_block_occurrences": 1,
        "integer_abs_sum_bound_B30": bound,
        "native_executed": False,
    }
    if "result" in cap:
        require(result == cap["result"], "resultat different de la capture")
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
