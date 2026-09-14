#!/usr/bin/env python3
"""Read-only closure of singleton-root builds/campaigns; optional exclusive report.

Explicitly reuses the frozen measure.py checks and the parent Pool closure.
Run this reader normally and with python3 -O: both execute every receipt mutant.
No C++ binary, compiler, benchmark or documentation command is invoked here.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
PARENT = BASE.parent / "q2_pool_bridge_20260914"
COMMIT = "e3af11a7b2ecba4a71929c7112610ff2618f813e"
RUNNER_SHA = "b3f9e39592004d12ff37d25edd2ca4a5f136f250fb6cb271cdb65c5e7a996eb7"
PARENT_RUNNER_SHA = "3e97507f7f42ec3161f7175c05add82ea5036956860f82e0ec0f50b2019a98e8"
PARENT_CLOSURE_SHA = "6bedfd9d4dc5cf91b4a53e0e1379ea1d05d86ec6816eeda6cc591891364c36d0"
PLAN_ROWS = {"pilot": 9, "check50k": 3, "repeat50k": 3, "growth": 6,
             "separation": 6, "clusters": 3}
SAN_ENV = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def finite(value):
    # Also rejects NaN/Infinity nested in arrays or previously unchecked fields.
    json.dumps(value, allow_nan=False)


def read_json(path):
    result = json.loads(path.read_text())
    finite(result)
    return result


def load_runner():
    require(sha(BASE / "measure.py") == RUNNER_SHA, "changed singleton-root runner")
    require(sha(PARENT / "measure.py") == PARENT_RUNNER_SHA, "changed parent runner")
    spec = importlib.util.spec_from_file_location("small_roots_measure", BASE / "measure.py")
    require(spec is not None and spec.loader is not None, "runner import unavailable")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def strict_row(row, runner):
    finite(row)
    require(row.get("timed_out") is False and row.get("deferred_signals") == [],
            "interrupted or timed-out receipt cannot be promoted")
    runner.check(row)
    result = row["result"]
    require(result["scope"] == "audit_adapter_q2_stream_not_full", "changed q2 scope")
    require(result["mode"] == "pool-pair" and result["cutoff"] == 64, "changed Pool policy")
    require(result["digest"]["encoding"] == "canonical_q2_support_v2", "changed support encoding")

    def counters(value):
        if isinstance(value, dict):
            for child in value.values():
                counters(child)
        elif isinstance(value, list):
            for child in value:
                counters(child)
        else:
            require(type(value) is int and 0 <= value < 2**64, "invalid unsigned counter")

    for key in ("front_work", "census_work", "pool_work", "order_work", "sibling_work"):
        counters(result[key])
    for key in ("n", "kmax", "s", "small_roots", "input_rectangles", "anchor_queries",
                "candidate_pairs", "accepted_pairs", "rejected_pairs"):
        counters(result[key])


def build_checks(pin, parent_build):
    reports, builds, gates = {}, {}, {}
    for name, sanitized, passed in (("r1", False, True), ("san_r1", True, False),
                                    ("san_r2", True, True)):
        path = BASE / (name + "_BUILD.json")
        record = read_json(path)
        require(record["schema"] == "mhgp8_audit_small_roots_build_v1" and
                record["source_commit"] == COMMIT and record["sanitizer"] is sanitized and
                record["status"] == ("passed" if passed else "failed") and
                record["audit_adapter"] is True and record["gcp_used"] is False and
                record["public_status"] == "not_claimed", "wrong build authority: " + name)
        archive = BASE / (name + "_sources.zip")
        require(record["archive"] == str(archive.relative_to(ROOT)) and
                sha(archive) == record["archive_sha256"], "changed build archive: " + name)
        with zipfile.ZipFile(archive) as zipped:
            members = zipped.namelist()
            require(len(members) == len(set(members)) and set(members) == set(record["source_sha256"]),
                    "archive entries differ from manifest")
            for source, digest in record["source_sha256"].items():
                require(hashlib.sha256(zipped.read(source)).hexdigest() == digest,
                        "changed archived source: " + source)
                if source.startswith("morsehgp3D_v8/src/") or source == "morsehgp3D_v8/bench/front_fixtures.hpp":
                    require(parent_build["source_sha256"].get(source) == digest,
                            "product source differs from pinned e3 parent: " + source)
        for source, digest in record["audit_inputs"].items():
            require(record["source_sha256"].get(source) == digest and sha(ROOT / source) == digest,
                    "changed audit input: " + source)
            pin(ROOT / source)
        snapshot = BASE / ".snapshot" / name
        compiler = "clang++" if sanitized else "g++"
        common = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                  "-I", str(snapshot / "morsehgp3D_v8/src"),
                  "-I", str(snapshot / "morsehgp3D_v8/bench"),
                  "-I", str(snapshot / PARENT.relative_to(ROOT))]
        common += (["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
                   if sanitized else ["-O2", "-DNDEBUG"])
        cpp = [str(snapshot / p) for p in sorted(record["source_sha256"])
               if p.startswith("morsehgp3D_v8/src/") and p.endswith(".cpp") and
               not p.endswith("/q2_census.cpp")]
        gate, probe = BASE / ".build" / name / "gate", BASE / ".build" / name / "probe"
        own = snapshot / BASE.relative_to(ROOT)
        expected = [(common + [str(own / "gate.cpp")] + cpp + ["-o", str(gate)], 0),
                    ([str(gate)], 0)]
        if not sanitized:
            expected += [(common + [str(own / "probe.cpp")] + cpp + ["-o", str(probe)], 0),
                         ([str(probe)], 1)]
        steps = record["steps"]
        require(len(steps) == len(expected), "missing/unexpected build step")
        for step, (command, code) in zip(steps, expected):
            require(step["command"] == command and step["expected_returncode"] == code,
                    "build command/receipt mismatch")
        require(steps[0]["returncode"] == 0 and not steps[0]["stderr"], "gate compilation failed")
        require(record["gate_environment"] == (SAN_ENV if sanitized else {}), "changed gate environment")
        if passed:
            require(all(step["returncode"] == code for step, (_, code) in zip(steps, expected)),
                    "unsuccessful recorded build/gate")
            require(not steps[1]["stderr"], "diagnostic during successful gate")
            require(sha(gate) == record["gate_sha256"], "changed gate binary")
            gates[name] = json.loads(steps[1]["stdout"])
            finite(gates[name])
            pin(gate)
            if not sanitized:
                require(not steps[2]["stderr"] and not steps[3]["stdout"] and
                        steps[3]["stderr"].startswith("usage: probe "), "probe build/usage rejection differs")
                require(record["binary"] == str(probe.relative_to(ROOT)) and
                        sha(probe) == record["binary_sha256"], "changed measurement binary")
                pin(probe)
        else:
            require(steps[1]["returncode"] != 0 and "LeakSanitizer" in steps[1]["stderr"] and
                    "ptrace" in steps[1]["stderr"], "lost initial LeakSanitizer/ptrace failure")
        reports[name] = {"status": record["status"], "sanitizer": sanitized,
                         "receipt_sha256": sha(path), "archive_sha256": sha(archive)}
        builds[name] = record
        pin(path)
        pin(archive)
    require(builds["r1"]["source_sha256"] == builds["san_r1"]["source_sha256"] ==
            builds["san_r2"]["source_sha256"], "functional sources changed between builds")
    require(gates["r1"] == gates["san_r2"], "Release/sanitizer gate outputs differ")
    gate = gates["r1"]
    require(gate["status"] == "passed" and gate["scope"] == "bounded_audit_singleton_roots" and
            gate["fixtures"] == 10 and gate["configurations"] == 120 and gate["pipeline_runs"] == 360 and
            gate["global_work_comparisons"] == 120 and gate["two_site_policy_checks"] == 12 and
            gate["supports"] > 1000 and gate["extra_shell_supports"] > 100 and
            gate["oracle_rejections"] > 100 and gate["singleton_roots"] > 0 and
            gate["selected_rectangles"] > 0 and gate["filtered_pairs"] > 0, "gate lost bounded coverage")
    return reports, gate


def old_results(pin, runner):
    closure_path = PARENT / "r1_VALIDATION.json"
    require(sha(closure_path) == PARENT_CLOSURE_SHA, "changed published parent closure")
    closure = read_json(closure_path)
    require(closure["status"] == "passed" and closure["normal_and_optimized_identical"] is True,
            "parent closure is not qualified")
    closed_pins = closure["checks"]["pins"]
    old = {}
    for directory in sorted(PARENT.glob("campaign_*")):
        for name in ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json"):
            path = directory / name
            require(closed_pins.get(str(path.relative_to(ROOT))) == sha(path),
                    "published parent campaign changed")
            pin(path)
        runner.parent.validate(directory)
        for line in (directory / "MEASURES.jsonl").read_text().splitlines():
            row = json.loads(line)
            finite(row)
            if row["key"][3] != "pool-pair":
                continue
            key = tuple(row["key"][:3])
            result = runner.discrete(row["result"])
            require(key not in old or old[key] == result, "parent repeat changed work/output")
            old[key] = result
    build_path = PARENT / "r2_BUILD.json"
    require(closed_pins.get(str(build_path.relative_to(ROOT))) == sha(build_path), "changed parent build")
    pin(build_path)
    pin(closure_path)
    return old, read_json(build_path)


def mutations(rows, runner):
    seed = next(row for row in rows if row["key"][3] == "complement")
    reports = []

    def reject(name, operation):
        try:
            operation()
        except (RuntimeError, ValueError, KeyError, TypeError) as error:
            reports.append({"name": name, "rejected_by": str(error)})
        else:
            raise RuntimeError("undetected receipt mutant: " + name)

    edits = (
        ("wrong_policy", lambda r: r.update(root_policy="global-pair")),
        ("wrong_root_population", lambda r: r.update(small_roots=r["small_roots"] + 1)),
        ("nested_nan", lambda r: r["front_work"]["size_class_pair_mass"].__setitem__(0, float("nan"))),
        ("wrong_count", lambda r: r["census_work"].__setitem__("count_node_visits", r["census_work"]["count_node_visits"] + 1)),
        ("lost_front_pair", lambda r: r["front_work"]["residual_pair_mass"].__setitem__(0, r["front_work"]["residual_pair_mass"][0] + 1)),
        ("lost_shell", lambda r: r["digest"].__setitem__("shell_ids", r["digest"]["shell_ids"] + 1)),
    )
    for name, edit in edits:
        row = copy.deepcopy(seed)
        edit(row["result"])
        row["stdout"] = json.dumps(row["result"])
        reject(name, lambda: strict_row(row, runner))
    for name, field, value in (("timed_out_promoted", "timed_out", True),
                               ("signal_promoted", "deferred_signals", [15]),
                               ("failed_status", "status", "failed")):
        row = copy.deepcopy(seed)
        row[field] = value
        reject(name, lambda: strict_row(row, runner))
    key = seed["key"][:3]
    trio = [copy.deepcopy(row) for row in rows if row["key"][:3] == key][:3]
    require(len(trio) == 3 and {row["key"][3] for row in trio} == set(runner.POLICIES),
            "no complete mutation seed")
    # A digest mutation can preserve all local ledgers: pairing must reject it.
    changed = next(row for row in trio if row["key"][3] == "global-pair")
    changed["result"]["digest"]["sum"] = "0" if changed["result"]["digest"]["sum"] != "0" else "1"
    changed["stdout"] = json.dumps(changed["result"])
    for row in trio:
        strict_row(row, runner)
    reject("paired_shell_digest", lambda: runner.check_pairing(trio))
    return reports


def check():
    runner = load_runner()
    pins = {}

    def pin(path):
        pins[str(path.relative_to(ROOT))] = sha(path)

    old, parent_build = old_results(pin, runner)
    builds, gate = build_checks(pin, parent_build)
    require(set(runner.PLANS) == set(PLAN_ROWS) and
            all(len(runner.PLANS[p]) == count for p, count in PLAN_ROWS.items()), "changed campaign plan")
    directories = sorted(BASE.glob("campaign_*"))
    require({p.name for p in directories} == {"campaign_" + p for p in PLAN_ROWS},
            "missing or extra campaign; all six complete plans are required")
    summaries, rows, distinct, matches = [], [], {}, 0
    for directory in directories:
        summary = runner.validate(directory)
        require(summary["rows"] == PLAN_ROWS[summary["plan"]], "wrong campaign population")
        summaries.append(summary)
        manifest = read_json(directory / "MANIFEST.json")
        require(manifest["schema"] == "mhgp8_audit_small_roots_campaign_v1" and
                manifest["build_receipt"] == str((BASE / "r1_BUILD.json").relative_to(ROOT)) and
                manifest["gcp_used"] is False and manifest["public_status"] == "not_claimed" and
                manifest["warmups"] == 0 and manifest["shared_host"] is True and
                manifest["timeout_seconds"] == 180 and
                manifest["timeout_policy"] == "kill_drain_preserve_failure" and
                manifest["signal_policy"] == "defer_until_capture_then_fail" and
                manifest["selected_cpu"] in manifest["allowed_cpus"], "changed measurement protocol")
        for source, digest in manifest["pins"].items():
            require(sha(ROOT / source) == digest, "changed campaign input")
            pin(ROOT / source)
        for name in ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json"):
            pin(directory / name)
        for line in (directory / "MEASURES.jsonl").read_text().splitlines():
            row = json.loads(line)
            strict_row(row, runner)
            rows.append(row)
            key = tuple(row["key"])
            discrete = runner.discrete(row["result"])
            # Unlike cross-policy pairing, a repeated policy must also retain
            # its exact cursor/structure work, capacity and all other counters.
            require(key not in distinct or distinct[key] == discrete, "repeat changed discrete result")
            distinct[key] = discrete
            if key[3] == "complement":
                require(old.get(key[:3]) == discrete, "Complement differs from immutable parent Pool/pairs")
                matches += 1
    require(len(rows) == 30 and len(distinct) == 27 and matches == 10, "incomplete final comparison matrix")
    receipt_mutants = mutations(rows, runner)
    pin(BASE / "measure.py")
    pin(PARENT / "measure.py")
    pin(Path(__file__))
    require(all(sha(ROOT / path) == digest for path, digest in pins.items()), "source changed during validation")
    return {"status": "passed", "schema": "mhgp8_audit_small_roots_validation_v1",
            "scope": "bounded_oracle_and_paired_q2_receipts_not_full", "public_status": "not_claimed",
            "gcp_used": False, "builds": builds, "gate": gate, "campaigns": summaries,
            "rows": len(rows), "configurations": len(distinct), "parent_matches": matches,
            "repeat50k_policies": 3, "receipt_mutants": receipt_mutants, "pins": pins}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="write a new report; refuse overwriting an existing path")
    args = parser.parse_args()
    stream = args.output.open("x") if args.output else None
    try:
        report = check()
    except BaseException as error:
        if stream:
            json.dump({"status": "failed", "schema": "mhgp8_audit_small_roots_validation_v1",
                       "error_type": type(error).__name__, "error": str(error)}, stream,
                      indent=2, sort_keys=True, allow_nan=False)
            stream.write("\n")
        raise
    else:
        encoded = json.dumps(report, indent=2, sort_keys=True, allow_nan=False) + "\n"
        if stream:
            stream.write(encoded)
        sys.stdout.write(encoded)
    finally:
        if stream:
            stream.close()


if __name__ == "__main__":
    # Importing the frozen reader is deliberately free of __pycache__ writes.
    sys.dont_write_bytecode = True
    main()
