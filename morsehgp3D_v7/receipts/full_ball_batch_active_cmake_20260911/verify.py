#!/usr/bin/env python3
"""Portable read-only active Release evidence, distinct from prior private SAN."""
import argparse
import hashlib
import json
from pathlib import Path
import posixpath
import re
import shlex
import xml.etree.ElementTree as ET

BASE = Path(__file__).resolve().parent
CAPTURE_PIN = "aaf5d7b5d74e293416d4e9e1d00e152eb1ead931c180665771ab68dfe8f4e60a"
HEADER = "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366"
IMPORTS = {
    "census": "f26e11a64ac44c6cc8f617b7e3b670b86fe75427a3aad5370705e8a98114f953",
    "callback": "6396a594daeab25f409e1f2a5ce812b29ec7bc599e4d7328632f02106533365c",
}
FILES = {"census": ["census_tower_gate.cpp", "census_tower_oracle.hpp"],
         "callback": ["full_ball_batch_gate.cpp", "full_ball_batch_geometry.hpp", "full_ball_batch_owner.hpp"]}
TARGETS = ["mhgp7_anchor_meb_gate", "mhgp7_full_ball_tower_gate", "mhgp7_full_ball_work_gate",
    "mhgp7_full_ball_tower_probe", "mhgp7_full_coverage_certificate_gate", "mhgp7_local_plateau_gate",
    "mhgp7_census_route_stub_gate", "mhgp7_full_gabriel_digest_gate", "mhgp7_facet_resolver_cache_gate",
    "mhgp7_witness_front_gate", "mhgp7_census_tower_gate", "mhgp7_full_ball_batch_gate"]
TIMES = {"index_s", "generate_s", "sort_s", "prefilter_s", "census_s", "tower_s", "digest_s", "total_s"}
CAPACITIES = {"static_worker_capacity_peak_bytes", "static_sampled_retained_capacity_peak_bytes"}


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    path = Path(name)
    need(not path.is_absolute() and ".." not in path.parts and path.as_posix() == name and name not in ("", "."), "safe relative path")
    return path


def qualify(read, captured):
    need(captured["scope"] == "active_Release_CPU83f1_40_CTests" and captured["public_status"] == "not_claimed" and
         captured["GCP_used"] is False and captured["device_executed"] is False, "bounded active scope")
    obj = lambda name: json.loads(read("active/" + name))
    receipt = obj("receipt.json")
    need(receipt["status"] == "passed" and receipt["error"] is None and receipt["commands"] == 23 and
         receipt["selected_CTests"] == 40 and receipt["build_parallelism"] == 1 and
         receipt["sources_and_dependencies_stable"] is True and receipt["cuda_enabled"] is False and
         receipt["GCP_used"] is False and receipt["probe_latency_claimed"] is False, "closed active capture")
    before, after = obj("sources_before.json"), obj("sources_after.json")
    need(before == after and len(before) == receipt["sources"] and len(before) >= 178, "own source closure")
    for name, pin in before.items():
        need(sha(read("active/source/" + name)) == pin, "captured own source bytes")
    need(before["morsehgp3D_v7/src/forest/full_ball_tower.hpp"] == HEADER, "active header pin")
    for name, pin in receipt["active_source_pins"].items():
        need(before["morsehgp3D_v7/" + name] == pin, "declared active pins")
    dependencies = obj("dependencies_before.json")
    need(dependencies == obj("dependencies_after.json") and len(dependencies) == receipt["used_dependencies"], "dependency closure")
    workspace = "/workspaces/E-HGP/"
    for name, pin in dependencies.items():
        if name.startswith(workspace) and name[len(workspace):] in before:
            need(before[name[len(workspace):]] == pin, "consumed own dependency bound to source")
    binaries = obj("binaries_before.json")
    need(binaries == obj("binaries_after.json") == receipt["binary_sha256"] and set(binaries) == set(TARGETS), "twelve ELF hashes only")
    commands = obj("commands.json")
    order = ["compiler_version", "cmake_version", "ctest_version", "compiler_frontend", "configure"] + \
        ["dependencies_" + name for name in TARGETS] + ["build", "ctest_list", "ctest", "worker_normal", "worker_optimized", "probe_200_static1"]
    need([row["name"] for row in commands] == order and len(commands) == 23, "all 23 sequential commands")
    prior = 0
    for row in commands:
        need(row["exit_code"] == row["expected_exit"] == 0 and prior <= row["started_ns"] <= row["ended_ns"], "command exact success and sequence")
        prior = row["ended_ns"]
        need(obj(row["name"] + ".command.json") == row, "command record identity")
        intent = obj(row["name"] + ".intent.json")
        need(intent["argv"] == row["argv"] and intent["expected_exit"] == 0 and intent["cwd"] == workspace.rstrip("/"), "command intent")
        for stream in ("stdout", "stderr"):
            need(sha(read("active/" + row["name"] + "." + stream)) == row[stream + "_sha256"], "stream pin")
    byname = {row["name"]: row for row in commands}
    configure = byname["configure"]["argv"]
    need("-DMHGP7_ENABLE_CUDA=OFF" in configure and "-DCMAKE_BUILD_TYPE=Release" in configure and
         configure[configure.index("-S") + 1] == workspace + "morsehgp3D_v7", "active Release CPU configuration")
    build = byname["build"]["argv"]
    need(build[build.index("--parallel") + 1] == "1" and build[build.index("--target") + 1:] == TARGETS, "exact twelve targets single compiler")
    selected = obj("selected_compile_commands.json")
    all_commands = obj("compile_commands.json")
    need(len(selected) == 12 and all(row in all_commands for row in selected), "selected actual compile commands")
    for target, row in zip(TARGETS, selected):
        args = shlex.split(row["command"])
        need("CMakeFiles/" + target + ".dir/" in row["command"] and all(flag in args for flag in
             ("-O3", "-DNDEBUG", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror")), "strict Release target flags")
        need(row["file"].startswith(workspace) and row["file"][len(workspace):] in before, "compiled TU own snapshot")
        dep_args = list(args)
        oi = dep_args.index("-o")
        del dep_args[oi:oi + 2]
        dep_args.remove("-c")
        dep_args.insert(1, "-M")
        need(dep_args == byname["dependencies_" + target]["argv"], "dependency scan same compile recipe")
        raw = read("active/dependencies_" + target + ".stdout").decode().replace("\\\n", " ")
        for name in shlex.split(raw.split(":", 1)[1]):
            full = name if name.startswith("/") else workspace + name
            # gcc -M emits normalized absolute paths for this capture.
            normalized = posixpath.normpath(full)
            need(normalized in dependencies, "every consumed dependency pinned")
        if target in ("mhgp7_census_tower_gate", "mhgp7_full_ball_batch_gate"):
            need(not any("MHGP7_TESTING" in arg for arg in args), "new tests have no product instrumentation")
    tests = obj("selected_tests.json")
    historical = re.findall(r"Test\s+#\d+:\s+(mhgp7_\w+)", read("references/historical_24.stdout").decode())
    need(sha(read("references/historical_24.stdout")) == "5ca6cba59109950dfba3651faf56564e266826029dc17d329306a89b4cfdf8e1", "historical 24 selection pin only")
    new_census = ["mhgp7_census_tower_" + kind for kind in ("historical", "line12", "shell14", "spatial12", "rejects",
        "bad_argument", "missing_argument", "mutant_assignment", "mutant_open", "mutant_adjacency", "mutant_census")]
    new_batch = ["mhgp7_full_ball_batch_" + kind for kind in ("cpu1", "cpu4", "rejects", "bad_argument", "missing_argument")]
    need(len(historical) == 24 and len(tests) == len(set(tests)) == 40 and set(tests) == set(historical + new_census + new_batch), "24 historical plus 16 newly active")
    inventory = obj("ctest_list.stdout")["tests"]
    need([test["name"] for test in inventory] == tests, "actual selected CTest inventory")
    xml = ET.fromstring(read("active/ctest.junit.xml"))
    cases = xml.findall("testcase")
    need(xml.attrib["tests"] == "40" and xml.attrib["failures"] == xml.attrib["skipped"] == "0" and
         {case.attrib["name"] for case in cases} == set(tests) and len(cases) == 40 and
         all(case.attrib["status"] == "run" and case.find("failure") is None for case in cases), "forty actual JUnit successes")
    outputs = {case.attrib["name"]: case.find("system-out").text or "" for case in cases}
    log = read("active/LastTest.log").decode()
    sections = re.split(r"^\d+/\d+ Testing: ", log, flags=re.MULTILINE)[1:]
    need(len(sections) == 40 and all("Test Passed." in section for section in sections), "forty physical CTest logs")
    for test, section in zip(tests, sections):
        need(section.startswith(test + "\n"), "physical CTest ordering")
    private_results = {}
    for name, pin in IMPORTS.items():
        prefix = "references/" + name + "/"
        manifest_bytes = read(prefix + "manifest.json")
        need(sha(manifest_bytes) == pin, "prior closed packet manifest")
        prior_manifest = json.loads(manifest_bytes)
        for logical in captured["reference_selections"][name]:
            data = read(prefix + logical)
            entry = prior_manifest["files"][logical]
            need(sha(data) == entry["sha256"] and len(data) == entry["size"], "selected prior original bytes")
        old_receipts = [json.loads(read(prefix + "captures/" + mode + "/receipt.json")) for mode in ("o2_r1", "san_root_r1")]
        need(all(r["status"] == "passed" and r["sources_stable"] and r["snapshot_stable"] for r in old_receipts) and
             old_receipts[0]["snapshot_before"] == old_receipts[1]["snapshot_before"] and
             old_receipts[0]["CTest_results"] == old_receipts[1]["CTest_results"], "prior private O2 SAN distinct closure")
        need(old_receipts[1]["sanitizer_environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1", "prior LSan active not inherited to Release")
        private_results[name] = old_receipts[0]["CTest_results"]
        for filename in FILES[name]:
            old = read(prefix + "sources/current/morsehgp3D_v7/tests/" + filename)
            source_key = "morsehgp3D_v7/tests/" + filename
            need(sha(old) == old_receipts[0]["snapshot_before"][source_key] and
                 read("active/source/" + source_key) == old + b"\n", "only one final newline added to qualified candidate")
    old_cmake = read("references/census/sources/current/proposal/baseline/CMakeLists.txt")
    anchor = b"mhgp7_product_executable(mhgp7_full_ball_work_gate tests/full_ball_work_gate.cpp)"
    fragments = b"".join(read("references/" + name + "/sources/current/proposal/CMake.fragment") for name in ("census", "callback"))
    need(old_cmake.count(anchor) == 1 and old_cmake.replace(anchor, fragments + anchor) ==
         read("active/source/morsehgp3D_v7/CMakeLists.txt"), "active CMake exact composition")
    def json_rows(output):
        return [json.loads(line) for line in output.splitlines() if line.startswith('{"status":')]
    actual_census = [row for name in new_census for row in json_rows(outputs[name])]
    actual_batch = [row for name in new_batch for row in json_rows(outputs[name])]
    need(actual_census == private_results["census"] and actual_batch == private_results["callback"], "new active physical observations equal private results")
    for name in ("mhgp7_full_ball_static_cpu1", "mhgp7_full_ball_static_cpu4"):
        rows = json_rows(outputs[name])
        need(len(rows) == 1 and rows[0]["clouds"] == 34 and rows[0]["orders"] == 150 and rows[0]["vertical_checks"] == 87230 and
             "post_seed_queries=20 post_seed_hits=4 post_seed_terminals=4" in outputs[name], "active static nonvacuity")
    a, b = obj("probe_200_static1.stdout"), obj("probe_O2_reference.stdout")
    need(sha(read("active/probe_O2_reference.stdout")) == "112010179a0539661bfa06636cd605ecf6b2b7d2b3646f1717d40668e71bb20c", "original clean O2 n200 observation pin")
    exclusions = obj("probe_comparison_exclusions.json")
    need(exclusions == {"timings": sorted(TIMES), "capacities": sorted(CAPACITIES),
         "reason": "FullBallStats_batch_fields_change_Worker_size_only"}, "exact two layout exclusions")
    ignored = TIMES | CAPACITIES
    need({k: v for k, v in a.items() if k not in ignored} == {k: v for k, v in b.items() if k not in ignored}, "all remaining n200 fields equal")
    q, h, t = (sum(row[key] for row in a["static_orders"]) for key in ("post_seed_queries", "post_seed_hits", "post_seed_terminals"))
    need((q, h, t, a["resolver_meb_calls"]) == (17419, 2945, 2945, 48618) and
         a["resolver_meb_calls"] == a["anchor_hits"] + a["intruder_queries"] and a["contract_qualified"] is False, "n200 actual work floor no timing claim")
    for name in ("worker_normal", "worker_optimized"):
        row = obj(name + ".stdout")
        need(row["status"] == "passed" and row["checks"] == 389 and row["CUDA_executed"] is False and
             row["GCP_used"] is False and row["subprocess_invoked"] is False, "worker format only")
    return {"status": "passed", "active_Release_CTests": 40, "active_commands": 23, "active_ELF_hashes_only": 12,
        "prior_private_CTests_O2_and_SAN_each": 16, "prior_SAN_not_inherited_by_active_Release": True,
        "only_final_newline_changed_candidates": 5, "probe_n": 200, "probe_layout_capacities_excluded": 2,
        "probe_M": 48618, "probe_Q": q, "probe_H": h, "probe_T": t,
        "callback_suite_cases": 65, "callback_actual_rejections": 64, "callback_positive_empty_case": 1,
        "source_header_sha256": HEADER, "own_sources": len(before), "used_dependency_pins": len(dependencies),
        "compiler_executed_here": False, "GCP_used": False, "public_status": "not_claimed", "latency_contract_qualified": False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--extract", type=Path)
    args = parser.parse_args()
    manifest = json.loads((BASE / "manifest.json").read_bytes())
    paths = [path for path in BASE.rglob("*") if path.is_file()]
    need(all(not path.is_symlink() for path in BASE.rglob("*")), "no symlink")
    need({p.relative_to(BASE).as_posix() for p in paths} == set(manifest["files"]) | {"manifest.json"}, "physical inventory")
    for name, entry in manifest["files"].items():
        data = (BASE / safe(name)).read_bytes()
        need(sha(data) == entry["sha256"] and len(data) == entry["size"] and data[:4] != b"\x7fELF" and
             not name.endswith((".pyc", ".pyo")), "physical pin no ELF or bytecode")
        data.decode("utf-8")
    raw = (BASE / "capture_manifest.json").read_bytes()
    need(sha(raw) == CAPTURE_PIN, "captured authority")
    captured = json.loads(raw)
    mapping = json.loads((BASE / "source_map.json").read_bytes())
    need(set(mapping) == set(captured["files"]), "logical inventory")
    def read(name):
        safe(name)
        entry = mapping[name]
        data = (BASE / safe(entry["storage"])).read_bytes()
        need({key: entry[key] for key in ("sha256", "size")} == captured["files"][name] and
             sha(data) == entry["sha256"] and len(data) == entry["size"], "logical pin")
        return data
    for name in mapping:
        read(name)
    report = qualify(read, captured)
    if args.extract:
        args.extract.mkdir(exist_ok=False)
        for name in mapping:
            target = args.extract / safe(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("xb") as output:
                output.write(read(name))
    report.update(logical_files=len(mapping), physical_files=len(paths))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
