#!/usr/bin/env python3
"""Read-only portable host-front receipt verifier; effective under python -O."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import re
import shlex

BASE = Path(__file__).resolve().parent
CAP = BASE / "capture"
HEADER = "morsehgp3D_v7/src/pipeline/witness_front.hpp"
MUTANTS = {
    "index_binding": ("if (!ix.valid || !run.matches_index(ix))", "if (!ix.valid)", "binding.wrong_cloud_rejected_before_query"),
    "generation_reserved": ("if (generation >= ~u64{0} - 1)", "if (generation == ~u64{0})", "generation.reserved_before_first_query"),
    "singleton_stale_work": ("if (ix.nodes.empty()) { *work = {}; return; }", "if (ix.nodes.empty()) return;", "work.singleton_clears_per_call_only"),
}
EXPECTED_STDOUT = "witness_front_gate=passed checks=431010 cases=1728 rows=88560 requests=311968 rejections=15 singleton_reuses=1 backend=cpu_only\n"
PINS = {
    HEADER: "fb6f1bcca0a6dee13d9c786250bdd26e4c388b8d6e7caac615f7a33972937b81",
    "morsehgp3D_v7/src/spindle/witness_batch.hpp": "66f31ead8b358dbbb09274b1a0e4fbfcc4777604827527f5c0b2220c1602cd52",
    "morsehgp3D_v7/tests/witness_front_gate.cpp": "f283bca024d593abc47305c5c40f32af1be47d89ff09d84da9225b7e48f1c4a7",
}


def need(value, reason):
    if not value:
        raise SystemExit("witness_front_receipt: " + reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path):
    return json.loads(path.read_text())


manifest = read_json(BASE / "manifest.json")
actual = {str(path.relative_to(BASE)) for path in BASE.rglob("*") if path.is_file() and path.name != "manifest.json"}
need(actual == set(manifest), "manifest file inventory")
for name, digest in manifest.items():
    relative = PurePosixPath(name)
    need(not relative.is_absolute() and ".." not in relative.parts, "manifest path")
    need(sha(BASE / name) == digest, "manifest pin " + name)
pins = read_json(CAP / "source_before.json")
need(len(pins) == 30 and all(pins[name] == digest for name, digest in PINS.items()), "final active pins")
for name, digest in pins.items():
    need(sha(BASE / "source_snapshot" / name) == digest, "frozen source " + name)
header_text = (BASE / "source_snapshot" / HEADER).read_text()
command_names = {"compiler", "dependencies"}
for kind in ("o2", "san", *MUTANTS):
    expected = dict(pins)
    if kind in MUTANTS:
        old, new, reason = MUTANTS[kind]
        mutated = BASE / "mutants" / kind / "witness_front.hpp"
        need(header_text.count(old) == 1, "unique mutation " + kind)
        need(mutated.read_text() == header_text.replace(old, new), "single causal edit " + kind)
        expected[HEADER] = sha(mutated)
    for suffix in ("_sources_before.json", "_sources_after.json", "_compiled_sources.json"):
        need(read_json(CAP / (kind + suffix)) == expected, "consumed closure " + kind + suffix)
    dep_text = (CAP / (kind + ".d")).read_text().replace("\\\n", " ").split(":", 1)[1]
    names = set()
    tree = "snapshot" if kind in ("o2", "san") else kind
    prefix = "/workspaces/E-HGP/build/v7_witness_front_qualification_20260910/" + tree + "/"
    for spelling in shlex.split(dep_text):
        normalized = posixpath.normpath(spelling)
        need(normalized.startswith(prefix), "compiler dependency outside frozen tree")
        names.add(normalized[len(prefix):])
    need(names == set(expected), "actual compiler dependency inventory " + kind)
    binary = (CAP / (kind + "_binary.sha256")).read_text().strip()
    need(re.fullmatch("[0-9a-f]{64}", binary), "binary identity " + kind)
    command_names.update((kind + "_compile", kind + "_selftest"))
    if kind in ("o2", "san"):
        command_names.add(kind + "_argument")
        need((CAP / "logs" / (kind + "_selftest.stdout")).read_text() == EXPECTED_STDOUT, "nominal nonvacuity " + kind)
        need(not (CAP / "logs" / (kind + "_selftest.stderr")).read_bytes(), "nominal diagnostics " + kind)
    else:
        need(not (CAP / "logs" / (kind + "_selftest.stdout")).read_bytes(), "mutant output " + kind)
        need((CAP / "logs" / (kind + "_selftest.stderr")).read_text() == "witness_front_gate: " + MUTANTS[kind][2] + "\n", "causal mutant diagnostic " + kind)
need({path.stem for path in (CAP / "logs").glob("*.json")} == command_names, "command inventory")
for name in command_names:
    row = read_json(CAP / "logs" / (name + ".json"))
    expected_code = 2 if name.endswith("_argument") else (1 if name.endswith("_selftest") and name[:-9] in MUTANTS else 0)
    need(row["exit_code"] == row["expected_exit_code"] == expected_code, "command code " + name)
    for suffix in ("stdout", "stderr"):
        need(sha(CAP / "logs" / (name + "." + suffix)) == row[suffix + "_sha256"], "command output pin " + name)
    if name.endswith("_compile"):
        need(all(flag in row["argv"] for flag in ("-std=c++20", "-pthread", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-MMD")), "strict compile " + name)
        need(("-fsanitize=address,undefined" in row["argv"]) == (name == "san_compile"), "sanitizer flags")
        need(not (CAP / "logs" / (name + ".stderr")).read_bytes(), "compile diagnostics")
    if name in ("san_selftest", "san_argument"):
        need(row["environment_overrides"] == {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "sanitizer environment")
print("witness_front_receipt=passed nominal=2 invalid_args=2 causal_mutants=3 checks=431010 cases=1728 rejections=15 singleton_reuses=1 backend=cpu_only GCP=not_used")
