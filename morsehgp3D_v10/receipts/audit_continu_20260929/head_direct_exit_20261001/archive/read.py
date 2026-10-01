#!/usr/bin/env python3
"""STATIC hash-first archive reader. Never imports recorder/source or executes native code."""
import argparse
import hashlib
import json
import math
import re
import shlex
from collections import Counter
from fractions import Fraction as F
from pathlib import Path

SNAP = ("src/head/head.cpp", "src/head/head.hpp", "src/points/dendrogram.cpp",
        "src/points/dendrogram.hpp", "src/core/status.hpp", "src/core/types.hpp", "src/core/reasons.def")
CODE = tuple("snapshot/" + p for p in SNAP) + ("mutant/head.cpp", "probe.cpp")
BASE = CODE + ("source_pins.json", "README.txt", "protocol.json", "record.py", "read.py")
PAYLOAD = set(BASE) | {"preparation.json", "capture.json", "source_close.json", "record_state.json"}
FLAGS = ["-std=c++20", "-O2", "-frounding-math", "-ffp-contract=off", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
OLD = b"      const double lam = s.lam[d.point_rank[x]];"
NEW = b"      const double lam = s.lam[d.node_rank[v]];"
EXPECTED_PROBE = "932ae7dfc6e4c32c8a11db732c2aadac3f3abd2463ee7c989c571785b3e0ce7a"
EXPECTED_HEAD = "371d1444f35d27217999e2fe64931fb37b22d9d7aed51f7042e2bec58e206193"


def need(c, m):
    if not c:
        raise RuntimeError(m)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def pairs(rows):
    d = {}
    for k, v in rows:
        need(k not in d, "duplicate JSON key")
        d[k] = v
    return d


def obj(raw):
    def bad_number(_s):
        raise RuntimeError("nonfinite JSON number")
    return json.loads(raw, object_pairs_hook=pairs, parse_constant=bad_number)


def plain_int(x, v):
    return type(x) is int and x == v


def component(v):
    need(set(v) == {"path", "sha256", "bytes"}, "component fields")
    need(type(v["path"]) is str and Path(v["path"]).is_absolute(), "component path")
    need(type(v["bytes"]) is int and v["bytes"] > 0, "component size")
    need(type(v["sha256"]) is str and re.fullmatch("[0-9a-f]{64}", v["sha256"]), "component SHA")


def rational(x):
    need(type(x) in (int, float) and math.isfinite(x), "finite numeric value, not bool")
    return F(x)


def close(x, q, tolerance=True):
    actual = rational(x)
    bound = 16 * F(1, 1 << 52) * max(F(1), abs(q)) if tolerance else F(0)
    need(abs(actual - q) <= bound, "independent rational numeric oracle")


def ints(xs, expected):
    need(type(xs) is list and len(xs) == len(expected), "integer vector inventory")
    need(all(plain_int(x, y) for x, y in zip(xs, expected)), "exact integer vector")


def deps(stdout, source_root):
    out = set()
    for line in stdout.replace("\\\n", " ").splitlines():
        need(":" in line, "dependency rule")
        for name in shlex.split(line.split(":", 1)[1]):
            out.add(Path(name).relative_to(Path(source_root)).as_posix())
    return out


def main():
    a = argparse.ArgumentParser()
    a.add_argument("archive")
    a.add_argument("--manifest-sha", required=True)
    args = a.parse_args()
    root = Path(args.archive).absolute()
    need(root.is_dir() and root.resolve() == root, "archive directory / no path symlink")
    entries = list(root.rglob("*"))
    need(all(not p.is_symlink() and (p.is_file() or p.is_dir()) for p in entries), "no symlinks/special entries")
    files = {p.relative_to(root).as_posix() for p in entries if p.is_file()}
    need(files == PAYLOAD | {"manifest.json"}, "closed file inventory")
    raw_manifest = (root / "manifest.json").read_bytes()
    need(re.fullmatch("[0-9a-f]{64}", args.manifest_sha) and sha(raw_manifest) == args.manifest_sha,
         "external manifest SHA before JSON loading")
    manifest = obj(raw_manifest)
    need(manifest["schema"] == "head_direct_exit_closed_v1" and
         manifest["status"] == "recorded_static_reader_not_yet_run", "manifest scope/status")
    need(set(manifest["files"]) == PAYLOAD, "manifest whitelist")
    data = {}
    for n in sorted(PAYLOAD):
        v = manifest["files"][n]
        need(set(v) == {"sha256", "bytes"} and type(v["bytes"]) is int and v["bytes"] >= 0, "payload metadata")
        need(type(v["sha256"]) is str and re.fullmatch("[0-9a-f]{64}", v["sha256"]), "payload SHA type")
        b = (root / n).read_bytes()
        need(len(b) == v["bytes"] and sha(b) == v["sha256"], "payload SHA before analysis: " + n)
        data[n] = b
    # Only now are JSON captures, protocol and source pins interpreted.
    prep = obj(data["preparation.json"])
    need(prep["status"] == "source_only_review_pending" and set(prep["files"]) == set(BASE), "preparation provenance")
    need(all(prep["files"][n] == manifest["files"][n] for n in BASE), "preparation/code closure")
    origin = obj(data["source_pins.json"])
    need(origin["source_stable_before_after"] is True, "original preparation before/after")
    need(set(origin["source_before"]) == set(SNAP), "origin dependencies")
    need(origin["source_before"] == origin["source_after"] == origin["snapshots"], "origin source closure")
    need(all(sha(data["snapshot/" + n]) == origin["snapshots"][n] for n in SNAP), "origin snapshot pins")
    need(sha(data["probe.cpp"]) == EXPECTED_PROBE and sha(data["snapshot/src/head/head.cpp"]) == EXPECTED_HEAD,
         "reviewed input/probe version")
    need(data["snapshot/src/head/head.cpp"].count(OLD) == 1 and
         data["snapshot/src/head/head.cpp"].replace(OLD, NEW, 1) == data["mutant/head.cpp"],
         "exactly one condense mutation; prepare unchanged")
    protocol = obj(data["protocol.json"])
    need(protocol["schema"] == "head_direct_exit_recordable_protocol_v1" and
         protocol["compiler"] == "/usr/bin/g++" and protocol["flags"] == FLAGS, "compiler protocol")
    capture = obj(data["capture.json"])
    source = obj(data["source_close.json"])
    state = obj(data["record_state.json"])
    need(capture["schema"] == "head_direct_exit_capture_v1" and capture["first_failure"] is None, "first failure")
    need(state["status"] == "recorded" and plain_int(state["native_compiles"], 2) and
         plain_int(state["native_runs"], 2), "recorded counts")
    need(source["schema"] == "head_direct_exit_source_close_v1", "source close schema")
    source_root, runtime = capture["source_root"], capture["runtime"]
    need(type(source_root) is str and Path(source_root).is_absolute() and ".." not in Path(source_root).parts,
         "captured source root")
    need(type(runtime) is str and Path(runtime).is_absolute() and runtime != source_root, "fresh runtime path")
    need(capture["runtime_before"] == [] and capture["runtime_after"] == ["baseline", "mutant"], "fresh outputs")
    expected_project = {n: sha(data[n]) for n in CODE}
    need(source["project_before"] == source["project_after"] == expected_project, "compiled source before/after")
    for key in ("compiler_driver", "compiler_backend"):
        component(source[key + "_before"])
        need(source[key + "_before"] == source[key + "_after"], "compiler unchanged")
    names = ["compiler_version", "compiler_backend"]
    for case in ("baseline", "mutant"):
        names += ["dependencies_" + case, "compile_" + case, "run_" + case]
    steps = capture["steps"]
    need(type(steps) is list and [s["stage"] for s in steps] == names, "complete ordered stages")
    by_name = {}
    for row in steps:
        expected_exit = 1 if row["stage"] == "run_mutant" else 0
        need(plain_int(row["exit"], expected_exit) and row["signal"] is None and
             row["timed_out"] is False and row["interruption"] is None,
             "exact exit, no signal/timeout")
        need(plain_int(row["timeout_s"], 5 if row["stage"].startswith("run_") else 10),
             "bounded timeout protocol for every stage")
        need(type(row["stdout"]) is str and type(row["stderr"]) is str, "complete streams")
        need(type(row["seconds"]) in (int, float) and math.isfinite(row["seconds"]) and row["seconds"] >= 0,
             "bounded recorded duration")
        by_name[row["stage"]] = row
    need(by_name["compiler_version"]["argv"] == ["/usr/bin/g++", "--version"] and
         by_name["compiler_backend"]["argv"] == ["/usr/bin/g++", "-print-prog-name=cc1plus"], "compiler argv")
    need("Free Software Foundation" in by_name["compiler_version"]["stdout"], "GNU compiler version captured")
    need(by_name["compiler_backend"]["stdout"].strip() == source["compiler_backend_before"]["path"],
         "backend attribution")
    need(set(source["dependencies"]) == set(source["binaries"]) == {"baseline", "mutant"}, "case inventory")
    for case in ("baseline", "mutant"):
        head = "snapshot/src/head/head.cpp" if case == "baseline" else "mutant/head.cpp"
        units = [source_root + "/" + n for n in ("probe.cpp", head, "snapshot/src/points/dendrogram.cpp")]
        common = ["/usr/bin/g++"] + FLAGS + ["-I" + source_root + "/snapshot/src"]
        need(by_name["dependencies_" + case]["argv"] == common + ["-MM"] + units, "dependencies argv")
        need(by_name["compile_" + case]["argv"] == common + units + ["-o", runtime + "/" + case], "compile argv")
        need(by_name["run_" + case]["argv"] == [runtime + "/" + case], "run argv")
        need(plain_int(by_name["run_" + case]["timeout_s"], 5) and
             plain_int(by_name["compile_" + case]["timeout_s"], 10), "timeout protocol")
        wanted = set(CODE) - {"mutant/head.cpp" if case == "baseline" else "snapshot/src/head/head.cpp"}
        need(deps(by_name["dependencies_" + case]["stdout"], source_root) == wanted, "independent project dep parsing")
        need(source["dependencies"][case]["before"] == source["dependencies"][case]["after"] ==
             {n: expected_project[n] for n in wanted}, "dependency before/after hashes")
        binary = source["binaries"][case]
        need(binary["absent_before_compile"] is True, "binary fresh")
        component({k: v for k, v in binary.items() if k != "absent_before_compile"})
        need(binary["path"] == runtime + "/" + case, "binary metadata attribution")
        lines = by_name["run_" + case]["stdout"].splitlines()
        need(len(lines) == 1, "one complete native JSON observation")
        obs = obj(lines[0])
        need(set(obs) == {"schema", "structural_reason", "domain_reason", "condense_reason", "cluster_reason",
                         "birth", "stability", "point_lambda", "point_cluster", "selected", "label", "cluster_label",
                         "failures", "contract_match"}, "observation inventory")
        need(obs["schema"] == "head-direct-exit-observation-v1", "observation schema")
        need(all(obs[k] == "none" for k in ("structural_reason", "domain_reason", "condense_reason", "cluster_reason")),
             "structural and joint validation accepted")
        need(type(obs["birth"]) is list and len(obs["birth"]) == 3, "birth inventory")
        for x, q in zip(obs["birth"], (F(0), F(1, 2), F(1, 2))):
            close(x, q, False)
        need(type(obs["stability"]) is list and len(obs["stability"]) == 3, "stability inventory")
        close(obs["stability"][0], F(7, 3) if case == "baseline" else F(5, 2), case == "baseline")
        for x in obs["stability"][1:]:
            close(x, F(1), False)
        need(type(obs["point_lambda"]) is list and len(obs["point_lambda"]) == 5, "lambda inventory")
        for x in obs["point_lambda"][:4]:
            close(x, F(1), False)
        close(obs["point_lambda"][4], F(1, 3) if case == "baseline" else F(1, 2), case == "baseline")
        ints(obs["point_cluster"], [1, 1, 2, 2, 0])
        ints(obs["selected"], [0])
        ints(obs["label"], [0, 0, 0, 0, -1] if case == "baseline" else [0, 0, 0, 0, 0])
        ints(obs["cluster_label"], [0, 0, 0])
        need(plain_int(obs["failures"], 0 if case == "baseline" else 5) and
             obs["contract_match"] is (case == "baseline"), "semantic check result")
        diag = [] if case == "baseline" else (
            ["ECHEC head_direct_exit root stability 7/3"] * 2 +
            ["ECHEC head_direct_exit direct root point lambda 1/3"] * 2 +
            ["ECHEC head_direct_exit four leaf points, direct root point noise"])
        need(Counter(by_name["run_" + case]["stderr"].splitlines()) == Counter(diag), "causal semantic diagnostics")
    checks = capture["checkpoints"]
    check_labels = ["initial"]
    check_steps = [0]
    for i, name in enumerate(names):
        check_labels += ["before_" + name, "after_" + name]
        check_steps += [i, i + 1]
    need([c["label"] for c in checks] == check_labels + ["complete"],
         "checkpoint inventory")
    need([c["steps"] for c in checks] == check_steps + [8], "checkpoint progress")
    for n in PAYLOAD:
        need(sha((root / n).read_bytes()) == manifest["files"][n]["sha256"], "archive changed during read")
    need((root / "manifest.json").read_bytes() == raw_manifest, "manifest changed during read")
    print(json.dumps({"scope": "STATIC archive/code/capture validation; NO native reexecution",
                      "native_reexecuted": False, "cases": 2, "payloads": len(PAYLOAD),
                      "baseline_exit": 0, "mutant_exit": 1, "engine_FULL_G4_claim": False}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
