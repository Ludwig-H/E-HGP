#!/usr/bin/env python3
"""Static receipt reader and optional LIVE replay of the dominant S2 group."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
AUDITS = HERE.parent
PROVENANCE = json.loads((HERE / "PROVENANCE.json").read_text())
TIMINGS = {"bvh_build_us", "palette_us", "collision_us", "test_us",
           "group_match_us", "pointwise_us"}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def check_static():
    for line in (HERE / "SHA256SUMS").read_text().splitlines():
        expected, name = line.split("  ", 1)
        require(digest(HERE / name) == expected, f"receipt SHA: {name}")
    require(digest(HERE / "dominant_group.bin") == PROVENANCE["group_sha256"],
            "group SHA")
    group = list(struct.iter_unpack("<IIII", (HERE / "dominant_group.bin").read_bytes()))
    require(len(group) == 67827 and sum(e[2] for e in group) == 368004895,
            "group count/F")
    require(len({(min(e[0], e[1]), max(e[0], e[1])) for e in group}) == len(group),
            "duplicate edge")
    masks = {2: 0, 4: 0, 6: 0}
    for a, b, F, mask in group:
        require(a != b and F >= 2 and mask in masks, "group edge schema")
        masks[mask] += 1
    require(masks == {2: 1225, 4: 31329, 6: 35273}, "group masks")
    dispatch = AUDITS / "paired_guard_dispatch_grid_20260923" / "RESULT.tsv"
    require(digest(dispatch) == PROVENANCE["upstream_dispatch_result_sha256"],
            "dispatch receipt SHA")
    groups = []
    for line in dispatch.read_text().splitlines():
        words = line.split()
        if words[:2] == ["G", "23"]:
            require(len(words) == 21, "dispatch group schema")
            groups.append((int(words[4]), int(words[2]), int(words[3])))
    groups.sort(reverse=True)
    require(len(groups) == 52 and groups[0] == (67827, 1305674776582, 0)
            and groups[1][0] == 9548, "group not largest by edge count")
    return group


def proof_candidates(group):
    path = AUDITS / "paired_guard_node_blocks_20260923" / "RESULT.stdout"
    require(digest(path) == PROVENANCE["proof_output_sha256"], "proof pool SHA")
    members = {(min(a, b), max(a, b)) for a, b, _, _ in group}
    pairs = set()
    lines = 0
    for line in path.read_text().splitlines():
        if not line.startswith("PROOF "):
            continue
        row = line.split()
        if row[1] != "1" or (int(row[4]), int(row[5])) not in members:
            continue

        def singleton_id(field):
            node, size, box, sites = field.split(":")
            require(int(size) == 1 and "," not in sites, "not a singleton proof")
            return int(sites.split("/")[0])

        g, h = singleton_id(row[10]), singleton_id(row[11])
        require(g != h, "reused guard")
        pairs.add((min(g, h), max(g, h)))
        lines += 1
    require(lines == 148 and len(pairs) == 101, "post-F candidate pool")
    return "".join(f"{g} {h}\n" for g, h in sorted(pairs))


def compile_cpp(source, binary, *, sanitizer=False, verify=False):
    args = ["clang++" if sanitizer else "g++", "-std=c++20",
            "-O2" if sanitizer else "-O3",
            "-Wall", "-Wextra", "-Werror"]
    if sanitizer:
        args += ["-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    if verify:
        args += ["-DVERIFY"]
    args += [str(source), "-o", str(binary)]
    subprocess.run(args, check=True)


def kv(line):
    words = line.split()
    require(len(words) % 2 == 0, "key/value output")
    out = {words[i]: int(words[i + 1]) for i in range(0, len(words), 2)}
    for key in TIMINGS:
        out.pop(key, None)
    return out


def parse_output(stderr):
    lines = stderr.splitlines()
    require(len(lines) in (3, 4), "shadow output line count")
    has_seed = lines[0].startswith("seed_count ")
    require(len(lines) == (4 if has_seed else 3), "seed/output line count")
    offset = int(has_seed)
    require(lines[offset].startswith("cand ") and
            lines[offset + 1].startswith("masks counts ") and
            lines[offset + 2].startswith("point_tests "), "shadow schema")
    masks = lines[offset + 1].split()
    require(len(masks) == 17 and masks[:2] == ["masks", "counts"] and
            masks[5] == "F" and masks[9] == "closed" and
            masks[13] == "Fclosed", "mask schema")
    return {
        "seed": kv(lines[0]) if has_seed else None,
        "group": kv(lines[offset]),
        "masks": {
            "counts": list(map(int, masks[2:5])),
            "F": list(map(int, masks[6:9])),
            "closed": list(map(int, masks[10:13])),
            "Fclosed": list(map(int, masks[14:17])),
        },
        "pointwise": kv(lines[offset + 2]),
    }


def live(points, ids, trace, sanitizer):
    require(digest(points) == PROVENANCE["points_sha256"] and
            digest(ids) == PROVENANCE["raw_ids_sha256"], "LIVE cloud SHA")
    group = check_static()
    with tempfile.TemporaryDirectory(prefix="mhgp9-group-bvh-") as temp:
        tmp = Path(temp)
        if trace is not None:
            for i, expected in enumerate(PROVENANCE["trace_parts_sha256"]):
                require(digest(trace / f"part_{i}.bin") == expected,
                        f"LIVE trace part {i} SHA")
            extractor = tmp / "extract"
            compile_cpp(HERE / "extract.cpp", extractor, sanitizer=sanitizer)
            emitted = tmp / "group.bin"
            result = subprocess.run([str(extractor), str(points), str(ids),
                                     str(trace), str(emitted)], check=True,
                                    capture_output=True, text=True)
            require("group 67827 F 368004895" in result.stderr and
                    emitted.read_bytes() == (HERE / "dominant_group.bin").read_bytes(),
                    "LIVE group extraction differs")

        candidates = tmp / "oracle_pool.tsv"
        candidates.write_text(proof_candidates(group))
        binary = tmp / "group_bvh"
        compile_cpp(HERE / "group_bvh.cpp", binary,
                    sanitizer=sanitizer, verify=True)
        cases = [("precore_16_leaf16", "seed:4", 16),
                 ("precore_16_leaf64", "seed:4", 64),
                 ("precore_16_leaf256", "seed:4", 256),
                 ("postF_panel_leaf64", str(candidates), 64)]
        measured = {}
        for name, selection, leafcap in cases:
            call = [str(binary), str(points), str(ids),
                    str(HERE / "dominant_group.bin"), selection, str(leafcap)]
            result = subprocess.run(call, check=True, capture_output=True, text=True)
            require(not result.stdout, "unexpected stdout")
            measured[name] = parse_output(result.stderr)
        expected = json.loads((HERE / "SUMMARY.json").read_text())
        require(measured == expected["cases"], "LIVE semantic counters differ")
        return {"cases": len(cases), "sanitizer": sanitizer,
                "trace_reextracted": trace is not None,
                "verified_uniform_incidence": sum(
                    row["group"]["validated_pairs"] for row in measured.values())}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--points", type=Path)
    parser.add_argument("--ids", type=Path)
    parser.add_argument("--trace", type=Path)
    parser.add_argument("--asan", action="store_true")
    args = parser.parse_args()
    require((args.points is None) == (args.ids is None),
            "points and IDs are required together")
    require(args.trace is None or args.points is not None,
            "trace requires points and IDs")
    check_static()
    if args.points is None:
        result = {"static": True, "group_edges": 67827}
    else:
        result = live(args.points, args.ids, args.trace, args.asan)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
