"""Read the sealed audit captures without compiling or running C++ code."""

from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path


PACKET = Path(__file__).resolve().parent
ROOT = PACKET.parents[2]
DEPENDENCY = ROOT / "morsehgp3D_v7/receipts/atlas_graph_full_20260911"
DEPENDENCY_SHA = "bb4a482385a7f75537c38d41397d4410d02c04f2a899c6377df79515caf0f9bc"
PARENT_SHA = "341c8c228a9d008084010db7b010adac77beefba123fa8a59d1d7c9023652013"
FILES = {"README.md", "historical_export_gate.cpp", "record.py", "verify.py",
         "o2.json", "san.json", "result.json"}


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def need(value: bool, why: str) -> None:
    if not value:
        raise SystemExit(why)


def main() -> None:
    seals = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, separator, name = line.partition("  ")
        need(separator == "  " and name in FILES and name not in seals, "invalid seal")
        need(sha((PACKET / name).read_bytes()) == digest, "changed receipt: " + name)
        seals[name] = digest
    need(set(seals) == FILES, "incomplete seal")
    need(sha((DEPENDENCY / "MANIFEST.json").read_bytes()) == DEPENDENCY_SHA, "dependency changed")
    spec = importlib.util.spec_from_file_location("historical_export_parent", DEPENDENCY / "verify.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    parent = module.Reader(DEPENDENCY)
    parent_result = parent.verify()
    need(parent_result["status"] == "passed", "parent reader failed")
    result = json.loads((PACKET / "result.json").read_text())
    modes = []
    for mode, filename in [("O2", "o2.json"), ("SAN", "san.json")]:
        capture = json.loads((PACKET / filename).read_text())
        need(capture["status"] == "completed" and capture["mode"] == mode and
             capture["dependency_manifest"] == DEPENDENCY_SHA and
             capture["dependency_parent_manifest"] == PARENT_SHA and
             capture["gcp_used"] is False and capture["performance_claim"] is False, "capture scope")
        own = {str((PACKET / name).relative_to(ROOT)): sha((PACKET / name).read_bytes())
               for name in ["historical_export_gate.cpp", "record.py"]}
        need(capture["own_sources_before"] == own == capture["own_sources_after"], "audit source differs")
        need(capture["project_sources_before"] == capture["project_sources_after"] and
             len(capture["project_sources_before"]) == 42, "project closure changed or vacuous")
        for logical, digest in capture["project_sources_before"].items():
            raw = ((PACKET / "historical_export_gate.cpp").read_bytes()
                   if logical == "audit/historical_export_gate.cpp" else parent.bytes(logical))
            need(sha(raw) == digest, "consumed source differs: " + logical)
        need(capture["binary_sha256_before"] == capture["binary_sha256_after"] and
             len(capture["binary_sha256_before"]) == 64 and
             capture["compiler_sha256_before"] == capture["compiler_sha256_after"] and
             len(capture["compiler_sha256_before"]) == 64, "binary or compiler stability")
        commands = capture["commands"]
        names = ["extract_dependency", "compiler", "dependencies_before", "compile", "selftest",
                 "unknown", "missing", "dependencies_after"]
        need([c["name"] for c in commands] == names, "command inventory")
        for command, expected in zip(commands, [0, 0, 0, 0, 0, 2, 2, 0]):
            need(command["expected_code"] == command["exit_code"] == expected, "command code")
        by_name = {c["name"]: c for c in commands}
        out = Path(capture["out"])
        historical_root = Path(capture["cwd"])
        need(out.is_relative_to(historical_root / "morsehgp3D_v7/audits"), "historical output scope")
        historical_gate = historical_root / "morsehgp3D_v7/audits" / PACKET.name / "historical_export_gate.cpp"
        dep = out / "dependency"
        cpp = dep / "build/v7_atlas_graph_20260911"
        graph = dep / "build/v7_graph_full_20260911/graph_full.hpp"
        compiler = by_name["compiler"]["argv"][0]
        need(by_name["compiler"]["argv"] == [compiler, "--version"] and
             by_name["compiler"]["stdout"] and by_name["compiler"]["stderr"] == "", "compiler capture")
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"]
        flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                   "-fno-pie", "-no-pie"] if mode == "SAN" else ["-O2"])
        flags += ["-I", str(cpp), "-I", str(cpp / "source/morsehgp3D_v7"),
                  '-DMHGP7_AUDIT_GRAPH_FULL_HEADER="' + str(graph) + '"']
        compile_base = [compiler, *flags, str(historical_gate)]
        binary = str(out / "historical_export_gate")
        need(by_name["compile"]["argv"] == [*compile_base, "-o", binary] and
             by_name["compile"]["stdout"] == by_name["compile"]["stderr"] == "", "strict compilation")
        for name in ["dependencies_before", "dependencies_after"]:
            need(by_name[name]["argv"] == [*compile_base, "-MM", "-MT", "audit_gate"] and
                 by_name[name]["stderr"] == "", "dependency command")
        need(by_name["dependencies_before"]["stdout"] == by_name["dependencies_after"]["stdout"],
             "dependency paths changed")
        for name, arguments in [("selftest", ["--selftest"]), ("unknown", ["--unknown"]), ("missing", [])]:
            need(by_name[name]["argv"] == [binary, *arguments], "gate argv")
        need(by_name["unknown"]["stdout"] == by_name["unknown"]["stderr"] == "" and
             by_name["missing"]["stdout"] == by_name["missing"]["stderr"] == "", "CLI diagnostic differs")
        expected_progress = "".join("case=" + fixture + " variant=" + str(variant) + "\n"
            for fixture in ["line3", "square4", "line5_equivalent_raw_metadata", "spatial8", "spatial16"]
            for variant in [0, 1])
        need(json.loads(by_name["selftest"]["stdout"]) == result and
             by_name["selftest"]["stderr"] == expected_progress, "selftest result or diagnostics")
        need(capture["environment"]["TMPDIR"] == str(out / "tmp"), "temporary scope")
        if mode == "SAN":
            need(capture["environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
                 capture["environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "SAN environment")
        extracted = json.loads(by_name["extract_dependency"]["stdout"])
        need(extracted["status"] == "passed" and extracted["standalone_source_tree"] is True and
             extracted["extracted_to"] == str(dep), "dependency extraction")
        modes.append(capture)
    need(modes[0]["project_sources_before"] == modes[1]["project_sources_before"], "O2/SAN source differs")
    need(modes[0]["commands"][4]["stdout"] == modes[1]["commands"][4]["stdout"], "O2/SAN output differs")
    for name, expected in {"runs": 10, "orders": 60, "census_balls": 1384, "normal_export_block_queries": 2704,
                           "nodes": 2184, "contributions": 1390, "verticals": 2056, "population_rows": 1388,
                           "silent_blocks": 1386, "silent_min_contributing_groups": 0,
                           "differing_global_raw_lots": 2, "differing_history_node_orders": 2,
                           "equivalent_raw_metadata_rescalings": 2, "abstract_causal_witnesses": 4}.items():
        need(result[name] == expected, "counter/non-vacuity differs: " + name)
    need(result["real_mutant_rejections"] == [10, 0, 2, 2] and result["real_mutant_causes"] == [
        "physical.bank_rows", "", "physical.raw_node", "physical.raw_contribution"] and
        result["mutant_order"] == ["bank_ball_id", "exclude_silent", "global_raw", "history_node_order"],
        "causal mutant results differ")
    need(result["status"] == "passed" and result["scope"] == "bounded_Builder83f1_physical_payload" and
         result["abstract_geometry_claimed"] is False and result["parallel_backend"] is False and
         result["gcp_used"] is False and result["vertical_method"] ==
         "pinned graph_full export then explicit ID transport", "test scope differs")
    print(json.dumps({"status": "passed_historical_export_receipt", "modes": ["O2", "SAN"],
                      "project_sources": 42, "runs_per_mode": 10, "orders_per_mode": 60,
                      "nodes_per_mode": 2184, "real_mutant_rejections_per_mode": [10, 0, 2, 2],
                      "silent_minimum_witness": "abstract_only", "parallel_backend": False,
                      "performance_claim": False, "public_status": "not_claimed", "gcp_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
