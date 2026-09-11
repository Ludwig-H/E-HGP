#!/usr/bin/env python3
"""Portable, non-executing judge for three closed static WSPD-factor captures."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys

BINARY_SHA = "f71f31190f5d50ac70f8332f969c6baa50549536bd08836702e6ba01ae7fcdce"
HEADER_SHA = "33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209"
INPUT_SHA = "b73744755477b18a5853084851075bb4e3e468ae7d1353c98d0991576a099639"
PAYLOAD_SHA = "cdd77e308f765d643ce6707ba0f613f94b4cf560124c0079ee8bfa58ca0c329b"
MATCH_FIELDS = (
    "schema", "status", "public_status", "backend", "contract_qualified", "n",
    "kmax", "threads", "seed", "coord", "orders", "input_digest", "payload_digest",
    "static_threads", "private_static_prototype", "resolver_cache_accounting",
    "residence_accounting", "nodes", "parent_refs", "contributions", "vertical_refs",
    "anchor_blocks", "representatives", "singleton_lots", "grouped_lots", "lot_dsu_slots",
    "lower_edges_indexed", "lower_nodes_activated", "lower_edges_activated", "lower_queries",
    "lower_find_steps", "lower_path_writes",
)


def need(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Files:
    def __init__(self, root: Path):
        self.root = root
        self.manifest = None
        self.entries = None
        if (root / "manifest.json").exists():
            self.manifest = json.loads((root / "manifest.json").read_text())
            need(self.manifest["schema"] == "mhgp7-static-s-factors-capture-v1", "manifest_schema")
            need(self.manifest["public_status"] == "not_claimed"
                 and self.manifest["GCP_used"] is False, "manifest_authority")
            entries = self.manifest["files"]
            self.entries = {row["path"]: row for row in entries}
            need(len(self.entries) == len(entries) and len(entries) > 75,
                 "manifest_unique_nonempty")
            for name, row in self.entries.items():
                path = PurePosixPath(name)
                need(not path.is_absolute() and ".." not in path.parts, "manifest_relative_path")
                need(re.fullmatch(r"[0-9a-f]{64}", row["sha256"]) is not None,
                     "manifest_digest_shape")
                need(row["object"] == "objects/" + row["sha256"], "manifest_object_path")
                data = self.read(name)
                need(len(data) == row["bytes"] and digest(data) == row["sha256"],
                     "manifest_object_integrity:" + name)
                need(not data.startswith(b"\x7fELF"), "ELF_forbidden")
            need(self.read("verify.py") == Path(__file__).read_bytes(), "reader_self_pin")

    def names(self) -> list[str]:
        if self.entries is not None:
            return sorted(self.entries)
        return [p.relative_to(self.root).as_posix()
                for p in sorted(self.root.rglob("*")) if p.is_file()]

    def read(self, name: str) -> bytes:
        if self.entries is not None:
            need(name in self.entries, "missing_manifest_file:" + name)
            return (self.root / self.entries[name]["object"]).read_bytes()
        return (self.root / name).read_bytes()

    def json(self, name: str):
        return json.loads(self.read(name))


def static_counts(row: dict) -> dict[str, int]:
    orders = row["static_orders"]
    need([item["K"] for item in orders] == list(range(1, 11)), "all_ten_static_orders")
    need(orders[0] == dict(K=1, requests=0, unique=0, seeded_unique=0), "K1_is_excluded")
    need(all(0 <= item["seeded_unique"] <= item["unique"] <= item["requests"]
             for item in orders), "static_counts_order")
    counts = {name: sum(item[name] for item in orders)
              for name in ("requests", "unique", "seeded_unique")}
    need(counts["requests"] > counts["unique"] > counts["seeded_unique"] > 0,
         "static_counts_nonvacuity")
    need(row["anchor_hits"] == counts["unique"] - counts["seeded_unique"],
         "unseeded_unique_anchor_work")
    need(row["resolver_meb_calls"] == row["anchor_hits"] + row["intruder_queries"],
         "MEB_initial_and_descent_identity")
    need(row["resolver_meb_calls"] > 0 and row["resolver_supports_tested"] > 0,
         "real_geometric_work")
    need(row["static_worker_threads_created_in_completed_pools"] == 36
         and row["static_lanes_used"] == 36, "nine_pools_four_workers")
    need(all(row[name] == 0 for name in row if name.startswith("resolver_cache_")
             and name != "resolver_cache_accounting"), "no_nominal_cache_in_static_mode")
    return counts


def main() -> None:
    need(len(sys.argv) <= 2, "one_optional_packet_path")
    root = Path(sys.argv[1]) if len(sys.argv) == 2 else Path(__file__).resolve().parent
    files = Files(root)
    if files.manifest is not None:
        checks = files.json("reader_checks/receipt.json")
        need(checks["status"] == "passed" and checks["Cpp_producer_mutants"] is False,
             "closed_reader_checks")
        commands = checks["commands"]
        need([row["name"] for row in commands] ==
             ["normal", "optimized", "faults_normal", "faults_optimized"],
             "normal_and_optimized_reader_commands")
        for command in commands:
            need(command["exit_code"] == 0, "reader_command_exit")
            need(("-O" in command["argv"]) == ("optimized" in command["name"]),
                 "reader_optimization_configuration")
            for stream in ("stdout", "stderr"):
                need(digest(files.read("reader_checks/" + command["name"] + "." + stream))
                     == command[stream + "_sha256"], "reader_check_stream_pin")
            result = files.json("reader_checks/" + command["name"] + ".stdout")
            if command["name"].startswith("faults_"):
                need(result["status"] == "reader_json_boundary_faults_rejected"
                     and len(result["positive"]) == 1 and len(result["rejected"]) == 6
                     and result["Cpp_producer_mutants"] is False, "six_reader_only_faults")
            else:
                need(result["status"] == "verified_static_s_factors_closed"
                     and [row["s"] for row in result["results"]] == [8, 10, 12],
                     "closed_reader_positive")
    source_pins = {name.removeprefix("source/"): digest(files.read(name))
                   for name in files.names() if name.startswith("source/")}
    need(source_pins["morsehgp3D_v7/src/forest/full_ball_tower.hpp"] == HEADER_SHA,
         "qualified_header_pin")
    need(len(source_pins) > 50, "complete_frozen_source_capture")
    need(files.read("original_micro_binary.sha256").decode().strip() == BINARY_SHA,
         "original_binary_pin")
    need(files.json("original_micro_receipt.json")["status"] == "passed", "closed_micro_origin")
    original_commands = files.json("original_micro_commands.json")
    need(len(original_commands) == 7 and original_commands[0]["name"] == "compile"
         and all(row["exit_code"] == 0 for row in original_commands), "closed_original_micro_commands")
    for when in ("before", "after"):
        need(files.json(f"reproduction/original_micro_sources_{when}.json") == source_pins,
             "original_compiled_source_before_after")
    for stream in ("stdout", "stderr"):
        need(digest(files.read("reproduction/original_micro_compile." + stream))
             == original_commands[0][stream + "_sha256"], "original_compile_stream_pin")
    reproduction = files.json("reproduction/receipt.json")
    need(reproduction["status"] == "passed"
         and reproduction["role"] == "post_run_reproduction_observation"
         and reproduction["no_retroactive_compile_environment_stability_claim"] is True
         and reproduction["dependency_vendor_payload_included"] is False,
         "reproduction_metadata_scope")
    need(files.json("reproduction/boost_package.json")["boost_version"] == 108300,
         "reproduction_boost_package")
    for command in reproduction["commands"]:
        need(command["exit_code"] == 0, "reproduction_command_exit")
        for stream in ("stdout", "stderr"):
            need(digest(files.read("reproduction/" + command["name"] + "." + stream))
                 == command[stream + "_sha256"], "reproduction_command_stream_pin")
    compile_argv = original_commands[0]["argv"]
    need(all(flag in compile_argv for flag in
             ("-O3", "-DNDEBUG", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread")),
         "original_strict_compile_flags")
    reference_meta = files.json("reference_copy.json")
    reference_pins = {name.removeprefix("reference_s8/"): digest(files.read(name))
                      for name in files.names() if name.startswith("reference_s8/")}
    need(reference_meta["copied_after_closure"] is True
         and reference_meta["original_pins"] == reference_pins, "s8_closed_copy_integrity")
    reference = files.json("reference_s8/stdout")
    summaries = []
    for factor in (8, 10, 12):
        prefix = "reference_s8" if factor == 8 else f"n8000_s{factor}_static4"
        receipt = files.json(prefix + "/receipt.json")
        need(receipt["status"] == "completed" and receipt["error"] is None
             and receipt["sources_and_binary_stable"] is True
             and receipt["exit_code"] == 0 and receipt["GCP_used"] is False
             and receipt["shared_host"] is True and receipt["public_status"] == "not_claimed",
             "closed_capture_receipt")
        for when in ("before", "after"):
            need(files.json(prefix + f"/sources_{when}.json") == source_pins,
                 "source_before_after_frozen")
            need(files.read(prefix + f"/binary_{when}.sha256").decode().strip() == BINARY_SHA,
                 "closed_binary_before_after")
            host = files.json(prefix + f"/host_{when}.json")
            need(all(name in host for name in
                     ("/proc/loadavg", "/proc/meminfo", "/sys/fs/cgroup/cpu.max", "affinity", "time_ns")),
                 "host_load_and_resource_capture")
        command = files.json(prefix + "/command.json")
        need(command["exit_code"] == 0 and command["ended_ns"] > command["started_ns"],
             "closed_command_exit")
        need(command["argv"][:2] == ["/usr/bin/time", "-v"]
             and command["argv"][-5:] == ["--n=8000", f"--s={factor}", "--kmax=10",
                                            "--threads=1", "--static-threads=4"],
             "command_configuration")
        for stream in ("stdout", "stderr"):
            need(digest(files.read(prefix + "/" + stream)) == command[stream + "_sha256"],
                 "closed_raw_stream_hash")
        intent = files.json(prefix + "/intent.json")
        need(intent["argv"] == command["argv"] and intent["algorithmic_timeout"] is None
             and intent["GCP_used"] is False, "no_algorithmic_ceiling_or_cloud")
        progress = files.json(prefix + "/progress.json")
        need(len(progress) >= 2 and all(item["processes"] for item in progress),
             "progress_nonvacuity")
        need(all(b["time_ns"] > a["time_ns"] for a, b in zip(progress, progress[1:])),
             "progress_chronology")
        row = files.json(prefix + "/stdout")
        need(row["status"] == "completed_relative" and row["orders"] == 10
             and row["contract_qualified"] is False and row["public_status"] == "not_claimed"
             and row["backend"] == "cpu_reference", "bounded_authority")
        need(row["s"] == factor and row["n"] == 8000 and row["threads"] == 1
             and row["static_threads"] == 4 and row["seed"] == 3 and row["coord"] == 65536,
             "actual_configuration")
        need(row["input_digest"] == INPUT_SHA and row["payload_digest"] == PAYLOAD_SHA,
             "closed_8k_input_and_full_payload")
        need(all(row[name] == reference[name] for name in MATCH_FIELDS),
             "input_output_calendar_equality")
        counts = static_counts(row)
        stderr = files.read(prefix + "/stderr").decode()
        rss = re.search(r"Maximum resident set size \(kbytes\): (\d+)", stderr)
        user = re.search(r"User time \(seconds\): ([0-9.]+)", stderr)
        cpu = re.search(r"Percent of CPU this job got: ([0-9]+)%", stderr)
        need(rss is not None and user is not None and cpu is not None, "observed_RSS_and_CPU")
        need("stage_complete=full_ball_tower" in stderr and "stage_complete=payload_digest" in stderr,
             "complete_tower_and_digest_stages")
        common = sorted(set(row) & set(reference))
        differences = {name: {"s8": reference[name], "current": row[name]}
                       for name in common if row[name] != reference[name]}
        if factor != 8:
            comparison = files.json(prefix + "/comparison.json")
            need(comparison["compared_fields"] == list(MATCH_FIELDS)
                 and comparison["output_calendar_matched"] is True
                 and comparison["static_totals"] == counts
                 and comparison["static_orders_matched"] == (row["static_orders"] == reference["static_orders"])
                 and comparison["all_common_field_differences"] == differences,
                 "independent_recomputed_comparison")
            need(comparison["upstream_invariance_assumed"] is False
                 and comparison["timing_optimum_claim"] is False
                 and comparison["contract_qualified"] is False, "comparison_bounded_authority")
            need(comparison["fields_only_current"] == sorted(set(row) - set(reference))
                 and comparison["fields_only_reference"] == sorted(set(reference) - set(row)),
                 "comparison_domain_complete")
        summaries.append(dict(s=factor, nodes=row["nodes"], payload_digest=row["payload_digest"],
                              static_totals=counts, total_s=row["total_s"], tower_s=row["tower_s"],
                              MEB=row["resolver_meb_calls"], supports=row["resolver_supports_tested"],
                              raw=row["raw"], unique=row["unique"], balls=row["balls"],
                              sampled_retained_capacity_bytes=row["static_sampled_retained_capacity_peak_bytes"],
                              peak_RSS_KiB=int(rss.group(1)), user_CPU_s=float(user.group(1)),
                              CPU_percent=int(cpu.group(1)),
                              all_differing_fields=sorted(differences)))
    print(json.dumps(dict(status="verified_static_s_factors_closed", results=summaries,
                          output_calendar_fields_compared=len(MATCH_FIELDS),
                          shared_host=True, timing_optimum_claim=False,
                          contract_qualified=False, public_status="not_claimed", GCP_used=False)))


if __name__ == "__main__":
    main()
