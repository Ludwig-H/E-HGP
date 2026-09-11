"""Read-only verification of the bounded fused-mark captures; no execution."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

BASE = Path(__file__).resolve().parent
REFERENCE_SHA = "1cf438cbac839c8f502de0c8daebbf89296a3884d2d40ba50715b5aaca161fa2"
FILES = {
    "README.md", "fused_marks.hpp", "gate.cpp", "record.py", "verify.py",
    "o2.json", "san.json", "result.json", "context_pins.json",
}


def need(value: bool, reason: str) -> None:
    if not value:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def unique(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result = {}
    for key, value in pairs:
        need(key not in result, "duplicate JSON key")
        result[key] = value
    return result


def read(name: str) -> dict:
    return json.loads((BASE / name).read_text(), object_pairs_hook=unique)


def verify() -> dict:
    seen = set()
    for line in (BASE / "SHA256SUMS").read_text().splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([a-zA-Z0-9_.]+)", line)
        need(match is not None, "invalid seal line")
        digest, name = match.groups()
        need(name in FILES and name not in seen, "unexpected seal file")
        need(sha((BASE / name).read_bytes()) == digest, "changed file: " + name)
        seen.add(name)
    need(seen == FILES, "incomplete seal")
    reference = BASE.parents[1] / "receipts/atlas_graph_full_20260911/objects" / REFERENCE_SHA
    need(sha(reference.read_bytes()) == REFERENCE_SHA, "changed reference")
    pins = read("context_pins.json")
    need(pins["reference_sha256"] == REFERENCE_SHA, "reference identity")
    expected = read("result.json")
    need(expected["status"] == "passed_bounded_fused_marks_model", "result status")
    fixed = {
        "runs": 30, "nodes": 184, "marks": 186,
        "BFS_mark_cut_comparisons": 6828, "first_sweep_union_attempts": 70,
        "removed_replay_union_attempts": 124, "mark_find_queries": 186,
        "event_dates": 156, "removed_logical_workspace_bytes_sum": 2928,
        "raw_equivalent_date_runs": 2, "physical_mutants_rejected": 2,
        "shared_input_guards_rejected": 5,
    }
    need(all(expected[key] == value for key, value in fixed.items()), "bounded counters")
    selftest_bytes = []
    for mode in ("o2", "san"):
        capture = read(mode + ".json")
        context = capture["context"]
        need(context["status"] == "passed" and context["failure"] is None, "capture failed")
        need(context["sanitizers"] is (mode == "san"), "sanitizer identity")
        compiler = context["compiler_before"]
        binary = context["executable_after_compile"]
        need(compiler == context["compiler_after"] and compiler["bytes"] > 0, "compiler closure")
        need(binary == context["executable_after_tests"] and binary["bytes"] > 0, "ELF closure")
        for item in (compiler, binary):
            need(re.fullmatch(r"[0-9a-f]{64}", item["sha256"]) is not None, "binary pin")
        source_pins = context["source_pins"]
        sources = {Path(path).name: (path, digest) for path, digest in source_pins.items()}
        need(len(sources) == len(source_pins) == 4, "source identity collisions")
        need(set(sources) == {REFERENCE_SHA, "fused_marks.hpp", "gate.cpp", "record.py"}, "source domain")
        need(sources[REFERENCE_SHA][1] == REFERENCE_SHA, "reference source pin")
        captured_root = Path(sources["record.py"][0]).parent
        for name in ("record.py", "fused_marks.hpp", "gate.cpp"):
            path, digest = sources[name]
            need(Path(path).parent == captured_root, "mixed source roots")
            need(sha((BASE / name).read_bytes()) == digest, "captured source differs")
        need(Path(binary["path"]).parent.parent == captured_root, "ELF target scope")
        commands = capture["commands"]
        labels = ["compiler", "compile", "selftest", "missing", "unknown"]
        need([row["label"] for row in commands] == labels, "command domain")
        need(set(capture["outputs"]) == set(labels), "output domain")
        for row in commands:
            label = row["label"]
            expected_exit = 2 if label in ("missing", "unknown") else 0
            need(row["exit"] == row["expected_exit"] == expected_exit, "command exit")
            need(not row["timed_out"] and row["execution_error"] is None, "command incomplete")
            need(row["timeout_seconds"] == (120 if label == "compile" else 30 if label == "selftest" else 10), "command timeout")
            for channel in ("stdout", "stderr"):
                raw = capture["outputs"][label][channel].encode()
                need(sha(raw) == row[channel + "_sha256"], "output binding")
                if channel == "stderr" or label in ("compile", "missing", "unknown"):
                    need(raw == b"", "unexpected diagnostic")
        need(commands[0]["argv"] == [compiler["path"], "--version"], "compiler command")
        flags = ["-O1", "-g0", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"] if mode == "san" else ["-O2"]
        compile_argv = [compiler["path"], "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
                        '-DMHGP7_AUDIT_CALENDAR_HEADER="' + sources[REFERENCE_SHA][0] + '"',
                        sources["gate.cpp"][0], "-o", binary["path"]]
        need(commands[1]["argv"] == compile_argv, "compile command")
        for row, suffix in zip(commands[2:], (["--selftest"], [], ["--unknown"])):
            need(row["argv"] == [binary["path"], *suffix], "executable identity")
        environment = context["environment"]
        need(environment["TMPDIR"] == str(captured_root / "tmp"), "temporary scope")
        if mode == "san":
            need(environment["ASAN_OPTIONS"] == "detect_leaks=1:abort_on_error=1", "ASan settings")
            need(environment["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "UBSan settings")
        raw = capture["outputs"]["selftest"]["stdout"].encode()
        need(json.loads(raw, object_pairs_hook=unique) == expected, "selftest result differs")
        selftest_bytes.append(raw)
    need(selftest_bytes[0] == selftest_bytes[1], "O2/SAN outputs differ")
    return {"status": "passed_fused_marks_receipt", **fixed,
            "commands_per_build": 5, "product_executed": False,
            "performance_claim": False, "gcp_used": False}


if __name__ == "__main__":
    try:
        print(json.dumps(verify(), sort_keys=True))
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
