#!/usr/bin/env python3
"""Read the archived full-frame q3/q4 rectangle histograms and v12 ledgers."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
INPUT = ROOT / "morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/full.u32le"
INPUT_SHA = "233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172"
HASHES = {
    "measure.cpp": "a3b7eb8bb50e28dad97a18cf03641453f2ed4e3a04c932a7362783ccdfb871dc",
    "k5.stdout": "dfa0d26f6c4ec3afde77ec7e9087c7f1e89a62419b6c8fde523d8dc941e708d6",
    "k10.stdout": "f37deddabfcddd16720feae59e38ccede0114860a6b0fb1e11dd93f8864d888b",
}
EXPECTED = {
    5: {16: (134765, 16534272), 64: (42020, 13763765)},
    10: {16: (198169, 28256693), 64: (54822, 24052640),
         1024: (2337, 14030989), 16384: (225, 8810601)},
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def key_values(parts: list[str]) -> dict[str, int]:
    require(len(parts) % 2 == 0, f"odd key/value count: {parts}")
    return {key: int(value) for key, value in zip(parts[::2], parts[1::2])}


def read_histogram(k: int) -> tuple[dict[str, int], dict[int, dict[str, int]]]:
    lines = (HERE / f"k{k}.stdout").read_text().splitlines()
    require(len(lines) > 1, f"empty K{k} histogram")
    first = lines[0].split()
    require(first[:2] == ["sites", "123389"], f"wrong K{k} input")
    # time_s is environment-dependent and has no standing as a performance result.
    i = first.index("time_s")
    float(first[i + 1])
    header = key_values(first[:i] + first[i + 2:])
    require(header["K"] == k, f"wrong K in K{k} file")
    buckets: dict[int, dict[str, int]] = {}
    for line in lines[1:]:
        parts = line.split()
        require(parts[0] == "bucket" and parts[1].startswith("2^"), f"bad bucket: {line}")
        exponent = int(parts[1][2:])
        require(exponent not in buckets, f"duplicate bucket {exponent}")
        bucket = key_values(parts[2:])
        for prefix in ("front", "open"):
            count, mass = bucket[f"{prefix}_rect"], bucket[f"{prefix}_mass"]
            require(count * (1 << exponent) <= mass <= count * ((2 << exponent) - 1),
                    f"K{k} bucket {exponent} mass bound")
        require(bucket["open_rect"] <= bucket["front_rect"], f"K{k} bucket {exponent} count")
        require(bucket["open_mass"] <= bucket["front_mass"], f"K{k} bucket {exponent} mass")
        require(bucket["open_rect"] <= bucket["open_q3"] + bucket["open_q4"] <= 2 * bucket["open_rect"],
                f"K{k} bucket {exponent} lane union")
        buckets[exponent] = bucket
    require(set(buckets) == set(range(max(buckets) + 1)), f"K{k} missing bucket")
    for name in ("front_rect", "front_mass", "open_rect", "open_mass"):
        require(sum(b[name] for b in buckets.values()) == header[name], f"K{k} {name} sum")
    require(header["front_rect"] == header["open_rect"] + header["closed_rect"], f"K{k} rect partition")
    require(header["front_mass"] == header["open_mass"] + header["closed_mass"], f"K{k} mass partition")
    require(header["front_rect"] == header["front_reported"], f"K{k} front callback count")
    for threshold, expected in EXPECTED[k].items():
        got = tuple(sum(b[name] for exponent, b in buckets.items() if (1 << exponent) >= threshold)
                    for name in ("open_rect", "open_mass"))
        require(got == expected, f"K{k} threshold {threshold}: {got} != {expected}")
    return header, buckets


def full_ledger(k: int) -> dict:
    folder = "lidar_raw_physical_scaling_20260923" if k == 5 else "lidar_raw_k10_density_20260923"
    path = HERE.parent / folder / "CASES.jsonl"
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    matches = [row for row in rows if row["sites"] == 123389 and row["input_sha256"] == INPUT_SHA
               and row.get("sector", row.get("level")) == "full" and
               row.get("density", "full") == "full"]
    require(len(matches) == 1, f"K{k} full-frame ledger cardinality")
    row = matches[0]
    require(row["probe"]["options"]["K"] == k, f"K{k} ledger K")
    require(row["probe"]["options"]["s"] == 8, f"K{k} ledger s")
    require(row["probe"]["status"] == "complete_relative", f"K{k} ledger status")
    return row["probe"]


def main() -> None:
    for name, expected in HASHES.items():
        require(digest(HERE / name) == expected, f"{name} SHA-256")
    require(digest(INPUT) == INPUT_SHA, "input SHA-256")
    for k in (5, 10):
        header, _ = read_histogram(k)
        ledger = full_ledger(k)
        work = ledger["ledger"]
        expected = {
            "front_rect": work["q34_input_rectangles"],
            "front_mass": work["witness_input_pair_mass"],
            "open_mass": work["expanded_pairs"],
            "closed_rect": work["witness_rejected_rectangles"],
            "rect_visits": work["witness_rect_node_visits"],
        }
        for name, value in expected.items():
            require(header[name] == value, f"K{k} published {name}: {header[name]} != {value}")
        require(header["open_mass"] == ledger["generator"]["q34_expanded_pairs"],
                f"K{k} generator pair mass")
        print(f"K{k}: {header['front_rect']} rectangles; {header['open_rect']} ouverts; "
              f"{header['open_mass']} paires développables; ledgers v12 concordants")


if __name__ == "__main__":
    main()
