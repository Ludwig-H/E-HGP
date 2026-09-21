#!/usr/bin/env python3
"""Fixtures d'égalité exactes pour les bornes de bloc q3 (enveloppe de centres, bornes de puissance) de la voie float32.

Tout est calculé en rationnels exacts (`fractions.Fraction`), sans flottant. Trois schémas d'enveloppe de centres sont
reproduits sans arrondi : hull(a,b,X) ∩ (a + W/(2G)) (schéma des sources non suivies `float32_q3_block.cpp`), l'enveloppe
universelle de A (λ ∈ [0, λ_max], λ = DQ/J) et son resserrement ; les bornes de puissance par axe (minimum au sommet
borné, maximum aux extrémités) sont comparées aux bornes « coins seulement », fausses pour le minimum. `run` écrit le
reçu `BLOCK_ENVELOPE_FIXTURES.json` (valeurs attendues en chaînes exactes) ; `read` recalcule tout et exige l'égalité.
Aucun assert : tient sous python3 -O.
"""
from __future__ import annotations

import argparse
from fractions import Fraction as Fr
from itertools import product
import json
from pathlib import Path
import random
import sys

HERE = Path(__file__).resolve().parent
OUTPUT = HERE / "BLOCK_ENVELOPE_FIXTURES.json"
SCHEMA = "audit_b_q3_block_envelope_fixtures_v1"


class Failure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


# --- vecteurs exacts ---------------------------------------------------------------------------------------------
def sub(p, q):
    return tuple(x - y for x, y in zip(p, q))


def dot(p, q):
    return sum(x * y for x, y in zip(p, q))


def cross(p, q):
    return (p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0])


# --- intervalles exacts (lo, hi) ---------------------------------------------------------------------------------
def I(x):
    return (Fr(x), Fr(x))


def iadd(p, q):
    return (p[0] + q[0], p[1] + q[1])


def isub(p, q):
    return (p[0] - q[1], p[1] - q[0])


def imul(p, q):
    v = [p[0] * q[0], p[0] * q[1], p[1] * q[0], p[1] * q[1]]
    return (min(v), max(v))


def isq(p):
    big = max(abs(p[0]), abs(p[1]))
    small = Fr(0) if p[0] <= 0 <= p[1] else min(abs(p[0]), abs(p[1]))
    return (small * small, big * big)


def idivpos(n, d):
    lo = n[0] / (d[0] if n[0] < 0 else d[1])
    hi = n[1] / (d[1] if n[1] < 0 else d[0])
    return (lo, hi)


def width(box):
    return tuple(b[1] - b[0] for b in box)


# --- géométrie d'une seed ----------------------------------------------------------------------------------------
def seed(a, b, x):
    """Centre circonscrit dans le plan, coordonnées barycentriques (α en a, β en b, ξ en x), acuité, propriété d'arête."""
    d, u = sub(b, a), sub(x, a)
    D, E, F = dot(d, d), dot(u, u), dot(d, u)
    G = D * E - F * F
    if G == 0:
        return None
    W = tuple(E * (D - F) * d[i] + D * (E - F) * u[i] for i in range(3))
    c = tuple(a[i] + Fr(W[i], 2 * G) for i in range(3))
    bary = (Fr(F * dot(sub(x, b), sub(x, b)), 2 * G), Fr(E * (D - F), 2 * G), Fr(D * (E - F), 2 * G))
    acute = F > 0 and D - F > 0 and E - F > 0
    owned = E <= D and dot(sub(x, b), sub(x, b)) <= D
    return dict(c=c, bary=bary, acute=acute, owned=owned, D=D, E=E, F=F, G=G, W=W)


def power(z, c, a):
    return dot(sub(z, c), sub(z, c)) - dot(sub(a, c), sub(a, c))


def hull_box(a, b, X):
    return tuple((min(a[i], b[i], X[i][0]), max(a[i], b[i], X[i][1])) for i in range(3))


def constructor_box(a, b, X):
    """Schéma de Float32Q3Block::make (sources non suivies) en rationnels exacts : hull, puis a + W/(2G) si G.low > 0."""
    hull = hull_box(a, b, X)
    av, bv = [I(a[i]) for i in range(3)], [I(b[i]) for i in range(3)]
    xv = [(Fr(X[i][0]), Fr(X[i][1])) for i in range(3)]
    d, u = [isub(bv[i], av[i]) for i in range(3)], [isub(xv[i], av[i]) for i in range(3)]
    cr = [isub(imul(d[1], u[2]), imul(d[2], u[1])), isub(imul(d[2], u[0]), imul(d[0], u[2])), isub(imul(d[0], u[1]), imul(d[1], u[0]))]
    gram = iadd(iadd(isq(cr[0]), isq(cr[1])), isq(cr[2]))
    if gram[0] <= 0:
        return hull, "gram_unresolved_hull"
    dd, uu = iadd(iadd(isq(d[0]), isq(d[1])), isq(d[2])), iadd(iadd(isq(u[0]), isq(u[1])), isq(u[2]))
    du = iadd(iadd(imul(d[0], u[0]), imul(d[1], u[1])), imul(d[2], u[2]))
    along_d, along_u = imul(uu, isub(dd, du)), imul(dd, isub(uu, du))
    den = iadd(gram, gram)
    out = []
    for i in range(3):
        lin = iadd(imul(along_d, d[i]), imul(along_u, u[i]))
        cand = iadd(av[i], idivpos(lin, den))
        out.append((max(hull[i][0], cand[0]), min(hull[i][1], cand[1])))
    if any(o[0] > o[1] for o in out):
        return hull, "empty_intersection_hull"
    return tuple(out), "tightened"


def a_box(a, b, X, lam_max, refined):
    """Enveloppe de A (README q3_seed_block_power) sans les conditions entières u16 : c = m + λ·P/(4D), λ ∈ [0, λ_max]."""
    d = sub(b, a)
    D = dot(d, d)
    w = [(2 * X[i][0] - a[i] - b[i], 2 * X[i][1] - a[i] - b[i]) for i in range(3)]

    def lin(coefs, spans):
        r = (Fr(0), Fr(0))
        for c, s in zip(coefs, spans):
            r = iadd(r, imul(I(c), s))
        return r

    ws = (sum(isq(s)[0] for s in w), sum(isq(s)[1] for s in w))
    qx = (ws[0] - D, ws[1] - D)
    t = lin(d, w)
    P = [lin([(D if i == j else 0) - d[i] * d[j] for j in range(3)], w) for i in range(3)]
    comps = [lin((0, -d[2], d[1]), w), lin((d[2], 0, -d[0]), w), lin((-d[1], d[0], 0), w)]
    J = (sum(isq(s)[0] for s in comps), sum(isq(s)[1] for s in comps))
    qlo, qhi = max(Fr(0), qx[0]), min(Fr(2 * D) if lam_max == Fr(2, 3) else qx[1], qx[1])
    tlo, thi = max(Fr(-D), t[0]), min(Fr(D), t[1])
    if qlo > qhi or tlo > thi:
        return None, "empty"
    low, high, used = Fr(0), lam_max, False
    if refined and J[0] > 0:
        used = True
        ts = isq((tlo, thi))
        rlo, rhi = D * D - ts[1], D * D - ts[0]
        low = max(low, Fr(D * qlo, J[1]), Fr(D * qlo, D * qlo + rhi) if D * qlo + rhi > 0 else Fr(0))
        high = min(high, Fr(D * qhi, J[0]), Fr(D * qhi, D * qhi + rlo) if D * qhi + rlo > 0 else high)
        if high < low:
            return None, "empty"
    m = [Fr(a[i] + b[i], 2) for i in range(3)]
    out = []
    for i in range(3):
        B = imul((low, high), P[i])
        out.append((m[i] + B[0] / (4 * D), m[i] + B[1] / (4 * D)))
    return tuple(out), ("refined" if used else "universal")


def true_hull(a, b, X, need_owned):
    centres = []
    for x in product(*(range(X[i][0], X[i][1] + 1) for i in range(3))):
        s = seed(a, b, x)
        if s is not None and s["acute"] and (s["owned"] or not need_owned):
            centres.append(s["c"])
    if not centres:
        return None, 0
    return tuple((min(c[i] for c in centres), max(c[i] for c in centres)) for i in range(3)), len(centres)


def bounds_vertex(a, C, Z):
    """Bornes du code : par axe et par extrémité de C_i, minimum en z = clamp(c, Z_i), maximum aux extrémités de Z_i."""
    lo = hi = Fr(0)
    for i in range(3):
        zl, zh = Fr(Z[i][0]), Fr(Z[i][1])
        axis_lo = axis_hi = None
        for c in (C[i][0], C[i][1]):
            g = lambda z, c=c: (z - a[i]) * (z + a[i] - 2 * c)
            v = min(max(c, zl), zh)
            mn, mx = g(v), max(g(zl), g(zh))
            axis_lo = mn if axis_lo is None else min(axis_lo, mn)
            axis_hi = mx if axis_hi is None else max(axis_hi, mx)
        lo, hi = lo + axis_lo, hi + axis_hi
    return lo, hi


def bounds_corners_only(a, C, Z):
    lo = hi = None
    for cz in product(*[(Fr(Z[i][0]), Fr(Z[i][1])) for i in range(3)]):
        for cc in product(*[(C[i][0], C[i][1]) for i in range(3)]):
            v = sum((cz[i] - a[i]) * (cz[i] + a[i] - 2 * cc[i]) for i in range(3))
            lo = v if lo is None else min(lo, v)
            hi = v if hi is None else max(hi, v)
    return lo, hi


def true_power_range(a, b, X, Z, owned):
    values = []
    for x in product(*(range(X[i][0], X[i][1] + 1) for i in range(3))):
        s = seed(a, b, x)
        if s is None or not s["acute"] or (owned and not s["owned"]):
            continue
        for z in product(*(range(Z[i][0], Z[i][1] + 1) for i in range(3))):
            values.append(power(z, s["c"], a))
    return (min(values), max(values)) if values else None


def decision(lo, hi):
    return "inside" if hi < 0 else ("outside" if lo >= 0 else "undecided")


# --- sérialisation exacte ----------------------------------------------------------------------------------------
def ser(v):
    if isinstance(v, Fr):
        return str(v)
    if isinstance(v, bool) or v is None or isinstance(v, str):
        return v
    if isinstance(v, int):
        return str(v)
    if isinstance(v, (tuple, list)):
        return [ser(x) for x in v]
    if isinstance(v, dict):
        return {k: ser(x) for k, x in v.items()}
    raise TypeError(type(v))


def rot(p):
    return tuple(500 + dot(r, p) for r in ((2, -2, 1), (1, 2, 2), (-2, -1, 2)))


def envelopes(a, b, X, owned_lane):
    lam = Fr(2, 3) if owned_lane else Fr(1)
    th, count = true_hull(a, b, X, owned_lane)
    cb, cs = constructor_box(a, b, X)
    ub, _ = a_box(a, b, X, lam, False)
    rb, rs = a_box(a, b, X, lam, True)
    boxes = dict(hull=hull_box(a, b, X), constructor=cb, a_universal=ub, a_refined=rb, true_hull=th)
    inclusion = {k: (th is None or v is None or all(v[i][0] <= th[i][0] and th[i][1] <= v[i][1] for i in range(3))) for k, v in boxes.items() if k != "true_hull"}
    return dict(a=a, b=b, X=X, owned_lane=owned_lane, valid_seeds=count, constructor_status=cs, refined_status=rs, boxes=boxes,
                widths={k: (width(v) if v else None) for k, v in boxes.items()}, inclusion=inclusion)


def with_bounds(env, Z, owned_lane):
    a, b, X = env["a"], env["b"], env["X"]
    out = dict(Z=Z, true_range=true_power_range(a, b, X, Z, owned_lane), bounds={})
    for k, C in env["boxes"].items():
        if C is None:
            continue
        lo, hi = bounds_vertex(a, C, Z)
        out["bounds"][k] = dict(lo=lo, hi=hi, decision=decision(lo, hi))
    return out


def compute():
    fx = {}
    # F1 : minimum au sommet, pas aux coins.
    a, b, x = (0, 0, 0), (10, 0, 0), (5, 6, 0)
    s = seed(a, b, x)
    C = tuple((ci, ci) for ci in s["c"])
    Z = ((0, 10), (0, 0), (0, 0))
    fx["f1_vertex_minimum"] = dict(a=a, b=b, x=x, centre=s["c"], acute=s["acute"], Z=Z, corners_only=bounds_corners_only(a, C, Z),
                                   vertex=bounds_vertex(a, C, Z), power_at_5_0_0=power((5, 0, 0), s["c"], a),
                                   claim="corners_only min 0 would deny the strict witness z=(5,0,0) of power -25")
    # F2 : fixture axiale de A, Z = {(30,31,20)}.
    a, b, X = (20, 20, 20), (40, 20, 20), ((30, 30), (32, 37), (20, 20))
    env = envelopes(a, b, X, True)
    fx["f2_axial_owned"] = dict(**env, query=with_bounds(env, ((30, 30), (31, 31), (20, 20)), True))
    env = envelopes(a, b, X, False)
    fx["f2_axial_unowned_lane"] = dict(**env, query=with_bounds(env, ((30, 30), (31, 31), (20, 20)), False))
    # F3 : la même fixture tournée (+500), quatre boîtes Z de 27 points.
    ra, rb = rot(a), rot(b)
    pts = [rot((30, y, 20)) for y in range(32, 38)]
    RX = tuple((min(p[i] for p in pts), max(p[i] for p in pts)) for i in range(3))
    env = envelopes(ra, rb, RX, True)
    queries = []
    for z0 in (rot((30, 31, 20)), rot((30, 40, 20)), rot((25, 25, 20)), rot((30, 34, 20))):
        queries.append(with_bounds(env, tuple((z0[i] - 1, z0[i] + 1) for i in range(3)), True))
    fx["f3_rotated_owned"] = dict(**env, queries=queries)
    # F4 : λ sans propriété d'arête (sup 1), avec propriété (2/3 à l'équilatéral) ; coordonnées u16 (translation).
    lam = {}
    for name, (a4, b4, x4) in dict(unowned_24_25=((0, 0, 0), (4, 0, 0), (2, 10, 0)), unowned_5304_5305=((0, 60, 60), (2, 60, 60), (1, 9, 8)),
                                    bisector_n10000=((0, 0, 0), (2, 0, 0), (1, 10000, 0)), equilateral=((30, 30, 30), (36, 36, 30), (36, 30, 36))).items():
        s = seed(a4, b4, x4)
        lam[name] = dict(a=a4, b=b4, x=x4, acute=s["acute"], owned=s["owned"], xi=s["bary"][2], lam=2 * s["bary"][2], bary=s["bary"])
    fx["f4_lambda"] = lam
    # F5 : G ambigu ou petit : repli hull du constructeur, enveloppe universelle finie.
    fx["f5_gram_fallback"] = [envelopes(a, b, ((30, 30), (y_lo, 40), (20, 20)), False) for y_lo in (36, 30, 21, 20)]
    # F6 : deux graines valides témoins l'une de l'autre (a=(0,0,0), b=(4,0,0), x1=(2,3,0), x2=(2,20,0)).
    a6, b6, x1, x2 = (0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 20, 0)
    s1, s2 = seed(a6, b6, x1), seed(a6, b6, x2)
    fx["f6_mutual_witnesses"] = dict(a=a6, b=b6, x1=x1, x2=x2, acute=(s1["acute"], s2["acute"]), power_x1_in_ball_x2=power(x1, s2["c"], a6),
                                     power_x1_in_own_ball=power(x1, s1["c"], a6), power_x2_in_ball_x1=power(x2, s1["c"], a6))
    # F7 : barycentriques et position du centre (intérieur strict ⟺ F, D−F, E−F > 0).
    bary = {}
    for name, (a7, b7, x7) in dict(equilateral=((30, 30, 30), (36, 36, 30), (36, 30, 36)), right_at_x=((0, 0, 0), (10, 0, 0), (2, 4, 0)),
                                    right_at_a=((0, 0, 0), (10, 0, 0), (0, 4, 0)), obtuse_at_x=((0, 0, 0), (10, 0, 0), (5, 2, 0)),
                                    obtuse_at_b=((0, 0, 0), (10, 0, 0), (12, 3, 0))).items():
        s = seed(a7, b7, x7)
        hull = hull_box(a7, b7, tuple((x7[i], x7[i]) for i in range(3)))
        bary[name] = dict(a=a7, b=b7, x=x7, bary=s["bary"], bary_sum=sum(s["bary"]), acute=s["acute"], centre=s["c"],
                          centre_in_hull=all(hull[i][0] <= s["c"][i] <= hull[i][1] for i in range(3)),
                          signs=(s["F"] > 0, s["D"] - s["F"] > 0, s["E"] - s["F"] > 0))
    fx["f7_barycentric"] = bary
    # F8 : identités sur 300 triangles entiers tirés (graine 21) : J = 4G, Q = 4(E−F), P = 2(Du−Fd), c = m + ξ·h(x), λ = 2ξ, (c−m)·d = 0.
    rng = random.Random(21)
    checked = 0
    while checked < 300:
        a8, b8, x8 = (tuple(rng.randrange(0, 200) for _ in range(3)) for _ in range(3))
        s = seed(a8, b8, x8)
        if s is None:
            continue
        d, u, w = sub(b8, a8), sub(x8, a8), tuple(2 * x8[i] - a8[i] - b8[i] for i in range(3))
        D, E, F, G = s["D"], s["E"], s["F"], s["G"]
        J = D * dot(w, w) - dot(d, w) ** 2
        Q = dot(w, w) - D
        P = tuple(D * w[i] - dot(d, w) * d[i] for i in range(3))
        m = tuple(Fr(a8[i] + b8[i], 2) for i in range(3))
        h = tuple(u[i] - Fr(F, D) * d[i] for i in range(3))
        xi = s["bary"][2]
        require(J == 4 * G and Q == 4 * (E - F) and P == tuple(2 * (D * u[i] - F * d[i]) for i in range(3)), "identités J, Q, P")
        require(all(s["c"][i] == m[i] + xi * h[i] for i in range(3)) and dot(sub(s["c"], m), d) == 0, "c = m + ξ·h(x), (c−m)·d = 0")
        require(Fr(D * Q, J) == 2 * xi and sum(s["bary"]) == 1, "λ = DQ/J = 2ξ, somme des barycentriques 1")
        checked += 1
    fx["f8_identities"] = dict(triangles=checked, seed=21, coordinate_range=[0, 200])
    return fx


def checks(fx):
    """Vérités attendues, recalculées à chaque lecture (indépendantes du reçu)."""
    f1 = fx["f1_vertex_minimum"]
    require(f1["acute"] and f1["corners_only"] == (Fr(0), Fr(0)) and f1["vertex"] == (Fr(-25), Fr(0)) and f1["power_at_5_0_0"] == -25, "F1")
    f2 = fx["f2_axial_owned"]
    require(all(f2["inclusion"].values()) and f2["constructor_status"] == "tightened", "F2 inclusion")
    q = f2["query"]["bounds"]
    require(q["constructor"]["decision"] == "undecided" and q["a_universal"]["decision"] == "undecided" and q["a_refined"]["decision"] == "inside"
            and q["a_refined"]["lo"] == Fr(-1722, 17) and q["a_refined"]["hi"] == Fr(-58, 3), "F2 decisions")
    require(f2["query"]["true_range"] == (q["a_refined"]["lo"], q["a_refined"]["hi"]) and f2["boxes"]["a_refined"] == f2["boxes"]["true_hull"], "F2 refined envelope equals the true hull")
    require(all(f2["widths"]["constructor"][i] >= f2["widths"]["a_refined"][i] for i in range(3)), "F2 constructor wider than refined")
    f3 = fx["f3_rotated_owned"]
    require(all(f3["inclusion"].values()), "F3 inclusion")
    for query in f3["queries"]:
        tr = query["true_range"]
        for k, bd in query["bounds"].items():
            require(bd["lo"] <= tr[0] and tr[1] <= bd["hi"], f"F3 bounds {k} enclose the true range")
    lam = fx["f4_lambda"]
    require(lam["unowned_24_25"]["lam"] == Fr(24, 25) and not lam["unowned_24_25"]["owned"] and lam["unowned_24_25"]["acute"], "F4 24/25")
    require(lam["unowned_5304_5305"]["lam"] == Fr(5304, 5305) and lam["bisector_n10000"]["lam"] == 1 - Fr(1, 10000 ** 2), "F4 near 1")
    require(lam["equilateral"]["lam"] == Fr(2, 3) and lam["equilateral"]["owned"] and lam["equilateral"]["bary"] == (Fr(1, 3), Fr(1, 3), Fr(1, 3)), "F4 equilateral")
    f5 = fx["f5_gram_fallback"]
    require([e["constructor_status"] for e in f5] == ["tightened", "tightened", "tightened", "gram_unresolved_hull"], "F5 statuses")
    require(f5[3]["boxes"]["constructor"] == f5[3]["boxes"]["hull"] and all(all(e["inclusion"].values()) for e in f5), "F5 hull fallback and inclusion")
    require(all(e["boxes"]["a_universal"][0] == (Fr(30), Fr(30)) and e["boxes"]["a_universal"][1] == (Fr(20), Fr(30)) for e in f5), "F5 universal envelope")
    f6 = fx["f6_mutual_witnesses"]
    require(all(f6["acute"]) and f6["power_x1_in_ball_x2"] < 0 and f6["power_x1_in_own_ball"] == 0 and f6["power_x2_in_ball_x1"] > 0, "F6")
    f7 = fx["f7_barycentric"]
    require(f7["equilateral"]["bary"] == (Fr(1, 3),) * 3 and f7["right_at_x"]["bary"][2] == 0 and f7["right_at_a"]["bary"][0] == 0, "F7 degenerate")
    require(f7["obtuse_at_x"]["bary"][2] < 0 and not f7["obtuse_at_x"]["centre_in_hull"] and f7["obtuse_at_b"]["bary"][1] < 0, "F7 obtuse")
    for e in f7.values():
        require(e["acute"] == all(e["signs"]) and e["bary_sum"] == 1, "F7 acute iff signs")
    require(fx["f8_identities"]["triangles"] == 300, "F8")


def run(args):
    fx = compute()
    checks(fx)
    receipt = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
                   mode="audit_independant_math_and_architecture", public_status="not_claimed", gcp_used=False,
                   scope="exact rational fixtures for q3 block centre envelopes and power bounds (float32 lane); no engine, no timing",
                   fixtures=ser(fx))
    args.output.write_text(json.dumps(receipt, sort_keys=True, indent=1) + "\n")
    print(json.dumps(dict(status="passed", fixtures=len(fx), output=str(args.output))))


def read(args):
    receipt = json.loads(args.output.read_text())
    require(receipt["schema"] == SCHEMA and receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False, "reçu hors cadre")
    fx = compute()
    checks(fx)
    require(ser(fx) == receipt["fixtures"], "valeurs recalculées différentes du reçu")
    print(json.dumps(dict(status="passed", fixtures=len(fx))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub_parsers = parser.add_subparsers(dest="operation", required=True)
    for name in ("run", "read"):
        p = sub_parsers.add_parser(name)
        p.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    try:
        run(args) if args.operation == "run" else read(args)
    except Failure as failure:
        print(json.dumps(dict(status="failed", error=str(failure))))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
