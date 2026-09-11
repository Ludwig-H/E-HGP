#!/usr/bin/env python3
"""Portable receipt reader only: no compilation, subprocess, Git or cloud."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath

BASE = Path(__file__).resolve().parent
RUN_PINS = {
    "o2_r1": "4c9ceaf37ed65bf0b45b6253a9b233838b6656b75879f1ad2cf50b14aa9c3cd7",
    "o2_r2": "c4aa9de0e0b7ef1c0c8138b5ef24b3c77b9041128b3c6526b356426095d79997",
    "san_root_r1": "8b94fb7e966215fbbe9c4bbaf23fe95176bb747d3d8668b9796b6f17bcb6d099",
    "chains_o2_root_r1": "b987561b281b31807448090d147ab91daaae4b64446623e5b04d7eb1ce5af5b9",
    "chains_san_root_r1": "6478a337245677ca2ff179ec51e89de6e5d289cfa72edfc3af4f7011d4dc5ea4",
}
SOURCE_PINS = {
    "filtered_calendar.hpp": "1cf438cbac839c8f502de0c8daebbf89296a3884d2d40ba50715b5aaca161fa2",
    "gate.cpp": "e0eb36f14906d4d6680beae6ca7a776e014ea46d917bafb495ad96cfe98725b7",
    "historical_chains.hpp": "686c2137a905e4f77c15a1d7f497541e6f8d87b9217f5c9c726cf4951be573c3",
    "historical_chains_gate.cpp": "f405cda33256a456446010c5779b1381e18635c1989d3af86a5333104a928ea3",
    "record.py": "1e775b3943c310399c177204525ada232d9460a55eba29823e2148048e1e156f",
    "record_chains.py": "1021050b256fdafda9ca68974911d76df111b75f2c9eb97cc41c1aa121241931",
    "README.md": "5b975644306b8f3211a4f1e14f95c0fb94d166fafbbf20ea0ac484efc46369f3",
}
SAN_ENV = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    path = PurePosixPath(name)
    need(isinstance(name, str) and not path.is_absolute() and path.parts
         and ".." not in path.parts and str(path) == name and "\\" not in name,
         "unsafe path")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--extract", type=Path)
    args = parser.parse_args()
    manifest = json.loads((BASE / "MANIFEST.json").read_text())
    need(manifest["schema"] == "filtered_calendar_packet_v1", "schema")
    need(manifest["captures"] == list(RUN_PINS), "five exact captures")
    for field in ("geometry_qualified", "performance_claim", "GCP_used"):
        need(manifest[field] is False, "scope: " + field)
    physical, logical = manifest["physical_files"], manifest["logical_files"]
    actual = set()
    for path in BASE.rglob("*"):
        need(not path.is_symlink(), "no symlinks")
        if path.is_file():
            actual.add(path.relative_to(BASE).as_posix())
    need(actual == set(physical) | {"MANIFEST.json"}, "exact physical inventory")
    for name, row in physical.items():
        data = (BASE / safe(name)).read_bytes()
        need(sha(data) == row["sha256"] and len(data) == row["size"], "physical bytes: " + name)
        need(not data.startswith(b"\x7fELF"), "no ELF")
    for name, row in logical.items():
        safe(name)
        need(row["path"] in physical and {k: row[k] for k in ("sha256", "size")} == physical[row["path"]],
             "logical mapping: " + name)
    need({row["path"] for row in logical.values()} == set(physical), "no orphan physical file")

    def data(name):
        return (BASE / logical[name]["path"]).read_bytes()

    def obj(name):
        return json.loads(data(name))

    for name, pin in SOURCE_PINS.items():
        need(sha(data("sources/" + name)) == pin, "qualified source: " + name)
    need(len(manifest["omitted"]) == 5, "five ELF omissions only")
    omitted = {row["logical_path"]: row for row in manifest["omitted"]}
    need(len(omitted) == 5 and all(row["reason"] == "ELF" and row["size"] > 0 for row in omitted.values()), "ELF declarations")
    summaries, command_count = {}, 0
    for run, pin in RUN_PINS.items():
        prefix = "captures/" + run + "/"
        need(sha(data(prefix + "receipt.json")) == pin, "receipt pin: " + run)
        receipt = obj(prefix + "receipt.json")
        chains, san = run.startswith("chains_"), "san" in run
        need(receipt["status"] == "passed" and receipt["error"] is None and receipt["source_stable"] is True, "closed run")
        need(receipt["geometry_qualified"] is False and receipt["gcp_used" if chains else "GCP_used"] is False, "run scope")
        need(receipt["sanitizer"] is san and receipt["sanitizer_environment"] == (SAN_ENV if san else {}), "SAN environment")
        need(omitted[prefix + "gate"]["sha256"] == receipt["binary_sha256"], "ELF attribution")
        if chains:
            before = receipt["sources_before"]
            need(before == receipt["sources_after"] == receipt["sources_copied"], "chains source stability")
        else:
            before = obj(prefix + "sources_before.json")
            need(before == obj(prefix + "sources_after.json"), "calendar source stability")
        expected_sources = ({"filtered_calendar.hpp", "historical_chains.hpp", "historical_chains_gate.cpp", "record_chains.py"}
                            if chains else {"filtered_calendar.hpp", "gate.cpp", "README.md", "record.py"})
        need(set(before) == expected_sources, "exact source closure")
        for name, source_pin in before.items():
            need(sha(data(prefix + name)) == source_pin, "copied source: " + run + "/" + name)
            if run != "o2_r1" or name not in ("gate.cpp", "README.md"):
                need(source_pin == SOURCE_PINS[name], "current source attribution")
        if run == "o2_r1":
            need(before["gate.cpp"] == "7556b08eb59ba25b0136d81b3af0a21d91f3025238ad16e21efff27a4d161ca8", "historical gate pin")
            need(before["README.md"] != SOURCE_PINS["README.md"], "historical note distinct")
        commands = receipt["commands"]
        expected = [("compiler", 0), ("compile", 0), ("selftest", 0)] if chains else [("compile", 0), ("selftest", 0), ("unknown", 2), ("missing", 2)]
        need([(row["name"], row["exit_code"]) for row in commands] == expected, "exact process outcomes")
        command_count += len(commands)
        for row in commands:
            for channel in ("stdout", "stderr"):
                payload = data(prefix + row["name"] + "." + channel)
                need(sha(payload) == row[channel + "_sha256"], "stream hash")
                if channel == "stderr" or row["name"] not in ("compiler", "selftest"):
                    need(not payload, "no diagnostics/unexpected payload")
            if not chains:
                need(obj(prefix + row["name"] + ".command.json") == row and row["expected_exit"] == row["exit_code"], "command record")
        compile_row = next(row for row in commands if row["name"] == "compile")
        flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"] + (["-pthread"] if chains else [])
        flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] + (["-fno-pie", "-no-pie"] if chains else [])) if san else ["-O2"]
        tu = "historical_chains_gate.cpp" if chains else "gate.cpp"
        argv = compile_row["argv"]
        need(argv[:-3] == flags and argv[-3].endswith("/" + run + "/" + tu) and argv[-2] == "-o" and argv[-1].endswith("/" + run + "/gate"), "compiled snapshot/strict flags")
        summary = obj(prefix + "selftest.stdout")
        need(summary["status"] == "passed", "gate success")
        summaries[run] = summary
    need(command_count == 18, "command nonvacuity")
    need(summaries["o2_r2"] == summaries["san_root_r1"], "calendar O2/SAN identical")
    need(summaries["chains_o2_root_r1"] == summaries["chains_san_root_r1"], "HLD O2/SAN identical")
    for run in ("o2_r1", "o2_r2", "san_root_r1"):
        row = summaries[run]
        for key, value in {"scope": "abstract_native_birth_filtered_graph", "cases": 264, "cuts": 6570,
                           "mark_reads": 138880, "input_edges": 7892, "forest_edges": 2378, "rejections": 17,
                           "checks": 468473 if run == "o2_r1" else 474524}.items():
            need(row[key] == value, "calendar exact nonvacuity: " + key)
        need(row["geometry_qualified"] is False and row["performance_claim"] is False and row["GCP_used"] is False, "calendar scope")
    for key, value in {"canonical_pairs": 128, "calendar_mutants": 2, "alternative_msf": 1, "rank_instantiations": 1,
                       "events": 3937, "native_births": 2650, "multifusions_ge3": 617, "multiple_events_same_date": 1065}.items():
        need(summaries["o2_r2"][key] == value, "r2 strengthening")
    hld = summaries["chains_o2_root_r1"]
    for key, value in {"checks": 922537, "queries": 307500, "rejected": 9, "cases": 5, "workers": [1, 2, 4],
                       "inactive": 76902, "light_steps": 159780, "binary_steps": 665319, "maximum_nodes": 8191,
                       "logical_index_bytes_total": 245928, "public_status": "not_claimed"}.items():
        need(hld[key] == value, "HLD exact nonvacuity: " + key)
    need(hld["construction_parallel"] is False and hld["batch_parallel"] is True and hld["geometry_tested"] is False and hld["gcp_used"] is False, "HLD scope")
    failure = obj("history/initial_chains_compile_failure.json")
    need(failure["status"] == "failed_historical_compile" and failure["completion"]["exit_code"] == 1
         and failure["capture_kind"] == "exec_tool_combined_output_not_separate_stdout_stderr"
         and "-Werror=sign-compare" in failure["first_tool_result"]["output"], "historical compile failure preserved")
    if args.extract:
        args.extract.mkdir(parents=True, exist_ok=False)
        for name in sorted(actual):
            target = args.extract / safe(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("xb") as stream:
                stream.write((BASE / name).read_bytes())
    print(json.dumps({"status": "passed", "captures": 5, "commands": command_count,
                      "logical_files": len(logical), "physical_files": len(physical), "ELF_omitted": 5,
                      "calendar_checks_current": 474524, "historical_checks": 922537,
                      "historical_queries": 307500, "historical_api_refusals": 8, "historical_semantic_mutants": 1,
                      "geometry_qualified": False, "performance_claim": False, "GCP_used": False,
                      "manifest_sha256": sha((BASE / "MANIFEST.json").read_bytes())}))


if __name__ == "__main__":
    main()
