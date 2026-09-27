#!/usr/bin/env python3
"""Post-capture comparison; no benchmark, mutation, or performance promotion."""
import json
import math
from pathlib import Path
import statistics

V9 = Path(__file__).resolve().parents[2]
NAMES = ("ng00", "uniform_8000", "uniform_16000", "uniform_32000")


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def main():
    rows = []
    for name in NAMES:
        old = json.loads((V9 / "receipts/full_real_drafts_20260927/r3" / name / "measure.stdout").read_text())
        new = json.loads((V9 / "receipts/full_first_real_drafts_20260927/r1" / name / "measure.stdout").read_text())
        need(old["status"] == new["status"] == "passed", "closed successful measurements")
        need(old["schema"] == "mhgp9_full_real_drafts_v1" and
             new["schema"] == "mhgp9_full_first_real_drafts_v1", "distinct generations")
        need([x["k"] for x in old["rows"]] == [x["k"] for x in new["rows"]] == list(range(1, 6)), "exact row inventory")
        for key in ("n", "input_hash_u64", "k", "s", "seed", "family", "mode", "workers", "static_threads",
                    "tower_digest", "catalogue_digest", "presentation_digest", "bank_capacity_bytes",
                    "draft_capacity_bytes_sum", "output_capacity_bytes_sum"):
            need(old[key] == new[key], "paired structure/input: " + key)
        per_k = []
        for x, y in zip(old["rows"], new["rows"]):
            for key in ("k", "flat_source", "batches", "actions", "parents", "contributions", "nodes", "continuations",
                        "draft_capacity_bytes", "output_capacity_bytes"):
                need(x[key] == y[key], "paired draft: " + key)
            need(y["continuations"] == 0 and y["prototype_workspace_requested_bytes"] == 16 * y["actions"], "fast path and scratch")
            for entry in (x, y):
                for field in ("native_ms", "prototype_ms"):
                    need(len(entry[field]) == 3 and all(math.isfinite(t) and t >= 0 for t in entry[field]), "three finite timings")
            native = statistics.median(y["native_ms"])
            fast = statistics.median(y["prototype_ms"])
            need(native > 0 and math.isfinite(fast), "finite paired timings")
            per_k.append(dict(k=y["k"], native_median_ms=native, first_median_ms=fast,
                              ratio=fast / native, sorted_historical_median_ms=statistics.median(x["prototype_ms"])))
        need(len(per_k) == 5, "all orders")
        rows.append(dict(case=name, n=new["n"], per_k=per_k,
                         native_sum_of_medians_ms=sum(x["native_median_ms"] for x in per_k),
                         first_sum_of_medians_ms=sum(x["first_median_ms"] for x in per_k),
                         scratch_first_bytes_sum=sum(y["prototype_workspace_requested_bytes"] for y in new["rows"]),
                         scratch_sorted_bytes_sum=sum(y["prototype_workspace_requested_bytes"] for y in old["rows"]),
                         chain_instrumented_ms=new["chain_instrumented_ms"], peak_rss_kib=new["peak_rss_kib"]))
    print(json.dumps(dict(status="passed", scope="same_real_drafts_observations_shared_host_not_stable_speedup",
                          all_three_digests_equal=True, rows=rows), sort_keys=True))


if __name__ == "__main__":
    main()
