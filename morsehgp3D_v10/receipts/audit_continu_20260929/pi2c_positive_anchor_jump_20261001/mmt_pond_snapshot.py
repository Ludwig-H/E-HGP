#!/usr/bin/env python3
"""MMt_{kappa,eta} a temps de couverture PONDERE (poids Pi2c par noeud) : noyau exact d'un point.

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=juge_final_parametres, public_status=not_claimed. GCP non utilise. Aucun moteur modifie.

Generalisation directe de majorites_continues/mmt.py (mmt_point, lu seulement) : la masse d'une composante C au niveau
s est m(C, s) = somme, sur les v de V_x descendants de C (C compris), de omega_v |[c_x(v), min(d_v, s, E2)]| ; la pente
d'une composante vivante entre deux evenements est le poids omega du noeud vivant s'il couvre x (0 sinon). Avec
omega = 1 partout, c'est exactement MMt (recoupe exacte `recouper_reference`). Echelle A = plus petit niveau de
couverture de poids > 0 ; bande [A, (1 + eta) A] ; W, G, mu, T(theta), T_1/2, date sup_theta (sqrt T(theta) -
kappa alpha (2 theta - 1)) et proprietaire comme MMt. Points critiques : sur un segment de pente p, le maximum
interieur de D(s) = sqrt(s) - kappa alpha (2 G(s)/W - 1) est en sqrt(s*) = W / (4 kappa alpha p).
Tout est exact (Fraction, QS) ; aucun assert.
"""
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
if ICI not in sys.path:
    sys.path.insert(0, ICI)
import pipeline as PL  # noqa: E402

QS = PL.QS
Rayon = PL.Rayon
mmt = PL.mmt
exiger = PL.exiger


def _anc(T, v, s):
    exiger(T.birth[v] <= s, 'noeud non ne')
    while T.parent[v] >= 0 and T.birth[T.parent[v]] <= s:
        v = T.parent[v]
    return v


def mmt_point_pond(T, cvw, eta, kappa):
    """cvw : {noeud : (c_x(v), omega_v)} avec omega_v > 0, clos vers le haut. Rend le meme dict que mmt.mmt_point."""
    eta, kappa = Fraction(eta), Fraction(kappa)
    exiger(eta > 0 and kappa > 0, 'parametres non positifs')
    exiger(cvw, 'structure ponderee vide')
    cv = {v: c for v, (c, _w) in cvw.items()}
    om = {v: w for v, (_c, w) in cvw.items()}
    A = min(cv.values())
    E2 = (1 + eta) * A
    massifs = [v for v, c in cv.items() if c < E2]

    def fin(v):
        d = T.death[v]
        return E2 if d is None or d > E2 else d

    W = sum((om[v] * (fin(v) - cv[v]) for v in massifs if fin(v) > cv[v]), Fraction(0))
    exiger(W > 0, 'masse totale nulle')
    evs = {A, E2}
    for v in massifs:
        evs.add(cv[v])
        u = v
        while T.parent[u] >= 0:
            u = T.parent[u]
            evs.add(T.birth[u])
    evs = sorted(evs)

    def masses(s):
        out = {}
        for v in massifs:
            if T.birth[v] > s:
                continue
            C = _anc(T, v, s)
            m = min(fin(v), s) - cv[v]
            m = om[v] * m if m > 0 else Fraction(0)
            old = out.get(C, (Fraction(0), 0))
            out[C] = (old[0] + m, old[1])
        for C in list(out):
            pente = om[C] if (C in cv and cv[C] <= s and s < E2) else Fraction(0)
            out[C] = (out[C][0], pente)
        return out

    demi = W / 2
    T_half = O = None
    termes = []
    crit = []
    T1 = None
    trace = []
    prec = None
    for e in evs:
        if prec is not None:
            s0, ms = prec
            gauche = {C: m + p * (e - s0) for C, (m, p) in ms.items()}
            if T_half is None:
                for C, (m, p) in ms.items():
                    if p > 0 and m <= demi < m + p * (e - s0):
                        exiger(T_half is None, 'franchissement non exclusif')
                        T_half, O = s0 + (demi - m) / p, C
                if T_half is not None:
                    Gm = gauche[O]
                    if Gm < W:
                        termes.append((e, 2 * Gm / W - 1))
                    crit.append((T_half, e, O, s0, ms[O][0], ms[O][1]))
            else:
                OC = _anc(T, O, s0)
                Gm = gauche[OC]
                if Gm < W:
                    termes.append((e, 2 * Gm / W - 1))
                if ms[OC][1] > 0:
                    crit.append((s0, e, OC, s0, ms[OC][0], ms[OC][1]))
        ms = masses(e)
        G = max(m for m, _p in ms.values()) if ms else Fraction(0)
        trace.append((e, G))
        if T_half is None and 2 * G > W:
            gagnants = [C for C, (m, _p) in ms.items() if 2 * m > W]
            exiger(len(gagnants) == 1, 'majorite non exclusive')
            T_half, O = e, gagnants[0]
        if T1 is None and G == W:
            T1 = e
        prec = (e, ms)
    exiger(T_half is not None and T1 is not None, 'majorite ou unanimite jamais atteinte')
    a = QS.sqrt(A)
    date = QS.sqrt(T_half)
    arg = ('T_half', T_half)
    for e, mu in termes:
        if e <= T_half:
            continue
        exiger(mu > 0, 'marge non positive apres T_1/2')
        val = QS.sqrt(e) - a * (kappa * mu)
        if val.cmp(date) > 0:
            date, arg = val, ('saut', e)
    for lo, hi, _C, s0, m0, p in crit:
        s_star = W * W / (16 * kappa * kappa * A * p * p)
        if lo < s_star < hi:
            G_star = m0 + p * (s_star - s0)
            val = QS.sqrt(s_star) - a * (kappa * (2 * G_star / W - 1))
            if val.cmp(date) > 0:
                date, arg = val, ('critique', s_star)
    rd = Rayon.somme(date)
    o = _anc(T, O, T_half)
    while T.parent[o] >= 0 and Rayon(b2=T.birth[T.parent[o]]).cmp(rd) <= 0:
        o = T.parent[o]
    return {'date': date, 'owner': o, 'T_half': T_half, 'O': O, 'W': W, 'A': A, 'E2': E2, 'T1': T1,
            'termes': termes, 'argmax': arg, 'N': len(massifs), 'trace': trace, 'alpha': a,
            'crit': [(lo, hi, s0, m0, p) for lo, hi, _C, s0, m0, p in crit]}


def seuil_kappa(r, B2):
    """Seuil exact tau = inf{kappa > 0 : t(kappa) <= sqrt(B2)} pour un point dont r = mmt_point_pond(...) (T_1/2, W,
    termes et segments de croissance ne dependent pas de kappa). Rend ('impossible', None) si sqrt(T_1/2) > sqrt(B2)
    (aucun kappa), ('toujours', QS 0) si tout kappa convient, ('seuil', tau QS) sinon. La date t(kappa) est
    decroissante en kappa : t <= B ssi kappa >= tau.
      - terme d'evenement (e, mu) avec e > B2 : kappa >= (sqrt(e) - B) / (alpha mu) ;
      - segment de croissance [lo, hi] de pente p, G(s) = m0 + p (s - s0) : sup sur u = sqrt(s) de
        W (u - B) / (alpha (2 p u^2 + c)), c = 2 m0 - 2 p s0 - W ; point interieur u* = B + sqrt(B^2 + c / (2p)),
        valeur W / (4 p alpha u*) = (W / (2c)) (sqrt((B^2 + c/(2p))/A) - sqrt(B^2/A)) (c != 0), W / (8 p alpha B) (c = 0)."""
    B2 = Fraction(B2)
    if r['T_half'] > B2:
        return 'impossible', None
    A, W = r['A'], r['W']
    B = QS.sqrt(B2)
    best = None

    def prendre(val):
        nonlocal best
        if best is None or val.cmp(best) > 0:
            best = val
    for e, mu in r['termes']:
        if e <= r['T_half'] or e <= B2:
            continue
        exiger(mu > 0, 'seuil : marge non positive')
        # (sqrt(e) - B) / (sqrt(A) mu) = (sqrt(e/A) - sqrt(B2/A)) / mu
        prendre((QS.sqrt(e / A) - QS.sqrt(B2 / A)) * (1 / mu))
    for lo, hi, s0, m0, p in r['crit']:
        if hi <= B2:
            continue
        c = 2 * m0 - 2 * p * s0 - W
        d2 = B2 + c / (2 * p)
        if d2 < 0:
            continue
        ustar = QS.sqrt(B2) + QS.sqrt(d2)
        # u* doit etre interieur a ]sqrt(lo), sqrt(hi)[ et au-dela de B
        if ustar.cmp(QS.sqrt(lo)) <= 0 or ustar.cmp(QS.sqrt(hi)) >= 0:
            continue
        if c != 0:
            val = (QS.sqrt(d2 / A) - QS.sqrt(B2 / A)) * (W / (2 * c))
        else:
            val = QS.sqrt(1 / (A * B2)) * (W / (8 * p))
        prendre(val)
    if best is None or best.sign() <= 0:
        return 'toujours', QS.rat(0)
    return 'seuil', best


def recouper_reference(T, cv, eta, kappa):
    """omega = 1 : meme date (exacte) et meme proprietaire que mmt.mmt_point. Rend le nombre de controles."""
    r1 = mmt.mmt_point(T, cv, eta, kappa, True)
    r2 = mmt_point_pond(T, {v: (c, Fraction(1)) for v, c in cv.items()}, eta, kappa)
    exiger(r1['date'].cmp(r2['date']) == 0, 'recoupe MMt ponderee : date differente')
    exiger(r1['owner'] == r2['owner'], 'recoupe MMt ponderee : proprietaire different')
    exiger(r1['T_half'] == r2['T_half'] and r1['W'] == r2['W'], 'recoupe MMt ponderee : T_1/2 ou W')
    return 3
