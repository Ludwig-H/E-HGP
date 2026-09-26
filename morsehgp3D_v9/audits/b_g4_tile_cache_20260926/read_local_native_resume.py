#!/usr/bin/env python3
"""Live readback of the bounded portable-CPU capture (also under -O)."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys


def need(condition: bool, cause: str) -> None:
    if not condition:
        raise ValueError(cause)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    directory = Path(__file__).resolve().parent
    root = directory.parents[2]
    receipt = directory / "local_native_receipts"
    value = json.loads((receipt / "manifest.json").read_text())
    need(value["schema"] == "mhgp9_tile_cache_native_resume_v1", "wrong_schema")
    need(value["scope"] == "portable_cpu_release_only_not_cuda_not_full", "wrong_scope")
    need(value["status"] == "passed" and value["failure"] is None, "capture_failed")
    need(value["gcp_used"] is False, "not_a_cpu_only_capture")
    need(value["sources_unchanged"] is True and value["source_before"] == value["source_after"],
         "source_closure_failed")
    need(sha(directory / "run_local_native_resume.py") == value["runner_sha256"], "runner_changed")
    for name, expected in value["source_after"].items():
        need(sha(root / name) == expected, "source_changed:" + name)
    for name, expected in value["binaries"].items():
        need(sha(Path(value["build"]) / name) == expected, "binary_changed:" + name)
    labels = ["compiler", "configure", "build", "five_gates", "probe_refusals",
              "compile_mapping_gate", "mapping_gate", "compile_mapping_mutant", "mapping_mutant"]
    need([record["label"] for record in value["commands"]] == labels, "commands_missing_or_reordered")
    for record in value["commands"]:
        need(record["pass"] is True and record["exit_code"] == record["expected_exit"], "command_failed")
        combined = b""
        for stream in ("stdout", "stderr"):
            path = receipt / record[stream]
            need(sha(path) == record[stream + "_sha256"], "log_changed:" + str(path))
            combined += path.read_bytes()
        need(record["required_text"] in combined.decode(errors="replace"), "required_output_missing")
    need(len(value["binaries"]) == 6, "missing_binaries")
    mapping = json.loads((receipt / "06_mapping_gate.stdout").read_text())
    need(mapping == {"status": "pass", "checked_additions": 10083, "configurations": 182,
                     "pairs": 336196, "representatives": 12012, "bypassed_pairs": 10486},
         "mapping_results_differ")
    witness = (receipt / "03_five_gates.stdout").read_text()
    need("queries=98784 traces=3176 full=5776 partial=3298 open=86534" in witness,
         "cache_nonvacuity_missing")
    need("baseline_visits=2343212 representative_and_fallback_visits=2289970 cache_tests=264924" in witness,
         "cache_work_changed")
    need("cause=witness_cache.endpoint_retest" in witness, "endpoint_mutant_missing")
    print(json.dumps({"status": "pass", "commands": len(labels), "binaries": 6,
                      "live_sources": len(value["source_after"]),
                      "scope": value["scope"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, OSError) as error:
        print(f"native_readback: {error}", file=sys.stderr)
        raise SystemExit(1)
