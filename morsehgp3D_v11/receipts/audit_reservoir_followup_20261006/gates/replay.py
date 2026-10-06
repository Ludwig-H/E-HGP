#!/usr/bin/env python3
"""Bounded source replay; no native child and no modification of the frozen proof."""
import argparse
import ast
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess


HERE = Path(__file__).resolve().parent


def need(value, message):
    if not value:
        raise ValueError(message)


def function(source, name, namespace):
    nodes = [n for n in ast.parse(source).body if isinstance(n, ast.FunctionDef) and n.name == name]
    need(len(nodes) == 1, "one source function " + name)
    tree = ast.Module(body=nodes, type_ignores=[])
    exec(compile(tree, name, "exec"), namespace)
    return namespace[name]


def verdict(check, mode):
    try:
        result = check(mode)
    except ValueError as error:
        return {"accepted": False, "error": str(error)}
    need(result == mode, "optimization returns its argument")
    return {"accepted": True}


def build():
    manifest = json.loads((HERE / "source_manifest.json").read_text())
    pin = manifest["pin"]
    sources = {}
    for path, identity in manifest["sources"].items():
        raw = subprocess.check_output(["git", "show", pin + ":" + path])
        need(hashlib.sha256(raw).hexdigest() == identity["sha256"], "source hash " + path)
        sources[path] = raw.decode()
    probe = sources["morsehgp3D_v11/bench/full_probe.cpp"]
    campaign = sources["morsehgp3D_v11/bench/full_campaign.py"]
    io = sources["morsehgp3D_v11/tests/tower/full_bench_io.py"]
    limit = re.findall(r"optimizations > ([0-9]+)\)\) return 2;", probe)
    need(len(limit) == 1, "one C++ maximum")
    maximum = int(limit[0])
    need(maximum == 262143, "new maximum")
    expected_guards = [
        "if ((optimizations & 8192) != 0 && (optimizations & 8) == 0) return 2;",
        "if ((optimizations & 128) != 0 && (optimizations & 8) == 0) return 2;",
        "if ((optimizations & 16384) != 0 && (optimizations & 2048) == 0) return 2;",
        "if ((optimizations & (32768 | 65536)) != 0 && (optimizations & (64 | 2048)) != (64 | 2048)) return 2;",
        "if ((optimizations & 32768) != 0 && (optimizations & 65536) != 0) return 2;",
    ]
    for guard in expected_guards:
        need(probe.count(guard) == 1, "exact C++ argument guard")
    need(probe.count('std::cout << "{\\\"phase\\\":\\\"exit\\\","; status(result);') == 1,
         "unconditional exit event after run")
    dictionaries = [n for n in ast.walk(ast.parse(io)) if isinstance(n, ast.Dict)
                    and any(isinstance(k, ast.Constant) and k.value == "opt_large" for k in n.keys)]
    need(len(dictionaries) == 1, "one refusal options map")
    old_sentinel = int(ast.literal_eval(dictionaries[0])["opt_large"])
    need(old_sentinel == 131072, "stale refusal sentinel")
    need("semantic.need(not rows, 'usage refuse avant execution')" in io, "no-event refusal requirement")
    namespace = {}
    function(sources["morsehgp3D_v11/bench/catalogue_semantic.py"], "need", namespace)
    old_check = function(campaign, "optimization", namespace)
    updated_campaign = campaign.replace("0 <= value <= 131071", "0 <= value <= 262143")
    updated_campaign = updated_campaign.replace("outside 0..131071", "outside 0..262143")
    new_namespace = {"need": namespace["need"]}
    new_check = function(updated_campaign, "optimization", new_namespace)
    updated_io = io.replace("'opt_large':'131072'", "'opt_large':'262144'")
    patch = ""
    for path, before, after in [
        ("morsehgp3D_v11/bench/full_campaign.py", campaign, updated_campaign),
        ("morsehgp3D_v11/tests/tower/full_bench_io.py", io, updated_io),
    ]:
        patch += "".join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                            fromfile="a/" + path, tofile="b/" + path))
    need(patch == (HERE / "proposal.patch").read_text(), "exact two-line proposal")
    modes = [131072, 180219, 212987, 262144]
    checks = {}
    for mode in modes:
        cpp = (0 <= mode <= maximum and
               (not mode & 8192 or bool(mode & 8)) and
               (not mode & 128 or bool(mode & 8)) and
               (not mode & 16384 or bool(mode & 2048)) and
               (not mode & (32768 | 65536) or mode & (64 | 2048) == (64 | 2048)) and
               not (mode & 32768 and mode & 65536))
        before, after = verdict(old_check, mode), verdict(new_check, mode)
        need(before["accepted"] is False, "old Python rejects replay/out-of-range")
        need(cpp == (mode != 262144), "C++ model boundary")
        need(after["accepted"] == cpp, "corrected Python/C++ policy agree")
        checks[str(mode)] = {"cpp_argument_model_accepts": cpp, "python_exact_before": before,
                             "python_exact_after_proposal": after}
    need(old_sentinel & (16384 | 32768 | 65536) == 0, "stale sentinel activates no leaf executor")
    return {
        "pin": pin, "native_runs": 0, "cloud_actions": 0,
        "kind": "source contradiction and exact Python function replay, not CTest execution",
        "maximum": maximum, "old_refusal_sentinel": old_sentinel,
        "proposed_refusal_sentinel": maximum + 1,
        "old_sentinel_reaches_run_and_exit_event": True,
        "gate_requires_usage_refusal_without_events": True,
        "argument_checks": checks,
        "proposal_changed_lines": 2,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--emit", action="store_true")
    args = parser.parse_args()
    result = build()
    if args.emit:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        need(result == json.loads((HERE / "proof.json").read_text()), "frozen proof unchanged")
        print("reservoir_gate_source_verdict conforme modes4 changed_lines2 native0 cloud0")


if __name__ == "__main__":
    main()
