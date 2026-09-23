#!/usr/bin/env python3
"""Pinned K10 LiDAR physical-sector/density measurements and strict reader."""

import argparse
import datetime as dt
import hashlib
import json
import math
import resource
import struct
import subprocess
import time
from pathlib import Path

if not __debug__:
    raise SystemExit("Assertions required; Python -O is unsupported.")

HERE = Path(__file__).resolve().parent
AUDITS = HERE.parent
INPUT = Path("/tmp/mhgp9-raw-k10-density-20260923")
PROBE = Path("/workspaces/E-HGP/build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe")
K5 = AUDITS / "lidar_raw_physical_scaling_20260923"
FULL = AUDITS / "lidar_raw_k10_density_20260923"
BINARY_SHA = "e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80"
MANIFEST_SHA = "6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095"
DENSITIES = ("quarter", "half", "full")
HALVES = ("half_x_neg", "half_x_nonneg")
QUARTERS = ("quarter_x_neg_y_neg", "quarter_x_neg_y_nonneg",
            "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg")
QUARTER_PRIORITY = (QUARTERS[2], QUARTERS[0], QUARTERS[1], QUARTERS[3])
SECTORS = ("full",) + HALVES + QUARTERS
ORDER = tuple((d, s) for s in HALVES for d in DENSITIES) + tuple(
    (d, s) for s in QUARTER_PRIORITY for d in DENSITIES)
LEVERS = ("atlas_saturate_deep", "q3_leaf_census", "q34_dead_lanes",
          "q34_witness_cache", "q34_dead_core", "tower_meb_proposal")
METRICS = ("chain_cpu_s", "chain_wall_s", "q34_wall_s", "dead_core_loads",
           "dead_core_form_sites", "expanded_pairs", "catalogue_balls",
           "tower_nodes", "core_cover_node_visits", "core_cover_bound_tests")
VOLATILE_LEDGER_PREFIXES = ("witness_cache_", "witness_pair_")


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def ids(blob):
    assert len(blob) % 4 == 0
    return [x for (x,) in struct.iter_unpack("<I", blob)]


def fnv(raw):
    assert len(raw) % 12 == 0
    h = 14695981039346656037
    for b in (len(raw) // 12).to_bytes(8, "little"):
        h = ((h ^ b) * 1099511628211) & ((1 << 64) - 1)
    for value in ids(raw):
        assert value < (1 << 18)
        for b in value.to_bytes(8, "little"):
            h = ((h ^ b) * 1099511628211) & ((1 << 64) - 1)
    return f"{h:016x}"


def pinned_blob(directory, name):
    hashes = {name: digest for digest, name in (
        line.split("  ", 1) for line in
        (directory / "SHA256SUMS").read_text().splitlines())}
    blob = (directory / name).read_bytes()
    assert sha(blob) == hashes[name]
    return blob


def manifest():
    blob = (INPUT / "MANIFEST.json").read_bytes()
    assert sha(blob) == sha((K5 / "MANIFEST.json").read_bytes()) == MANIFEST_SHA
    obj = json.loads(blob)
    assert obj["schema"] == "mhgp9_raw_physical_sector_density_inputs_v1"
    assert obj["raw_to_grid_bijective"] and obj["raw_to_float32_bijective"]
    return obj


def entry_for(m, density, sector):
    entry = m["datasets"][density][sector]
    point = (INPUT / entry["points_file"]).read_bytes()
    original = (INPUT / entry["raw_return_ids_file"]).read_bytes()
    grid = (INPUT / entry["grid_full_site_ids_file"]).read_bytes()
    assert sha(point) == entry["points_sha256"]
    assert sha(original) == entry["raw_return_ids_sha256"]
    assert sha(grid) == entry["grid_full_site_ids_sha256"]
    assert len(point) == 12 * entry["sites"]
    assert len(original) == len(grid) == 4 * entry["sites"]
    raw_ids = set(ids(original))
    assert len(raw_ids) == entry["sites"]
    return entry, point, raw_ids


def validate_inputs(m):
    by_density = {}
    for d in DENSITIES:
        sets = {s: entry_for(m, d, s)[2] for s in SECTORS}
        assert sets[HALVES[0]].isdisjoint(sets[HALVES[1]])
        assert sets[HALVES[0]] | sets[HALVES[1]] == sets["full"]
        assert sets[QUARTERS[0]] | sets[QUARTERS[1]] == sets[HALVES[0]]
        assert sets[QUARTERS[2]] | sets[QUARTERS[3]] == sets[HALVES[1]]
        assert sum(map(len, (sets[s] for s in QUARTERS))) == len(sets["full"])
        by_density[d] = sets
    for s in SECTORS:
        assert by_density["quarter"][s] < by_density["half"][s] < by_density["full"][s]


def command(entry):
    return ["nice", "-n", "19", str(PROBE), str(INPUT / entry["points_file"]),
            "10", "8", "--s=8", "--static=8", "--grid=1mm"]


def validate_probe(probe, entry, point, d, s):
    assert probe["schema"] == "mhgp9_tower_probe_v12"
    assert probe["status"] == "complete_relative"
    assert probe["reason"] == "complete_relative_to_cross_checked_catalogue"
    assert probe["input"] == {"format": "u32le", "grid": "1mm",
                              "sites": entry["sites"], "hash": fnv(point)}
    opt = probe["options"]
    assert opt["K"] == opt["K_effective"] == 10
    assert opt["s"] == opt["workers"] == opt["tower_static_threads"] == 8
    assert opt["run_tower"] and opt["levers"] == {x: True for x in LEVERS}
    assert [o["K"] for o in probe["orders"]] == list(range(1, 11))
    assert all(o["nodes"] == o["births"] + o["merges"] for o in probe["orders"])
    cat = probe["catalogue"]
    assert cat["balls"] == cat["unique_keys"] == sum(cat["by_qmin"])
    assert cat["shell_over_12"] == 0
    assert probe["chain_cpu_s"] > 0 and probe["times_ms"]["chain_total"] > 0
    digest = probe["tower_digest"]
    assert len(digest) == 16 and all(c in "0123456789abcdef" for c in digest)
    k5 = json.loads(pinned_blob(K5, f"{d}_{s}.stdout"))
    assert k5["input"] == probe["input"]
    assert k5["orders"] == probe["orders"][:5]


def validate_row(row, m):
    d, s = row["density"], row["sector"]
    name = f"{d}_{s}"
    assert (d, s) in ORDER and row["name"] == name
    entry, point, _ = entry_for(m, d, s)
    assert row["schema"] == "mhgp9_raw_k10_sector_density_case_v1"
    assert row["sites"] == entry["sites"]
    assert row["input_sha256"] == sha(point) and row["input_fnv"] == fnv(point)
    assert row["binary_sha256"] == BINARY_SHA and row["argv"] == command(entry)
    if "binary_sha256_before" in row:
        assert row["binary_sha256_before"] == row["binary_sha256_after"] == BINARY_SHA
        assert row["input_sha256_before"] == row["input_sha256_after"] == sha(point)
    # Initial unguarded attempts are retained and checked, but never selected
    # for the final summary. Their guarded retries carry the authority.
    out = (HERE / row["stdout_file"]).read_bytes()
    err = (HERE / row["stderr_file"]).read_bytes()
    assert sha(out) == row["stdout_sha256"] and sha(err) == row["stderr_sha256"]
    if row["validated"]:
        assert row["exit_code"] == 0 and not row["timed_out"]
        assert json.loads(out) == row["probe"]
        validate_probe(row["probe"], entry, point, d, s)
    return row["validated"]


def run(m, timeout, stage):
    assert sha(PROBE.read_bytes()) == BINARY_SHA
    receipt = HERE / "CASES.jsonl"
    rows = [json.loads(x) for x in receipt.read_text().splitlines() if x] if receipt.exists() else []
    done = set()
    for row in rows:
        if validate_row(row, m) and "binary_sha256_before" in row:
            assert row["name"] not in done
            done.add(row["name"])
    selected = ORDER[:6] if stage == "halves" else ORDER[:9] if stage == "hot_quarter" else ORDER
    for d, s in selected:
        name = f"{d}_{s}"
        if name in done:
            continue
        attempt = sum(row["name"] == name for row in rows)
        tag = name if attempt == 0 else f"{name}.retry{attempt}"
        stdout_file, stderr_file = f"{tag}.stdout", f"{tag}.stderr"
        entry, point, _ = entry_for(m, d, s)
        argv = command(entry)
        binary_before = sha(PROBE.read_bytes())
        input_before = sha((INPUT / entry["points_file"]).read_bytes())
        started = dt.datetime.now(dt.timezone.utc).isoformat()
        before = resource.getrusage(resource.RUSAGE_CHILDREN)
        tick = time.monotonic()
        try:
            p = subprocess.run(argv, capture_output=True, timeout=timeout)
            out, err, code, timed_out = p.stdout, p.stderr, p.returncode, False
        except subprocess.TimeoutExpired as ex:
            out, err, code, timed_out = ex.stdout or b"", ex.stderr or b"", None, True
        elapsed = time.monotonic() - tick
        after = resource.getrusage(resource.RUSAGE_CHILDREN)
        binary_after = sha(PROBE.read_bytes())
        input_after = sha((INPUT / entry["points_file"]).read_bytes())
        ended = dt.datetime.now(dt.timezone.utc).isoformat()
        (HERE / stdout_file).write_bytes(out)
        (HERE / stderr_file).write_bytes(err)
        probe, failure = None, None
        try:
            assert binary_before == binary_after == BINARY_SHA
            assert input_before == input_after == sha(point)
            assert code == 0 and not timed_out
            probe = json.loads(out)
            validate_probe(probe, entry, point, d, s)
        except (AssertionError, KeyError, TypeError, ValueError) as ex:
            failure = f"{type(ex).__name__}: {ex}"
        row = {"schema": "mhgp9_raw_k10_sector_density_case_v1", "name": name,
               "density": d, "sector": s, "sites": entry["sites"],
               "input_sha256": sha(point), "input_fnv": fnv(point),
               "binary_sha256": BINARY_SHA, "argv": argv,
               "binary_sha256_before": binary_before,
               "binary_sha256_after": binary_after,
               "input_sha256_before": input_before,
               "input_sha256_after": input_after,
               "started_utc": started, "ended_utc": ended,
               "external_wall_s": elapsed,
               "external_cpu_user_s": after.ru_utime - before.ru_utime,
               "external_cpu_system_s": after.ru_stime - before.ru_stime,
               "timed_out": timed_out, "exit_code": code,
               "validated": failure is None, "validation_error": failure,
               "attempt": attempt, "stdout_file": stdout_file,
               "stderr_file": stderr_file, "stdout_sha256": sha(out),
               "stderr_sha256": sha(err), "probe": probe}
        with receipt.open("a") as stream:
            stream.write(json.dumps(row, sort_keys=True) + "\n")
        rows.append(row)
        print(json.dumps({"name": name, "sites": entry["sites"],
                          "wall_s": elapsed, "validated": failure is None,
                          "cpu_s": probe.get("chain_cpu_s") if isinstance(probe, dict) else None,
                          "forms": probe.get("ledger", {}).get("dead_core_form_sites")
                          if isinstance(probe, dict) else None}), flush=True)
        if failure is not None:
            raise RuntimeError(f"case {name} failed; attempt retained")


def metrics(probe):
    return {"sites": probe["input"]["sites"],
            "chain_cpu_s": probe["chain_cpu_s"],
            "chain_wall_s": probe["times_ms"]["chain_total"] / 1000,
            "q34_wall_s": probe["times_ms"]["q34"] / 1000,
            "dead_core_loads": probe["ledger"]["dead_core_loads"],
            "dead_core_form_sites": probe["ledger"]["dead_core_form_sites"],
            "expanded_pairs": probe["ledger"]["expanded_pairs"],
            "catalogue_balls": probe["catalogue"]["balls"],
            "tower_nodes": sum(o["nodes"] for o in probe["orders"]),
            "core_cover_node_visits": probe["ledger"]["core_cover_node_visits"],
            "core_cover_bound_tests": probe["ledger"]["core_cover_bound_tests"],
            "peak_rss_kb": probe["peak_rss_kb"], "tower_digest": probe["tower_digest"]}


def verify(m, stage):
    validate_inputs(m)
    rows = [json.loads(x) for x in (HERE / "CASES.jsonl").read_text().splitlines() if x]
    good = {}
    for row in rows:
        if validate_row(row, m) and "binary_sha256_before" in row:
            assert row["name"] not in good
            good[row["name"]] = row
    selected = ORDER[:6] if stage == "halves" else ORDER[:9] if stage == "hot_quarter" else ORDER
    wanted = {f"{d}_{s}" for d, s in selected}
    assert set(good) == wanted
    replay = {}
    initial = {row["name"]: row for row in rows if "binary_sha256_before" not in row}
    for name, old in initial.items():
        assert name in good
        fresh = good[name]
        before, after = old["probe"], fresh["probe"]
        stable_fields = ("catalogue", "orders", "tower_digest", "status",
                         "reason", "options", "input", "generator", "tower_work")
        for key in stable_fields:
            assert before[key] == after[key], (name, key)
        assert before["ledger"].keys() == after["ledger"].keys()
        changed_ledger = sorted(
            key for key in before["ledger"]
            if before["ledger"][key] != after["ledger"][key])
        assert all(key.startswith(VOLATILE_LEDGER_PREFIXES)
                   for key in changed_ledger), (name, changed_ledger)
        prior_m, final_m = metrics(before), metrics(after)
        replay[name] = {
            "equal_fields": list(stable_fields),
            "changed_ledger_keys": changed_ledger,
            "metric_relative_change": {
                key: (final_m[key] - prior_m[key]) / prior_m[key]
                for key in METRICS + ("peak_rss_kb",)}}
    by_name = {name: metrics(row["probe"]) for name, row in good.items()}
    for d in DENSITIES:
        probe = json.loads(pinned_blob(FULL, f"{d}.stdout"))
        entry, point, _ = entry_for(m, d, "full")
        validate_probe(probe, entry, point, d, "full")
        by_name[f"{d}_full"] = metrics(probe)
    slopes = {}
    for s in SECTORS:
        if f"quarter_{s}" not in by_name:
            continue
        slopes[s] = []
        for a, b in zip(DENSITIES, DENSITIES[1:]):
            left, right = by_name[f"{a}_{s}"], by_name[f"{b}_{s}"]
            slopes[s].append({"from": a, "to": b,
                "n_ratio": right["sites"] / left["sites"],
                "p": {key: math.log(right[key] / left[key]) /
                       math.log(right["sites"] / left["sites"]) for key in METRICS}})
    spatial = {}
    for d in DENSITIES:
        whole = by_name[f"{d}_full"]
        spatial[d] = {}
        for group, sectors in (("halves", HALVES), ("quarters", QUARTERS)):
            if not all(f"{d}_{s}" in by_name for s in sectors):
                continue
            pieces = [by_name[f"{d}_{s}"] for s in sectors]
            assert sum(p["sites"] for p in pieces) == whole["sites"]
            spatial[d][group] = {
                "quadratic_reference": sum((p["sites"] / whole["sites"]) ** 2 for p in pieces),
                "sum_piece_over_full": {key: sum(p[key] for p in pieces) / whole[key]
                                        for key in METRICS}}
    links = (("full", "half_x_neg"), ("full", "half_x_nonneg"),
             ("half_x_neg", "quarter_x_neg_y_neg"),
             ("half_x_neg", "quarter_x_neg_y_nonneg"),
             ("half_x_nonneg", "quarter_x_nonneg_y_neg"),
             ("half_x_nonneg", "quarter_x_nonneg_y_nonneg"))
    parent_child = {}
    for d in DENSITIES:
        parent_child[d] = []
        for parent, child in links:
            if f"{d}_{child}" not in by_name:
                continue
            p, c = by_name[f"{d}_{parent}"], by_name[f"{d}_{child}"]
            parent_child[d].append({"parent": parent, "child": child,
                "n_ratio_parent_over_child": p["sites"] / c["sites"],
                "p_parent_over_child": {
                    key: math.log(p[key] / c[key]) / math.log(p["sites"] / c["sites"])
                    for key in METRICS}})
    result = {"schema": "mhgp9_raw_k10_sector_density_summary_v1",
              "binary_sha256": BINARY_SHA, "source_manifest_sha256": MANIFEST_SHA,
              "stage": stage, "attempts": len(rows), "valid_new_cases": len(good),
              "unguarded_initial_attempts": sum(
                  "binary_sha256_before" not in row for row in rows),
              "replay_consistency": replay,
              "published_full_cases": 3, "metrics": by_name,
              "density_slopes_by_sector": slopes,
              "spatial_sums_by_density": spatial,
              "spatial_parent_child_slopes_by_density": parent_child}
    (HERE / "SUMMARY.json").write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"stage": stage, "valid_new_cases": len(good),
                      "density_slopes_by_sector": slopes,
                      "spatial_sums_by_density": spatial}, indent=2, sort_keys=True))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("run", "verify"))
    parser.add_argument("--stage", choices=("halves", "hot_quarter", "all"), default="all")
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()
    m = manifest()
    if args.mode == "run":
        run(m, args.timeout, args.stage)
    else:
        verify(m, args.stage)


if __name__ == "__main__":
    main()
