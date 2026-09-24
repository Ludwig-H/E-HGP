#!/usr/bin/env python3
"""Join sparse, exact pre-core shadow decisions to the pinned S2/F trace.

Input TSV: s2_ordinal<TAB>proved_mask. The mask uses q3=2, q4=4; absent
ordinals mean no proof. This computes work *eligible* to skip, never a measured
or certified product speedup.
"""

import csv
import hashlib
import json
import re
import sys
from pathlib import Path


HERE = Path(__file__).resolve().parent
TRACE = HERE / "quarter_1288.trace.tsv"
RESULT = HERE / "RESULT.json"
DECIMAL = re.compile(r"(?:0|[1-9][0-9]*)\Z")


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def number(value, field):
    need(value is not None and DECIMAL.fullmatch(value) is not None, f"invalid {field}")
    return int(value)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def read_decisions(path, survivors):
    decisions = {}
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        need(reader.fieldnames == ["s2_ordinal", "proved_mask"], "decision TSV schema")
        for row in reader:
            need(set(row) == {"s2_ordinal", "proved_mask"}, "decision row width")
            ordinal = number(row["s2_ordinal"], "s2_ordinal")
            mask = number(row["proved_mask"], "proved_mask")
            need(ordinal < survivors, f"decision ordinal outside trace: {ordinal}")
            need(ordinal not in decisions, f"duplicate decision ordinal: {ordinal}")
            need(mask & ~6 == 0, f"decision contains a non-q3/q4 bit: {ordinal}")
            decisions[ordinal] = mask
    return decisions


def join(path):
    receipt = json.loads(RESULT.read_text())
    need(receipt["scope"] == "audit_only_no_ground_quarter_08_000200", "receipt scope")
    need(sha256(TRACE) == receipt["trace_sha256"], "trace SHA differs from RESULT.json")
    decisions = read_decisions(path, receipt["survivors"])
    totals = {"full_closed_edges": 0, "eligible_core_skip_F": 0,
              "q3_proved_edges": 0, "q3_proved_F": 0,
              "q4_proved_edges": 0, "q4_proved_F": 0,
              "union_proved_F": 0, "intersection_proved_F": 0}
    total_f = trace_rows = 0
    with TRACE.open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        need(reader.fieldnames == ["s2_ordinal", "rectangle_ordinal", "local_a", "local_b", "raw_a",
                                    "raw_b", "s2_mask", "F", "post_core_mask", "post_s3_mask"],
             "trace TSV schema")
        for j, row in enumerate(reader):
            need(None not in row, "trace row width")
            need(number(row["s2_ordinal"], "trace s2_ordinal") == j, "noncontiguous trace ordinal")
            before = number(row["s2_mask"], "trace s2_mask")
            f = number(row["F"], "trace F")
            need(before in (2, 4, 6) and 2 <= f <= receipt["sites"], "invalid S2 mask or F")
            proved = decisions.get(j, 0)
            need(proved & ~before == 0, f"decision proves a lane absent at ordinal {j}")
            q3, q4 = bool(proved & 2), bool(proved & 4)
            totals["q3_proved_edges"] += q3
            totals["q4_proved_edges"] += q4
            totals["q3_proved_F"] += f if q3 else 0
            totals["q4_proved_F"] += f if q4 else 0
            totals["union_proved_F"] += f if q3 or q4 else 0
            totals["intersection_proved_F"] += f if q3 and q4 else 0
            if proved == before:
                totals["full_closed_edges"] += 1
                totals["eligible_core_skip_F"] += f
            total_f += f
            trace_rows += 1
    need(trace_rows == receipt["survivors"] and total_f == receipt["sum_F"], "trace count or F ledger")
    need(totals["q3_proved_F"] + totals["q4_proved_F"] ==
         totals["union_proved_F"] + totals["intersection_proved_F"], "lane F inclusion-exclusion")
    return {"scope": receipt["scope"], "status": "eligibility_only_not_measured_savings",
            "decision_rows": len(decisions), "decisions_sha256": sha256(path), **totals}


def main():
    if len(sys.argv) != 2:
        print("usage: join_shadow.py decisions.tsv", file=sys.stderr)
        return 2
    try:
        print(json.dumps(join(Path(sys.argv[1])), sort_keys=True))
    except (OSError, KeyError, ValueError) as exc:
        print(f"join_shadow: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
