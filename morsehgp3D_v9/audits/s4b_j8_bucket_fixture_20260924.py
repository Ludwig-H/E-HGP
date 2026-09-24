#!/usr/bin/env python3
"""Independent exact-arithmetic fixture for the S4b J8 live-bucket cost.

This checks the geometry and the loop count; it does not run the product or
claim a LiDAR growth bound. No files are read or written.
"""

from fractions import Fraction
from math import isqrt


def need(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def ceil_sqrt_half(q: int) -> int:
    m = isqrt(q // 2)
    while 2 * m * m < q:
        m += 1
    need(m == 0 or 2 * (m - 1) * (m - 1) < q, "sqrt bound")
    return m


def check(l: int) -> tuple[int, int, int]:
    need(1 <= l <= 13107, "u18 parameter")
    d = 400 * l**2
    e = f = 244 * l**2
    gram = 57600 * l**4
    q = d * (3 * gram - 2 * e * f)
    need(q == 21491200 * l**6, "parameter bound")
    mubar = ceil_sqrt_half(q)
    g5 = mubar // 4
    zs = range(11 * l, 23 * l // 2 + 1)
    last_root = None
    lens4 = 0

    for z in zs:
        power = 57600 * l**4 * (z * z - 100 * l * l)
        side = 240 * l**2 * z
        root = Fraction(power, side)
        need(power > 0 and side > 0 and 0 < root < g5,
             "strictly interior root in live bucket j=4")
        need(power - g5 * side < 0, "right-end sign")
        lens4 += power < 0 and power - g5 * side < 0
        need(last_root is None or last_root < root, "distinct ordered roots")
        last_root = root

        # Complete cover radius |ab|, longest-edge owner ab, positive q4.
        need(z * z < d and 12 * 12 * l * l < d and
             100 * l * l + z * z < d and
             144 * l * l + z * z < d and (l // 2)**2 < d,
             "cover or owner")
        wx = Fraction(11, 72)
        wy = Fraction(z * z - 100 * l * l, 2 * z * z)
        wa = (1 - wx - wy) / 2
        need(wx > 0 and wy > 0 and wa > 0, "strict barycentric weights")

    events = len(zs)
    need(events == l // 2 + 1, "event count")
    need(g5 >= 819 * l**3, "bucket upper bound")
    # No site has negative power at the left endpoint g4=0; the three
    # support sites are on the whole family, hence lens[4] is zero.
    need(lens4 == 0, "lens")
    return events, events * events, min(events, 3)


def main() -> None:
    for l in (8, 10, 16, 64, 256, 13107):
        events, comparisons, shallow_k5 = check(l)
        print(f"L={l} events={events} comparisons={comparisons} "
              f"shallow_K5={shallow_k5}")


if __name__ == "__main__":
    main()
