"""Independent read-only provenance check; never import a runner or execute C++.

Default operation uses published captures only. --check-local-elf additionally
requires and hashes the six retained executables; it is not needed after cleanup.
System headers excluded by -MMD and the complete inherited environment are outside
this check. It validates capture binding, not the scientific geometry judgments.
"""
from __future__ import annotations

import argparse
from datetime import datetime
import gzip
import hashlib
import json
from pathlib import Path
import posixpath
import shlex

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
HISTORIC_ROOT = "/workspaces/E-HGP"
PACKET = "morsehgp3D_v7/audits/receipts_full_meb_20260906/"
PREVIOUS = "morsehgp3D_v7/audits/receipts_full_successor_20260905/source/"
BUILD_NAMES = ("O2", "sanitized", "reset_work", "calendar_O2", "calendar_sanitized", "calendar_q4_first")
PREFIX = ["timeout", "--signal=TERM", "--kill-after=5", "180", "taskset", "-c", "1"]
EMPTY_SHA = hashlib.sha256(b"").hexdigest()
SOURCE_BINDING_SHA = "d83769f39431b907be9d1d35cddeb7abb809c657c9d605a1a238b292822d6913"
CLOSURE_SHA = "8b5a7dc21d91de1efbd4b716e2a690687b320d201d595544cbea7115ce5ac607"


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha(path: Path) -> str:
    return digest(path.read_bytes())


def read_json(path: Path):
    return json.loads(path.read_text())


def historical_relative(path: str) -> str:
    normalized = posixpath.normpath(path)
    require(normalized.startswith(HISTORIC_ROOT + "/"), "path.outside_historical_repository")
    return normalized[len(HISTORIC_ROOT) + 1:]


def source_binding() -> dict:
    require(sha(HERE / "source_binding.json") == SOURCE_BINDING_SHA, "source_binding.sealed_bytes")
    binding = read_json(HERE / "source_binding.json")
    substitutions = {
        "morsehgp3D_v7/audits/full_meb_bridge.cpp": "preparation_adapter.cpp.txt",
        "morsehgp3D_v7/audits/full_meb_run.py": "preparation_runner.py.txt"}
    for path, pin in binding["pins"].items():
        actual = HERE / substitutions[path] if path in substitutions else ROOT / path
        require(sha(actual) == pin, "source_binding.pin:" + path)
    require(len(binding["pins"]) == 30, "source_binding.inventory")
    require(binding["preparation_snapshots"] ==
            {name: binding["pins"][path] for path, name in substitutions.items()},
            "source_binding.snapshot_association")
    adapter = (HERE / "preparation_adapter.cpp.txt").read_bytes()
    for build in ("O2", "sanitized", "reset_work"):
        require((HERE / (build + "_adapter.cpp")).read_bytes() == adapter,
                "adapter.exact_compiled_snapshot:" + build)
    return {"pins": 30, "initial_snapshot_substitutions": substitutions,
            "compiled_adapter_copies_equal": 3, "source_binding_sha256": SOURCE_BINDING_SHA}


def command_captures() -> tuple[dict, dict]:
    expected = {build + "_build" for build in BUILD_NAMES}
    expected |= {build + "_" + corpus + "_P" + str(cap)
                 for build in ("O2", "sanitized") for corpus in ("legacy", "mixed", "higher")
                 for cap in (0, 1, 1000000)}
    expected.add("reset_work_legacy_P1")
    expected |= {build + "_" + mode for build in BUILD_NAMES if build.startswith("calendar_")
                 for mode in ("run", "normal", "optimized")}
    actual = {path.name.removesuffix(".intent.json") for path in HERE.glob("*.intent.json")}
    require(actual == expected and len(actual) == 34, "command.closed_inventory")
    terminals, plain_stdout = {}, {}
    for name in sorted(expected):
        intent = read_json(HERE / (name + ".intent.json"))
        terminal = read_json(HERE / (name + ".json"))
        for key in ("argv", "cwd", "environment", "input_sha256", "started_utc"):
            require(intent[key] == terminal[key], "command.intent_binding:" + name + ":" + key)
        require(terminal["argv"][:7] == PREFIX and terminal["cwd"] == HISTORIC_ROOT,
                "command.CPU1_timeout180:" + name)
        start, end = map(datetime.fromisoformat, (terminal["started_utc"], terminal["ended_utc"]))
        require(0 <= (end - start).total_seconds() < 180 and terminal["exit_code"] == 0,
                "command.success_within_deadline:" + name)
        packed = (HERE / (name + ".stdout.gz")).read_bytes()
        raw = gzip.decompress(packed)
        require(digest(packed) == terminal["stdout_gzip_sha256"] and
                digest(raw) == terminal["stdout_sha256"] and
                sha(HERE / (name + ".stderr")) == terminal["stderr_sha256"],
                "command.stream_binding:" + name)
        require(terminal["stderr_sha256"] == EMPTY_SHA, "command.no_stderr:" + name)
        if "_P" in name and not name.endswith("_build"):
            corpus = name.rsplit("_P", 1)[0].split("_")[-1]
            require(corpus in ("legacy", "mixed", "higher"), "command.input_corpus:" + name)
            require(sha(HERE / (corpus + ".stdin")) == terminal["input_sha256"],
                    "command.actual_input:" + name)
        else:
            require(terminal["input_sha256"] == EMPTY_SHA, "command.empty_stdin:" + name)
        is_sanitized_engine = (name.startswith("sanitized_") and not name.endswith("_build")) or name == "calendar_sanitized_run"
        wanted_env = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                      "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if is_sanitized_engine else {}
        require(terminal["environment"] == wanted_env, "command.declared_environment_overrides:" + name)
        terminals[name], plain_stdout[name] = terminal, raw
    return terminals, plain_stdout


def builds_and_runs(terminals: dict, check_local: bool) -> dict:
    binaries = {}
    dependency_entries = 0
    for build in BUILD_NAMES:
        record = read_json(HERE / (build + "_binary.json"))
        command = terminals[build + "_build"]["argv"]
        require(command[7] == "g++", "build.recorded_compiler:" + build)
        for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-MMD"):
            require(flag in command, "build.strict_flag:" + build + ":" + flag)
        require(not any("MHGP7_TESTING" in value for value in command), "build.nominal_macro:" + build)
        sanitized = "sanitized" in build
        if sanitized:
            require(all(flag in command for flag in ("-O1", "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=all", "-fno-omit-frame-pointer")), "build.SAN_flags:" + build)
        else:
            require("-O2" in command and not any(value.startswith("-fsanitize=") for value in command),
                    "build.O2_flags:" + build)
        output_path = command[command.index("-o") + 1]
        require(output_path == HISTORIC_ROOT + "/morsehgp3D_v7/audits/.work_full_meb_20260906/" + build + ".bin",
                "build.output_identity:" + build)
        require(historical_relative(command[command.index("-MF") + 1]) == PACKET + build + ".d",
                "build.dependency_output:" + build)
        dependency_text = (HERE / (build + ".d")).read_text().replace("\\\n", " ")
        target, dependencies = dependency_text.split(":", 1)
        require(shlex.split(target) == [output_path], "dependency.binary_target:" + build)
        paths = {historical_relative(path) for path in shlex.split(dependencies)}
        require(paths == set(record["dependencies"]), "dependency.complete_non_system_inventory:" + build)
        for path, pin in record["dependencies"].items():
            require(path.startswith(PACKET) or path.startswith(PREVIOUS), "dependency.no_live_product:" + path)
            require(sha(ROOT / path) == pin, "dependency.captured_bytes:" + path)
        source_files = [historical_relative(value) for value in command if value.endswith(".cpp")]
        require(len(source_files) == 1 and source_files[0] in paths, "dependency.compilation_unit:" + build)
        if build.startswith("calendar_"):
            require(source_files == [PACKET + "local_calendar.cpp"], "build.calendar_unit:" + build)
        else:
            require(source_files == [PACKET + build + "_adapter.cpp"], "build.full_unit:" + build)
        dependency_entries += len(paths)
        local = ROOT / historical_relative(output_path)
        if check_local:
            require(local.is_file() and local.read_bytes().startswith(b"\x7fELF"), "binary.local_ELF_present:" + build)
            require(sha(local) == record["sha256"], "binary.local_ELF_hash:" + build)
        binaries[build] = {"sha256": record["sha256"], "historical_path": output_path,
                           "non_system_dependencies": len(paths), "local_ELF_rehashed": check_local,
                           "engine_runs": []}
    for name, terminal in terminals.items():
        executable = terminal["argv"][7]
        if not executable.endswith(".bin"):
            continue
        build = Path(executable).name.removesuffix(".bin")
        require(build in binaries and executable == binaries[build]["historical_path"], "run.binary_binding:" + name)
        require(terminal["started_utc"] >= terminals[build + "_build"]["ended_utc"], "run.after_build:" + name)
        args = terminal["argv"][8:]
        if build.startswith("calendar_"):
            require(name == build + "_run" and args == [], "run.calendar_args:" + name)
        else:
            require(args == [name.rsplit("_P", 1)[1]], "run.proposal_cap_arg:" + name)
        binaries[build]["engine_runs"].append(name)
    require([len(binaries[name]["engine_runs"]) for name in BUILD_NAMES] == [9, 9, 1, 1, 1, 1],
            "run.per_binary_nonvacuum_counts")
    return {"binaries": binaries, "dependency_entries": dependency_entries,
            "local_ELF_rehashed": 6 if check_local else 0}


def close_and_judges(terminals: dict, stdout: dict) -> dict:
    require(sha(HERE / "closure.json") == CLOSURE_SHA, "closure.sealed_bytes")
    closure = read_json(HERE / "closure.json")
    cpp = {}
    for name, command in terminals.items():
        kind = "compiler" if command["argv"][7] == "g++" else "engine" if command["argv"][7].endswith(".bin") else None
        if kind:
            cpp[name] = {"name": name, "kind": kind, "ended_utc": command["ended_utc"], "exit_code": 0}
    require(closure["commands"] == [cpp[name] for name in sorted(cpp)], "closure.complete_cpp_commands")
    require(closure["status"] == "closed" and closure["cpp_compilations"] == 6 and
            closure["cpp_engine_runs"] == 22 and len(cpp) == 28, "closure.counts")
    intervals = sorted((terminals[name]["started_utc"], terminals[name]["ended_utc"]) for name in cpp)
    require(all(a[1] <= b[0] for a, b in zip(intervals, intervals[1:])), "closure.cpp_serial_no_overlap")
    require(closure["last_cpp_ended_utc"] == max(x[1] for x in intervals), "closure.last_cpp_end")
    require(sha(HERE / "normal.json") == sha(HERE / "optimized.json") == closure["normal_optimized_identical_sha256"],
            "closure.full_judgment_bytes")
    require(sha(HERE / "local_calendar_judge.py") == "c19de5caf639831e3d5dd2b542779eff08eaff518f135142fe5c984da9d4fcad",
            "calendar.judge_source")
    for build in BUILD_NAMES:
        if not build.startswith("calendar_"):
            continue
        for mode in ("normal", "optimized"):
            name = build + "_" + mode
            argv = terminals[name]["argv"]
            expected = ["-B"] + (["-O"] if mode == "optimized" else []) + [
                HISTORIC_ROOT + "/" + PACKET + "local_calendar_judge.py",
                HISTORIC_ROOT + "/morsehgp3D_v7/audits/.work_full_meb_20260906/" + build + ".jsonl"]
            if build == "calendar_q4_first":
                expected.append("--expect-q4-first")
            require(Path(argv[7]).name == "python3" and argv[8:] == expected, "calendar.judge_argv:" + name)
            result = json.loads(stdout[name])
            require(result["output_sha256"] == terminals[build + "_run"]["stdout_sha256"],
                    "calendar.judge_input_from_engine_output:" + name)
            require(result["status"] == ("refuted" if build == "calendar_q4_first" else "passed"),
                    "calendar.judge_recorded_verdict:" + name)
        require(stdout[build + "_normal"] == stdout[build + "_optimized"], "calendar.normal_optimized_identical:" + build)
    return {"cpp_compilations": 6, "cpp_engine_runs": 22, "python_calendar_reads": 6,
            "last_cpp_ended_utc": closure["last_cpp_ended_utc"], "cpp_overlap": False,
            "closure_sha256": CLOSURE_SHA}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-local-elf", action="store_true")
    args = parser.parse_args()
    binding = source_binding()
    terminals, stdout = command_captures()
    builds = builds_and_runs(terminals, args.check_local_elf)
    closed = close_and_judges(terminals, stdout)
    print(json.dumps({"schema": "mhgp7-independent-full-meb-capture-binding-v1", "status": "passed",
                      "public_status": "not_claimed", "gcp": "not_used", "engines_invoked": 0,
                      "runners_imported": 0, "source_binding": binding, "commands": len(terminals),
                      "stream_triplets_verified": len(terminals), "builds": builds, "closure": closed,
                      "portable_without_ELF": not args.check_local_elf,
                      "limits": ["Hashes and recorded commands bind captures; no new scientific geometry judgment.",
                                 "Compiler identity is recorded g++ and version text, not a hermetic toolchain capture.",
                                 "System headers omitted by -MMD are not checked.",
                                 "Only declared process environment overrides are recorded and checked.",
                                 "Default mode checks preserved binary identities without reading or executing ELF."]},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
