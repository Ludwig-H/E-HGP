#!/usr/bin/env python3
"""Read-only, bounded independent ledger check for the one-sector S2/S3 trace."""

import csv
import hashlib
import json
import struct
from pathlib import Path


HERE = Path(__file__).resolve().parent
PANEL = HERE.parent / "s4a_cpu_scene02_physical_panel_20260924"
CASE = "quarter_quarter_x_nonneg_y_nonneg"
STEM = HERE / "quarter_1288"
INPUT = PANEL / "inputs" / f"{CASE}.u32le"
RAW = PANEL / "inputs" / f"{CASE}.raw_return_ids.u32le"
BASELINE = PANEL / "runs" / f"attempt_0042_{CASE}_s3.stdout"


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    manifest = json.loads((PANEL / "MANIFEST.json").read_text())
    panel = json.loads((PANEL / "SUMMARY.json").read_text())
    baseline = json.loads(BASELINE.read_text())
    provenance = json.loads((PANEL / "BUILD_PROVENANCE.json").read_text())
    fixture = manifest["cases"][CASE]
    require(sha256(INPUT) == fixture["input_sha256"] == panel["cases"][CASE]["input_sha256"],
            "prepared input SHA differs from panel receipt")
    require(sha256(RAW) == fixture["raw_return_ids_sha256"], "raw-return mapping SHA differs")
    require(panel["binary_sha256"] == provenance["binary_sha256"], "panel engine binary SHA inconsistent")
    require(provenance["source_commit"] == "7ceadffad1de860e30325ae357ba3243f269d48b",
            "panel source snapshot changed")
    require(baseline["options"]["K"] == 10 and baseline["options"]["s"] == 8 and
            baseline["options"]["workers"] == 8 and baseline["q34_batch"]["backend"] == "cpu" and
            baseline["q34_batch"]["certificate_backend"] == "cpu", "baseline options changed")
    raw_bytes = RAW.read_bytes()
    require(len(raw_bytes) == 1288 * 4, "raw mapping length")
    raw_ids = list(struct.unpack("<1288I", raw_bytes))
    require(len(set(raw_ids)) == 1288, "duplicate raw ID")

    rows = []
    total_f = q3_f = q4_f = union_f = intersection_f = closed_f = 0
    q3_count = q4_count = core_closed = post_s3_zero = 0
    seen_pairs = set()
    with (STEM.with_suffix(".trace.tsv")).open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        require(reader.fieldnames == ["s2_ordinal", "rectangle_ordinal", "local_a", "local_b", "raw_a",
                                      "raw_b", "s2_mask", "F", "post_core_mask", "post_s3_mask"],
                "trace schema changed")
        for j, record in enumerate(reader):
            row = {k: int(v) for k, v in record.items()}
            require(row["s2_ordinal"] == j, "noncontiguous source ordinal")
            a, b = row["local_a"], row["local_b"]
            require(0 <= a < 1288 and 0 <= b < 1288 and a != b, "local endpoint outside input")
            require(row["raw_a"] == raw_ids[a] and row["raw_b"] == raw_ids[b], "raw/local ID join mismatch")
            pair = tuple(sorted((row["raw_a"], row["raw_b"])))
            require(pair not in seen_pairs, "duplicate raw edge")
            seen_pairs.add(pair)
            before, core, after, f = (row[x] for x in ("s2_mask", "post_core_mask", "post_s3_mask", "F"))
            require(before in (2, 4, 6) and core & ~before == 0 and after & ~core == 0,
                    "lane mask widened or invalid")
            require(2 <= f <= 1288, "F outside exact-core range")
            p3 = before & 2 and not core & 2
            p4 = before & 4 and not core & 4
            total_f += f
            q3_count += bool(p3)
            q4_count += bool(p4)
            q3_f += f if p3 else 0
            q4_f += f if p4 else 0
            union_f += f if p3 or p4 else 0
            intersection_f += f if p3 and p4 else 0
            core_closed += core == 0
            closed_f += f if core == 0 else 0
            post_s3_zero += after == 0
            rows.append(row)

    prev = rects = 0
    with (STEM.with_suffix(".segments.tsv")).open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        require(reader.fieldnames == ["rectangle_ordinal", "begin", "end", "s2_rectangle_mask"],
                "segment schema changed")
        for i, record in enumerate(reader):
            rect, begin, end, mask = (int(record[x]) for x in
                                      ("rectangle_ordinal", "begin", "end", "s2_rectangle_mask"))
            require(rect == i and begin == prev and begin <= end <= len(rows), "non-prefix segments")
            require(mask in (0, 2, 4, 6), "invalid rectangle mask")
            for row in rows[begin:end]:
                require(row["rectangle_ordinal"] == i and row["s2_mask"] & ~mask == 0,
                        "edge outside its rectangle segment")
            prev, rects = end, i + 1
    ledger, batch = baseline["ledger"], baseline["q34_batch"]
    require(prev == len(rows) == batch["survivors"] == ledger["core_builds"] == ledger["dead_core_loads"],
            "survivor/core count differs from pinned stdout")
    require(rects == batch["rectangles"] == ledger["q34_input_rectangles"], "rectangle count differs")
    require(batch["deferred"] == 0 and baseline["generator"]["q34_expanded_pairs"] == ledger["expanded_pairs"],
            "deferred or expanded-pair ledger differs")
    require(total_f == ledger["core_sites"], "F sum differs from pinned stdout")
    require(q3_count == ledger["dead_core_q3_proved"] and q4_count == ledger["dead_core_q4_proved"] and
            core_closed == ledger["core_closed_edges"] and post_s3_zero >= core_closed,
            "per-lane core decisions differ from pinned stdout")
    require(q3_f + q4_f == union_f + intersection_f, "lane F inclusion-exclusion")
    result = {
        "scope": "audit_only_no_ground_quarter_08_000200",
        "historical_engine_sha256": provenance["binary_sha256"],
        "historical_source_commit": provenance["source_commit"],
        "sites": len(raw_ids), "rectangles": rects, "survivors": len(rows), "expanded_pairs": ledger["expanded_pairs"],
        "sum_F": total_f, "q3_core_proved_edges": q3_count, "q4_core_proved_edges": q4_count,
        "q3_core_proved_F": q3_f, "q4_core_proved_F": q4_f,
        "core_proved_union_F": union_f, "core_proved_intersection_F": intersection_f,
        "core_closed_edges": core_closed, "core_closed_F": closed_f, "post_s3_zero_masks": post_s3_zero,
        "trace_sha256": sha256(STEM.with_suffix(".trace.tsv")),
        "segments_sha256": sha256(STEM.with_suffix(".segments.tsv")),
    }
    receipt = json.loads((HERE / "RESULT.json").read_text())
    require(receipt["historical_panel_engine_sha256"] == result["historical_engine_sha256"] and
            receipt["historical_panel_source_commit"] == result["historical_source_commit"] and
            receipt["sidecar_source_sha256"] == sha256(HERE / "trace.cpp") and
            receipt["shadow_join_source_sha256"] == sha256(HERE / "join_shadow.py"),
            "audit source or historical engine provenance differs from RESULT.json")
    for key, value in result.items():
        if key in ("historical_engine_sha256", "historical_source_commit"):
            continue
        require(receipt[key] == value, f"RESULT.json differs at {key}")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
