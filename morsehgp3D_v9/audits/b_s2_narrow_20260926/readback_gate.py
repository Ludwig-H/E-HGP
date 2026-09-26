#!/usr/bin/env python3
"""Small negative controls for the isolated experiment's LIVE reader."""
import copy
import json
from pathlib import Path
import sys
from run import check


def main() -> None:
    data = json.loads(Path(sys.argv[1]).read_text())
    check(data)
    mutations = []
    bad = copy.deepcopy(data)
    bad["status"] = "failed"
    mutations.append(bad)
    bad = copy.deepcopy(data)
    bad["files"][0]["sha256"] = "0" * 64
    mutations.append(bad)
    bad = copy.deepcopy(data)
    bad["commands"][0]["returncode"] = 1
    mutations.append(bad)
    bad = copy.deepcopy(data)
    bad["commands"][0]["stderr"] = "failure"
    mutations.append(bad)
    for field in ("queries", "visits", "open", "eligible"):
        bad = copy.deepcopy(data)
        row = json.loads(bad["commands"][2]["stdout"])
        row[field] += 1
        bad["commands"][2]["stdout"] = json.dumps(row)
        mutations.append(bad)
    for field, value in (("numeric_checks", 1), ("narrow", 0)):
        bad = copy.deepcopy(data)
        row = json.loads(bad["commands"][0]["stdout"])
        row[field] = value
        bad["commands"][0]["stdout"] = json.dumps(row)
        mutations.append(bad)
    for values in ((0.1,), (-1, 0.1, 0.1)):
        bad = copy.deepcopy(data)
        row = json.loads(bad["commands"][2]["stdout"])
        row["candidate_s"] = values
        bad["commands"][2]["stdout"] = json.dumps(row)
        mutations.append(bad)
    for i, bad in enumerate(mutations):
        try:
            check(bad)
        except ValueError:
            continue
        raise ValueError(f"reader mutation survived: {i}")
    print(json.dumps({"status": "pass", "mutations_rejected": len(mutations)}))


if __name__ == "__main__":
    main()
