#!/usr/bin/env python3
"""LIVE reader of the captured native-family experiment, also valid under -O."""
import hashlib
import json
import math
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    out = HERE / "results"
    m = json.loads((out / "MANIFEST.json").read_text())
    need(m["schema"] == "audit_q4_sorted_events_v1" and m["status"] == "completed", "unclosed capture")
    need(m["gcp_used"] is False and m["scope"] == "one_synthetic_family_not_product_T1", "scope")
    need(m["source_hashes_before"] == m["source_hashes_after"], "sources changed")
    for name, expected in m["source_hashes_before"].items():
        need(sha(ROOT / name) == expected, "source differs: " + name)
    for name, expected in m["binary_hashes"].items():
        need(sha(Path(name)) == expected, "live binary differs: " + name)
    expected_commands = ["gxx", "clangxx", "compile_release", "compile_sanitize", "selftest_release",
                         "selftest_sanitize", "bad_option", "bad_size", "bench_8000", "bench_16000", "bench_32000"]
    need([c["label"] for c in m["commands"]] == expected_commands, "command sequence")
    for c in m["commands"]:
        expected = {"bad_option": 2, "bad_size": 1}.get(c["label"], 0)
        need(c["expected"] == expected and c["returncode"] == expected, "command failure")
        need(math.isfinite(c["wall_seconds"]) and c["wall_seconds"] >= 0, "wall time")
        for stream in ("stdout", "stderr"):
            need(sha(out / (c["label"] + "." + stream)) == c[stream + "_sha256"], "stream changed")
    gate = json.loads((out / "selftest_release.stdout").read_text())
    need(gate == json.loads((out / "selftest_sanitize.stdout").read_text()), "selftests differ")
    need(gate == {"status": "pass", "fixtures": 46, "groups": 3083, "direct_power_tests": 517506,
                  "shallow_queries": 114, "payload_ids": 378, "root_ties": 84, "causal_mutants": 2}, "gate coverage")
    rows = []
    discrete = ("events", "groups", "sort_comparisons", "scan_events", "shallow_queries",
                "payload_ids", "early_point_tests", "early_rejected")
    for n in (8000, 16000, 32000):
        group = [json.loads(line) for line in (out / ("bench_" + str(n) + ".stdout")).read_text().splitlines()]
        need(len(group) == 3 and [r["repeat"] for r in group] == [0, 1, 2], "repeat coverage")
        for r in group:
            need(r["n"] == n and all(type(r[k]) is int and r[k] >= 0 for k in discrete), "counts")
            need(all(r[k] == group[0][k] for k in discrete), "work differs across repeats")
            need(r["scan_events"] == r["events"] == n - 4 and r["groups"] == n - 6, "events")
            need(r["shallow_queries"] == r["payload_ids"] == 0 and r["early_rejected"] == r["groups"], "dense scope")
            need(r["sort_comparisons"] > 0 and r["early_point_tests"] > 0, "vacuous work")
            for k in ("sorted_ms", "payload_ms", "early_stop_ms"):
                need(math.isfinite(r[k]) and r[k] > 0, "timing")
        row = {"n": n, **{k: group[0][k] for k in discrete}}
        row.update({k: statistics.median(r[k] for r in group)
                    for k in ("sorted_ms", "payload_ms", "early_stop_ms")})
        if rows:
            row["sort_work_growth"] = row["sort_comparisons"] / rows[-1]["sort_comparisons"]
            row["sort_time_growth"] = row["sorted_ms"] / rows[-1]["sorted_ms"]
        rows.append(row)
    print(json.dumps({"status": "pass", "scope": m["scope"], "gate": gate, "rows": rows}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
