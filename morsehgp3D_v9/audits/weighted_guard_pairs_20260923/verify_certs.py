#!/usr/bin/env python3
"""Independently validate archived weighted matchings at all 64 box corners."""

import json
import struct
import sys
from pathlib import Path

from verify_math import corners, margin


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def main(cert_path, result_path, points_path=None, raw_ids_path=None):
    expected = json.loads(Path(result_path).read_text(encoding="utf-8"))
    expected_ordinals = {x["ordinal"] for x in
                         expected["positive_big"]["gained_positive_ordinals"]}
    check(len(expected_ordinals) == 15, "expected positive rectangle count")
    points = Path(points_path).read_bytes() if points_path else None
    raw_ids = Path(raw_ids_path).read_bytes() if raw_ids_path else None
    check((points is None) == (raw_ids is None), "provide both input files")
    if points is not None:
        check(len(points) == 123389 * 12 and len(raw_ids) == 123389 * 4,
              "input byte lengths")
    seen = set()
    box = None
    groups = {3: [], 4: []}
    pair_count = 0
    corner_count = 0
    raw_ids_checked = set()
    for line in Path(cert_path).read_text(encoding="ascii").splitlines():
        t = line.split()
        check(t, "blank certificate line")
        if t[0] == "CERTBOX":
            check(len(t) == 15 and box is None, "bad CERTBOX")
            v = [int(x) for x in t[1:]]
            ordinal, mask = v[:2]
            check(ordinal in expected_ordinals and ordinal not in seen,
                  "unexpected/duplicate certificate")
            check(mask in (2, 4, 6), "bad lane mask")
            a_lo, a_hi, b_lo, b_hi = (tuple(v[i:i + 3]) for i in (2, 5, 8, 11))
            check(all(0 <= l <= h < (1 << 18)
                      for lo, hi in ((a_lo, a_hi), (b_lo, b_hi))
                      for l, h in zip(lo, hi)), "box domain/order")
            box = (ordinal, mask, a_lo, a_hi, b_lo, b_hi)
            groups = {3: [], 4: []}
        elif t[0] == "CERTPAIR":
            check(len(t) == 13 and box is not None, "bad CERTPAIR")
            v = [int(x) for x in t[1:]]
            ordinal, lane, gi = v[:3]
            g = tuple(v[3:6])
            hi = v[6]
            h = tuple(v[7:10])
            lam, mu = v[10:12]
            check(ordinal == box[0] and lane in (3, 4), "pair context")
            check(box[1] & (2 if lane == 3 else 4), "inactive lane pair")
            check(gi != hi and 0 <= gi < 123389 and 0 <= hi < 123389,
                  "site index")
            check((lam, mu) in {(1, 1), (1, 2), (2, 1), (2, 3), (3, 2)},
                  "unbudgeted ratio")
            check(all(0 <= x < (1 << 18) for x in g + h), "guard domain")
            for guard in (g, h):
                check(not any(all(low <= x <= high
                                  for low, x, high in zip(lo, guard, hi))
                              for lo, hi in ((box[2], box[3]), (box[4], box[5]))),
                      "guard inside a factor box")
            if points is not None:
                check(struct.unpack_from("<III", points, 12 * gi) == g and
                      struct.unpack_from("<III", points, 12 * hi) == h,
                      "guard coordinate/input mismatch")
                raw_ids_checked.add(struct.unpack_from("<I", raw_ids, 4 * gi)[0])
                raw_ids_checked.add(struct.unpack_from("<I", raw_ids, 4 * hi)[0])
            for a in corners(box[2], box[3]):
                for b in corners(box[4], box[5]):
                    H, F = margin(a, b, g, h, lam, mu, lane)
                    check(H > 0 and F > 0, "invalid strict corner certificate")
                    corner_count += 1
            groups[lane].append((gi, hi))
            pair_count += 1
        elif t[0] == "CERTEND":
            check(len(t) == 2 and box is not None and int(t[1]) == box[0],
                  "bad CERTEND")
            for lane, bit, need in ((3, 2, 4), (4, 4, 3)):
                ids = [site for pair in groups[lane] for site in pair]
                check(len(groups[lane]) == (need if box[1] & bit else 0),
                      "missing/excess matching pairs")
                check(len(ids) == len(set(ids)), "reused site within lane")
                if raw_ids is not None:
                    check(len({struct.unpack_from("<I", raw_ids, 4 * site)[0]
                               for site in ids}) == len(ids),
                          "reused raw return ID within lane")
            seen.add(box[0])
            box = None
        else:
            raise RuntimeError("unknown certificate record")
    check(box is None and seen == expected_ordinals, "incomplete certificate set")
    print(json.dumps({"rectangles": len(seen), "witness_pairs": pair_count,
                      "strict_corner_checks": corner_count,
                      "distinct_raw_return_ids_checked": len(raw_ids_checked)},
                     sort_keys=True))


if __name__ == "__main__":
    check(len(sys.argv) in (3, 5),
          "usage: verify_certs.py CERTIFICATES.tsv RESULTS.json [points.u32le raw_ids.u32le]")
    main(*sys.argv[1:])
