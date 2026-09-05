#!/usr/bin/env python3
"""Derive normalization work from sealed lazy successes; never run the engine.

The source premise is the 13c6 FULL implementation and its 3d+1 contract.
These additional necessary identities do not replace the receipt/provenance
judges or certify timings, geometry, completeness, or a future implementation.
"""

from __future__ import annotations

from copy import deepcopy
from fractions import Fraction
import hashlib
import json
from pathlib import Path


AUDIT = Path(__file__).resolve().parent
V7 = AUDIT.parent
BASE = V7 / "receipts/full_gabriel_lazy_mono_20260905"
HEADER = "13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627"
ATTEMPTS = [f"paired/n8000_s{s}_k10_lazy_c1000000" for s in (8, 10, 12)] + [
    "scale16/n16000_s8_k10_lazy_c1000000",
    "scale32/n32000_s8_k10_lazy_c1000000",
]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def derive(row: dict) -> dict:
    require(row["outcome"] == "complete_relative", "success required")
    require(row["alias_policy"] == "lazy_first_c_strict_resolutions_v1",
            "lazy contract required")
    f, d = row["face_visits"], row["connection_catalogue_records"]
    s, a = row["successor_steps"], row["normalized_anchors"]
    noop = row["no_op_connections"]
    require(f == row["minimum_hits"] + row["cache_hits"] + row["terminal_direct"],
            "one resolved caller per strict facet")
    require(row["terminal_direct"] == row["portal_requests"], "closed portals")
    require(0 <= noop <= d and 0 <= a <= f + d, "normalization populations")
    h, remainder = divmod(s - f - d, 3)
    require(remainder == 0 and h >= a, "3d+1 identity")
    post_nonzero = d - noop
    pre_a, pre_h = a - post_nonzero, h - post_nonzero
    require(0 <= pre_a <= f and pre_h >= pre_a, "pre/post split")
    return {
        "k": row["k"], "strict_requests": f, "directs": d,
        "calls": f + d, "positive_depth_calls": a, "depth_sum": h,
        "successor_steps": s, "closure_steps": d + 3 * post_nonzero,
        "prelot_depth_sum": pre_h, "prelot_positive_depth_calls": pre_a,
        "prelot_mean_depth": str(Fraction(pre_h, f)) if f else None,
        "remaining_steps_if_tail_pair_omitted": s - 2 * a,
        "omitted_fraction": str(Fraction(2 * a, s)) if s else None,
    }


def main() -> None:
    pins, results, failed, successful_rows = {}, {}, [], []

    def read(path: Path) -> bytes:
        raw = path.read_bytes()
        pins[str(path.relative_to(V7))] = sha(raw)
        return raw

    for attempt in ATTEMPTS:
        receipt = json.loads(read(BASE / (attempt + ".receipt.json")))
        raw_path = BASE / (attempt + ".raw.txt")
        raw = read(raw_path)
        require(receipt["status"] == "completed" and receipt["error"] is None,
                "closed captured attempt")
        require(receipt["streams"][raw_path.name] ==
                {"bytes": len(raw), "sha256": sha(raw)}, "raw stream binding")
        records = [json.loads(line) for line in raw.splitlines() if line.startswith(b"{")]
        config = [r for r in records if r.get("type") == "configuration"]
        orders = [r for r in records if r.get("type") == "order"]
        terminals = [r for r in records if r.get("type") == "terminal"]
        require(len(config) == 1 and len(terminals) == 1, "one config/terminal")
        require(orders == receipt["orders"] and terminals[0] == receipt["terminal"],
                "raw/receipt equality")
        require(terminals[0]["exit_code"] == receipt["exit_code"], "exit binding")
        before = json.loads(read(BASE / (attempt + ".sources_before.json")))
        after = json.loads(read(BASE / (attempt + ".sources_after.json")))
        require(before == after, "source capture stability")
        require(before["files"]["morsehgp3D_v7/src/forest/full_gabriel.hpp"] == HEADER,
                "3d+1 source premise")
        derived = []
        for row in orders:
            if row["outcome"] == "complete_relative":
                derived.append(derive(row))
                successful_rows.append(row)
            else:
                require(row["k"] == 9 and config[0]["n"] == 32000 and
                        row["reason"] == "full_gabriel_successor_budget",
                        "expected separate refusal")
                failed.append({"attempt": attempt, "k": row["k"],
                               "reason": row["reason"],
                               "successor_steps": row["successor_steps"],
                               "derived_success_identities_applied": False})
        results[attempt] = derived
    require(len(successful_rows) == 48 and len(failed) == 1, "nonvacuous inventory")
    paired = [results[p] for p in ATTEMPTS[:3]]
    require(paired[0] == paired[1] == paired[2], "same normalization work across s")
    # Necessary-identity guards stay active under Python -O. These are data
    # corruptions, not product mutants or replacements for publication seals.
    sample = successful_rows[-1]
    mutations = []
    for name, field, value in [
        ("nonintegral_depth_sum", "successor_steps", sample["successor_steps"] + 1),
        ("too_many_nonroot_calls", "normalized_anchors",
         sample["face_visits"] + sample["connection_catalogue_records"] + 1),
        ("too_many_noops", "no_op_connections", sample["connection_catalogue_records"] + 1),
        ("non_success_input", "outcome", "resource_exhausted"),
    ]:
        changed = deepcopy(sample)
        changed[field] = value
        try:
            derive(changed)
        except ValueError:
            mutations.append(name)
        else:
            raise ValueError("unrefuted data corruption: " + name)
    require(len(mutations) == 4, "mutation floor")
    # No duplicate copy of the 48 rows: retain three useful derived endpoints.
    selection = [ATTEMPTS[0], ATTEMPTS[3], ATTEMPTS[4]]
    output = {
        "schema": "mhgp7-successor-work-diagnostic-v1", "status": "passed",
        "source_premise": HEADER, "public_status": "not_claimed",
        "scope": "normalization work identities on closed lazy orders only",
        "engine_runs": 0, "cpp_delta_qualified": False, "timing_qualified": False,
        "success_orders_checked": 48, "paired_s_work_equal": True,
        "selected_endpoints": {p: results[p][-1] for p in selection},
        "refused_orders_excluded": failed, "data_mutations_refuted": mutations,
        "input_pins": pins, "script_sha256": sha(Path(__file__).read_bytes()),
        "limits": ["not a maximum-depth measurement", "not a K9 completion forecast",
                   "not a whole-tower speedup or geometric qualification"],
    }
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
