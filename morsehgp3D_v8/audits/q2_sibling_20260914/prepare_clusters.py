#!/usr/bin/env python3
"""Explicit Python port of the pinned v8 cluster input recipe, not an engine."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import time

BASE = Path(__file__).resolve().parent
REPO = BASE.parents[2]
HEADER = "morsehgp3D_v8/bench/front_fixtures.hpp"
CAPTURE = "morsehgp3D_v8/receipts/wspd_q2_census_20260914/shared_matrix/"
PINS = {
    HEADER: "a2d3ddbbff837de1609a2e1104c8797682cd7a435d4fa417d31388eb1f402b3b",
    CAPTURE + "MANIFEST.json": "63e5566e37e7c5c504e999516f0c34da3f13f18591eccede173149dc5ff06c3d",
    CAPTURE + "MEASURES.jsonl": "0dc30b9a659477c9cac0f7f590b1c1646cf4d2a5e17a7d1803d9fda9548d757d",
}
SIZES = (8000, 16000)
SEED = 3
MASK = (1 << 64) - 1
FNV_OFFSET = 14695981039346656037
SCOPE = "input_recipe_reproduction_only_no_census_or_performance_qualification"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fnv(data: bytes, initial: int = FNV_OFFSET) -> int:
    value = initial
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & MASK
    return value


def generate(n: int, seed: int = SEED) -> tuple[bytes, dict]:
    require(2 <= n <= (1 << 33) and 0 <= seed <= MASK, "invalid recipe size/seed")
    state = seed
    work = {"rng_calls": 0, "proposed_points": 0, "duplicate_rejections": 0, "accepted_points": 0}

    def random_word() -> int:
        nonlocal state
        work["rng_calls"] += 1
        state = (state + 0x9E3779B97F4A7C15) & MASK
        value = ((state ^ (state >> 30)) * 0xBF58476D1CE4E5B9) & MASK
        value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK
        return value ^ (value >> 31)

    seen: set[int] = set()
    data = bytearray()
    input_hash = fnv(struct.pack("<QQ", 1, n))
    while len(seen) < n:
        corner = random_word() & 7
        point = tuple((50000 if corner & (1 << axis) else 20000)
                      + (random_word() & 1023) for axis in range(3))
        work["proposed_points"] += 1
        key = point[0] | (point[1] << 16) | (point[2] << 32)
        if key in seen:
            work["duplicate_rejections"] += 1
            continue
        seen.add(key)
        data.extend(struct.pack("<HHH", *point))
        input_hash = fnv(struct.pack("<QQQ", *point), input_hash)
        work["accepted_points"] += 1
    require(work["accepted_points"] == n and len(data) == 6 * n, "incomplete generation")
    return bytes(data), {"n": n, "seed": seed, "generation_work": work,
                         "constructor_input_hash_encoding1": f"{input_hash:x}"}


def reference() -> tuple[dict, dict]:
    payloads = {path: (REPO / path).read_bytes() for path in PINS}
    require(all(sha(payloads[path]) == expected for path, expected in PINS.items()), "reference pin changed")
    manifest = json.loads(payloads[CAPTURE + "MANIFEST.json"])
    require(manifest["source_sha256"][HEADER] == PINS[HEADER], "constructor header mismatch")
    selected = {n: [] for n in SIZES}
    for line, text in enumerate(payloads[CAPTURE + "MEASURES.jsonl"].splitlines(), 1):
        row = json.loads(text)
        result = row["result"]
        if result["family"] != "clusters" or result["n"] not in selected:
            continue
        require(row["status"] == "completed" and row["exit_code"] == 0
                and json.loads(row["stdout"]) == result, "invalid constructor row")
        require(result["recipe"] == "eight_corner_clusters_splitmix64_v1" and result["seed"] == SEED
                and result["kmax"] == 10 and result["front_mode"] == "samples"
                and result["census_mode"] == "shared", "wrong constructor recipe/configuration")
        selected[result["n"]].append({"line": line, "s": result["s"],
            "input_hash": result["input_hash"], "generation_work": result["generation_work"],
            "digest": result["digest"]})
    require(all(sorted(row["s"] for row in selected[n]) == [8, 10, 12] for n in SIZES),
            "expected three reference configurations per size")
    return selected, {"reference_commit": "f7edd646", "reference_authority": "explicit_header_and_constructor_receipt_sha256",
                      "capture_commit": manifest["commit"], "source_sha256": PINS,
                      "header_text": payloads[HEADER].decode("utf-8")}


def prepared() -> tuple[dict[int, bytes], list[dict], dict]:
    refs, provenance = reference()
    outputs, descriptions = {}, []
    for n in SIZES:
        data, info = generate(n)
        for row in refs[n]:
            require(info["constructor_input_hash_encoding1"] == row["input_hash"], "constructor input hash mismatch")
            require(all(value == row["generation_work"][key] for key, value in info["generation_work"].items()),
                    "constructor generation work mismatch")
        outputs[n] = data
        descriptions.append(dict(info, path=f"inputs/clusters_n{n}.u16le", bytes=len(data), sha256=sha(data),
                                 input_fnv64=f"{fnv(data):x}", constructor_references=refs[n]))
    require(outputs[16000].startswith(outputs[8000]), "cluster inputs lost prefix nesting")
    return outputs, descriptions, provenance


def run(write: bool) -> dict:
    source = Path(__file__).resolve()
    source_sha = sha(source.read_bytes())
    start = time.perf_counter_ns()
    outputs, descriptions, provenance = prepared()
    preparation_ns = time.perf_counter_ns() - start
    receipt_path = BASE / "INPUTS.json"
    if write:
        paths = [BASE / row["path"] for row in descriptions]
        require(not receipt_path.exists() and not any(path.exists() for path in paths), "refusing to overwrite inputs/receipt")
        (BASE / "inputs").mkdir(exist_ok=True)
        for n, path in zip(SIZES, paths):
            with path.open("xb") as stream:
                stream.write(outputs[n])
        receipt = {"schema": "mhgp8_sibling_cluster_inputs_v1", "scope": SCOPE,
            "status": "prepared", "created_utc": datetime.now(timezone.utc).isoformat(),
            "generator": {"path": source.name, "sha256": source_sha},
            "command": [sys.executable, *sys.argv], "cpus": sorted(os.sched_getaffinity(0)),
            "recipe": "eight_corner_clusters_splitmix64_v1", "order": "generator_acceptance_order_original_ids",
            "format": "xyz_u16_little_endian_6_bytes_per_point", "provenance": provenance,
            "constructor_input_hash_encoding1": "FNV1a64 on u64LE words: 1,n,x0,y0,z0,...",
            "input_fnv64": "FNV1a64 on emitted u16LE bytes, without version/count prefix",
            "generation_and_reference_check_ns": preparation_ns,
            "preparation_excluded_from_future_pipeline_timings": True,
            "python_set_comparisons_not_cpp_set_comparisons": True,
            "datasets": descriptions}
        require(sha(source.read_bytes()) == source_sha, "generator changed during preparation")
        require(all(sha((REPO / path).read_bytes()) == h for path, h in PINS.items()), "reference changed during preparation")
        with receipt_path.open("x", encoding="utf-8") as stream:
            json.dump(receipt, stream, indent=2, sort_keys=True)
            stream.write("\n")
    else:
        receipt = json.loads(receipt_path.read_text())
        require(receipt["generator"]["sha256"] == source_sha and receipt["provenance"] == provenance,
                "receipt source provenance mismatch")
        require(receipt["datasets"] == descriptions, "receipt differs from reproduced inputs")
    require(all((BASE / row["path"]).read_bytes() == outputs[row["n"]] for row in descriptions),
            "stored input differs from exact generation")
    return {"status": "passed", "scope": SCOPE, "datasets": len(descriptions),
            "constructor_rows_checked": sum(len(row["constructor_references"]) for row in descriptions),
            "input_hashes_encoding1": [row["constructor_input_hash_encoding1"] for row in descriptions],
            "receipt_sha256": sha(receipt_path.read_bytes()), "generator_sha256": source_sha}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--write", action="store_true")
    group.add_argument("--check", action="store_true")
    args = parser.parse_args()
    print(json.dumps(run(args.write), sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}), file=sys.stderr)
        sys.exit(2)
