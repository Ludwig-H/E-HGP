#!/usr/bin/env python3
"""B3 source and prior test-log reader; no native execution."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    need(len(sys.argv) == 3, "usage: check.py DEPOT_GIT SNAPSHOT_B3")
    repo, snap = map(lambda s: Path(s).resolve(), sys.argv[1:])
    here = Path(__file__).resolve().parent
    c = json.loads((here / "capture.json").read_text())
    src = snap / "source_main/morsehgp3D_v12"
    for p, h in c["source_pins"].items():
        need(sha((src / p).read_bytes()) == h, "snapshot " + p)
        need(sha(subprocess.check_output(["git", "show", c["source_git"] + ":morsehgp3D_v12/" + p], cwd=repo)) == h,
             "Git " + p)
    for pin in [c["source_git"], c["base_retrait"]]:
        data = subprocess.check_output(["git", "show", pin + ":morsehgp3D_v12/src/tower/pipeline_run.cpp"], cwd=repo)
        need(sha(data) == c["main_pipeline_r1_sha256"], "R1 pipeline")
    for build, spec in c["prototype_sources"].items():
        paths = [*c["source_pins"], "src/tower/pipeline_run.cpp"]
        pins = {p: sha((snap / "proto_sources" / build / "morsehgp3D_v12" / p).read_bytes()) for p in paths}
        inventory = "".join(h + "  " + p + "\n" for p, h in sorted(pins.items()))
        need(len(pins) == spec["count"] and sha(inventory.encode()) == spec["inventory_sha256"], "prototype inventory")
        diff = {p: h for p, h in pins.items() if h != c["source_pins"].get(p)}
        need(diff == spec["differences_vs_main"], "prototype differences")
        need(pins["src/tower/pipeline_run.cpp"] == "60ca4cabbc554e051d7997f71d9c353eb502490585c0e733b216766ccad9e6c5", "A6b origin")
    raw = {}
    for p, spec in c["primary"].items():
        data = (snap / p).read_bytes()
        need(len(data) == spec["bytes"] and sha(data) == spec["sha256"], p)
        raw[p] = data.decode()
    need(raw["main/logs_main/b3.etat"].splitlines() == ["build_ok", "ctest 0"], "main driver")
    results = {}
    cohorts = [("lot", "proto/logs/b3r_lot.ctest.log", "proto/build_b3r_lot", 754, 0, "749.31"),
               ("cles", "proto/logs/b3r_cles.ctest.log", "proto/build_b3r_cles", 82, 0, "214.17"),
               ("main", "main/logs_main/b3_ctest.log", "main/build_v12_u21", 747, 1, "584.67")]
    for name, log, build, total, skips, wall in cohorts:
        rows = re.findall(r"^\s*(\d+)/" + str(total) + r" Test\s+#(\d+):\s+(\S+)\s+\.+\s*(Passed|\*\*\*Skipped)\s+[0-9.]+ sec$", raw[log], re.M)
        need(len(rows) == total and len({r[2] for r in rows}) == total, name + " rows")
        need(sorted(int(r[0]) for r in rows) == list(range(1, total + 1)), name + " numbering")
        skipped = [r[2] for r in rows if r[3] != "Passed"]
        need(skipped == (["mhgp12_support_lidar_sentinel"] if skips else []), name + " skipped")
        need("0 tests failed out of " + str(total) in raw[log] and "= " + wall + " sec" in raw[log], name + " summary")
        last = raw[build + "/Testing/Temporary/LastTest.log"]
        names = re.findall(r"^\d+/\d+ Testing: (\S+)$", last, re.M)
        need(len(names) == total and set(names) == {r[2] for r in rows}, name + " detailed cohort")
        need(len(re.findall(r"^Test Passed\.$", last, re.M)) == total - skips, name + " detailed passed")
        need("End testing:" in last and not re.search(r"^Test Failed\.$", last, re.M), name + " closure")
        cache = raw[build + "/CMakeCache.txt"].splitlines()
        for line in ["CMAKE_BUILD_TYPE:STRING=Release", "MHGP12_COORD_BITS:STRING=21", "MHGP12_ENABLE_CUDA:BOOL=OFF"]:
            need(line in cache, name + " configuration")
        results[name] = {"selected": total, "passed": total - skips, "skipped": skips, "failed": 0, "wall_seconds": float(wall)}
    for name, spec in c["postrun_artifacts"].items():
        data = (snap / "main/build_v12_u21" / name).read_bytes()
        need(data.startswith(b"\x7fELF") and len(data) == spec["bytes"] and sha(data) == spec["sha256"], name)
    new = []
    for module, floor, ids in [("catalogue", 38, ["cles_case_absente_nulle", "cles_bits_decales", "cles_non_transmises", "cles_tranches_case_fausse"]),
                               ("tower", 57, ["cles_garde_hors_nuage", "balayage_dernier_oublie"])]:
        manifest = json.loads((src / ("tests/mutants/" + module + ".json")).read_text())
        need(manifest["plancher"] == len(manifest["mutants"]) == floor, module + " manifest")
        by_id = {m["id"]: m for m in manifest["mutants"]}
        for mid in ids:
            m = by_id[mid]
            need((src / m["fichier"]).read_text().count(m["cherche"]) == 1, mid + " substitution")
            new.append(mid)
    with tempfile.TemporaryDirectory() as td:
        dst = Path(td) / "morsehgp3D_v12"
        shutil.copytree(src, dst)
        subprocess.run(["git", "apply", str(here / "proposition.patch")], cwd=td, check=True, capture_output=True)
        for p, h in c["proposition_postimages"].items():
            need(sha((dst / p).read_bytes()) == h, "postimage " + p)
    result = {"source_git": c["source_git"], "sources_fermees": len(c["source_pins"]), "traces_fermees": len(raw),
              "ctest": results, "mutants_prepares_verifies": new, "mutants_natifs_admis": 0,
              "elf_postrun_uniquement": sorted(c["postrun_artifacts"]), "postimages_patch": len(c["proposition_postimages"]),
              "patch_compile": False, "natif_execute_par_auditeur": False}
    rendered = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    expected = here / "results.json"
    if expected.exists():
        need(expected.read_text() == rendered, "results mismatch")
    print(rendered, end="")


if __name__ == "__main__":
    main()
