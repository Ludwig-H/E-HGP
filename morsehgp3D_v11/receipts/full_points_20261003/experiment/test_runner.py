#!/usr/bin/env python3
"""Portable runner protocol test with exact Python-encoded dumps and mocks.

No native executable, sklearn fit, GCP command, or real input is executed. The
mock emits the independent Gamma/Fraction encoder used by test_qualified.py.
"""
from contextlib import redirect_stdout
import copy
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

import numpy as np

from qualified import HierarchyScorer, analyse, load
import run_experiment as runner
from test_qualified import Oracle

CHECKS = 0


def check(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def rejected(action, message):
    try:
        action()
    except ValueError:
        check(True, message)
        return
    check(False, message)


def encoded(points, ids, kmax):
    oracle = Oracle(points, kmax)
    payload = bytearray(oracle.encode())
    struct.pack_into("<Q", payload, 10, 21)
    for i, original_id in enumerate(ids):
        struct.pack_into("<Q", payload, 42 + 32*i + 24, int(original_id))
    return bytes(payload)


class MockNative:
    def __init__(self, mismatch=False):
        self.calls = []
        self.mismatch = mismatch

    def __call__(self, command, capture_output, timeout):
        check(capture_output and timeout > 0, "captured bounded mock subprocess")
        _probe, xyz_path, ids_path, output_path, kmax, _workers, _budget = command
        xyz = np.fromfile(xyz_path, dtype="<u4").reshape(-1, 3)
        ids = np.fromfile(ids_path, dtype="<u4")
        check(xyz.dtype == np.dtype("<u4") and ids.dtype == np.dtype("<u4"), "binary input dtypes")
        # Alternate native site order deliberately; native SiteIdx != input row.
        permutation = (2, 0, 1) if len(self.calls) % 2 == 0 else (1, 2, 0)
        points = [tuple(int(x) for x in xyz[i]) for i in permutation]
        identifiers = [int(ids[i]) for i in permutation]
        if self.mismatch:
            points[0] = tuple(x+1 for x in points[0])
        payload = encoded(points, identifiers, int(kmax))
        Path(output_path).write_bytes(payload)
        self.calls.append(dict(points=points, ids=identifiers, payload=payload))
        return SimpleNamespace(returncode=0, stderr=b"",
                               stdout=json.dumps(dict(status="ok", sites=len(ids), mock_only=True)).encode())


def mock_hdbscan(points, labels, k):
    check(points.dtype == np.dtype("<u4") and labels.dtype == np.dtype("<i4"),
          "HDBSCAN receives the same typed rows")
    check(1 <= k <= len(points), "HDBSCAN order forwarding")
    scorer = HierarchyScorer([int(x) for x in labels])
    for site in range(len(labels)):
        scorer.activate(site, 0.0)
    scorer.observe(0.0)
    for site in range(1, len(labels)):
        scorer.union(0, site)
    scorer.observe(1.0)
    answer = scorer.finish()
    answer["mock_only"] = True
    return answer


def main():
    xyz = np.asarray([(2, 2, 2), (4, 2, 2), (7, 2, 2)], dtype="<u4")
    ids = np.asarray([15, 3, 99], dtype="<u4")
    labels = np.asarray([0, 0, 1], dtype="<i4")
    config = dict(kmax=3, workers=1, budget_bytes=1 << 20, native_timeout_seconds=1,
                  orders=[2, 3], jitter_orders=["2", "3"], jitter_thresholds=["3"],
                  jitter=False, jitter_case="toy", jitter_seed=1, jitter_pair_seed=2)
    with tempfile.TemporaryDirectory(prefix="ehgp-audit-runner-") as temporary:
        directory = Path(temporary)
        args = SimpleNamespace(work=directory/"work", out=directory/"out", probe=Path("mock-native"))
        args.work.mkdir()
        args.out.mkdir()
        native = MockNative()
        with patch.object(runner.subprocess, "run", native):
            first, trees, first_ids = runner.project(args, "first", xyz, ids, labels, config, True)
            second, alternate, second_ids = runner.project(args, "second", xyz, ids, labels, config, True)
        check(first_ids == [99, 15, 3] and second_ids == [3, 99, 15], "original IDs survive permutations")
        check(all(len(x["tree"]["height"]) >= 3
                  for order in trees["orders"].values() for x in order["qualified"].values()),
              "retained full trees survive output compaction")
        check(all("tree" not in value and "tree_sha256" in value
                  for _k, _name, value in runner.methods(first["projection"])), "all methods compacted")
        check(not (args.work/"first"/"points.bin").exists() and
              not (args.work/"first"/"sites.u32le").exists(), "successful private data removed")
        for call, result in zip(native.calls, (first, second)):
            data = load(call["payload"])
            rows = {int(identifier): i for i, identifier in enumerate(ids)}
            truth = [int(labels[rows[identifier]]) for identifier in call["ids"]]
            expected = analyse(data, truth, orders=config["orders"])
            for k, name, value in runner.methods(expected):
                compact = runner.compact(value)
                if name.startswith("qualified_m"):
                    got = result["projection"]["orders"][k]["qualified"][name[len("qualified_m"):]]
                else:
                    got = result["projection"]["orders"][k][name]
                check(compact == got, "native label/PointId mapping matches independent expectation")
        jitter = runner.jitter_check(trees, alternate, first_ids, second_ids, 71)
        check(jitter["status"] == "ok" and jitter["pair_checks"] > 0,
              "jitter matches IDs, not incidental SiteIdx")
        # A changed native coordinate with an unchanged ID is a protocol error.
        with patch.object(runner.subprocess, "run", MockNative(mismatch=True)):
            rejected(lambda: runner.project(args, "mismatch", xyz, ids, labels, config),
                     "XYZ/ID mismatch rejected")
        with patch.object(runner.subprocess, "run", MockNative()), \
                patch.object(runner, "analyze_hdbscan", mock_hdbscan):
            with redirect_stdout(io.StringIO()):
                runner.one_case(args, "toy", xyz, ids, labels, {"mock_only": True}, config)
        result = json.loads((args.out/"toy.json").read_text())
        check(result["status"] == "ok" and set(result["hdbscan"]) == {"2", "3"},
              "whole case completes and serializes mocked hierarchies")
        check(result["label_sha256"] == hashlib.sha256(labels.tobytes()).hexdigest(),
              "label input bytes attested")
        # A completed build is insufficient: refuse a failed, incomplete or
        # differently pinned exact gate before any campaign can start.
        probe = directory/"mock-probe"
        probe.write_bytes(b"mock-only, never executed")
        pins = {str(probe.resolve()): runner.digest(probe)}
        valid = dict(status="pass", errors=[], check_count=1,
                     cases=[dict(status="pass")], sources_before=pins, sources_after=pins)
        path = directory/"gate.json"
        path.write_text(json.dumps(valid))
        check(runner.read_gate(probe, path) == valid, "successful exact gate accepted")
        mutants = []
        for field, value in (("status", "fail"), ("errors", ["failure"]),
                             ("check_count", 0), ("cases", []),
                             ("cases", [dict(status="fail")]),
                             ("sources_after", {})):
            mutant = copy.deepcopy(valid)
            mutant[field] = value
            mutants.append(mutant)
        mutant = copy.deepcopy(valid)
        mutant["sources_before"] = mutant["sources_after"] = {str(probe.resolve()): "wrong"}
        mutants.append(mutant)
        for mutant in mutants:
            path.write_text(json.dumps(mutant))
            rejected(lambda: runner.read_gate(probe, path), "invalid exact gate refused")
    print(json.dumps(dict(status="pass", checks=CHECKS,
                          scope="mocked protocol only; no native, fit, real data or GCP"), sort_keys=True))


if __name__ == "__main__":
    main()
