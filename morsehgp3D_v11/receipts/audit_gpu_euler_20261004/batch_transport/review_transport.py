"""Portable stdlib source/provenance checks; no C++/CUDA/native execution."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
checks = 0


def require(condition, message):
    global checks
    checks += 1
    if not condition:
        raise ValueError(message)


before = json.loads((ROOT / "SOURCE_BEFORE.json").read_text())
require(before["changed_during_original_copy"] == [], "source drift in original snapshot")
require(before["base_commit"] == "66372e621dcee58daaa7d7309875ab157894acf4", "wrong base")
require(len(before["files"]) == 18, "source count")
for rel, record in sorted(before["files"].items()):
    data = (ROOT / rel).read_bytes()
    require(hashlib.sha256(data).hexdigest() == record["sha256"], "hash " + rel)
    require(len(data) == record["bytes"], "size " + rel)

matrix = json.loads((ROOT / "MATRIX.json").read_text())
rows = matrix["rows"]
require(len(rows) == 9, "coverage count")
require(len({r["id"] for r in rows}) == 9, "coverage identities")
require([r["id"] for r in rows if r["status"].startswith("P1_")] == ["device_payload_budget"], "findings")
for row in rows:
    require(bool(row["sources"]), "unanchored " + row["id"])
    for anchor in row["sources"]:
        rel, separator, line = anchor.rpartition(":")
        if not separator:
            rel, line = anchor, None
        path = ROOT / "sources" / rel
        require(path.is_file(), "missing anchor " + anchor)
        if line is not None:
            require(1 <= int(line) <= len(path.read_text().splitlines()), "anchor line " + anchor)

cuda = (ROOT / "sources/src/catalogue/leaf_batch_cuda.cu").read_text()
allocation = cuda.split("struct DeviceArray {", 1)[1].split("\n};", 1)[0]
require("bytes += count * sizeof(T);" in allocation, "device diagnostic sum")
require("cudaMalloc(&data, count * sizeof(T))" in allocation, "device allocation")
require("MemoryBudget" not in allocation and ".admit(" not in allocation, "device guard changed")
require("balls[j] = s == leaf_device::kOk ? sink.balls : 0;" in cuda, "unresolved count")
require("if (j >= v.count || status[j] != leaf_device::kOk) return;" in cuda, "unresolved fill")
require(cuda.count("cudaDeviceSynchronize()") == 2, "phase synchronization")
fallback = (ROOT / "sources/src/catalogue/single_pass_batch.cpp").read_text()
require("plain.batch_leaves = plain.cuda_leaves = false;" in fallback, "whole-leaf fallback flags")
require("if (result.status[j] == leaf_device::kOk) continue;" in fallback, "fallback selection")
single = (ROOT / "sources/src/catalogue/single_pass.cpp").read_text()
require("execution.geometry_passes = 1;" in single, "work diagnostic changed")
parallel = (ROOT / "sources/src/catalogue/parallel.cpp").read_text()
require("if (weight != 1) return fail(Reason::multiplicity_unsupported);" in parallel, "unit multiplicities")
print(json.dumps({"schema": "ehgp.audit.gpu_batch_transport.check.v1", "checks": checks,
                  "sources": 18, "contracts": 9, "findings": ["device_payload_budget"],
                  "scope": "source/provenance only; no native, CUDA, numerical or performance qualification"},
                 sort_keys=True))
