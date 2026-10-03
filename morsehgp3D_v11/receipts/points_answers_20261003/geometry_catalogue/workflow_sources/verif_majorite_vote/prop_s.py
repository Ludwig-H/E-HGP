#!/usr/bin/env python3
"""Proposition S du rapport majorite_vote : recalcul independant.

1. Forme close : niveaux de X_rho et Y_rho par mes propres formules (paires |p-q|^2/4, boule circonscrite par Heron
   sur les carres des cotes), comparees a vfull (v10) et a l'oracle v11.
2. delta = plus grand decalage des niveaux de noeuds apparies (appariement structurel : sites couverts a la naissance,
   enfants) et des debuts de couverture apparies ; refus si la combinatoire differe.
3. ER0h(1, 12), ER0hr(1, 10), ER0(1, 12), H_1 et H_3 (niveau), H^r_1 et H^r_3 (rayon) par mes_regles.py.
4. Minoration annoncee (64 kappa / 129) rho^2 - 1.
Familles : isocele (rapport section 5.1), radiale (y2 deplace de 1 selon x, L = 48 rho^4, h = L / rho),
selle a cinq sites (u(x, g1)).
"""
from fractions import Fraction
import decimal
import json
import sys

sys.dont_write_bytecode = True
import mes_regles as M  # noqa: E402
from vrad import R  # noqa: E402

D = decimal.Context(prec=80)


def dec(r):
    """R -> Decimal (80 chiffres)."""
    s = decimal.Decimal(0)
    for q, c in r.t.items():
        s = D.add(s, D.multiply(D.divide(decimal.Decimal(c.numerator), decimal.Decimal(c.denominator)),
                                D.sqrt(D.divide(decimal.Decimal(q.numerator), decimal.Decimal(q.denominator)))))
    return s


def sq(r):
    v = dec(r)
    return D.multiply(v, v)


def signatures(T):
    """Signature structurelle de chaque noeud, sans lire les niveaux."""
    sig = {}
    order = sorted(range(len(T)), key=lambda v: T.birth[v])
    for v in order:
        if not T.children[v]:
            born = frozenset(x for x in range(T.n) if T.cov[x].get(v) == T.birth[v])
            sig[v] = ('n', born)
        else:
            sig[v] = ('f', frozenset(sig[c] for c in T.children[v]))
    return sig


def delta(TX, TY):
    sx, sy = signatures(TX), signatures(TY)
    ix = {s: v for v, s in sx.items()}
    iy = {s: v for v, s in sy.items()}
    M.exiger(len(ix) == len(sx) and len(iy) == len(sy) and set(ix) == set(iy), 'combinatoire differente')
    d = Fraction(0)
    pair = {}
    for s, vx in ix.items():
        vy = iy[s]
        pair[vx] = vy
        d = max(d, abs(TX.birth[vx] - TY.birth[vy]))
    for x in range(TX.n):
        M.exiger(set(pair[v] for v in TX.cov[x]) == set(TY.cov[x]), 'couverture non appariee')
        for v, c in TX.cov[x].items():
            d = max(d, abs(c - TY.cov[x][pair[v]]))
    return d, pair


def heron_R2(p, q, r):
    """Rayon carre circonscrit d'un triangle par les carres des cotes (Heron)."""
    def d2(a, b):
        return sum((Fraction(a[i]) - b[i]) ** 2 for i in range(3))
    a2, b2, c2 = d2(q, r), d2(p, r), d2(p, q)
    s16 = 2 * (a2 * b2 + b2 * c2 + c2 * a2) - (a2 * a2 + b2 * b2 + c2 * c2)   # 16 aire^2
    return a2 * b2 * c2 / s16, (a2, b2, c2)


def regles(T, x):
    out = {}
    out['ER0h'] = M.er0h(T, x, 1, 12)
    out['ER0hr10'] = M.er0h(T, x, 1, 10, relatif=True)
    out['ER0'] = M.er0h(T, x, 1, 12, sans_heritage=True)
    out['H1'] = M.hm_niveau(T, x, 1)
    out['H3'] = M.hm_niveau(T, x, 3)
    out['Hr1'] = M.hm_rayon(T, x, 1)
    out['Hr3'] = M.hm_rayon(T, x, 3)
    return out


def u(T, hang, i, j):
    return M.u_niveau(T, hang, i, j)


def famille_isocele():
    lignes = []
    for rho in (5, 10, 20, 40, 80, 160):
        L, h = 8 * rho * rho, 8 * rho
        X = [(0, 0, 0), (L, h, 0), (L, -h, 0)]
        Y = [(0, 0, 0), (L, h, 0), (L, -h, 1)]
        TX, _ = M.full(X, 2)
        TY, _ = M.full(Y, 2)
        # forme close
        o = Fraction(16 * rho ** 2 * (rho ** 2 + 1))
        b = Fraction(16 * (rho ** 2 + 1) ** 2)
        epsp = Fraction((rho ** 2 + 1) * (192 * rho ** 2 - 63), 4 * (256 * rho ** 4 + rho ** 2 + 1))
        RX, sidesX = heron_R2(*X)
        RY, sidesY = heron_R2(*Y)
        aigu = all(2 * max(s) < sum(s) for s in (sidesX, sidesY))
        T11 = M.arbre_v11(Y, 2)
        ok_forme = (sorted(TX.birth) == sorted([o, o, Fraction(h * h), b]) and RX == b and RY == b + epsp
                    and sorted(TY.birth) == sorted([o, o + Fraction(1, 4), Fraction(h * h) + Fraction(1, 4), b + epsp])
                    and sorted(T11.birth) == sorted(TY.birth) and aigu)
        d, _pair = delta(TX, TY)
        rx = regles(TX, 0)
        ry = regles(TY, 0)
        row = {'rho': rho, 'L': L, 'forme_close_ok': ok_forme, 'delta': str(d), 'eps_prime': str(epsp)}
        for k in rx:
            du = abs(sq(rx[k][0]) - sq(ry[k][0]))
            row[k] = float(D.divide(du, decimal.Decimal(d.numerator) / decimal.Decimal(d.denominator)))
        row['minoration'] = float(Fraction(64 * 12, 129) * rho * rho - 1)
        row['ER0h_date_X'] = rx['ER0h'][0].texte()
        row['ER0h_date_Y'] = ry['ER0h'][0].texte()
        lignes.append(row)
        print(json.dumps(row), flush=True)
    return lignes


def famille_radiale():
    lignes = []
    for rho in (5, 10, 20, 40):
        L = 48 * rho ** 4
        h = L // rho
        X = [(0, 0, 0), (L, h, 0), (L, -h, 0)]
        Y = [(0, 0, 0), (L, h, 0), (L + 1, -h, 0)]
        TX, _ = M.full(X, 2)
        TY, _ = M.full(Y, 2)
        d, _ = delta(TX, TY)
        rx = regles(TX, 0)
        ry = regles(TY, 0)
        row = {'rho': rho, 'L': L, 'h': h, 'delta_niveau': float(d)}
        for k in rx:
            row[k + '_saut_rayon'] = float(abs(dec(rx[k][0]) - dec(ry[k][0])))
            row[k + '_niveau_sur_delta'] = float(D.divide(abs(sq(rx[k][0]) - sq(ry[k][0])),
                                                        decimal.Decimal(d.numerator) / decimal.Decimal(d.denominator)))
        lignes.append(row)
        print(json.dumps(row), flush=True)
    return lignes


def famille_selle():
    lignes = []
    for base in ((1700, 1000, 100), (1720, 1000, 100), (1728, 1000, 50), (1731, 1000, 20)):
        for lam in (50, 400, 2000, 20000):
            a, b, e = (lam * c for c in base)
            X = [(0, 0, 0), (a, b, 0), (a, b, e), (a, -b, 0), (a, -b, -e)]
            Y = [(0, 0, 0), (a, b, 0), (a, b, e), (a + 1, -b, 0), (a + 1, -b, -e)]
            TX, _ = M.full(X, 2)
            TY, _ = M.full(Y, 2)
            try:
                d, _ = delta(TX, TY)
            except M.Erreur as err:
                print(json.dumps({'base': base, 'lam': lam, 'refus': str(err)}), flush=True)
                continue
            hx = {k: [] for k in ('ER0h', 'ER0hr10', 'Hr3', 'H3')}
            hy = {k: [] for k in hx}
            for x in range(5):
                for k, f in (('ER0h', lambda T, x: M.er0h(T, x, 1, 12)),
                             ('ER0hr10', lambda T, x: M.er0h(T, x, 1, 10, relatif=True)),
                             ('Hr3', lambda T, x: M.hm_rayon(T, x, 3)), ('H3', lambda T, x: M.hm_niveau(T, x, 3))):
                    hx[k].append(f(TX, x))
                    hy[k].append(f(TY, x))
            row = {'base': base, 'lam': lam, 'delta_niveau': float(d)}
            for k in hx:
                ux = u(TX, hx[k], 0, 1)
                uy = u(TY, hy[k], 0, 1)
                row[k + '_u01_niveau_sur_delta'] = float(D.divide(abs(sq(ux) - sq(uy)),
                                                                decimal.Decimal(d.numerator) / decimal.Decimal(d.denominator)))
                row[k + '_u01_saut_rayon'] = float(abs(dec(ux) - dec(uy)))
                row[k + '_proprio_x_Y_couvre_g1'] = (1 in [y for y in range(5) if hy[k][0][1] in TY.cov[y]])
            lignes.append(row)
            print(json.dumps(row), flush=True)
    return lignes


if __name__ == '__main__':
    quoi = sys.argv[1] if len(sys.argv) > 1 else 'tout'
    out = {}
    if quoi in ('tout', 'isocele'):
        out['isocele'] = famille_isocele()
    if quoi in ('tout', 'radiale'):
        out['radiale'] = famille_radiale()
    if quoi in ('tout', 'selle'):
        out['selle'] = famille_selle()
    with open('recus/prop_s_%s.json' % quoi, 'w') as f:
        json.dump(out, f, indent=1)
