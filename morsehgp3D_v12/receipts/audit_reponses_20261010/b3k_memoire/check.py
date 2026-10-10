#!/usr/bin/env python3
"""Read pinned sources; replay a small allocation model. Never execute native code."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent


def need(value, message):
    if not value:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def capacity(size):
    if size < 262144:
        return size
    numerators = (1024, 1117, 1218, 1328, 1448, 1579, 1722, 1878)
    for k in range(208):
        raw = (256 * numerators[k % 8]) << (k // 8)
        cap = (raw + 4095) // 4096 * 4096
        if cap >= size:
            return cap
    return size


class CacheModel:
    """One synchronous caller, 8 GiB idle capacity, no system allocation failure.

    Starting base represents other live reservations, with no prior idle blocks.
    This models account transitions, never RSS or a reachable geometric fixture.
    """
    def __init__(self, base, cursor, limit):
        self.used = self.held = self.peak = base + cursor
        self.limit = limit
        self.idle = []
        self.evictions = 0
        need(self.held <= limit, "initial model budget")

    def release(self, reserved):
        self.used -= reserved
        if reserved >= 262144 and capacity(reserved) == reserved:
            need(sum(self.idle) + reserved <= 8 << 30, "model cache capacity")
            self.idle.append(reserved)
        else:
            self.held -= reserved

    def hold(self, size):
        if size > self.limit:
            return False
        while self.held + size > self.limit and self.idle:
            block = max(self.idle)
            self.idle.remove(block)
            self.held -= block
            self.evictions += 1
        if self.held + size > self.limit:
            return False
        self.held += size
        return True

    def allocate(self, size):
        cap = capacity(size)
        if size >= 262144 and cap in self.idle:
            self.idle.remove(cap)
            got = cap
        elif size >= 262144 and self.hold(cap):
            got = cap
        elif self.hold(size):
            got = size
        else:
            return 0
        self.used += got
        self.peak = max(self.peak, self.used)
        need(self.used + sum(self.idle) == self.held <= self.limit, "cache invariant")
        return got


def replay_models():
    pairs = 0
    for sites in range(65):
        for balls in range(65):
            c, k = 8 * sites, 16 * balls
            old, new = c + k, max(c, k)
            need(old - new == min(c, k), "no-cache coexistence bound")
            for room in (0, 1, c, k, max(c, k), c + k):
                need(not (room >= old) or room >= new, "no-cache admission monotonicity")
            pairs += 1
    # Same class reuse: reset lowers live used, not held; the keys reuse the block.
    reuse = CacheModel(1048576, 524288, 2097152)
    held = reuse.held
    reuse.release(524288)
    need(reuse.used == 1048576 and reuse.held == held, "retained cursor")
    need(reuse.allocate(524288) == 524288 and reuse.held == held, "reuse")
    # Different class under pressure: the inactive cursor is evictable.
    old = CacheModel(1048576, 524288, 2097152)
    need(old.allocate(1048576) == 0, "live cursor refusal witness")
    new = CacheModel(1048576, 524288, 2097152)
    new.release(524288)
    need(new.allocate(1048576) == 1048576 and new.evictions == 1, "eviction witness")
    # Early release can admit a rounded class instead of the old exact allocation.
    old = CacheModel(1000000, 8000, 1286720)
    new = CacheModel(1000000, 8000, 1286720)
    need(old.allocate(16 * 16385) == 262160, "old exact fallback")
    new.release(8000)
    need(new.allocate(16 * 16385) == 286720, "new rounded class")
    need((old.peak, new.peak) == (1270160, 1286720), "no universal cache peak gain")
    return {"no_cache_pairs": pairs, "cache_witnesses": 3,
            "cache_class_change_peaks": [old.peak, new.peak]}


def main():
    need(len(sys.argv) == 2, "usage: python check.py /path/to/repository")
    repo = Path(sys.argv[1])
    meta = json.loads((HERE / "capture.json").read_text())
    pin = meta["source_commit"]
    source = {}
    for name, wanted in meta["source_sha256"].items():
        raw = subprocess.check_output(["git", "-C", str(repo), "show", pin + ":morsehgp3D_v12/" + name])
        need(digest(raw) == wanted, "source digest: " + name)
        source[name] = raw.decode()
    for name in ("src/tower/pipeline.cpp", "src/tower/pipeline.hpp", "tests/tower/pipeline_levers.cpp"):
        before = subprocess.check_output(["git", "-C", str(repo), "show", "aa6338ee8:morsehgp3D_v12/" + name])
        need(before.decode() == source[name], "CST-0244 port: " + name)
    name = "src/catalogue/slices.cpp"
    anchor = "  for (u64 i = 0; i < n; ++i) values[cursor[idx(balls[i].support[0])]++] = make_id<BallIdx>(static_cast<u32>(i));\n"
    need(source[name].count(anchor) == 1, "patch anchor")
    after = source[name].replace(anchor, anchor + "  cursor.reset();  // aucune utilisation apres la repartition ; peut rendre le bloc au cache\n")
    patch = "".join(difflib.unified_diff(source[name].splitlines(True), after.splitlines(True),
                    fromfile="a/morsehgp3D_v12/" + name, tofile="b/morsehgp3D_v12/" + name))
    need(patch == (HERE / "proposition.patch").read_text(), "patch application by exact replacement")
    need(digest(patch.encode()) == meta["patch_sha256"], "patch hash")
    need(digest(after.encode()) == meta["patched_sha256"], "postimage hash")
    function = after.split("Outcome host_table(", 1)[1].split("Outcome materialize_levels(", 1)[0]
    need("cursor" not in function.split("  TableRows rows", 1)[1], "cursor dead before Pool tasks")
    rows = source[name].split("struct TableRows", 1)[1].split("}  // namespace", 1)[0]
    need("cursor" not in rows, "callback alias")
    out = {"source_commit": pin, "source_hashes": len(source), "patch": "unapplied; exact static postimage",
           "native_execution": False, **replay_models()}
    print(json.dumps(out, sort_keys=True, ensure_ascii=False))


if __name__ == "__main__":
    main()
