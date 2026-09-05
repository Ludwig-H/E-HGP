#!/usr/bin/env python3
"""Read the constructor's closed Trace geometry gate without executing C++."""
import argparse
from datetime import datetime
import hashlib
import json
from math import comb
from pathlib import Path
import posixpath
import sys

HERE = Path(__file__).resolve().parent
AUDITS = HERE.parents[1]
BASE = "/workspaces/E-HGP/build/v7_meb_dual_budget_geometry"
RUN = BASE + "/run_20260905"
PRODUCT = "/workspaces/E-HGP/morsehgp3D_v7"
COMPILER = "/usr/bin/x86_64-linux-gnu-g++-13"
BINARY_SHA = "bccde6fee8d8c57cd6768208d6989d8460f4e4428aec60dce557e38b97bc9844"
LABELS = ("compiler", "compile", "nominal", "charge_after_mutant", "unknown_argument", "extra_argument")
CODES = (0, 0, 0, 4, 2, 2)
COUNTERS = ("ordinals", "scenes", "orders", "main_comparisons", "boundary_comparisons", "reference_rank_calls", "prospective_violations", "proposal_forms", "pair_selections", "legacy_charges", "actual_fallback_candidates", "certified", "fallback", "complete", "degenerate", "capped", "fast_q2", "fast_q3", "fast_q4", "named_fast_q2", "named_fast_q3", "named_fast_q4", "q4_two_pivots", "q4_high_limb", "exhausted_fallback", "initial_p_fallback", "shell_fallback", "forced_fallback", "direct_form_checks", "direct_form_rejected")


def require(value, message):
    if not value:
        raise ValueError(message)


def unique(pairs):
    output = {}
    for key, value in pairs:
        require(key not in output, "duplicate JSON key " + key)
        output[key] = value
    return output


def decode(data):
    return json.loads(data, object_pairs_hook=unique)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def judge_output(data, mutant):
    result = decode(data)
    require(set(result) == set(COUNTERS) | {"schema", "status", "cause", "public_status"}, "exact geometry output fields")
    require(result["schema"] == "mhgp7-private-dual-budget-geometry-v1" and result["public_status"] == "not_claimed", "geometry output scope")
    require(all(type(result[key]) is int and result[key] >= 0 for key in COUNTERS), "nonnegative integer counters, no booleans")
    require(result["status"] == ("causal_violation" if mutant else "passed") and result["cause"] == ("charge_not_prospective" if mutant else "none"), "actual causal classification")
    require(result["ordinals"] == sum(comb(n, q) for n in range(2, 12) for q in range(2, min(n, 4) + 1)) == 1507, "ordinal inventory")
    require(result["scenes"] == 176 and result["orders"] == 2 * 176 + 2 + 6 + 24 == 384, "scene/order inventory")
    require(result["main_comparisons"] == 384 * 8 * 3 == 9216 and result["boundary_comparisons"] == 123 and result["reference_rank_calls"] == 384, "nonvacuum comparison counts")
    require(result["complete"] + result["degenerate"] + result["capped"] == 9339, "terminal partition")
    require(result["direct_form_checks"] == 6 and result["direct_form_rejected"] == 4, "six independent direct forms")
    require(result["prospective_violations"] == (result["proposal_forms"] + 6 if mutant else 0), "causal forms accounting")
    require(all(result[key] > 0 for key in COUNTERS if key != "prospective_violations"), "geometric route nonvacuum")
    require([result["named_fast_q" + str(q)] for q in (2, 3, 4)] == [8, 16, 52], "named extreme fast paths")
    require(result["certified"] + result["fallback"] <= 9339, "exclusive routes")
    fast = sum(result["fast_q" + str(q)] for q in (2, 3, 4))
    require(fast <= min(result["certified"], result["complete"]), "certified distinct from public success")
    require(result["actual_fallback_candidates"] <= result["legacy_charges"] and result["forced_fallback"] <= 5, "derived fallback bounds")
    require(result["q4_high_limb"] <= result["fast_q4"] and result["q4_two_pivots"] <= result["fast_q4"], "q4 witness bounds")
    return result


def inspect(live=False):
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    entries = capture["entries"]
    def raw(key):
        entry = entries[key]
        path = AUDITS / entry["audit_path"]
        require(path.resolve().is_relative_to(AUDITS), "safe audit reference")
        data = path.read_bytes()
        require(sha(data) == entry["sha256"] and len(data) == entry["bytes"], "captured source hash " + key)
        return data
    def obj(key):
        return decode(raw(key))
    for key, entry in entries.items():
        if "audit_path" in entry:
            raw(key)
    receipt = obj("run/receipt.json")
    require(receipt["status"] == "completed" and receipt["errors"] == [] and receipt["schema"] == "mhgp7-private-meb-dual-budget-geometry-capture-v1", "closed geometry receipt")
    require(receipt["scope"] == "private_local_differential_to_F_not_independent_arithmetic_or_pipeline_proof" and receipt["observer_route"] == "Trace_only_NoObserver_not_qualified", "Trace-only differential scope")
    require(receipt["public_status"] == "not_claimed" and receipt["testing_macro"] is False and receipt["sanitizers"] is False and receipt["gcp_used"] is False, "explicit nonpromotion/instrumentation")
    pins = receipt["source_before"]
    require(len(pins) == 66 and pins == receipt["source_after"] == obj("run/inputs_before.json") == obj("run/inputs_after.json") == capture["source_authorities_verified_live"], "66 stable source/authority pins")
    require(receipt["sources_stable"] is True, "source boundary")
    origins = {entry["origin"]: entry for entry in entries.values()}
    for name, entry in origins.items():
        if name in pins:
            require(entry["sha256"] == pins[name], "source captures match compile authority")
    artifacts = receipt["artifacts"]
    require(len(artifacts) == 38 and {"run/" + name for name in artifacts} == {key for key in entries if key.startswith("run/")} - {"run/receipt.json"}, "artifact coverage")
    for name, pin in artifacts.items():
        require(pin == {key: entries["run/" + name][key] for key in ("bytes", "sha256")}, "artifact integrity")
    require(obj("run/binary_before.json") == obj("run/binary_after.json") == {"sha256": BINARY_SHA} and artifacts["geometry_gate"] == {"sha256": BINARY_SHA, "bytes": 118088}, "closed gate binary identity")
    dep = raw("run/dependencies.d").decode().replace("\\\n", " ")
    require(dep.count(":") == 1 and "$" not in dep, "depfile syntax")
    target, names = dep.split(":")
    require(target.strip() == RUN + "/geometry_gate", "actual depfile target")
    paths = [posixpath.normpath(name) for name in names.split()]
    binding = obj("run/dependency_binding.json")
    require(len(paths) == len(set(paths)) == 21 and set(paths) == set(binding), "21 actual compile dependencies")
    require(all(pins[name] == pin == origins[name]["sha256"] for name, pin in binding.items()), "all local dependencies captured or referenced")
    require({BASE + "/geometry_gate.cpp", BASE + "/additional_scenes.inc", PRODUCT + "/src/forest/silent_incidence.hpp", "/workspaces/E-HGP/build/v7_meb_dual_budget_prototype/pivot.hpp", "/workspaces/E-HGP/build/v7_meb_pivot_prototype/pivot.hpp"} <= set(binding), "gate/data/both headers/F Builder dependencies")
    f_build = obj("authority/F_build.json")
    require(f_build["sources_before"] == f_build["sources_after"] and len(f_build["sources_before"]) == 51 and all(pins[PRODUCT + "/" + name] == pin for name, pin in f_build["sources_before"].items()), "included F has own 51-source build")
    plan = obj("plan/additional_scenes.json")
    points = [row["points"] for row in plan["additional_scenes"]]
    require(len(points) == 8 and len({row["name"] for row in plan["additional_scenes"]}) == 8, "eight named added scenes")
    for cloud in points:
        require(2 <= len(cloud) <= 11 and all(len(point) == 3 and all(type(x) is int and 0 <= x <= 65535 for x in point) for point in cloud) and len(set(map(tuple, cloud))) == len(cloud), "added u16 distinct-point domain")
    projection = "".join("  {" + ",".join("{" + ",".join(map(str, point)) + "}" for point in cloud) + "},\n" for cloud in points).encode()
    require(projection == raw("protocol/additional_scenes.inc"), "data-only JSON-to-C++ projection")
    env = obj("run/environment.json")
    require(env["cpu_affinity"] == [0] and env["locale"] == {"LANG": "C", "LC_ALL": "C", "TZ": "UTC"} and not any(env["denied_overrides"].values()), "recorded environment")
    require(env["compiler"] == COMPILER and env["compiler_sha256"] == pins[COMPILER], "compiler authority")
    duration = (datetime.fromisoformat(receipt["finished_at"]) - datetime.fromisoformat(receipt["started_at"])).total_seconds()
    require(receipt["combined_deadline_seconds"] == 60 and 0 < duration < 60, "total receipt boundary")
    expected = [[COMPILER, "--version"], [COMPILER, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-I", PRODUCT, "-MMD", "-MF", RUN + "/dependencies.d", BASE + "/geometry_gate.cpp", "-o", RUN + "/geometry_gate"], [RUN + "/geometry_gate"], [RUN + "/geometry_gate", "--mutant=charge-after"], [RUN + "/geometry_gate", "--unknown"], [RUN + "/geometry_gate", "--mutant=charge-after", "extra"]]
    require(set(receipt["commands"]) == set(LABELS), "six complete commands")
    pids, timeouts, outputs = [], [], {}
    for label, code, argv in zip(LABELS, CODES, expected):
        result = obj("run/" + label + ".result.json")
        attempt = obj("run/" + label + ".attempt.json")
        spawned = obj("run/" + label + ".spawned.json")
        require(result == receipt["commands"][label] and result["argv"] == attempt["argv"] == argv, "raw command/intent/summary")
        require(type(result["returncode"]) is int and result["returncode"] == result["expected_rc"] == attempt["expected_rc"] == code and result["status"] == "completed" and result["error"] is None and result["timed_out"] is False, "exact successful command terminal")
        require(attempt["cpu_affinity"] == [0] and 0 < result["elapsed_seconds"] <= attempt["timeout_seconds"] <= 58, "per-command bound")
        require(type(spawned["pid"]) is int and spawned["pid"] == spawned["pgid"] > 0, "recorded owned process")
        pids.append(spawned["pid"]); timeouts.append(attempt["timeout_seconds"])
        stdout, stderr = raw("run/" + label + ".stdout"), raw("run/" + label + ".stderr")
        require(sha(stdout) == result["stdout_sha256"] and sha(stderr) == result["stderr_sha256"] and stderr == b"", "raw stream hashes")
        if label in ("nominal", "charge_after_mutant"):
            outputs[label] = judge_output(stdout, label == "charge_after_mutant")
        elif label != "compiler":
            require(stdout == b"", "compile/argument output empty")
    require(len(set(pids)) == 6 and all(a > b for a, b in zip(timeouts, timeouts[1:])), "distinct process records, decreasing deadline")
    same = lambda output: {key: value for key, value in output.items() if key not in {"status", "cause", "prospective_violations"}}
    require(same(outputs["nominal"]) == same(outputs["charge_after_mutant"]), "mutant changes only causal result")
    for line in raw("protocol/SHA256SUMS").decode().splitlines():
        digest, name = line.split("  ", 1)
        require(sha(raw("protocol/" + name)) == digest, "preparation protocol hashes")
    local = {"performed": live}
    if live:
        local_pins = {**pins, RUN + "/geometry_gate": BINARY_SHA}
        require(all(sha(Path(name).read_bytes()) == pin for name, pin in local_pins.items()), "current authority drift")
        local["pins_verified"] = len(local_pins)
    return {"status": "constructor_Trace_geometry_gate_closed_verified", "public_status": "not_claimed", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference", "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture", "observer_route": receipt["observer_route"], "started_at": receipt["started_at"], "finished_at": receipt["finished_at"], "commands": dict(zip(LABELS, CODES)), "source_authorities": 66, "local_compile_dependencies": 21, "outputs": outputs, "live": local, "capture_sha256": sha((HERE / "capture_manifest.json").read_bytes()), "auditor_Cpp_or_build_runs": 0, "GCP": "non utilisé", "scope": "Own constructor Trace executions, not transferred triangle or auditor rational geometry results; local differential to shared F arithmetic, no global proof."}


def self_test():
    capture = decode((HERE / "capture_manifest.json").read_bytes())["entries"]
    raw = lambda key: (AUDITS / capture[key]["audit_path"]).read_bytes()
    nominal = decode(raw("run/nominal.stdout")); mutant = decode(raw("run/charge_after_mutant.stdout"))
    judge_output(json.dumps(nominal), False); judge_output(json.dumps(mutant), True)
    changes = {"empty_corpus": {"scenes": 0}, "wrong_ordinal_count": {"ordinals": 1506}, "extra_boundary": {"boundary_comparisons": 129}, "wrong_terminal_total": {"complete": nominal["complete"] - 1}, "bool_counter": {"forced_fallback": True}, "missing_forced_fallback": {"forced_fallback": 0}, "missing_named_q4": {"named_fast_q4": 0}, "missing_high_limb": {"q4_high_limb": 0}, "exact_claim": {"public_status": "exact"}}
    for name, change in changes.items():
        try:
            judge_output(json.dumps({**nominal, **change}), False)
        except ValueError:
            continue
        raise ValueError("surviving reader mutant " + name)
    for name, bad in (("missing_direct_form_causality", {**mutant, "prospective_violations": mutant["proposal_forms"]}), ("zero_causal_violation", {**mutant, "prospective_violations": 0})):
        try:
            judge_output(json.dumps(bad), True)
        except ValueError:
            continue
        raise ValueError("surviving causal mutant " + name)
    try:
        decode('{"status":0,"status":1}')
    except ValueError:
        pass
    else:
        raise ValueError("duplicate JSON accepted")
    return {"status": "passed", "positives": 2, "rejections": sorted(list(changes) + ["missing_direct_form_causality", "zero_causal_violation", "duplicate_JSON"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        print(json.dumps(self_test() if args.self_test else inspect(args.live), indent=2, sort_keys=True))
    except (OSError, KeyError, TypeError, ValueError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        sys.exit(1)
