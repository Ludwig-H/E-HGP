#!/usr/bin/env python3
"""Inventaire statique A6c epingle ; aucun moteur, compilateur ni CTest."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

SOURCE = "aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66"
BASE = "8a0716e7470197c95953b38d79f26b8d8f2379fc"
PREFIX = "morsehgp3D_v12/"
RELS = ("src/tower/pipeline.hpp", "src/tower/pipeline.cpp", "src/tower/pipeline_steps.cpp",
        "src/tower/pipeline_run.cpp", "tests/tower/pipeline_unit.cpp", "tests/tower/pipeline_fault.cpp",
        "tests/tower/pipeline_levers.cpp", "tests/tower/region_unit.cpp", "tests/tower/tests.cmake",
        "tests/mutants/tower.json", "tests/mutants/run_mutants.py")

def need(ok, message):
    if not ok:
        raise ValueError(message)

def main():
    need(len(sys.argv) == 2, "usage: check.py REPO")
    repo = Path(sys.argv[1])
    cache = {}
    def read(rel, rev=SOURCE):
        key = (rev, rel)
        if key not in cache:
            cache[key] = subprocess.check_output(["git", "show", rev + ":" + PREFIX + rel], cwd=repo)
        return cache[key].decode("utf-8")
    for rel in RELS:
        read(rel)
    manifest = json.loads(read("tests/mutants/tower.json"))
    baseline = json.loads(read("tests/mutants/tower.json", BASE))
    old = {m["id"] for m in baseline["mutants"]}
    substitutions = 0
    for mutant in manifest["mutants"]:
        mutated = {}
        for edit in [mutant] + mutant.get("aussi", []):
            rel, find = edit["fichier"], edit["cherche"]
            text = mutated.setdefault(rel, read(rel))
            need(text.count(find) == 1, "motif non unique: " + mutant["id"])
            mutated[rel] = text.replace(find, edit["remplace"])
            substitutions += 1
    added = [m for m in manifest["mutants"] if m["id"] not in old]
    cmake = read("tests/tower/tests.cmake")
    gates = set()
    for target, body in re.findall(r"mhgp12_add_unit\((mhgp12_\w+)\s+([\s\S]*?)\)", cmake):
        match = re.search(r"GROUPS\s+(.+?)\s+LABELS", body, re.S)
        if match:
            gates.update(target + "_" + group for group in match.group(1).split())
    need(all(m["porte"] in gates for m in added), "porte ajoutee absente de tests.cmake")
    header = read("src/tower/pipeline.hpp")
    threshold = int(re.search(r"kChainSites = (\d+)", header).group(1))
    unit = read("tests/tower/pipeline_unit.cpp")
    fault = read("tests/tower/pipeline_fault.cpp")
    levers = read("tests/tower/pipeline_levers.cpp")
    need("chain_sites" not in unit and "chain_sites" not in fault, "modes des portes modifies")
    for token in ("30 + static_cast<u32>(rng() % 200)", "60 + static_cast<u32>(rng() % 100)",
                  "m.ok = tower::detail::admit_session(*run).ok() && tower::detail::play_session(*run).ok();"):
        need(token in unit, "fixture admission/refus modifiee")
    for token in ("make_chain(70, 4, 5, budget, *pool)", "make_chain(60, 4, 9, budget, *three)",
                  "auto tower = build_tower(*c.index, *c.catalogue, budget, pool);"):
        need(token in fault, "fixture penurie modifiee")
    for token in ("const auto off = play(c, budget, *pool, kNever);", "const auto on = play(c, budget, *pool, 0);",
                  "CHECK_EQ(digest_of(c, a), digest_of(c, b));", "CHECK(same_registry(a.orders[i], b.orders[i]));",
                  "CHECK(same_history(a.orders[i], b.orders[i]));", "CHECK(off->diag.forest.work[i] == on->diag.forest.work[i]);",
                  "CHECK(compared == 9 && hints_on > 0);"):
        need(token in levers, "fixture bascule modifiee")
    need(229 < threshold and 159 < threshold and 70 < threshold, "fixtures plus sous le seuil")
    results = {
        "schema": "audit_a6c_portes_static_v1", "source": SOURCE, "base": BASE,
        "native_executed": False, "compiler_executed": False, "gcp_used": False,
        "mutants": {"baseline": len(old), "candidate": len(manifest["mutants"]), "floor": manifest["plancher"],
                    "substitutions_unique_sequential": substitutions,
                    "added": [{"id": m["id"], "gate": m["porte"]} for m in added]},
        "coverage_from_source": {"chain_sites": threshold,
            "admission": {"sites_min": 30, "sites_max": 229, "orders": [2, 3, 4, 5], "mode": "default_off"},
            "refusal": {"sites_min": 60, "sites_max": 159, "orders": [4], "mode": "default_off"},
            "fault_throwing": {"sites": 70, "orders": [4], "mode": "default_off"},
            "fault_nothrow": {"sites": 60, "orders": [4], "mode": "default_off"},
            "toggle": {"forced_on_off": True, "success_only_unlimited_budget": True,
                       "sessions_pairs": 9, "sites": [1000, 2000, 3000], "orders": [5], "threads": [1, 3, 8]}},
        "source_sha256": {rel: hashlib.sha256(cache[(SOURCE, rel)]).hexdigest()
                          for rev, rel in sorted(cache) if rev == SOURCE}}
    expected = json.loads(Path(__file__).with_name("results.json").read_text())
    need(results == expected, "ecart avec results.json")
    print("a6c_portes_ok mutants=72 substitutions=76 nouveaux=17 admission_penurie=chaine_non_engagee natif=non")

if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print("REFUS " + str(error), file=sys.stderr)
        sys.exit(2)
