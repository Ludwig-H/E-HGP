#!/usr/bin/env python3
"""Paired S3/S4a CPU panel on physical sectors of ground-free 08/000200.

All generated files live beside this script.  Use ``python3 -B`` to avoid
bytecode writes elsewhere.  The binary is pinned by bytes and by immutable
source objects; the live worktree is never rebuilt or changed here.
"""

import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import math
import os
import resource
import signal
import struct
import subprocess
import time
from pathlib import Path


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
GENERATOR = REPO / "morsehgp3D_v9/audits/lidar_ground_hot_quarter_multiseed_20260923/run_and_check.py"
PRIOR = REPO / "morsehgp3D_v9/audits/s4a_ground_hot_quarter_20260923"
PRIOR_SCRIPT = PRIOR / "run_and_check.py"
BINARY = Path(os.environ.get(
    "MHGP9_PANEL_BINARY",
    str(REPO / "build/v9-open-worktree/build/v9-exp/mhgp9_tower_probe")))
WORKTREE = REPO / "build/v9-open-worktree"
F32 = REPO / "morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_02_float32"
GRID = F32.parent / "scene_02_grid"
SEED = 0x7D1C9A5EB3F24680
K = 10
S = 8
WORKERS = 8
SECTORS = ("full", "half_x_neg", "half_x_nonneg", "quarter_x_neg_y_neg",
           "quarter_x_neg_y_nonneg", "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg")
DENSITIES = ("quarter", "half", "full")
ARMS = ("s3", "s4a")
HOT = "quarter_x_nonneg_y_neg"
HOT_NAMES = {"quarter": "s1_quarter", "half": "s1_half", "full": "physical_full"}
GENERATOR_SHA = "139e1f0715810bc8b959eda16705b21e33c4eb599eb2ba474dee112835ab304b"
PRIOR_PINS = {
    "run_and_check.py": "fa538504e7afd57e76bb6942c360611c2c7364e9768e4015a79015f20123600a",
    "PREPARED.json": "75d2417436f9c6573ca3a459dcc8d31c433b212fdcf219873730e054f5609d2e",
    "CASES.jsonl": "ffeb936603b3718e0ba99240afbf12dbd3c20e8c2bf23084acfc246f56022323",
    "SUMMARY.json": "a76aef2084a5c46902de452d17a4ebc75229a59cb96028a128195128db899af6",
}
BINARY_SHA = "eea3040cf4150599ca7ab5c71a4f0e3738f2975483584527f339d3d2600d08e9"
SNAPSHOT_HEAD = "7ceadffad1de860e30325ae357ba3243f269d48b"
SOURCE_OBJECTS = {
    "morsehgp3D_v9/src": "064e30629574e057bf699ba7a2bbe911240a99aa",
    "morsehgp3D_v9/bench/tower_probe.cpp": "2231b1964f434255cd655090bb06f77ec8ebd683",
    "morsehgp3D_v9/CMakeLists.txt": "143b7b0b31bee62f551ee21c999ee46a253e8a51",
}
LEGACY_LEVERS = ("atlas_saturate_deep", "q3_leaf_census", "q34_dead_lanes",
                 "q34_witness_cache", "q34_dead_core", "tower_meb_proposal")
SHARED_LEVERS = ("q34_jobs_by_mass", "q34_fine_jobs", "tower_overlap_static",
                 "q2_jobs_by_mass", "q34_batch_filter", "q34_batch_certificates")
COMMON_LEDGER = ("expanded_pairs", "cover_builds", "cover_sites", "cover_node_visits",
                 "q3_edges", "q4_edges", "both_edges", "core_builds", "core_sites",
                 "dead_core_loads", "dead_core_form_sites", "dead_core_uniform_tests",
                 "dead_core_q3_proved", "dead_core_q3_open", "dead_core_q4_proved",
                 "dead_core_q4_open", "core_cover_node_visits", "core_cover_bound_tests")
METRICS = ("core_sites", "expanded_pairs", "chain_cpu_s", "chain_total_ms",
           "lanes_census_point_tests")


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


class HostBusy(RuntimeError):
    """No attempt was launched because the host failed a timing preflight."""


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    return sha(path.read_bytes())


def canonical(value):
    return (json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n").encode()


def pinned_file(path, expected):
    need(file_sha(path) == expected, f"pinned file changed: {path}")


def put_immutable(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        need(path.is_file() and path.read_bytes() == data, f"existing file differs: {path}")
    else:
        with path.open("xb") as stream:
            stream.write(data)


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    need(spec is not None and spec.loader is not None, f"cannot import {path}")
    item = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(item)
    return item


def words(data):
    need(len(data) % 4 == 0, "unaligned u32 input")
    return [x[0] for x in struct.iter_unpack("<I", data)]


def binary_identity():
    pinned_file(BINARY, BINARY_SHA)
    for path, expected in SOURCE_OBJECTS.items():
        out = subprocess.run(("git", "-C", str(WORKTREE), "rev-parse",
                              f"{SNAPSHOT_HEAD}:{path}"), capture_output=True, text=True)
        need(out.returncode == 0 and out.stdout.strip() == expected,
             f"pinned source object differs: {path}")


def source_inputs():
    pinned_file(GENERATOR, GENERATOR_SHA)
    gen = module(GENERATOR, "mhgp9_pinned_hot_quarter_generator")
    grid_points, grid_ids, grid_raw, hot_raw, hot_points = gen.source()
    need(len(grid_ids) == 45845 and len(grid_points) == 12 * len(grid_ids), "grid full frame")
    fm = json.loads((F32 / "MANIFEST.json").read_text())
    f32_points = gen.dataset(F32, fm, "points")
    f32_ids = words(gen.dataset(F32, fm, "original"))
    raw_to_original = words((F32 / "raw_to_original.u32le").read_bytes())
    raw_by_original = {original: raw for raw, original in enumerate(raw_to_original)}
    need(len(raw_by_original) == len(raw_to_original), "raw/original bijection")
    need(len(f32_ids) == len(grid_ids) and len(f32_points) == 12 * len(f32_ids), "float32 full frame")
    signs = {}
    for i, original in enumerate(f32_ids):
        raw = raw_by_original[original]
        x, y, z = struct.unpack_from("<fff", f32_points, 12 * i)
        need(all(math.isfinite(value) for value in (x, y, z)), "nonfinite float32 coordinate")
        need(raw not in signs, "duplicate retained raw return")
        signs[raw] = (x < 0.0, y < 0.0)
    need(set(signs) == set(grid_raw), "float32/grid retained return identity")
    sector_of = {}
    for raw, (xneg, yneg) in signs.items():
        sector_of[raw] = ("half_x_neg" if xneg else "half_x_nonneg",
                          f"quarter_x_{'neg' if xneg else 'nonneg'}_y_{'neg' if yneg else 'nonneg'}")
    # The float32 preparation supplies an independent physical partition of
    # the same retained returns.  Check every sector before selecting density.
    for sector in SECTORS:
        entry = fm["datasets"][sector]
        ids_data = (F32 / entry["original_site_ids_file"]).read_bytes()
        need(sha(ids_data) == entry["original_site_ids_sha256"], f"float32 IDs: {sector}")
        expected = {raw_by_original[original] for original in words(ids_data)}
        actual = {raw for raw in grid_raw if sector == "full" or sector in sector_of[raw]}
        need(expected == actual, f"physical float32 sector mismatch: {sector}")
    actual_hot = b"".join(grid_points[12 * i:12 * i + 12]
                          for i, raw in enumerate(grid_raw) if HOT in sector_of[raw])
    need(set(raw for raw in grid_raw if HOT in sector_of[raw]) == hot_raw and
         actual_hot == hot_points, "hot-quarter physical source mismatch")
    return gen, grid_points, grid_ids, grid_raw, sector_of


def generated():
    gen, points, original, raw, sector_of = source_inputs()
    n = len(original)
    rank = sorted(range(n), key=lambda i: (gen.mix(original[i] ^ SEED), original[i]))
    selected = {"quarter": set(rank[:n // 4]), "half": set(rank[:n // 2]),
                "full": set(range(n))}
    need(selected["quarter"] < selected["half"] < selected["full"], "global density nesting")
    blobs = {}
    entries = {}
    indices = {}
    for density in DENSITIES:
        for sector in SECTORS:
            case = f"{density}_{sector}"
            take = [i for i in range(n) if i in selected[density] and
                    (sector == "full" or sector in sector_of[raw[i]])]
            indices[case] = set(take)
            data = {
                "points": b"".join(points[12 * i:12 * i + 12] for i in take),
                "original_site_ids": b"".join(struct.pack("<I", original[i]) for i in take),
                "raw_return_ids": b"".join(struct.pack("<I", raw[i]) for i in take),
                "full_site_ranks": b"".join(struct.pack("<I", i) for i in take),
            }
            blobs[case] = data
            entries[case] = {"density": density, "sector": sector, "sites": len(take),
                             "input_file": f"inputs/{case}.u32le",
                             "input_sha256": sha(data["points"]),
                             "input_fnv64": gen.fnv_points(data["points"]),
                             **{kind + "_sha256": sha(data[kind]) for kind in
                                ("original_site_ids", "raw_return_ids", "full_site_ranks")}}
    for density in DENSITIES:
        full = indices[f"{density}_full"]
        halves = [indices[f"{density}_{sector}"] for sector in SECTORS[1:3]]
        quarters = [indices[f"{density}_{sector}"] for sector in SECTORS[3:]]
        need(halves[0].isdisjoint(halves[1]) and halves[0] | halves[1] == full,
             f"half reconstruction: {density}")
        need(all(quarters[i].isdisjoint(quarters[j]) for i in range(4) for j in range(i + 1, 4)) and
             set().union(*quarters) == full, f"quarter reconstruction: {density}")
    for density in DENSITIES:
        case = f"{density}_{HOT}"
        for first, second in (("quarter", "half"), ("half", "full")):
            need(indices[f"{first}_{HOT}"] < indices[f"{second}_{HOT}"], "hot-quarter nesting")
        prior = json.loads((PRIOR / "PREPARED.json").read_text())["cases"][HOT_NAMES[density]]
        need(entries[case]["sites"] == prior["sites"] and
             entries[case]["input_sha256"] == prior["sha256"] and
             entries[case]["input_fnv64"] == prior["fnv64"],
             f"reused hot-quarter input differs: {density}")
    return entries, blobs


def manifest_data():
    for name, pin in PRIOR_PINS.items():
        pinned_file(PRIOR / name, pin)
    entries, blobs = generated()
    manifest = {
        "schema": "mhgp9_s4a_cpu_physical_panel_inputs_v1",
        "status": "prepared_inputs_only", "scene": "SemanticKITTI 08/000200 ground-free",
        "backend": "cpu_reference", "grid": "1mm/u18", "K": K, "s": S,
        "workers": WORKERS, "static_threads": WORKERS,
        "seed": f"{SEED:016x}",
        "selection": "full-frame ground mask; global splitmix64(grid original site ID XOR seed), floor(n/4) and floor(n/2); intersect physical float32 signs; preserve global grid coordinates and input order",
        "source_generator_sha256": GENERATOR_SHA,
        "source_grid_manifest_sha256": file_sha(GRID / "MANIFEST.json"),
        "source_float32_manifest_sha256": file_sha(F32 / "MANIFEST.json"),
        "source_mask_sha256": file_sha(GRID / "mask.u8"),
        "source_kept_return_ids_sha256": file_sha(GRID / "kept_return_ids.u32le"),
        "binary_sha256": BINARY_SHA,
        "source_head": SNAPSHOT_HEAD, "source_objects": SOURCE_OBJECTS,
        "reused_receipt_pins": PRIOR_PINS, "cases": entries,
    }
    return manifest, blobs


def prepare():
    binary_identity()
    manifest, blobs = manifest_data()
    for case, data in blobs.items():
        put_immutable(HERE / "inputs" / f"{case}.u32le", data["points"])
        for kind in ("original_site_ids", "raw_return_ids", "full_site_ranks"):
            put_immutable(HERE / "inputs" / f"{case}.{kind}.u32le", data[kind])
    put_immutable(HERE / "MANIFEST.json", canonical(manifest))
    print("prepared", len(blobs), "physical sector/density inputs", flush=True)
    return manifest


def command(entry, arm, path, binary=BINARY):
    argv = ["nice", "-n", "19", str(binary), str(path), str(K), str(WORKERS),
            f"--s={S}", f"--static={WORKERS}", "--grid=1mm", "--catalogue-digest"]
    argv.extend(f"--lever={name}=1" for name in (*LEGACY_LEVERS, *SHARED_LEVERS))
    argv.extend(("--lever=q34_gpu_filter=0", "--lever=q34_gpu_certificates=0",
                 f"--lever=q34_batch_q3={int(arm == 's4a')}", "--lever=q34_gpu_q3=0"))
    return argv


def check_probe(value, entry, arm):
    need(type(value) is dict and value.get("schema") == "mhgp9_tower_probe_v20" and
         value.get("status") == "complete_relative" and
         value.get("reason") == "complete_relative_to_cross_checked_catalogue",
         "v20 complete-relative status")
    need(value.get("input") == {"format": "u32le", "grid": "1mm",
                                "sites": entry["sites"], "hash": entry["input_fnv64"]},
         "probe input identity")
    opts = value["options"]
    need(opts["K"] == opts["K_effective"] == K and opts["s"] == S and
         opts["workers"] == opts["tower_static_threads"] == WORKERS and
         opts["run_tower"] and not opts["certificate_judge"] and
         not opts["lanes_judge"], "probe options")
    levers = opts["levers"]
    need(all(levers[name] is True for name in (*LEGACY_LEVERS, *SHARED_LEVERS)) and
         all(levers[name] is False for name in ("q34_gpu_filter", "q34_gpu_certificates", "q34_gpu_q3")) and
         levers["q34_batch_q3"] is (arm == "s4a"), "probe levers")
    need([row["K"] for row in value["orders"]] == list(range(1, K + 1)) and
         value["catalogue"]["euler"]["status"] == "holds", "orders/Euler")
    ledger, batch = value["ledger"], value["q34_batch"]
    need(ledger["core_sites"] == ledger["dead_core_form_sites"] +
         2 * ledger["dead_core_loads"] and ledger["core_sites"] > 0,
         "core-sites identity")
    need(batch["used"] and batch["backend"] == "cpu" and
         batch["certificate_backend"] == "cpu" and
         all(batch[name] == 0 for name in ("device_ms", "certificate_device_ms",
                                               "filter_kernel_ms", "filter_transfer_ms")),
         "CPU batch backend")
    if arm == "s4a":
        need(batch["lanes_backend"] == "cpu" and batch["lanes_asked"] > 0 and
             batch["lanes_asked"] == batch["lanes_decided"] + batch["lanes_deferred"] and
             ledger["lanes_edges"] == batch["lanes_asked"] and
             ledger["lanes_census_point_tests"] > 0 and batch["lanes_device_ms"] == 0,
             "S4a CPU lanes")
    else:
        need(batch["lanes_asked"] == 0 and ledger["lanes_edges"] == 0 and
             ledger["lanes_census_point_tests"] == 0, "S3 CPU lanes")


def check_pair(left, right):
    need(left["tower_digest"] == right["tower_digest"] and
         left["catalogue_digest"] == right["catalogue_digest"] and
         left["orders"] == right["orders"] and left["catalogue"] == right["catalogue"] and
         left["generator"] == right["generator"], "S3/S4a output mismatch")
    need(all(left["ledger"][name] == right["ledger"][name] for name in COMMON_LEDGER),
         "S3/S4a common ledger mismatch")


def attempts():
    path = HERE / "ATTEMPTS.jsonl"
    return [json.loads(line) for line in path.read_text().splitlines() if line] if path.exists() else []


def verify_attempt_inventory(rows):
    need([row["attempt"] for row in rows] == list(range(1, len(rows) + 1)),
         "attempt numbers are not contiguous")
    expected = {str(captured_path(row["attempt"], row["case"], row["arm"], suffix).relative_to(HERE))
                for row in rows for suffix in ("stdout", "stderr")}
    folder = HERE / "runs"
    actual = {str(path.relative_to(HERE)) for path in folder.iterdir()} if folder.exists() else set()
    need(actual == expected, "unrecorded or missing raw attempt file")


def append(row):
    with (HERE / "ATTEMPTS.jsonl").open("a") as stream:
        stream.write(json.dumps(row, sort_keys=True, allow_nan=False) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def next_attempt():
    rows = attempts()
    verify_attempt_inventory(rows)
    return len(rows) + 1


def captured_path(number, case, arm, suffix):
    return HERE / "runs" / f"attempt_{number:04d}_{case}_{arm}.{suffix}"


def accepted_rows(manifest):
    found = {}
    for row in attempts():
        case, arm = row["case"], row["arm"]
        need(case in manifest["cases"] and arm in ARMS, "unknown attempt")
        if row["outcome"] == "accepted":
            need((case, arm) not in found, f"duplicate accepted case: {case}/{arm}")
            found[(case, arm)] = row
    return found


def cpu_busy_ticks():
    fields = (Path("/proc/stat").read_text().splitlines()[0]).split()
    need(fields[0] == "cpu" and len(fields) >= 9, "Linux aggregate CPU counters")
    values = [int(item) for item in fields[1:9]]
    return sum(values[i] for i in (0, 1, 2, 5, 6, 7))


def preflight_other_cores():
    before = cpu_busy_ticks()
    tick = time.monotonic()
    time.sleep(0.5)
    elapsed = time.monotonic() - tick
    return (cpu_busy_ticks() - before) / os.sysconf("SC_CLK_TCK") / elapsed


def import_hot(manifest):
    for name, pin in PRIOR_PINS.items():
        pinned_file(PRIOR / name, pin)
    previous = [json.loads(line) for line in (PRIOR / "CASES.jsonl").read_text().splitlines()]
    prior_entries = json.loads((PRIOR / "PREPARED.json").read_text())["cases"]
    hot = module(PRIOR_SCRIPT, "mhgp9_pinned_s4a_hot_quarter_reader")
    done = accepted_rows(manifest)
    for density in DENSITIES:
        case = f"{density}_{HOT}"
        old_name = HOT_NAMES[density]
        for arm in ARMS:
            if (case, arm) in done:
                continue
            accepted = [row for row in previous if row["name"] == old_name and
                        row["arm"] == arm and row["validated"] and
                        row["load_before"][0] <= 8 and row["load_after"][0] <= 8]
            need(len(accepted) == 1, f"unique prior accepted run: {old_name}/{arm}")
            source = accepted[0]
            old_input = PRIOR / prior_entries[old_name]["file"]
            need(file_sha(old_input) == manifest["cases"][case]["input_sha256"] and
                 old_input.read_bytes() == (HERE / manifest["cases"][case]["input_file"]).read_bytes(),
                 f"prior input differs: {case}")
            out_data = (PRIOR / source["stdout_file"]).read_bytes()
            err_data = (PRIOR / source["stderr_file"]).read_bytes()
            need(sha(out_data) == source["stdout_sha256"] and
                 sha(err_data) == source["stderr_sha256"], "prior raw output hash")
            probe = json.loads(out_data)
            hot.check_probe(probe, old_name, arm, prior_entries[old_name])
            check_probe(probe, manifest["cases"][case], arm)
            number = next_attempt()
            stdout_path = captured_path(number, case, arm, "stdout")
            stderr_path = captured_path(number, case, arm, "stderr")
            put_immutable(stdout_path, out_data)
            put_immutable(stderr_path, err_data)
            append({"schema": "mhgp9_s4a_cpu_physical_panel_attempt_v1",
                    "attempt": number, "kind": "imported", "outcome": "accepted",
                    "case": case, "arm": arm, "source_receipt": str(PRIOR.relative_to(REPO)),
                    "source_attempt": source["attempt"],
                    "source_row_sha256": sha(canonical(source)),
                    "source_stdout": source["stdout_file"],
                    "source_stderr": source["stderr_file"],
                    "argv": source["argv"], "binary_sha256_before": BINARY_SHA,
                    "binary_sha256_after": BINARY_SHA,
                    "input_sha256_before": manifest["cases"][case]["input_sha256"],
                    "input_sha256_after": manifest["cases"][case]["input_sha256"],
                    "stdout_file": str(stdout_path.relative_to(HERE)),
                    "stderr_file": str(stderr_path.relative_to(HERE)),
                    "stdout_sha256": sha(out_data), "stderr_sha256": sha(err_data),
                    "wall_s": source["wall_s"], "cpu_user_s": source["cpu_user_s"],
                    "cpu_system_s": source["cpu_system_s"],
                    "load_before": source["load_before"], "load_after": source["load_after"],
                    "started_utc": source["started_utc"], "ended_utc": source["ended_utc"],
                    "exit_code": source["exit_code"], "timed_out": False,
                    "validation_error": None})
            print("imported", case, arm, "prior attempt", source["attempt"], flush=True)


def run_one(manifest, case, arm, max_load, max_other_busy, timeout):
    binary_identity()
    entry = manifest["cases"][case]
    input_path = HERE / entry["input_file"]
    before_input = file_sha(input_path)
    need(before_input == entry["input_sha256"], "input changed before run")
    load_before = os.getloadavg()
    if load_before[0] > max_load:
        raise HostBusy(f"host load {load_before[0]:.2f} exceeds {max_load}; no case started")
    pre_other = preflight_other_cores()
    if pre_other > max_other_busy:
        raise HostBusy(f"other processes use {pre_other:.2f} cores before run; no case started")
    argv = command(entry, arm, input_path)
    number = next_attempt()
    stdout_path = captured_path(number, case, arm, "stdout")
    stderr_path = captured_path(number, case, arm, "stderr")
    need(not stdout_path.exists() and not stderr_path.exists(), "attempt path already used")
    start = dt.datetime.now(dt.timezone.utc).isoformat()
    resources_before = resource.getrusage(resource.RUSAGE_CHILDREN)
    host_busy_before = cpu_busy_ticks()
    tick = time.monotonic()
    process = None
    caught_signals = []

    def on_signal(signum, _frame):
        caught_signals.append(signal.Signals(signum).name)
        if process is not None and process.poll() is None:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass

    previous = {kind: signal.getsignal(kind) for kind in (signal.SIGINT, signal.SIGTERM)}
    for kind in previous:
        signal.signal(kind, on_signal)
    try:
        stdout, stderr, timed_out, launch_error = b"", b"", False, None
        try:
            process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                       start_new_session=True)
            if caught_signals:
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            try:
                stdout, stderr = process.communicate(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    stdout, stderr = process.communicate(timeout=5)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    stdout, stderr = process.communicate()
        except OSError as caught:
            launch_error = str(caught)
            stderr = launch_error.encode()
        code = process.returncode if process is not None else None
        elapsed = time.monotonic() - tick
        resources_after = resource.getrusage(resource.RUSAGE_CHILDREN)
        host_busy_after = cpu_busy_ticks()
        end = dt.datetime.now(dt.timezone.utc).isoformat()
        put_immutable(stdout_path, stdout)
        put_immutable(stderr_path, stderr)
        try:
            input_after = file_sha(input_path)
        except OSError:
            input_after = None
        try:
            binary_after = file_sha(BINARY)
        except OSError:
            binary_after = None
        load_after = os.getloadavg()
        own_cpu = (resources_after.ru_utime - resources_before.ru_utime +
                   resources_after.ru_stime - resources_before.ru_stime)
        other_busy = max(0.0, (host_busy_after - host_busy_before) /
                         os.sysconf("SC_CLK_TCK") - own_cpu) / max(elapsed, 1e-6)
        error = None
        try:
            need(not caught_signals and not timed_out and launch_error is None and code == 0 and
                 input_after == before_input and binary_after == BINARY_SHA,
                 "interrupted, timed out, failed, or input/binary drifted")
            check_probe(json.loads(stdout), entry, arm)
        except Exception as caught:
            error = str(caught)
        timing_ok = (load_before[0] <= max_load and
                     pre_other <= max_other_busy and other_busy <= max_other_busy)
        outcome = ("interrupted" if caught_signals else
                   "accepted" if error is None and timing_ok else
                   "contended" if error is None else "failed")
        append({"schema": "mhgp9_s4a_cpu_physical_panel_attempt_v1",
                "attempt": number, "kind": "executed", "outcome": outcome,
                "case": case, "arm": arm, "argv": argv,
                "binary_sha256_before": BINARY_SHA, "binary_sha256_after": binary_after,
                "input_sha256_before": before_input, "input_sha256_after": input_after,
                "stdout_file": str(stdout_path.relative_to(HERE)),
                "stderr_file": str(stderr_path.relative_to(HERE)),
                "stdout_sha256": sha(stdout), "stderr_sha256": sha(stderr),
                "wall_s": elapsed,
                "cpu_user_s": resources_after.ru_utime - resources_before.ru_utime,
                "cpu_system_s": resources_after.ru_stime - resources_before.ru_stime,
                "load_before": load_before, "load_after": load_after,
                "max_load": max_load, "max_other_busy_cores": max_other_busy,
                "preflight_other_busy_cores": pre_other,
                "during_other_busy_cores": other_busy,
                "started_utc": start, "ended_utc": end,
                "exit_code": code, "timed_out": timed_out,
                "caught_signals": caught_signals,
                "validation_error": error})
        print(case, arm, outcome, f"wall={elapsed:.3f}s",
              f"load={load_before[0]:.2f}->{load_after[0]:.2f}",
              f"other_busy={other_busy:.2f}", flush=True)
        need(outcome in ("accepted", "contended"),
             f"case retained as {outcome}: {case}/{arm}: {error}")
    finally:
        for kind, handler in previous.items():
            signal.signal(kind, handler)


def run(max_load, max_other_busy, timeout, max_new, wait_for_host):
    manifest = prepare()
    verify()
    import_hot(manifest)
    done = verify()
    newly_run = 0
    waited = 0.0
    # Spatial growth at full density is the first useful partial panel.
    # The manifest and summary keep their canonical quarter/half/full order.
    for density in reversed(DENSITIES):
        for sector in SECTORS:
            case = f"{density}_{sector}"
            for arm in ARMS:
                if (case, arm) in done:
                    continue
                if newly_run >= max_new:
                    print("stopped at max-new", newly_run, flush=True)
                    return
                while True:
                    try:
                        run_one(manifest, case, arm, max_load, max_other_busy, timeout)
                        break
                    except HostBusy as caught:
                        if waited >= wait_for_host:
                            raise
                        pause = min(15.0, wait_for_host - waited)
                        print(f"waiting {pause:g}s for host: {caught}", flush=True)
                        time.sleep(pause)
                        waited += pause
                newly_run += 1
                done = verify()
            if all((case, arm) in done for arm in ARMS):
                left, right = [json.loads((HERE / done[(case, arm)]["stdout_file"]).read_text())
                               for arm in ARMS]
                check_pair(left, right)
    finish()


def metric(value, name):
    if name == "chain_cpu_s":
        return value["chain_cpu_s"]
    if name == "chain_total_ms":
        return value["times_ms"]["chain_total"]
    return value["ledger"][name]


def slope(parent, child):
    need(parent["input"]["sites"] > child["input"]["sites"] > 0, "slope size order")
    return {name: (math.log(metric(parent, name) / metric(child, name)) /
                   math.log(parent["input"]["sites"] / child["input"]["sites"]))
            for name in METRICS if metric(parent, name) > 0 and metric(child, name) > 0}


def strict_time(row):
    return row["kind"] == "executed" and row["outcome"] == "accepted"


def guarded_wall_slope(parent_row, child_row, parent_sites, child_sites):
    if not (strict_time(parent_row) and strict_time(child_row)):
        return None
    return (math.log(parent_row["wall_s"] / child_row["wall_s"]) /
            math.log(parent_sites / child_sites))


def summary_of(manifest, selected):
    probes = {(case, arm): json.loads((HERE / row["stdout_file"]).read_text())
              for (case, arm), row in selected.items()}
    cases = {}
    for density in DENSITIES:
        for sector in SECTORS:
            case = f"{density}_{sector}"
            left, right = (probes[(case, arm)] for arm in ARMS)
            check_pair(left, right)
            cases[case] = {
                "sites": manifest["cases"][case]["sites"], "input_sha256": manifest["cases"][case]["input_sha256"],
                "tower_digest": left["tower_digest"], "catalogue_digest": left["catalogue_digest"],
                "catalogue_balls": left["catalogue"]["balls"],
                "core_sites": left["ledger"]["core_sites"],
                "dead_core_loads": left["ledger"]["dead_core_loads"],
                "expanded_pairs": left["ledger"]["expanded_pairs"],
                "arms": {arm: {"attempt": selected[(case, arm)]["attempt"],
                               "kind": selected[(case, arm)]["kind"],
                               "external_wall_s_observed": selected[(case, arm)]["wall_s"],
                               "timing_class": ("legacy_load_only" if selected[(case, arm)]["kind"] == "imported"
                                                else "new_guard_pass" if selected[(case, arm)]["outcome"] == "accepted"
                                                else "contention_censored"),
                               "chain_total_ms": probes[(case, arm)]["times_ms"]["chain_total"],
                               "chain_cpu_s": probes[(case, arm)]["chain_cpu_s"],
                               "lanes_census_point_tests": probes[(case, arm)]["ledger"]["lanes_census_point_tests"],
                               "lanes_asked": probes[(case, arm)]["q34_batch"]["lanes_asked"],
                               "lanes_deferred": probes[(case, arm)]["q34_batch"]["lanes_deferred"],
                               "peak_rss_kb": probes[(case, arm)]["peak_rss_kb"]}
                         for arm in ARMS},
            }
    density_slopes = []
    for sector in SECTORS:
        for low, high in (("quarter", "half"), ("half", "full")):
            low_case, high_case = f"{low}_{sector}", f"{high}_{sector}"
            low_sites, high_sites = (manifest["cases"][case]["sites"] for case in
                                     (low_case, high_case))
            density_slopes.append({"sector": sector, "from": low, "to": high,
                                   "from_sites": low_sites, "to_sites": high_sites,
                                   "site_ratio": high_sites / low_sites,
                                   "s3": slope(probes[(f"{high}_{sector}", "s3")],
                                               probes[(f"{low}_{sector}", "s3")]),
                                   "s4a": slope(probes[(f"{high}_{sector}", "s4a")],
                                                probes[(f"{low}_{sector}", "s4a")]),
                                   "qualified_external_wall_exponent": {
                                       arm: guarded_wall_slope(selected[(high_case, arm)],
                                                               selected[(low_case, arm)],
                                                               high_sites, low_sites)
                                       for arm in ARMS}})
    spatial_slopes = []
    edges = (("full", "half_x_neg"), ("full", "half_x_nonneg"),
             ("half_x_neg", "quarter_x_neg_y_neg"),
             ("half_x_neg", "quarter_x_neg_y_nonneg"),
             ("half_x_nonneg", "quarter_x_nonneg_y_neg"),
             ("half_x_nonneg", "quarter_x_nonneg_y_nonneg"))
    for density in DENSITIES:
        for parent, child in edges:
            parent_case, child_case = f"{density}_{parent}", f"{density}_{child}"
            parent_sites, child_sites = (manifest["cases"][case]["sites"] for case in
                                         (parent_case, child_case))
            spatial_slopes.append({"density": density, "parent": parent, "child": child,
                                   "parent_sites": parent_sites, "child_sites": child_sites,
                                   "site_ratio": parent_sites / child_sites,
                                   "s3": slope(probes[(f"{density}_{parent}", "s3")],
                                               probes[(f"{density}_{child}", "s3")]),
                                   "s4a": slope(probes[(f"{density}_{parent}", "s4a")],
                                                probes[(f"{density}_{child}", "s4a")]),
                                   "qualified_external_wall_exponent": {
                                       arm: guarded_wall_slope(selected[(parent_case, arm)],
                                                               selected[(child_case, arm)],
                                                               parent_sites, child_sites)
                                       for arm in ARMS}})
    spatial_partitions = []
    partitions = (("full", "half_x_neg", "half_x_nonneg"),
                  ("half_x_neg", "quarter_x_neg_y_neg", "quarter_x_neg_y_nonneg"),
                  ("half_x_nonneg", "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg"))
    for density in DENSITIES:
        for parent, first, second in partitions:
            parent_case = f"{density}_{parent}"
            child_cases = (f"{density}_{first}", f"{density}_{second}")
            parent_sites = manifest["cases"][parent_case]["sites"]
            child_sites = [manifest["cases"][case]["sites"] for case in child_cases]
            need(sum(child_sites) == parent_sites, "spatial partition size")
            quadratic_baseline = sum((n / parent_sites) ** 2 for n in child_sites)
            arm_rows = {}
            for arm in ARMS:
                parent_probe = probes[(parent_case, arm)]
                children = [probes[(case, arm)] for case in child_cases]
                ratios = {name: sum(metric(child, name) for child in children) /
                          metric(parent_probe, name)
                          for name in ("core_sites", "expanded_pairs", "chain_cpu_s")}
                time_rows = [selected[(case, arm)] for case in (parent_case, *child_cases)]
                wall_ok = all(strict_time(row) for row in time_rows)
                wall_ratio = (sum(row["wall_s"] for row in time_rows[1:]) /
                              time_rows[0]["wall_s"] if wall_ok else None)
                arm_rows[arm] = {"R_sum_children_over_parent": ratios,
                                 "R_over_B": {name: value / quadratic_baseline
                                              for name, value in ratios.items()},
                                 "qualified_external_wall_R": wall_ratio,
                                 "qualified_external_wall_R_over_B":
                                     wall_ratio / quadratic_baseline if wall_ok else None,
                                 "wall_timing_classes": ["legacy_load_only" if row["kind"] == "imported"
                                                         else "new_guard_pass" if row["outcome"] == "accepted"
                                                         else "contention_censored"
                                                         for row in time_rows]}
            spatial_partitions.append({"density": density, "parent": parent,
                                       "children": [first, second],
                                       "parent_sites": parent_sites,
                                       "child_sites": child_sites,
                                       "B_quadratic_site_fraction": quadratic_baseline,
                                       "arms": arm_rows})
    rows = list(selected.values())
    return {"schema": "mhgp9_s4a_cpu_physical_panel_summary_v3",
            "status": "complete_relative", "scope": "finite_growth_diagnostic_only",
            "backend": "cpu_reference", "scene": "SemanticKITTI 08/000200 ground-free",
            "K": K, "s": S, "workers": WORKERS, "static_threads": WORKERS,
            "seed": f"{SEED:016x}", "binary_sha256": BINARY_SHA,
            "cases": cases, "density_slopes": density_slopes,
            "spatial_slopes": spatial_slopes, "spatial_partitions": spatial_partitions,
            "wall_policy": "External wall exponents and partition ratios are qualified only when all endpoints are new executed attempts with load_before<=8 and preflight/during other_busy_cores<=2.5; imported walls and censored walls remain observed diagnostics. Distinct accepted attempts are not temporally paired repetitions.",
            "slope_metric_policy": {"primary": ["core_sites", "expanded_pairs", "chain_cpu_s",
                                         "lanes_census_point_tests"],
                                    "wall_indicative": ["chain_total_ms"]},
            "counts": {"inputs": len(cases), "validated_arms": len(rows),
                       "new_guard_pass_arms": sum(row["kind"] == "executed" and
                                                  row["outcome"] == "accepted" for row in rows),
                       "contention_censored_arms": sum(row["outcome"] == "contended" for row in rows),
                       "imported_arms": sum(row["kind"] == "imported" for row in rows),
                       "executed_arms": sum(row["kind"] == "executed" for row in rows),
                       "attempts_total": len(attempts()),
                       "density_links_core_ge_2": sum(row["s4a"]["core_sites"] >= 2 for row in density_slopes),
                       "spatial_links_core_ge_2": sum(row["s4a"]["core_sites"] >= 2 for row in spatial_slopes)},
            "external_wall_s_observed_sum": sum(row["wall_s"] for row in rows),
            "chain_cpu_s_sum": sum(probes[key]["chain_cpu_s"] for key in probes)}


def verify(complete=False):
    manifest, blobs = manifest_data()
    need((HERE / "MANIFEST.json").read_bytes() == canonical(manifest), "manifest differs")
    for case, data in blobs.items():
        need((HERE / "inputs" / f"{case}.u32le").read_bytes() == data["points"], f"input differs: {case}")
        for kind in ("original_site_ids", "raw_return_ids", "full_site_ranks"):
            need((HERE / "inputs" / f"{case}.{kind}.u32le").read_bytes() == data[kind],
                 f"input IDs differ: {case}/{kind}")
    rows = attempts()
    verify_attempt_inventory(rows)
    seen_attempts = set()
    selected = {}
    prior_rows = [json.loads(line) for line in (PRIOR / "CASES.jsonl").read_text().splitlines()]
    for row in rows:
        case, arm, number = row["case"], row["arm"], row["attempt"]
        need(row["schema"] == "mhgp9_s4a_cpu_physical_panel_attempt_v1" and
             case in manifest["cases"] and arm in ARMS and
             type(number) is int and number > 0 and number not in seen_attempts,
             "attempt metadata")
        seen_attempts.add(number)
        entry = manifest["cases"][case]
        stdout_path = captured_path(number, case, arm, "stdout")
        stderr_path = captured_path(number, case, arm, "stderr")
        need(row["stdout_file"] == str(stdout_path.relative_to(HERE)) and
             row["stderr_file"] == str(stderr_path.relative_to(HERE)) and
             file_sha(stdout_path) == row["stdout_sha256"] and
             file_sha(stderr_path) == row["stderr_sha256"] and
             row["binary_sha256_before"] == BINARY_SHA and
             row["input_sha256_before"] == entry["input_sha256"], "raw attempt hashes")
        if row["kind"] == "imported":
            old_name = HOT_NAMES[entry["density"]]
            need(entry["sector"] == HOT and row["source_receipt"] == str(PRIOR.relative_to(REPO)) and
                 row["source_row_sha256"] and row["outcome"] == "accepted", "imported source metadata")
            source = [item for item in prior_rows if item["attempt"] == row["source_attempt"]]
            need(len(source) == 1 and source[0]["name"] == old_name and source[0]["arm"] == arm and
                 sha(canonical(source[0])) == row["source_row_sha256"] and
                 source[0]["argv"] == row["argv"] and
                 file_sha(PRIOR / source[0]["stdout_file"]) == row["stdout_sha256"] and
                 file_sha(PRIOR / source[0]["stderr_file"]) == row["stderr_sha256"] and
                 source[0]["wall_s"] == row["wall_s"], "imported source identity")
        else:
            need(row["kind"] == "executed" and len(row["argv"]) >= 4 and
                 row["argv"] == command(entry, arm, HERE / entry["input_file"],
                                        Path(row["argv"][3])),
                 "executed command")
        if row["outcome"] in ("accepted", "contended"):
            need(row["exit_code"] == 0 and not row["timed_out"] and
                 row["validation_error"] is None and
                 row["binary_sha256_after"] == BINARY_SHA and
                 row["input_sha256_after"] == entry["input_sha256"] and
                 row["load_before"][0] <= 8,
                 "validated attempt quality")
            if row["kind"] == "imported":
                need(row["outcome"] == "accepted" and row["load_after"][0] <= 8,
                     "legacy imported time guard")
            if row["kind"] == "executed":
                need(row["max_load"] <= 8 and
                     row["load_before"][0] <= row["max_load"] and
                     row["max_other_busy_cores"] <= 2.5 and
                     row["preflight_other_busy_cores"] <= row["max_other_busy_cores"],
                     "executed attempt preflight guard")
                if row["outcome"] == "accepted":
                    need(row["during_other_busy_cores"] <= row["max_other_busy_cores"],
                         "accepted attempt CPU contention")
            probe = json.loads(stdout_path.read_text())
            check_probe(probe, entry, arm)
            previous = selected.get((case, arm))
            if previous:
                old_probe = json.loads((HERE / previous["stdout_file"]).read_text())
                need(all(probe[name] == old_probe[name] for name in
                         ("tower_digest", "catalogue_digest", "orders", "catalogue",
                          "generator", "ledger")), "repeated valid output differs")
            if previous is None or (previous["outcome"] != "accepted" and
                                    row["outcome"] == "accepted"):
                selected[(case, arm)] = row
    for density in DENSITIES:
        for sector in SECTORS:
            case = f"{density}_{sector}"
            if all((case, arm) in selected for arm in ARMS):
                left, right = [json.loads((HERE / selected[(case, arm)]["stdout_file"]).read_text())
                               for arm in ARMS]
                check_pair(left, right)
    summary_path = HERE / "SUMMARY.json"
    if complete or summary_path.exists():
        need(len(selected) == 42 and summary_path.read_bytes() == canonical(summary_of(manifest, selected)),
             "complete summary differs")
    sums = HERE / "SHA256SUMS"
    if sums.exists():
        files = sorted(path for path in HERE.rglob("*") if path.is_file() and path != sums)
        expected = "".join(f"{file_sha(path)}  {path.relative_to(HERE)}\n" for path in files).encode()
        need(sums.read_bytes() == expected, "SHA256SUMS file inventory or hash differs")
    print("verified", len(selected), "/ 42 validated arms;", len(attempts()), "attempts", flush=True)
    return selected


def finish():
    selected = verify()
    need(len(selected) == 42, "cannot finish an incomplete output panel")
    manifest = json.loads((HERE / "MANIFEST.json").read_text())
    put_immutable(HERE / "SUMMARY.json", canonical(summary_of(manifest, selected)))
    verify(complete=True)
    print("complete panel", flush=True)


def close_hashes():
    verify(complete=True)
    sums = HERE / "SHA256SUMS"
    files = sorted(path for path in HERE.rglob("*") if path.is_file() and path != sums)
    data = "".join(f"{file_sha(path)}  {path.relative_to(HERE)}\n" for path in files).encode()
    put_immutable(sums, data)
    verify(complete=True)
    print("closed", len(files), "files", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("prepare", "run", "verify", "finish", "close"))
    parser.add_argument("--max-load", type=float, default=8.0)
    parser.add_argument("--max-other-busy", type=float, default=2.5)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--max-new", type=int, default=36)
    parser.add_argument("--wait-for-host", type=float, default=0.0,
                        help="maximum total seconds spent waiting for preflight gates")
    args = parser.parse_args()
    need(math.isfinite(args.max_load) and 0 < args.max_load <= 8 and
         math.isfinite(args.max_other_busy) and 0 < args.max_other_busy <= 2.5 and
         1 <= args.timeout <= 3600 and args.max_new >= 0 and
         math.isfinite(args.wait_for_host) and 0 <= args.wait_for_host <= 3600,
         "runner arguments")
    if args.mode == "prepare":
        prepare()
    elif args.mode == "run":
        run(args.max_load, args.max_other_busy, args.timeout, args.max_new,
            args.wait_for_host)
    elif args.mode == "verify":
        verify()
    elif args.mode == "finish":
        finish()
    else:
        close_hashes()


if __name__ == "__main__":
    main()
