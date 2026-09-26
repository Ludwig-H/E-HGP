#!/usr/bin/env python3
"""Recompute the bounded consumer comparison, effective also under -O."""
import hashlib
import json
from pathlib import Path
import statistics


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def require(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def main():
    capture = HERE / "results"
    manifest = json.loads((capture / "MANIFEST.json").read_text())
    result = json.loads((capture / "RESULTS.json").read_text())
    require(manifest["status"] == "completed" and not manifest["gcp_used"], "capture_scope")
    require(manifest["source_sha256"] == manifest["source_after_sha256"], "closing_hashes")
    for name, expected in manifest["source_sha256"].items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, "source_changed:" + name)
    for name, expected in manifest["binaries_sha256"].items():
        require(hashlib.sha256(Path(name).read_bytes()).hexdigest() == expected, "binary_changed:" + name)
    require(len(manifest["commands"]) == 11, "command_count")
    for command in manifest["commands"]:
        require(command["returncode"] == command["expected"], "command_failed")
    a = json.loads((capture / "selftest_release.stdout").read_text())
    b = json.loads((capture / "selftest_sanitize.stdout").read_text())
    require(a == b == result["selftest"], "selftest_mismatch")
    require(a["status"] == "pass" and a["differential"] == 276 and a["refusals"] == 34 and
            a["extra_shell_fallbacks"] == 1 and a["coordinated_count_forgery_survives_local"] == 3,
            "selftest_coverage")
    raw = []
    summaries = []
    previous = None
    for n in (8000, 16000, 32000):
        rows = [json.loads(line) for line in (capture / ("bench_" + str(n) + ".stdout")).read_text().splitlines()]
        raw.extend(rows)
        require(len(rows) == 5 and {r["repeat"] for r in rows} == set(range(5)), "repetitions")
        require(len({r["digest"] for r in rows}) == 1, "digest_repetitions")
        for row in rows:
            require(row["sites"] == n and row["packets"] == n // 6 and row["workers"] == 1,
                    "workload")
            require(row["import_tree_nodes"] == 0 and row["recensus_nodes"] > 0 and
                    row["recensus_leaf_tests"] == (n // 6) * 6, "work_accounting")
            require(row["import_ms"] > 0 and row["recensus_ms"] > 0 and row["packet_producer_ms_excluded"] > 0,
                    "positive_costs")
        summary = {key: statistics.median(row[key] for row in rows)
                   for key in ("sites", "packets", "owner_ms", "packet_producer_ms_excluded", "import_ms",
                               "recensus_ms", "recensus_nodes", "recensus_leaf_tests")}
        summary["consumer_speedup"] = summary["recensus_ms"] / summary["import_ms"]
        if previous is not None:
            summary["doubling_import_time"] = summary["import_ms"] / previous["import_ms"]
            summary["doubling_recensus_nodes"] = summary["recensus_nodes"] / previous["recensus_nodes"]
        summaries.append(summary)
        previous = summary
    require(raw == result["measurements"], "summary_does_not_match_stdout")
    print(json.dumps({"status": "pass", "authority": "live_local_capture_consumer_only",
                      "selftest": a, "measurements": summaries}, indent=2))


if __name__ == "__main__":
    main()
