#!/usr/bin/env python3
"""Read-only closure of serial front continuations, with in-memory receipt mutants.

No compiler, C++ binary, benchmark or source-changing command is executed. Git
is read only to compare archived product sources with the declared commit.
An optional --output creates one report exclusively; it never overwrites one.
This checks captured evidence, not workers, census callbacks or a speedup claim.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
PARENT = BASE.parent / "q2_pool_bridge_20260914"
COMMIT = "ba11e3abfe686d8078c13bacfc066e372d6bfc10"
AUTHORITIES = {"r1": False, "san_r1": True}
INPUTS = [("single_000000", 8000), ("single_000100", 8000),
          ("single_000200", 8000), ("single_000000", 16000),
          ("single_000000", 32000), ("single_000000", 50000)]
WIDTHS = (0, 1, 64, 256)
MATRIX = [[dataset, n, width] for dataset, n in INPUTS for width in WIDTHS]
SAN_ENV = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
HELPER_PINS = {
    BASE.parent / "q2_small_roots_20260914/measure.py":
        "b3f9e39592004d12ff37d25edd2ca4a5f136f250fb6cb271cdb65c5e7a996eb7",
    PARENT / "measure.py":
        "3e97507f7f42ec3161f7175c05add82ea5036956860f82e0ec0f50b2019a98e8",
    PARENT / "support_io.hpp":
        "2bfcf29394e321c2bea9c409545aab3df94b6093c7e6bfcb76109d7554e4387f",
}
SCALARS = set("""product_visits diagonal_splits diagonal_leaves disjoint_splits
separation_tests witness_searches witness_descent_steps witness_box_distance_tests
proposed_sites proposals_in_factors h_bound_tests xi_bound_tests witness_lane_credits
fully_rejected_products emitted_rectangles emitted_factor_sites max_factor_size
leaf_pair_rectangles max_stack_size max_product_depth""".split())
ARRAYS = {"size_class_rectangles": 5, "size_class_pair_mass": 5,
          "rejected_pair_mass": 3, "residual_pair_mass": 3, "lane_rectangles": 3}
JOB_FIELDS = set("a b mask depth seed_mass product_visits witness_descent_steps "
                 "rectangles pair_mass elapsed_ms".split())


class Rejected(RuntimeError):
    """A named closure failure; also the only expected mutant outcome."""


def require(condition, code):
    if not condition:
        raise Rejected(code)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def sha(path):
    return digest(path.read_bytes())


def finite(value):
    try:
        json.dumps(value, allow_nan=False)
    except (ValueError, TypeError) as error:
        raise Rejected("nonfinite_json") from error


def object_pairs(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate_json_key")
        result[key] = value
    return result


def loads(text):
    value = json.loads(text, object_pairs_hook=object_pairs)
    finite(value)
    return value


def read(path):
    return loads(path.read_text())


def uint(value):
    require(type(value) is int and 0 <= value < 2**64, "invalid_counter")


def duration(value):
    require(type(value) in (float, int) and math.isfinite(value) and value >= 0,
            "invalid_duration")


def work(value):
    require(type(value) is dict and set(value) == SCALARS | set(ARRAYS), "work_schema")
    for key in SCALARS:
        uint(value[key])
    for key, size in ARRAYS.items():
        require(type(value[key]) is list and len(value[key]) == size, "work_array")
        for counter in value[key]:
            uint(counter)


def input_path(dataset, n):
    return BASE.parent / "lidar08_20260914/prepared" / dataset / f"n{n}.u16le"


def strict_row(row):
    finite(row)
    require(row.get("status") == "completed" and type(row.get("returncode")) is int and
            row["returncode"] == 0 and row.get("stderr") == "" and
            row.get("timed_out") is False and row.get("deferred_signals") == [],
            "interrupted_or_failed_row")
    require(row["key"] in MATRIX and type(row["key"][1]) is int and
            type(row["key"][2]) is int, "undeclared_width_or_input")
    _, n, width = row["key"]
    result = loads(row["stdout"])
    require(result["status"] == "completed" and
            result["schema"] == "mhgp8_front_tasks_probe_v1" and
            result["scope"] == "serial_front_only_no_census_no_parallel_speedup" and
            result["public_status"] == "not_claimed" and result["gcp_used"] is False and
            result["threads"] == 1 and result["n"] == n and result["width"] == width and
            result["kmax"] == 10 and result["s"] == 8 and
            result["input_fnv64"] == row["input_fnv64"], "result_configuration")
    for field in ("n", "kmax", "s", "width", "threads", "rectangles", "residual_mass",
                  "maximum_ready", "task_bytes", "cloud_coordinate_copies",
                  "cloud_validation_points", "cloud_retained_bytes", "index_retained_bytes"):
        uint(result[field])
    for field in ("load_ms", "cloud_ms", "index_ms", "front_ms", "prefix_ms", "adapter_ms",
                  "validation_destruction_ms", "total_ms"):
        duration(result[field])
    d = result["digest"]
    require(set(d) == {"encoding", "sum", "xor"} and
            d["encoding"] == "oriented_index_rectangles_v1", "digest_encoding")
    for field in ("sum", "xor"):
        require(type(d[field]) is str and 1 <= len(d[field]) <= 16 and
                all(c in "0123456789abcdef" for c in d[field]), "digest_value")
    front, prefix, jobs = result["front_work"], result["prefix_work"], result["jobs"]
    work(front)
    work(prefix)
    require(type(jobs) is list, "jobs_schema")
    total = n * (n - 1) // 2
    require(front["rejected_pair_mass"][0] + front["residual_pair_mass"][0] == total and
            front["residual_pair_mass"][0] == result["residual_mass"] and
            front["emitted_rectangles"] == result["rectangles"] == sum(front["size_class_rectangles"]) and
            sum(front["size_class_pair_mass"]) == result["residual_mass"] and
            front["lane_rectangles"][0] == result["rectangles"] and
            front["xi_bound_tests"] == 0 and front["max_product_depth"] <= 96,
            "front_ledger")
    for field in ("rejected_pair_mass", "residual_pair_mass", "lane_rectangles"):
        require(front[field][1:] == [0, 0] and prefix[field][1:] == [0, 0], "inactive_lane")
    require(prefix["emitted_rectangles"] == sum(prefix["size_class_rectangles"]) ==
            prefix["lane_rectangles"][0] and
            prefix["residual_pair_mass"][0] == sum(prefix["size_class_pair_mass"]),
            "prefix_output_ledger")
    require(result["cloud_coordinate_copies"] == result["cloud_validation_points"] == n,
            "repeated_cloud_preparation")
    if width == 0:
        require(not jobs and result["maximum_ready"] == result["task_bytes"] == 0 and
                result["prefix_ms"] == result["adapter_ms"] == 0 and
                all(prefix[k] == 0 for k in SCALARS) and
                all(prefix[k] == [0] * size for k, size in ARRAYS.items()), "public_used_adapter")
    else:
        require(0 < result["maximum_ready"] <= width + 1 and
                len(jobs) <= result["maximum_ready"] and (not jobs or len(jobs) >= width) and
                0 < result["task_bytes"] <= 64 and front["max_stack_size"] <= 97,
                "ready_or_stack_bound")
        for job in jobs:
            require(type(job) is dict and set(job) == JOB_FIELDS, "job_schema")
            for field in JOB_FIELDS - {"elapsed_ms"}:
                uint(job[field])
            duration(job["elapsed_ms"])
            require(job["mask"] == 1, "restored_job_mask")
            require(job["depth"] <= front["max_product_depth"] and job["a"] < 2*n - 1 and
                    job["b"] < 2*n - 1 and job["product_visits"] > 0 and
                    job["pair_mass"] <= job["seed_mass"] <= total, "job_provenance")
        for field, child in (("product_visits", "product_visits"),
                             ("witness_descent_steps", "witness_descent_steps"),
                             ("emitted_rectangles", "rectangles")):
            require(prefix[field] + sum(job[child] for job in jobs) == front[field],
                    "job_ledger_" + field)
        require(prefix["residual_pair_mass"][0] + sum(job["pair_mass"] for job in jobs) ==
                result["residual_mass"], "prefix_output_mass")
        require(prefix["residual_pair_mass"][0] + prefix["rejected_pair_mass"][0] +
                sum(job["seed_mass"] for job in jobs) == total, "prefix_seed_mass")
        require(result["adapter_ms"] + 1e-5 >= result["prefix_ms"] +
                sum(job["elapsed_ms"] for job in jobs), "job_duration_enclosure")
    require(result["total_ms"] + 1e-5 >= result["front_ms"] and
            result["front_ms"] + 1e-5 >= result["adapter_ms"], "duration_enclosure")
    return result


def paired(rows):
    require([row["key"] for row in rows] == MATRIX, "campaign_matrix")
    results = [strict_row(row) for row in rows]
    for offset in range(0, len(results), len(WIDTHS)):
        reference = results[offset]
        expected = {k: v for k, v in reference["front_work"].items() if k != "max_stack_size"}
        for result in results[offset:offset + len(WIDTHS)]:
            actual = {k: v for k, v in result["front_work"].items() if k != "max_stack_size"}
            require(actual == expected and result["digest"] == reference["digest"] and
                    result["rectangles"] == reference["rectangles"], "changed_geometry_or_digest")
    return results


class Closure:
    def __init__(self):
        self.pins = {}

    def pin(self, path, expected=None):
        path = path.resolve()
        require(path.is_relative_to(ROOT), "pin_outside_repository")
        value = sha(path)
        key = str(path.relative_to(ROOT))
        require(expected is None or value == expected, "changed_pin:" + key)
        require(key not in self.pins or self.pins[key] == value, "changed_during_read:" + key)
        self.pins[key] = value
        return value

    def finish(self):
        for path, value in self.pins.items():
            require(sha(ROOT / path) == value, "changed_during_closure:" + path)


def product_sources():
    command = ["git", "ls-tree", "-r", "--name-only", COMMIT, "morsehgp3D_v8/src"]
    paths = subprocess.check_output(command, cwd=ROOT, text=True).splitlines()
    paths = [p for p in paths if p.endswith((".hpp", ".cpp"))]
    paths.append("morsehgp3D_v8/bench/front_fixtures.hpp")
    require(len(paths) > 10 and "morsehgp3D_v8/src/wspd/front.cpp" in paths, "git_sources_vacuous")
    return {p: digest(subprocess.check_output(["git", "show", COMMIT + ":" + p], cwd=ROOT))
            for p in paths}


def build_checks(closure, products):
    own_paths = {BASE / name for name in ("front_tasks.hpp", "probe.cpp", "gate.cpp", "build.py")}
    own_paths |= {PARENT / "support_io.hpp", PARENT / "probe.cpp"}
    own = {str(path.relative_to(ROOT)) for path in own_paths}
    records, gates, history = {}, {}, []
    receipts = sorted(BASE.glob("*_BUILD.json"))
    names = {p.name.removesuffix("_BUILD.json") for p in receipts}
    require(set(AUTHORITIES) <= names, "missing_authoritative_build")
    for receipt in receipts:
        closure.pin(receipt)
        record = read(receipt)
        name = receipt.name.removesuffix("_BUILD.json")
        authoritative = name in AUTHORITIES
        require(record["schema"] == "mhgp8_audit_front_tasks_build_v1" and
                record["source_commit"] == COMMIT and record["audit_adapter"] is True and
                record["public_status"] == "not_claimed" and record["gcp_used"] is False and
                type(record["sanitizer"]) is bool and bool(record["compiler"]) and
                bool(record["started_utc"]) and bool(record["finished_utc"]), "build_metadata")
        require(record["status"] == ("passed" if authoritative else "failed"), "build_authority")
        if authoritative:
            require(record["sanitizer"] is AUTHORITIES[name], "build_compiler_mode")
        archive = BASE / (name + "_sources.zip")
        require(record["archive"] == str(archive.relative_to(ROOT)), "archive_path")
        closure.pin(archive, record["archive_sha256"])
        sources, audit = record["source_sha256"], record["audit_inputs"]
        require(set(audit) == own and set(sources) == own | set(products), "build_source_set")
        require(all(sources[p] == value for p, value in products.items()), "git_product_mismatch")
        with zipfile.ZipFile(archive) as zipped:
            entries = zipped.namelist()
            require(len(entries) == len(set(entries)) and set(entries) == set(sources), "archive_entries")
            for path, value in sources.items():
                require(digest(zipped.read(path)) == value, "archive_content:" + path)
                closure.pin(BASE / ".snapshot" / name / path, value)
        for path, value in audit.items():
            require(sources[path] == value, "audit_archive_disagreement")
            if authoritative:
                closure.pin(ROOT / path, value)
        sanitized = record["sanitizer"]
        snapshot = BASE / ".snapshot" / name
        compiler = "clang++" if sanitized else "g++"
        common = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                  "-I", str(snapshot / "morsehgp3D_v8/src"),
                  "-I", str(snapshot / "morsehgp3D_v8/bench"),
                  "-I", str(snapshot / PARENT.relative_to(ROOT))]
        common += (["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
                   if sanitized else ["-O2", "-DNDEBUG"])
        cpp = [str(snapshot / p) for p in products if p.endswith(".cpp")]
        gate, probe = BASE / ".build" / name / "gate", BASE / ".build" / name / "probe"
        local = snapshot / BASE.relative_to(ROOT)
        expected = [(common + [str(local / "gate.cpp")] + cpp + ["-o", str(gate)], 0),
                    ([str(gate)], 0)]
        if not sanitized:
            expected += [(common + [str(local / "probe.cpp")] + cpp + ["-o", str(probe)], 0),
                         ([str(probe)], 1)]
        steps = record["steps"]
        require(0 < len(steps) <= len(expected), "build_steps_count")
        for step, (command, code) in zip(steps, expected):
            require(step["command"] == command and type(step["expected_returncode"]) is int and
                    step["expected_returncode"] == code, "build_command")
        if not authoritative:
            require(bool(record.get("error_type")) and bool(record.get("error")), "missing_failure_record")
            history.append({"build": name, "status": "failed", "error_type": record["error_type"],
                            "returncodes": [step.get("returncode") for step in steps]})
            continue
        require(len(steps) == len(expected) and all(type(step.get("returncode")) is int and
                step["returncode"] == code for step, (_, code) in zip(steps, expected)), "failed_build_promoted")
        require(record["gate_environment"] == (SAN_ENV if sanitized else {}), "sanitizer_environment")
        require(steps[0]["stdout"] == steps[0]["stderr"] == steps[1]["stderr"] == "", "build_diagnostic")
        closure.pin(gate, record["gate_sha256"])
        gates[name] = loads(steps[1]["stdout"])
        if not sanitized:
            require(steps[2]["stdout"] == steps[2]["stderr"] == steps[3]["stdout"] == "" and
                    steps[3]["stderr"] == "usage: probe input.u16le n Kmax s width (0=public front)\n",
                    "probe_usage_gate")
            require(record["binary"] == str(probe.relative_to(ROOT)), "probe_path")
            closure.pin(probe, record["binary_sha256"])
        records[name] = record
    require(records["r1"]["source_sha256"] == records["san_r1"]["source_sha256"] and
            gates["r1"] == gates["san_r1"], "release_sanitizer_disagreement")
    gate = gates["r1"]
    require(gate["status"] == "passed" and gate["scope"] == "front_only_bounded_continuations",
            "gate_scope")
    for field in ("checks", "clouds", "public_runs", "partition_runs", "rectangles", "oracle_point_tests",
                  "covered_lane_pairs", "rejected_lane_pairs", "inherited_mask_jobs", "positive_depth_jobs",
                  "diagonal_jobs", "prefix_emission_runs", "prefix_and_jobs_runs", "exhausted_prefix_runs",
                  "nonidentity_orders", "invalid_inputs", "callback_exceptions", "model_mutants"):
        uint(gate[field])
        require(gate[field] > 0, "vacuous_gate:" + field)
    require(gate["clouds"] == 18 and gate["public_runs"] == 864 and gate["partition_runs"] == 4320 and
            gate["invalid_inputs"] == 10 and gate["callback_exceptions"] == 2 and gate["model_mutants"] == 4,
            "incomplete_gate")
    return records, gate, history


def campaign_checks(closure, release):
    directory = BASE / "campaign"
    manifest_path, rows_path, done_path = [directory / name for name in
                                         ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json")]
    for path in (manifest_path, rows_path, done_path):
        closure.pin(path)
    manifest, done = read(manifest_path), read(done_path)
    require(done["status"] == "completed" and done["manifest_sha256"] == sha(manifest_path) and
            done["measures_sha256"] == sha(rows_path) and bool(done["finished_utc"]), "campaign_not_closed")
    require(manifest["schema"] == "mhgp8_front_tasks_campaign_v1" and manifest["matrix"] == MATRIX and
            manifest["runner_sha256"] == closure.pin(BASE / "measure.py") and
            manifest["scope"] == "serial_front_only" and manifest["public_status"] == "not_claimed" and
            manifest["gcp_used"] is False and manifest["shared_host"] is True and
            manifest["warmups"] == 0 and manifest["timeout_seconds"] == 180, "campaign_metadata")
    cpus = manifest["allowed_cpus"]
    require(type(cpus) is list and bool(cpus) and all(type(cpu) is int and cpu >= 0 for cpu in cpus) and
            cpus == sorted(set(cpus)) and type(manifest["selected_cpu"]) is int and
            manifest["selected_cpu"] == cpus[-1], "cpu_affinity_contract")
    binary = ROOT / release["binary"]
    mandatory = {BASE / "measure.py", BASE / "r1_BUILD.json", ROOT / release["archive"], binary,
                 *(ROOT / p for p in release["audit_inputs"]), *HELPER_PINS,
                 *(input_path(dataset, n) for dataset, n in INPUTS)}
    require(set(manifest["pins"]) == {str(p.relative_to(ROOT)) for p in mandatory}, "campaign_pin_set")
    for path, value in manifest["pins"].items():
        closure.pin(ROOT / path, value)
    rows = [loads(line) for line in rows_path.read_text().splitlines()]
    require([row["key"] for row in rows] == MATRIX, "campaign_matrix")
    input_pins = {}
    for dataset, n in INPUTS:
        data = input_path(dataset, n).read_bytes()
        require(len(data) == 6*n, "input_size")
        fnv = 14695981039346656037
        for byte in data:
            fnv = ((fnv ^ byte) * 1099511628211) & ((1 << 64) - 1)
        input_pins[(dataset, n)] = (digest(data), format(fnv, "x"))
    for row in rows:
        dataset, n, width = row["key"]
        command = [str(binary), str(input_path(dataset, n)), str(n), "10", "8", str(width)]
        require(row["command"] == command, "campaign_command")
        require((row["input_sha256"], row["input_fnv64"]) == input_pins[(dataset, n)], "input_identity")
    return rows, paired(rows)


def receipt_mutants(rows):
    outcomes = {}
    source = next((row for row in rows if row["key"][2] == 1 and
                   loads(row["stdout"])["jobs"]), None)
    require(source is not None, "mutant_jobs_vacuous")

    def reject(name, row, code):
        try:
            strict_row(row)
        except Rejected as error:
            require(str(error) == code, "unexpected_mutant_rejection:" + name + ":" + str(error))
            outcomes[name] = code
        else:
            raise Rejected("mutant_escaped:" + name)

    changed = copy.deepcopy(source)
    changed["key"][2] = 2
    reject("bad_width", changed, "undeclared_width_or_input")
    for name, field, value, code in (
        ("nan_job_time", "elapsed_ms", float("nan"), "nonfinite_json"),
        ("restored_mask", "mask", 3, "restored_job_mask"),
    ):
        changed = copy.deepcopy(source)
        result = loads(changed["stdout"])
        result["jobs"][0][field] = value
        changed["stdout"] = json.dumps(result)
        reject(name, changed, code)
    changed = copy.deepcopy(source)
    result = loads(changed["stdout"])
    require(result["jobs"][0]["product_visits"] > 1, "mutant_job_visits_vacuous")
    result["jobs"][0]["product_visits"] -= 1
    changed["stdout"] = json.dumps(result)
    reject("lost_job_visit", changed, "job_ledger_product_visits")
    prefix_source = next((row for row in rows if row["key"][2] > 0 and
                          loads(row["stdout"])["prefix_work"]["rejected_pair_mass"][0] > 0), None)
    require(prefix_source is not None, "mutant_prefix_mass_vacuous")
    changed = copy.deepcopy(prefix_source)
    result = loads(changed["stdout"])
    result["prefix_work"]["rejected_pair_mass"][0] -= 1
    changed["stdout"] = json.dumps(result)
    reject("lost_prefix_mass", changed, "prefix_seed_mass")
    for name, field, value in (("timeout_promoted", "timed_out", True),
                               ("signal_promoted", "deferred_signals", [15])):
        changed = copy.deepcopy(source)
        changed[field] = value
        reject(name, changed, "interrupted_or_failed_row")
    require(len(outcomes) == 7, "mutants_incomplete")
    return outcomes


def verify():
    closure = Closure()
    closure.pin(Path(__file__))
    for path, value in HELPER_PINS.items():
        closure.pin(path, value)
    products = product_sources()
    builds, gate, history = build_checks(closure, products)
    rows, results = campaign_checks(closure, builds["r1"])
    mutants = receipt_mutants(rows)
    require(len(results) == 24 and sum(len(result["jobs"]) for result in results) > 0,
            "campaign_vacuous")
    closure.finish()
    return {"schema": "mhgp8_front_tasks_closure_v1", "status": "passed",
            "scope": "captured_serial_front_only_no_census_no_parallel_speedup",
            "public_status": "not_claimed", "gcp_used": False, "source_commit": COMMIT,
            "authoritative_builds": list(AUTHORITIES), "failed_builds_preserved": history,
            "inputs": len(INPUTS), "rows": len(rows), "widths": list(WIDTHS), "gate": gate,
            "receipt_mutants": mutants, "maximum_ready": max(r["maximum_ready"] for r in results),
            "maximum_local_stack": max(r["front_work"]["max_stack_size"] for r in results if r["width"]),
            "closure_sha256": dict(sorted(closure.pins.items()))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="create a report exclusively; never overwrite")
    args = parser.parse_args()
    report = verify()
    encoded = json.dumps(report, sort_keys=True, indent=2, allow_nan=False) + "\n"
    if args.output is not None:
        with args.output.open("x") as stream:
            stream.write(encoded)
    print(encoded, end="")


if __name__ == "__main__":
    try:
        main()
    except (Rejected, OSError, KeyError, TypeError, ValueError, subprocess.SubprocessError) as error:
        print(json.dumps({"status": "failed", "error_type": type(error).__name__, "error": str(error)},
                         sort_keys=True, allow_nan=False), file=sys.stderr)
        sys.exit(1)
