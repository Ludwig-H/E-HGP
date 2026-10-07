#!/usr/bin/env python3
"""Recalcul independant (audit L12) des attendus des fixtures exactes publiees par les auditeurs de la v10.

Chaque controle compare une valeur recalculee par gamma_oracle.py a la valeur ECRITE dans le corpus des auditeurs.
Sortie : une ligne par controle, code 1 si un seul echoue. Aucune dependance au moteur.
"""
import sys
from fractions import Fraction as F
from itertools import combinations
from gamma_oracle import Gamma, meb, census, qmin, line, dot, sub

fails = 0
count = 0


def check(name, got, want):
    global fails, count
    count += 1
    ok = got == want
    if not ok:
        fails += 1
    print(("PASS " if ok else "FAIL ") + name + ("" if ok else "  obtenu=%r attendu=%r" % (got, want)))


def last_merge(g):
    """Plus grand niveau ou la coupe ouverte a plusieurs composantes (ou aucune) et la coupe fermee une seule."""
    out = None
    for l in g.levels:
        if len(g.components(l, True)) == 1 and len(g.components(l, False)) != 1:
            out = l
    return out


def merges(g):
    """Niveaux ou au moins deux composantes de la coupe ouverte sont reunies (vraies fusions)."""
    out = []
    for l in g.levels:
        before = g.components(l, False)
        after = g.components(l, True)
        for c in after:
            parents = [b for b in before if set(b) <= set(c)]
            if len(parents) >= 2:
                out.append((l, len(parents)))
    return out


def idx(X, pts):
    return sorted(X.index(tuple(p)) for p in pts)


# ---- F1 : recouvrement des couvertures, {0,2,4}, K2, r = 1 (beta = 1) -------------------------------------------
X = line(0, 2, 4)
g = Gamma(X, 2)
check("F1 {0,2,4} K2 beta=1 : couvertures {0,2} et {2,4}", g.covers(F(1)), [[0, 1], [1, 2]])
check("F1 fusion des deux composantes a beta=4 (rayon 2)", last_merge(g), F(4))

# ---- F2 : discontinuite de la premiere couverture, {0,999,2000} et {0,1001,2000}, K2 ----------------------------
ga, gb = Gamma(line(0, 999, 2000), 2), Gamma(line(0, 1001, 2000), 2)
check("F2 m=999 : paire gauche-milieu nee a rayon 499,5", ga.vlevel[(0, 1)], F(999, 2) ** 2)
check("F2 m=1001 : paire milieu-droite nee a rayon 499,5 (milieu d'abord couvert a droite)", gb.vlevel[(1, 2)], F(999, 2) ** 2)
check("F2 m=1001 : paire gauche-milieu nee a 500,5", gb.vlevel[(0, 1)], F(1001, 2) ** 2)
check("F2 fusion des deux composantes K2 au rayon 1000 (les deux nuages)",
      (last_merge(ga), last_merge(gb)),
      (F(1000) ** 2, F(1000) ** 2))
check("F2 core : d_2 du milieu = 999 puis 1001", (ga.dk2(1), gb.dk2(1)), (999 ** 2, 999 ** 2))
# remarque : d_2(milieu) est la distance au plus proche voisin = 999 dans les deux nuages ; la reunion core
# gauche-milieu est max(d_2(gauche), d_2(milieu)) = 999 puis 1001 (d_2 du site gauche).
check("F2 core : reunion gauche-milieu = d_2(gauche) = 999 puis 1001", (ga.dk2(0), gb.dk2(0)), (999 ** 2, 1001 ** 2))

# ---- F3 : croisement multi-K, {0,20,22,50,52} ---------------------------------------------------------------------
X = line(0, 20, 22, 50, 52)
g1, g2 = Gamma(X, 1), Gamma(X, 2)
check("F3 K1 rayon 10 : blocs {0,20,22} et {50,52}", g1.covers(F(100)), [[0, 1, 2], [3, 4]])
core2 = sorted(sorted(i for i in range(5) if g2.dk2(i) <= 225 and any(i in v for v in c)) for c in g2.components(F(225)))
check("F3 K2 rayon 15 : bloc core {20,22,50,52}, le site 0 non entre", core2, [[1, 2, 3, 4]])

# ---- F3bis : {0,1,4,7} (auditeur independant) : {0,1} a K1 a=1/4 ; {1,4} a K4 a=36 -------------------------------
X = line(0, 1, 4, 7)
check("F3bis K1 a=1/4 : bloc {0,1}", Gamma(X, 1).covers(F(1, 4))[0], [0, 1])
check("F3bis K4 a=36 : sites a d_4 <= 6 = {1,4}", [i for i in range(4) if Gamma(X, 4).dk2(i) <= 36], [1, 2])

# ---- Lemme de couverture : temoins ------------------------------------------------------------------------------
X = [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 1, 0)]
c, r2 = meb(X)
I, U = census(X, c, r2)
check("LC K4 triangle + interieur : centre (2,5/6,0), rayon 13/6", (c, r2), ((F(2), F(5, 6), F(0)), F(169, 36)))
check("LC K4 : p=1, q_min=3", (len(I), qmin(U, c)), (1, 3))
X = [(-1, -1, 0), (-1, 1, 0), (1, -1, 0), (1, 1, 0)]
c, r2 = meb(X)
I, U = census(X, c, r2)
check("LC carre K4 : q_min=2 et quatre sites sur la coquille", (qmin(U, c), len(U), len(I)), (2, 4, 0))
X = [(0, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4), (2, 2, 2)]
c, r2 = meb(X[:4])
I, U = census(X, c, r2)
check("LC tetraedre + centre : boule q4 de rayon carre 12, p=1", (r2, len(I), qmin(U, c)), (F(12), 1, 4))
check("LC tetraedre : faces q3 de rayon carre 32/3", sorted(set(meb(t)[1] for t in combinations(X[:4], 3))), [F(32, 3)])
X = line(-2, 0, 2)
c, r2 = meb(X)
I, U = census(X, c, r2)
check("LC {-2,0,2} K2 : boule de fusion p=1, q_min=2 (p+q_min = K+1, a ne pas amputer)", (r2, len(I), qmin(U, c)), (F(4), 1, 2))
g = Gamma(line(0, 2, 6), 2)
check("LC {0,2,6} K2 R=2 : le site 2 est couvert par DEUX composantes", [c for c in g.covers(F(4)) if 1 in c], [[0, 1], [1, 2]])
g = Gamma(line(-2, 0, 4), 2)
check("LC {-2,0,4} K2 R=2 : le site 0 est couvert par deux composantes", [c for c in g.covers(F(4)) if 1 in c], [[0, 1], [1, 2]])

# ---- Majorite uniforme : 0,1,100,101 K2 -------------------------------------------------------------------------
g = Gamma(line(0, 1, 100, 101), 2)
check("M9 temoins propres beta = 1/4, 9801/4, 1/4", [g.vlevel[v] for v in ((0, 1), (1, 2), (2, 3))], [F(1, 4), F(9801, 4), F(1, 4)])
check("M9 fusion FULL des trois composantes a beta = 2500 (fusions : %r)" % (merges(g),), last_merge(g), F(2500))
strong = []
for v in g.verts:
    c, r2 = meb([g.X[i] for i in v])
    I, U = census(g.X, c, r2)
    if len(I) + qmin(U, c) <= 2 and len(I) + len(U) >= 2:
        strong.append(v)
check("M9 univers fort p+q_min<=2 : exactement trois boules", strong, [(0, 1), (1, 2), (2, 3)])

# ---- Section 10 : tetraedre orthogonal et contact coquille/interieur ------------------------------------------
for M in (16, 2048):
    Cc, A, B, D = (0, 0, 0), (4 * M, 0, 0), (0, 5 * M, 0), (0, 0, 100 * M)
    check("S10 M=%d : fusion ABC a beta = 41 M^2 / 4" % M, meb([Cc, A, B])[1], F(41 * M * M, 4))
    C2 = (1, 1, 1)
    check("S10 M=%d : C deplace en (1,1,1), fusion ABC inchangee" % M, meb([C2, A, B])[1], F(41 * M * M, 4))
    check("S10 M=%d : paire C'A a beta = ((4M-1)^2+2)/4" % M, meb([C2, A])[1], F((4 * M - 1) ** 2 + 2, 4))
    Xs = [C2, A, B, D]
    stat = []
    for pair in ((A, B), (A, D), (B, D)):
        c, r2 = meb(list(pair))
        I, U = census(Xs, c, r2)
        stat.append((len(I), qmin(U, c)))
    check("S10 M=%d : AB, AD, BD passent a p=1, q_min=2 (hors univers fort K2)" % M, stat, [(1, 2)] * 3)
    X0 = [Cc, A, B, D]
    stat0 = []
    for pair in ((A, B), (A, D), (B, D)):
        c, r2 = meb(list(pair))
        I, U = census(X0, c, r2)
        stat0.append((len(I), len(U), qmin(U, c)))
    check("S10 M=%d : avant deplacement, C est sur la coquille de AB, AD, BD (p=0)" % M, stat0, [(0, 3, 2)] * 3)

# ---- Section 11 : entrees frontiere strictement internes -----------------------------------------------------
X = [(15, 4, 0), (5, 4, 0), (7, 8, 0), (7, 0, 0), (1, 4, 0), (0, 4, 1)]
g = Gamma(X, 3)
check("S11 K3 : x=(15,4,0) n'est couvert qu'a beta = 25", g.alpha(0), F(25))
births = sorted(l for v, l in g.vlevel.items() if l < 25 and any(l == h[0] and h[2] for h in g.history()))
hist = g.history()
leaf_births = []
prev = 0
for l, ncomp, nborn, _ in hist:
    # naissance de composante : le nombre de composantes fermees augmente par rapport a la coupe ouverte
    opened = len(g.components(l, closed=False))
    alive_before_join = len([c for c in g.components(l, True)])
    # composantes nouvelles = composantes fermees ne contenant aucun sommet strictement anterieur
    new = [c for c in g.components(l, True) if all(g.vlevel[v] == l for v in c)]
    leaf_births += [l] * len(new)
check("S11 K3 : quatre feuilles nees a beta = 13/2, 13, 13, 16", leaf_births, [F(13, 2), F(13), F(13), F(16)])
check("S11 K3 : toutes les feuilles ont fusionne a beta <= 169/9 (fusions : %r)" % (merges(g),), max(l for l, _ in merges(g)) <= F(169, 9), True)
check("S11 K3 : a beta=25, une seule composante et la boule couvrant x a pour coquille {x,a,b,c}, p=0, q_min=2",
      (lambda cb: (len(census(X, *cb)[0]), sorted(census(X, *cb)[1]), qmin(census(X, *cb)[1], cb[0])))(meb([X[0], X[1]])),
      (0, sorted([X[0], X[1], X[2], X[3]]), 2))

X = [(325, 325, 650), (520, 325, 65), (200, 325, 25), (325, 416, 13), (325, 130, 65), (442, 481, 65), (250, 225, 25)]
ctr = (325, 325, 325)
check("S11 K5 : sept sites sur la sphere de centre (325,325,325), rayon 325", [dot(sub(p, ctr), sub(p, ctr)) for p in X], [325 * 325] * 7)
g = Gamma(X, 5)
check("S11 K5 : x n'entre qu'a beta = 105625", g.alpha(0), F(105625))
hist = g.history()
check("S11 K5 : trois feuilles fusionnent a beta = 116715625/3409 (vraies fusions : %r)" % (merges(g),),
      merges(g)[0], (F(116715625, 3409), 3))
new5 = []
for l, ncomp, nborn, _ in hist:
    new5 += [l] * len([c for c in g.components(l, True) if all(g.vlevel[v] == l for v in c)])
check("S11 K5 : trois feuilles (naissances de composantes)", len(new5), 3)
cb = meb(X)
I, U = census(X, *cb)
check("S11 K5 : boule commune p=0, q_min=3, coquille de sept sites", (len(I), len(U), qmin(U, cb[0])), (0, 7, 3))

# ---- Q7 : cinq points collineaires, saut du routage sur tour condensee ---------------------------------------
for eps in (F(0), F(1, 10), F(-1, 10)):
    X = [(F(-11), 0, 0), (F(-10), 0, 0), (eps, 0, 0), (F(10), 0, 0), (F(11), 0, 0)]
    g = Gamma(X, 2)
    lv = g.vlevel
    check("Q7 eps=%s : feuilles a 1/4, (10+eps)^2/4, (10-eps)^2/4, 1/4" % eps,
          [lv[(0, 1)], lv[(1, 2)], lv[(2, 3)], lv[(3, 4)]], [F(1, 4), (10 + eps) ** 2 / 4, (10 - eps) ** 2 / 4, F(1, 4)])
    hist = g.history()
    check("Q7 eps=%s : fusion finale des deux branches a beta = 100" % eps, last_merge(g), F(100))
    aL, aR = (11 + eps) ** 2 / 4, (11 - eps) ** 2 / 4
    check("Q7 eps=%s : la feuille frontiere gauche rejoint la branche dense a aL=(11+eps)^2/4" % eps,
          any(set((0, 1, 2)) <= set(c) for c in g.covers(aL)) and not any(set((0, 1, 2)) <= set(c) for c in g.covers(aL, closed=False)), True)

# ---- Auditeur independant : cinq sites, majorite fixe ------------------------------------------------------------
X = [(1, 1, 0), (2, 1, 0), (0, 2, 0), (0, 0, 0), (0, 1, 1)]
g = Gamma(X, 2)
check("5S K2 : paire {0,1} nee a beta = 1/4", g.vlevel[(0, 1)], F(1, 4))
check("5S K2 : fusion globale a beta = 5/4 (fusions : %r)" % (merges(g),), last_merge(g), F(5, 4))
check("5S K2 : a beta = 2/3 une composante couvre {0,2,3,4}", [0, 2, 3, 4] in g.covers(F(2, 3)), True)

# ---- Carre K2 (Q13) -------------------------------------------------------------------------------------------
X = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
g = Gamma(X, 2)
h = g.history()
check("Q13 carre K2 : quatre lentilles d'aretes nees a beta=1, fusion a beta=2", [(l, n) for l, n, _, _ in h], [(F(1), 4), (F(2), 1)])

# ---- Massif : semantique des tuiles -----------------------------------------------------------------------------
g = Gamma(line(0, 1, 2), 2)
check("MS {0,1,2} K2 : le site 1 couvert deux fois a beta=1/4, fusion a beta=1",
      ([c for c in g.covers(F(1, 4)) if 1 in c], last_merge(g)), ([[0, 1], [1, 2]], F(1)))
g = Gamma(line(0, 1, 10, 11), 2)
h = g.history()
check("MS {0,1,10,11} K2 : composante nee dans le vide a beta=81/4", g.vlevel[(1, 2)], F(81, 4))
comps_before = len(g.components(F(25), closed=False))
check("MS {0,1,10,11} K2 : fusion a beta=25 de trois parents", (comps_before, len(g.components(F(25)))), (3, 1))

# ---- Contact de la majorite uniforme (eps = 0 puis eps > 0) --------------------------------------------------
for eps in (F(0), F(1, 100)):
    a, b, z, y = (0, 0, 0), (10, 0, 0), (9 - eps, 3, 0), (0, 0, 9)
    X = [a, b, z, y]
    g = Gamma(X, 2)
    check("UM eps=%s : alpha_a^2 = 81/4 (paire a-y)" % eps, g.alpha(0), F(81, 4))
    c, r2 = meb([a, b])
    I, U = census(X, c, r2)
    check("UM eps=%s : boule AB, p=%d" % (eps, 0 if eps == 0 else 1), len(I), 0 if eps == 0 else 1)
    check("UM eps=%s : evenement ABZ a beta=25 present dans Gamma_2" % eps, meb([a, b, z])[1], F(25))
    check("UM eps=%s : evenement AYZ a beta=(171-18eps+eps^2)/4" % eps, meb([a, y, z])[1], (171 - 18 * eps + eps * eps) / 4)

# ---- Duree positive annulee par soustraction flottante -----------------------------------------------------------
A, B, C = (0, 0, 0), (94642, 0, 0), (80782, 33461, 0)
lab, labc = meb([A, B])[1], meb([A, B, C])[1]
check("DP niveau de AB = 2239277041", lab, F(2239277041))
check("DP niveau du triangle = 10028723337177985445/4478554084", labc, F(10028723337177985445, 4478554084))
check("DP duree exacte = 1/4478554084 > 0", labc - lab, F(1, 4478554084))
check("DP la soustraction des deux doubles vaut 0", float(labc.numerator) / float(labc.denominator) - float(lab), 0.0)
check("DP produit scalaire au sommet C = 1 (triangle strictement aigu)", dot(sub(A, C), sub(B, C)), 1)
A, B, C = (0, 0, 0), (1048576, 0, 0), (1048575, 1024, 0)
check("DP u21 : duree exacte 2^-22", meb([A, B, C])[1] - meb([A, B])[1], F(1, 2 ** 22))

# ---- Coalescence des rangs (trois sites) -----------------------------------------------------------------------
A, B, C = (0, 0, 0), (261120, 2, 0), (1, 512, 0)
check("CR beta(AB) = 17045913601 et beta(ABC) - beta(AB) = 17045913601/17873935364259844",
      (meb([A, B])[1], meb([A, B, C])[1] - meb([A, B])[1]), (F(17045913601), F(17045913601, 17873935364259844)))

# ---- Deux triangles de la these (variantes entieres) -----------------------------------------------------------
for name, shift, cd, glob in (("aretes plus courtes", 0, 1000000, 3731956), ("pont plus court", -2, 998001, 3728225)):
    A, B, C = (268, 3000, 0), (268, 1000, 0), (2000, 2000, 0)
    D, E, Fp = (4000 + shift, 2000, 0), (5732 + shift, 3000, 0), (5732 + shift, 1000, 0)
    X = [A, B, C, D, E, Fp]
    g = Gamma(X, 2)
    lv = g.vlevel
    check("2T %s : AC, BC, DE, DF a beta = 999956" % name, [lv[(0, 2)], lv[(1, 2)], lv[(3, 4)], lv[(3, 5)]], [F(999956)] * 4)
    check("2T %s : AB, EF a beta = 1000000 ; CD a beta = %d" % (name, cd), [lv[(0, 1)], lv[(4, 5)], lv[(2, 3)]], [F(1000000), F(1000000), F(cd)])
    check("2T %s : fusions des triangles a beta = 249978000484/187489" % name, (meb([A, B, C])[1], meb([D, E, Fp])[1]), (F(249978000484, 187489),) * 2)
    hist = g.history()
    check("2T %s : fusion globale a beta = %d" % (name, glob), last_merge(g), F(glob))
    mid = F(1500) ** 2  # rho = 1,5 r0, entre les deux fusions
    check("2T %s : entre les fusions, FULL a ABC, CD, DEF (couvertures recouvrantes)" % name, g.covers(mid), [[0, 1, 2], [2, 3], [3, 4, 5]])
    check("2T %s : fusion globale = multifusion de TROIS composantes" % name, len(g.components(F(glob), closed=False)), 3)
    check("2T %s : entree core de C et D a d_2^2 >= fusion globale ? (C entre a %s)" % (name, g.dk2(2)), g.dk2(2) > mid, True)

print("controles=%d echecs=%d" % (count, fails))
sys.exit(1 if fails else 0)
