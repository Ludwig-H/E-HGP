#!/usr/bin/env python3
"""Pre-core edge-length dispatch diagnostic over the pinned full S2 survivor trace."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "paired_guards_precore_20260923"
PROVENANCE = json.loads((OLD / "PROVENANCE.json").read_text())
POWERS = (20, 21, 22, 23, 24, 25, 26, 27)


def check(ok, message):
    if not ok:
        raise RuntimeError(message)


def read_checked(path, digest):
    data = path.read_bytes()
    check(sha256(data).hexdigest() == digest, f"input SHA mismatch: {path}")
    return data


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--inputs", type=Path, required=True)
    p.add_argument("--trace", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    check(not args.output.exists(), "refuse to overwrite receipt")
    expected = PROVENANCE["input_sha256"]
    xyz = list(struct.iter_unpack("<III", read_checked(args.inputs / "s00_full_full.u32le", expected["points"])))
    ids = [x[0] for x in struct.iter_unpack("<I", read_checked(
        args.inputs / "s00_full_full.raw_return_ids.u32le", expected["raw_ids"]))]
    check(len(xyz) == len(ids) == 123389, "cloud length")
    where = {rid: xyz[i] for i, rid in enumerate(ids)}
    check(len(where) == len(ids), "unique raw IDs")
    counts = {str(power): {"all": 0, "heavy_F_ge_1000": 0, "heavy_F_sum": 0}
              for power in POWERS}
    total = {"all": 0, "heavy_F_ge_1000": 0, "heavy_F_sum": 0}
    files = sorted(args.trace.glob("part_*.bin"))
    check(len(files) == 8, "eight pinned trace parts required")
    for part, path in enumerate(files):
        data = read_checked(path, expected["trace_parts"][part])
        check(len(data) % 16 == 0, "truncated trace")
        for a, b, F, mask in struct.iter_unpack("<IIII", data):
            check(a < b and F >= 2 and mask in (2, 4, 6), "S2 record")
            pa, pb = where[a], where[b]
            D = sum((pa[i] - pb[i]) ** 2 for i in range(3))
            total["all"] += 1
            heavy = F >= 1000
            if heavy:
                total["heavy_F_ge_1000"] += 1
                total["heavy_F_sum"] += F
            for power in POWERS:
                if D >= (1 << power):
                    count = counts[str(power)]
                    count["all"] += 1
                    if heavy:
                        count["heavy_F_ge_1000"] += 1
                        count["heavy_F_sum"] += F
    sample = {str(power): {"edges": 0, "closable_B16": 0, "F_closable_B16": 0}
              for power in POWERS}
    for name in ("RESULT.json", "RESULT_SEED2.json"):
        for row in json.loads((OLD / name).read_text())["rows"]:
            pa, pb = where[row["a"]], where[row["b"]]
            D = sum((pa[i] - pb[i]) ** 2 for i in range(3))
            closed = int(row["results"]["16"]["closed"])
            for power in POWERS:
                if D >= (1 << power):
                    s = sample[str(power)]
                    s["edges"] += 1
                    s["closable_B16"] += closed
                    s["F_closable_B16"] += row["F"] * closed
    payload = {"schema": "mhgp9_audit_precore_length_dispatch_v1",
               "context": "Full pinned S2-survivor trace; D=|b-a|^2 uses only endpoints before core. F is diagnostic, obtained after core.",
               "trace_parts_sha256": expected["trace_parts"],
               "all_S2_survivors": total,
               "threshold_D_ge_2_power": counts,
               "stratified_120_sample": sample}
    args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"total": total, "thresholds": counts, "sample": sample}, sort_keys=True))


if __name__ == "__main__":
    main()
