#!/usr/bin/env python3
"""Read-only post-capture arithmetic; run LIVE receipt readers separately."""
import json
from pathlib import Path
import statistics

V9 = Path(__file__).resolve().parents[2]


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def main():
    rows = []
    for name in ("ng00", "uniform_8000", "uniform_16000", "uniform_32000"):
        old = json.loads((V9 / "receipts/full_first_real_drafts_20260927/r1" / name / "measure.stdout").read_text())
        new = json.loads((V9 / "receipts/full_parallel_real_drafts_20260927/r1" / name / "measure.stdout").read_text())
        need(new["schema"] == "mhgp9_full_parallel_real_drafts_v1" and new["status"] == "passed", "measurement schema")
        for key in ("n", "input_hash_u64", "k", "s", "seed", "family", "mode", "workers", "static_threads",
                    "tower_digest", "catalogue_digest", "presentation_digest", "bank_capacity_bytes",
                    "draft_capacity_bytes_sum", "output_capacity_bytes_sum"):
            need(old[key] == new[key], "same input and native chain: " + key)
        need([x["k"] for x in old["rows"]] == [x["k"] for x in new["rows"]] == list(range(1, 6)), "exact order inventory")
        per_k = []
        for before, current in zip(old["rows"], new["rows"]):
            for key in ("k", "flat_source", "batches", "actions", "parents", "contributions", "nodes", "continuations",
                        "draft_capacity_bytes", "output_capacity_bytes"):
                need(before[key] == current[key], "same draft counters: " + key)
            medians = {field: statistics.median(current[field]) for field in ("native_ms", "parallel1_ms", "parallel4_ms")}
            per_k.append(dict(k=current["k"], **medians))
        rows.append(dict(case=name, n=new["n"], per_k_median_ms=per_k,
                         sum_of_medians_ms={f: sum(r[f] for r in per_k) for f in ("native_ms", "parallel1_ms", "parallel4_ms")},
                         actions=sum(r["actions"] for r in new["rows"]), parents=sum(r["parents"] for r in new["rows"]),
                         contributions=sum(r["contributions"] for r in new["rows"]),
                         principal_scratch_bytes_sum=sum(r["prototype_workspace_requested_bytes"] for r in new["rows"]),
                         w4_thread_starts=sum(r["w4_thread_starts"] for r in new["rows"]),
                         chain_instrumented_ms=new["chain_instrumented_ms"], capture_copy_ms_sum=new["capture_copy_ms_sum"],
                         peak_rss_kib=new["peak_rss_kib"]))
    print(json.dumps(dict(status="passed", scope="paired_local_encoder_observations_not_FULL_wall",
                         input_digests_and_draft_sizes_unchanged=True, rows=rows), sort_keys=True))


if __name__ == "__main__":
    main()
