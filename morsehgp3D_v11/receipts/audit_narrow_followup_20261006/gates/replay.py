#!/usr/bin/env python3
"""Source-only replay; real Builder uses an intercepted call, never CMake/native."""
import argparse
import ast
import copy
import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess
from types import SimpleNamespace


HERE = Path(__file__).resolve().parent
IDS = ("etendue_seuil_double", "etendue_sans_fermeture")
ROOT = "morsehgp3D_v11/"


def need(value, message):
    if not value:
        raise ValueError(message)


def build():
    manifest = json.loads((HERE / "source_manifest.json").read_text())
    pin = manifest["pin"]
    texts = {}

    def source(path):
        if path not in texts:
            raw = subprocess.check_output(["git", "show", pin + ":" + path])
            if path in manifest["sources"]:
                need(hashlib.sha256(raw).hexdigest() == manifest["sources"][path]["sha256"], path + " hash")
            texts[path] = raw.decode()
        return texts[path]

    for path in manifest["sources"]:
        source(path)
    catalogue_text = source(ROOT + "tests/mutants/catalogue.json")
    catalogue = json.loads(catalogue_text)
    tower = json.loads(source(ROOT + "tests/mutants/tower.json"))
    pattern_counts = {}
    for name, module in (("catalogue", catalogue), ("tower", tower)):
        primary = extra = 0
        for mutant in module["mutants"]:
            changed = {}
            for index, edit in enumerate([mutant] + mutant.get("aussi", [])):
                path = ROOT + edit["fichier"]
                text = changed.get(path, source(path))
                need(text.count(edit["cherche"]) == 1, name + ": unique anchor " + mutant["id"])
                changed[path] = text.replace(edit["cherche"], edit["remplace"])
                primary += index == 0
                extra += index != 0
        need(primary == module["plancher"], "unchanged module floor")
        pattern_counts[name] = {"primary": primary, "additional": extra, "all_unique": True}
    need(sum(x["primary"] for x in pattern_counts.values()) == 223, "223 primary anchors")
    configs = json.loads(source(ROOT + "tools/g4_matrix.json"))["configurations"]
    mutants = [c for c in configs if c["name"] == "mutants"]
    need(len(mutants) == 1, "one official mutant configuration")
    base_options = mutants[0]["cmake_options"]
    need("-DMHGP11_COORD_BITS=18" in base_options, "official base u18")
    cmake = source(ROOT + "CMakeLists.txt")
    need("-DMHGP11_COORD_BITS=${MHGP11_COORD_BITS}" in cmake and
         "list(APPEND mutant_args --cmake-arg=${definition})" in cmake, "profile forwarded to launcher")
    leaf = source(ROOT + "src/catalogue/leaf_device.hpp")
    test = source(ROOT + "tests/catalogue/leaf_device_narrow_test.cpp")
    need("if constexpr (kBits > 20)\n      wide =" in leaf, "production compile-time guard")
    need("if constexpr (kCoordBits > 20) {" in test, "test compile-time guard")
    need("CHECK_EQ(wide.counts.prefixes, 0u);" in test, "refusal before prefixes checked")
    need("CHECK_EQ(by_closure.status, ld::kUnresolved);" in test, "closure refusal checked")
    need("(u64(leaf.wide) - 1)" in leaf, "wide suppresses initial prefixes")
    runner = source(ROOT + "tests/mutants/run_mutants.py")
    need("self.args.cmake_arg + list(options)" in runner, "local options appended after base")
    need("problem = witness(builder, source, work, str(label), options, gates)" in runner,
         "same local options for witness")
    corrected = copy.deepcopy(catalogue)
    changed_text = catalogue_text
    for name in IDS:
        rows = [m for m in corrected["mutants"] if m["id"] == name]
        need(len(rows) == 1 and not rows[0].get("options"), "local profile initially absent")
        rows[0]["options"] = ["-DMHGP11_COORD_BITS=21"]
        marker = '"id": "' + name + '",'
        need(changed_text.count(marker) == 1, "patch insertion unique")
        changed_text = changed_text.replace(marker, marker + '\n      "options": ["-DMHGP11_COORD_BITS=21"],')
    need(json.loads(changed_text) == corrected, "only two option lists added")
    patch = "".join(difflib.unified_diff(catalogue_text.splitlines(True), changed_text.splitlines(True),
                                       fromfile="a/" + ROOT + "tests/mutants/catalogue.json",
                                       tofile="b/" + ROOT + "tests/mutants/catalogue.json"))
    need(patch == (HERE / "proposal.patch").read_text(), "exact proposal")
    calls = []

    def intercepted(argv, timeout):
        calls.append(list(argv))
        return 0, "intercepted; no process launched"

    namespace = {"os": os, "call": intercepted}
    tree = ast.parse(runner)
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id in
                                               ("CONFIGURE_TIMEOUT", "BUILD_TIMEOUT") for t in node.targets):
            exec(compile(ast.Module(body=[node], type_ignores=[]), "runner_constants", "exec"), namespace)
    classes = [n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == "Builder"]
    need(len(classes) == 1, "one exact Builder")
    exec(compile(ast.Module(body=classes, type_ignores=[]), "actual_Builder", "exec"), namespace)
    args = SimpleNamespace(cmake="cmake", generator=None, cmake_arg=["-DMHGP11_COORD_BITS=18"])
    builder = namespace["Builder"](args, ["catalogue"])
    configurations = {}
    for name in IDS:
        row = next(m for m in corrected["mutants"] if m["id"] == name)
        before = len(calls)
        need(builder.configure_and_build("fixture", row["options"], 1) == (None,
             "intercepted; no process launchedintercepted; no process launched"), "real Builder completed with stub")
        need(len(calls) == before + 2, "configure/build intercepted")
        definitions = [x for x in calls[before] if x.startswith("-DMHGP11_COORD_BITS=")]
        need(definitions == ["-DMHGP11_COORD_BITS=18", "-DMHGP11_COORD_BITS=21"], "u21 last")
        configurations[name] = {"profile_definitions": definitions, "last_definition": 21}
    span = 1 << 20
    boundary = {}
    for bits in (18, 21, 24):
        top = 1 << bits
        active = bits > 20
        if not active:
            boundary[str(bits)] = {"guard_compiled": False, "outside_span_tests_compiled": False,
                                   "maximum_envelope": top, "both_mutations_equivalent": True}
            need(top <= span, "u18 entire legal cube inside narrow span")
            continue
        far = top - 1 - span
        sites_low, sites_high = far, top - 1
        box_low, box_high = top - 3000, top
        full_span = max(sites_high, box_high) - min(sites_low, box_low)
        no_closure_span = max(sites_high, box_low) - min(sites_low, box_low)
        need(full_span == span + 1 and no_closure_span == span, "real test closure boundary")
        need(full_span > span and not full_span > 2 * span and not no_closure_span > span,
             "both altered guards admit the rejected boundary")
        boundary[str(bits)] = {"guard_compiled": True, "outside_span_tests_compiled": True,
                               "closure_fixture_span": full_span, "reference_wide": True,
                               "doubled_threshold_wide": False, "without_closure_wide": False,
                               "both_mutations_equivalent": False}
    return {"pin": pin, "native_runs": 0, "cmake_runs": 0, "cloud_actions": 0,
            "kind": "source equivalence and intercepted exact Builder, not CTest execution",
            "official_mutant_profile": 18, "targeted_local_profile": 21,
            "mutants": list(IDS), "floors_unchanged": {"catalogue": 72, "tower": 151},
            "anchors": pattern_counts, "configurations_after_proposal": configurations,
            "profile_boundaries": boundary}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--emit", action="store_true")
    args = parser.parse_args()
    proof = build()
    if args.emit:
        print(json.dumps(proof, indent=2, sort_keys=True))
    else:
        need(proof == json.loads((HERE / "proof.json").read_text()), "frozen proof unchanged")
        print("narrow_gate_source_verdict conforme profiles3 mutants2 anchors223 native0 cmake0 cloud0")


if __name__ == "__main__":
    main()
