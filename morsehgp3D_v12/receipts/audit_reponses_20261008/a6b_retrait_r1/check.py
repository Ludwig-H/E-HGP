#!/usr/bin/env python3
"""Verify source restoration from Git only. Never invokes native tests or the engine."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def require(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def main():
    require(len(sys.argv) in (2, 3), "usage: check.py DEPOT_GIT [COMMIT_DE_RETRAIT]")
    repo = Path(sys.argv[1]).resolve()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / "capture.json").read_text())

    def git(*args):
        return subprocess.check_output(["git", *args], cwd=repo)

    def tree(pin):
        paths = git("ls-tree", "-r", "--name-only", pin, "--", *cap["scopes"]).decode().splitlines()
        return {p: git("show", pin + ":" + p) for p in paths}

    base = tree(cap["base_r1"])
    old = tree(cap["prototype_a6b_livre"])
    inventory = "".join(hashlib.sha256(raw).hexdigest() + "  " + p + "\n" for p, raw in sorted(base.items()))
    require(hashlib.sha256(inventory.encode()).hexdigest() == cap["inventaire_sha256"], "inventory")
    require(len(base) == cap["fichiers_locaux"] == 163, "source cohort")
    require(sum(p.startswith("morsehgp3D_v12/src/") for p in base) == cap["fichiers_src"] == 137, "src cohort")
    changes = sorted(p for p in old.keys() & base.keys() if old[p] != base[p])
    removed = sorted(old.keys() - base.keys())
    added = sorted(base.keys() - old.keys())
    require(changes == cap["changements_depuis_a6b"] and len(changes) == 13, "changed files")
    require(removed == cap["supprimes_depuis_a6b"] and len(removed) == 3, "removed files")
    require(added == cap["ajoutes_depuis_a6b"] == [], "added files")
    prefix = "morsehgp3D_v12/"
    before = json.loads(old[prefix + "tests/mutants/tower.json"])
    after = json.loads(base[prefix + "tests/mutants/tower.json"])
    require(before["plancher"] == len(before["mutants"]) == 66, "A6b manifest")
    require(after["plancher"] == len(after["mutants"]) == 55, "R1 manifest")
    remaining = {m["id"]: m for m in after["mutants"]}
    relocated = []
    for mutant in before["mutants"]:
        if mutant["id"] in remaining:
            final = remaining[mutant["id"]]
            if mutant != final:
                require(mutant["id"] == "phases_sans_tranches", "unexpected changed mutant")
                require(mutant["fichier"] == "src/tower/pipeline_steps.cpp" and
                        final["fichier"] == "src/tower/pipeline_run.cpp", "relocation")
                require({k: v for k, v in mutant.items() if k != "fichier"} ==
                        {k: v for k, v in final.items() if k != "fichier"}, "relocated contents")
                relocated.append(mutant["id"])
    require(relocated == ["phases_sans_tranches"], "relocated cohort")
    dropped = [m["id"] for m in before["mutants"] if m["id"] not in remaining]
    pipeline = base[prefix + "src/tower/pipeline_run.cpp"].decode()
    require("const bool last = p.in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1;" in pipeline,
            "CST-0241 last withdraw")
    require("MHGP12_TEST(terminaison, 11)" in base[prefix + "tests/tower/region_unit.cpp"].decode(),
            "termination gate")
    registry = hashlib.sha256(base[prefix + "src/tower/registry_branches.cpp"]).hexdigest()
    require(registry == cap["registry_branches_sha256"], "R1 registry")
    if len(sys.argv) == 3:
        require(tree(sys.argv[2]) == base, "withdrawal commit differs from R1")
    result = {
        "schema": "a6b-retrait-r1-result-v1",
        "capture": "état local non commis après " + cap["head_observe"],
        "source_r1": cap["base_r1"],
        "source_a6b": cap["prototype_a6b_livre"],
        "fichiers_identiques_r1": len(base),
        "src_identiques_r1": cap["fichiers_src"],
        "restaures": len(changes),
        "supprimes": removed,
        "mutants_supprimes": dropped,
        "mutant_relocalise": relocated,
        "plancher_restant": after["plancher"],
        "CST-0241": "corps et porte de terminaison conservés à l’identique de R1",
        "CST-0242": "objet A6b absent du produit capturé, pas de qualification universelle du pont",
        "R1": "registre conservé à l’identique",
        "nouvelles_portes_natives_jouees_par_auditeur": 0,
    }
    rendered = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    expected = here / "results.json"
    if expected.exists():
        require(expected.read_text() == rendered, "result mismatch")
    print(rendered, end="")


if __name__ == "__main__":
    main()
