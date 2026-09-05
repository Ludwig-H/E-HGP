#!/usr/bin/env python3
"""Inspect the closed private triangle dual-budget gate, never execute it."""
import argparse
from datetime import datetime
import hashlib
import json
import posixpath
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
AUDITS = HERE.parents[1]
BASE = "/workspaces/E-HGP/build/v7_meb_dual_budget_prototype"
RUN = BASE + "/run_20260905"
PRODUCT = "/workspaces/E-HGP/morsehgp3D_v7"
COMPILER = "/usr/bin/x86_64-linux-gnu-g++-13"
BINARY_SHA = "6a96390fac144fd8f02f7316f7e8d20e515ca61d8711bd66a7692a2c41beccd6"
F_SHA = "ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85"
LABELS = ("compiler", "compile", "nominal", "charge_after_mutant", "unknown_argument", "extra_argument")
EXIT_CODES = (0, 0, 0, 4, 2, 2)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def unique(items):
    result = {}
    for key, value in items:
        require(key not in result, "duplicate key " + key)
        result[key] = value
    return result


def decode(data):
    return json.loads(data, object_pairs_hook=unique)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def output_verdict(stdout, mutant):
    text = stdout.decode()
    require(text.endswith("\n") and len(text.splitlines()) == 11, "eleven complete output lines")
    rows = []
    for line in text.splitlines()[:8]:
        require(line.startswith("triangle "), "triangle line prefix")
        pairs = re.findall(r"(\w+)=(\d+)", line)
        row = unique([(key, int(value)) for key, value in pairs])
        require(set(row) == {"P", "L", "proposal_forms", "legacy", "fallback", "terminal_equal", "prospective_violations"}, "triangle field set")
        require(line == "triangle " + " ".join(key + "=" + value for key, value in pairs), "no unparsed triangle text")
        rows.append(row)
    require([(row["P"], row["L"]) for row in rows] == [(p, l) for p in (0, 1, 4, 5) for l in (1, 4)], "eight distinct P/L cases")
    for row in rows:
        require(row["proposal_forms"] == row["P"] and row["legacy"] == row["L"] and row["terminal_equal"] == 1, "literal proposal/legacy/terminal counters")
        require(row["fallback"] == int(row["P"] < 5), "fallback only below complete proposal")
        require(row["prospective_violations"] == (row["P"] if mutant else 0), "actual prospective discrepancy per case")
    require(text.splitlines()[8] == "cumulative calls=4 P=7 L=12 proposal_forms=7 F_fallback_candidates=8 legacy=12", "cumulative marker")
    require(text.splitlines()[9] == "near_max proposal_increment=1 legacy_increment=4 overflow=0", "unsigned boundary marker")
    final = re.fullmatch(r"dual_budget accounting=reference_ordinal_plus_proposal_v1 prospective_violations=(\d+) public_status=not_claimed", text.splitlines()[10])
    require(final is not None, "accounting/public scope")
    total = int(final.group(1))
    require(total == (28 if mutant else 0), "aggregate causal violations")
    return {"triangle_rows": rows, "triangle_cases": 8, "cumulative_calls": 4, "near_MAX_cases": 1, "prospective_violations": total}


def depfile_paths(data):
    text = data.decode().replace("\\\n", " ")
    require(text.count(":") == 1 and "$" not in text, "supported depfile syntax")
    target, body = text.split(":")
    require(target.strip() == RUN + "/dual_budget_gate", "depfile target")
    paths = [posixpath.normpath(word) for word in body.split()]
    require(len(paths) == len(set(paths)) == 20, "twenty unique local dependencies")
    mandatory = {BASE + "/dual_budget_gate.cpp", BASE + "/pivot.hpp", "/workspaces/E-HGP/build/v7_meb_pivot_prototype/pivot.hpp", PRODUCT + "/src/forest/silent_incidence.hpp"}
    require(mandatory <= set(paths), "both prototype headers and actual F Builder")
    return paths


def command_verdict(label, result, attempt, spawned, stdout, stderr, code):
    require(type(result["returncode"]) is int and result["returncode"] == result["expected_rc"] == code, "exact command return " + label)
    require(result["status"] == "completed" and result["error"] is None and result["timed_out"] is False, "terminal command " + label)
    require(result["argv"] == attempt["argv"] and attempt["expected_rc"] == code and attempt["cwd"] == "/workspaces/E-HGP" and attempt["cpu_affinity"] == [0], "intent/command binding " + label)
    require(0 < result["elapsed_seconds"] <= attempt["timeout_seconds"] <= 58, "command deadline " + label)
    require(type(spawned["pid"]) is int and spawned["pid"] == spawned["pgid"] > 0, "recorded owned process group")
    require(sha(stdout) == result["stdout_sha256"] and sha(stderr) == result["stderr_sha256"] and stderr == b"", "raw command streams " + label)
    if label in ("compile", "unknown_argument", "extra_argument"):
        require(stdout == b"", "empty compile/refusal output")
    if label == "nominal":
        return output_verdict(stdout, False)
    if label == "charge_after_mutant":
        return output_verdict(stdout, True)
    return None


def inspect(live=False):
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    entries = capture["entries"]

    def raw(key):
        entry = entries[key]
        require("audit_path" in entry, "private binary is not in portable capture")
        path = AUDITS / entry["audit_path"]
        require(path.resolve().is_relative_to(AUDITS), "capture path confined to audits")
        data = path.read_bytes()
        require(sha(data) == entry["sha256"] and len(data) == entry["bytes"], "captured pin " + key)
        return data

    def obj(key):
        return decode(raw(key))

    for key, entry in entries.items():
        if "audit_path" in entry:
            raw(key)
    receipt = obj("run/receipt.json")
    require(receipt["schema"] == "mhgp7-private-meb-dual-budget-gate-v1" and receipt["status"] == "completed" and receipt["errors"] == [], "closed successful gate receipt")
    require(receipt["scope"] == "private_triangle_budget_fixture_not_global_q3_q4_or_pipeline_qualification" and receipt["public_status"] == "not_claimed" and receipt["meb_work_accounting"] == "reference_ordinal_plus_proposal_v1", "bounded scope")
    require(receipt["testing_macro"] is False and receipt["sanitizers"] is False and receipt["gcp_used"] is False, "build scope")
    require(receipt["source_before"] == receipt["source_after"] == obj("run/inputs_before.json") == obj("run/inputs_after.json") == capture["source_authorities_verified_live"], "source boundaries and independent capture")
    require(len(receipt["source_before"]) == 61 and receipt["sources_stable"] is True, "61 stable source/authority pins")
    for entry in entries.values():
        if entry["origin"] in receipt["source_before"]:
            require(entry["sha256"] == receipt["source_before"][entry["origin"]], "captured authority byte binding")
    artifacts = receipt["artifacts"]
    require(len(artifacts) == 38 and {"run/" + name for name in artifacts} == {key for key in entries if key.startswith("run/")} - {"run/receipt.json"}, "38-artifact manifest exhaustive")
    for name, pin in artifacts.items():
        entry = entries["run/" + name]
        require(pin["sha256"] == entry["sha256"] and pin["bytes"] == entry["bytes"], "artifact/capture binding")
    require(obj("run/binary_before.json") == obj("run/binary_after.json") == {"sha256": BINARY_SHA}, "tested binary before/after")
    require(artifacts["dual_budget_gate"] == {"sha256": BINARY_SHA, "bytes": 79080}, "gate identity")
    dep_paths = depfile_paths(raw("run/dependencies.d"))
    binding = obj("run/dependency_binding.json")
    require(set(dep_paths) == set(binding) and all(receipt["source_before"][path] == pin for path, pin in binding.items()), "real depfile agrees with authority pins")
    origins = {entry["origin"]: entry for entry in entries.values()}
    require(all(path in origins and origins[path]["sha256"] == pin for path, pin in binding.items()), "all twenty actual dependency sources preserved or referenced")
    f_build = obj("authority/F_build.json")
    require(f_build["binary"]["sha256"] == F_SHA and f_build["sources_before"] == f_build["sources_after"] and len(f_build["sources_before"]) == 51, "F preserved with own build")
    require(all(receipt["source_before"][PRODUCT + "/" + name] == pin for name, pin in f_build["sources_before"].items()), "F includes current F source closure")
    elapsed = (datetime.fromisoformat(receipt["finished_at"]) - datetime.fromisoformat(receipt["started_at"])).total_seconds()
    require(receipt["combined_deadline_seconds"] == 60 and 0 < elapsed < 60, "recorded total boundary")
    env = obj("run/environment.json")
    require(env["cpu_affinity"] == [0] and env["locale"] == {"LANG": "C", "LC_ALL": "C", "TZ": "UTC"} and not any(env["denied_overrides"].values()), "recorded compile/loader environment")
    require(env["compiler"] == COMPILER and env["compiler_sha256"] == receipt["source_before"][COMPILER], "compiler pin")
    expected_argv = {
        "compiler": [COMPILER, "--version"],
        "compile": [COMPILER, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-I", PRODUCT, "-MMD", "-MF", RUN + "/dependencies.d", BASE + "/dual_budget_gate.cpp", "-o", RUN + "/dual_budget_gate"],
        "nominal": [RUN + "/dual_budget_gate"],
        "charge_after_mutant": [RUN + "/dual_budget_gate", "--mutant=charge-after"],
        "unknown_argument": [RUN + "/dual_budget_gate", "--unknown"],
        "extra_argument": [RUN + "/dual_budget_gate", "--mutant=charge-after", "extra"],
    }
    require(set(receipt["commands"]) == set(LABELS), "six commands")
    judged = {}
    timeouts, pids = [], []
    for label, code in zip(LABELS, EXIT_CODES):
        result = obj("run/" + label + ".result.json")
        attempt = obj("run/" + label + ".attempt.json")
        spawned = obj("run/" + label + ".spawned.json")
        require(receipt["commands"][label] == result and result["argv"] == expected_argv[label], "exact recorded argv " + label)
        verdict = command_verdict(label, result, attempt, spawned, raw("run/" + label + ".stdout"), raw("run/" + label + ".stderr"), code)
        if verdict is not None:
            judged[label] = verdict
        timeouts.append(attempt["timeout_seconds"])
        pids.append(spawned["pid"])
    require(len(set(pids)) == 6 and all(a > b for a, b in zip(timeouts, timeouts[1:])), "six distinct processes and declining deadline")
    require("13.3.0" in raw("run/compiler.stdout").decode(), "real compiler version output")
    for line in raw("protocol/SHA256SUMS").decode().splitlines():
        digest, name = line.split("  ", 1)
        require(sha(raw("protocol/" + name)) == digest, "historical preparation checksums")
    geometry = obj("geometry_plan/pins.json")
    require(geometry["status"] == "design_authorities_only_future_gate_and_runner_not_written" and geometry["historical_results_reused"] is False, "geometry plan, not a completed gate")
    require(geometry["files_relative_to_repository"]["build/v7_meb_dual_budget_prototype/run_20260905/receipt.json"] == sha(raw("run/receipt.json")), "geometry plan references actual closed triangle receipt")
    local = {"performed": live}
    if live:
        pins = dict(receipt["source_before"])
        pins[RUN + "/dual_budget_gate"] = BINARY_SHA
        mismatches = [name for name, pin in pins.items() if sha(Path(name).read_bytes()) != pin]
        require(not mismatches, "current local authority drift " + str(mismatches))
        local.update({"pins_verified": len(pins), "mismatches": []})
    return {"schema": "mhgp7-dual-budget-independent-review-v1", "status": "closed_triangle_budget_gate_verified", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference", "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture", "public_status": "not_claimed", "started_at": receipt["started_at"], "finished_at": receipt["finished_at"], "commands": dict(zip(LABELS, EXIT_CODES)), "sources_and_authorities": 61, "local_compile_dependencies": 20, "preserved_artifacts_excluding_binary": 37, "private_binary_preservation": "hash_only_not_copied", "source_boundary_binding": "recorded_not_hermetic", "nominal_and_mutant": judged, "geometry_gate_status": "preparation_only_in_captured_protocol", "local": local, "auditor_Cpp_build_or_engine_runs": 0, "GCP": "non utilisé", "capture_sha256": sha((HERE / "capture_manifest.json").read_bytes()), "limits": ["One triangle, eight P/L cases, four cumulative calls and one injected UINT64 boundary per invocation; no q4 scene.", "Terminals, legacy statistics and literal balls are checked by the compiled fixture against the included F Builder; no independent geometric oracle is supplied by this reader.", "Intermediate cumulative states and the fallback-candidate total are enforced or derived in the gate source; stdout is not a per-call/per-F-form trace.", "MMD closes local non-system sources, not the toolchain and system-header environment hermetically.", "The accounting caps support candidates, not elapsed time, pair search, power tests, memory, or total tower cost.", "The retained README and preparation describe their earlier noncompiled state, superseded only for this triangle gate by the closed receipt."]}


def self_test():
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    def data(key):
        return (AUDITS / capture["entries"][key]["audit_path"]).read_bytes()
    nominal, mutant = data("run/nominal.stdout"), data("run/charge_after_mutant.stdout")
    output_verdict(nominal, False)
    output_verdict(mutant, True)
    cases = {
        "empty": (b"", False),
        "missing_triangle": (b"\n".join(nominal.split(b"\n")[1:]), False),
        "duplicate_triangle": (nominal.replace(nominal.splitlines()[1], nominal.splitlines()[0]), False),
        "terminal_divergence": (nominal.replace(b"terminal_equal=1", b"terminal_equal=0", 1), False),
        "counter_unpaid": (nominal.replace(b"proposal_forms=4", b"proposal_forms=3", 1), False),
        "counter_reset": (nominal.replace(b"proposal_forms=7", b"proposal_forms=5"), False),
        "MAX_overflow": (nominal.replace(b"overflow=0", b"overflow=1"), False),
        "mutant_loses_causal_signal": (mutant.replace(b"prospective_violations=28", b"prospective_violations=0"), True),
        "nominal_not_prospective": (mutant, False),
        "exact_claim": (nominal.replace(b"public_status=not_claimed", b"public_status=exact"), False),
    }
    for name, args in cases.items():
        try:
            output_verdict(*args)
        except ValueError:
            continue
        raise ValueError("surviving parser mutant " + name)
    try:
        decode('{"status":0,"status":1}')
    except ValueError:
        pass
    else:
        raise ValueError("duplicate JSON survived")
    dep = data("run/dependencies.d")
    depfile_paths(dep)
    try:
        depfile_paths(dep.replace((PRODUCT + "/src/forest/silent_incidence.hpp").encode(), b"/fake/other.hpp"))
    except ValueError:
        pass
    else:
        raise ValueError("missing F dependency survived")
    result = decode(data("run/charge_after_mutant.result.json"))
    attempt = decode(data("run/charge_after_mutant.attempt.json"))
    spawned = decode(data("run/charge_after_mutant.spawned.json"))
    command_verdict("charge_after_mutant", result, attempt, spawned, mutant, b"", 4)
    for label, change in (("mutant_wrong_exit", {"returncode": 0}), ("command_failed", {"status": "failed"})):
        try:
            command_verdict("charge_after_mutant", {**result, **change}, attempt, spawned, mutant, b"", 4)
        except ValueError:
            continue
        raise ValueError("surviving command mutant " + label)
    return {"status": "passed", "positive_outputs": 2, "positive_depfile": 1, "positive_mutant_command": 1, "rejections": sorted(list(cases) + ["duplicate_JSON", "missing_F_dependency", "mutant_wrong_exit", "command_failed"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        print(json.dumps(self_test() if args.self_test else inspect(args.live), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        sys.exit(1)
