#!/usr/bin/env python3
"""Read the compact receipt, or replay the pinned full S2 trace byte for byte."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
PANEL = HERE.parent / "paired_guards_precore_20260923"
PROVENANCE = json.loads((HERE / "PROVENANCE.json").read_text())


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def checked_bytes(path, digest):
    data = path.read_bytes()
    require(sha256(data).hexdigest() == digest, f"SHA mismatch: {path}")
    return data


def panel_tsv():
    out = []
    seen = set()
    for name, digest in PROVENANCE["panel_sha256"].items():
        document = json.loads(checked_bytes(PANEL / name, digest))
        require(len(document["rows"]) == 60, "panel size")
        for row in document["rows"]:
            a, b, core = row["a"], row["b"], row["F"]
            require(a < b and core >= 1000 and (a, b) not in seen, "panel edge")
            seen.add((a, b))
            closed = int(row["results"]["16"]["closed"])
            out.append(f"{a} {b} {core} {closed}\n")
    require(len(seen) == 120, "two disjoint panel seeds")
    return "".join(out)


def parse_receipt(data):
    rows = {}
    groups = {}
    samples = {}
    total = None
    grids = {}
    for line in data.decode("ascii").splitlines():
        fields = line.split()
        kind = fields[0]
        if kind == "H":
            continue
        values = tuple(map(int, fields[1:]))
        if kind == "T":
            require(total is None and len(values) == 4, "total row")
            total = values
        elif kind == "M":
            require(len(values) == 3 and values[0] not in grids, "grid row")
            grids[values[0]] = values[1:]
        elif kind == "S":
            require(len(values) == 10, "sample row")
            key = values[:2]
            require(key not in samples, "duplicate sample row")
            samples[key] = values[2:]
        elif kind == "R":
            require(len(values) == 9, "routing row")
            key = values[:3]
            require(key not in rows, "duplicate routing row")
            rows[key] = values[3:]
        elif kind == "G":
            require(len(values) == 20, "group row")
            key = values[:3]
            require(key not in groups, "duplicate group row")
            groups[key] = values[3:]
        else:
            raise RuntimeError(f"unknown row: {kind}")
    require(total == (3986433, 91267, 429563593, 559661741), "S2 totals")
    require(grids == {10: (6923, 708), 11: (2698, 2386),
                      12: (969, 6016), 13: (273, 14091),
                      14: (73, 26888)}, "grid sizes")
    require(len(samples) == 120 and len(rows) == 275, "row cardinality")
    panel = {}
    for line in panel_tsv().splitlines():
        a, b, F, closed = map(int, line.split())
        panel[a, b] = (F, closed)
    for key, value in samples.items():
        require(key in panel and value[:2] == panel[key], "sample F/closure")
        require(value[2] > 0 and all(x >= 0 for x in value[3:]), "sample descriptor")
    for key, value in rows.items():
        n, heavy, heavy_f, sample_n, closed, closed_f = value
        require(0 <= heavy <= n <= total[0], "routing count")
        require(0 <= heavy_f <= total[2] and
                0 <= closed <= sample_n <= 120 and
                0 <= closed_f <= 280728, "routing outcomes")
    for (power, bits, threshold), value in rows.items():
        if threshold > 1:
            smaller = rows[power, bits, threshold // 2]
            require(all(x <= y for x, y in zip(value, smaller)),
                    "nonmonotone occupancy threshold")
        if power > 20:
            smaller = rows[power - 1, bits, threshold]
            require(all(x <= y for x, y in zip(value, smaller)),
                    "nonmonotone edge length threshold")
    for power in (23, 24):
        selected = [v for k, v in groups.items() if k[0] == power]
        require(selected, "missing groups")
        totals = tuple(sum(v[i] for v in selected) for i in range(3))
        require(totals == rows[power, 12, 1024][:3], "group partition")
        for v in selected:
            require(0 < v[3] <= v[4], "group D range")
            for j in range(3):
                require(v[5 + j] <= v[8 + j] and
                        v[11 + j] <= v[14 + j], "group endpoint boxes")
    return total, grids, rows, groups, samples


def summary(receipt):
    total, grids, rows, groups, _ = receipt
    picks = [(22, 12, 1), (23, 12, 1), (23, 12, 256),
             (23, 12, 1024), (24, 12, 1024)]
    selected = {f"D2^{d}_cell2^{bits}_count{count}": {
        "edges": rows[d, bits, count][0],
        "heavy_edges": rows[d, bits, count][1],
        "heavy_core_sites": rows[d, bits, count][2],
        "panel_edges": rows[d, bits, count][3],
        "panel_closable": rows[d, bits, count][4],
        "panel_closable_core_sites": rows[d, bits, count][5],
    } for d, bits, count in picks}
    dominant = {}
    cell = (19 << 36) | (18 << 18) | 6
    for power in (23, 24):
        g = groups[power, cell, 0]
        dominant[str(power)] = {
            "edges": g[0], "heavy_edges": g[1],
            "heavy_core_sites": g[2], "D_min": g[3], "D_max": g[4],
            "left_low": g[5:8], "left_high": g[8:11],
            "right_low": g[11:14], "right_high": g[14:17],
        }
    return {
        "schema": "mhgp9_audit_precore_grid_dispatch_summary_v1",
        "total": dict(zip(("S2_survivors", "heavy_F_ge_1000",
                           "heavy_F_sum", "all_F_sum"), total)),
        "grid_cells_and_max_population": {str(k): v for k, v in grids.items()},
        "selected_thresholds": selected,
        "group_counts": {str(power): {
            "cell_axis_groups": sum(k[0] == power for k in groups),
            "midpoint_cells": len({k[1] for k in groups if k[0] == power}),
        } for power in (23, 24)},
        "dominant_group_cell_19_18_6_axis_x": dominant,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--points", type=Path)
    parser.add_argument("--ids", type=Path)
    parser.add_argument("--trace", type=Path)
    args = parser.parse_args()
    data = (HERE / "RESULT.tsv").read_bytes()
    expected = summary(parse_receipt(data))
    require(json.loads(json.dumps(expected)) ==
            json.loads((HERE / "SUMMARY.json").read_text()),
            "summary differs from receipt")
    live = [args.points, args.ids, args.trace]
    require(all(x is None for x in live) or all(x is not None for x in live),
            "provide all three LIVE paths")
    if args.points is not None:
        checked_bytes(args.points, PROVENANCE["points_sha256"])
        checked_bytes(args.ids, PROVENANCE["raw_ids_sha256"])
        for part, digest in enumerate(PROVENANCE["trace_parts_sha256"]):
            checked_bytes(args.trace / f"part_{part}.bin", digest)
        with tempfile.TemporaryDirectory(prefix="mhgp9-grid-dispatch-") as tmp:
            tmp = Path(tmp)
            (tmp / "panel.tsv").write_text(panel_tsv())
            binary = tmp / "measure"
            subprocess.run(["g++", "-std=c++20", "-O3", "-Wall", "-Wextra",
                            "-Werror", str(HERE / "measure.cpp"), "-o",
                            str(binary)], check=True)
            output = subprocess.run([str(binary), str(args.points), str(args.ids),
                                     str(args.trace), str(tmp / "panel.tsv")],
                                    check=True, capture_output=True).stdout
            require(output == data, "LIVE routing output differs")
    print(json.dumps({"status": "PASS", "mode": "LIVE" if args.points else
                      "static", "rows": 480, "selected":
                      expected["selected_thresholds"]}, sort_keys=True))


if __name__ == "__main__":
    main()
