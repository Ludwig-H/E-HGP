#!/usr/bin/env python3
"""Juge Fraction des sorties de probe (normal et UBSan). Arithmetique exacte, independante des formules natives
pour les niveaux (centre par systeme lineaire de Gram), aucun assert (valable sous python3 -O).
Usage : judge.py SORTIE.jsonl -> JSON ; code 0 si tous les verdicts attendus sont observes.
"""
import json
import sys
from fractions import Fraction as F


def solve3(A, b):
    """Resolution exacte (Cramer) d'un systeme 3x3."""
    def det(m):
        return (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
                + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))
    d = det(A)
    out = []
    for i in range(3):
        m = [row[:] for row in A]
        for r in range(3):
            m[r][i] = b[r]
        out.append(F(det(m), d))
    return out


def circumcenter4(P):
    a = P[0]
    A = [[2 * (p[i] - a[i]) for i in range(3)] for p in P[1:]]
    b = [sum(p[i] ** 2 - a[i] ** 2 for i in range(3)) for p in P[1:]]
    return solve3(A, b)


def circumcenter3(P):
    a, b, c = P
    u = [b[i] - a[i] for i in range(3)]
    v = [c[i] - a[i] for i in range(3)]
    n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
    A = [[2 * u[i] for i in range(3)], [2 * v[i] for i in range(3)], n]
    rhs = [sum(b[i] ** 2 - a[i] ** 2 for i in range(3)), sum(c[i] ** 2 - a[i] ** 2 for i in range(3)),
           sum(n[i] * a[i] for i in range(3))]
    return solve3(A, rhs)


def d2(p, q):
    return sum((F(p[i]) - F(q[i])) ** 2 for i in range(3))


def orient(p, q, r, s):
    a = [q[i] - p[i] for i in range(3)]
    b = [r[i] - p[i] for i in range(3)]
    c = [s[i] - p[i] for i in range(3)]
    return (a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) + a[2] * (b[0] * c[1] - b[1] * c[0]))


def sgn(x):
    return (x > 0) - (x < 0)


def main():
    rows = [json.loads(line) for line in open(sys.argv[1])]
    verdicts = []
    for r in rows:
        c = r["case"]
        if c == "level4_regular_tetra":
            M = r["M"]
            P = [(0, 0, 0), (M, M, 0), (M, 0, M), (0, M, M)]
            cc = circumcenter4(P)
            want = d2(cc, P[0])
            got = F(int(r["num"]), int(r["den"])) if int(r["den"]) else None
            den_true = int(r["D"]) ** 2
            verdicts.append({"case": c, "B": r["B"], "true_level": str(want), "observed": str(got),
                             "correct": got == want, "num_zero": int(r["num"]) == 0,
                             "den_true_bits": den_true.bit_length(), "den_truncated": int(r["den"]) != den_true,
                             "den_mod_2p128": int(r["den"]) == den_true % (1 << 128)})
        elif c == "level_at_most_port":
            lvl = F(int(r["num"]), int(r["den"]))
            truth = lvl <= r["e"]
            verdicts.append({"case": c, "level": str(lvl), "truth": truth, "naive_correct": bool(r["naive_head_rule"]) == truth,
                             "wide_correct": bool(r["wide"]) == truth})
        elif c == "level_at_most_b24_tetra":
            M = (1 << 24) - 1
            lvl = F(int(r["num"]), int(r["den"]))
            truth = lvl <= r["e"]
            verdicts.append({"case": c, "level_equals_3M2_over_4": lvl == F(3 * M * M, 4), "truth": truth,
                             "num_bits": int(r["num"]).bit_length(), "den_bits": int(r["den"]).bit_length(),
                             "naive_correct": bool(r["naive_head_rule"]) == truth,
                             "wide_correct": bool(r["wide"]) == truth})
        elif c == "side_q3_equilateral":
            M = r["M"]
            P = [(0, 0, 0), (M, M, 0), (M, 0, M)]
            cc = circumcenter3(P)
            z = tuple(r["query"])
            truth = sgn(d2(z, cc) - d2(P[0], cc))
            verdicts.append({"case": c, "B": r["B"], "truth": truth, "observed": r["side"], "correct": truth == r["side"]})
        elif c == "orient_center_q4_regular_tetra":
            B = r["B"]
            M = (1 << B) - 1
            P = [(0, 0, 0), (M, M, 0), (M, 0, M), (0, M, M)]
            cc = circumcenter4(P)
            truth = []
            for f in range(4):
                p, q, s = P[(f + 1) % 4], P[(f + 2) % 4], P[(f + 3) % 4]
                a = [q[i] - p[i] for i in range(3)]
                b = [s[i] - p[i] for i in range(3)]
                w = [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
                truth.append(sgn(sum(w[i] * (cc[i] - p[i]) for i in range(3))))
            verdicts.append({"case": c, "B": B, "truth": truth, "observed": r["orient_center"],
                             "correct": truth == r["orient_center"], "inside_observed": r["strictly_inside"]})
        elif c == "morton64_mask":
            verdicts.append({"case": c, "collision_000_vs_2p21": r["key_000"] == r["key_2p21_00"],
                             "collision_100_vs_2p21p1": r["key_100"] == r["key_2p21p1_00"]})
        elif c == "knn_u64":
            t = 3 * r["coord"] ** 2
            verdicts.append({"case": c, "coord": r["coord"], "truth": t, "truth_bits": t.bit_length(),
                             "observed": r["kth_distance_k2"], "correct": t == r["kth_distance_k2"]})
    # verdicts attendus (codes par cas)
    expect = []
    for v in verdicts:
        if v["case"] == "level4_regular_tetra":
            expect.append(v["correct"] == (v["B"] <= 20))
            if v["B"] == 24:
                expect.append(v["num_zero"])
            if v["B"] >= 21:
                expect.append(v["den_truncated"] and v["den_mod_2p128"])
        elif v["case"] == "level_at_most_port":
            expect.append((not v["naive_correct"]) and v["wide_correct"])
        elif v["case"] == "level_at_most_b24_tetra":
            expect.append(v["level_equals_3M2_over_4"] and (not v["naive_correct"]) and v["wide_correct"])
        elif v["case"] == "side_q3_equilateral":
            expect.append(v["correct"] == (v["B"] <= 20))
        elif v["case"] == "morton64_mask":
            expect.append(v["collision_000_vs_2p21"] and v["collision_100_vs_2p21p1"])
        elif v["case"] == "knn_u64":
            expect.append(v["correct"] == (v["coord"] < (1 << 32) - 1))
    ok = all(expect) and len(expect) >= 12
    print(json.dumps({"status": "EXPECTED_FAILURES_CONFIRMED" if ok else "UNEXPECTED", "checks": len(expect),
                      "verdicts": verdicts}, indent=1))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
