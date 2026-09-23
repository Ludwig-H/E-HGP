#!/usr/bin/env python3
"""Paired K5/K10 hot-quarter density audit with two additional global seeds."""

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
AUDITS = HERE.parent
ROOT = HERE.parents[2]
ORIGINAL = AUDITS / "lidar_raw_physical_scaling_20260923"
K10 = AUDITS / "lidar_raw_k10_sectors_20260923"
PAIRS = AUDITS / "paired_guards_precore_20260923"
PARENT_SUMS_SHA = {
    ORIGINAL: "b40751faebc8f86869c3424b6a57b4375cb5a6c3769d1efeb21c3a354ebfb679",
    K10: "f0f850b75ae125058796f2ecb0e29363552ec94402d63cfa40b38d056324d455",
}
PROBE = ROOT / "build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe"
BINARY_SHA = "e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80"
BASE_MANIFEST_SHA = "6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095"
ORIGINAL_SEED = 0xD1DA73A520260923
SEEDS = {"s1": 0x7D1C9A5EB3F24680, "s2": 0x2B85D41E0C93A76F}
SECTOR = "quarter_x_nonneg_y_neg"
MODES = ("quarter", "half")
LEVERS = ("atlas_saturate_deep", "q3_leaf_census", "q34_dead_lanes",
          "q34_witness_cache", "q34_dead_core", "tower_meb_proposal")
MASK64 = (1 << 64) - 1


def need(test, message):
    if not test:
        raise RuntimeError(message)


def digest(blob):
    return hashlib.sha256(blob).hexdigest()


def file_sha(path):
    return digest(path.read_bytes())


def pinned_bytes(directory, name):
    need(file_sha(directory / "SHA256SUMS") == PARENT_SUMS_SHA[directory],
         "historical SHA256SUMS changed")
    hashes = dict(line.split("  ", 1)[::-1] for line in
                  (directory / "SHA256SUMS").read_text().splitlines())
    blob = (directory / name).read_bytes()
    need(name in hashes and digest(blob) == hashes[name], f"pinned SHA: {name}")
    return blob


def u32s(blob):
    need(len(blob) % 4 == 0, "u32 alignment")
    return [row[0] for row in struct.iter_unpack("<I", blob)]


def fnv_points(blob):
    need(len(blob) % 12 == 0, "point alignment")
    h = 14695981039346656037
    for byte in (len(blob) // 12).to_bytes(8, "little"):
        h = ((h ^ byte) * 1099511628211) & MASK64
    for (value,) in struct.iter_unpack("<I", blob):
        need(value < (1 << 18), "point outside u18")
        for byte in value.to_bytes(8, "little"):
            h = ((h ^ byte) * 1099511628211) & MASK64
    return f"{h:016x}"


def splitmix64(value):
    value = (value + 0x9E3779B97F4A7C15) & MASK64
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK64
    return value ^ (value >> 31)


def source(base):
    need(file_sha(base / "MANIFEST.json") == BASE_MANIFEST_SHA, "base manifest changed")
    proof = json.loads((PAIRS / "PROVENANCE.json").read_text())
    xyz = (base / "s00_full_full.u32le").read_bytes()
    ids_blob = (base / "s00_full_full.raw_return_ids.u32le").read_bytes()
    sector_blob = (base / f"s00_full_{SECTOR}.raw_return_ids.u32le").read_bytes()
    need(digest(xyz) == proof["input_sha256"]["points"], "base points hash")
    need(digest(ids_blob) == proof["input_sha256"]["raw_ids"], "base raw IDs hash")
    need(digest(sector_blob) == proof["sector_raw_ids_sha256"][SECTOR], "physical sector hash")
    ids, sector = u32s(ids_blob), set(u32s(sector_blob))
    need(len(ids) == 123389 and len(set(ids)) == len(ids) and
         len(xyz) == 12 * len(ids) and sector <= set(ids), "base identity")
    return xyz, ids, sector


def thinning(ids, seed):
    ranked = sorted(ids, key=lambda rid: (splitmix64(rid ^ seed), rid))
    return {"quarter": set(ranked[:len(ids) // 4]),
            "half": set(ranked[:len(ids) // 2])}


def materialize(xyz, ids, sector, selected):
    point = bytearray()
    raw = bytearray()
    for i, rid in enumerate(ids):
        if rid in sector and rid in selected:
            point.extend(xyz[12*i:12*(i+1)])
            raw.extend(struct.pack("<I", rid))
    return bytes(point), bytes(raw)


def baseline_check(xyz, ids, sector):
    m = json.loads((ORIGINAL / "MANIFEST.json").read_text())
    selected = thinning(ids, ORIGINAL_SEED)
    for mode in MODES:
        p, raw = materialize(xyz, ids, sector, selected[mode])
        expected = m["datasets"][mode][SECTOR]
        need(len(p) == 12 * expected["sites"] and
             digest(p) == expected["points_sha256"] and
             digest(raw) == expected["raw_return_ids_sha256"],
             f"original-seed control failed: {mode}")


def cases_for(base, out):
    xyz, ids, sector = source(base)
    baseline_check(xyz, ids, sector)
    out.mkdir(parents=True, exist_ok=True)
    cases = {}
    for label, seed in SEEDS.items():
        selected = thinning(ids, seed)
        need(selected["quarter"] < selected["half"], "non-nested global thinning")
        for mode in MODES:
            p, raw = materialize(xyz, ids, sector, selected[mode])
            name = f"{label}_{mode}"
            points_file = out / f"{name}.u32le"
            raw_file = out / f"{name}.raw_ids.u32le"
            points_file.write_bytes(p)
            raw_file.write_bytes(raw)
            cases[name] = {"seed": f"{seed:016x}", "density": mode,
                           "sites": len(p) // 12, "points_sha256": digest(p),
                           "raw_ids_sha256": digest(raw), "input_fnv": fnv_points(p)}
    for label in SEEDS:
        a = set(u32s((out / f"{label}_quarter.raw_ids.u32le").read_bytes()))
        b = set(u32s((out / f"{label}_half.raw_ids.u32le").read_bytes()))
        need(a < b < sector, "non-nested physical sector")
    manifest = {"schema": "mhgp9_hot_quarter_multiseed_inputs_v1",
                "source_manifest_sha256": BASE_MANIFEST_SHA,
                "source_physical_sector": SECTOR,
                "selection": "global splitmix64(raw_return_id XOR seed), first floor(n/4), first floor(n/2), then physical-sector intersection; original whole-scene u18/1mm coordinates",
                "baseline_seed_checked": f"{ORIGINAL_SEED:016x}", "cases": cases}
    target = HERE / "MANIFEST.json"
    blob = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    if target.exists():
        need(target.read_bytes() == blob, "existing audit manifest differs")
    else:
        target.write_bytes(blob)
    return manifest


def validate_probe(probe, meta, k):
    need(probe["schema"] == "mhgp9_tower_probe_v12", "probe schema")
    need(probe["status"] == "complete_relative" and
         probe["reason"] == "complete_relative_to_cross_checked_catalogue", "probe status")
    need(probe["input"] == {"format": "u32le", "grid": "1mm",
                            "sites": meta["sites"], "hash": meta["input_fnv"]}, "probe input")
    opt = probe["options"]
    need(opt["K"] == opt["K_effective"] == k and opt["s"] ==
         opt["workers"] == opt["tower_static_threads"] == 8 and opt["run_tower"],
         "probe options")
    need(opt["levers"] == {x: True for x in LEVERS}, "probe levers")
    need([row["K"] for row in probe["orders"]] == list(range(1, k+1)), "tower orders")
    ledger = probe["ledger"]
    need(ledger["core_sites"] == ledger["dead_core_form_sites"] +
         2 * ledger["dead_core_loads"], "actual core forms")
    need(probe["catalogue"]["balls"] == probe["catalogue"]["unique_keys"] ==
         sum(probe["catalogue"]["by_qmin"]), "catalogue count")
    need(probe["chain_cpu_s"] > 0 and probe["times_ms"]["chain_total"] > 0,
         "positive times")


def command(path, k):
    return ["nice", "-n", "19", str(PROBE), str(path), str(k), "8",
            "--s=8", "--static=8", "--grid=1mm"]


def one_run(name, k, meta, out, timeout):
    tag = f"{name}_k{k}"
    input_file = out / f"{name}.u32le"
    argv = command(input_file, k)
    before_binary, before_input = file_sha(PROBE), file_sha(input_file)
    need(before_binary == BINARY_SHA and before_input == meta["points_sha256"],
         "binary/input preflight")
    usage0 = resource.getrusage(resource.RUSAGE_CHILDREN)
    tick = time.monotonic()
    started = dt.datetime.now(dt.timezone.utc).isoformat()
    timed_out = False
    try:
        p = subprocess.run(argv, capture_output=True, timeout=timeout)
        stdout, stderr, code = p.stdout, p.stderr, p.returncode
    except subprocess.TimeoutExpired as ex:
        stdout, stderr, code, timed_out = ex.stdout or b"", ex.stderr or b"", None, True
    wall = time.monotonic() - tick
    usage1 = resource.getrusage(resource.RUSAGE_CHILDREN)
    ended = dt.datetime.now(dt.timezone.utc).isoformat()
    after_binary, after_input = file_sha(PROBE), file_sha(input_file)
    (HERE / f"{tag}.stdout").write_bytes(stdout)
    (HERE / f"{tag}.stderr").write_bytes(stderr)
    row = {"schema": "mhgp9_hot_quarter_multiseed_case_v1", "name": tag,
           "input": name, "K": k, "argv": argv,
           "binary_sha256_before": before_binary, "binary_sha256_after": after_binary,
           "input_sha256_before": before_input, "input_sha256_after": after_input,
           "started_utc": started, "ended_utc": ended,
           "external_wall_s": wall,
           "external_cpu_user_s": usage1.ru_utime-usage0.ru_utime,
           "external_cpu_system_s": usage1.ru_stime-usage0.ru_stime,
           "stdout_sha256": digest(stdout), "stderr_sha256": digest(stderr),
           "exit_code": code, "timed_out": timed_out}
    with (HERE / "CASES.jsonl").open("a") as file:
        file.write(json.dumps(row, sort_keys=True) + "\n")
    validate_row(row, {name: meta}, out)


def validate_row(row, metas, out):
    need(row["schema"] == "mhgp9_hot_quarter_multiseed_case_v1", "case schema")
    name, k = row["input"], row["K"]
    need(name in metas and k in (5, 10) and row["name"] == f"{name}_k{k}", "case identity")
    meta = metas[name]
    path = out / f"{name}.u32le"
    # The archived /tmp path is provenance for the original process, while a
    # LIVE reader may regenerate byte-identical inputs in a different /tmp.
    expected = command(path, k)
    archived = row["argv"]
    need(len(archived) == len(expected) and archived[:4] == expected[:4] and
         archived[5:] == expected[5:] and Path(archived[4]).name == path.name,
         "case command")
    need(row["binary_sha256_before"] == row["binary_sha256_after"] == BINARY_SHA,
         "binary drift")
    need(row["input_sha256_before"] == row["input_sha256_after"] ==
         meta["points_sha256"] == file_sha(path), "input drift")
    stdout = (HERE / f"{row['name']}.stdout").read_bytes()
    stderr = (HERE / f"{row['name']}.stderr").read_bytes()
    need(digest(stdout) == row["stdout_sha256"] and
         digest(stderr) == row["stderr_sha256"], "output hash")
    need(row["exit_code"] == 0 and not row["timed_out"], "probe failed")
    probe = json.loads(stdout)
    validate_probe(probe, meta, k)
    return probe


def gather(manifest, out, base):
    rows = [json.loads(x) for x in (HERE / "CASES.jsonl").read_text().splitlines() if x]
    expected = {f"{name}_k{k}" for name in manifest["cases"] for k in (5, 10)}
    need(len(rows) == len(expected) and {r["name"] for r in rows} == expected,
         "case count/uniqueness")
    probes = {r["name"]: validate_row(r, manifest["cases"], out) for r in rows}
    for name in manifest["cases"]:
        need(probes[f"{name}_k5"]["orders"] ==
             probes[f"{name}_k10"]["orders"][:5], "K5/K10 per-order aggregate counters")
    original_manifest = json.loads((ORIGINAL / "MANIFEST.json").read_text())
    baseline = {5: {}, 10: {}}
    for k, directory in ((5, ORIGINAL), (10, K10)):
        for density in ("quarter", "half", "full"):
            baseline_input = (base / f"s00_{density}_{SECTOR}.u32le").read_bytes()
            entry = original_manifest["datasets"][density][SECTOR]
            need(digest(baseline_input) == entry["points_sha256"] and
                 len(baseline_input) == 12 * entry["sites"],
                 f"historical input: {density}")
            meta = {"sites": entry["sites"], "input_fnv": fnv_points(baseline_input)}
            probe = json.loads(pinned_bytes(directory, f"{density}_{SECTOR}.stdout"))
            validate_probe(probe, meta, k)
            baseline[k][density] = probe
    for density in ("quarter", "half", "full"):
        need(baseline[5][density]["orders"] == baseline[10][density]["orders"][:5],
             f"historical K5/K10 per-order aggregate counters: {density}")
    summary = {"schema": "mhgp9_hot_quarter_multiseed_summary_v1",
               "source": "one physical quarter of raw SemanticKITTI 08/000000, original whole-scene 1mm/u18 grid",
               "K": {}}
    for k in (5, 10):
        full = baseline[k]["full"]
        full_n = full["input"]["sites"]
        full_forms = full["ledger"]["core_sites"]
        per_seed = {}
        for label in ("original", *SEEDS):
            if label == "original":
                a, b = baseline[k]["quarter"], baseline[k]["half"]
            else:
                a = probes[f"{label}_quarter_k{k}"]
                b = probes[f"{label}_half_k{k}"]
            series = (a, b, full)
            ns = [p["input"]["sites"] for p in series]
            forms = [p["ledger"]["core_sites"] for p in series]
            need(ns[0] < ns[1] < ns[2] == full_n and forms[0] > 0 and
                 forms[1] > 0 and forms[2] == full_forms, "series identity")
            def slopes(key):
                values = [p["ledger"][key] if key in p["ledger"] else
                          p["generator"][key] if key in p["generator"] else
                          p[key] for p in series]
                return [math.log(values[i+1]/values[i]) /
                        math.log(ns[i+1]/ns[i]) for i in (0, 1)]
            per_seed[label] = {"sites": ns, "core_sites": forms,
                "p_core_sites": slopes("core_sites"),
                "p_dead_core_form_sites": slopes("dead_core_form_sites"),
                "p_expanded_pairs": slopes("expanded_pairs"),
                "p_chain_cpu_s": slopes("chain_cpu_s"),
                "catalogue_balls": [p["catalogue"]["balls"] for p in series]}
        summary["K"][str(k)] = per_seed
    target = HERE / "SUMMARY.json"
    blob = (json.dumps(summary, indent=2, sort_keys=True) + "\n").encode()
    if target.exists():
        need(target.read_bytes() == blob, "summary drift")
    else:
        target.write_bytes(blob)
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("run", "verify"))
    parser.add_argument("--base", type=Path, required=True)
    parser.add_argument("--inputs-out", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=900)
    args = parser.parse_args()
    manifest = cases_for(args.base, args.inputs_out)
    if args.mode == "run":
        need(file_sha(PROBE) == BINARY_SHA, "probe binary changed")
        old = (HERE / "CASES.jsonl")
        done = {json.loads(x)["name"] for x in old.read_text().splitlines() if x} if old.exists() else set()
        for label in SEEDS:
            for density in MODES:
                name = f"{label}_{density}"
                for k in (5, 10):
                    if f"{name}_k{k}" not in done:
                        one_run(name, k, manifest["cases"][name], args.inputs_out, args.timeout)
    result = gather(manifest, args.inputs_out, args.base)
    for k, rows in result["K"].items():
        for seed, row in rows.items():
            print(f"K{k} {seed}: n={row['sites']} core={row['core_sites']} "
                  f"p_core={row['p_core_sites']} p_cpu={row['p_chain_cpu_s']}")


if __name__ == "__main__":
    main()
