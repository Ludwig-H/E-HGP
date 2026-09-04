#!/usr/bin/env python3
"""Explicit, terminal-only, allowlisted public projection of one private session.

No GCP, SSH, subprocess, key access, private log traversal, or recursive copy.
The reviewed controller remains the authority for raw/semantic verification.
"""
from __future__ import annotations

import argparse
import ctypes
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import stat
import tempfile

HERE = Path(__file__).resolve().parent
CONTROLLER_SHA = "a07d271456705c5277c1bd898e93d3aa79faddd81f45d3ba7310807175bbc4a8"
SCHEMA = "ehgp.v7.g4.snapshot.v1"
CPU_STATUSES = {"engine_completed", "engine_refused", "censored", "failed", "invalid", "not_attempted"}
PACKAGES = {"build-essential", "cmake", "libboost-dev", "time"}
RUN_NAMES = ["cpu_bootstrap", "cpu_cmake_version", "cpu_compiler_version", "cpu_make_version", "cpu_time_version",
             "cpu_package_versions", "cpu_cpp20_compile", "cpu_cpp20_probe", "cpu_build", "nvcc_version",
             "gpu_hardware", "candidate_uniform", "candidate_wide", "gpu_cmake_install", "gpu_inventory",
             "gpu_build", "gpu_primitives"] + [f"cpu_v{v}_{f}{suffix}" for v in (6, 7)
             for f in ("uniform", "terrain") for suffix in ("", "_k5")]


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def encode(value):
    return (json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + "\n").encode()


def hash_value(value):
    require(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value), "invalid SHA-256")
    return value


def number(value, *, integer=False, minimum=0, maximum=10**18):
    require(type(value) is int if integer else type(value) in (int, float), "invalid numeric type")
    require(math.isfinite(value) and minimum <= value <= maximum, "numeric value outside bounds")
    return value


def choice(value, options):
    require(value in options, "unrecognized public enumeration")
    return value


def timestamp(value):
    require(isinstance(value, str) and re.fullmatch(r"[0-9T:.+Z-]{20,40}", value), "invalid timestamp")
    require(datetime.fromisoformat(value.replace("Z", "+00:00")).tzinfo is not None, "timestamp lacks timezone")
    return value


class SelectedReader:
    def __init__(self, session):
        require(not session.is_symlink() and session.is_dir(), "session must be a real directory")
        self.session = session.resolve(strict=True)
        self.inputs = {}

    def raw(self, relative, expected=None):
        parts = PurePosixPath(relative).parts
        require(parts and not relative.startswith("/") and all(p not in (".", "..") for p in parts), "unsafe selected path")
        path = self.session
        for part in parts:
            path = path / part
            require(not path.is_symlink(), "selected input is a symlink")
        info = path.stat()
        require(stat.S_ISREG(info.st_mode) and 0 < info.st_size <= 16 * 1024 * 1024, "selected JSON size/type")
        raw = path.read_bytes()
        pin = digest(raw)
        if expected is not None:
            require(pin == expected, "selected JSON does not match controller-certified manifest")
        self.inputs[relative] = {"sha256": pin, "bytes": len(raw)}
        return raw

    def read(self, relative, expected=None):
        def unique(pairs):
            result = {}
            for key, value in pairs:
                require(key not in result, "duplicate JSON key")
                result[key] = value
            return result
        return json.loads(self.raw(relative, expected), object_pairs_hook=unique,
                          parse_constant=lambda _: (_ for _ in ()).throw(ValueError("nonfinite JSON constant")))

    def stable(self):
        before = dict(self.inputs)
        for relative, item in before.items():
            self.raw(relative, item["sha256"])
        require(before == self.inputs, "selected inputs changed during export")


def source_projection(manifest):
    require(manifest.get("schema") == SCHEMA and manifest.get("public_status") == "not_claimed", "source schema/status")
    require(re.fullmatch(r"[0-9a-f]{40}", manifest.get("head", "")), "source HEAD")
    records, seen = [], set()
    for item in manifest.get("files", []):
        path = item.get("path", "")
        require(re.fullmatch(r"[A-Za-z0-9_.+/-]+", path) and not path.startswith("/") and
                all(part not in (".", "..") for part in PurePosixPath(path).parts) and path not in seen,
                "source path outside public repository grammar")
        allowed = re.fullmatch(r"morsehgp3D_v[67]/(CMakeLists\.txt|(?:src|cli|oracle|tests|cmake|bench)/.+|receipts/conformite_v5/.+|docs/V6_SOURCE_SNAPSHOT\.json)", path)
        allowed = allowed or path in {"gcp-migration/" + name for name in
            ("start_and_verify.sh", "stop_and_verify.sh", "session_campagne_v7_g4.sh", "v7_g4_session.py", "private_cmake_v7.py")}
        require(allowed, "source path outside reviewed snapshot scope")
        seen.add(path)
        records.append({"path": path, "sha256": hash_value(item["sha256"]), "bytes": number(item["size"], integer=True),
                        "mode": choice(item["mode"], {0o444, 0o555})})
    controller = next((item for item in records if item["path"] == "gcp-migration/v7_g4_session.py"), None)
    require(controller is not None and controller["sha256"] == CONTROLLER_SHA, "unreviewed source controller")
    return {"head": manifest["head"], "files": records, "worktree_status": "omitted_private_path_metadata",
            "source_binding": "actual_hashed_worktree_not_clean_commit_attestation"}


def observation_projection(item, semantic_validation):
    status = choice(item.get("status"), CPU_STATUSES)
    public = {"status": status, "version": choice(item["version"], {"v6", "v7"}),
              "family": choice(item["family"], {"uniform", "terrain"}), "n": choice(item["n"], {50000}),
              "seed": choice(item["seed"], {3}), "kmax": choice(item["kmax"], {5, 10}),
              "s": choice(item["s"], {8}), "threads": choice(item["threads"], {48}),
              "process_wall_seconds": number(item["process_wall_seconds"], maximum=4320),
              "timing_scope": "process_including_generation_and_digest", "semantic_validation": semantic_validation,
              "exit_code": None if item.get("exit_code") is None else number(item["exit_code"], integer=True, minimum=-128, maximum=255)}
    if "reason" in item:
        require(isinstance(item["reason"], str), "reason type")
        public["reason_sha256"] = digest(item["reason"].encode())
    if "refusal_status" in item:
        public["refusal_status"] = choice(item["refusal_status"], {"unsupported_degeneracy", "resource_exhausted"})
    if status != "engine_completed":
        require("digests" not in item and "cardinalities" not in item, "non-success CPU observation carries objects")
        return public
    if semantic_validation != "controller_revalidated":
        public["object_export"] = "withheld_without_controller_semantic_completion"
        return public
    public.update(object_projection(item, public["kmax"]))
    public["pipeline_ms"] = number(item["pipeline_ms"], maximum=4_320_000)
    public["max_rss_kb"] = number(item["max_rss_kb"], integer=True)
    public["counts"] = counter_projection(item["counts"])
    return public


def object_projection(item, kmax):
    public = {}
    digests = item["digests"]
    require(set(digests) == {"digest_all"} | {f"digest_forest_K{k}" for k in range(1, kmax + 1)}, "digest order coverage")
    public["digests"] = {key: hash_value(value) for key, value in digests.items()}
    expected = digest(("mhgp4-digest-v1:all" + "".join(digests[f"digest_forest_K{k}"] for k in range(1, kmax + 1))).encode())
    require(expected == digests["digest_all"], "digest chain mismatch")
    require(set(item["cardinalities"]) == {str(k) for k in range(1, kmax + 1)}, "cardinality order coverage")
    fields = {"evenements", "facettes", "deltas", "attachements", "fusions", "noeuds"}
    public["cardinalities"] = {}
    for k, counts in item["cardinalities"].items():
        require(set(counts) == fields, "cardinality fields")
        public["cardinalities"][k] = {key: number(value, integer=True) for key, value in counts.items()}
    return public


def counter_projection(counts):
    fields = {"n", "coord", "s", "smax", "seed", "threads", "emis", "boules_uniques", "mortes_profondeur",
              "survivantes", "census_int", "census_shell", "evenements", "facettes", "fusions", "deltas", "noeuds"}
    require(set(counts) == fields, "CPU counter fields")
    return {key: number(value, integer=True) for key, value in counts.items()}


def stage_projection(text):
    """Only numeric matches of fixed scientific lines; never publish raw text."""
    decimal = r"([0-9]+\.[0-9]+)"
    integer = r"([0-9]+)"
    specs = {
        "stage_times_ms": ("temps_ms ", r"temps_ms index=" + decimal + " gen=" + decimal + r" \(wspd " + decimal +
            " rects " + decimal + r"\) rle=" + decimal + " prefiltre=" + decimal + " census=" + decimal +
            " comptage=" + decimal + " expansion=" + decimal + " fold=" + decimal + r" \(tri " + decimal +
            " intern " + decimal + " fusion " + decimal + " reduce " + decimal + r"\) digest=" + decimal,
            ("index", "generation", "wspd", "rectangles", "rle", "prefilter", "census", "count", "expansion",
             "fold_cumulative", "fold_sort", "fold_intern", "fold_merge", "fold_reduce", "digest_cumulative")),
        "fold_wall": ("temps_fold_mur_ms=", "temps_fold_mur_ms=" + decimal + r" \(etages A et B, fold_inflight=" +
                      integer + ", fold_join=" + integer + ", pic_mesure_en_vol=" + integer + r"\)",
                      ("milliseconds", "inflight", "join", "measured_inflight_peak")),
        "workers": ("ouvriers ", r"ouvriers wspd=" + integer + " rects=" + integer + " rle=" + integer + " prefiltre=" +
                    integer + " census=" + integer + " expansion=" + integer + " fold=" + integer,
                    ("wspd", "rectangles", "rle", "prefilter", "census", "expansion", "fold")),
        "census_counters": ("vcensus ", r"vcensus prefiltre_nœuds=" + integer + " prefiltre_feuilles=" + integer +
                            " range_add=" + integer + " census_nœuds=" + integer + " census_feuilles=" + integer,
                            ("prefilter_nodes", "prefilter_leaf_tests", "range_add", "census_nodes", "census_leaf_tests")),
    }
    stages = ("after_generation", "after_rle", "after_prefilter", "after_census", "max_fold", "end")
    for prefix, key in (("rss_mb ", "rss_mb"), ("residence_hwm_mb ", "historical_hwm_mb")):
        specs[key] = (prefix, re.escape(prefix) + "apres_generation=" + integer + " apres_rle=" + integer +
                      " apres_prefiltre=" + integer + " apres_census=" + integer + " max_fold=" + integer +
                      " fin=" + integer + r" \([^\r\n]*\)", stages)
    result = {"timing_caveat": "fold_and_digest_are_cumulative_stage_timers_not_additive_pipeline_wall",
              "memory_caveat": "RSS_is_instantaneous_HWM_is_process_historical_not_stage_peak"}
    for key, (prefix, pattern, fields) in specs.items():
        lines = [line for line in text.splitlines() if line.startswith(prefix)]
        require(len(lines) == 1, "scientific stage line absent or duplicated")
        match = re.fullmatch(pattern, lines[0])
        require(match is not None and len(match.groups()) == len(fields), "scientific stage grammar mismatch")
        result[key] = {name: number(float(value) if "." in value else int(value))
                       for name, value in zip(fields, match.groups())}
    return result


def outcome_projection(item, *, gpu=False):
    statuses = {"completed", "unavailable", "not_requested"} if gpu else CPU_STATUSES
    result = {"status": choice(item.get("status"), statuses)}
    if "scope" in item:
        result["scope"] = choice(item["scope"], {"device_primitives_only"})
    if "exit_code" in item:
        result["exit_code"] = number(item["exit_code"], integer=True, minimum=-128, maximum=255)
    if "refusal_status" in item:
        result["refusal_status"] = choice(item["refusal_status"], {"unsupported_degeneracy", "resource_exhausted"})
    if "mathematical_refusal" in item:
        require(type(item["mathematical_refusal"]) is bool, "mathematical refusal type")
        result["mathematical_refusal"] = item["mathematical_refusal"]
    if "reason" in item:
        require(isinstance(item["reason"], str), "diagnostic reason type")
        result["reason_sha256"] = digest(item["reason"].encode())
        if item["reason"] in {"budget_insufficient", "disabled", "tools_or_780_second_budget_unavailable",
                              "private_cmake_install_failed_or_censored", "private_cmake_120s_plus_gpu780s_and_drain20s_budget_unavailable"}:
            result["reason_category"] = item["reason"]
    return result


def candidate_projection(item, semantic):
    result = outcome_projection(item)
    result["semantics"] = "normalized_horizontal_h0_candidate"
    if result["status"] != "engine_completed":
        require("digests" not in item and "cardinalities" not in item, "non-completed candidate carries objects")
        return result
    result["pipeline_ms"] = number(item["pipeline_ms"], maximum=4_320_000)
    result["max_rss_kb"] = number(item["max_rss_kb"], integer=True)
    result["counts"] = counter_projection(item["counts"])
    require(result["counts"]["n"] in (8000, 50000), "candidate input cardinality")
    result["silent"] = {}
    fields = {"core", "with_two_intruders", "steps", "added", "max_chain", "query_nodes", "meb_supports"}
    require(set(item["silent"]) == {str(k) for k in range(2, 11)}, "silent order coverage")
    for k, counts in item["silent"].items():
        require(set(counts) == fields, "silent counter fields")
        result["silent"][k] = {key: number(value, integer=True) for key, value in counts.items()}
    if semantic == "controller_revalidated":
        result.update(object_projection(item, 10))
    else:
        result["object_export"] = "withheld_without_controller_semantic_completion"
    return result


def gpu_test_projection(text):
    summary = re.findall(r"^100% tests passed, 0 tests failed out of ([0-9]+)$", text, re.M)
    require(len(summary) == 1 and int(summary[0]) > 0, "GPU completed CTest summary missing")
    rows = re.findall(r"^\s*([0-9]+)/([0-9]+) Test\s+#[0-9]+: (mhgp7_(?:device_witness|census_device)[A-Za-z0-9_-]*)\s+\.+\s+Passed\s+([0-9]+\.[0-9]+) sec$", text, re.M)
    n = int(summary[0])
    require(len(rows) == n and {int(row[0]) for row in rows} == set(range(1, n + 1)) and
            all(int(row[1]) == n for row in rows) and len({row[2] for row in rows}) == n,
            "GPU passed test inventory mismatch")
    return {"tests": n, "passed": n, "failed": 0,
            "cases": [{"name": row[2], "seconds": number(float(row[3]), maximum=300)} for row in rows],
            "scope": "device_primitives_only_not_full_gpu_pipeline"}


def build_public(session):
    reader = SelectedReader(session)
    state, prepared = reader.read("session.json"), reader.read("prepared.json")
    require(state.get("schema") == SCHEMA and prepared.get("schema") == SCHEMA, "session/prepared schema")
    require(state.get("status") in {"completed", "failed"} and state.get("stopped_verified") is True and
            "ended" in state and not state.get("shutdown_failure"), "session not terminal with certified exact stop")
    require((state["status"] == "completed") == (state.get("exit_code") == 0), "controller status/exit contradiction")
    require(state.get("source_manifest_sha256") == prepared.get("manifest_sha256"), "session/source binding")
    target = state.get("target", {})
    require(set(target) == {"project", "zone", "instance"} and all(isinstance(v, str) and re.fullmatch(r"[a-z0-9-]+", v)
                                                                  for v in target.values()), "target identity grammar")
    require(not state.get("no_session_created"), "this export requires a real started generation")
    generation = timestamp(state.get("generation"))
    started, ended = timestamp(state.get("started")), timestamp(state.get("ended"))
    require(datetime.fromisoformat(ended) >= datetime.fromisoformat(started), "negative session duration")
    source = reader.read("source/source_manifest.json", hash_value(prepared["manifest_sha256"]))
    source_pins = source_projection(source)
    require(prepared.get("head") == source_pins["head"], "prepared/source HEAD mismatch")
    # These three exact structured stop files and the selected guard marker
    # are read for identity/stop proof only. No raw GCE/network/account fields
    # or private command argv are copied to the public projection.
    stopped = reader.read("control/post_stop.stdout")
    stop_run, stop_check = reader.read("control/stop.json"), reader.read("control/post_stop.json")
    require(stop_run.get("exit_code") == 0 and stop_check.get("exit_code") == 0, "targeted stop/check failed")
    require(stopped.get("status") == "TERMINATED" and stopped.get("lastStartTimestamp") == generation and
            stopped.get("name") == target["instance"] and stopped.get("zone", "").rsplit("/", 1)[-1] == target["zone"] and
            stopped.get("selfLink", "").endswith(f"/projects/{target['project']}/zones/{target['zone']}/instances/{target['instance']}"),
            "stopped exact target/generation mismatch")
    instance_id = str(stopped.get("id", ""))
    require(re.fullmatch(r"[0-9]{1,30}", instance_id), "missing exact instance id")
    schedule = stopped.get("scheduling", {})
    require(stopped.get("machineType", "").rsplit("/", 1)[-1] == "g4-standard-48" and
            stopped.get("labels", {}).get("project") == "e-hgp" and schedule.get("provisioningModel") == "SPOT" and
            schedule.get("instanceTerminationAction") == "STOP" and
            str(schedule.get("maxRunDuration", {}).get("seconds")) == "3600", "stopped target safety configuration")
    marker = {}
    for line in reader.raw("guard-marks/double_guard_verified").decode().splitlines():
        key, sep, value = line.partition("=")
        require(sep and key not in marker, "invalid double-guard marker")
        marker[key] = value
    expected_marker = dict(target, schema="e-hgp.guard-mark.v1", mark="double_guard_verified", generation=generation,
                           max_run_seconds="3600", guest_shutdown_minutes="45")
    require(all(marker.get(k) == v for k, v in expected_marker.items()), "double-guard marker identity mismatch")
    public = {"schema": "ehgp.v7.gcp-public-receipt.v1", "public_status": "not_claimed", "performance_claim": False,
              "controller_status": state["status"], "controller_exit_code": number(state["exit_code"], integer=True, maximum=255),
              "started": started, "ended": ended,
              "session_elapsed_seconds": (datetime.fromisoformat(ended) - datetime.fromisoformat(started)).total_seconds(),
              "source_manifest_sha256": hash_value(prepared["manifest_sha256"]),
              "source_archive_sha256": hash_value(prepared["archive_sha256"]), "controller_sha256": CONTROLLER_SHA,
              "stop": {"verified": True, "status": "TERMINATED", "generation": generation,
                       "target": dict(target, id=instance_id),
                       "target_sha256": digest(encode(target)), "target_generation_sha256": digest(encode(dict(target, generation=generation))),
                       "zone": target["zone"], "machine_type": "g4-standard-48", "provisioning": "SPOT", "gce_seconds": 3600,
                       "guest_minutes": 45, "termination_action": "STOP", "double_guard_marker": expected_marker,
                       "stop_and_check_seconds": number(stop_run["stop_and_check_seconds"], maximum=480),
                       "stop_ended": timestamp(stop_run["ended"]), "check_ended": timestamp(stop_check["ended"]),
                       "authority": "reviewed_guarded_controller_terminal_state_and_selected_stop_fields"},
              "redactions": ["private_paths", "accounts", "emails", "network_addresses",
                             "raw_commands_and_logs", "free_diagnostic_messages", "worktree_status"],
              "authority": "sanitized_projection_not_a_replacement_for_private_controller_receipt"}
    if "failure" in state:
        require(isinstance(state["failure"], str), "failure type")
        public["controller_failure_sha256"] = digest(state["failure"].encode())
    if "result_manifest_sha256" not in state:
        public["results"] = {"status": "not_controller_certified_or_not_received"}
    else:
        manifest_sha = hash_value(state["result_manifest_sha256"])
        result_manifest = reader.read("received/out/result_manifest.json", manifest_sha)
        require(result_manifest.get("schema") == "ehgp.v7.g4.results.v1", "result manifest schema")
        entries = {}
        for item in result_manifest.get("files", []):
            path = item.get("path")
            require(isinstance(path, str) and not path.startswith("/") and ".." not in PurePosixPath(path).parts and path not in entries,
                    "result manifest path collision/traversal")
            entries[path] = {"sha256": hash_value(item["sha256"]), "bytes": number(item["size"], integer=True)}

        def selected(name):
            require(name in entries, "required selected result absent")
            value = reader.read("received/out/" + name, entries[name]["sha256"])
            require(reader.inputs["received/out/" + name]["bytes"] == entries[name]["bytes"], "selected result size")
            return value

        terminal = selected("worker_terminal.json")
        require(terminal.get("exit_code") == state.get("remote_exit_code"), "worker/controller remote outcome mismatch")
        require(terminal.get("exit_code") != 0 or terminal.get("diagnostics_completed") is True, "successful worker lacks completed validation")
        semantic = "controller_revalidated" if terminal.get("diagnostics_completed") is True else "raw_manifest_and_terminal_only"
        public["results"] = {"manifest_sha256": manifest_sha, "worker_exit_code": number(terminal["exit_code"], integer=True, maximum=255),
                             "semantic_validation": semantic, "worker_ended": timestamp(terminal["ended"])}
        result = public["results"]
        if "cpu_campaign.json" in entries:
            cpu = selected("cpu_campaign.json")
            result["cpu"] = {"status": choice(cpu["status"], {"completed", "failed_or_incomplete"}),
                "semantics": choice(cpu["semantics"], {"verified_events_only"}), "public_status": "not_claimed",
                "observations": [observation_projection(item, semantic) for item in cpu["observations"]], "pairs": {}}
            for original, projected in zip(cpu["observations"], result["cpu"]["observations"]):
                if original["status"] != "engine_completed":
                    continue
                name = f"cpu_{projected['version']}_{projected['family']}" + ("_k5" if projected["kmax"] == 5 else "") + ".stdout"
                require(name in entries, "scientific stdout not in certified manifest")
                raw = reader.raw("received/out/" + name, entries[name]["sha256"])
                require(len(raw) == entries[name]["bytes"], "scientific stdout size")
                projected["stages"] = stage_projection(raw.decode())
            for key, pair in cpu["pairs"].items():
                require(key in {f"{f}_k{k}" for f in ("uniform", "terrain") for k in (5, 10)}, "CPU pair key")
                status = choice(pair["status"], {"completed", "diverged", "not_comparable", "not_attempted"})
                row = {"status": status}
                if "equal" in pair:
                    require(status in {"completed", "diverged"} and type(pair["equal"]) is bool, "false comparable pair")
                    if semantic == "controller_revalidated":
                        row["equal"] = pair["equal"]
                result["cpu"]["pairs"][key] = row
        for name, key, gpu in (("candidate_status.json", "candidate_default", False),
                               ("candidate_wide_status.json", "candidate_wide", False), ("gpu_status.json", "gpu", True)):
            if name in entries:
                item = selected(name)
                result[key] = outcome_projection(item, gpu=True) if gpu else candidate_projection(item, semantic)
                result[key]["semantic_validation"] = semantic
                if not gpu and item["status"] == "engine_completed":
                    raw_name = "candidate_wide.stdout" if key == "candidate_wide" else "candidate_uniform.stdout"
                    require(raw_name in entries, "completed candidate stdout missing")
                    raw = reader.raw("received/out/" + raw_name, entries[raw_name]["sha256"])
                    require(len(raw) == entries[raw_name]["bytes"], "candidate stdout size")
                    result[key]["stages"] = stage_projection(raw.decode())
                if gpu and item["status"] == "completed":
                    require("gpu_primitives.stdout" in entries, "GPU CTest stdout missing")
                    raw = reader.raw("received/out/gpu_primitives.stdout", entries["gpu_primitives.stdout"]["sha256"])
                    require(len(raw) == entries["gpu_primitives.stdout"]["bytes"], "GPU CTest stdout size")
                    result[key]["ctest"] = gpu_test_projection(raw.decode())
        for name, key, expected in (("cpu_binaries.json", "cpu_binary_sha256", {"v6", "v7"}),
                                    ("gpu_binaries.json", "gpu_binary_sha256", {"mhgp7_device_witness", "mhgp7_census_device_gate"})):
            if name in entries:
                values = selected(name)
                require(set(values) == expected, "binary identity set")
                result[key] = {label: hash_value(item["sha256"]) for label, item in values.items()}
        result["timelines"] = []
        for name in RUN_NAMES:
            if name + ".json" not in entries:
                continue
            record = selected(name + ".json")
            row = {"name": name, "status": choice(record["status"], {"completed", "failed", "censored", "interrupted", "not_attempted"})}
            for field in ("elapsed_seconds", "timeout_seconds"):
                if field in record:
                    row[field] = number(record[field], maximum=4320)
            if record.get("exit_code") is not None:
                row["exit_code"] = number(record["exit_code"], integer=True, minimum=-128, maximum=255)
            for suffix in ("stdout", "stderr"):
                if name + "." + suffix in entries:
                    row[suffix + "_sha256"] = entries[name + "." + suffix]["sha256"]
            result["timelines"].append(row)
        if "cpu_toolchain.json" in entries:
            tools = selected("cpu_toolchain.json")
            require(set(tools["packages"]) == PACKAGES, "CPU package set")
            versions = {}
            for name, value in tools["packages"].items():
                require(isinstance(value, str) and re.fullmatch(r"[0-9][A-Za-z0-9.+:~_-]{0,100}", value), "package version grammar")
                versions[name] = value
            result["cpu_package_versions"] = versions
            probe = re.fullmatch(r"cplusplus=([0-9]+) boost=([0-9]+)\n", tools["probe"])
            require(probe is not None, "CPU probe grammar")
            result["cpu_language_probe"] = {"cplusplus": int(probe[1]), "boost": int(probe[2])}
        if "gpu_tools.json" in entries:
            tools = selected("gpu_tools.json")
            require(type(tools.get("requested")) is bool, "GPU request type")
            result["gpu_requested"] = tools["requested"]
            version = re.search(r"release ([0-9]+\.[0-9]+), V([0-9]+\.[0-9]+\.[0-9]+)", tools.get("version", ""))
            if version:
                result["cuda_version"] = {"release": version[1], "build": version[2]}
        if "gpu_cmake_toolchain.json" in entries:
            tools = selected("gpu_cmake_toolchain.json")
            result["gpu_cmake"] = {"source": choice(tools["source"], {"system", "private"})}
            if tools["source"] == "private":
                install = tools["installation"]
                require(install["version"] == "3.31.6", "private CMake version")
                result["gpu_cmake"].update(version="3.31.6", helper_sha256=hash_value(install["helper_sha256"]),
                    wheel_sha256=hash_value(install["wheel_sha256"]), wheel_bytes=number(install["wheel_bytes"], integer=True),
                    executable_sha256={name: hash_value(install["probes"][name]["binary_sha256"]) for name in ("cmake", "ctest")})
    reader.stable()
    public["selected_private_inputs"] = reader.inputs
    return {"receipt.json": public, "source_pins.json": source_pins}


def publish(values, destination):
    require(not destination.exists() and not destination.is_symlink(), "public destination already exists")
    require(destination.parent.is_dir() and not destination.parent.is_symlink(), "public parent unavailable")
    encoded = {name: encode(value) for name, value in values.items()}
    encoded["manifest.json"] = encode({"schema": "ehgp.v7.public-export.v1", "exporter_sha256": digest(Path(__file__).read_bytes()),
        "files": [{"path": name, "bytes": len(raw), "sha256": digest(raw)} for name, raw in encoded.items()]})
    staging = Path(tempfile.mkdtemp(prefix=".public-export-", dir=destination.parent))
    staging.chmod(0o700)
    for name, raw in encoded.items():
        with (staging / name).open("xb") as stream:
            stream.write(raw)
            stream.flush()
            os.fsync(stream.fileno())
        (staging / name).chmod(0o444)
    fd = os.open(staging, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)
    rename = getattr(ctypes.CDLL(None, use_errno=True), "renameat2", None)
    require(rename is not None, "Linux create-only atomic rename unavailable")
    rename.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_uint]
    rename.restype = ctypes.c_int
    if rename(-100, os.fsencode(staging), -100, os.fsencode(destination), 1) != 0:
        raise OSError(ctypes.get_errno(), "public create-only rename failed")
    confirmed = True
    try:
        fd = os.open(destination.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)
    except OSError:
        confirmed = False
    return {"status": "published", "directory_sync_confirmed": confirmed}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", type=Path)
    parser.add_argument("--output", type=Path, default=HERE / "published")
    parser.add_argument("--execute", action="store_true")
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps({"status": "not_executed", "requires": "explicit_execute_and_terminal_certified_stop"}))
        return 0
    require(args.session is not None, "explicit private session required")
    values = build_public(args.session)
    print(json.dumps(publish(values, args.output)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
