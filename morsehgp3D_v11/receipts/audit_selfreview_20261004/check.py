#!/usr/bin/env python3
"""Closed portable inventory/results only; no product, fit or cloud."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
checks = 0


def need(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)


def manifest(folder):
    records = {}
    for line in (folder / "SHA256SUMS").read_text().splitlines():
        digest, rel = line.split("  ", 1)
        need(len(digest) == 64 and all(c in "0123456789abcdef" for c in digest), "hash syntax")
        need(not Path(rel).is_absolute() and ".." not in Path(rel).parts, "unsafe path")
        need(rel not in records, "duplicate entry")
        records[rel] = digest
    actual = {p.relative_to(folder).as_posix() for p in folder.rglob("*")
              if p.is_file() and p != folder / "SHA256SUMS"}
    need(set(records) == actual, "inventory " + folder.name)
    for rel, digest in records.items():
        need(hashlib.sha256((folder / rel).read_bytes()).hexdigest() == digest, "changed " + rel)
    return records


def main():
    records = manifest(ROOT)
    for name in ("pipeline_guard", "export_width", "flat_verdict", "eom_scope",
                 "subgrid_bounds", "counter_arena", "cell_memo", "export_gate_stdlib"):
        manifest(ROOT / name)
    expected = {"pipeline_guard/check_abandon.py": 45,
                "export_width/check_codec.py": 359,
                "flat_verdict/check_lidar_decision.py": 89,
                "flat_verdict/review_scope.py": 190,
                "eom_scope/check_cohort_witness.py": 504,
                "subgrid_bounds/check.py": 6219,
                "counter_arena/check_contracts.py": 2313,
                "cell_memo/check.py": 30,
                "export_gate_stdlib/check_decoder.py": 297}
    runs = json.loads((ROOT / "REPLAYS.json").read_text())["runs"]
    need(len(runs) == 18, "replay count")
    for script, count in expected.items():
        pair = [r for r in runs if r["script"] == script]
        need(len(pair) == 2 and {r["optimized"] for r in pair} == {False, True}, "mode pair")
        need(all(r["exit_code"] == 0 and not r["stderr"] for r in pair), "replay failed")
        need(pair[0]["stdout_sha256"] == pair[1]["stdout_sha256"], "optimized differs")
        for run in pair:
            rel = run["stdout_file"]
            need(rel in records and records[rel] == run["stdout_sha256"], "result identity")
            need(json.loads((ROOT / rel).read_text())["checks"] == count, "result count")
    g4 = json.loads((ROOT / "G4_QUALIFICATION_REVIEW.json").read_text())
    readers = g4["reader_replays"]
    need(len(readers) == 2 and {r["optimized"] for r in readers} == {False, True}, "G4 reader pair")
    for reader in readers:
        need(reader["exit_code"] == 0 and reader["no_site_packages"], "G4 reader exit")
        need(hashlib.sha256(reader["stdout"].encode()).hexdigest() == reader["stdout_sha256"], "G4 reader stdout")
        need(reader["stdout"].endswith("qualification_p1p2_verdict conforme\n"), "G4 reader verdict")
    need(readers[0]["stdout_sha256"] == readers[1]["stdout_sha256"], "G4 reader mode differs")
    need(g4["executed_commit"].startswith("eb036dbe2"), "G4 source pin")
    review = g4["archive_review"]
    configs = review["matrix"]["configs"]
    need(set(configs) == set(review["matrix"]["requested"]), "G4 configurations present")
    need(review["matrix"]["complete"] and review["matrix"]["conforming"], "G4 matrix verdict")
    traces = review["p1p2_material_traces"]
    need(sum(map(len, traces.values())) == 18, "G4 named trace count")
    for row in traces.values():
        need({t["name"] for t in row} == set(g4["trace_review"]["named_gates"]), "G4 named gate coverage")
        need(all(t["selected"] and t["status"] == "run" and not t["failure"] for t in row), "G4 trace status")
    print(json.dumps({"status": "PASS", "integrity_checks": checks, "payloads": len(records),
                      "bounded_checks": sum(expected.values()), "native_runs": 0,
                      "fits": 0, "gcp_actions": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
