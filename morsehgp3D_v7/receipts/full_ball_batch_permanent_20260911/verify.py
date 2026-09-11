#!/usr/bin/env python3
"""Read-only portable CMake/CTest evidence reader; no geometry or compiler."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import shlex

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "149082a432a765bcd89b045efecdf4b9e39c0f6f4b771edcd44a7ed913057270"
HEADER = "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366"
TARGET = "mhgp7_full_ball_batch_gate"
TARGET_DIR = "cmake_build/CMakeFiles/" + TARGET + ".dir/"
KINDS = ["cpu1", "cpu4", "rejects", "bad_argument", "missing_argument"]
CANDIDATES = ["full_ball_batch_gate.cpp", "full_ball_batch_geometry.hpp", "full_ball_batch_owner.hpp"]
NOMINAL = {"batch_calls": 49, "batch_requests": 103, "direct_terminals": 103,
           "diagnostic_reference_MEB_calls": 119, "payload_checks": 5704, "work_checks": 442,
           "product_instrumentation": False, "status": "passed_batch_callback"}
FULL = {"status": "passed", "checks": 298742, "clouds": 34, "orders": 150,
        "facets": 33870, "cuts": 3916, "vertical_checks": 87230, "same_radius_steps": 4,
        "authority": "bounded_independent_Gram_Gamma_not_WSPD_completeness"}



def need(good, reason):
    if not good:
        raise RuntimeError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    path = Path(name)
    need(not path.is_absolute() and ".." not in path.parts and str(path) not in ("", "."), "unsafe path")
    return path


def qualify(read, captured):
    need(captured["scope"] == "permanent_batch_CTest_CPU83f1" and
         captured["public_status"] == "not_claimed" and captured["device_executed"] is False and
         captured["gcp_used"] is False and captured["callback_supplied"] is True and captured["product_instrumentation"] is False, "scope")
    need(set(captured["captures"]) == {"o2_r1", "san_root_r1"}, "capture list")
    receipts, summaries, incidental_seen = {}, {}, set()
    for name in ("o2_r1", "san_root_r1"):
        prefix = "captures/" + name + "/"
        value = lambda relative: json.loads(read(prefix + relative))
        receipt = value("receipt.json")
        need(sha(read(prefix + "receipt.json")) == captured["captures"][name], "receipt pin")
        need(receipt["status"] == "passed" and receipt["sources_stable"] is True and
             receipt["snapshot_stable"] is True and receipt["baseline_before"] == receipt["baseline_after"] and
             receipt["proposal_before"] == receipt["proposal_after"] and receipt["snapshot_before"] == receipt["snapshot_after"] and
             receipt["product_header_sha256"] == HEADER and receipt["device_executed"] is False and
             receipt["gcp_used"] is False and receipt["compiler_jobs"] == receipt["CTest_jobs"] == 1, "closed capture")
        need(receipt["mode"] == ("san" if name.startswith("san") else "o2"), "capture mode")
        for source, pin in receipt["snapshot_before"].items():
            logical = prefix + "source_snapshot/" + source
            if source.endswith(".pyc"):
                entry = captured["incidental_pins_no_bytes"][logical]
                need("__pycache__" in Path(source).parts and entry["sha256"] == pin and
                     entry["kind"] == "unconsumed_python_bytecode", "incidentals are only pinned pycache")
                incidental_seen.add(logical)
            else:
                need(sha(read(logical)) == pin, "compiled snapshot source")
        for source, pin in receipt["proposal_before"].items():
            need(receipt["snapshot_before"].get("proposal/" + source) == pin, "proposal snapshot binding")
        for source, pin in receipt["baseline_before"].items():
            if source not in ("CMakeLists.txt", "src/forest/full_ball_tower.hpp"):
                need(receipt["snapshot_before"].get("morsehgp3D_v7/" + source) == pin, "unmodified baseline closure")
        overlay = value("source_snapshot/proposal/overlay.json")
        need(overlay == receipt["product_overlay"] and overlay["callback_supplied"] is True and overlay["product_instrumentation"] is False and
             overlay["results_inherited"] is False and overlay["new_product_helpers"] == [] and
             overlay["files"] == {"src/forest/full_ball_tower.hpp": HEADER}, "explicit header-only overlay")
        need(sha(read(prefix + "source_snapshot/morsehgp3D_v7/src/forest/full_ball_tower.hpp")) == HEADER ==
             sha(read(prefix + "source_snapshot/proposal/overlay/morsehgp3D_v7/src/forest/full_ball_tower.hpp")), "overlay bytes")
        origin = value("source_snapshot/proposal/origin.json")
        need(sha(read(prefix + "source_snapshot/proposal/origin.json")) == receipt["origin_sha256"] and
             origin["inherited_results"] is False, "explicit origin")
        for source, pin in origin["source_helper_pins"].items():
            need(sha(read(prefix + "source_snapshot/proposal/originals/" + source)) == pin, "historical judge pin")
        for source, pin in origin["baseline_source_pins"].items():
            need(receipt["baseline_before"].get(source) == pin and
                 sha(read(prefix + "source_snapshot/proposal/baseline/" + source)) == pin, "prepared baseline pin")
        for source in CANDIDATES:
            need(read(prefix + "source_snapshot/morsehgp3D_v7/tests/" + source) ==
                 read(prefix + "source_snapshot/proposal/candidate/morsehgp3D_v7/tests/" + source), "candidate compiled bytes")
        old_cmake = read(prefix + "source_snapshot/proposal/baseline/CMakeLists.txt").decode()
        new_cmake = read(prefix + "source_snapshot/morsehgp3D_v7/CMakeLists.txt").decode()
        patch = "".join(difflib.unified_diff(old_cmake.splitlines(keepends=True), new_cmake.splitlines(keepends=True),
                     fromfile="a/morsehgp3D_v7/CMakeLists.txt", tofile="b/morsehgp3D_v7/CMakeLists.txt"))
        need(patch.encode() == read(prefix + "source_snapshot/proposal/CMake.diff"), "compiled CMake diff")
        commands = receipt["commands"]
        need([command["name"] for command in commands] == ["compiler", "configure", "build", "inventory", "ctest"], "commands complete")
        for command in commands:
            need(command["exit_code"] == command["expected_exit_code"] == 0, "exact command status")
            need(not any(".pyc" in arg or "__pycache__" in arg for arg in command["argv"]), "no bytecode command")
            for stream in ("stdout", "stderr"):
                need(sha(read(prefix + command["name"] + "." + stream)) == command[stream + "_sha256"], "stream pin")
        need("--target" in commands[2]["argv"] and TARGET in commands[2]["argv"] and
             commands[2]["argv"][-2:] == ["--parallel", "1"] and "-j1" in commands[4]["argv"], "bounded target and serial qualification")
        need(captured["binary_pins_no_ELF"][prefix + "cmake_build/" + TARGET]["sha256"] == receipt["binary_sha256"], "main ELF pin")
        compile_raw = read(prefix + "cmake_build/compile_commands.json")
        need(sha(compile_raw) == receipt["compile_commands_sha256"], "compile commands pin")
        compile_all = json.loads(compile_raw)
        need(all(".pyc" not in item["command"] and "__pycache__" not in item["command"] for item in compile_all), "no bytecode compile command")
        chosen = [item for item in compile_all if item["file"].endswith("/tests/full_ball_batch_gate.cpp")]
        need(len(chosen) == 1, "exact target compile record")
        args = shlex.split(chosen[0]["command"])
        need(all(flag in args for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror")), "strict target flags")
        flags = read(prefix + TARGET_DIR + "flags.make").decode()
        link = read(prefix + TARGET_DIR + "link.txt").decode()
        if name.startswith("san"):
            need(all(flag in args and flag in flags and flag in link for flag in
                     ("-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer")), "SAN compile and link flags")
            need(receipt["sanitizer_environment"] == {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "LSan active")
        else:
            need("-O2" in args and "-DNDEBUG" in args, "O2 release flags")
        deps = read(prefix + TARGET_DIR + "tests/full_ball_batch_gate.cpp.o.d").decode()
        need(".pyc" not in deps and "__pycache__" not in deps, "incidental bytecode absent from consumed dependencies")
        need(all(token in deps for token in ("full_ball_batch_gate.cpp", "full_ball_batch_geometry.hpp", "full_ball_batch_owner.hpp",
             "full_ball_tower_gate.cpp", "full_ball_tower.hpp", "local_plateau_oracle.hpp")), "real target dependency closure")
        inventory = value("inventory.stdout")["tests"]
        need([test["name"] for test in inventory] == ["mhgp7_full_ball_batch_" + kind for kind in KINDS], "five intended CTests")
        log_raw = read(prefix + "LastTest.log")
        need(sha(log_raw) == receipt["ctest_log_sha256"], "CTest log pin")
        log = log_raw.decode()
        sections = re.split(r"^\d+/\d+ Testing: ", log, flags=re.MULTILINE)[1:]
        need(len(sections) == 5 and sum("Test Passed." in section for section in sections) == 5, "five physical CTest passes")
        for kind, test, section in zip(KINDS, inventory, sections):
            expected = 2 if kind in ("bad_argument", "missing_argument") else 0
            argument = {"cpu1": "--selftest-1", "cpu4": "--selftest-4", "rejects": "--rejects",
                        "bad_argument": "--unknown", "missing_argument": ""}[kind]
            command = test["command"]
            cause = ""
            need("-DEXPECTED=" + str(expected) in command and "-DARGS=" + argument in command and
                 "-DEXPECT_LINE=" + cause in command and any(arg.endswith("/cmake/run_expect.cmake") for arg in command),
                 "same execution exact code and cause wrapper")
            need(section.startswith(test["name"] + "\n") and "Test Passed." in section, "CTest identity in physical log")
            if cause:
                need("\n" + cause + "\n" in section, "actual causal line not command echo")
            need(not any(".pyc" in arg or "__pycache__" in arg for arg in command), "no CTest bytecode command")
        results = [json.loads(line) for line in log.splitlines() if line.startswith('{"status":')]
        need(results == receipt["CTest_results"] and len(results) == 5, "five physical JSON results")
        for result in (results[0], results[2]):
            need(all(result.get(key) == value for key, value in FULL.items()), "independent FULL physical results")
        need(results[1] == {**NOMINAL, "workers": 1} and results[3] == {**NOMINAL, "workers": 4},
             "two actual callbacks direct terminal and work nonvacuities")
        need(results[4] == {"status": "passed_batch_rejections", "cases": 65, "prefix_cases": 32,
             "known_paid": 44, "product_instrumentation": False}, "actual 65 callback refusals")
        need(not any("MHGP7_TESTING" in arg for arg in args), "no product testing macro")
        summaries[name] = results
        receipts[name] = receipt
    need(receipts["o2_r1"]["snapshot_before"] == receipts["san_root_r1"]["snapshot_before"], "full O2 SAN source closure")
    need(summaries["o2_r1"] == summaries["san_root_r1"] == captured["results"], "same O2 SAN observations")
    need(incidental_seen == set(captured["incidental_pins_no_bytes"]), "complete incidental pin accounting")
    for source, pin in receipts["o2_r1"]["snapshot_before"].items():
        if not source.endswith(".pyc"):
            need(sha(read("current/" + source)) == pin, "current visible qualified source")
    need(not incidental_seen, "no captured bytecode")
    return {"status": "passed", "captures": 2, "commands": 10, "CTests_each": 5,
            "FULL_orders_per_worker": 150, "FULL_clouds_per_worker": 34,
            "batch_calls_per_worker": 49, "direct_terminals_per_worker": 103,
            "diagnostic_reference_MEB_calls_per_worker": 119,
            "rejections_each": 65, "prefix_rejections_each": 32, "known_paid_rejections_each": 44,
            "incidental_bytecode_pins": 0, "callback_supplied": True, "product_instrumentation": False,
            "geometry_executed_by_reader": False, "device_executed": False, "gcp_used": False}



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--extract", type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "manifest.json").read_text())
    actual = {path.relative_to(ROOT).as_posix() for path in ROOT.rglob("*") if path.is_file()}
    need(actual == set(manifest["files"]) | {"manifest.json"}, "physical coverage")
    need(all(not path.is_symlink() for path in ROOT.rglob("*")), "no symlink")
    for name, entry in manifest["files"].items():
        data = (ROOT / safe(name)).read_bytes()
        need(sha(data) == entry["sha256"] and len(data) == entry["size"], "physical pin")
        need(not name.endswith(".pyc") and data[:4] != b"\x7fELF", "no ELF or bytecode payload")
        data.decode("utf-8")
    raw = (ROOT / "capture_manifest.json").read_bytes()
    need(sha(raw) == CAPTURE_PIN, "capture authority")
    captured = json.loads(raw)
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(captured["files"]), "logical coverage")
    for name, entry in mapping.items():
        safe(name)
        need(not name.endswith(".pyc"), "incidental bytes not distributed")
        data = (ROOT / safe(entry["storage"])).read_bytes()
        need({key: entry[key] for key in ("sha256", "size")} == captured["files"][name] and
             sha(data) == entry["sha256"] and len(data) == entry["size"], "logical pin")

    def read(name):
        need(name in mapping, "missing logical file: " + name)
        return (ROOT / safe(mapping[name]["storage"])).read_bytes()

    report = qualify(read, captured)
    if args.extract:
        args.extract.mkdir(exist_ok=False)
        for name in mapping:
            path = args.extract / safe(name)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("xb") as output:
                output.write(read(name))
    report.update(logical_files=len(mapping), physical_files=len(actual), ELF_pins_only=len(captured["binary_pins_no_ELF"]))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
