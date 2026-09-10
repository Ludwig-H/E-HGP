#!/usr/bin/env python3
"""Read-only verifier for the private residence experiment, also under -O."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent


def need(value, why):
    if not value:
        raise RuntimeError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def probe(variant):
    rows = []
    for line in (BASE / f"logs/{variant}_probe.stdout").read_text().splitlines():
        if not line.startswith("n="):
            continue
        row = dict(part.split("=", 1) for part in line.split())
        rows.append({key: (value if key == "physical_digest" else
                          float(value) if key == "tower_seconds" else int(value))
                     for key, value in row.items()})
    return rows


def check():
    summary = {}
    fields = ("n", "balls", "nodes", "parents", "contributions", "populations", "physical_digest")
    baseline = probe("baseline")
    need([row["n"] for row in baseline] == [200, 400, 800], "baseline cardinalities")
    for variant in ("baseline", "release", "reserve", "both"):
        rows = probe(variant)
        need(len(rows) == 3, "three cases per variant")
        for old, new in zip(baseline, rows):
            need(all(old[field] == new[field] for field in fields), f"physical output drift: {variant}")
            need(new["nodes"] > new["n"] and new["contributions"] > new["n"], "nonvacuity")
        summary[variant] = rows
    phase = probe("phase")
    need(len(phase) == 1, "one phase diagnostic")
    for field in baseline[-1]:
        if field != "tower_seconds":
            need(phase[0][field] == baseline[-1][field], f"phase instrumentation changed {field}")
    for row in summary["both"]:
        old = next(v for v in baseline if v["n"] == row["n"])
        need(row["retained"] < old["retained"] and row["requested_total"] < old["requested_total"], "memory delta absent")
        need(row["requested_peak"] <= old["requested_peak"], "peak regression")
    for gate in ("full_coverage_certificate_gate", "full_ball_tower_gate"):
        a = BASE / f"logs/both_final_{gate}_o2.stdout"
        b = BASE / f"logs/both_final_{gate}_san.stdout"
        need(a.read_bytes() == b.read_bytes(), f"O2/SAN differ: {gate}")
        for mode in ("o2", "san"):
            need(not (BASE / f"logs/both_final_{gate}_{mode}.stderr").read_bytes(), "successful gate diagnostics")
            need(json.loads((BASE / f"logs/both_final_{gate}_{mode}.json").read_text())["exit_code"] == 0, "gate code")
            need(json.loads((BASE / f"logs/both_final_{gate}_{mode}_arg.json").read_text())["exit_code"] == 2, "arg code")
    tower = json.loads((BASE / "logs/both_final_full_ball_tower_gate_o2.stdout").read_text())
    need(tower["clouds"] == 28 and tower["orders"] == 112 and tower["checks"] == 170320, "new grouped gate missing")
    journal = (BASE / "logs/both_final_full_coverage_certificate_gate_o2.stdout").read_text()
    need("checks=823 " in journal and "allocation_rejects=20 " in journal, "allocation fail-closed coverage changed")
    for path in sorted((BASE / "logs").glob("*.json")):
        row = json.loads(path.read_text())
        if path.name == "both_full_coverage_certificate_gate_o2_compile.json":
            need(row["exit_code"] == 1, "setup failure not retained")
            need("boost/multiprecision/cpp_int.hpp: No such file" in path.with_suffix(".stderr").read_text(), "unexpected setup failure")
        else:
            need(row["exit_code"] == row["expected_exit_code"], f"unexpected command failure: {path.name}")
        need(sha(path.with_suffix(".stdout")) == row["stdout_sha256"], f"stdout drift: {path.name}")
        need(sha(path.with_suffix(".stderr")) == row["stderr_sha256"], f"stderr drift: {path.name}")
    frozen = json.loads((BASE / "sources_frozen.json").read_text())
    need(all(sha(BASE / "baseline" / name) == digest for name, digest in frozen.items()), "baseline source drift")
    before = json.loads((BASE / "both_final_sources_before.json").read_text())
    need(all(sha(BASE / "both_final" / name) == digest for name, digest in before.items()), "final qualified source drift")
    if (BASE / "both_final_sources_after.json").exists():
        need(before == json.loads((BASE / "both_final_sources_after.json").read_text()), "before/after source mismatch")
    summary["phase"] = phase
    summary["final_gate"] = tower
    summary["journal_stdout"] = journal.strip()
    return summary


if __name__ == "__main__":
    manifest = json.loads((BASE / "manifest.json").read_text())
    for name, digest in manifest.items():
        relative = Path(name)
        need(not relative.is_absolute() and ".." not in relative.parts, "unsafe manifest path")
        need(sha(BASE / relative) == digest, f"manifest drift: {name}")
    check()
    print(f"PASS residence private packet: {len(manifest)} hashes, 12 physical results, 28-cloud O2/SAN gates")
