#!/usr/bin/env python3
"""Rejudge historical G4 evidence and expose disjoint q34 intervals."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
V9 = Path(__file__).resolve().parents[2]
PRIOR = V9 / "audits/b_critical_path_20260927/analyze.py"
PRIOR_PIN = "4aaf560f3ebfb517f2e673b1035be42e7580e781def9d62be2ea3de536e2174e"


def need(ok, why):
    if not ok:
        raise ValueError(why)


def analyze(snapshot):
    need(hashlib.sha256(PRIOR.read_bytes()).hexdigest() == PRIOR_PIN,
         "historical authority changed")
    spec = importlib.util.spec_from_file_location("q34_prior_ledger", PRIOR)
    prior = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prior)
    # Also checks the exact source pins and source manifest of the G4 run.
    authority = prior.analyze(snapshot)
    need(authority["status"] == "passed", "closed capture failed")
    names = ("front_ms", "filter_ms", "certificate_ms", "edges_ms",
             "lanes_wait_ms", "tail_ms")
    rows = []
    for checked in authority["rows"]:
        index = checked["index"]
        path = V9 / f"receipts/g4_core_warm_20260927/vm/probe_{index}.stdout"
        raw = json.loads(path.read_text())
        q34 = raw["q34_batch"]
        exposed = {key: q34[key] for key in names}
        total = raw["times_ms"]["q34"]
        remainder = total - sum(exposed.values())
        need(all(value >= 0 for value in exposed.values()) and remainder >= -0.01,
             "disjoint q34 phase nesting")
        need(q34["deferred"] == 0 and q34["lanes_deferred"] == 0,
             "this ledger discusses the no-deferred capture")
        rows.append(dict(
            probe=index, raw_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
            q34_ms=total, disjoint_timed_intervals_ms=exposed,
            sum_disjoint_ms=sum(exposed.values()), unassigned_ms=remainder,
            outside_s2_ms=total-q34["filter_ms"],
            overlapped_lanes_wall_ms_not_additive=q34["lanes_ms"],
            overlapped_gpu_prepare_ms_not_additive=q34["gpu_prepare_ms"],
            R=q34["rectangles"], S=q34["survivors"],
            lane_records=q34["lanes_records"],
            lanes_tasks=q34["lanes_tasks"],
            lanes_max_task_steps=q34["lanes_max_task_steps"]))
    return dict(status="passed", GCP_calls=False,
                scope="historical_first_pass_q34_ledger_not_new_benchmark",
                source_commit=authority["source_commit"],
                prior_reader_sha256=PRIOR_PIN, rows=rows)


if __name__ == "__main__":
    need(len(sys.argv) == 2, "usage: analyze.py <closed G4 snapshot.tar.gz>")
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2, sort_keys=True))
