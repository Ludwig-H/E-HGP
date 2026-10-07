#!/usr/bin/env python3
"""Controle numerique adverse des bornes F6 proposees pour les noyaux filtres du generateur v11.

Ce n'est PAS une preuve : c'est un temoin que les expositions E, les majorants M et les seuils tau ecrits dans
CONCEPTION_GENERATEUR.md ne sont pas contredits. Modele d'arrondi : chaque operation elementaire (+, -, x, et la
multiplication-addition contractee) rend l'un des deux binaire64 qui encadrent le resultat exact, choisi au hasard
(ce qui couvre les quatre modes d'arrondi, changeant d'une operation a l'autre) ; les sommes sont evaluees dans un
ordre et un parenthesage tires au hasard ; la contraction est tiree au hasard. Les majorants M sont evalues en
arrondissant toujours vers le bas (cas le plus defavorable pour un seuil).
Verifie pour chaque valeur filtree : |valeur approchee - valeur exacte| <= tau, donc |approchee| > tau => signe exact.
"""
import math
import random
import sys
from fractions import Fraction

rng = random.Random(20261002)
INF = float("inf")


def grid(x, mode):
    """Arrondi de l'exact x (Fraction) sur la grille binaire64 ; mode : 'rand', 'down', 'near'."""
    if x == 0:
        return Fraction(0)
    d = float(x)  # au plus proche
    fd = Fraction(d)
    if fd == x:
        return fd
    if mode == "near":
        return fd
    lo, hi = (d, math.nextafter(d, INF)) if fd < x else (math.nextafter(d, -INF), d)
    if mode == "down":  # vers zero en valeur absolue (majorants positifs : vers le bas)
        return Fraction(lo if x > 0 else hi)
    return Fraction(rng.choice((lo, hi)))


def op(x, mode="rand"):
    return grid(x, mode)


def add(a, b):
    return op(a + b)


def sub(a, b):
    return op(a - b)


def mul(a, b):
    return op(a * b)


def muladd(a, b, c):
    """a b + c, contracte ou non au hasard."""
    if rng.random() < 0.5:
        return op(a * b + c)
    return add(mul(a, b), c)


def nsum(terms):
    """Somme dans un ordre et un parenthesage au hasard."""
    t = list(terms)
    rng.shuffle(t)
    while len(t) > 1:
        i = rng.randrange(len(t) - 1)
        t[i:i + 2] = [add(t[i], t[i + 1])]
    return t[0]


def dn(x):
    return grid(x, "down")


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


worst = {}


def check(name, approx, exact, tau):
    err = abs(approx - exact)
    if err > tau:
        print("VIOLATION", name, float(err), float(tau))
        sys.exit(1)
    r = float(err / tau) if tau > 0 else 0.0
    w = worst.setdefault(name, [0, 0.0, 0])
    w[0] += 1
    w[1] = max(w[1], r)
    w[2] += int(abs(approx) <= tau)


def rvec(e, flat=None):
    lim = (1 << e) - 1
    return [rng.randint(-lim, lim) for _ in range(3)]


def kernel_q3(u, v, a, h, zs):
    """Centre q3 dans la boite et recensement q3 ; u, v, a, h, z : entiers exacts (differences et coordonnees locales)."""
    uu, vv = dot(u, u), dot(v, v)
    w = cross(u, v)
    t = [uu * v[k] - vv * u[k] for k in range(3)]  # exact pour e <= 16 (budget_bits.py)
    N_ex = cross(t, w)
    D_ex = 2 * dot(w, w)
    # approche
    N, MN = [], []
    for k in range(3):
        i, j = (k + 1) % 3, (k + 2) % 3
        p1 = Fraction(t[i]) * w[j]
        p2 = Fraction(t[j]) * w[i]
        if rng.random() < 0.5:
            N.append(op(Fraction(t[i]) * w[j] - op(p2)))  # contraction
        else:
            N.append(sub(op(p1), op(p2)))
        MN.append(dn(dn(abs(p1)) + dn(abs(p2))))
    ww = nsum([mul(Fraction(x), Fraction(x)) for x in w])
    D = op(2 * ww)
    for k in range(3):
        g = muladd(Fraction(a[k]), D, N[k])
        Mg = dn(dn(abs(a[k]) * D) + MN[k])
        check("q3 boite, a D + N", g, a[k] * D_ex + N_ex[k], Mg / (1 << 48))
        kx = sub(mul(Fraction(h[k]), D), g)
        Mk = dn(dn(h[k] * D) + Mg)
        check("q3 boite, h D - (a D + N)", kx, h[k] * D_ex - (a[k] * D_ex + N_ex[k]), Mk / (1 << 48))
    for z in zs:
        dd = dot(z, z)
        lhs = mul(D, Fraction(dd))
        rhs = op(2 * nsum([mul(N[k], Fraction(z[k])) for k in range(3)]))
        side = sub(lhs, rhs)
        M = dn(dn(D * dd) + dn(2 * dn(sum(dn(MN[k] * abs(z[k])) for k in range(3)))))
        check("recensement q3", side, D_ex * dd - 2 * dot(N_ex, z), M / (1 << 48))


def kernel_q4(u, v, s, a, h, zs):
    uu, vv, ss = dot(u, u), dot(v, v), dot(s, s)
    uv, us, vs = dot(u, v), dot(u, s), dot(v, s)
    c1, c2, c3 = cross(v, s), cross(s, u), cross(u, v)
    det = dot(u, c1)  # exact pour e <= 16
    sg = 1 if det > 0 else -1
    D_ex = 2 * abs(det)
    N_ex = [sg * (uu * c1[k] + vv * c2[k] + ss * c3[k]) for k in range(3)]
    if det != 0:
        N, MN = [], []
        for k in range(3):
            terms = [mul(Fraction(uu), Fraction(c1[k])), mul(Fraction(vv), Fraction(c2[k])), mul(Fraction(ss), Fraction(c3[k]))]
            N.append(sg * nsum(terms))
            MN.append(dn(sum(dn(Fraction(x) * abs(y)) for x, y in ((uu, c1[k]), (vv, c2[k]), (ss, c3[k])))))
        D = Fraction(D_ex)
        for k in range(3):
            g = muladd(Fraction(a[k]), D, N[k])
            Mg = dn(dn(abs(a[k]) * D) + MN[k])
            check("q4 boite, a D + N", g, a[k] * D_ex + N_ex[k], Mg / (1 << 48))
            kx = sub(mul(Fraction(h[k]), D), g)
            Mk = dn(dn(h[k] * D) + Mg)
            check("q4 boite, h D - (a D + N)", kx, h[k] * D_ex - (a[k] * D_ex + N_ex[k]), Mk / (1 << 48))
        for z in zs:
            dd = dot(z, z)
            lhs = mul(D, Fraction(dd))
            rhs = op(2 * nsum([mul(N[k], Fraction(z[k])) for k in range(3)]))
            side = sub(lhs, rhs)
            M = dn(dn(D * dd) + dn(2 * dn(sum(dn(MN[k] * abs(z[k])) for k in range(3)))))
            check("recensement q4", side, D_ex * dd - 2 * dot(N_ex, z), M / (1 << 48))
    # interieur strict, forme retenue (reemploi du centre de Cramer) : A' = N'.c1, B' = N'.c2, C' = N'.c3,
    # interieur <=> A', B', C' > 0 et A' + B' + C' < 2 det^2. Seuils : 2^-48 M pour A', B', C' ; 2^-47 M pour le dernier.
    F = Fraction
    Np, MNp = [], []
    for k in range(3):
        Np.append(nsum([mul(F(uu), F(c1[k])), mul(F(vv), F(c2[k])), mul(F(ss), F(c3[k]))]))
        MNp.append(dn(sum(dn(F(x) * abs(y)) for x, y in ((uu, c1[k]), (vv, c2[k]), (ss, c3[k])))))
    Nx = [uu * c1[k] + vv * c2[k] + ss * c3[k] for k in range(3)]
    vals, mags, exs = [], [], []
    for cj in (c1, c2, c3):
        vals.append(nsum([mul(Np[k], F(cj[k])) for k in range(3)]))
        mags.append(dn(sum(dn(MNp[k] * abs(cj[k])) for k in range(3))))
        exs.append(dot(Nx, cj))
    for nm, vv_, mm_, ex_ in zip(("A'", "B'", "C'"), vals, mags, exs):
        check("interieur q4 (Cramer), " + nm, vv_, ex_, mm_ / (1 << 48))
    T2 = op(2 * mul(F(det), F(det)))
    Lc = nsum([T2, -vals[0], -vals[1], -vals[2]])
    ML = dn(dn(mags[0] + mags[1]) + dn(mags[2] + T2))
    check("interieur q4 (Cramer), 2 det^2 - A' - B' - C'", Lc, 2 * det * det - sum(exs), ML / (1 << 47))
    if exs[0] != (vv * ss - vs * vs) * uu + (vs * us - uv * ss) * vv + (uv * vs - vv * us) * ss:
        print("IDENTITE FAUSSE : A' != A de Gram")
        sys.exit(1)
    # interieur strict (Gram), forme de l'audit L05 et de J3 : temoin de comparaison

    def minor(p, q, r, t):
        if rng.random() < 0.5:
            return op(F(p) * q - mul(F(r), F(t)))
        return sub(mul(F(p), F(q)), mul(F(r), F(t)))

    g11, g22, g33 = minor(vv, ss, vs, vs), minor(uu, ss, us, us), minor(uu, vv, uv, uv)
    g12, g13, g23 = minor(vs, us, uv, ss), minor(uv, vs, vv, us), minor(us, uv, uu, vs)
    e11, e22, e33 = vv * ss - vs * vs, uu * ss - us * us, uu * vv - uv * uv
    e12, e13, e23 = vs * us - uv * ss, uv * vs - vv * us, us * uv - uu * vs
    A = nsum([mul(g11, F(uu)), mul(g12, F(vv)), mul(g13, F(ss))])
    B = nsum([mul(g12, F(uu)), mul(g22, F(vv)), mul(g23, F(ss))])
    C = nsum([mul(g13, F(uu)), mul(g23, F(vv)), mul(g33, F(ss))])
    dG = nsum([mul(F(uu), g11), mul(F(uv), g12), mul(F(us), g13)])
    D2 = op(2 * dG)
    L0 = nsum([D2, -A, -B, -C])
    Aex, Bex, Cex = e11 * uu + e12 * vv + e13 * ss, e12 * uu + e22 * vv + e23 * ss, e13 * uu + e23 * vv + e33 * ss
    L0ex = 2 * (uu * e11 + uv * e12 + us * e13) - Aex - Bex - Cex
    m = max(uu, vv, ss)
    tau = dn(dn(F(m) * m) * m) / (1 << 42)
    check("interieur q4, A", A, Aex, tau)
    check("interieur q4, B", B, Bex, tau)
    check("interieur q4, C", C, Cex, tau)
    check("interieur q4, 2 det G - A - B - C", L0, L0ex, tau)


def near_flat(e):
    """Tetraedre presque plat : s dans le plan de u, v a quelques unites pres."""
    u, v = rvec(e), rvec(e)
    al, be = rng.randint(-3, 3), rng.randint(-3, 3)
    s = [al * u[k] // 4 + be * v[k] // 4 + rng.randint(-2, 2) for k in range(3)]
    lim = (1 << e) - 1
    return u, v, [max(-lim, min(lim, x)) for x in s]


def main(n):
    for it in range(n):
        e = rng.choice((10, 11, 12, 13, 14, 15, 16))
        lim = (1 << e) - 1
        a = [rng.randint(-lim, lim) for _ in range(3)]
        h = [rng.randint(1, lim) for _ in range(3)]
        zs = [rvec(e) for _ in range(3)]
        if it % 3 == 0:
            u, v, s = near_flat(e)
        elif it % 3 == 1:
            u, v, s = rvec(e), rvec(e), rvec(e)
        else:  # petits uplets dans une grande feuille, et grandes valeurs extremes
            sm = rng.choice((2, 4, 8, e))
            u, v, s = rvec(sm), rvec(sm), rvec(sm)
            if rng.random() < 0.3:
                u = [rng.choice((-lim, lim)) for _ in range(3)]
        if any(cross(u, v)):
            # sites du recensement : au hasard, et sur la sphere a une unite pres (les trois generateurs y sont)
            kernel_q3(u, v, a, h, zs + [u, v, [u[0] + 1, u[1], u[2]], [0, 0, 0]])
        kernel_q4(u, v, s, a, h, zs + [u, v, s, [s[0], s[1] - 1, s[2]], [0, 0, 0]])
    print("controles sans violation :")
    for name, (cnt, r, unc) in sorted(worst.items()):
        print(f"  {name:38s} cas {cnt:7d}  max |erreur| / tau = {r:.3e}  indecis (|approchee| <= tau) {unc}")


if __name__ == "__main__":
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 3000)
