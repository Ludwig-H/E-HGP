#!/usr/bin/env python3
"""Read fresh T2 evidence, with concrete log checks and no disabled asserts."""
import argparse
import copy
import hashlib
import itertools
import json
import posixpath
from pathlib import Path

CASES = ("line12", "shell14", "spatial12")
CAPTURES = ("o2_r1", "san_root_r1")
GATE = "0458157768eee3c1d082ebed1f4cff6b75c082f00233b08899a79c08834af4ed"
PINS = {
    "morsehgp3D_v7/src/forest/full_ball_tower.hpp": "6763a877f43d79a45532bce4426feca645b6ee97f18c9a4ee1fea4c47cd408a5",
    "morsehgp3D_v7/tests/full_ball_tower_gate.cpp": "c4f39462487610e32fd5b23210c268a4f7300490aff511252a3cf859afe9cb88",
    "morsehgp3D_v7/bench/full_ball_tower_probe.cpp": "e96f8d36a523c6cfb64848ae5ee86933fef2279050691e6b4f774a430a0f2f46",
}
MUTANTS = {
    "mutant-assignment": "T2 unassigned subset",
    "mutant-open": "T2.historical.exact_Gamma",
    "mutant-adjacency": "T2.historical.exact_Gamma",
    "mutant-census": "T2.census.exhaustive_ball_inventory",
}
SCOPE = dict(public_status="not_claimed", GCP_used=False, device_executed=False,
             performance_contract=False, inherited_results=False, probe_compiled=False)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def validate(manifest, objects):
    need(manifest["schema"] == "mhgp7_t2_post_exchange_cpu_v1", "schema")
    need(manifest["scope"] == SCOPE, "scope")
    files = manifest["files"]
    for name, row in files.items():
        data = objects(row["sha256"])
        need(sha(data) == row["sha256"] and len(data) == row["bytes"], "object.hash")
        need(not data.startswith(b"\x7fELF") and not name.startswith("/") and ".." not in Path(name).parts, "object.safe")

    def read(name):
        need(name in files, "required:" + name)
        return objects(files[name]["sha256"])

    def load(name):
        return json.loads(read(name))

    baseline = load("current/baseline.json")
    need(baseline["expected_active_pins"] == PINS, "active.pins")
    need(files["current/source/t2_gate.cpp"]["sha256"] == GATE, "instrumented.gate")
    for name, pin in baseline["files"].items():
        if name == "t2_gate.cpp":
            continue  # Baseline has the pre-instrumentation judge; new pin above.
        path = "current/source/" + ("source/" if name.startswith("morsehgp3D_v7/") else "") + name
        need(files[path]["sha256"] == pin, "baseline.unchanged:" + name)
    for name, pin in PINS.items():
        need(files["current/source/source/" + name]["sha256"] == pin, "active.source:" + name)
    gate = read("current/source/t2_gate.cpp").decode()
    for marker in ("T2.metadata.order_identity", "T2.metadata.vertical_node_indexed",
                   "T2post.static1_static4_exact_MEB_work", "T2post.static1_static4_exact_descent_work",
                   "T2post.no_temporal_cache", "actual_census(", "build_full_ball_tower(ix, balls, 10, threads)"):
        need(marker in gate, "source.guard:" + marker)
    all_cases = {}
    all_details = {}
    for capture in CAPTURES:
        prefix = "captures/" + capture + "/"
        receipt = load(prefix + "receipt.json")
        need(receipt["status"] == "passed" and receipt["sources_stable"] and
             receipt["sources_before"] == receipt["sources_after"], capture + ".closed_stable")
        for key in ("GCP_used", "device_executed", "inherited_results", "probe_compiled"):
            need(receipt[key] is False, capture + ".scope." + key)
        need(receipt["public_status"] == "not_claimed", capture + ".scope.public")
        need(receipt["binary_before"] == receipt["binary_after"] and
             len(receipt["binary_before"]) == 64, capture + ".binary_stable")
        for name, pin in receipt["sources_before"].items():
            need(files[prefix + "source_snapshot/" + name]["sha256"] == pin == files["current/" + name]["sha256"],
                 capture + ".same_sources")
        commands = receipt["commands"]
        expected = [(name, 0) for name in ("compile", "historical", *CASES, "rejects")]
        expected += [(name, 1) for name in MUTANTS] + [("invalid-args", 2)]
        need([(r["name"], r["exit_code"]) for r in commands] == expected, capture + ".commands")
        for row in commands:
            name = row["name"]
            need(row["expected_exit"] == row["exit_code"] and row["ended_ns"] >= row["started_ns"], capture + ".exit_time")
            for stream in ("stdout", "stderr"):
                need(files[prefix + name + "." + stream]["sha256"] == row[stream + "_sha256"], capture + ".log_pin")
            intent = load(prefix + name + ".intent.json")
            need(intent == dict(argv=row["argv"], expected_exit=row["expected_exit"],
                 causal_diagnostic=row["diagnostic"]), capture + ".intent")
            if name in MUTANTS:
                cause = MUTANTS[name]
                need(row["diagnostic"] == cause and row["diagnostic_matched"] and
                     cause in read(prefix + name + ".stderr").decode(), capture + ".causal_mutant")
            if name != "compile":
                need(row["argv"] == [commands[0]["argv"][-1], "--" + ("invalid" if name == "invalid-args" else name)],
                     capture + ".binary_and_mode")
        compile_args = commands[0]["argv"]
        for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-MMD"):
            need(flag in compile_args, capture + ".strict")
        need(compile_args[-3].endswith("/" + capture + "/source_snapshot/source/t2_gate.cpp"), capture + ".compiled_snapshot")
        need(not any("MUTANT=" in arg for arg in compile_args), capture + ".nominal_source")
        if capture == "san_root_r1":
            need(receipt["mode"] == "san" and "-fsanitize=address,undefined" in compile_args and
                 "-fno-sanitize-recover=all" in compile_args and receipt["sanitizer_environment"] ==
                 dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"),
                 "san.no_disabled_sanitizers")
        else:
            need(receipt["mode"] == "o2" and "-O2" in compile_args, "o2.flags")
        dependencies = read(prefix + "gate.d").decode().replace("\\\n", " ").split()[1:]
        normalized = [posixpath.normpath(item) for item in dependencies]
        snapshot_prefix = compile_args[-3].split("/source/t2_gate.cpp")[0] + "/"
        for path in normalized:
            need(path.startswith(snapshot_prefix), capture + ".private_dependencies")
            need(prefix + "source_snapshot/" + path.removeprefix(snapshot_prefix) in files, capture + ".dep_present")
        for suffix in ("/tests/full_ball_tower_gate.cpp", "/src/forest/full_ball_tower.hpp"):
            need(any(path.endswith(suffix) for path in normalized), capture + ".active_consumed")
        need(not any(path.endswith("/bench/full_ball_tower_probe.cpp") for path in normalized), capture + ".probe_not_consumed")
        need(load(prefix + "historical.stdout") == dict(status="passed", oracle_mebs=1022, oracle_components=14724),
             capture + ".historical_nonvacuity")
        need(load(prefix + "rejects.stdout") == dict(status="passed", rejections=9), capture + ".rejects")
        all_cases[capture] = {case: load(prefix + case + ".stdout") for case in CASES}
        all_details[capture] = {}
        for case, result in all_cases[capture].items():
            need(result["status"] == "passed" and result["scope"] == "bounded_real_census_FULL_K1_K10", case + ".scope")
            need((result["clouds"], result["orders"], result["census_runs"], result["physical_tower_pairs"],
                  result["post_static_towers"], result["post_work_pairs"]) == (2,180,6,16,12,6), case + ".shape")
            details = load(prefix + case + ".post_exchange.json")
            all_details[capture][case] = details
            stderr = read(prefix + case + ".stderr").decode().splitlines()
            for tag, key in (("T2POST_ORDER ", "orders"), ("T2POST_TOWER ", "towers")):
                need(details[key] == [json.loads(line.removeprefix(tag)) for line in stderr if line.startswith(tag)],
                     case + ".detail_from_stderr")
            order_map = {(r["variant"],r["s"],r["threads"],r["K"]):r for r in details["orders"]}
            tower_map = {(r["variant"],r["s"],r["threads"]):r for r in details["towers"]}
            need(len(details["orders"]) == 180 and set(order_map) == set(itertools.product((0,1),(8,10,12),(0,1,4),range(1,11))),
                 case + ".orders_exhaustive_no_duplicates")
            need(len(details["towers"]) == 18 and set(tower_map) == set(itertools.product((0,1),(8,10,12),(0,1,4))),
                 case + ".towers_exhaustive_no_duplicates")
            for key, tower in tower_map.items():
                rows = [order_map[(*key,k)] for k in range(1,11)]
                for letter in ("Q","H","T"):
                    need(tower[letter] == sum(r[letter] for r in rows), case + ".tower_order_sums")
                for row in rows:
                    work = [row[name] for name in ("R","U","initial_seeds","Q","H","T")]
                    need(all(type(value) is int and value >= 0 for value in work), case + ".unsigned_work")
                    if key[2] == 0:
                        need(work == [0]*6, case + ".cache_no_static_work")
                    else:
                        need(row["initial_seeds"] <= row["U"] <= row["R"] and row["H"] == row["T"] <= row["Q"] and
                             row["H"] <= row["U"] - row["initial_seeds"], case + ".order_partition")
                if key[2]:
                    need(tower["Q"] == tower["descending_steps"] + tower["same_radius_steps"] and
                         tower["anchor_hits"] + tower["T"] == sum(r["U"]-r["initial_seeds"] for r in rows) and
                         tower["meb_calls"] == tower["anchor_hits"] + tower["intruder_queries"], case + ".work_partition")
                if key[2] == 4:
                    one = dict(tower_map[(*key[:2],1)]); four = dict(tower)
                    del one["threads"], four["threads"]
                    need(one == four, case + ".static_tower_work_equal")
                    for k in range(1,11):
                        one = dict(order_map[(*key[:2],1,k)]); four = dict(order_map[(*key,k)])
                        del one["threads"], four["threads"]
                        need(one == four, case + ".static_order_work_equal")
            for field, letter in (("post_seed_queries","Q"),("post_seed_hits","H"),("post_seed_terminals","T")):
                need(result[field] == sum(r[letter] for r in details["towers"]), case + ".case_sums")
        totals = {field: sum(row[field] for row in all_cases[capture].values()) for field in receipt["campaign_totals"]}
        need(totals == receipt["campaign_totals"] == dict(orders=540, census_runs=18, physical_tower_pairs=48,
             cuts=13000, vertical_checks=8103948, post_seed_queries=228, post_seed_hits=120, post_seed_terminals=120,
             post_static_towers=36, post_work_pairs=18), capture + ".exact_nonvacuity")
        need(receipt["post_seed_coverage"] == "actual_nonzero_hits", capture + ".hit_nonvacuity")
    need(all_cases["o2_r1"] == all_cases["san_root_r1"] and all_details["o2_r1"] == all_details["san_root_r1"],
         "o2_san_exact_outputs_and_work")
    return dict(status="passed", logical_files=len(files), towers_per_build=54, Q=228, H=120, T=120,
                static_work_pairs_per_build=18, cases=all_cases["o2_r1"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("packet", nargs="?", default=str(Path(__file__).resolve().parent))
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    packet = Path(args.packet)
    raw = (packet / "manifest.json").read_bytes()
    manifest = json.loads(raw)
    need((packet / "manifest.sha256").read_text() == sha(raw) + "  manifest.json\n", "manifest.sidecar")
    objects = lambda pin: (packet / "objects" / pin).read_bytes()
    result = validate(manifest, objects)
    if args.selftest:
        rejected = 0
        for fault in range(6):
            altered = copy.deepcopy(manifest)
            overlay = {}
            if fault == 0:
                altered["scope"]["device_executed"] = True
            else:
                path = "captures/" + ("san_root_r1" if fault == 1 else "o2_r1") + "/receipt.json"
                receipt = json.loads(objects(altered["files"][path]["sha256"]))
                if fault == 1:
                    receipt["sanitizer_environment"]["ASAN_OPTIONS"] = "detect_leaks=0"
                elif fault == 2:
                    receipt["campaign_totals"]["post_seed_hits"] = 0
                elif fault == 3:
                    receipt["commands"][6]["diagnostic_matched"] = False
                elif fault == 4:
                    receipt["probe_compiled"] = True
                else:
                    path = "captures/o2_r1/spatial12.post_exchange.json"
                    receipt = json.loads(objects(altered["files"][path]["sha256"]))
                    receipt["orders"][0]["K"] = 2
                data = (json.dumps(receipt, sort_keys=True) + "\n").encode()
                pin = sha(data)
                overlay[pin] = data
                altered["files"][path] = dict(sha256=pin, bytes=len(data))
            try:
                validate(altered, lambda pin: overlay[pin] if pin in overlay else objects(pin))
            except (KeyError, ValueError):
                rejected += 1
        need(rejected == 6, "reader.faults")
        result["reader_only_faults_rejected"] = rejected
    result["manifest_sha256"] = sha(raw)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
