"""Read the closed packet without compiling or consuming the active engine."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import tarfile


HERE = Path(__file__).resolve().parent


def need(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def configuration(run: dict, is_san: bool) -> None:
    options = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
               "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if is_san else {}
    need(run["sanitizer_options"] == options, "sanitizer environment")
    required = {"-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-isystem"}
    required |= {"-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"} if is_san else {"-O2"}
    for command in run["commands"]:
        if not command["name"].startswith("compile_"):
            continue
        argv = command["argv"]
        need(required <= set(argv), "compiler flags: " + command["name"])
        need([arg for arg in argv if arg.startswith("-O")] == ["-O1" if is_san else "-O2"], "optimization flags")
        need([arg for arg in argv if arg.startswith("-fsanitize=")] ==
             (["-fsanitize=address,undefined"] if is_san else []), "sanitizer compile flags")


def main() -> int:
    files = {}
    for line in (HERE / "SHA256SUMS").read_text().splitlines():
        expected, name = line.split("  ", 1)
        need(name not in files and Path(name).name == name, "manifest member")
        need(sha((HERE / name).read_bytes()) == expected, "changed: " + name)
        files[name] = expected
    need({"prefix_gate.cpp", "record.py", "verify.py", "source_snapshot.tar.gz",
          "source_pins.json", "README.md", "o2_mutants.json", "san.json", "san_replay.json"} <= files.keys(),
         "missing packet authority")
    pins = json.loads((HERE / "source_pins.json").read_text())
    need(files["source_snapshot.tar.gz"] == pins["archive_sha256"], "archive authority")
    with tarfile.open(HERE / "source_snapshot.tar.gz") as source:
        members = source.getmembers()
        need(sorted(m.name for m in members) == sorted(pins["files"]), "source membership")
        for member in members:
            need(member.isfile(), "not a regular source")
            need(sha(source.extractfile(member).read()) == pins["files"][member.name],
                 "source bytes: " + member.name)
    outputs = []
    for name, is_san, is_mutants in (("o2_mutants.json", False, True), ("san_replay.json", True, False)):
        run = json.loads((HERE / name).read_text())
        need(run["status"] == "passed" and run["inputs_stable"], name + ": closed pass")
        need(run["sanitizers"] == is_san and run["mutants"] == is_mutants, "capture mode")
        configuration(run, is_san)
        need(run["public_status"] == "not_claimed" and run["gcp_used"] is False, "scope")
        need(run["inputs_before"] == run["inputs_after"], "stable inputs")
        for path, digest in run["inputs_before"].items():
            need(files[path] == digest, "captured input: " + path)
        need(run["compiled_source_final_pins"] == pins["files"], "restored nominal sources")
        commands = {c["name"]: c for c in run["commands"]}
        expected_names = {"compiler_version", "compile_nominal", "run_nominal", "reject_arguments"}
        if is_mutants:
            expected_names |= {prefix + mutant for prefix in ("compile_", "run_")
                               for mutant in ("future_root", "backdated_contribution", "reused_continuation")}
        need(set(commands) == expected_names and len(commands) == len(run["commands"]), "commands")
        for command in commands.values():
            expected = (2 if command["name"] == "reject_arguments" else
                        1 if command["name"].startswith("run_") and command["name"] != "run_nominal" else 0)
            need(command["exit_code"] == command["expected_exit"] == expected, "exit: " + command["name"])
            if expected == 0:
                need(not command["stderr"], "diagnostic: " + command["name"])
        if is_mutants:
            for mutant, reason in {
                "future_root": "FAIL prefix.old.cut.immutable\n",
                "backdated_contribution": "FAIL contribution.level\n",
                "reused_continuation": "FAIL late.parent.or.level.rejection\n",
            }.items():
                need(commands["run_" + mutant]["stderr"] == reason, "causal refusal: " + mutant)
        outputs.append(commands["run_nominal"]["stdout"])
    need(outputs[0] == outputs[1], "O2/SAN equality")
    counts = json.loads(outputs[0])
    for key, expected in {"encodings": 4, "prefixes": 20,
                          "cut_root_and_read_queries": 5616, "invariant_cut_pairs": 260,
                          "successor_writes": 24, "closed_changes": 16,
                          "rejects": 16, "same_kernel_facade_comparisons": 20}.items():
        need(counts[key] == expected, "non-vacuity: " + key)
    need(counts["checks"] > 11000 and counts["status"] == "passed", "non-vacuity: checks")
    first_san = json.loads((HERE / "san.json").read_text())
    configuration(first_san, True)
    need(first_san["status"] == "failed" and first_san["inputs_stable"] and
         first_san["compiled_source_final_pins"] == pins["files"], "preserved first SAN refusal")
    need(first_san["inputs_before"] == first_san["inputs_after"], "first SAN stable inputs")
    for path, digest in first_san["inputs_before"].items():
        need(files[path] == digest, "first SAN input: " + path)
    first_commands = first_san["commands"]
    need([c["name"] for c in first_commands] == ["compiler_version", "compile_nominal", "run_nominal"] and
         [c["exit_code"] for c in first_commands] == [0, 0, 1], "first SAN command outcomes")
    need("LeakSanitizer does not work under ptrace" in first_commands[-1]["stderr"], "first SAN environment refusal")
    print(json.dumps({"status": "passed", "manifest_files": len(files),
                      "source_files": len(pins["files"]), "nominal_stdout": outputs[0].strip(),
                      "causal_mutants": 3, "public_status": "not_claimed", "gcp_used": False}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, TypeError) as error:
        print("invalid packet:", error)
        raise SystemExit(1)
