#!/usr/bin/env python3
"""Independent Python-integer check of the tiny spatial-node fixture."""

import itertools
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def basis(a, b):
    v = [b[i] - a[i] for i in range(3)]
    main = max(range(3), key=lambda i: (abs(v[i]), -i))
    i, j = (main + 1) % 3, (main + 2) % 3
    h = abs(v[main])
    sign = 1 if v[main] > 0 else -1
    A, B = [0, 0, 0], [0, 0, 0]
    A[i], A[main] = h, -sign * v[i]
    B[j], B[main] = h, -sign * v[j]
    return A, B, sum(value * value for value in v)


def form(a, b, A, B, D, Q, z, alpha, beta):
    w = [2 * z[i] - a[i] - b[i] for i in range(3)]
    return Q * (sum(value * value for value in w) - D) - 2 * sum(
        w[i] * (alpha * A[i] + beta * B[i]) for i in range(3))


def corners(low, high, cell):
    spatial = itertools.product(*[(low[i], high[i]) for i in range(3)])
    return [(z, alpha, beta) for z in spatial
            for alpha in cell[:2] for beta in cell[2:]]


def main():
    data = json.loads((HERE / "fixtures" / "geometry_cases.json").read_text())
    a, b = data["edge"]["a"], data["edge"]["b"]
    Q = data["Q"]
    A, B, D = basis(a, b)
    need((A, B, D) == ([0, 4, 0], [0, 0, 4], 16), "basis mismatch")
    cases = data["cases"]
    for name in ("endpoint_shell", "midpoint_interior"):
        c = cases[name]
        need(form(a, b, A, B, D, Q, c["z"], c["alpha"], c["beta"]) == c["H"],
             f"{name} form mismatch")
    for name in ("corner_failure_box", "uniform_midpoint_cell"):
        c = cases[name]
        maximum = max(form(a, b, A, B, D, Q, z, alpha, beta)
                      for z, alpha, beta in corners(c["low"], c["high"], c["cell"]))
        need(maximum == c["max_32_H"], f"{name} maximum mismatch")
    bad = cases["corner_failure_box"]
    need(bad["max_32_H"] >= 0, "failed maximum sign")
    need(form(a, b, A, B, D, Q, bad["actual_interior_site"], 0, 0) < 0,
         "corner failure lost interior site")
    need(cases["uniform_midpoint_cell"]["max_32_H"] < 0, "uniform sign")
    print("geometry_fixture_ok")


if __name__ == "__main__":
    main()
