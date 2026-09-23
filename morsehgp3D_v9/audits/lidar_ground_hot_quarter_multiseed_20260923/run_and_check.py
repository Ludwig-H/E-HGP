#!/usr/bin/env python3
"""Repeat globally thinned K10 density slopes in one physical no-ground quarter."""

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


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
SOURCE = REPO / "morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6"
GRID = SOURCE / "scene_02_grid"
F32 = SOURCE / "scene_02_float32"
PHYSICAL = HERE.parent / "lidar_scene02_physical_cut_20260923"
HISTORICAL = HERE.parent / "lidar_density_scene02_20260923"
BINARY = REPO / "build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe"
SECTOR = "quarter_x_nonneg_y_neg"
SEEDS = {"s1": 0x7D1C9A5EB3F24680, "s2": 0x2B85D41E0C93A76F}
HISTORICAL_SEED = 0xD1DA73A520260923
BINARY_SHA = "e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80"
GRID_MANIFEST_SHA = "a37d4e46b529885b9fc370a25e361a302762b95109f00928b2b2e96fcac799da"
F32_MANIFEST_SHA = "ce1ffee9de0a43a4aed101d880bb769614b9a5cb1583c3e4af6899835eadd387"
PHYSICAL_MANIFEST_SHA = "93c519a47d7879ae2ab0c2dc250dba72b96ab7da3f979a3120c34f49270d10aa"
PHYSICAL_RESULTS_SHA = "9301359bf8aba24f989fafd4c7c2acac9cff192a35d2d1ee961b602e01330d96"
PHYSICAL_STDOUT_SHA = "7493d0fa054886198c73b760be41ff29809ad4badda3bc94363be447cf25d374"
HISTORICAL_SUMMARY_SHA = "14af25adc2ccf26e46f7a78c6ce5395191f083e9b56d38d4f004c12ff041bde7"
MASK64 = (1 << 64) - 1
LEVERS = ("atlas_saturate_deep", "q3_leaf_census", "q34_dead_lanes",
          "q34_witness_cache", "q34_dead_core", "tower_meb_proposal")


def need(test, message):
    if not test:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    return sha(path.read_bytes())


def words(data):
    need(len(data) % 4 == 0, "u32 alignment")
    return [value for (value,) in struct.iter_unpack("<I", data)]


def mix(value):
    value = (value + 0x9E3779B97F4A7C15) & MASK64
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK64
    return value ^ (value >> 31)


def fnv_points(data):
    need(len(data) % 12 == 0, "point alignment")
    h = 14695981039346656037
    for value in (len(data) // 12, *words(data)):
        need(value < (1 << 18), "u18 domain")
        for shift in range(0, 64, 8):
            h = ((h ^ ((value >> shift) & 255)) * 1099511628211) & MASK64
    return f"{h:016x}"


def dataset(folder, manifest, kind):
    entry = manifest["datasets"]["full"]
    field = {"points": "points", "original": "original_site_ids", "site": "site_ids"}[kind]
    data = (folder / entry[field + "_file"]).read_bytes()
    need(sha(data) == entry[field + "_sha256"], f"source {folder.name}/{kind} SHA")
    return data


def source():
    need(file_sha(GRID / "MANIFEST.json") == GRID_MANIFEST_SHA, "grid manifest SHA")
    need(file_sha(F32 / "MANIFEST.json") == F32_MANIFEST_SHA, "float32 manifest SHA")
    gm, fm = [json.loads((p / "MANIFEST.json").read_text()) for p in (GRID, F32)]
    gpoints, fpoints = dataset(GRID, gm, "points"), dataset(F32, fm, "points")
    gids, fids = words(dataset(GRID, gm, "original")), words(dataset(F32, fm, "original"))
    n = len(gids)
    need(n == len(fids) == 45845 and len(gpoints) == len(fpoints) == 12*n,
         "retained full size")
    need(words(dataset(GRID, gm, "site")) == list(range(n)) and
         words(dataset(F32, fm, "site")) == list(range(n)), "source site rank")
    gmap, fmap = [words((p / "raw_to_original.u32le").read_bytes()) for p in (GRID, F32)]
    need(len(gmap) == len(fmap) == 125526 and len(set(gmap)) == len(set(fmap)) == len(gmap),
         "raw-to-original bijections")
    ginv, finv = ({original: raw for raw, original in enumerate(mapping)}
                  for mapping in (gmap, fmap))
    graw = [ginv[original] for original in gids]
    fraw = [finv[original] for original in fids]
    kept = words((GRID / "kept_return_ids.u32le").read_bytes())
    mask = (GRID / "mask.u8").read_bytes()
    need(len(mask) == len(gmap) and len(set(graw)) == len(set(fraw)) ==
         len(set(kept)) == n and set(graw) == set(fraw) == set(kept) and
         {raw for raw, state in enumerate(mask) if state != 1} == set(kept),
         "whole-frame ground mask / retained-return join")
    f32_sector = {finv[original] for original in
                  words((F32 / fm["datasets"][SECTOR]["original_site_ids_file"]).read_bytes())}
    f32_sector_blob = (F32 / fm["datasets"][SECTOR]["original_site_ids_file"]).read_bytes()
    need(sha(f32_sector_blob) == fm["datasets"][SECTOR]["original_site_ids_sha256"],
         "float32 physical sector IDs SHA")
    physical = set()
    for i, raw in enumerate(fraw):
        x, y, _ = struct.unpack_from("<fff", fpoints, 12*i)
        if x >= 0 and y < 0:
            physical.add(raw)
    need(physical == f32_sector and len(physical) == 14828, "physical float32 sector")
    pm = json.loads((PHYSICAL / "MANIFEST.json").read_text())
    need(pm["source_grid_manifest_sha256"] == GRID_MANIFEST_SHA and
         pm["source_float32_manifest_sha256"] == F32_MANIFEST_SHA and
         file_sha(GRID / "raw_to_original.u32le") == pm["source_grid_raw_to_original_sha256"] and
         file_sha(F32 / "raw_to_original.u32le") == pm["source_float32_raw_to_original_sha256"],
         "physical reference sources")
    entry = pm["cases"][SECTOR]
    full_points = b"".join(gpoints[12*i:12*i+12] for i, raw in enumerate(graw)
                           if raw in physical)
    need(sha(full_points) == entry["points_sha256"] and entry["sites"] == 14828,
         "physical full input")
    return gpoints, gids, graw, physical, full_points


def select(gpoints, gids, graw, physical, seed):
    n = len(gids)
    ranked = sorted(range(n), key=lambda i: (mix(gids[i] ^ seed), gids[i]))
    selected = {"quarter": set(ranked[:n//4]), "half": set(ranked[:n//2])}
    need(selected["quarter"] < selected["half"], "global nesting")
    result = {}
    for density, indices in selected.items():
        chosen = [i for i, raw in enumerate(graw) if i in indices and raw in physical]
        points = b"".join(gpoints[12*i:12*i+12] for i in chosen)
        original = b"".join(struct.pack("<I", gids[i]) for i in chosen)
        raw = b"".join(struct.pack("<I", graw[i]) for i in chosen)
        result[density] = {"points": points, "original": original, "raw": raw}
    need(set(words(result["quarter"]["raw"])) < set(words(result["half"]["raw"])) <
         physical, "physical nesting")
    return result


def materialize(out):
    gpoints, gids, graw, physical, full_points = source()
    out.mkdir(parents=True, exist_ok=True)
    full_path = out / "physical_full.points.u32le"
    if full_path.exists():
        need(full_path.read_bytes() == full_points, "physical full regeneration differs")
    else:
        full_path.write_bytes(full_points)
    historical = select(gpoints, gids, graw, physical, HISTORICAL_SEED)
    old = json.loads((HISTORICAL / "MANIFEST.json").read_text())
    for density in ("quarter", "half"):
        entry = old["datasets"][density][SECTOR]
        data = historical[density]
        need(sha(data["points"]) == entry["points_sha256"] and
             sha(data["original"]) == entry["original_site_ids_sha256"] and
             len(data["points"]) == 12 * entry["sites"],
             f"historical physical input equals gridded-sector input at {density}")
    manifest = {"schema": "mhgp9_ground_physical_hot_quarter_multiseed_inputs_v1",
                "source_grid_manifest_sha256": GRID_MANIFEST_SHA,
                "source_float32_manifest_sha256": F32_MANIFEST_SHA,
                "source_mask_sha256": file_sha(GRID / "mask.u8"),
                "source_kept_return_ids_sha256": file_sha(GRID / "kept_return_ids.u32le"),
                "physical_full_points_sha256": sha(full_points),
                "physical_full_sites": 14828,
                "sector": SECTOR,
                "selection": "global splitmix64(grid-original-site-ID XOR seed) over all 45845 mask-retained sites; first floor(n/4) and floor(n/2), then intersect float32 sensor signs; emit original full-grid u18 coordinates in original order",
                "historical_seed_control": f"{HISTORICAL_SEED:016x}",
                "historical_inputs": {density: {
                    "sites": len(data["points"]) // 12,
                    "points_sha256": sha(data["points"]),
                    "original_sha256": sha(data["original"]),
                    "raw_sha256": sha(data["raw"])}
                    for density, data in historical.items()},
                "cases": {}}
    for label, seed in SEEDS.items():
        for density, data in select(gpoints, gids, graw, physical, seed).items():
            name = f"{label}_{density}"
            for kind, blob in data.items():
                path = out / f"{name}.{kind}.u32le"
                if path.exists():
                    need(path.read_bytes() == blob, f"regenerated {name}/{kind} differs")
                else:
                    path.write_bytes(blob)
            manifest["cases"][name] = {"seed": f"{seed:016x}", "density": density,
                "sites": len(data["points"]) // 12,
                **{kind + "_sha256": sha(blob) for kind, blob in data.items()},
                "input_fnv64": fnv_points(data["points"])}
    path = HERE / "MANIFEST.json"
    blob = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    if path.exists():
        need(path.read_bytes() == blob, "manifest drift")
    else:
        path.write_bytes(blob)
    return manifest


def validate_probe(probe, entry, grid_label):
    need(probe["schema"] == "mhgp9_tower_probe_v12" and
         probe["status"] == "complete_relative" and
         probe["reason"] == "complete_relative_to_cross_checked_catalogue",
         "probe status")
    need(probe["input"] == {"format": "u32le", "grid": grid_label,
                           "sites": entry["sites"], "hash": entry["input_fnv64"]},
         "probe input")
    o = probe["options"]
    need(o["K"] == o["K_effective"] == 10 and o["s"] == o["workers"] ==
         o["tower_static_threads"] == 8 and o["run_tower"] and
         o["levers"] == {lever: True for lever in LEVERS}, "probe options")
    need([row["K"] for row in probe["orders"]] == list(range(1, 11)), "tower orders")
    ledger = probe["ledger"]
    need(ledger["core_sites"] == ledger["dead_core_form_sites"] +
         2 * ledger["dead_core_loads"] and
         ledger["expanded_pairs"] == probe["generator"]["q34_expanded_pairs"],
         "core/pair counters")
    cat = probe["catalogue"]
    need(cat["balls"] == cat["unique_keys"] == sum(cat["by_qmin"]),
         "catalogue internal counts")
    need(probe["chain_cpu_s"] > 0 and probe["times_ms"]["chain_total"] > 0,
         "positive chain times")


def command(path):
    return ["nice", "-n", "19", str(BINARY), str(path), "10", "8",
            "--s=8", "--static=8", "--grid=1mm"]


def run_one(name, entry, out, timeout):
    input_path = out / f"{name}.points.u32le"
    argv = command(input_path)
    before_binary, before_input = file_sha(BINARY), file_sha(input_path)
    need(before_binary == BINARY_SHA and before_input == entry["points_sha256"],
         "binary / input before")
    old = resource.getrusage(resource.RUSAGE_CHILDREN)
    tick = time.monotonic()
    started = dt.datetime.now(dt.timezone.utc).isoformat()
    timed_out = False
    try:
        p = subprocess.run(argv, capture_output=True, timeout=timeout)
        stdout, stderr, code = p.stdout, p.stderr, p.returncode
    except subprocess.TimeoutExpired as ex:
        stdout, stderr, code, timed_out = ex.stdout or b"", ex.stderr or b"", None, True
    wall = time.monotonic() - tick
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    ended = dt.datetime.now(dt.timezone.utc).isoformat()
    (HERE / f"{name}.stdout").write_bytes(stdout)
    (HERE / f"{name}.stderr").write_bytes(stderr)
    row = {"schema": "mhgp9_ground_hot_quarter_multiseed_case_v1", "name": name,
           "argv": argv, "binary_sha256_before": before_binary,
           "binary_sha256_after": file_sha(BINARY),
           "input_sha256_before": before_input,
           "input_sha256_after": file_sha(input_path),
           "started_utc": started, "ended_utc": ended,
           "external_wall_s": wall,
           "external_cpu_user_s": usage.ru_utime-old.ru_utime,
           "external_cpu_system_s": usage.ru_stime-old.ru_stime,
           "stdout_sha256": sha(stdout), "stderr_sha256": sha(stderr),
           "exit_code": code, "timed_out": timed_out}
    with (HERE / "CASES.jsonl").open("a") as stream:
        stream.write(json.dumps(row, sort_keys=True) + "\n")
    validate_row(row, entry, out)


def validate_row(row, entry, out):
    need(row["schema"] == "mhgp9_ground_hot_quarter_multiseed_case_v1" and
         row["name"] in ("s1_quarter", "s1_half", "s2_quarter", "s2_half"),
         "case identity")
    name = row["name"]
    expected = command(out / f"{name}.points.u32le")
    archived = row["argv"]
    need(len(archived) == len(expected) and archived[:4] == expected[:4] and
         archived[5:] == expected[5:] and Path(archived[4]).name ==
         Path(expected[4]).name, "case command")
    need(row["binary_sha256_before"] == row["binary_sha256_after"] == BINARY_SHA,
         "binary mutated")
    need(row["input_sha256_before"] == row["input_sha256_after"] ==
         entry["points_sha256"] == file_sha(out / f"{name}.points.u32le"),
         "input mutated")
    stdout, stderr = [(HERE / f"{name}.{suffix}").read_bytes()
                      for suffix in ("stdout", "stderr")]
    need(sha(stdout) == row["stdout_sha256"] and
         sha(stderr) == row["stderr_sha256"] and
         row["exit_code"] == 0 and not row["timed_out"], "case output")
    probe = json.loads(stdout)
    validate_probe(probe, {"sites": entry["sites"],
                           "input_fnv64": entry["input_fnv64"]}, "1mm")
    return probe


def slopes(probes):
    n = [p["input"]["sites"] for p in probes]
    need(n[0] < n[1] < n[2] == 14828, "nested sites")
    result = {"sites": n}
    for key, vals in {
            "core_sites": [p["ledger"]["core_sites"] for p in probes],
            "expanded_pairs": [p["ledger"]["expanded_pairs"] for p in probes],
            "chain_cpu_s": [p["chain_cpu_s"] for p in probes],
            "dead_core_form_sites": [p["ledger"]["dead_core_form_sites"] for p in probes],
            "catalogue_balls": [p["catalogue"]["balls"] for p in probes]}.items():
        need(all(v > 0 for v in vals), f"positive {key}")
        result[key] = vals
        result["p_" + key] = [math.log(vals[i+1]/vals[i]) /
                             math.log(n[i+1]/n[i]) for i in (0, 1)]
    return result


def gather(manifest, out):
    old_rows = [json.loads(line) for line in (HERE / "CASES.jsonl").read_text().splitlines() if line]
    need(len(old_rows) == 4 and {row["name"] for row in old_rows} == set(manifest["cases"]),
         "complete case matrix")
    probes = {row["name"]: validate_row(row, manifest["cases"][row["name"]], out)
              for row in old_rows}
    need(file_sha(HISTORICAL / "SUMMARY.json") == HISTORICAL_SUMMARY_SHA,
         "historical summary SHA")
    need(file_sha(PHYSICAL / "MANIFEST.json") == PHYSICAL_MANIFEST_SHA and
         file_sha(PHYSICAL / "RESULTS.json") == PHYSICAL_RESULTS_SHA,
         "physical receipt SHA")
    old = json.loads((HISTORICAL / "SUMMARY.json").read_text())["cases"]["10"][SECTOR]
    pm = json.loads((PHYSICAL / "MANIFEST.json").read_text())["cases"][SECTOR]
    full_stdout = PHYSICAL / f"{SECTOR}.stdout"
    need(file_sha(full_stdout) == PHYSICAL_STDOUT_SHA, "physical full stdout SHA")
    original_full = json.loads(full_stdout.read_text())
    full_input = (out / "physical_full.points.u32le").read_bytes()
    need(sha(full_input) == pm["points_sha256"] and len(full_input) == 14828*12,
         "physical full regeneration")
    validate_probe(original_full, {"sites": 14828,
                                   "input_fnv64": fnv_points(full_input)}, "unspecified")
    pr = json.loads((PHYSICAL / "RESULTS.json").read_text())["cases"][SECTOR]["physical"]
    need(pr["core_sites"] == original_full["ledger"]["core_sites"] and
         pr["expanded_pairs"] == original_full["ledger"]["expanded_pairs"] and
         pr["chain_cpu_s"] == original_full["chain_cpu_s"] and
         pr["tower_digest"] == original_full["tower_digest"],
         "physical reference counters and digest")
    historical_input = []
    for density in ("quarter", "half"):
        data = old[density]
        need(data["status"] == "complete_relative" and data["n"] ==
             json.loads((HISTORICAL / "MANIFEST.json").read_text())["datasets"][density][SECTOR]["sites"] and
             data["input_sha256"] == manifest["historical_inputs"][density]["points_sha256"],
             "historical status and sites")
        historical_input.append({"input": {"sites": data["n"]},
            "ledger": {"core_sites": data["core_sites"],
                       "expanded_pairs": data["expanded_pairs"],
                       "dead_core_form_sites": data["dead_core_form_sites"]},
            "catalogue": {"balls": data["catalogue_balls"]},
            "chain_cpu_s": data["chain_cpu_s"]})
    series = {"historical": slopes([*historical_input, original_full])}
    for label in SEEDS:
        series[label] = slopes([probes[f"{label}_quarter"],
                                probes[f"{label}_half"], original_full])
    summary = {"schema": "mhgp9_ground_hot_quarter_multiseed_summary_v1",
               "sector": SECTOR, "K": 10, "full_physical_sites": 14828,
               "historical_gridded_full_sites": 14829,
               "series": series,
               "note": "historical reduced inputs equal physical reduced inputs; all series share physical full input"}
    path = HERE / "SUMMARY.json"
    blob = (json.dumps(summary, indent=2, sort_keys=True) + "\n").encode()
    if path.exists():
        need(path.read_bytes() == blob, "summary drift")
    else:
        path.write_bytes(blob)
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("run", "verify"))
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=900)
    args = parser.parse_args()
    manifest = materialize(args.out)
    if args.mode == "run":
        need(file_sha(BINARY) == BINARY_SHA, "binary SHA")
        rows = HERE / "CASES.jsonl"
        done = {json.loads(line)["name"] for line in rows.read_text().splitlines() if line} if rows.exists() else set()
        for label in SEEDS:
            for density in ("quarter", "half"):
                name = f"{label}_{density}"
                if name not in done:
                    run_one(name, manifest["cases"][name], args.out, args.timeout)
    summary = gather(manifest, args.out)
    for seed, series in summary["series"].items():
        print(seed, "sites", series["sites"], "p_core", series["p_core_sites"],
              "p_pairs", series["p_expanded_pairs"], "p_cpu", series["p_chain_cpu_s"])


if __name__ == "__main__":
    main()
