#!/usr/bin/env python3
"""Create-only mechanical relocation of the strengthened T2 judge, no compile."""
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
SOURCE = REPO / "build/v7_t2_metadata_20260911/source"
CANDIDATE = ROOT / "candidate/morsehgp3D_v7/tests"
PINS = {"t2_gate.cpp": "13f06875da99ae8413b0e64115964979bc5afe9f0c6f7224a7cd4efe773883dc",
    "t2_oracle.hpp": "57b615240896f4631434421fa2360d8e5b87db41b0cfc74a28afa0bd0727939c"}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError("unexpected relocation source: " + old)
    return text.replace(old, new)


def main():
    if CANDIDATE.exists() or (ROOT / "origin.json").exists():
        raise RuntimeError("create-only candidate already exists")
    data = {}
    for name, pin in PINS.items():
        payload = (SOURCE / name).read_bytes()
        if sha(payload) != pin:
            raise RuntimeError("source judge changed")
        write(ROOT / "originals" / name, payload)
        data[name] = payload.decode()
    gate = data["t2_gate.cpp"]
    for old, new in (
        ("// Private bounded qualification. Product receives only the real census.",
         "// Permanent bounded census -> FULL qualification; product receives only the real census.\n"
         "// Derived from T2 13f06875: no product code or exhaustive oracle is moved into src/."),
        ('#include "t2_oracle.hpp"', '#include "census_tower_oracle.hpp"'),
        ('#define main inherited_bounded_gate_main', '#define main mhgp7_inherited_bounded_census_gate_main'),
        ('#define local_plateau_oracle t2_plateau_oracle', '#define local_plateau_oracle census_tower_oracle'),
        ('#include "source/morsehgp3D_v7/tests/full_ball_tower_gate.cpp"', '#include "full_ball_tower_gate.cpp"'),
        ('#include "source/morsehgp3D_v7/src/pipeline/generate.hpp"', '#include "../src/pipeline/generate.hpp"')):
        gate = replace_once(gate, old, new)
    oracle = replace_once(data["t2_oracle.hpp"],
        '#include "source/morsehgp3D_v7/oracle/local_plateau_oracle.hpp"', '#include "../oracle/local_plateau_oracle.hpp"')
    if oracle.count("mhgp7::t2_plateau_oracle") != 2:
        raise RuntimeError("oracle namespace occurrence changed")
    oracle = oracle.replace("mhgp7::t2_plateau_oracle", "mhgp7::census_tower_oracle")
    write(CANDIDATE / "census_tower_gate.cpp", gate.encode())
    write(CANDIDATE / "census_tower_oracle.hpp", oracle.encode())
    active = REPO / "morsehgp3D_v7"
    baseline = {}
    for name in ("CMakeLists.txt", "cmake/run_expect.cmake", "tests/full_ball_tower_gate.cpp",
                 "oracle/local_plateau_oracle.hpp", "src/forest/full_ball_tower.hpp"):
        payload = (active / name).read_bytes()
        baseline[name] = sha(payload)
        write(ROOT / "baseline" / name, payload)
    cmake = (active / "CMakeLists.txt").read_text()
    insertion = (ROOT / "CMake.fragment").read_text()
    anchor = "mhgp7_product_executable(mhgp7_full_ball_work_gate tests/full_ball_work_gate.cpp)"
    updated = replace_once(cmake, anchor, insertion + anchor)
    write(ROOT / "CMake.diff", "".join(difflib.unified_diff(cmake.splitlines(keepends=True), updated.splitlines(keepends=True),
        fromfile="a/morsehgp3D_v7/CMakeLists.txt", tofile="b/morsehgp3D_v7/CMakeLists.txt")).encode())
    write(ROOT / "origin.json", (json.dumps({"status": "prepared_not_compiled", "source_judge_pins": PINS,
        "baseline_source_pins": baseline, "product_source_commit": "b8336ad3f8deecb1fdb24545005cf33e7ded9faa",
        "product_header_sha256": baseline["src/forest/full_ball_tower.hpp"],
        "inherited_results": False, "gcp_used": False}, indent=2, sort_keys=True) + "\n").encode())
    print(json.dumps({"status": "prepared", "new_test_files": 2, "CTest_cases": 11, "compiled": False}))


if __name__ == "__main__":
    main()
