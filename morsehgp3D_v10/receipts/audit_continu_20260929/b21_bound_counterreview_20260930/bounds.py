#!/usr/bin/env python3
"""Grand livre independant des bornes du palier B21 (verificateur adversarial, lentille PREUVES).

Chaque majorant est rederive ici par inegalite triangulaire sur l'arbre d'expression du code (HEAD 5dd83b5), en
rationnels exacts, puis compare aux affirmations des commentaires et des notes. Aucune dependance au C++.
Aucune dependance a assert (tient sous python3 -O). Code 0 si toutes les affirmations verifiees tiennent, 1 sinon ;
les constats (preuve qui ne se referme pas telle qu'ecrite) sont listes a part et n'influencent pas le code.
"""
import math
import random
import sys
from fractions import Fraction as F

FAILS, NOTES = [], []
U = 2 ** 53
u = F(1, U)


def out(s=""):
    print(s)


def check(cond, what):
    out(("ok     " if cond else "ECHEC  ") + what)
    if not cond:
        FAILS.append(what)


def note(what):
    out("CONSTAT " + what)
    NOTES.append(what)


def bl(x):
    return abs(int(x)).bit_length()


def blF(x):
    """longueur en bits du plus petit entier >= x (majorant rationnel)."""
    return bl(math.ceil(x))


def gamma(k):
    return k * u / (1 - k * u)


def exact_double(x):
    return F(x)  # Fraction(float) : valeur exacte du double


def half_ulp_bound(T):
    """Borne de |fl(t) - t| pour 0 < t <= T (arrondi au plus proche, pas de sous-normal) : demi-ulp de la binade de T."""
    e = math.floor(math.log2(T))
    while F(2) ** (e + 1) <= T:
        e += 1
    while F(2) ** e > T:
        e -= 1
    return F(2) ** (e - 53)


def filter_margin(bits):
    m = math.ldexp(5.0, 2 * bits - 49)
    return m if m > 0.02 else 0.02


def main():
    out("# Grand livre independant, palier B21")
    out()
    # ------------------------------------------------------------ 1. formes et predicats, par B
    out("## 1. Majorants par B (bits), arbre d'expression du code")
    claims = {  # (B18, B21) affirmes dans l'en-tete de geometry.hpp / notes § 3.2
        "q3 N 24E^5": (95, 110), "q3 D 24E^4": (77, 89), "q4 D 12E^3": (58, 67), "q4 N 18E^4": (77, 89),
        "cote q3 D|d|^2 72E^6": (115, 133), "cote q3 2N.d 144E^6": (116, 134), "cle q3 216E^6": (116, 134),
        "cote q4 D|d|^2 36E^5": (96, 111), "cote q4 2N.d 108E^5": (97, 112),
        "orient cc q4 30E^4": (77, 89), "orient cc q3 48E^5": (96, 111), "orient somme q4 180E^6": (116, 134),
        "niveau q3 num 27E^6": (113, 131), "niveau q3 den 48E^4": (78, 90),
        "niveau q4 num 972E^8": (154, 178), "niveau q4 den 144E^6": (116, 134),
    }
    forms = {
        "q3 N 24E^5": (24, 5), "q3 D 24E^4": (24, 4), "q4 D 12E^3": (12, 3), "q4 N 18E^4": (18, 4),
        "cote q3 D|d|^2 72E^6": (72, 6), "cote q3 2N.d 144E^6": (144, 6), "cle q3 216E^6": (216, 6),
        "cote q4 D|d|^2 36E^5": (36, 5), "cote q4 2N.d 108E^5": (108, 5),
        "orient cc q4 30E^4": (30, 4), "orient cc q3 48E^5": (48, 5), "orient somme q4 180E^6": (180, 6),
        "niveau q3 num 27E^6": (27, 6), "niveau q3 den 48E^4": (48, 4),
        "niveau q4 num 972E^8": (972, 8), "niveau q4 den 144E^6": (144, 6),
    }
    # derivation des coefficients (rederivee, pas lue) :
    #  u, v, s : |.| <= E ; |u x v|_i <= 2E^2 ; |u|^2 <= 3E^2 ; t = uu v - vv u : 6E^3 ; N3 = t x w : 2 (6E^3)(2E^2) = 24E^5
    #  D3 = 2|w|^2 <= 2 * 3 (2E^2)^2 = 24E^4 ; det <= 6E^3 -> D4 = 12E^3 ; N4_i = uu(vxs)_i + vv(sxu)_i + ss(uxv)_i <= 3*3E^2*2E^2
    #  cote : D |d|^2 <= D 3E^2 ; 2 N.d <= 2 * 3 N E ; cle <= somme ; cc = N + D (a - p) : N + D E
    #  orient somme <= 3 (2E^2)(cc) ; niveaux q3 num = uu vv dd <= 27E^6, den = 4|w|^2 <= 48E^4 ; q4 num = |N|^2 <= 3 (18E^4)^2
    rederive = {
        "q3 N 24E^5": 2 * 6 * 2, "q3 D 24E^4": 2 * 3 * 4, "q4 D 12E^3": 2 * 6, "q4 N 18E^4": 3 * 3 * 2,
        "cote q3 D|d|^2 72E^6": 24 * 3, "cote q3 2N.d 144E^6": 2 * 3 * 24, "cle q3 216E^6": 72 + 144,
        "cote q4 D|d|^2 36E^5": 12 * 3, "cote q4 2N.d 108E^5": 2 * 3 * 18,
        "orient cc q4 30E^4": 18 + 12, "orient cc q3 48E^5": 24 + 24, "orient somme q4 180E^6": 3 * 2 * 30,
        "niveau q3 num 27E^6": 27, "niveau q3 den 48E^4": 4 * 3 * 4,
        "niveau q4 num 972E^8": 3 * 18 * 18, "niveau q4 den 144E^6": 144,
    }
    for k, (coef, p) in forms.items():
        check(rederive[k] == coef, f"coefficient rederive {k} : {rederive[k]}")
        b18 = bl(coef * (2 ** 18 - 1) ** p)
        b21 = bl(coef * (2 ** 21 - 1) ** p)
        check((b18, b21) == claims[k], f"{k} : bits B18/B21 = {b18}/{b21} (affirme {claims[k]})")
    # widths of the level types and comparators
    out()
    for B in (21, 22, 23):
        E = 2 ** B - 1
        out(f"B{B} : q4 num {bl(972 * E ** 8)} bits, q4 den {bl(144 * E ** 6)}, q3 num {bl(27 * E ** 6)}, "
            f"q3 den {bl(48 * E ** 4)}")
    check(bl(972 * (2 ** 22 - 1) ** 8) <= 192 and bl(144 * (2 ** 22 - 1) ** 6) <= 192,
          "num et den q4 <= 192 bits jusqu'a B22 (garde level4 jamais declenchee a B <= 21)")
    check(bl(972 * (2 ** 23 - 1) ** 8) > 192, "B23 : majorant num q4 > 192 bits (garde utile, affirme 194)")
    check(bl(972 * (2 ** 23 - 1) ** 8) == 194, "B23 : majorant num q4 = 194 bits")
    E21 = 2 ** 21 - 1
    check(bl(972 * E21 ** 8) + bl(144 * E21 ** 6) <= 384, "compare : num den' < 2^384 (Wide<6>)")
    check(bl(3 * E21 ** 2) <= 64 and bl(3 * E21 ** 2) + bl(144 * E21 ** 6) <= 256,
          "level_at_most : e (K-NN, <= 3E^2 < 2^64) fois den I192 dans Wide<4>")
    check(bl(3 * E21 ** 2) == 44, "K-NN : e <= 3 E^2 sur 44 bits a B21 (u64, exact en double)")
    # ------------------------------------------------------------ 2. conditions de voie courte (preuve symbolique)
    out()
    out("## 2. Conditions de voie courte")
    # cote : |d_i| < 2^bd ; |d|^2 <= 3 (2^bd - 1)^2 < 2^(2bd+2) ; lhs < 2^(bD + 2bd + 2) ; |N_i d_i| < 2^(bN+bd) ;
    # |N.d| < 3 2^(bN+bd) < 2^(bN+bd+2) ; rhs < 2^(bN+bd+3). Verification exhaustive sur les triplets (bD, bN, bd).
    worst_side = 0
    ok = True
    for bd in range(0, 64):
        for bD in range(0, 128):
            if bD + 2 * bd + 2 > 125:
                continue
            for bN in range(0, 128):
                if bN + bd + 3 > 125:
                    continue
                Dm, Nm, dm = 2 ** bD - 1, 2 ** bN - 1, 2 ** bd - 1
                lhs = Dm * 3 * dm * dm
                partial = [dm * dm, 2 * dm * dm, 3 * dm * dm]  # sommes partielles de |d|^2
                rhs_parts = [Nm * dm, 2 * Nm * dm, 3 * Nm * dm, 6 * Nm * dm]
                mx = max([lhs] + partial + rhs_parts + [lhs + 6 * Nm * dm])
                worst_side = max(worst_side, mx.bit_length())
                if mx >= 2 ** 127:
                    ok = False
    check(ok, f"cote : toute quantite intermediaire < 2^127 sous la condition (pire : {worst_side} bits)")
    # orientation : |cc_i| < 2^bD+bap + 2^bN ; produit w cc < 2^(bw + bcc) <= 2^123 ; somme < 3 2^123
    check(2 ** 125 + 2 ** 125 <= 2 ** 127 - 1 + 1 and 3 * 2 ** 123 < 2 ** 127,
          "orientation : |cc| < 2^126, somme de trois produits < 3 2^123 < 2^127")
    # ------------------------------------------------------------ 3. drapeau de feuille E < 2^19
    out()
    out("## 3. Drapeau de feuille (etendue E de la liste < 2^19)")
    for E in (2 ** 19 - 1, 2 ** 19, 2 ** 20 - 1):
        bd = bl(E)
        bD3, bN3 = bl(24 * E ** 4), bl(24 * E ** 5)
        bD4, bN4 = bl(12 * E ** 3), bl(18 * E ** 4)
        bw, bcc4 = bl(2 * E ** 2), bl(30 * E ** 4)
        side3 = (bD3 + 2 * bd + 2, bN3 + bd + 3)
        side4 = (bD4 + 2 * bd + 2, bN4 + bd + 3)
        side2 = (bl(2) + 2 * bd + 2, bl(E) + bd + 3)
        ori4 = (bD4 + bd, bN4, bw + bcc4 + 2)
        allok = max(side3 + side4 + side2 + ori4) <= 125
        out(f"E = {E} : cote q3 {side3}, q4 {side4}, q2 {side2} ; orientation q4 {ori4} -> {'<= 125' if allok else '> 125'}")
        if E == 2 ** 19 - 1:
            check(allok and side3 == (121, 122) and ori4[2] == 122,
                  "E = 2^19 - 1 : conditions 121 / 122 (cote q3) et 122 (orientation q4), toutes <= 125")
    # orientation avec centre q3 dans une feuille courte : jamais appelee en voie courte ? (sinon debordement)
    E = 2 ** 19 - 1
    ori3 = bl(2 * E ** 2) + bl(48 * E ** 5) + 2
    out(f"orientation avec un centre q3 dans une feuille courte : {ori3} > 125 -> interdit en voie courte "
        "(le code n'y appelle que orient_center_wide via center_in_plane)")
    # tirages adverses dans une feuille d'etendue 2^19 - 1 : magnitudes reelles
    rnd = random.Random(20260930)
    worst = [0, 0, 0]
    for _ in range(20000):
        pts = [tuple(rnd.choice((0, E, rnd.randrange(E + 1))) for _ in range(3)) for _ in range(5)]
        a, b, c, d, z = pts
        uu_, vv_ = [x - y for x, y in zip(b, a)], [x - y for x, y in zip(c, a)]
        w = (uu_[1] * vv_[2] - uu_[2] * vv_[1], uu_[2] * vv_[0] - uu_[0] * vv_[2], uu_[0] * vv_[1] - uu_[1] * vv_[0])
        if w == (0, 0, 0):
            continue
        U2, V2 = sum(t * t for t in uu_), sum(t * t for t in vv_)
        t = [U2 * vv_[i] - V2 * uu_[i] for i in range(3)]
        N = (t[1] * w[2] - t[2] * w[1], t[2] * w[0] - t[0] * w[2], t[0] * w[1] - t[1] * w[0])
        D = 2 * sum(x * x for x in w)
        dz = [x - y for x, y in zip(z, a)]
        bd = max(bl(x) for x in dz)
        worst[0] = max(worst[0], bl(D) + 2 * bd + 2)
        worst[1] = max(worst[1], max(bl(x) for x in N) + bd + 3)
    check(max(worst[:2]) <= 125, f"tirages adverses q3 (feuille 2^19 - 1) : conditions observees {worst[:2]} <= 125")
    # ------------------------------------------------------------ 4. marge des filtres
    out()
    out("## 4. Marge certifiee des filtres (rationnels exacts)")
    alpha = (1 + u) ** 2 / (1 - u) - 1  # quotient fl(fl(N)/fl(D))
    check(alpha <= F(301, 100) * u, "centre : erreur relative du quotient <= 3,01 u")
    rows = []
    gap_doc = []
    for B in range(1, 22):
        L = 2 ** B - 1
        dlt_exact = (alpha + u + u * alpha) * L
        dlt = F(402, 100) * u * L
        g5, g3 = gamma(5), gamma(3)
        eps = 3 * g5 * (L + dlt) ** 2 + 3 * dlt * (2 * L + dlt)
        inv = (6 * g5 + 3 * g5 ** 2 + 3 * g3 * (1 + g5) ** 2) * 2 ** (2 * B)
        m = exact_double(filter_margin(B))
        Tmax = 3 * (L + dlt) ** 2 * (1 + g5) + m          # r2a + m (closed_ball, nearest W + m)
        h = half_ulp_bound(Tmax)
        joint = 6 * g5 * (L + dlt) ** 2 + 6 * L * dlt     # meme centre : erreur de d_z - d_a
        # I3 (deux centres) sous c dans conv(support) : r^2 <= 3 L^2 / 4
        r2c = F(3, 4) * L * L
        eps_conv = g5 * (r2c + 3 * L * dlt + 3 * dlt * dlt) + 3 * L * dlt + 3 * dlt * dlt
        h_conv = half_ulp_bound(r2c * (1 + g5) + 3 * L * dlt + 3 * dlt ** 2 + 1)
        rows.append((B, m, 2 * eps, 2 * inv, h, joint, eps_conv))
        check(dlt_exact <= dlt, f"B{B} : delta exact {float(dlt_exact):.4g} <= 4,02 u L")
        check(m >= 2 * eps, f"B{B} : m = {float(m)} >= 2 eps = {float(2 * eps):.6g}")
        check(m >= 2 * inv, f"B{B} : m >= 2 borne inventaire = {float(2 * inv):.6g}")
        check(m >= joint + h, f"B{B} : m >= erreur conjointe {float(joint):.6g} + arrondi du seuil {float(h):.3g}")
        check(m >= 2 * eps_conv + h_conv, f"B{B} : I3 sous conv : m >= 2 eps_conv + h = {float(2 * eps_conv + h_conv):.6g}")
        if m < 2 * eps + h:
            gap_doc.append((B, float(2 * eps + h), float(m)))
        if B == 21:
            claim_delta = F(2) ** B * F(1, 2 ** 51)
            out(f"B21 : delta = 4,02 u L = {float(dlt):.6g} ; affirme 'delta < 2^B 2^-51 (4,7e-10)' : 2^B 2^-51 = "
                f"{float(claim_delta):.6g}")
            if not dlt < claim_delta:
                note("site_tree.cpp (1) : 'delta < 2^B 2^-51 (4,7e-10 a B = 21)' est faux : delta = 4,02 u L = "
                     f"{float(dlt):.4g} > 2^(B-51) = {float(claim_delta):.4g} (et 4,7e-10 est la moitie de la valeur)")
            out(f"B21 : 2 eps = {float(2 * eps):.7g}, arrondi du seuil r2a +- m <= {float(h):.6g}, "
                f"somme = {float(2 * eps + h):.7g} vs m = {float(m)}")
    if gap_doc:
        note("preuve documentee de la marge (2 eps(B) <= m) : l'arrondi des seuils r2a +- m, W + m, r2a_prec - m est "
             "omis ; avec lui, la chaine ecrite ne se referme pas : " +
             ", ".join(f"B{b} : 2 eps + h = {x:.7g} > m = {y}" for b, x, y in gap_doc))
    # I3 sans l'hypothese conv (cube seul) : deux centres
    B = 21
    L = 2 ** B - 1
    dlt = F(402, 100) * u * L
    g5 = gamma(5)
    eps = 3 * g5 * (L + dlt) ** 2 + 3 * dlt * (2 * L + dlt)
    m = exact_double(filter_margin(B))
    hI3 = half_ulp_bound(3 * (L + dlt) ** 2 * (1 + g5))
    out(f"I3 a B21 sans conv (cube seul) : 2 eps + h = {float(2 * eps + hI3):.7g} vs m = {float(m)} -> "
        f"{'ferme' if m >= 2 * eps + hI3 else 'NE FERME PAS'}")
    if m < 2 * eps + hI3:
        note("garde I3 de la tour (deux centres differents) : sous la seule hypothese 'centre dans le cube', "
             "2 eps(21) + demi-ulp(r2a) > m ; il faut l'hypothese c dans conv(F) (MEB), qui donne r^2 <= 3 L^2 / 4 "
             "et ferme la borne (voir ligne 'I3 sous conv')")
    # borne uniforme de la preuve documentee, mais seuil borne par c dans conv(support) : r2a <= 3 L^2 / 4 (+ erreur)
    hconv = half_ulp_bound(F(3, 4) * L * L * (1 + g5) + 6 * L * dlt + m)
    out(f"B21, borne documentee 2 eps + arrondi d'un seuil < 2^(2B) (centres de MEB, c dans conv) : "
        f"{float(2 * eps + hconv):.7g} vs m = {float(m)} -> {'ferme' if m >= 2 * eps + hconv else 'NE FERME PAS'}")
    check(m >= 2 * eps + hconv, "B21 : sous c dans conv(support), la chaine documentee se referme avec l'arrondi du seuil")
    # ------------------------------------------------------------ 5. filtre d'orientation semi-statique de la tour
    out()
    out("## 5. Filtre d'orientation semi-statique (tour)")
    g2 = gamma(2)
    err = g2 + g2 / (1 - g2)
    check(16 * u * (1 - g2) > err, f"|s - v| <= (g2 + g2/(1-g2)) S = {float(err / u):.6f} u S < 16 u fl(S) (1 - g2)")
    check(bl(2 * E21 ** 2) <= 53, "w exact en double (|w_i| <= 2 E^2 < 2^43)")
    check(bl(30 * E21 ** 4) <= 127, "cc q4 en i128 (30 E^4 < 2^89)")
    # ------------------------------------------------------------ 6. to_double et approx
    out()
    out("## 6. to_double (Horner) et approx")
    for L_ in range(1, 9):
        check(gamma(L_) < (L_ + 2) * u, f"gamma_{L_} < ({L_} + 2) u")
    ratio = (1 + gamma(3)) * (1 + u) / (1 - gamma(3)) - 1
    check(ratio < 8 * u, f"approx : erreur relative (1+g3)(1+u)/(1-g3) - 1 = {float(ratio / u):.4f} u < 8 u")

    def to_double(words, neg):
        d = 0.0
        for w in reversed(words):
            d = d * 18446744073709551616.0 + float(w)
        return -d if neg else d
    worst_rel = {}
    rnd = random.Random(7)
    for Lw in (2, 3, 4, 6, 8):
        wr = F(0)
        for _ in range(4000):
            words = []
            for i in range(Lw):
                kind = rnd.randrange(4)
                if kind == 0:
                    words.append((1 << 64) - 1)
                elif kind == 1:
                    words.append((1 << 63) + (1 << 10) + 1)
                elif kind == 2:
                    words.append(rnd.getrandbits(64))
                else:
                    words.append((1 << 53) + 1 << rnd.randrange(11))
            v = sum(w << (64 * i) for i, w in enumerate(words))
            if v == 0:
                continue
            d = to_double(words, False)
            rel = abs(F(d) - v) / v
            wr = max(wr, rel)
        worst_rel[Lw] = wr
        check(wr <= gamma(Lw), f"to_double L = {Lw} : erreur relative observee max {float(wr / u):.4f} u <= gamma_L")
    # ------------------------------------------------------------ 7. generateur T6 a B21
    out()
    out("## 7. Generateur (repere T6, kT = 6) a B21")
    Xmax = (2 ** 21 - 1) << 6
    check(Xmax < 2 ** 27, "X < 2^27")
    check(bl(3 * Xmax ** 2) <= 63, f"X2 = |X|^2 < 3 2^54 en i64 ({bl(3 * Xmax ** 2)} bits)")
    check(2 * (21 + 6) + 5 <= 63 and 27 * (2 ** 27) ** 2 < 2 ** 59, "static_assert 59 <= 63 ; cle dd < 27 2^54 < 2^59")
    hi_box = Xmax + 1
    c_ = 2 * (2 ** 27) ** 2
    P_ = 6 * 2 ** 54 + 3 * 2 * (2 * hi_box) * 2 ** 27
    check(c_ <= 2 ** 55 and P_ < 2 ** 63, f"lemme Z : |c| < 2^55, |P| < 2^59 en i64 (P <= {bl(P_)} bits)")
    check(2 * 2 ** 27 * P_ < 2 ** 127 and 4 * 2 ** 27 * c_ < 2 ** 127, "lemme Z : produits i128 (< 2^87, < 2^84)")
    cib = (2 ** 21 * 24 * E21 ** 4 + 24 * E21 ** 5) * 64
    check(bl(cib) <= 117 and bl(hi_box * 24 * E21 ** 4) <= 116, f"center_in_box q3 : {bl(cib)} bits (i128)")
    mid = 2 * (2 ** 21 * 24 * E21 ** 4 + 24 * E21 ** 5)
    check(bl(mid) <= 112, f"is_midpoint q3 : {bl(mid)} bits (i128)")
    check(bl(3 * E21 ** 2) <= 63 and bl(2 * E21 ** 2) <= 63, "dot, cross en i64")
    # ------------------------------------------------------------ 8. domaine du filtre de SiteTree
    out()
    out("## 8. Domaine du filtre de SiteTree (bornes (3), (5), (6))")
    for B in range(1, 22):
        E = 2 ** B - 1
        okB = (24 * E ** 4 < 2 ** (4 * B + 5) and 24 * E ** 5 < 2 ** (5 * B + 5) and 12 * E ** 3 < 2 ** (4 * B + 5)
               and 18 * E ** 4 < 2 ** (5 * B + 5) and 2 ** (4 * B + 10) * E + 2 ** (5 * B + 10) < 2 ** 127)
        if not okB:
            check(False, f"B{B} : bornes (3) (5) (6)")
    check(True, "B = 1..21 : D < 2^(4B+5) <= 2^(4B+10), |N| < 2^(5B+5), D a + N < 2^(5B+11) <= 2^116 (i128)")
    out()
    out(f"echecs : {len(FAILS)} ; constats : {len(NOTES)}")
    for n_ in NOTES:
        out(" - " + n_)
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())
