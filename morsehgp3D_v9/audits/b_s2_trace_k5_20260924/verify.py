#!/usr/bin/env python3
"""Independent structural reader for the pinned 1,288-site K5 S2/S3 trace."""

import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path


HERE = Path(__file__).resolve().parent
PANEL = HERE.parent / "s4a_cpu_scene02_physical_panel_20260924"
CASE = "quarter_quarter_x_nonneg_y_nonneg"
INPUT = PANEL / "inputs" / f"{CASE}.u32le"
RAW = PANEL / "inputs" / f"{CASE}.raw_return_ids.u32le"
TRACE = HERE / "quarter_1288_k5.trace.tsv"
SEGMENTS = HERE / "quarter_1288_k5.segments.tsv"
STDOUT = HERE / "TRACE.stdout"
PRODUCT = HERE / "LOCAL_PRODUCT.stdout"
RESULT = HERE / "RESULT.json"
EXPECTED = {
    "input_sha256": "33630aea9492d3b059001e10c74ba30bec143dd7a1a0be6b25f8701b2f2a5e8f",
    "raw_return_ids_sha256": "d473e6314cf322c9213998906d3c43f59ee5044b809a8545fe4febe9113e053a",
    "trace_sha256": "7d5823ea50c45ef0dd7a95ddd17bf09cf2819f13ae11355d92fc9100e2e40297",
    "segments_sha256": "fd0fb60a17ee0dceab3ab90f29a9b4b321c64844f6af19d5bbd286feee53e4aa",
    "source_sha256": "012e645d4d5bd6667a72c94c3a45b8a09601a011e7f0b5f7bde855e3f3b8326a",
    "sites": 1288, "K": 5, "s": 8, "rectangles": 37459,
    "expanded_pairs": 46218, "survivors": 27099, "sum_F": 298205,
    "core_closed_edges": 7020, "post_s3_zero_masks": 9447,
    "q3_core_proved_edges": 5734, "q4_core_proved_edges": 6811,
    "q3_core_proved_F": 108979, "q4_core_proved_F": 131727,
    "core_closed_F": 146394,
}


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def decimal(value):
    need(value is not None and value.isascii() and value.isdecimal(), "nondecimal TSV field")
    return int(value)


def read_tsv(path, header):
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        need(reader.fieldnames == header, f"unexpected header: {path.name}")
        for row in reader:
            need(None not in row and None not in row.values(), f"row width: {path.name}")
            yield {key: decimal(value) for key, value in row.items()}


def read_trace(raw_ids):
    header = ["s2_ordinal", "rectangle_ordinal", "local_a", "local_b", "raw_a", "raw_b",
              "s2_mask", "F", "post_core_mask", "post_s3_mask"]
    rows = []
    seen = set()
    totals = {key: 0 for key in ("sum_F", "core_closed_edges", "core_closed_F",
                                    "post_s3_zero_masks", "q3_core_proved_edges",
                                    "q4_core_proved_edges", "q3_core_proved_F", "q4_core_proved_F",
                                    "core_proved_union_F", "core_proved_intersection_F")}
    for ordinal, row in enumerate(read_tsv(TRACE, header)):
        need(row["s2_ordinal"] == ordinal, "noncontiguous S2 ordinal")
        a, b = row["local_a"], row["local_b"]
        need(0 <= a < 1288 and 0 <= b < 1288 and a != b, "invalid local edge")
        need(row["raw_a"] == raw_ids[a] and row["raw_b"] == raw_ids[b], "raw/local ID mismatch")
        pair = tuple(sorted((row["raw_a"], row["raw_b"])))
        need(pair not in seen, "duplicate raw edge")
        seen.add(pair)
        s2, core, s3, f = (row[x] for x in ("s2_mask", "post_core_mask", "post_s3_mask", "F"))
        need(s2 in (2, 4, 6) and core & ~s2 == 0 and s3 & ~core == 0,
             "invalid or widened lane mask")
        need(2 <= f <= 1288, "F outside core range")
        p3, p4 = bool(s2 & 2 and not core & 2), bool(s2 & 4 and not core & 4)
        totals["sum_F"] += f
        totals["core_closed_edges"] += core == 0
        totals["core_closed_F"] += f if core == 0 else 0
        totals["post_s3_zero_masks"] += s3 == 0
        totals["q3_core_proved_edges"] += p3
        totals["q4_core_proved_edges"] += p4
        totals["q3_core_proved_F"] += f if p3 else 0
        totals["q4_core_proved_F"] += f if p4 else 0
        totals["core_proved_union_F"] += f if p3 or p4 else 0
        totals["core_proved_intersection_F"] += f if p3 and p4 else 0
        rows.append(row)
    need(len(rows) == EXPECTED["survivors"], "survivor count")
    need(totals["q3_core_proved_F"] + totals["q4_core_proved_F"] ==
         totals["core_proved_union_F"] + totals["core_proved_intersection_F"],
         "lane F inclusion-exclusion")
    for key in EXPECTED.keys() & totals.keys():
        need(totals[key] == EXPECTED[key], f"pinned K5 trace ledger: {key}")
    return rows, totals


def read_segments(rows):
    header = ["rectangle_ordinal", "begin", "end", "s2_rectangle_mask"]
    previous = 0
    for ordinal, segment in enumerate(read_tsv(SEGMENTS, header)):
        begin, end, mask = segment["begin"], segment["end"], segment["s2_rectangle_mask"]
        need(segment["rectangle_ordinal"] == ordinal and begin == previous and
             begin <= end <= len(rows) and mask in (0, 2, 4, 6), "invalid segment")
        for row in rows[begin:end]:
            need(row["rectangle_ordinal"] == ordinal and row["s2_mask"] & ~mask == 0,
                 "edge outside its segment")
        previous = end
    need(ordinal + 1 == EXPECTED["rectangles"] and previous == len(rows),
         "segments do not partition S2 survivors")


def main(check_local_binary=False):
    for key, path in (("input_sha256", INPUT), ("raw_return_ids_sha256", RAW),
                      ("trace_sha256", TRACE), ("segments_sha256", SEGMENTS),
                      ("source_sha256", HERE / "trace.cpp")):
        need(sha(path) == EXPECTED[key], f"pinned SHA: {key}")
    raw = RAW.read_bytes()
    need(len(raw) == 4 * EXPECTED["sites"] and INPUT.stat().st_size == 12 * EXPECTED["sites"],
         "input length")
    raw_ids = struct.unpack("<1288I", raw)
    need(len(set(raw_ids)) == 1288, "duplicate raw return ID")
    rows, totals = read_trace(raw_ids)
    read_segments(rows)
    stdout = json.loads(STDOUT.read_text())
    need(stdout["scope"] == "audit_only_no_ground_quarter_08_000200_K5", "trace stdout scope")
    for key in ("K", "s", "sites", "rectangles", "expanded_pairs", "survivors", "sum_F",
                "core_closed_edges", "post_s3_zero_masks", "q3_core_proved_edges",
                "q4_core_proved_edges", "q3_core_proved_F", "q4_core_proved_F", "core_closed_F"):
        need(stdout[key] == EXPECTED[key], f"trace stdout: {key}")
    need(stdout["core_builds"] == stdout["dead_core_loads"] == len(rows) and
         stdout["core_sites"] == totals["sum_F"], "trace core work ledger")
    receipt = json.loads(RESULT.read_text())
    need(receipt["schema"] == "mhgp9_b_s2_trace_k5_audit_v1" and
         receipt["scope"] == "audit_only_no_ground_quarter_08_000200_K5" and
         receipt["status"] == "matched_local_cpu_product_same_code_family" and
         receipt["existing_historical_K5_product_receipt"] == "missing" and
         receipt["workers"] == 8,
         "RESULT status/scope")
    for key, value in EXPECTED.items():
        need(receipt[key] == value, f"RESULT: {key}")
    need(receipt["stdout_sha256"] == sha(STDOUT) and
         receipt["verify_source_sha256"] == sha(HERE / "verify.py") and
         receipt["local_product_command_sha256"] == sha(HERE / "LOCAL_PRODUCT.command.txt") and
         receipt["shadow_join_source_sha256"] == sha(HERE / "join_shadow.py"),
         "archived audit source/stdout/command SHA")
    if check_local_binary:
        for path_key, sha_key in (("sidecar_binary_path", "sidecar_binary_sha256"),
                                  ("gen_library_path", "gen_library_sha256"),
                                  ("local_product_binary_path", "local_product_binary_sha256")):
            need(sha(Path(receipt[path_key])) == receipt[sha_key], f"live binary SHA: {path_key}")
    for key, value in totals.items():
        need(receipt[key] == value, f"RESULT trace total: {key}")
    need(PRODUCT.is_file(), "closed receipt requires LOCAL_PRODUCT.stdout")
    product = json.loads(PRODUCT.read_text())
    need(receipt["local_product_stdout_sha256"] == sha(PRODUCT), "local product SHA")
    need(product["status"] == "complete_relative" and
         product["reason"] == "complete_relative_to_cross_checked_catalogue" and
         product["input"]["format"] == "u32le" and
         product["input"]["grid"] == "1mm" and
         product["input"]["hash"] == "9e91c2c06b43a6ff" and
         product["input"]["sites"] == 1288 and product["options"]["K"] == 5 and
         product["options"]["s"] == 8 and product["options"]["workers"] == 8 and
         product["options"]["tower_static_threads"] == 8 and
         product["options"]["run_tower"] is True,
         "local product scope")
    levers = product["options"]["levers"]
    need(all(levers[name] for name in ("atlas_saturate_deep", "q3_leaf_census",
                                           "q34_dead_lanes", "q34_witness_cache",
                                           "q34_dead_core", "tower_meb_proposal",
                                           "q34_jobs_by_mass", "q34_fine_jobs",
                                           "tower_overlap_static", "q2_jobs_by_mass",
                                           "q34_batch_filter", "q34_batch_certificates")) and
         not any(levers[name] for name in ("q34_gpu_filter", "q34_gpu_certificates",
                                               "q34_batch_q3", "q34_gpu_q3", "q34_batch_q4")),
         "local product S3 CPU levers")
    need(product["q34_batch"]["backend"] == "cpu" and
         product["q34_batch"]["certificate_backend"] == "cpu" and
         product["q34_batch"]["deferred"] == 0, "local CPU certificate backend")
    ledger = product["ledger"]
    need(product["q34_batch"]["rectangles"] == EXPECTED["rectangles"] and
         product["q34_batch"]["survivors"] == EXPECTED["survivors"] and
         ledger["expanded_pairs"] == EXPECTED["expanded_pairs"] and
         ledger["core_sites"] == EXPECTED["sum_F"] and
         ledger["core_builds"] == ledger["dead_core_loads"] == EXPECTED["survivors"] and
         ledger["core_closed_edges"] == EXPECTED["core_closed_edges"] and
         ledger["dead_core_q3_proved"] == EXPECTED["q3_core_proved_edges"] and
         ledger["dead_core_q4_proved"] == EXPECTED["q4_core_proved_edges"] and
         product["generator"]["q34_cover_builds"] ==
         EXPECTED["survivors"] - EXPECTED["core_closed_edges"],
         "local product S2/S3 ledger differs")
    print(json.dumps({"scope": stdout["scope"], "K": 5, "sites": 1288,
                      "rectangles": EXPECTED["rectangles"], "expanded_pairs": EXPECTED["expanded_pairs"],
                      "survivors": len(rows), "sum_F": totals["sum_F"],
                      "core_closed_edges": totals["core_closed_edges"],
                      "historical_product_crosscheck": "missing",
                      "local_product_crosscheck": "matched_local_product_same_code_family",
                      "local_binary_check": "matched_live" if check_local_binary else "not_requested"},
                     sort_keys=True))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check-local-binary", action="store_true")
    args = parser.parse_args()
    main(args.check_local_binary)
