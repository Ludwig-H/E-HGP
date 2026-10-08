#!/usr/bin/env python3
"""Read already-produced CTest logs and Git objects; never runs native code."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def main():
    require(len(sys.argv) == 3, "usage: check.py DEPOT_GIT SNAPSHOT_PRIMAIRE")
    repo, snapshot = (Path(a).resolve() for a in sys.argv[1:])
    here = Path(__file__).resolve().parent
    capture = json.loads((here / "capture.json").read_text())
    raw = {}
    for name, spec in capture["primary"].items():
        data = (snapshot / name).read_bytes()
        require(len(data) == spec["bytes"] and hashlib.sha256(data).hexdigest() == spec["sha256"], name)
        raw[name] = data.decode()
    require(len(raw) == 6, "trace cohort")
    require(raw["logs_main/ra6b.etat"].splitlines() == ["build_ok", "ctest 0"], "driver status")
    text = raw["logs_main/ra6b_ctest.log"]
    rows = re.findall(r"^\s*(\d+)/747 Test\s+#(\d+):\s+(\S+)\s+\.+\s*(Passed|\*\*\*Skipped)\s+([0-9.]+) sec$",
                      text, re.M)
    require(len(rows) == 747 and sorted(int(r[0]) for r in rows) == list(range(1, 748)), "CTest rows")
    require(len({r[1] for r in rows}) == 747 and len({r[2] for r in rows}) == 747, "unique tests")
    passed = [r[2] for r in rows if r[3] == "Passed"]
    skipped = [r[2] for r in rows if r[3] == "***Skipped"]
    require(len(passed) == 746 and skipped == ["mhgp12_support_lidar_sentinel"], "test statuses")
    require("100% tests passed, 0 tests failed out of 747" in text, "CTest summary")
    require("Total Test time (real) = 587.74 sec" in text, "wall")
    last = raw["build_v12_u21/Testing/Temporary/LastTest.log"]
    details = re.findall(r"^\d+/\d+ Testing: (\S+)$", last, re.M)
    require(len(details) == 747 and set(details) == {r[2] for r in rows}, "detail cohort")
    require(len(re.findall(r"^Test Passed\.$", last, re.M)) == 746, "detail passed")
    require(not re.search(r"^Test Failed\.$", last, re.M), "detail failure")
    require("Start testing: Oct 08 17:43 UTC" in last and "End testing: Oct 08 17:53 UTC" in last, "closure")
    cache = raw["build_v12_u21/CMakeCache.txt"]
    for line in ["CMAKE_BUILD_TYPE:STRING=Release", "MHGP12_COORD_BITS:STRING=21",
                 "MHGP12_ENABLE_CUDA:BOOL=OFF", "CMAKE_HOME_DIRECTORY:INTERNAL=/workspaces/E-HGP/morsehgp3D_v12"]:
        require(line in cache.splitlines(), "configuration: " + line.split(":", 1)[0])

    scopes = ["morsehgp3D_v12/src", "morsehgp3D_v12/tests/tower", "morsehgp3D_v12/tests/mutants/tower.json"]
    pin = capture["retrait_git"]
    paths = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", pin, "--", *scopes], cwd=repo).decode().splitlines()
    entries = []
    for path in paths:
        content = subprocess.check_output(["git", "show", pin + ":" + path], cwd=repo)
        entries.append(hashlib.sha256(content).hexdigest() + "  " + path + "\n")
    require(len(paths) == 163, "source cohort")
    require(hashlib.sha256("".join(entries).encode()).hexdigest() == capture["source_inventory_sha256"], "source inventory")
    gates = sorted(name for name in passed if name.startswith("mhgp12_tower_region_") or
                   name.startswith("mhgp12_tower_pipeline_terminaison"))
    result = {
        "schema": "a6b-retrait-ctest-result-v1",
        "retrait_git": pin,
        "traces_primaire_fermees": len(raw),
        "sources_raccordees": len(paths),
        "selected": 747,
        "passed": len(passed),
        "skipped": skipped,
        "failed": 0,
        "driver_ctest_code": 0,
        "wall_seconds": 587.74,
        "configuration": capture["configuration"],
        "portes_terminaison_et_carte": gates,
        "elf_qualifies_par_ce_recu": [],
        "nouveaux_mutants_qualifies": 0,
        "natif_execute_par_auditeur": False,
    }
    rendered = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    expected = here / "results.json"
    if expected.exists():
        require(expected.read_text() == rendered, "result mismatch")
    print(rendered, end="")


if __name__ == "__main__":
    main()
