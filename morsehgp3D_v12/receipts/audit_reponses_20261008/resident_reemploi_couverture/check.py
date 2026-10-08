#!/usr/bin/env python3
"""Source pins and declarative protocol only. No native code, device or data access."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def read_git(repo, pin, path):
    return subprocess.check_output(["git", "show", pin + ":" + path], cwd=repo)


def main():
    require(len(sys.argv) == 2, "usage: check.py DEPOT_GIT")
    repo = Path(sys.argv[1]).resolve()
    here = Path(__file__).resolve().parent
    capture = json.loads((here / "capture.json").read_text())
    plan = json.loads((here / "protocole.json").read_text())
    sources = {}
    old_equal = 0
    for entry in capture["sources"]:
        raw = read_git(repo, capture["source_commit"], entry["path"])
        require(hashlib.sha256(raw).hexdigest() == entry["sha256"], entry["path"])
        eq = raw == read_git(repo, capture["source_l1t"], entry["path"])
        require(eq == entry["identique_caf9585e4"], "old pin: " + entry["path"])
        old_equal += int(eq)
        sources[entry["path"].split("morsehgp3D_v12/", 1)[1]] = raw.decode()
    require(len(sources) == 12 and old_equal == 11, "source cohort")

    budget = sources["tests/catalogue/device_budget_test.cpp"]
    trial = budget.split("Trial device_trial(", 1)[1].split("}  // namespace", 1)[0]
    require("CatalogueDevice::open(host, device_budget)" in trial, "fresh context")
    require("MHGP12_TEST(pipeline_budget_reuse, 6)" in budget, "reuse gate")
    require("MemoryBudget host(free.host_peak - 1)" in budget, "refusal before small case")
    pipeline = sources["tests/catalogue/device_pipeline_test.cpp"]
    actual = pipeline.split("MHGP12_TEST(device_open, 1)", 1)[1]
    require("MemoryBudget budget(MemoryBudget::kUnlimited)" in actual, "unlimited device gate")
    require(actual.count("build_catalogue_device(") == 2, "two successful calls")
    sliced = sources["tests/catalogue/slices_test.cpp"].split("MHGP12_TEST(slices_device_reuse, 7)", 1)[1]
    require(sliced.count("finish_slices >= 1") == 2, "sliced large calls")
    require("small_diag.finish_slices == 0" in sliced and "device(peak / 3)" in sliced, "small intervening call")
    require("2 * buffer.size()" in sources["src/catalogue/traversal.hpp"], "Pool growth")
    require("a.cap + a.cap / 2 + 1024" in sources["src/catalogue/device_cuda.cu"], "CUDA growth")
    support = sources["tests/catalogue/device_support.hpp"]
    staged = support.split("struct StagedExecutor : PoolExecutor", 1)[1].split("// Voie appareil jouee", 1)[0]
    require("Outcome ensure(" not in staged and "Outcome ensure_keep(" not in staged, "inherited growth")
    require("bool adopt(A&, Buffer<T>&, u64)" in staged and "return false;" in staged, "no adoption")

    diag, fixed = plan["diagnostic"], plan["porte_figee"]
    fractions = diag["budget_appareil_fractions_pic_froid"]
    require(fractions == [[1, 1], [15, 16], [7, 8], [3, 4], [5, 8], [1, 2], [3, 8], [1, 3]], "fractions")
    require(len(diag["fixtures_publiques"]) == 2 and diag["passes_par_contexte"] == 3, "diagnostic cohort")
    require(fixed["passes"] == 3 and fixed["resultats_detruits_entre_passes"], "lifetime")
    require(fixed["aucun_petit_nuage_intercale"] and fixed["aucune_purge_appelant"], "same immediate input")
    require(diag["route_requise_passe0"]["front_rendu"] and diag["route_requise_passe0"]["finish_slices"] == 0,
            "full after release witness")
    require(plan["gpu"]["qualification_distincte"] and capture["nouveaux_resultats_natifs"] is False,
            "scope")
    result = {
        "schema": "audit-resident-reemploi-couverture-result-v1",
        "source_commit": capture["source_commit"],
        "sources_verifiees": len(sources),
        "sources_identiques_l1t": old_equal,
        "device_open": "deux appels par témoin, contexte conservé, budget illimité",
        "device_open_budget": "14 appels, contextes neufs",
        "slices_device_reuse": "gros par tranches / petit complet / gros par tranches",
        "capacites_pool_cuda_identiques": False,
        "diagnostic_contextes_bornes": len(diag["fixtures_publiques"]) * len(fractions),
        "diagnostic_passes_bornees_demandees": len(diag["fixtures_publiques"]) * len(fractions) * 3,
        "native_execute_par_auditeur": False,
        "correctif_qualifie": False,
        "CST-0243": "ouvert",
    }
    rendered = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    expected = here / "results.json"
    if expected.exists():
        require(rendered == expected.read_text(), "results mismatch")
    print(rendered, end="")


if __name__ == "__main__":
    main()
