#!/usr/bin/env python3
"""Rejoue CST-0007/0019 sur le buffer réellement porté en v12 ; Release, moins de 1 Mio."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
PIN = "95247cf4b"


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def main():
    paths = [f"morsehgp3D_v12/src/core/{name}" for name in
             ("buffer.cpp", "buffer.hpp", "types.hpp", "status.hpp", "reasons.def")]
    hashes = {}
    for rel in paths:
        data = (ROOT / rel).read_bytes()
        need(data == subprocess.check_output(["git", "-C", str(ROOT), "show", f"{PIN}:{rel}"]),
             f"source changée : {rel}")
        hashes[rel] = hashlib.sha256(data).hexdigest()
    code = r'''
#include <cstdio>
#include <new>
#include "core/buffer.hpp"
std::size_t requested_from_allocator = 0;
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  requested_from_allocator = n;
  try { return ::operator new(n); } catch (...) { return nullptr; }
}
int main() {
  constexpr mhgp12::u64 count = 262145;
  mhgp12::MemoryBudget budget(count, 1048576);
  mhgp12::Buffer<mhgp12::u8> bytes;
  if (!bytes.allocate(count, budget).ok()) return 1;
  const auto live = budget.used();
  bytes.reset();
  std::printf("{\"limit\":%llu,\"live_accounted\":%llu,\"allocator_request\":%zu,"
              "\"after_reset_used\":%llu,\"after_reset_idle\":%llu,\"admit_limit_again\":%s}\n",
              static_cast<unsigned long long>(budget.limit()), static_cast<unsigned long long>(live),
              requested_from_allocator, static_cast<unsigned long long>(budget.used()),
              static_cast<unsigned long long>(budget.cache_stats().idle), budget.admit(count).ok()?"true":"false");
}
'''
    with tempfile.TemporaryDirectory(prefix="v12-audit-cache-") as tmp:
        folder = Path(tmp)
        (folder / "w.cpp").write_text(code)
        subprocess.run(["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-DMHGP12_COORD_BITS=21", "-I", str(ROOT / "morsehgp3D_v12/src"), str(folder / "w.cpp"),
            str(ROOT / paths[0]), "-o", str(folder / "w")], check=True, capture_output=True)
        result = json.loads(subprocess.check_output([str(folder / "w")]))
    need(result == {"limit":262145, "live_accounted":262145, "allocator_request":286720,
                    "after_reset_used":0, "after_reset_idle":286720, "admit_limit_again":True},
         f"témoin différent : {result}")
    print(json.dumps({"pin":PIN, "source_sha256":hashes, "result":result,
        "scope":"real v12 buffer; requested allocation capacity, not RSS; no sanitizer campaign",
        "verdict":"CST-0007/0019 remain open after the mechanical port"}, indent=2))


if __name__ == "__main__":
    main()
