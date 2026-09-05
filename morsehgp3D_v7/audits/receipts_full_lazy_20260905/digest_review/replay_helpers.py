#!/usr/bin/env python3
"""Audit data models for captured Python helpers; no probe receipt is invented."""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
from typing import Any


HERE = Path(__file__).resolve().parent
Json = dict[str, Any]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, required=True)
    args = parser.parse_args()
    captures = json.loads((HERE / "sources_as_read.json").read_text())
    for item in captures["sources"]:
        require(digest(HERE / Path(item["capture"]).name) == item["sha256"], "capture_pin")
    path = HERE / "full_gabriel_lazy_probe_audit.py"
    spec = importlib.util.spec_from_file_location("captured_lazy_judge", path)
    require(spec is not None and spec.loader is not None, "import_spec")
    judge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(judge)

    def row() -> Json:
        return {name: 0 for name in judge.WORK_FIELDS}

    lazy = row()
    lazy.update(k=2, certificate_minima=2, connection_catalogue_records=1,
                face_visits=2, minimum_lookups=2, minimum_hits=1,
                cache_lookups=1, cache_inserts=1, portal_requests=1,
                meb_calls=1, geometry_meb_calls=1, terminal_direct=1,
                singleton_intruder_resolutions=1, direct_lookups=1)
    eager = row()
    eager.update(k=2, certificate_minima=2, connection_catalogue_records=1,
                 face_visits=5, aliases=3, alias_hits=2)
    zero = copy.deepcopy(lazy)
    zero.update(cache_inserts=0, cache_skips=1)
    positives: list[Json] = []
    for name, value, policy, cap in (
            ("lazy_first_one", lazy, "lazy", 1),
            ("lazy_zero_cache", zero, "lazy", 0),
            ("eager_obtuse_style_counters", eager, "eager", 0)):
        judge.numeric(value)
        judge.work_policy(value, policy, cap)
        judge.success_identities(value, policy)
        positives.append({"name": name, "data_model": value, "policy": policy, "capacity": cap})

    mutations = [
        ("lazy_alias_present", lazy, "lazy", 1, {"aliases": 1}, "lazy_aliases"),
        ("mirrored_meb_erased", lazy, "lazy", 1,
         {"meb_calls": 0, "geometry_meb_calls": 0, "direct_lookups": 0}, "success_work_identity"),
        ("lazy_partition_broken", lazy, "lazy", 1, {"minimum_hits": 0}, "lazy_success_identity"),
        ("eager_alias_extra", eager, "eager", 0, {"aliases": 4}, "eager_success_identity"),
    ]
    rejected: list[Json] = []
    for name, original, policy, cap, patch, reason in mutations:
        changed = copy.deepcopy(original)
        changed.update(patch)
        try:
            judge.numeric(changed)
            judge.work_policy(changed, policy, cap)
            judge.success_identities(changed, policy)
        except ValueError as error:
            require(str(error) == reason, "wrong_rejection:" + name + ":" + str(error))
            rejected.append({"name": name, "replacement_values": patch, "reason": reason})
    require(len(rejected) == 4, "rejection_floor")

    skipped = copy.deepcopy(lazy)
    skipped.update(cache_inserts=0, cache_skips=1)
    judge.numeric(skipped)
    judge.work_policy(skipped, "lazy", 1)
    judge.success_identities(skipped, "lazy")
    require(skipped["cache_inserts"] != min(1, skipped["portal_requests"]), "first_c_counterfixture")

    # Independent byte assembly of the public aggregate wire. It does not
    # execute the C++ serializer or recalculate any reported forest digest.
    def tag(value: bytes) -> bytes:
        return struct.pack("<Q", len(value)) + value

    bindings: list[Json] = []
    input_hash = "01" * 32
    for hashes in ([], ["23" * 32], ["23" * 32, "45" * 32]):
        wire = tag(b"mhgp7-full-semantic-v1:horizontal-orders")
        wire += tag(input_hash.encode("ascii")) + struct.pack("<Q", len(hashes))
        for index, value in enumerate(hashes, 1):
            wire += struct.pack("<Q", index) + tag(value.encode("ascii"))
        expected = hashlib.sha256(wire).hexdigest()
        require(judge.aggregate_digest(input_hash, hashes) == expected, "aggregate_wire")
        bindings.append({"input_hash": input_hash, "order_hashes": hashes,
                         "wire_hex": wire.hex(), "expected_sha256": expected})
    require(len(positives) == 3 and len(bindings) == 3, "positive_floor")
    output = {
        "schema": "mhgp7-lazy-digest-helper-review-v1", "status": "passed",
        "python_optimized": bool(sys.flags.optimize), "public_status": "not_claimed",
        "scope": "counter_data_models_and_python_aggregate_wire_only",
        "not_a_probe_admission": True, "not_a_geometric_fixture": True,
        "cpp_or_engine_invoked": False, "canonical_cli_selftests_invoked": False,
        "pins": captures, "script_sha256": digest(Path(__file__)),
        "counts": {"positive_counter_models": 3, "data_mutations_rejected": 4,
                   "first_c_corruptions_accepted_by_captured_helpers": 1,
                   "independent_aggregate_preimages": 3},
        "positive_counter_models": positives, "rejected_data_mutations": rejected,
        "first_c_counterfixture": {
            "kind": "audit_counter_data_corruption_not_product_mutant",
            "policy": "lazy_first_c_strict_resolutions_v1", "capacity": 1,
            "changed_row": skipped, "captured_helper_verdict": "accepted",
            "violated_necessary_success_identity": "cache_inserts=min(capacity,portal_requests)",
            "expected_inserts": 1, "observed_corrupted_inserts": 0,
            "scope": "work_policy_and_success_identities_not_whole_receipt_judge",
        },
        "aggregate_wire_examples": bindings,
    }
    args.result.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "passed", "counts": output["counts"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
