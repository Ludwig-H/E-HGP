#!/usr/bin/env python3
"""Copy closed inputs and record a per-K comparison, no C++ execution."""
import hashlib
import json
from pathlib import Path
import subprocess


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parents[1]
    observed = root / "build/v7_unique_representatives_20260911/runs/observed_uniform_n8000_s8"
    static = root / "build/v7_static_scale_20260911/n8000_s8_static4"
    if (here / "manifest.json").exists():
        raise RuntimeError("comparison already captured")
    selected = {"observed.stdout": observed / "stdout", "observed.receipt.json": observed / "receipt.json"}
    for name in ("stdout", "receipt.json", "command.json", "sources_before.json", "sources_after.json", "binary_before.sha256", "binary_after.sha256"):
        selected["static." + name] = static / name
    for name, path in selected.items():
        (here / name).write_bytes(path.read_bytes())
    a = json.loads((here / "observed.stdout").read_text())
    b = json.loads((here / "static.stdout").read_text())
    pairs = [{"K": y["K"], "R_observed": x["representative_occurrences"], "R_static": y["requests"],
              "U_observed": x["unique_representative_keys"], "U_static": y["unique"], "static_seeded_unique": y["seeded_unique"]}
             for x, y in zip(a["observer"]["rows"][1:], b["static_orders"][1:])]
    (here / "comparison.json").write_text(json.dumps({"authority": "read_only_per_order_counter_comparison_not_new_geometric_oracle",
        "input_digest": a["input_digest"], "payload_digest": a["payload_digest"], "orders": pairs,
        "observed_packet_manifest_sha256": "6f2c2c448a7de9d2d5f5af9f689e551fb78e4bf4bdb18c1b2b99cfcd9bdcb8fe"}, indent=2) + "\n")
    files = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(here.iterdir()) if p.is_file()}
    encoded = (json.dumps({"files": files}, indent=2, sort_keys=True) + "\n").encode()
    (here / "manifest.json").write_bytes(encoded)
    for mode in [[], ["-O"]]:
        subprocess.run(["python3", "-B", *mode, str(here / "verify.py")], check=True)
    print("manifest_sha256=" + hashlib.sha256(encoded).hexdigest())


if __name__ == "__main__":
    main()
