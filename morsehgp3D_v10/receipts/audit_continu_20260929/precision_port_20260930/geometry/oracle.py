"""Exact Fraction judge of small positive supports, independent of geom bodies.

Native rc0 is observational: only this judge assesses numeric agreement.
No assertion, coordinate quantization, engine, or GCP is used.
"""

import json
import sys
from fractions import Fraction


def sign(value):
    return (value > 0) - (value < 0)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def solve(matrix, values):
    augmented = [list(map(Fraction, row)) + [Fraction(value)]
                 for row, value in zip(matrix, values)]
    for col in range(len(values)):
        pivot = next((row for row in range(col, len(values))
                      if augmented[row][col]), None)
        if pivot is None:
            raise ValueError("singular support")
        augmented[col], augmented[pivot] = augmented[pivot], augmented[col]
        divisor = augmented[col][col]
        augmented[col] = [entry / divisor for entry in augmented[col]]
        for row in range(len(values)):
            if row == col:
                continue
            factor = augmented[row][col]
            augmented[row] = [entry - factor * base for entry, base
                              in zip(augmented[row], augmented[col])]
    return [row[-1] for row in augmented]


def sphere(points):
    anchor = points[0]
    vectors = [tuple(Fraction(x - y) for x, y in zip(point, anchor))
               for point in points[1:]]
    matrix = [[2 * dot(u, v) for v in vectors] for u in vectors]
    lambdas = solve(matrix, [dot(u, u) for u in vectors])
    center = [Fraction(anchor[axis]) +
              sum(weight * vector[axis] for weight, vector
                  in zip(lambdas, vectors)) for axis in range(3)]
    radius2 = sum((value - anchor[axis]) ** 2
                  for axis, value in enumerate(center))
    weights = [1 - sum(lambdas)] + lambdas
    if not all(weight > 0 for weight in weights):
        raise ValueError("support is not strictly positive")
    if not all(sum((Fraction(x) - value) ** 2
                   for x, value in zip(point, center)) == radius2
               for point in points):
        raise ValueError("support sphere check failed")
    return center, radius2, weights


def orient(points):
    a, b, c, d = points
    u, v, w = [tuple(x - y for x, y in zip(point, a))
               for point in (b, c, d)]
    return sign(u[0] * (v[1] * w[2] - v[2] * w[1]) -
                u[1] * (v[0] * w[2] - v[2] * w[0]) +
                u[2] * (v[0] * w[1] - v[1] * w[0]))


def fixture(mode, bits):
    maximum = (1 << bits) - 1
    a = (0, 0, 0)
    b = (maximum, maximum, 0)
    c = (maximum, 0, maximum)
    d = (0, maximum, maximum)
    points = [a, b, c, d]
    if mode in ("side2", "level2"):
        points = [a, (maximum,) * 3]
    elif mode in ("side3", "box3"):
        points = points[:3]
    center, radius2, weights = sphere(points)
    expected = {
        "mode": mode, "bits": bits, "support": [list(p) for p in points],
        "center": [str(v) for v in center], "radius2": str(radius2),
        "barycentric_weights": [str(v) for v in weights],
        "strictly_positive_support": True,
    }
    if mode == "orient":
        expected["orientations"] = [orient(points),
                                    orient([a, c, b, d]), 0]
    if mode == "box3":
        expected["box"] = {"lo": [0] * 3, "hi": [64 * maximum + 1] * 3}
        expected["ownership"] = all(
            lo <= 64 * value < hi
            for value, lo, hi in zip(center, expected["box"]["lo"],
                                     expected["box"]["hi"]))
    if mode in ("side2", "side3"):
        queries = [a, (maximum,) * 3, (maximum // 2,) * 3] if mode == "side2" else [d]
        powers = [sum((Fraction(x) - value) ** 2
                      for x, value in zip(query, center)) - radius2
                  for query in queries]
        expected["queries"] = [
            {"xyz": list(query), "side": sign(power), "power": str(power)}
            for query, power in zip(queries, powers)]
    return expected


CASES = [
    ("level4", 18), ("level4", 20), ("level4", 21), ("level4", 24),
    ("side3", 18), ("side3", 24), ("box3", 18), ("box3", 24),
    ("side2", 32), ("level2", 32), ("center4", 32), ("orient", 32),
]


def judge(actual):
    expected = fixture(actual["mode"], actual["bits"])
    violations = []
    if actual["support"] != expected["support"]:
        violations.append("support construction mismatch")
    if actual["mode"] == "orient":
        if actual["orientations"] != expected["orientations"]:
            violations.append("orientation mismatch")
    else:
        center = actual["center"]
        denominator = int(center["D"])
        if not actual["constructed"] or denominator <= 0:
            violations.append("center absent or nonpositive denominator")
        else:
            coordinates = [
                Fraction(int(value), denominator) for value in center["N"]]
            if coordinates != list(map(Fraction, expected["center"])):
                violations.append("rational center mismatch")
        if "level" in actual:
            num, den = int(actual["level"]["num"]), int(actual["level"]["den"])
            if den <= 0:
                violations.append("level denominator nonpositive")
            elif Fraction(num, den) != Fraction(expected["radius2"]):
                violations.append("radius level mismatch")
        if "ownership" in actual:
            if actual["box"] != expected["box"]:
                violations.append("box mismatch")
            if actual["ownership"] != expected["ownership"]:
                violations.append("half-open center ownership mismatch")
        if "queries" in actual:
            if len(actual["queries"]) != len(expected["queries"]):
                violations.append("query count mismatch")
            for got, wanted in zip(actual["queries"], expected["queries"]):
                if got["xyz"] != wanted["xyz"] or got["side"] != wanted["side"]:
                    violations.append("query side mismatch")
                if Fraction(got["key"]) != Fraction(wanted["power"]) * denominator:
                    violations.append("query power key mismatch")
    result = {"status": "MISMATCH" if violations else "MATCH",
              "violations": violations, "expected": expected, "actual": actual}
    print(json.dumps(result, sort_keys=True))
    return 1 if violations else 0


def main():
    if sys.argv[1:] == ["fixtures"]:
        panel = [fixture(mode, bits) for mode, bits in CASES]
        if len(panel) != 12 or not all(item["strictly_positive_support"]
                                      for item in panel):
            raise RuntimeError("non-vacuity failure")
        print(json.dumps(panel, sort_keys=True))
        return 0
    if len(sys.argv) == 3 and sys.argv[1] == "judge":
        return judge(json.loads(sys.argv[2]))
    return 2


if __name__ == "__main__":
    sys.exit(main())
