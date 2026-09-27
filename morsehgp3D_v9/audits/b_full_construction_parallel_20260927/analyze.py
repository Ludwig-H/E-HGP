#!/usr/bin/env python3
"""Read closed historical G4 receipts; no benchmark, build, or cloud call."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
PRIOR = V9 / "audits/b_critical_path_20260927/analyze.py"
PRIOR_PIN = "4aaf560f3ebfb517f2e673b1035be42e7580e781def9d62be2ea3de536e2174e"
SOURCE_PINS = {
    "src/tower/forest/full_ball_tower.hpp": "124d3e9b52b1e6155e5a2ffd2a32cda7e5b403dda21155da2949b9ca8c90c0d0",
    "src/tower/forest/full_coverage_certificate.hpp": "8259a3cb8110a9dd8333eaf48455def03bc886014d869f5fcf499898615a7a62",
}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def analyze(snapshot):
    need(hashlib.sha256(PRIOR.read_bytes()).hexdigest() == PRIOR_PIN,
         "historical scope reader changed")
    for relative, pin in SOURCE_PINS.items():
        need(hashlib.sha256((V9 / relative).read_bytes()).hexdigest() == pin,
             "reviewed source changed: " + relative)
    spec = importlib.util.spec_from_file_location("full_prior_scope", PRIOR)
    prior = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prior)
    # Replays the existing closed-capture authority and its source pins.
    checked = prior.analyze(snapshot)
    need(checked["status"] == "passed", "historical capture scope")
    receipt = V9 / "receipts/g4_core_warm_20260927/vm"
    names = ("validate", "static", "lots", "populations", "images", "bank")
    rows = []
    for checked_row in checked["rows"]:
        index = checked_row["index"]
        raw_path = receipt / f"probe_{index}.stdout"
        raw = json.loads(raw_path.read_text())
        phases = raw["tower_phases_ms"]
        exposed = {key: phases[key] for key in names}
        total = checked_row["tower_outside_encode_ms"]
        residual = total - sum(exposed.values())
        need(residual >= -0.01, "nested phase sum")
        subtotals = {key: sum(phases[key]) for key in (
            "static_collect_by_k", "static_sort_by_k",
            "static_groups_by_k", "static_resolve_by_k")}
        rows.append({
            "probe": index,
            "raw_sha256": hashlib.sha256(raw_path.read_bytes()).hexdigest(),
            "outside_encode_ms": total,
            "exposed_phases_ms": exposed,
            "unassigned_outside_encode_ms": residual,
            "static_subtotals_ms": subtotals,
            "validate_parts_ms": phases["validate_parts"],
            "lots_by_k_ms_not_additive": phases["lots_by_k"],
            "static_by_k_ms": phases["static_by_k"],
        })
    return {
        "status": "passed",
        "scope": "historical_first_pass_FULL_construction_not_new_benchmark",
        "GCP_calls": False,
        "reviewed_commit": "a7e80d7f964ae284902401bec3e2df1026441b5a",
        "source_pins": SOURCE_PINS,
        "prior_reader_sha256": hashlib.sha256(PRIOR.read_bytes()).hexdigest(),
        "rows": rows,
    }


if __name__ == "__main__":
    need(len(sys.argv) == 2, "usage: analyze.py <closed G4 snapshot.tar.gz>")
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2, sort_keys=True))
