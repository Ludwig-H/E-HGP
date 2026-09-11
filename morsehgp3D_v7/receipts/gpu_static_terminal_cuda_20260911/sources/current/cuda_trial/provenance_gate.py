#!/usr/bin/env python3
"""Pure provenance predicates used by record.py, with causal stale-source rejects."""
import copy
import hashlib
import json
from pathlib import Path
import tempfile

import record


def main():
    checks = 0
    refusals = 0

    def need(good, why):
        nonlocal checks
        checks += 1
        if not good:
            raise RuntimeError(why)

    pins = {
        "terminal.cuh": "terminal",
        "terminal_owner.hpp": "owner",
        "intruder.cuh": "intruder",
        "nvcc_strict_host.py": "compiler",
        "source/morsehgp3D_v7/src/core/wide.hpp": "arithmetic",
        "source/morsehgp3D_v7/src/lanes/level.hpp": "levels",
        "source/morsehgp3D_v7/src/gpu/anchor_meb_key.cuh": "key",
        "source/morsehgp3D_v7/oracle/local_plateau_oracle.hpp": "oracle",
        "cuda_trial/export.cpp": "exporter",
        "cuda_trial/reference.hpp": "reference",
        "cuda_trial/fixture_types.hpp": "types",
        "cuda_trial/extra_helper.cuh": "extra",
        "cuda_trial/device_gate.cu": "consumer",
    }
    shared = record.common_closure(pins)
    binding = record.fixture_binding(pins)
    nominal = {"status": "passed", "sources_stable": True,
        "sources_before": pins, "sources_after": pins}
    need(record.compatible_prerequisite(nominal, shared), "nominal prerequisite")
    exported = {**nominal, "mode": "export", "snapshot_stable": True,
        "fixture_binding": binding, "fixture_sha256": "fixture"}
    need(record.compatible_export(exported, binding, "fixture"), "nominal export")
    for name in shared:
        for omit in (False, True):
            stale = copy.deepcopy(nominal)
            for side in ("sources_before", "sources_after"):
                # deepcopy preserves aliases in nominal: detach each dictionary.
                stale[side] = dict(stale[side])
                if omit:
                    del stale[side][name]
                else:
                    stale[side][name] = "old"
            need(not record.compatible_prerequisite(stale, shared), "old/missing prerequisite source: " + name)
            refusals += 1
    for name in binding:
        stale = copy.deepcopy(exported)
        stale["sources_before"] = dict(stale["sources_before"])
        stale["sources_after"] = dict(stale["sources_after"])
        stale["sources_before"][name] = stale["sources_after"][name] = "old"
        stale["fixture_binding"] = dict(stale["fixture_binding"])
        stale["fixture_binding"][name] = "old"
        need(not record.compatible_export(stale, binding, "fixture"), "old fixture authority: " + name)
        refusals += 1
    for field, value in (("status", "failed"), ("sources_stable", False), ("snapshot_stable", False),
        ("mode", "stub"), ("fixture_sha256", "other"), ("fixture_binding", {}), ("sources_after", {})):
        stale = copy.deepcopy(exported)
        stale[field] = value
        need(not record.compatible_export(stale, binding, "fixture"), "invalid export closure: " + field)
        refusals += 1
    extra = dict(pins)
    extra["source/new_helper.hpp"] = "new"
    need(not record.compatible_prerequisite(nominal, record.common_closure(extra)), "new shared dependency")
    need(not record.compatible_export(exported, record.fixture_binding(extra), "fixture"), "new export dependency")
    refusals += 2
    with tempfile.TemporaryDirectory(prefix="mhgp7-terminal-provenance-") as temporary:
        root = Path(temporary)
        payload = b"pinned source\n"
        target = root / "header.hpp"
        target.write_bytes(payload)
        digest = hashlib.sha256(payload).hexdigest()
        record.verify_snapshot(root, {"header.hpp": digest})
        need(True, "nominal snapshot")
        for mapping in ({"header.hpp": "wrong"}, {"../header.hpp": digest}, {str(target): digest}):
            caught = False
            try:
                record.verify_snapshot(root, mapping)
            except RuntimeError:
                caught = True
            need(caught, "snapshot drift/path refused")
            refusals += 1
    need(refusals >= 30, "nonvacuity")
    print(json.dumps({"status": "passed", "checks": checks, "rejections": refusals,
        "geometry_executed": False, "device_executed": False, "scope": "recorder_provenance_only"}))


if __name__ == "__main__":
    main()
