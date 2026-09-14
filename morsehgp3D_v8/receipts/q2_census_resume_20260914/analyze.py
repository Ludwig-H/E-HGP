#!/usr/bin/env python3
"""Recheck closed local evidence; summarize work, never a FULL/G4 speed claim."""
import argparse
import base64
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q2_resume_checks as resume  # noqa: E402
import run_wspd_q2_parallel_matrix as parallel  # noqa: E402


def require(value, message):
    if not value:
        raise RuntimeError(message)


def full_rows(path):
    result = []
    for line in (path / "MEASURES.jsonl").read_text().splitlines():
        record = json.loads(line)
        require(record["status"] == "completed" and record["exit_code"] == 0, "failed full-q2 record")
        row = json.loads(base64.b64decode(record["stdout_base64"], validate=True))
        require(row == record["result"], "full-q2 raw/parsed mismatch")
        result.append(row)
    return result


def main():
    require(len(sys.argv) == 1, "analysis accepts no arguments")
    logs = io.StringIO()
    with redirect_stdout(logs):
        for name in ("qualification_0ir4r0bq", "qualification_g4tpzakt", "matrix_7ys75y08"):
            resume.read(HERE / name)
        require(parallel.check(argparse.Namespace(receipt=HERE / "full_q2_regression", summary=False)) == 0,
                "full q2 strict reader failed")
    tsan = HERE / "tsan_dy2fgg87"
    completed = json.loads((tsan / "completion.json").read_text())
    attempt = json.loads((tsan / "attempt.json").read_text())
    require(completed["status"] == attempt["status"] == "passed" and attempt["exit_code"] == 0 and
            completed["attempt_sha256"] == resume.digest(tsan / "attempt.json") and
            completed["source_sha256_after"] == attempt["source_sha256"] and
            completed["artifact_sha256_after"] == attempt["artifact_sha256"], "TSan closure failed")
    require(all(resume.digest(ROOT / p) == h for p, h in attempt["artifact_sha256"].items()), "TSan artifact changed")
    measures = [json.loads(p.read_text())["row"] for p in sorted((HERE / "matrix_7ys75y08").glob("record_*.json"))]
    current = full_rows(HERE / "full_q2_regression")
    old_root = ROOT / "morsehgp3D_v8/receipts/q2_dynamic_front_20260914/campaigns"
    old = full_rows(old_root / "synthetic8k") + full_rows(old_root / "growth")
    historical_pins = {str((old_root / family / name).relative_to(ROOT)): resume.digest(old_root / family / name)
                       for family in ("synthetic8k", "growth")
                       for name in ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json")}
    key = lambda row: tuple(row[field] for field in parallel.KEYS)
    compared = 0
    for row in current:
        previous = [candidate for candidate in old if key(candidate) == key(row) and candidate["schedule"] == "coarse"]
        require(len(previous) == 1, "historical paired configuration missing or duplicate")
        for field in parallel.DISCRETE_FIELDS:
            require(row[field] == previous[0][field], "existing complete q2 path changed its discrete result")
        for field, value in row["pool_work"].items():
            if field not in ("preparation_ms_sum", "selected_total_ms_sum"):
                require(value == previous[0]["pool_work"][field], "Pool work changed")
        compared += 1
    specifications = {"candidate_pairs": ("candidate_pairs",), "front_products": ("front_work", "product_visits"),
                      "witness_descents": ("front_work", "witness_descent_steps"),
                      "census_visits": ("census_work", "count_node_visits"),
                      "order_structural_splits": ("order_work", "structural_splits"),
                      "supports": ("census_work", "payload_supports")}
    growth = []
    for family in resume.FAMILIES:
        rows = sorted((r for r in current if r["family"] == family), key=lambda r: r["n"])
        require([r["n"] for r in rows] == [8000, 16000, 32000], "missing growth regime")
        metrics = {}
        for name, path in specifications.items():
            values = []
            for row in rows:
                value = row
                for part in path:
                    value = value[part]
                values.append(value)
            ratios = [values[i + 1] / values[i] if values[i] else None for i in (0, 1)]
            metrics[name] = dict(values=values, ratios=ratios,
                                 ratios_at_least_four=[v for v in ratios if v is not None and v >= 4])
        growth.append(dict(family=family, sizes=[8000, 16000, 32000], metrics=metrics,
                           total_ms=[row["timings"]["total_ms"] for row in rows]))
    summary = dict(schema="mhgp8_resume_evidence_summary_v1", status="passed", full_contract_qualified=False,
        release_ctests=62, sanitize_ctests=62, tsan_gate="passed", selected_anchor_measures=len(measures),
        complete_q2_regression_measures=len(current), unchanged_complete_q2_against_tranche14=compared,
        selected_anchor_max_transitions=max(r["resume_work"]["transitions"] for r in measures),
        selected_anchor_max_stack_tasks=max(r["resume_work"]["max_pending_tasks"] for r in measures),
        selected_anchor_stack_capacity_bytes=sorted({r["memory"]["stack_bytes"] for r in measures}),
        selected_anchor_max_payload_capacity_bytes=max(r["memory"]["payload_bytes"] for r in measures),
        selected_anchor_quantum1_max_slice_ms=max(r["timing_ms"]["max_slice"] for r in measures if r["quantum"] == 1),
        selected_anchor_quantum256_max_slice_ms=max(r["timing_ms"]["max_slice"] for r in measures if r["quantum"] == 256),
        complete_q2_growth=growth, timings_policy="concurrent_qualification_load_no_performance_gain_claim",
        historical_data_policy="read_only_discrete_differential_not_inherited_qualification", historical_sha256=historical_pins,
        reader_output=logs.getvalue())
    print(json.dumps(summary, indent=2, sort_keys=True, allow_nan=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
