"""Native bounded witness for the cohort scratch multiplier; no timing claim."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile


def sha(payload):
    return hashlib.sha256(payload).hexdigest()


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--sites", type=int, default=512)
    args = parser.parse_args()
    n = args.sites
    if not 32 <= n <= 2048:
        raise ValueError("bounded witness: 32 <= sites <= 2048")
    root = Path(__file__).resolve().parent
    xyz = b"".join(struct.pack("<III", i, 0, 0) for i in range(n))
    ids = struct.pack("<" + "I" * n, *range(n))
    rows = []
    with tempfile.TemporaryDirectory(dir=root, prefix="cohort_input_") as tmp:
        temporary = Path(tmp)
        points = temporary / "xyz.u32le"
        names = temporary / "ids.u32le"
        points.write_bytes(xyz)
        names.write_bytes(ids)

        def one(name, workers, budget, dense):
            dump = temporary / (name + ".full")
            mask = 8192 + 8 + (512 if dense else 0)
            command = [str(args.binary), str(points), str(names), str(dump),
                       "2", "8", "32", "0", "4294967295", str(budget),
                       str(workers), str(mask)]
            child = subprocess.run(command, capture_output=True, text=True, timeout=30)
            events = [json.loads(line) for line in child.stdout.splitlines() if line]
            row = dict(name=name, argv=command, workers=workers, budget=budget, dense=dense,
                       exit_code=child.returncode, events=events, stderr=child.stderr,
                       dump_sha256=sha(dump.read_bytes()) if dump.exists() else None)
            rows.append(row)
            full = next((e for e in events if e["phase"] == "full"), None)
            print(name, child.returncode, None if full is None else
                  (full["status"], full["reason"], full["peak_reserved_bytes"]))
            return full

        one_w = one("wide_budget_w1_dense", 1, 1 << 28, True)
        eight_w = one("wide_budget_w8_dense", 8, 1 << 28, True)
        sparse = one("wide_budget_w8_sparse", 8, 1 << 28, False)
        if any(row["exit_code"] != 0 for row in rows):
            raise RuntimeError("baseline refusal")
        if len({row["dump_sha256"] for row in rows}) != 1:
            raise RuntimeError("baseline FULL differs")
        for full in (one_w, eight_w, sparse):
            if full["orders"][1]["births"] != n - 1:
                raise RuntimeError("wrong analytic cohort cardinal")
        small = max(one_w["peak_reserved_bytes"], sparse["peak_reserved_bytes"])
        large = eight_w["peak_reserved_bytes"]
        if large <= small:
            raise RuntimeError("this bounded fixture does not expose the cohort memory peak")
        budget = (small + large) // 2
        # A scalar preflight may ask for more than actual peak. Keep each outcome in the receipt.
        one("tight_budget_w1_dense", 1, budget, True)
        one("tight_budget_w8_dense", 8, budget, True)
        one("tight_budget_w8_sparse", 8, budget, False)
    result = dict(snapshot="33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae",
                  binary_sha256=sha(args.binary.read_bytes()), sites=n, kmax=2,
                  coordinate_formula="(i, 0, 0) for i in range(sites)",
                  xyz_sha256=sha(xyz), ids_sha256=sha(ids),
                  flags=dict(concurrent=8192, batch=8, dense=512),
                  gcp_used=False, timing_claim=False, rows=rows)
    destination = root / "COHORT_MEMORY_RELEASE_U21.json"
    destination.write_text(json.dumps(result, indent=2) + "\n")
    tight = {r["name"]: r for r in rows if r["name"].startswith("tight")}
    if tight["tight_budget_w1_dense"]["exit_code"] != 0:
        raise RuntimeError("W1 did not pass tight budget; inspect receipt")
    if tight["tight_budget_w8_sparse"]["exit_code"] != 0:
        raise RuntimeError("W8 sparse did not pass tight budget; inspect receipt")
    refused = tight["tight_budget_w8_dense"]
    if refused["exit_code"] == 0 or refused["events"][-1]["reason"] != "memory_budget":
        raise RuntimeError("W8 dense did not refuse for memory; inspect receipt")


if __name__ == "__main__":
    run()
