#!/usr/bin/env python3
"""Lentille 2 (auditeur C, v9) : controle exact des formules q2 et de leurs bornes u18.

Reproduit en entiers Python (non bornes) les formules de
  morsehgp3D_v9/src/gen/spindle/predicates.hpp        (h_minimum, h_maximum_times_four, xi_bounds)
  morsehgp3D_v9/src/gen/spindle/q2_prepared_bounds.hpp (Q2PreparedBounds::bounds_unchecked)
  morsehgp3D_v9/src/gen/pipeline/q2_joint_bounds.hpp   (Q2JointPreparedBounds::bounds_unchecked)
  morsehgp3D_v9/src/gen/pipeline/q2_census.cpp:50-93   (ball_key, point_power4, pair_bounds)
et les compare a une force brute exacte (grille entiere pour les minima, demi-grille pour
les maxima continus en z). Calcule aussi les pires magnitudes a M=262143 et verifie qu'elles
tiennent dans i64 (resp. i128). Aucune assertion : sortie 0 si tout concorde, 1 sinon.
"""
import itertools
import random
import sys

M = (1 << 18) - 1
I64 = (1 << 63) - 1
I128 = (1 << 127) - 1
fail = []
checks = 0


def req(cond, msg):
    global checks
    checks += 1
    if not cond:
        fail.append(msg)


# ---------------------------------------------------------------- formules du code
def h_minimum(A, B, Z):
    tot = 0
    for ax in range(3):
        (al, ah), (bl, bh), (zl, zh) = A[ax], B[ax], Z[ax]
        def at_z(v):
            prods = [(v - ah) * (bl - v), (v - ah) * (bh - v), (v - al) * (bl - v), (v - al) * (bh - v)]
            return min(prods)
        tot += min(at_z(zl), at_z(zh))
    return tot


def h_maximum_times_four(A, b, Z):
    tot = 0
    for ax in range(3):
        (al, ah), bc, (zl, zh) = A[ax], b[ax], Z[ax]
        lo2, hi2 = 2 * zl, 2 * zh
        def at_a(ac):
            s = ac + bc
            closest = min(max(s, lo2), hi2)
            d = closest - s
            dist = bc - ac
            return dist * dist - d * d
        tot += max(at_a(al), at_a(ah))
    return tot


def prepared(a, B, Z):  # Q2PreparedBounds(a, B).bounds(Z)
    mn = mx = 0
    for ax in range(3):
        lo2, hi2 = 2 * Z[ax][0], 2 * Z[ax][1]
        amin, amax = None, None
        for e in B[ax]:
            C = a[ax] + e
            D = (e - a[ax]) ** 2
            ld, hd = lo2 - C, hi2 - C
            ls, hs = ld * ld, hd * hd
            near = ls if ld > 0 else (hs if hd < 0 else 0)
            v1 = D - max(ls, hs)
            v2 = D - near
            amin = v1 if amin is None else min(amin, v1)
            amax = v2 if amax is None else max(amax, v2)
        mn += amin
        mx += amax
    return mn, mx


def joint(A, B, Z):  # Q2JointPreparedBounds(A, B).bounds(Z)
    mn = mx = 0
    for ax in range(3):
        lo2, hi2 = 2 * Z[ax][0], 2 * Z[ax][1]
        amin, amax = None, None
        for ae in A[ax]:
            for be in B[ax]:
                C = ae + be
                D = (be - ae) ** 2
                ld, hd = lo2 - C, hi2 - C
                ls, hs = ld * ld, hd * hd
                near = ls if ld > 0 else (hs if hd < 0 else 0)
                v1 = D - max(ls, hs)
                v2 = D - near
                amin = v1 if amin is None else min(amin, v1)
                amax = v2 if amax is None else max(amax, v2)
        mn += amin
        mx += amax
    return mn, mx


def ball_key(a, b):
    return [a[i] + b[i] for i in range(3)], sum((a[i] - b[i]) ** 2 for i in range(3))


def point_power4(key, p):
    c, d2 = key
    return d2 - sum((2 * p[i] - c[i]) ** 2 for i in range(3))


def pair_bounds(key, Z):
    c, d2 = key
    near = far = 0
    for ax in range(3):
        lo, hi = 2 * Z[ax][0], 2 * Z[ax][1]
        n_ = lo - c[ax] if c[ax] < lo else (c[ax] - hi if c[ax] > hi else 0)
        near += n_ * n_
        far += max((lo - c[ax]) ** 2, (hi - c[ax]) ** 2)
    return d2 - far, d2 - near


# ---------------------------------------------------------------- force brute exacte
def H4(a, b, z):
    return 4 * sum((z[i] - a[i]) * (b[i] - z[i]) for i in range(3))


def pts(box):
    return itertools.product(*[range(lo, hi + 1) for lo, hi in box])


def half_pts(box):  # 2z entier : maxima continus d'une forme concave en z
    return itertools.product(*[[k / 2 for k in range(2 * lo, 2 * hi + 1)] for lo, hi in box])


def rand_box(rng, lim, maxw):
    out = []
    for _ in range(3):
        lo = rng.randint(0, lim)
        hi = min(lim, lo + rng.randint(0, maxw))
        out.append((lo, hi))
    return out


def brute_checks(rng, rounds, lim, maxw):
    for _ in range(rounds):
        A, B, Z = rand_box(rng, lim, maxw), rand_box(rng, lim, maxw), rand_box(rng, lim, maxw)
        # h_minimum = min exact sur A x B x Z (entier ; atteint aux sommets)
        bf = min(H4(a, b, z) for a in pts(A) for b in pts(B) for z in pts(Z))
        req(4 * h_minimum(A, B, Z) == bf, f"h_minimum {A} {B} {Z}")
        # joint min/max (max continu : demi-grille en z)
        jm, jM = joint(A, B, Z)
        req(jm == bf, f"joint min {A} {B} {Z}")
        bfM = max(H4(a, b, z) for a in pts(A) for b in pts(B) for z in half_pts(Z))
        req(jM == bfM, f"joint max {A} {B} {Z}")
        # prepared (a fixe)
        a = tuple(rng.randint(lo, hi) for lo, hi in A)
        pm, pM = prepared(a, B, Z)
        req(pm == min(H4(a, b, z) for b in pts(B) for z in pts(Z)), f"prepared min {a} {B} {Z}")
        req(pM == max(H4(a, b, z) for b in pts(B) for z in half_pts(Z)), f"prepared max {a} {B} {Z}")
        # h_maximum_times_four (b fixe) = max continu
        b = tuple(rng.randint(lo, hi) for lo, hi in B)
        req(h_maximum_times_four(A, b, Z) == max(H4(a2, b, z) for a2 in pts(A) for z in half_pts(Z)),
            f"hmax4 {A} {b} {Z}")
        # pair_bounds (a, b fixes) contre force brute
        key = ball_key(a, b)
        km, kM = pair_bounds(key, Z)
        req(km == min(point_power4(key, z) for z in pts(Z)), f"pair min {a} {b} {Z}")
        req(kM == max(point_power4(key, z) for z in half_pts(Z)), f"pair max {a} {b} {Z}")
        # identite 4H = D - |2z - C|^2
        z = tuple(rng.randint(lo, hi) for lo, hi in Z)
        req(point_power4(key, z) == H4(a, b, z), "identite 4H")


# ---------------------------------------------------------------- pires magnitudes a M
def worst_magnitudes():
    rows = []
    # per-axis (z-a)(b-z) in [-M^2, M^2/4]; sums of three
    rows.append(("h_minimum |somme|", 3 * M * M, I64))
    rows.append(("4H prepare/joint/pair |somme|", 12 * M * M, I64))
    rows.append(("carre (2z-C)^2 par axe", (2 * M) ** 2, I64))
    rows.append(("Q2BallKey.center_twice (u32)", 2 * M, (1 << 32) - 1))
    rows.append(("Q2BallKey.diameter_squared", 3 * M * M, I64))
    rows.append(("Xi (3 carres de 2M^2)", 3 * (2 * M * M) ** 2, I128))
    rows.append(("3*H^2 point_witness", 3 * (3 * M * M) ** 2, I128))
    rows.append(("16*Xi.low classify", 16 * 12 * M ** 4, I128))
    rows.append(("3*h_max4^2 classify", 3 * (12 * M * M) ** 2, I128))
    rows.append(("front gap2/diag2", 3 * M * M, I64))
    rows.append(("front s^2*diag2, s=2^32-1", ((1 << 32) - 1) ** 2 * 3 * M * M, I128))
    rows.append(("front midpoint_distance4", 3 * (4 * M) ** 2, I64))
    rows.append(("Pool score |dir.p|", 3 * (2 * M) * M, I64))
    for name, v, lim in rows:
        req(v <= lim, f"borne {name}")
        print(f"  {name:34s} <= {v:.4e}  (2^{v.bit_length()})  limite 2^{lim.bit_length()}")
    # exact extremes of per-axis H at M (u18 corner cases)
    lo = min((z - a) * (b - z) for a in (0, M) for b in (0, M) for z in (0, M))
    req(lo == -M * M, "min par axe -M^2")
    # profondeur d'index : halving floor(e/2) depuis M, 18 pas par axe
    e, steps = M, 0
    while e > 0:
        e //= 2
        steps += 1
    req(steps == 18, "18 halvings par axe")
    print(f"  halvings par axe depuis M : {steps} -> profondeur <= {3 * steps}")


def main():
    rng = random.Random(20260923)
    print("pires magnitudes (M=262143) :")
    worst_magnitudes()
    brute_checks(rng, 700, 6, 3)                 # petites boites, force brute complete
    # boites aux extremes u18 : largeur 1-2, coordonnees pres de 0 et de M
    rng2 = random.Random(7)
    for _ in range(250):
        def ext_box():
            out = []
            for _ in range(3):
                base = rng2.choice([0, 1, M // 2, M - 3, M - 2])
                lo = base
                hi = min(M, lo + rng2.randint(0, 2))
                out.append((lo, hi))
            return out
        A, B, Z = ext_box(), ext_box(), ext_box()
        bf = min(H4(a, b, z) for a in pts(A) for b in pts(B) for z in pts(Z))
        req(4 * h_minimum(A, B, Z) == bf, "h_minimum u18")
        jm, jM = joint(A, B, Z)
        req(jm == bf and jM == max(H4(a, b, z) for a in pts(A) for b in pts(B) for z in half_pts(Z)), "joint u18")
        for v in (jm, jM):
            req(abs(v) <= 12 * M * M, "4H u18 hors borne")
    print(f"controles={checks} echecs={len(fail)}")
    for f in fail[:10]:
        print("ECHEC", f)
    return 0 if not fail else 1


if __name__ == "__main__":
    sys.exit(main())
