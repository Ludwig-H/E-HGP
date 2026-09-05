#!/usr/bin/env python3
"""Fresh successor-normalization FULL qualification, inert by default; explicit ROOT GO only.

Pinned legacy modules provide byte/provenance primitives, not old test results.
Only eight declared binaries and twenty exact-labelled CTests are executed.
--snapshot reads sources without running commands. --execute requires this
controller's SHA256 and the canonical source-map SHA256 printed by --snapshot.
"""
from __future__ import annotations

import argparse
from collections import Counter
import copy
import json
import math
import os
from pathlib import Path
import re
import shlex
import signal
import types
import xml.etree.ElementTree as ET

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_successor_20260905_controller"
SOURCE = ROOT / "morsehgp3D_v7"
LEGACY = ROOT / "build/v7_full_lazy_20260905_controller/capture.py"
LEGACY_SHA = "528175a4fae239aa62630c32c27355be34db1092bef7f8cdb98e589022663bb4"
COMMON = ROOT / "build/v7_full_lazy_20260905_controller/publish.py"
COMMON_SHA = "5c7f18a2577ee388a8f9652c3596ffe9ab9ade6bbc3101ae45f18fc91da6dfba"
CHECKER = ROOT / "tools/check_v7_receipt_publication.py"
CHECKER_SHA = "32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580"
SINGLETON = "mhgp7_full_gabriel_singleton_gate"
SINGLETON_SOURCE = "tests/full_gabriel_singleton_gate.cpp"
SINGLETON_FIXTURES = "tests/full_gabriel_singleton_fixtures.hpp"
SUCCESSOR = "mhgp7_full_gabriel_successor_gate"
SUCCESSOR_SOURCE = "tests/full_gabriel_successor_gate.cpp"
LABELS = "^(full_certificate|full_gabriel|full_gabriel_lazy|full_gabriel_digest|full_gabriel_singleton|full_gabriel_successor)$"
SCHEMA = "mhgp7-successor-qualification-v1"


def imported(name, original, expected):
    import hashlib
    local = Path(__file__).resolve().parent / name
    path = local if local.is_file() and not local.is_symlink() else original
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ValueError("imported protocol SHA256 mismatch: " + str(path))
    module = types.ModuleType("inert_successor_" + name.replace(".", "_") + "_" + expected[:8])
    module.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), module.__dict__)
    return module, path, raw


def context(out, tag):
    C, _, _ = imported("legacy_capture.py", LEGACY, LEGACY_SHA)
    C.OUT, C.REGEX = out, LABELS
    suffix = "_" + tag if tag else ""
    C.BUILDS = {mode: ROOT / ("build/v7_successor_20260905" + suffix + "_" + mode)
                for mode in ("release", "san")}
    C.TESTS.update({"mhgp7_full_gabriel_singleton": (SINGLETON, "--selftest", 0),
                    "mhgp7_full_gabriel_singleton_rejects": (SINGLETON, "--rejects", 0),
                    "mhgp7_full_gabriel_singleton_bad_argument": (SINGLETON, "--unknown", 2),
                    "mhgp7_full_gabriel_successor": (SUCCESSOR, "--selftest", 0),
                    "mhgp7_full_gabriel_successor_rejects": (SUCCESSOR, "--rejects", 0),
                    "mhgp7_full_gabriel_successor_bad_argument": (SUCCESSOR, "--unknown", 2)})
    C.judge_junit = lambda raw: junit(raw, C)
    # No Git command is part of this controller. HEAD may be declared by ROOT.
    C.TOOLS = tuple(name for name in C.TOOLS if name != "/usr/bin/git")
    return C


def sources(C):
    result = C.source_snapshot()
    for name in (SINGLETON_SOURCE, SINGLETON_FIXTURES, SUCCESSOR_SOURCE):
        result[str((SOURCE / name).relative_to(ROOT))] = C.read_stable(SOURCE / name)[1]["sha256"]
    return dict(sorted(result.items()))


def targets(C):
    return C.TARGETS | {SINGLETON: SINGLETON_SOURCE, SUCCESSOR: SUCCESSOR_SOURCE}


def commands(C, mode):
    result = []
    for label, argv, timeout in C.commands(mode):
        if label == "build":
            argv += [SINGLETON, SUCCESSOR]
        if label in ("inventory", "ctest"):
            argv = ["-L" if value == "-R" else value for value in argv]
        result.append((label, argv, 180 if label == "ctest" else timeout))
    return result


def inventory(raw, build, C):
    rows = json.loads(raw).get("tests", [])
    names = [row.get("name") for row in rows]
    C.require(len(names) == len(C.TESTS) == 20 and len(set(names)) == 20 and sorted(names) == sorted(C.TESTS),
              "exact twenty-test inventory required")
    for row in rows:
        target, argument, code = C.TESTS[row["name"]]
        C.require(row["command"] == ["/usr/bin/cmake", "-DEXPECTED=" + str(code), "-DCMD=" + str(build / target),
                  "-DARGS=" + argument, "-DEXPECT_LINE=", "-DEXPECT_PREFIX=", "-P", str(SOURCE / "cmake/run_expect.cmake")],
                  "test argv/expected-code binding")
        props = {item["name"]: item["value"] for item in row.get("properties", [])}
        C.require(props.get("TIMEOUT") == 60 and props.get("WORKING_DIRECTORY") == str(build) and
                  any(re.fullmatch(LABELS, value) for value in props.get("LABELS", [])), "test timeout/label binding")
        C.require(not any(key in props for key in ("DISABLED", "WILL_FAIL", "PASS_REGULAR_EXPRESSION", "SKIP_RETURN_CODE",
                  "ENVIRONMENT", "ENVIRONMENT_MODIFICATION", "FIXTURES_REQUIRED", "DEPENDS")), "test mutation property")
    return {"tests": 20, "names": sorted(names), "commands_codes_and_timeouts_exact": True, "label_filter": LABELS}


def junit(raw, C):
    root = ET.fromstring(raw)
    rows = root.findall("testcase")
    names = [row.get("name") for row in rows]
    C.require(root.tag == "testsuite" and root.get("tests") == "20" and
              all(root.get(key) == "0" for key in ("failures", "disabled", "skipped")) and
              len(names) == len(set(names)) == 20 and sorted(names) == sorted(C.TESTS), "JUnit inventory/status")
    for row in rows:
        elapsed = float(row.get("time", "nan"))
        C.require(row.get("status") == "run" and math.isfinite(elapsed) and elapsed >= 0 and
                  not any(row.find(key) is not None for key in ("failure", "error", "skipped")), "JUnit case not run")
    return {"tests": 20, "names": sorted(names), "all_run": True}


def selftest(C):
    # These are explicitly metadata MODELS, never fabricated engine receipts.
    build = ROOT / "build/successor_metadata_model_not_an_engine_run"
    rows = []
    for name, (target, argument, code) in sorted(C.TESTS.items()):
        rows.append({"name": name, "command": ["/usr/bin/cmake", "-DEXPECTED=" + str(code),
                     "-DCMD=" + str(build / target), "-DARGS=" + argument, "-DEXPECT_LINE=", "-DEXPECT_PREFIX=",
                     "-P", str(SOURCE / "cmake/run_expect.cmake")],
                     "properties": [{"name": "TIMEOUT", "value": 60}, {"name": "WORKING_DIRECTORY", "value": str(build)},
                                    {"name": "LABELS", "value": ["gate", "full_gabriel"]}]})
    base = {"tests": rows}
    xml = ET.Element("testsuite", tests="20", failures="0", disabled="0", skipped="0")
    for name in sorted(C.TESTS):
        ET.SubElement(xml, "testcase", name=name, status="run", time="0.25")
    inventory(C.encoded(base), build, C)
    junit(ET.tostring(xml), C)
    mutants = []
    for name in ("missing", "duplicate", "expected_code", "disabled", "label", "timeout", "environment"):
        value = copy.deepcopy(base)
        if name == "missing":
            value["tests"].pop()
        elif name == "duplicate":
            value["tests"][-1] = copy.deepcopy(value["tests"][0])
        elif name == "expected_code":
            value["tests"][0]["command"][1] = "-DEXPECTED=7"
        elif name == "disabled":
            value["tests"][0]["properties"].append({"name": "DISABLED", "value": True})
        elif name == "label":
            value["tests"][0]["properties"][-1]["value"] = ["full_gabriel_unreviewed_suffix"]
        elif name == "timeout":
            value["tests"][0]["properties"][0]["value"] = 61
        else:
            value["tests"][0]["properties"].append({"name": "ENVIRONMENT", "value": ["ASAN_OPTIONS=detect_leaks=0"]})
        mutants.append(("inventory_" + name, lambda data=value: inventory(C.encoded(data), build, C)))
    for name in ("missing", "duplicate", "not_run", "skipped", "failure"):
        value = copy.deepcopy(xml)
        if name == "missing":
            value.remove(value[-1])
        elif name == "duplicate":
            value[-1].set("name", value[0].get("name"))
        elif name == "not_run":
            value[0].set("status", "notrun")
        elif name == "skipped":
            ET.SubElement(value[0], "skipped")
        else:
            value.set("failures", "1")
        mutants.append(("junit_" + name, lambda data=value: junit(ET.tostring(data), C)))
    killed = []
    for name, function in mutants:
        try:
            function()
        except RuntimeError:
            killed.append(name)
    C.require(len(killed) == len(mutants) == 12, "metadata mutant survived")
    return {"status": "selftests_passed", "positive_models": 2, "mutants_killed": killed,
            "models_are_engine_receipts": False, "engine_runs": 0}


def binaries(build, C):
    result = {}
    for target in targets(C):
        C.require(os.access(build / target, os.X_OK), "missing executable:" + target)
        result[target] = C.read_stable(build / target)[1]
    return result


def testing_binding(build, mode, before, C, target, primary_source):
    relobject = "CMakeFiles/" + target + ".dir/" + primary_source + ".o"
    rows = [row for row in json.loads((build / "compile_commands.json").read_text()) if row.get("output") == relobject]
    C.require(len(rows) == 1, "testing target compile database entry")
    row = rows[0]
    base = "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie" if mode == "san" else ""
    opts = "-O1 -g -DNDEBUG" if mode == "san" else "-O3 -DNDEBUG"
    linker = "-fsanitize=address,undefined -no-pie" if mode == "san" else ""
    argv = shlex.split(row["command"])
    C.require(row["directory"] == str(build) and row["file"] == str(SOURCE / primary_source) and
              argv[0] == "/usr/bin/g++" and Counter(argv[1:]) == Counter(shlex.split(base + " " + opts) +
              ["-DMHGP7_TESTING=1", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-std=c++20", "-o", relobject,
               "-c", str(SOURCE / primary_source)]), "testing target exact flags including sole testing macro")
    link_path = build / ("CMakeFiles/" + target + ".dir/link.txt")
    link_argv = shlex.split(link_path.read_text())
    C.require(link_argv[0] == "/usr/bin/g++" and Counter(link_argv[1:]) ==
              Counter(shlex.split(base + " " + opts + " " + linker) + [relobject, "-o", target]), "testing target link flags")
    dep_path = build / (relobject + ".d")
    header, body = dep_path.read_text().replace("\\\n", " ").split(":", 1)
    C.require(header.strip() == relobject, "testing target dependency target")
    dependencies = [(build / token).resolve() for token in shlex.split(body)]
    project, external = {}, []
    for path in dependencies:
        if path.is_relative_to(ROOT):
            name, pin = str(path.relative_to(ROOT)), C.read_stable(path)[1]["sha256"]
            C.require(name in before and before[name] == pin, "testing target dependency new or changed:" + name)
            project[name] = pin
        else:
            external.append(str(path))
    C.require(str((SOURCE / primary_source).relative_to(ROOT)) in project, "testing target primary source absent")
    return {"compile": row, "link_argv": link_argv, "project_dependencies": project,
        "external_headers": external, "object": C.read_stable(build / relobject)[1],
        "depfile": C.read_stable(dep_path)[1], "link_file": C.read_stable(link_path)[1]}



def binding(build, mode, before, C):
    # The six product binaries remain macro-free; both testing gates opt in.
    result = C.compile_binding(build, mode, before)
    for target, primary_source in ((SINGLETON, SINGLETON_SOURCE), (SUCCESSOR, SUCCESSOR_SOURCE)):
        result["targets"][target] = testing_binding(build, mode, before, C, target, primary_source)
    return result


def phase(mode, before, tools_before, controller_sha, C):
    build, out = C.BUILDS[mode], C.OUT / mode
    out.mkdir()
    records, errors = [], []
    result = {"mode": mode, "status": "failed", "commands": records, "errors": errors}
    first_binding = first_binaries = fence = previous = None
    try:
        C.require(not build.exists() and not build.is_symlink(), "build no longer fresh")
        C.save(out / "host_before.json", C.host())
        C.save(out / "environment.json", C.environment(mode))
        for label, argv, timeout in commands(C, mode):
            C.require(sources(C) == before and C.tool_snapshot() == tools_before and
                      C.read_stable(Path(__file__))[1]["sha256"] == controller_sha, "pre-command source/tool/protocol drift")
            if label == "ctest":
                C.require(C.host()["process_status"].get("TracerPid") == "0", "traced CTest forbidden")
                C.require(binaries(build, C) == first_binaries and binding(build, mode, before, C) == first_binding,
                          "pre-CTest binaries/binding drift")
                log = build / "Testing/Temporary/LastTest.log"
                if log.exists() or log.is_symlink():
                    previous = C.copy_exact(log, out / "LastTest.preexisting.stdout")
                C.save(out / "LastTest.prestate.json", {"present": previous is not None, "binding": previous})
                C.require(not (out / "ctest.junit.xml").exists(), "preexisting JUnit forbidden")
                marker = build / "Testing/Temporary/.successor_owned_ctest_fence"
                marker.parent.mkdir(parents=True, exist_ok=True)
                C.save(marker, {"run": str(out), "utc_diagnostic_only": C.utc()})
                fence = C.copy_exact(marker, out / "ctest.fence")
                C.save(out / "ctest.fence.json", fence)
                C.save(out / "pre_ctest_binding.json", {"source_sha256": C.sha(C.encoded(before)),
                       "binary_map_sha256": C.sha(C.encoded(first_binaries)), "compile_binding_sha256": C.sha(C.encoded(first_binding))})
            command = C.run(out, label, argv, timeout, C.environment(mode), records)
            C.require(command["status"] == "completed" and command["exit_code"] == 0, label + " failed")
            C.require(sources(C) == before, "post-command source drift")
            if label == "build":
                first_binding, first_binaries = binding(build, mode, before, C), binaries(build, C)
                C.save(out / "compile_binding.json", first_binding)
                C.save(out / "binaries.json", first_binaries)
            elif label == "inventory":
                result["inventory"] = inventory((out / "inventory.stdout").read_bytes(), build, C)
    except BaseException as error:
        errors.append(f"{type(error).__name__}: {error}")
    def observe(name, function):
        try:
            value = function()
            C.save(out / (name + ".json"), value)
            return value
        except BaseException as error:
            errors.append(f"{name}:{type(error).__name__}: {error}")
            return None
    after = observe("sources_after", lambda: sources(C))
    last_binaries = observe("binaries_after", lambda: binaries(build, C))
    try:
        last_binding = binding(build, mode, before, C)
        result["compile_binding_stable"] = first_binding is not None and last_binding == first_binding
        C.save(out / "compile_binding_after_sha256.json", {"sha256": C.sha(C.encoded(last_binding))})
    except BaseException as error:
        result["compile_binding_stable"] = False
        errors.append(f"final_compile_binding:{type(error).__name__}: {error}")
    observe("host_after", C.host)
    after_tools = observe("toolchain_after", C.tool_snapshot)
    # Generated configuration/flags/dependency files are copied even on failure.
    old_targets = C.TARGETS
    try:
        C.TARGETS = targets(C)
        observe("build_artifacts", lambda: C.collect_build_artifacts(build, out))
    finally:
        C.TARGETS = old_targets
    if fence is not None:
        result["test_outputs"] = observe("test_outputs", lambda: C.archive_test_outputs(build, out, fence, previous))
    else:
        errors.append("CTest not admitted; no test success inferred")
    result.update(sources_stable=after == before, binaries_stable=first_binaries is not None and last_binaries == first_binaries,
                  source_sha256=C.sha(C.encoded(before)), toolchain_stable=after_tools == tools_before)
    if (not errors and all(result[key] for key in ("sources_stable", "binaries_stable", "compile_binding_stable", "toolchain_stable"))
            and result.get("test_outputs") and result.get("inventory") and
            [row["label"] for row in records] == [row[0] for row in commands(C, mode)]):
        result["status"] = "completed"
    C.save(out / "summary.json", result)
    return result


def execute(args, C):
    C.require(re.fullmatch("[0-9a-f]{64}", args.expected_source_sha256 or ""), "explicit source SHA256 required")
    C.require(not C.OUT.exists() and not C.OUT.is_symlink(), "create-only capture exists")
    C.OUT.mkdir()
    result = {"schema": SCHEMA, "kind": "qualification", "status": "failed", "started_utc": C.utc(),
              "public_status": "not_claimed", "source_sha256": args.expected_source_sha256,
              "producer_sha256": None,
              "controller_sha256": args.expected_controller_sha256, "build_tag": args.build_tag,
              "development": args.development, "repository_head_declared_by_ROOT": args.repository_head,
              "scope": "20 targeted CTests per fresh Release/SAN build; not F or whole suite",
              "tests": C.TESTS, "targets": targets(C), "phases": [], "errors": [], "hermetic": False,
              "permission_override": "none", "historical_results_reused": False, "gcp_used": False}
    before = tools_before = None
    try:
        # Preserve the executable protocol even if source/build admission fails.
        protocol_dir = C.OUT / "protocol"
        protocol_dir.mkdir()
        C.copy_exact(Path(__file__), protocol_dir / "capture.py")
        for name, original, pin in (("legacy_capture.py", LEGACY, LEGACY_SHA),
                                    ("publication_common.py", COMMON, COMMON_SHA),
                                    ("check_v7_receipt_publication.py", CHECKER, CHECKER_SHA)):
            _, path, _ = imported(name, original, pin)
            C.copy_exact(path, protocol_dir / name)
        C.require(all(not path.exists() and not path.is_symlink() for path in C.BUILDS.values()), "both build dirs must be absent")
        C.save(C.OUT / "freshness.json", {"observed_utc": C.utc(), "builds": {str(path): "absent" for path in C.BUILDS.values()}})
        before = sources(C)
        C.save(C.OUT / "sources_before.json", before)
        result["producer_sha256"] = before["morsehgp3D_v7/src/forest/full_gabriel.hpp"]
        C.require(C.sha(C.encoded(before)) == args.expected_source_sha256, "source admission fingerprint differs")
        C.save(C.OUT / "host_before.json", C.host())
        C.require(C.host()["process_status"].get("TracerPid") == "0", "traced qualification forbidden")
        C.environment("release"), C.environment("san")
        tools_before = C.tool_snapshot()
        C.save(C.OUT / "toolchain_before.json", tools_before)
        C.save(C.OUT / "boost_inventory.json", {"authority": "dependency_identities_only_no_old_test_result",
               "header_count": C.BOOST_HEADER_COUNT, "canonical_header_map_sha256": C.BOOST_MAP_SHA,
               "depfile_copy": C.copy_exact(C.BOOST_DEPFILE, C.OUT / "boost_dependency_identities.stdout")})
        observations = []
        for label, argv in (("cmake_version", ["/usr/bin/cmake", "--version"]),
                            ("ctest_version", ["/usr/bin/ctest", "--version"]),
                            ("compiler_version", ["/usr/bin/g++", "--version"]),
                            ("compiler_target", ["/usr/bin/g++", "-dumpmachine"])):
            row = C.run(C.OUT, label, ["/usr/bin/taskset", "-c", "0", *argv], 15, C.environment("release"), observations)
            C.require(row["status"] == "completed", "toolchain observation failed")
        for mode in ("normal", "optimized"):
            argv = ["/usr/bin/python3", "-B"] + (["-O"] if mode == "optimized" else []) + [str(Path(__file__)), "--selftest"]
            row = C.run(C.OUT, "metadata_selftest_" + mode, ["/usr/bin/taskset", "-c", "0", *argv],
                        15, C.environment("release"), observations)
            C.require(row["status"] == "completed", "metadata selftest failed")
            observed = json.loads((C.OUT / ("metadata_selftest_" + mode + ".stdout")).read_bytes())
            C.require(observed["status"] == "selftests_passed" and len(observed["mutants_killed"]) == 12 and
                      observed["positive_models"] == 2 and observed["models_are_engine_receipts"] is False,
                      "metadata selftest nonvacuum")
        C.save(C.OUT / "toolchain_commands.json", observations)
        for mode in ("release", "san"):
            observed = phase(mode, before, tools_before, args.expected_controller_sha256, C)
            result["phases"].append(observed)
            C.require(observed["status"] == "completed", mode + " qualification failed; no automatic retry")
        C.require(sources(C) == before and C.tool_snapshot() == tools_before and
                  C.read_stable(Path(__file__))[1]["sha256"] == args.expected_controller_sha256, "terminal binding drift")
        result["status"] = "completed"
    except BaseException as error:
        result["errors"].append(f"{type(error).__name__}: {error}")
    for name, function in (("sources_after", lambda: sources(C)), ("toolchain_after", C.tool_snapshot), ("host_after", C.host)):
        try:
            value = function()
            C.save(C.OUT / (name + ".json"), value)
            if name == "sources_after":
                C.require(before is not None and value == before, "terminal source snapshot differs")
            elif name == "toolchain_after":
                C.require(tools_before is not None and value == tools_before, "terminal toolchain differs")
        except BaseException as error:
            result["status"] = "failed"
            result["errors"].append(f"{name}:{type(error).__name__}: {error}")
    result["ended_utc"] = C.utc()
    result["artifacts"] = {str(path.relative_to(C.OUT)): C.read_stable(path)[1]["sha256"]
                           for path in sorted(C.OUT.rglob("*")) if path.is_file()}
    C.save(C.OUT / "receipt.json", result)
    print(C.encoded({"status": result["status"], "receipt": str(C.OUT / "receipt.json"),
                     "sha256": C.read_stable(C.OUT / "receipt.json")[1]["sha256"]}).decode(), end="")
    return 0 if result["status"] == "completed" else 1


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--snapshot", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--expected-controller-sha256")
    parser.add_argument("--expected-source-sha256")
    parser.add_argument("--id", default="qualification")
    parser.add_argument("--build-tag", default="")
    parser.add_argument("--development", action="store_true")
    parser.add_argument("--repository-head")
    args = parser.parse_args(argv)
    C = context(BASE / ("run_" + args.id), args.build_tag)
    C.require(re.fullmatch("[a-z0-9][a-z0-9_-]{0,63}", args.id) and
              (args.build_tag == "" or re.fullmatch("[a-z0-9][a-z0-9_-]{0,63}", args.build_tag)), "capture/build tag")
    C.require(sum((args.execute, args.snapshot, args.selftest)) <= 1, "snapshot/selftest/execution are separate actions")
    if args.selftest:
        print(C.encoded(selftest(C)).decode(), end="")
        return 0
    if not args.execute:
        output = {"status": "prepared_not_executed", "requires_ROOT_GO": True, "tests": C.TESTS, "targets": targets(C),
                  "commands": {mode: commands(C, mode) for mode in ("release", "san")},
                  "controller_sha256": C.read_stable(Path(__file__))[1]["sha256"]}
        if args.snapshot:
            before = sources(C)
            output.update(sources=before, source_sha256=C.sha(C.encoded(before)))
        print(C.encoded(output).decode(), end="")
        return 0
    C.require(re.fullmatch("[0-9a-f]{64}", args.expected_controller_sha256 or "") and
              C.read_stable(Path(__file__))[1]["sha256"] == args.expected_controller_sha256, "reviewed controller SHA required")
    C.require(args.repository_head is None or re.fullmatch("[0-9a-f]{40}", args.repository_head), "declared repository HEAD")
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, C.interrupted)
    return execute(args, C)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError) as error:
        print("Successor qualification refused:", error)
        raise SystemExit(1)
