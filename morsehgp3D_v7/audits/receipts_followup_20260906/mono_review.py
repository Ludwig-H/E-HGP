#!/usr/bin/env python3
"""Independently reread published mono captures; no engines or imported judges.

Run from any directory with python3 -B (also -O). Only stdout is written.
Published receipt pins select historical inputs from their archived copies.
This verifies functional field comparisons and reported resource observations,
not catalogue completeness, hardware isolation, latency gains or a FULL tower.
"""
from __future__ import annotations

from datetime import datetime
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
RECEIPTS = ROOT / "morsehgp3D_v7/receipts"
PACKET = RECEIPTS / "full_gabriel_successor_mono_20260905"
PACKETS = [PACKET, RECEIPTS / "full_gabriel_singleton_mono_20260905",
           RECEIPTS / "full_gabriel_lazy_mono_20260905"]
CALENDAR = "full_successor_reads_writes_no_last_pair_v2"
HEADER = "85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad"
BINARY = "8ff0dd10bdc0b43d405abde53809242029ca5094be3c61b04120359af28b0780"
ORDER_MEASURES = set("build_ms digest_ms expand_ms hwm_mib_sample read_ms release_ms rss_mib_sample".split())
TERMINAL_MEASURES = set("compute_read_release_ms_subtracted_diagnostic digest_ms elapsed_before_terminal_ms generation_rects_ms generation_wspd_ms hwm_mib_sample provisional_output_ms rss_mib_sample stage_ms".split())
VERSIONS = {"schema", "successor_accounting"}
PINS: dict[str, str] = {}
INDEX: dict[tuple[str, str], Path] = {}


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def read(path: Path, expected: str | None = None) -> bytes:
    raw = path.read_bytes()
    pin = sha(raw)
    name = str(path.relative_to(ROOT))
    require(expected is None or pin == expected, "pin:" + name)
    require(name not in PINS or PINS[name] == pin, "drift:" + name)
    PINS[name] = pin
    return raw


def unique(items: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in items:
        require(key not in result, "duplicate_key")
        result[key] = value
    return result


def loads(raw: bytes | str) -> object:
    def reject(value: str) -> None:
        raise ValueError("nonfinite:" + value)
    return json.loads(raw, object_pairs_hook=unique, parse_constant=reject)


def obj(path: Path) -> dict:
    value = loads(read(path))
    require(type(value) is dict, "object:" + str(path))
    return value


def equal(left: object, right: object, reason: str) -> None:
    require(json.dumps(left, sort_keys=True) == json.dumps(right, sort_keys=True), reason)


def strip(row: dict, exclusions: set[str]) -> dict:
    return {key: value for key, value in row.items() if key not in exclusions}


def archive_argument(argument: str, parent: Path | None = None) -> Path:
    written, pin = argument.rsplit("=", 1)
    key = (Path(written).name, pin)
    require(key in INDEX, "published_copy_missing:" + written)
    path = parent / Path(written).name if parent is not None else INDEX[key]
    read(path, pin)
    return path


def attempt(path: Path) -> tuple[dict, list[dict], dict]:
    record = obj(path)
    stem = path.name.removesuffix(".receipt.json")
    raw_path = path.with_name(stem + ".raw.txt")
    binding = record["streams"][raw_path.name]
    raw = read(raw_path, binding["sha256"])
    require(len(raw) == binding["bytes"], "raw_length")
    rows = []
    for line in raw.decode().splitlines():
        if not line.startswith("{"):
            break
        rows.append(loads(line))
    require(len(rows) >= 2 and rows[0]["type"] == "configuration"
            and rows[-1]["type"] == "terminal", "raw_inventory")
    equal(rows[1:-1], record["orders"], "orders_raw_binding")
    equal(rows[-1], record["terminal"], "terminal_raw_binding")
    require(record["status"] == "completed" and record["error"] is None, "transport")
    require(record["exit_code"] == rows[-1]["exit_code"], "exit_binding")
    equal(obj(path.with_name(stem + ".sources_before.json")),
          obj(path.with_name(stem + ".sources_after.json")), "source_drift")
    return rows[0], rows[1:-1], rows[-1]


def compare_rows(old: list[dict], new: list[dict], count: int) -> int:
    require(count > 0 and len(old) >= count and len(new) >= count, "nonvacuum")
    saving = 0
    for k, (left, right) in enumerate(zip(old[:count], new[:count]), 1):
        require(left["k"] == right["k"] == k, "order_sequence")
        require(left["outcome"] == right["outcome"] == "complete_relative", "successful_only")
        require("successor_accounting" not in left and right["successor_accounting"] == CALENDAR,
                "explicit_calendar")
        delta = 2 * left["normalized_anchors"]
        require(type(left["normalized_anchors"]) is int and delta >= 0, "integer_anchors")
        require(right["successor_steps"] == left["successor_steps"] - delta, "work_identity")
        equal(strip(left, ORDER_MEASURES | VERSIONS | {"successor_steps"}),
              strip(right, ORDER_MEASURES | VERSIONS | {"successor_steps"}), "all_other_order_fields")
        saving += delta
    return saving


def main() -> dict:
    manifests = {}
    for packet in PACKETS:
        manifest = obj(packet / "manifest.json")
        manifests[packet.name] = sha(read(packet / "manifest.json"))
        for name, meta in manifest.items():
            INDEX[(Path(name).name, meta["sha256"])] = packet / name
    manifest = obj(PACKET / "manifest.json")
    actual = {str(p.relative_to(PACKET)) for p in PACKET.rglob("*") if p.is_file()}
    require(actual == set(manifest) | {"manifest.json", "SHA256SUMS"}, "closed_file_inventory")
    for name, meta in manifest.items():
        require(len(read(PACKET / name, meta["sha256"])) == meta["bytes"], "manifest_bytes")
    for line in read(PACKET / "SHA256SUMS").decode().splitlines():
        pin, name = line.split("  ", 1)
        read(PACKET / name, pin)
    require(len(actual) == 1162 and len(manifest) == 1160, "published_nonvacuum")

    comparison = obj(PACKET / "comparisons/receipt.json")
    require(comparison["comparisons_valid"] == 29 and comparison["all_planned_comparisons_valid"],
            "comparison_plan")
    cases = []
    refused_old = None
    for command_path in sorted((PACKET / "comparisons").glob("[0-9][0-9]_*_normal.command.json")):
        command = obj(command_path)
        require(command["exit_code"] == 0 and command["status"] == "completed", "comparison_capture")
        argv = command["argv"]
        def option(key: str) -> str:
            return argv[argv.index(key) + 1]
        old_path = archive_argument(option("--old"))
        new_path = archive_argument(option("--new"))
        oc, old, ot = attempt(old_path)
        nc, new, nt = attempt(new_path)
        equal(strip(oc, VERSIONS), strip(nc, VERSIONS), "configuration_caps_unchanged")
        require(ot["input_digest"] == nt["input_digest"], "labelled_input")
        for arm, path in (("old", old_path), ("new", new_path)):
            protocol_path = archive_argument(option("--" + arm + "-protocol"), path.parent)
            source_path = archive_argument(option("--" + arm + "-sources"), path.parent)
            require(protocol_path.parent == source_path.parent == path.parent, "published_arm_scope")
            source, protocol = obj(source_path), obj(protocol_path)
            require(source["files"]["morsehgp3D_v7/src/forest/full_gabriel.hpp"] == protocol["producer_sha256"],
                    "producer_protocol")
            require(source["binary_sha256"] == protocol["binary_sha256"], "binary_protocol")
            if arm == "new":
                require(protocol["producer_sha256"] == HEADER and protocol["binary_sha256"] == BINARY,
                        "current_producer_binary")
        complete = option("--scope") == "complete"
        count = nc["kmax_effective"] if complete else int(option("--prefix-orders"))
        saving = compare_rows(old, new, count)
        if complete:
            require(ot["exit_code"] == nt["exit_code"] == 0 and len(old) == len(new) == count,
                    "whole_horizontal_completion")
            left, right = strip(ot, TERMINAL_MEASURES | VERSIONS), strip(nt, TERMINAL_MEASURES | VERSIONS)
            left["last_order_work"] = dict(left["last_order_work"])
            left["last_order_work"]["successor_steps"] -= 2 * left["last_order_work"]["normalized_anchors"]
            equal(left, right, "terminal_success_fields")
        else:
            require(nc["n"] == 32000 and count == 8 and ot["exit_code"] == nt["exit_code"] == 2,
                    "bounded_refusal_prefix")
            require(ot["certificate_digest"] == nt["certificate_digest"] == ""
                    and not ot["complete_requested_horizontal_orders"]
                    and not nt["complete_requested_horizontal_orders"], "refusal_not_promoted")
            refused_old = ot
        normal = command_path.with_name(command_path.name.replace(".command.json", ".stdout"))
        optimized = normal.with_name(normal.name.replace("_normal.stdout", "_optimized.stdout"))
        equal(loads(read(normal)), loads(read(optimized)), "constructor_normal_optimized")
        verdict = obj(normal)
        require(verdict["orders_compared"] == count and verdict["scope"] == option("--scope")
                and verdict["reported_work_fields_compared_except_successor_steps"] == 24,
                "reported_scope")
        cases.append({"id": command["id"].removesuffix("_normal"), "orders": count,
                      "saved_accesses": saving, "scope": option("--scope")})
    require(len(cases) == 29 and sum(c["orders"] for c in cases) == 204, "204_order_floor")

    heavy = []
    for path in sorted((PACKET / "heavy").glob("n*.receipt.json")):
        config, orders, terminal = attempt(path)
        record = obj(path)
        require(record["argv"][:8] == ["timeout", "--signal=TERM", "--kill-after=10s", "600s",
                "taskset", "-c", "6", "/usr/bin/time"], "bounded_cpu6_run")
        require(record["process_vm_max_bytes"] == 26 * (1 << 30), "VM_guard")
        require(config["max_successor_steps_per_order"] == 128000000
                and config["max_meb_calls_per_order"] == 4000000, "unchanged_caps")
        raw = read(path.with_name(path.name.replace(".receipt.json", ".raw.txt"))).decode()
        rss = re.findall(r"Maximum resident set size \(kbytes\): (\d+)", raw)
        require(len(rss) == 1, "GNU_time_RSS")
        heavy.append({"n": config["n"], "s": config["s"], "exit_code": record["exit_code"],
            "orders_completed": terminal["completed_orders_diagnostic"], "reason": terminal["reason"],
            "elapsed_ms": terminal["elapsed_before_terminal_ms"], "full_ms": terminal["stage_ms"]["full"],
            "rss_kib": int(rss[0]), "started": record["started"], "ended": record["ended"]})
    require(len(heavy) == 5 and sorted(h["exit_code"] for h in heavy) == [0, 0, 0, 0, 2], "four_success_one_refusal")
    boundary = obj(PACKET / "heavy/n32000_s8_k10_lazy_c1000000.receipt.json")["terminal"]
    work = boundary["last_order_work"]
    require(boundary["last_order"] == 9 and boundary["completed_orders_diagnostic"] == 8
            and boundary["reason"] == "full_gabriel_meb_call_budget", "K9_boundary")
    require((work["meb_calls"], work["meb_supports"], work["successor_steps"])
            == (4000000, 553128490, 125373952), "K9_work")
    require(refused_old is not None and refused_old["reason"] == "full_gabriel_successor_budget", "old_other_stop")

    original = PACKET / "heavy/n8000_s8_k10_lazy_c1000000.receipt.json"
    replay = PACKET / "replay/n8000_s8_k10_lazy_c1000000.receipt.json"
    oc, oo, ot = attempt(original)
    rc, ro, rt = attempt(replay)
    equal(oc, rc, "v3_v3_configuration")
    require(len(oo) == len(ro) == 10 and ot["exit_code"] == rt["exit_code"] == 0, "replay_complete")
    for left, right in zip(oo, ro):
        equal(strip(left, ORDER_MEASURES), strip(right, ORDER_MEASURES), "v3_v3_all_order_nonmeasure_fields")
    equal(strip(ot, TERMINAL_MEASURES), strip(rt, TERMINAL_MEASURES), "v3_v3_terminal_nonmeasure_fields")
    publication = obj(PACKET / "publication.json")
    incident = publication["excluded_original_timing"]
    require(incident["timing_eligible"] is False, "original_timing_excluded")
    orec, rrec = obj(original), obj(replay)
    ts = datetime.fromisoformat
    require(ts(orec["started"]) <= ts(incident["incident_interval_utc"][0])
            < ts(incident["incident_interval_utc"][1]) <= ts(orec["ended"]), "overlap_inside_original")
    require(all(ts(h["ended"]) < ts(rrec["started"]) for h in heavy), "replay_after_heavy_closure")
    require(publication["single_replay"]["quiet_window_independently_certified"] is False,
            "no_unearned_isolation")
    # Re-read every scientific input before emitting the result.
    for name, pin in list(PINS.items()):
        read(ROOT / name, pin)
    return {"status": "passed", "scope": "independent_raw_field_and_receipt_reread",
        "script_sha256": sha(Path(__file__).read_bytes()), "packet_files_verified": len(actual),
        "packet_manifests": manifests, "historical_cases": cases, "historical_orders_compared": 204,
        "other_serialized_work_counters_equal": 24, "unserialized_geometry_counters_not_tested": 8,
        "heavy": heavy, "K9_refusal_work": work, "old_K9_reason": refused_old["reason"],
        "K9_identity_not_applied_to_refusals": True,
        "v3_v3_replay": {"orders": 10, "all_nonmeasure_fields_equal": True,
            "successor_counter_not_transformed": True, "original_timing_excluded": True,
            "replay_after_five_captures": True, "hardware_isolation_certified": False,
            "elapsed_ms": rt["elapsed_before_terminal_ms"], "full_ms": rt["stage_ms"]["full"]},
        "inputs": PINS, "engine_invoked": False, "comparator_imported": False,
        "performance_gain_claimed": False, "geometry_completeness_claimed": False,
        "public_status": "not_claimed", "gcp_used": False}


if __name__ == "__main__":
    print(json.dumps(main(), indent=2, sort_keys=True))
