#!/usr/bin/env python3
"""Exact integer census of nominal q3/q4 disks crossing the cloud bbox.

Consumes the already-qualified, per-edge lazy-prefix trace; it never runs HGP.
The trace is intentionally not vendored, so hashes in RUN.json are mandatory.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


POINT = struct.Struct("<III")
RAW_ID = struct.Struct("<I")
EDGE = struct.Struct("<IIIIIIII")
CHUNK_RECORDS = 65536


def checked_bytes(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise ValueError(f"SHA-256 mismatch: {path}: {actual} != {expected}")
    return data


def disk_relation(a: tuple[int, ...], b: tuple[int, ...], lo: list[int], hi: list[int],
                  multiplier: int) -> str:
    d = [b[i] - a[i] for i in range(3)]
    length2 = sum(x * x for x in d)
    if length2 == 0:
        raise ValueError("zero-length edge in certified unique-site input")
    tangent = False
    for i in range(3):
        q = min(a[i] + b[i] - 2 * lo[i], 2 * hi[i] - a[i] - b[i])
        difference = multiplier * q * q - (length2 - d[i] * d[i])
        if difference < 0:
            return "outside"
        tangent |= difference == 0
    return "tangent" if tangent else "inside"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_json", type=Path)
    parser.add_argument("xyz_u32le", type=Path)
    parser.add_argument("raw_ids_u32le", type=Path)
    args = parser.parse_args()
    run = json.loads(args.run_json.read_text())
    if run.get("schema") != "mhgp9_lazy_prefix_run_v1":
        raise ValueError("unexpected trace schema")
    xyz = checked_bytes(args.xyz_u32le, run["file_sha256"]["input"])
    ids = checked_bytes(args.raw_ids_u32le, run["file_sha256"]["ids"])
    if len(xyz) % POINT.size or len(ids) % RAW_ID.size:
        raise ValueError("truncated site input")
    points = list(POINT.iter_unpack(xyz))
    raw_ids = [item[0] for item in RAW_ID.iter_unpack(ids)]
    if len(points) != len(raw_ids) or len(set(raw_ids)) != len(raw_ids):
        raise ValueError("site/ID count mismatch or duplicate raw ID")
    by_id = dict(zip(raw_ids, points))
    lo = [min(point[i] for point in points) for i in range(3)]
    hi = [max(point[i] for point in points) for i in range(3)]
    modes = {"q3": 2, "q4": 4}
    stats = {name: {"open_edges": 0, "open_core_sites": 0, "outside_edges": 0,
                    "outside_core_sites": 0, "tangent_edges": 0, "tangent_core_sites": 0}
             for name in (*modes, "union")}
    whole_core_possible = {"edges": 0, "core_sites": 0}
    total_edges = 0
    total_core_sites = 0
    for name, expected in sorted(run["trace_part_sha256"].items()):
        path = args.run_json.parent / "trace" / name
        digest = hashlib.sha256()
        with path.open("rb") as source:
            while block := source.read(CHUNK_RECORDS * EDGE.size):
                digest.update(block)
                if len(block) % EDGE.size:
                    raise ValueError(f"truncated edge record: {path}")
                for raw_a, raw_b, core_sites, _, masks, _, _, _ in EDGE.iter_unpack(block):
                    total_edges += 1
                    total_core_sites += core_sites
                    if core_sites < 2 or raw_a == raw_b:
                        raise ValueError("invalid edge/core")
                    mask = masks & 255
                    if mask == 0 or mask & ~6:
                        raise ValueError(f"invalid active lane mask: {mask}")
                    a, b = by_id[raw_a], by_id[raw_b]
                    outside_any = False
                    outside_all = True
                    tangent_any = False
                    for lane, bit in modes.items():
                        if not mask & bit:
                            continue
                        row = stats[lane]
                        row["open_edges"] += 1
                        row["open_core_sites"] += core_sites
                        relation = disk_relation(a, b, lo, hi, 3 if lane == "q3" else 2)
                        if relation == "outside":
                            row["outside_edges"] += 1
                            row["outside_core_sites"] += core_sites
                            outside_any = True
                        elif relation == "tangent":
                            row["tangent_edges"] += 1
                            row["tangent_core_sites"] += core_sites
                            tangent_any = True
                        if relation != "outside":
                            outside_all = False
                    union = stats["union"]
                    union["open_edges"] += 1
                    union["open_core_sites"] += core_sites
                    if outside_any:
                        union["outside_edges"] += 1
                        union["outside_core_sites"] += core_sites
                    elif tangent_any:
                        union["tangent_edges"] += 1
                        union["tangent_core_sites"] += core_sites
                    if outside_all:
                        whole_core_possible["edges"] += 1
                        whole_core_possible["core_sites"] += core_sites
        if digest.hexdigest() != expected:
            raise ValueError(f"SHA-256 mismatch: {path}")
    ledger = run["ledger"]
    if total_edges != ledger["dead_core_loads"] or total_core_sites != ledger["core_sites"]:
        raise ValueError("trace/ledger mismatch")
    if total_core_sites - 2 * total_edges != ledger["dead_core_form_sites"]:
        raise ValueError("trace physical-form ledger mismatch")
    result = {"schema": "mhgp9_s2_bbox_clipping_v2", "source_commit": run["source_commit"],
              "source_run": str(args.run_json), "sites": len(points), "bbox_lo": lo,
              "bbox_hi": hi, "edges": total_edges, "core_sites": total_core_sites,
              "stats": stats, "whole_core_possible": whole_core_possible,
              "input_sha256": run["file_sha256"]["input"],
              "ids_sha256": run["file_sha256"]["ids"],
              "trace_part_sha256": run["trace_part_sha256"]}
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
