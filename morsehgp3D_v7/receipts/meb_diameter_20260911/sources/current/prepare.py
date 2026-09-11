#!/usr/bin/env python3
"""Pin a private source closure and derive one named diameter-only variant."""
import difflib
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    source = HERE / "source"
    if source.exists():
        raise RuntimeError("create-only source")
    payload = {}
    for directory in ("src", "oracle"):
        for path in sorted((REPO / "morsehgp3D_v7" / directory).rglob("*")):
            if path.is_file() and path.suffix in (".hpp", ".h", ".cuh", ".cpp", ".cu"):
                payload[path.relative_to(REPO).as_posix()] = path.read_bytes()
    name = "morsehgp3D_v7/tests/anchor_meb_gate.cpp"
    payload[name] = (REPO / name).read_bytes()
    header = "morsehgp3D_v7/src/forest/anchor_meb.hpp"
    if sha(payload[header]) != "386072c8a02bbb836d0070a10e1421418a36d3f4024ade63b4c9014c5536f786":
        raise RuntimeError("MEB baseline mismatch")
    for relative, data in payload.items():
        path = source / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as output:
            output.write(data)
    text = payload[header].decode()
    body = text[text.index("inline AnchorMebResult anchor_meb("):text.rindex("}  // namespace mhgp7")]
    body = body.replace("anchor_meb(std::span<const P3> sites, AnchorMebWork& work)",
                        "anchor_meb_diameter(std::span<const P3> sites, AnchorMebWork& work, u64& diameter_pairs)", 1)
    start = "  bool finished = false;\n"
    prepass = """  const auto n = static_cast<u8>(sites.size());
  i64 diameter = -1;
  u8 extreme_a = 0, extreme_b = 1;
  for (u8 a = 0; a < n; ++a) for (u8 b = a + 1; b < n; ++b) {
    if (!anchor_meb_detail::charge(diameter_pairs))
      return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_diameter_pairs_overflow");
#if MHGP7_DIAMETER_MUTANT == 2
    --diameter_pairs;  // Deliberately hide a physically evaluated distance.
#endif
    const i64 distance = p3_norm2(p3_sub(sites[a], sites[b]));
#if MHGP7_DIAMETER_MUTANT == 1
    const bool farther = distance >= diameter;
#else
    const bool farther = distance > diameter;
#endif
    if (farther) { diameter = distance; extreme_a = a; extreme_b = b; }
  }
  // Visit each site exactly once: the two witnesses first, then the old order.
  std::array<u8, kFacetMaxK> power_order{};
  power_order[0] = extreme_a; power_order[1] = extreme_b;
  u8 at = 2;
  for (u8 i = 0; i < n; ++i) if (i != extreme_a && i != extreme_b) power_order[at++] = i;
  bool finished = false;
"""
    if body.count(start) != 1:
        raise RuntimeError("prepass splice")
    body = body.replace(start, prepass, 1)
    old_loop = "    for (const auto& point : sites) {\n"
    new_loop = """    for (u8 position = 0; position < n; ++position) {
#if MHGP7_DIAMETER_MUTANT == 3
      const auto& point = sites[position];
#else
      const auto& point = sites[power_order[position]];
#endif
"""
    if body.count(old_loop) != 1:
        raise RuntimeError("containment splice")
    body = body.replace(old_loop, new_loop, 1)
    body = body.replace("      if (power == 0) ++shell;", """      if (power == 0) ++shell;
#if MHGP7_DIAMETER_MUTANT == 4
      if (position < 2 && power == 0) ++shell;
#endif""", 1)
    old_pairs = """  const auto n = static_cast<u8>(sites.size());
  for (u8 a = 0; a < n && !finished; ++a)
    for (u8 b = a + 1; b < n && !finished; ++b)
      finished = attempt({a, b, 0, 0}, 2);
"""
    if body.count(old_pairs) != 1:
        raise RuntimeError("q2 splice")
    body = body.replace(old_pairs, "  finished = attempt({extreme_a, extreme_b, 0, 0}, 2);\n", 1)
    variant = """#pragma once
// PRIVATE: same canonical MEB, one lexicographically first maximum pair.
// The extra counter charges each squared-distance pair actually evaluated.
#include "source/morsehgp3D_v7/src/forest/anchor_meb.hpp"
#ifndef MHGP7_DIAMETER_MUTANT
#define MHGP7_DIAMETER_MUTANT 0
#endif
namespace mhgp7 {
""" + body + "}  // namespace mhgp7\n"
    # This artifact is mechanically derived, never substituted into the snapshot.
    (HERE / "anchor_meb_diameter.hpp").write_text(variant)
    original_function = text[text.index("inline AnchorMebResult anchor_meb("):text.rindex("}  // namespace mhgp7")]
    (HERE / "variant.diff").write_text("".join(difflib.unified_diff(original_function.splitlines(True), body.splitlines(True),
        fromfile="baseline/anchor_meb", tofile="private/anchor_meb_diameter")))
    for relative, data in payload.items():
        if (REPO / relative).read_bytes() != data:
            raise RuntimeError("active source drift during copy")
    (HERE / "origin.json").write_text(json.dumps({"scope": "private_diameter_MEB_only", "inherited_results": False,
        "source_pins": {relative: sha(data) for relative, data in payload.items()},
        "variant_sha256": sha(variant.encode()), "active_modified": False}, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
