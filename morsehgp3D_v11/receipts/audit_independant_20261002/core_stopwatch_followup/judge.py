#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
checks = 0

def require(condition, message):
    global checks
    checks += 1
    if not condition:
        raise RuntimeError(message)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
after = json.loads((HERE / "SOURCE_AFTER.json").read_text())
require(len(before["sources"]) == 6, "snapshot domain")
require(before["sources"] == after["sources"] and after["stable"], "live source drift")
require(after["snapshot_intact"], "snapshot changed")
for item in before["sources"]:
    require(digest(HERE / "sources" / item["path"]) == item["sha256"], "source hash: " + item["path"])

for name in ("compiler_version", "dependencies", "compile", "native"):
    result = json.loads((HERE / (name + ".json")).read_text())
    require(result["exit_code"] == 0, name + " execution")
    require(digest(HERE / (name + ".stdout.txt")) == result["stdout_sha256"], name + " stdout")
    require(digest(HERE / (name + ".stderr.txt")) == result["stderr_sha256"], name + " stderr")
require((HERE / "compile.stderr.txt").read_bytes() == b"", "compiler diagnostics")
require((HERE / "native.stderr.txt").read_bytes() == b"", "terminate or runtime diagnostics")

dependencies = (HERE / "dependencies.stdout.txt").read_text().replace("\\\n", " ").split()
expected = {str(HERE / "sources" / item["path"]) for item in before["sources"] if item["path"].startswith("src/")}
actual = {token for token in dependencies if token.startswith(str(HERE / "sources"))}
require(actual == expected, "project dependency closure")
compiled = json.loads((HERE / "compile.json").read_text())["argv"]
require(str(HERE / "sources/src/core/ledger.cpp") in compiled, "compile frozen unit")
require("-DMHGP11_COORD_BITS=18" in compiled, "explicit profile")

native = json.loads((HERE / "native.stdout.txt").read_text())
expected_output = {"status": "PASS", "monotone_reads": 10001, "monotone": True,
                   "trivially_destructible": True, "mutable_destructor_allocations": 0,
                   "reset_destructor_allocations": 0, "mutation_did_not_publish": True,
                   "reset_did_not_publish": True, "explicit_guarded_reason": "memory_budget",
                   "transaction_preserved": True, "existing_no_allocation": True, "retry_published": True}
for key, value in expected_output.items():
    require(native.get(key) == value, "native outcome: " + key)

# At replay after closure, verify every artifact too. No assert: identical checks under Python -O.
manifest = HERE / "SHA256SUMS"
if manifest.exists():
    for line in manifest.read_text().splitlines():
        sha, name = line.split("  ", 1)
        if digest(HERE / name) != sha:
            raise RuntimeError("closed artifact hash: " + name)

print(json.dumps({"status": "PASS", "checks": checks, "scope": "Frozen Stopwatch follow-up, GCC normal only; no full integration qualification"}, sort_keys=True))
