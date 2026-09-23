#!/usr/bin/env python3
"""Static independent reader for bounded LiDAR rectangle triple shadow."""

import json
from itertools import product
from pathlib import Path

ROOT = Path(__file__).resolve().parent
lines = (ROOT / "RUN.stdout").read_text().splitlines()
rows = {}
proofs = {}
meta = None


def check(condition, message):
    if not condition:
        raise AssertionError(message)


for line in lines:
    words = line.split()
    if words[0] == "META":
        check(meta is None, "duplicate META")
        meta = list(map(int, words[1:]))
    elif words[0] == "ROW":
        check(len(words) == 36, "ROW width")
        key = int(words[2])
        check(key not in rows, "duplicate ROW")
        rows[key] = {"stratum": int(words[1]), "product": int(words[3]),
                     "segment": int(words[7]), "forms": int(words[8]),
                     "mask": int(words[6]), "greedy3": int(words[22]),
                     "greedy4": int(words[23]), "max3": int(words[34]),
                     "max4": int(words[35]), "pair_select_ns": int(words[31]),
                     "pair_class_ns": int(words[32])}
    elif words[0] == "TRIPLE":
        check(len(words) == 20, "TRIPLE width")
        key = int(words[1])
        check(key in rows and "triple3" not in rows[key], "orphan TRIPLE")
        row = rows[key]
        check((int(words[2]), int(words[3]), int(words[4]), int(words[5]),
               int(words[6])) == (row["segment"], row["forms"], row["mask"],
                                   row["max3"], row["max4"]), "ROW/TRIPLE mismatch")
        row.update(triple3=int(words[7]), triple4=int(words[8]),
                   mixed3=int(words[9]), mixed4=int(words[10]),
                   enumerated=int(words[11]), point3=int(words[12]),
                   point4=int(words[13]), tested=int(words[14]),
                   corners=int(words[15]), box3=int(words[16]),
                   box4=int(words[17]), select_ns=int(words[18]),
                   class_ns=int(words[19]))
    elif words[0] == "PROOF":
        key, lane = map(int, words[1:3])
        check((key, lane) not in proofs, "duplicate PROOF")
        nums = list(map(int, words[3:]))
        check(len(nums) >= 13, "short PROOF")
        A = (tuple(nums[:3]), tuple(nums[3:6]))
        B = (tuple(nums[6:9]), tuple(nums[9:12]))
        group_count = nums[12]
        index = 13
        groups = []
        for _ in range(group_count):
            size = nums[index]
            check(size in (2, 3), "group size")
            index += 1
            group = []
            for _ in range(size):
                guard = (nums[index], tuple(nums[index + 1:index + 4]))
                index += 4
                group.append(guard)
            groups.append(group)
        check(index == len(nums), "PROOF trailing data")
        proofs[key, lane] = (A, B, groups)
    elif words[0] == "STRATUM":
        continue
    else:
        raise AssertionError(f"unknown line: {words[0]}")

check(meta is not None and meta[0] == 123389 and meta[3] == 238364135,
      "input/front ledger")
check(meta[5] == 22034426 and meta[7:9] == [3986433, 559661741],
      "open/S2 ledger")
check(len(rows) == 1771 and all("triple3" in row for row in rows.values()),
      "row count/completeness")
big = [row for row in rows.values() if row["product"] >= 1024]
check(len(big) == 1747 and sum(row["segment"] > 0 for row in big) == 299,
      "large rectangle/positive count")


def closed(row, prefix):
    return ((not row["mask"] & 2 or row[prefix + "3"] >= 4)
            and (not row["mask"] & 4 or row[prefix + "4"] >= 3))


pair_closed = [row for row in big if closed(row, "max")]
mix_closed = [row for row in big if closed(row, "mixed")]
extra = {key for key, row in rows.items()
         if row["product"] >= 1024 and row["segment"] > 0
         and closed(row, "mixed") and not closed(row, "max")}
check((len(pair_closed), sum(r["segment"] for r in pair_closed),
       sum(r["forms"] for r in pair_closed)) == (1051, 2175, 5059809),
      "pair baseline")
check((len(mix_closed), sum(r["segment"] for r in mix_closed),
       sum(r["forms"] for r in mix_closed)) == (1055, 2066, 4194061),
      "mixed arm")
check(len(extra) == 2 and sum(rows[key]["segment"] for key in extra) == 44
      and sum(rows[key]["forms"] for key in extra) == 53523,
      "incremental closure")
check(set(key for key, _ in proofs) == extra, "proof rectangle set")
receipt = json.loads((ROOT / "RESULTS.json").read_text())
check(receipt["global_s2_edges"] == meta[7] and receipt["global_F"] == meta[8],
      "receipt global ledger")
check(receipt["pair_or_mixed_potential"]["positive_F"]
      == sum(row["forms"] for row in big
             if row["segment"] > 0 and (closed(row, "max") or closed(row, "mixed"))),
      "receipt union F")
check(receipt["pair_or_mixed_potential"]["new_positive_ordinals"]
      == [{"ordinal": key, "edges": rows[key]["segment"],
           "F": rows[key]["forms"]} for key in sorted(extra)],
      "receipt new rectangle details")
for name, subset in (("work_all_sampled", list(rows.values())),
                     ("work_big", big)):
    for key in ("enumerated", "point3", "point4", "tested", "corners",
                "box3", "box4", "select_ns", "class_ns", "pair_select_ns",
                "pair_class_ns"):
        check(receipt[name][key] == sum(row[key] for row in subset),
              f"receipt {name}/{key}")
failed_pairs = [row for row in big if not closed(row, "max")]
conditional = receipt["work_only_after_pair_max_failure"]
check(conditional["rectangles"] == len(failed_pairs)
      and conditional["positive"] == sum(row["segment"] > 0 for row in failed_pairs),
      "conditional rectangle ledger")
for key in ("enumerated", "tested", "corners", "select_ns", "class_ns"):
    check(conditional[key] == sum(row[key] for row in failed_pairs),
          f"conditional work/{key}")


def corners(box):
    return product(*zip(box[0], box[1]))


def score(a, b, guards):
    d = [b[i] - a[i] for i in range(3)]
    D = sum(v * v for v in d)
    ws = [[2 * g[i] - a[i] - b[i] for i in range(3)] for g in guards]
    H = len(guards) * D - sum(sum(v * v for v in w) for w in ws)
    W = [sum(w[i] for w in ws) for i in range(3)]
    C = (d[1] * W[2] - d[2] * W[1],
         d[2] * W[0] - d[0] * W[2],
         d[0] * W[1] - d[1] * W[0])
    X = sum(v * v for v in C)
    return H, X


tests = 0
for (key, lane), (A, B, groups) in proofs.items():
    row = rows[key]
    check(row["mask"] & (2 if lane == 3 else 4), "proof lane absent")
    check(len(groups) == row[f"mixed{lane}"] and len(groups) >= (4 if lane == 3 else 3),
          "proof credits")
    ids = [guard_id for group in groups for guard_id, _ in group]
    check(len(ids) == len(set(ids)), "guards not disjoint")
    for group in groups:
        guards = [coord for _, coord in group]
        for a, b in product(corners(A), corners(B)):
            H, X = score(a, b, guards)
            check(H > 0 and (3 * H * H > 4 * X if lane == 3 else H * H > 2 * X),
                  "failed exact corner certificate")
            tests += 1

print(f"PASS {len(big)} large rectangles, {len(extra)} new mixed closures, "
      f"{tests} independently checked strict corner tests")
