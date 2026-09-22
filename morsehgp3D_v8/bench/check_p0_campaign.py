#!/usr/bin/env python3
"""Check P0 receipts and summarize component work, never certify FULL."""

from __future__ import annotations

import argparse
import base64
import hashlib
import itertools
import json
import statistics
from collections import defaultdict
from pathlib import Path

# Largeur de coordonnée du moteur entier (18 bits depuis le 22 septembre 2026) : au plus 18 coupes
# au milieu par axe, donc 54 niveaux d'index et 55 cadres de pile ; bornes prouvées, jamais des quotas.
COORDINATE_BITS = 18
MAX_INDEX_DEPTH = 3 * COORDINATE_BITS
INDEX_STACK_FRAMES = MAX_INDEX_DEPTH + 1


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    keys = ("family", "kmax", "separation_s", "lane", "n", "strategy")
    rows = []
    signatures = {}
    campaigns = sorted(args.receipt.glob("*/MANIFEST.json"))
    require(bool(campaigns), "no campaign manifests")
    for path in campaigns:
        manifest = json.loads(path.read_text())
        completion = json.loads((path.parent / "COMPLETION.json").read_text())
        require(manifest.get("receipt_validation_version") == 2,
                "this reader requires the corrected v2 campaign receipt")
        require(completion["status"] == "completed" and completion["source_hashes_unchanged"],
                f"incomplete/invalid campaign: {path}")
        require(completion["probe_hash_unchanged"] is True and
                completion["probe_sha256_closing"] == manifest["probe_sha256"] and
                completion["source_sha256_closing"] == manifest["source_sha256"],
                "closing hashes differ from the campaign's source/binary pins")
        require(manifest["gcp_used"] is False and manifest["threads"] == 1 and
                manifest["public_status"] == "not_claimed", f"scope: {path}")
        for name, digest in manifest["source_sha256"].items():
            require(hashlib.sha256((root / name).read_bytes()).hexdigest() == digest,
                    f"source has changed since this campaign: {name}")
        expected = set(itertools.product(
            manifest["families"], manifest["kmax"], manifest["separations"],
            manifest["lanes"], manifest["sizes"], manifest["strategies"],
            range(manifest["repeats"])))
        observed = set()
        for line in (path.parent / "MEASURES.jsonl").read_text().splitlines():
            record = json.loads(line)
            require(record["exit_code"] == 0 and not record["stderr"], "failed measurement")
            require(record["status"] == "completed" and
                    record["probe_sha256_before"] == manifest["probe_sha256"] and
                    record["probe_sha256_after"] == manifest["probe_sha256"],
                    "invalid row or changed binary")
            row = record["result"]
            raw = base64.b64decode(record["stdout_base64"], validate=True).decode("utf-8")
            require(raw == record["stdout"] and json.loads(raw) == row and
                    record["stderr_base64"] == "", "raw/parsed receipt mismatch")
            key = tuple(row[name] for name in keys)
            require(record["command"] == [manifest["probe"], str(row["n"]), row["strategy"],
                                          str(row["lane"]), row["family"], str(row["kmax"]),
                                          str(row["separation_s"])],
                    "command/result tuple mismatch")
            identity = (*key, record["repeat"])
            require(identity not in observed, "duplicated measurement")
            observed.add(identity)
            require(row["status"] == "completed" and row["public_status"] == "not_claimed" and
                    row["scope"] == "single_separated_rectangle_credits", "row scope")
            require(row["threads"] == 1 and row["candidates_expanded"] is False and
                    row["downstream_measured"] is False, "work scope")
            require(row["n_a"] + row["n_b"] == row["n"] and
                    row["total_pairs"] == row["n_a"] * row["n_b"], "cardinal input")
            require(0 <= row["candidate_pairs"] <= row["total_pairs"] and
                    row["rejected_pairs"] + row["candidate_pairs"] == row["total_pairs"],
                    "cardinal output")
            require(row["threshold"] == max(0, row["kmax"] + 2 - row["lane"]) and
                    row["core_credit"] == 0, "threshold/core")
            require(row["candidate_descriptors"] <= row["threshold"] * (row["threshold"] + 1) // 2,
                    "descriptor count")
            times = [row[name] for name in ("generation_ms", "prepare_ms", "plan_ms")]
            require(all(value >= 0 for value in times) and
                    abs(sum(times) - row["total_component_ms"]) < 0.000001, "timing partition")
            work = row["plan_work"]
            require(work["tube_sweep_tests"] <= 2 * work["tube_records"], "linear tube sweep")
            require(work["max_tree_depth"] <= MAX_INDEX_DEPTH and work["max_task_depth"] <= 2 * MAX_INDEX_DEPTH,
                    "18-bit depth representation bound")
            invariant = (row["input_fnv1a64_le_u16_xyz"], row["candidate_pairs"],
                         row["candidate_descriptors"], row["preparation_work"], row["plan_work"])
            # s changes only a passed precondition here, never the rectangle.
            stable_key = tuple(row[name] for name in keys if name != "separation_s")
            if stable_key in signatures:
                require(signatures[stable_key] == invariant, "nondeterministic work or residual")
            signatures[stable_key] = invariant
            # No-core sheets defeat local universal credits only on active
            # lanes. A zero threshold (e.g. q3/Kmax=1) emits no candidates.
            if row["family"] == "sheet" and row["threshold"] > 0:
                require(row["candidate_pairs"] == row["total_pairs"], "sheet counter-fixture changed")
            if row["family"] == "rails" and row["lane"] == 4 and row["kmax"] == 10:
                require(row["candidate_pairs"] == (1846881 if row["strategy"] == "pool" else 2916),
                        "rails counter-fixture changed")
            rows.append(row)
        require(observed == expected and len(observed) == completion["runs"] == completion["attempts"],
                "incomplete matrix")
    grouped = defaultdict(list)
    for row in rows:
        grouped[tuple(row[name] for name in keys)].append(row)
    summary = []
    for key, group in sorted(grouped.items()):
        item = dict(zip(keys, key))
        item.update(repeats=len(group), candidate_pairs=group[0]["candidate_pairs"],
                    total_pairs=group[0]["total_pairs"], plan_work=group[0]["plan_work"])
        for name in ("generation_ms", "prepare_ms", "plan_ms", "total_component_ms"):
            item[f"median_{name}"] = statistics.median(row[name] for row in group)
        summary.append(item)
        if key[-1] == "dual":
            for alternative in ("pool", "tubes"):
                other = grouped.get((*key[:-1], alternative))
                if other:
                    require(group[0]["candidate_pairs"] <= other[0]["candidate_pairs"],
                            "exact box credits worse than a minorant")
    result = {"status": "passed", "campaigns": len(campaigns), "measurements": len(rows),
              "distinct_configurations": len(grouped), "scope": "p0_component_receipts",
              "full_contract_qualified": False, "gcp_used": False}
    if args.summary:
        result["summary"] = summary
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
